/*
//	errors.c
*/
# include	<stdio.h>
# include	<stdlib.h>
# include	<string.h>
# include	<stdarg.h>

# include	"errors.h"

char*	progname (char* argv0){
	static	char*	progname_	= 0;
	if (progname_ == 0) {
		char*	t	= strrchr (argv0, '/');
		if (t)
			progname_	= t+1;
		else
			progname_	= argv0;
	}
	return	progname_;
}
void    fatal (char* fmt, ...) {
	va_list ap;
	va_start (ap, fmt);
	fprintf (stderr, "FATAL: %s - ", progname(0));
	vfprintf (stderr, fmt, ap);
	va_end (ap);
	exit (EXIT_FAILURE);
}

void    error (char* fmt, ...) {
	va_list ap;
	va_start (ap, fmt);
	fprintf (stderr, "ERROR: %s - ", progname(0));
	vfprintf (stderr, fmt, ap);
	va_end (ap);
}

