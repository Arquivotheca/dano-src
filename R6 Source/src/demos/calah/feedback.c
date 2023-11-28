

#include	<stdio.h>
#include	"rico.h"


bool	feedback( ),
	complain( );
char	*strconcat( ),
	*strrchr( ),
	*getenv( );
void	*malloc( ),
	*realloc( );


main( argc, argv)
char	*argv[];
{

	if (argc < 2)
		return (usage( ));
	feedback( &argv[1]);
	return (1);
}


static bool
feedback( av)
char	*av[];
{
	int	fd[2];
	char	*sh,
		*av0,
		*com;
	char	s0[99],
		s1[99];

	if (pipe( fd) < 0)
		return (complain( "cannot create pipe"));
	sprintf( s0, "FEEDBACKSTDIN=%d", dup( 0));
	sprintf( s1, "FEEDBACKSTDOUT=%d", dup( 1));
	putenv( s0);
	putenv( s1);
	close( 0);
	close( 1);
	dup( fd[0]);
	dup( fd[1]);
	close( fd[0]);
	close( fd[1]);
	unless (sh = getenv( "SHELL"))
		sh = "/bin/sh";
	if (av0 = strrchr( sh, '/'))
		++av0;
	else
		av0 = sh;
	unless (com = strconcat( av))
		return (complain( "out of memory"));
	execl( sh, av0, "-c", com, (uchar *)0);
	return (complain( "cannot execute %s", sh));
}


static char	*
strconcat( sv)
char	**sv;
{
	char	*p;

	if (p = malloc( 1)) {
		strcpy( p, "");
		while ((*sv)
		and (p = realloc( p, strlen( p)+1+strlen( *sv)+1))) {
			strcat( p, " ");
			strcat( p, *sv++);
		}
	}
	return (p);
}


static bool
complain( mesg, arg0)
char	*mesg;
{

	fprintf( stderr, "feedback: ");
	fprintf( stderr, mesg, arg0);
	fprintf( stderr, "\n");
	return (FALSE);
}


static
usage( )
{

	fprintf( stderr, "usage: feedback shellcommand\n");
	return (2);
}
