

#include	<stdarg.h>
#include	"calah.h"


typedef	long long	tick_t;


static tick_t	tcount,
		lastt;
bool		gflag;
uint		sdepth,
		ecount;
bool		query( ),
		help( ),
		usage( );
tick_t		system_time( );


main( argc, argv)
char	*argv[];
{

	unless (init( argv))
		return (done( FALSE));
	loop {
		game( );
		unless (query( "Another game? "))
			return (done( TRUE));
	}
}


static
init( av)
char	**av;
{
	char	*p;

	++av;
	while ((p = *av++)
	and (*p == '-'))
		while (*++p)
			switch (*p) {
			case 'g':
				gflag = TRUE;
				break;
			case 's':
				unless (*av)
					return (usage( ));
				sdepth = atoi( *av++);
				break;
			case 'h':
				return (help( ));
			default:
				return (usage( ));
			}
	if (p)
		return (usage( ));
	printf( "Using a search depth of %u.\n", sdepth);
	return (TRUE);
}


static
done( ok)
bool	ok;
{

	return (ok? 0: 1);
}


static bool
query( )
{
	char	line[10];

	loop {
		printf( "Another game? ");
		fflush( stdout);
		line[0] = 0;
		fgets( line, sizeof line, stdin);
		printf( "\n");
		switch (line[0]) {
		case 'y':
		case 'Y':
			return (TRUE);
		case 0:
		case 'n':
		case 'N':
			return (FALSE);
		}
	}
}


tickstart( )
{

	ecount = 0;
	tcount = 0;
	lastt = system_time( );
}


tick( )
{

	if ((gflag)
	and (lastt+1000000 < system_time( )))
		tickreport( );
}


tickreport( )
{
	char	line[10];

	if (gflag) {
		tick_t t = system_time( );
		tcount += t - lastt;
		printf( "tick %10u %.f\n", ecount, (double)tcount);
		fflush( stdout);
		unless (fgets( line, sizeof line, stdin))
			exit( 1);
		lastt = t;
	}
}


bool
nomem( )
{

	return (complain( "out of memory"));
}


bool
complain( char *mesg, ...)
{
	va_list	ap;

	va_start( ap, mesg);
	fprintf( stderr, "calah: ");
	vfprintf( stderr, mesg, ap);
	fprintf( stderr, "\n");
	return (FALSE);
}


static bool
help( )
{

	fprintf( stderr, "-g	augment i/o for GUI use\n");
	fprintf( stderr, "-s d	limit tree search to depth `d'\n");
	return (FALSE);
}


static bool
usage( )
{

	fprintf( stderr, "usage: calah [ -help ] [ -g ] [ -s depth ]\n");
	return (FALSE);
}
