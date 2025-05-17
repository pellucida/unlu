/*
//
//	List/Extract LU (.lbr) archives CPM/80 - MSDOS era.
*/
# include	<unistd.h>
# include	<stdio.h>
# include	<ctype.h>
# include	<stdint.h>
# include	<stdlib.h>
# include	<string.h>
# include	<stdbool.h>
# include	<time.h>
# if	!defined( USE_STRCMP)
# include	<fnmatch.h>
# endif
# include	<errno.h>

# include	"dostime.h"
# include	"dirent.h"
# include	"errors.h"
# include	"crc16.h"

// CP/M Text file EOF [Ctrl-Z] also honoured by MS-DOS(?)

# define	ZEOF	('\x1a') 

// time_t -> formatted string
static	size_t	time_strftime (char* s, size_t max, char* format, time_t time){
	size_t	result	= 0;
	struct	tm	tm_time;
	struct	tm*	tm	= gmtime (&time);
	if (tm) {
		tm_time	= *tm;
		result	= strftime (s, max, format, &tm_time);
	}
	return	result;
}

// Convert lu entries into more posixy form  
// map to lower case.
// CP/M and DOS have fairly limited file names

typedef	struct	namerec_t	{
	size_t	offset;
	size_t	size;
	char	name	[NAMEREC_LEN];
	time_t	ctime;
	time_t	mtime;
	uint16_t	crc;
}	namerec_t;

// Convert the 8+3 names in separated blank terminated format.

static	int	convert_name (char* name, size_t namesize, ludirent_t de) {
	int	result	= err;
	char*	s	= de.name;
	size_t	i	= 0;
	size_t	j	= 0;
	char	copy [NAMEREC_LEN];

	while (i < NAMELEN && s[i] != ' ') {
		copy [j]	= tolower (s[i]);
		++i; ++j;
	}
	if (j!=0) { // ignore files with only an ext.
		i	= 0;
		s	= de.ext;
		if (s [0] != ' '){
			copy [j++]	= '.';
		}
		while (i < EXTLEN && s[i] != ' ') {
			copy [j]	= tolower (s[i]);
			++i; ++j;
			}
		if (j==0) {
			copy [j++]	= '.';
		}
		copy [j++]	= '\0';
	}
	else if (de.ext [0]==' ') {
		copy [j++]	= '.';
		copy [j++]	= '\0';
	}
	if (0 < j && j<=namesize) {
		memcpy (name, copy, j);
		result	= ok;
	}
	return	result;
}

static	int	convert (namerec_t* d, ludirent_t de) {
	namerec_t	dd;
	
	int	rv	= convert_name (dd.name, sizeof(dd.name), de);
	if (rv==ok) {
		dd.offset	= de.first_sector * SECTOR;
		dd.size		= de.sectors_used * SECTOR - de.pad;
		dd.ctime	= dos_mktime (de.ctim_day, de.ctim_hms);
		dd.mtime	= dos_mktime (de.mtim_day, de.mtim_hms);
		dd.crc		= de.crc;
		*d	= dd;
	}
	return	rv;
}

// Active, name & ext all blanks, offset is 0 and used is directory size

static	int	verify_directory_control (ludirent_t dce) {
	int	result	= err;
	if (dce.status==ST_ACTIVE) {
		if (memcmp (dce.name,"        ", NAMELEN)==0 
			&& memcmp (dce.name,"   ", EXTLEN)==0) {
			if (dce.first_sector==0) {
				if (dce.sectors_used > 0) {
					result	= ok;
				}
			}
		
		}
	}
	return	result;
}

typedef	struct	args_t	{
	int	cmd; // tlx
	int	verbose;
	char*	file;
	char*	extract_dir;
	char**	members;
	size_t	nmembers;
	size_t	n_dirent;
	void*	mapped;
}	args_t;

