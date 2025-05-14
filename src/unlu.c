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
# include	<time.h>
# include	<errno.h>

# include	"dostime.h"
# include	"dirent.h"
# include	"errors.h"
# include	"crc16.h"

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

// Convert lu entries into more posixy form viz 

typedef	struct	namerec_t	{
	size_t	offset;
	size_t	size;
	int	attribute;
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
		dd.attribute	= status_of (de);
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
	size_t	n_dirent;
	void*	mapped;
}	args_t;

int	directory_list (args_t args) {
	int	result	= ok;
	ludirent_t*	dir	= args.mapped; //args.dir;
	size_t	i	= 0;
	size_t	finish	= args.n_dirent;
	while (i != finish) {
		ludirent_t	lude	= dir [i];
		if (lude.status != ST_UNUSED) {
			namerec_t	dirent;
			if (convert (&dirent, lude) != ok) {
				error ("[%s] couldn't convert lu archive member name ('%-8.8s'(.)'%-3.3s').\n",__FUNCTION__,
					lude.name, lude.ext);
			}
			else	{
				if (args.verbose) {
					char	timestamp [sizeof("1977-12-32_23:59:59")+1];
					time_strftime (timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", dirent.ctime);
					printf ("%c", dirent.attribute);
					printf (" %s ", timestamp);
					
					printf ("% 8lu", dirent.size);
					// printf ("[%4x]", dirent.crc);
				}
				printf ("\t%-14.14s", dirent.name);
				printf ("\n");
			}
			++i;
		}
		else	finish	= i;
	}
	return	result;
}
int	directory_test (args_t args) {
	int	result	= ok;
	ludirent_t*	dir	= args.mapped; //args.dir;
	size_t	i	= 1;	// skip '.'
	size_t	finish	= args.n_dirent;
	while (i != finish) {
		char*	mapped	= args.mapped;
		ludirent_t	lude	= dir [i];
		if (lude.status != ST_UNUSED) {
			namerec_t	dirent;
			int	rv	= convert (&dirent, lude);
			size_t	begin	= dirent.offset;
			uint16_t	crc	= crc16 (mapped+begin,lude.sectors_used*SECTOR);
			fprintf (stdout,"%-16.16s", dirent.name);
			if (crc != dirent.crc) {
				fprintf (stdout, "\tchecksum differs.");
			}
			else	{
				fprintf (stdout, "\tok.");
			}
			fprintf (stdout,"\n");
			i++;
		}
		else	finish	= i;
	}
	return	result;
}
int	directory_extract (args_t args) {
	int	result	= ok;
	ludirent_t*	dir	= args.mapped; //args.dir;
	size_t	i	= 1;	// skip '.'
	size_t	finish	= args.n_dirent;
	while (i != finish) {
		char*	mapped	= args.mapped;
		ludirent_t	lude	= dir [i];
		if (lude.status != ST_UNUSED) {
			namerec_t	dirent;
			int	rv	= convert (&dirent, lude);
			if (rv==ok) {
				size_t	begin	= dirent.offset;
				size_t	size	= dirent.size;
				size_t	j	= 0;
				size_t	nonprint	= 0;
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
					if (nonprint > 2) { // non text heuristic
						fputc (mapped [begin+size-2], output);
						fputc (mapped [begin+size-1], output);
					}
					else	{
						int	ch	= mapped [begin+size-2];
						if (ch != '\x1a') {
							fputc (ch, output);
							ch	= mapped [begin+size-1];
							if (ch!='\x1a') {
								fputc (ch, output);
							}
						}
					}
					fclose (output);
					if (args.verbose) {
						uint16_t	crc	= crc16 (mapped+begin,lude.sectors_used*SECTOR);
						fprintf (stdout,"%s", dirent.name);
						if (crc != dirent.crc) {
							fprintf (stdout, "\tchecksum differs.");
						}
						fprintf (stdout,"\n");
					}
				}
				else	{
					error ("[%s] couldn't open file '%s' in directory '%s' (%s)\n",__FUNCTION__,
						dirent.name, args.extract_dir, strerror (errno));
				}
			}
			else	{
				error ("[%s] couldn't convert lu archive member name ('%-8.8s'(.)'%-3.3s').\n",__FUNCTION__,
					lude.name, lude.ext);
			}
			i++;
		}
		else	finish	= i;
	}
	return	result;
}

int	process (FILE* input, args_t args) {
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
					directory_list (args);
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
					if (args.cmd=='x')
						directory_extract (args);
					else if (args.cmd=='t')
						directory_test (args);
				}
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
	fprintf (stderr, "Usage: %s [-C dir] [-l|-x|-t][-v][-f file]\n", progname(0));
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
