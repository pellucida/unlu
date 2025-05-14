/*
//
*/
# if	!defined(LU_ERRORS_H)
# define	LU_ERRORS_H	1

# define	ok	(0)
# define	err	(-1)

char*	progname (char*);
void	fatal (char* fmt,...);
void	error (char* fmt,...);

# endif