static	int	match (args_t args, char* name) {
	int	result	= false;
	size_t	i	= 0;
	size_t	j	= args.nmembers;
	char**	pats	= args.members;
	while (i!=j) {
# if	!defined( USE_STRCMP)
		// No FNM_ flags required here.
		if (fnmatch (pats[i], name, 0)==0) {
# else
		if (strcmp (pats[i], name)==0) {
# endif
			result	= true;
			j	= i;
		}
		else	++i;
	}
	return	result;
}

static	void	do_list (args_t args, namerec_t dirent) {
	if (args.verbose) {
		char	timestamp [sizeof("1977-12-32_23:59:59")+1];
		time_strftime (timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", dirent.ctime);
		fprintf (stdout, "%s ", timestamp);

		fprintf (stdout, "% 8lu  ", dirent.size);
	}
	fprintf (stdout, "%-14.14s", dirent.name);
	fprintf (stdout, "\n");
}	

// The first entry requires special casing for CRC calculation.
// the dce.crc must be set to 0 before calculating the directory CRC
// Make a copy of the directory sectors and set CRC=0 there

static	inline	int	is_dce (ludirent_t lude){
	return	lude.first_sector==0;
}
static	uint16_t	directory_calc_crc (args_t args) {
	uint16_t	result	= 0;
	ludirent_t*	dir	= args.mapped;
	ludirent_t	dce	= dir[0];
	size_t		used	= dce.sectors_used*SECTOR;
	size_t		nents	= used/sizeof (ludirent_t);
	ludirent_t	copy [nents];
	memcpy (copy, dir, used);
	copy[0].crc	= 0;
	result	= crc16 (copy, used);
	return	result;
}
static	void	do_crc_check (args_t args, ludirent_t lude, namerec_t dirent) {
	char*	mapped	= args.mapped;
	size_t	begin	= dirent.offset;
	uint16_t	crc	= 0;
	if (is_dce (lude)) {
		crc	= directory_calc_crc (args);
	}
	else	{
		crc	= crc16 (mapped+begin,lude.sectors_used*SECTOR);
	}
		
	if (crc != dirent.crc) {
		fprintf (stdout, "[%-2.2s]  ", "XX");
	}
	else	{
		fprintf (stdout, "[%-2.2s]  ", "OK");
	}
	if (args.verbose) {
		fprintf (stdout, "[%-04.4X/%-4.4x]", dirent.crc, crc);
		char	timestamp [sizeof("1977-12-32_23:59:59")+1];
		time_strftime (timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", dirent.ctime);
		fprintf (stdout, " %s ", timestamp);
		fprintf (stdout, "% 8lu  ", dirent.size);
	}
	fprintf (stdout,"%-16.16s", dirent.name);
	fprintf (stdout,"\n");
}

static	void	do_extract (args_t args, ludirent_t lude, namerec_t dirent) {

	char*	mapped	= args.mapped;
	size_t	begin	= dirent.offset;
	size_t	size	= dirent.size;
	size_t	j	= 0;
	size_t	nonprint	= 0;
	if (!is_dce (lude)) {
		FILE*	output	= fopen (dirent.name, "w");
		if (output) {
			for (j=0; j < size-2; ++j){
				int	ch	= mapped[begin+j];
				if (isprint(ch) || isspace(ch)) {
					;
				}
				else	{
					++nonprint;
				}
				fputc (ch, output);
			}
		/* I noticed in my example lbr files the file length included two final ^Z
		   which isn't very useful on *ix. But left \r\n alone as most Linux code
		   accomodates those or can be easily postprocessed.
		*/
			if (nonprint > 2) { // non text heuristic
				fputc (mapped [begin+size-2], output);
				fputc (mapped [begin+size-1], output);
			}
			else	{	// Text file: omit the trailing ^Z
				int	ch	= mapped [begin+size-2];
				if (ch != ZEOF) {
					fputc (ch, output);
					ch	= mapped [begin+size-1];
					if (ch != ZEOF) {
						fputc (ch, output);
					}
				}
			}
			fclose (output);
		}
		else	{
			error ("[%s] couldn't open file '%s' in directory '%s' (%s)\n",__FUNCTION__,
				dirent.name, args.extract_dir, strerror (errno));
		}
	}
	if (args.verbose) {
		do_crc_check (args, lude, dirent);
	}
	else	{
		fprintf (stdout,"%-16.16s\n", dirent.name);
	}
}

// Iterate over the directory apply args.cmd viz list, test, extract
//
static	int	apply_operation (args_t args) {
	int	result	= ok;
	ludirent_t*	dir	= args.mapped; //args.dir;
	size_t	i	= 0;
	size_t	finish	= args.n_dirent;
	while (i != finish) {
		ludirent_t	lude	= dir [i];
		uint8_t	status	= lude.status;
		if (status == ST_ACTIVE) {
			namerec_t	dirent;
			if (convert (&dirent, lude) != ok) {
				error ("[%s] couldn't convert lu archive member name ('%-8.8s'(.)'%-3.3s').\n",__FUNCTION__,
					lude.name, lude.ext);
			}
			else	{
				if (args.members==0 || match (args, dirent.name)) {
					switch (args.cmd) {
					case	'l':
						do_list (args, dirent);
					break;
					case	't':
						do_crc_check (args, lude, dirent);
					break;
					case	'x':
						do_extract (args, lude, dirent);
					break;
					}
				}
			}
			++i;
		}
		else if (status == ST_DELETED) {
			++i;
		}
		else	{
			if (status != ST_UNUSED) {
				error ("[%s] expected status UNUSED got '0x02x'\n",__FUNCTION__, status);
			}
			finish	= i;
		}
	}
	return	result;
}

static	int	process (FILE* input, args_t args) {
	struct	ludirent_t	master;
	size_t	nread	= fread (&master, sizeof(master), 1, input);
	if (nread == 1) {
		if (verify_directory_control (master)==0) {

			size_t	n_dirent	= master.sectors_used*(SECTOR/sizeof(ludirent_t));
			size_t	finish	= n_dirent;
			ludirent_t*	dir	= calloc (n_dirent, sizeof(ludirent_t));
			if (dir) {
				size_t	maxsector	= 0;
				size_t	maxsector_size	= 0;

				size_t	count	= 0;
				dir [count++]	= master;
				while (count != finish) {
					struct	ludirent_t	de;
					size_t	n	= fread (&de, sizeof(de), 1, input);
					if (n==1) {
						dir [count++]	= de;
						if (de.status==ST_ACTIVE) {
							if (maxsector < de.first_sector) {
								maxsector	= de.first_sector;
								maxsector_size	= de.sectors_used;
							}
						}
					}
					else	{
						finish	= count;
					}
				}

				args.n_dirent	= n_dirent;
				if (args.cmd == 'l') {
					args.mapped	= dir;
				}
				else if (args.cmd == 'x' || args.cmd == 't') {
					char*	slurp	= 0;
					if (chdir (args.extract_dir)!=ok) {
						fatal ("[%s] couldn't change directory to '%s' (%s)\n",__FUNCTION__,
							args.extract_dir, strerror (errno));
					}

					// Slurp the whole archive into memory.
					// Max size of an archive 16Mb
					// Max offset ((2^16)-1)*2^7 + max size ((2^16)-1)*2^7
					// ie ((2^16)-1)*2^8 < 2^24 ie 16 Mb
					slurp	= calloc (maxsector+maxsector_size, SECTOR);
					if (slurp) {
						size_t	total_sectors	= maxsector + maxsector_size;
						size_t	dirsectors	= master.sectors_used;
						size_t	dirbytes	= dirsectors*SECTOR;
						size_t	filesectors	= total_sectors - dirsectors;
				
						memcpy (slurp, dir, dirbytes);
						fread (slurp + dirbytes, filesectors, SECTOR, input);
						args.mapped	= slurp;
						free (dir);
					}
					else	{
						fatal ("[%s] couldn't allocate %lu bytes for %s (%s}\n",__FUNCTION__,
							(maxsector+maxsector_size)*SECTOR, args.file);
					}
				}
				apply_operation (args);
			}
		}
	}
	else	{
		fatal ("[%s] couldn't read the directory of archive '%s' (%s).\n",__FUNCTION__,
			args.file, strerror (errno));
		}
}

char*	progname (char*);
static	void	Usage () {
	fprintf (stderr, "Usage: %s [-C dir] [-l|-x|-t][-v][-f archive] [files ...]\n", progname(0));
	exit (EXIT_FAILURE);
}

int	main (int argc, char* argv[]) {
	int	opt	= EOF;
	args_t	args	= { .cmd = 'l', .verbose = 0, .extract_dir = ".", .file = "--stdin",};
	int	t_flag	= 0;
	int	l_flag	= 0;
	int	x_flag	= 0;
	int	v_flag	= 0;
	int	C_flag	= 0;
	int	f_flag	= 0;

	progname (argv[0]);
	opterr	= 0;
	while ((opt = getopt (argc, argv, "hC:f:xtlv"))!=EOF) {
		switch (opt) {
		case	't':
			if (l_flag||x_flag||t_flag++) {
				Usage();
			}
			args.cmd	= 't';
		break;
		case	'l':
			if (x_flag||t_flag||l_flag++) {
				Usage();
			}
			args.cmd	= 'l';
		break;
		case	'x':
			if (l_flag||t_flag||x_flag++) {
				Usage();
			}
			args.cmd	= 'x';
		break;
		case	'v':
			if (v_flag++) {
				Usage();
			}
			args.verbose	= 1;
		break;
		case	'f':
			if (f_flag++) {
				Usage();
			}
			args.file	= optarg;
		break;
		case	'C':
			if (C_flag++) {
				Usage();
			}
			args.extract_dir	= optarg;
		break;
		case	'h':
		case	'?':
		default:
			Usage();
		break;
		}
	}
	if ((t_flag|l_flag|x_flag)==0) {
		Usage ();
	}
	if (optind < argc) {
		args.members	= &argv [optind];
	}
	else	{
		args.members	= 0;
	}
	args.nmembers	= argc - optind;
	
	if (f_flag) {
		FILE*	input	= fopen (args.file, "rb");
		if (input != 0) {
			process (input, args);
		}
		else	{
			fatal ("[%s] couldn't open file '%s' (%s)\n", __FUNCTION__, args.file, strerror (errno));
		}
	}
	else	process (stdin, args);
}
