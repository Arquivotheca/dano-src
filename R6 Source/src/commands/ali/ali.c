

#include	<OS.h>
#include	<termios.h>
#include	<sys/ioctl.h>
#include	<signal.h>
#include	<string.h>
#include	"ali.h"
#include	"reg.h"


#define	SCANSIZ	256			/* scanning increment */
#define	INCR	32			/* memory allocation increment */


struct repartee {
	struct repartee	*next;
	struct regprog	*pattern;
	char		*response;
};

struct repartee	*rlist;
uint		nparam,
		lineno;
char		**params;

void	onsigalrm( );


main( argc, argv)
char	*argv[];
{
	bool	scan( );

	init( argc, argv);
	if (scan( ))
		return (0);
	return (1);
}


init( ac, av)
char	**av;
{
	struct repartee	*rp,
			*rp2;
	FILE		*f;
	uint		i;
	char		line[500],
			*p,
			*q,
			*econv( );

	if (ac < 2) {
		fprintf( stderr, "usage: ali patternfile [ scriptparam ] ...\n");
		exit( 2);
	}
	f = fopen( av[1], "r");
	if (f == NULL)
		croak( "can't open %s", av[1]);
	nparam = ac - 2;
	params = &av[2];
	while (fgets( line, sizeof line, f) != NULL) {
		++lineno;
		i = strlen( line) - 1;
		if (line[i] != '\n')
			croak( "line %d too long", lineno);
		line[i] = '\0';
		p = strchr( line, '\t');
		if (not p)
			croak( "bad line %d", lineno);
		while (*p == '\t')
			*p++ = '\0';
		rp = malloc( sizeof *rp);
		if (not rp)
			nomem( );
		rp->next = rlist;
		rlist = rp;
		q = econv( line);
		rp->pattern = regcomp( q);
		if (not rp->pattern)
			croak( "bad pattern on line %d", lineno);
		free( q);
		rp->response = econv( p);
	}
	fclose( f);
	rp2 = 0;
	while (rp = rlist) {
		rlist = rp->next;
		rp->next = rp2;
		rp2 = rp;
	}
	rlist = rp2;
	signal( SIGALRM, onsigalrm);
}


bool
scan( )
{
	struct repartee	*rp;
	char		ibuf[SCANSIZ*4];
	uint		n;
	int		i;

	n = 0;
	while (rp = rlist) {
		if (n > SCANSIZ*3) {
			copybuf( ibuf, n);
			n -= SCANSIZ;
		}
		unless (i = readbuf( ibuf+n, sizeof( ibuf)-n-1))
			break;
		n += i;
		ibuf[n] = '\0';
		do {
			if (regfind( rp->pattern, ibuf)) {
				if (streq( rp->response, "EXIT"))
					return (FALSE);
				if (streq( rp->response, "BREAK"))
					sendbreak( fileno( stdout));
				else {
					fputs( rp->response, stdout);
					fflush( stdout);
				}
				rlist = rp->next;
				n = 0;
				break;
			}
		} while (rp = rp->next);
	}
	return (TRUE);
}


char	*
econv( s)
char	*s;
{
	uint	c;
	char	*p,
		*q,
		*stashc( );

	p = malloc( INCR);
	if (not p)
		nomem( );
	*p = '\0';
	loop
		switch (c = *s++) {
		case '\0':
			return (p);
		case '$':
			c = *s++;
			if (c<='0' || c>'9')
				croak( "bad string param, line %d", lineno);
			c -= '1';
			if (c < nparam)
				for (q=params[c]; *q; ++q)
					p = stashc( p, *q);
			break;
		case '\\':
			switch (c = *s++) {
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 'b':
				c = '\b';
				break;
			case 't':
				c = '\t';
				break;
			case '\0':
				--s;
				break;
			default:
				if (isdigit( c)) {
					c -= '0';
					if (isdigit( *s)) {
						c = c*8 + *s++ - '0';
						if (isdigit( *s))
							c = c*8 + *s++ - '0';
					}
				}
			}
		default:
			p = stashc( p, c);
		}
}


char	*
stashc( p, c)
char	*p;
{
	uint	i;

	i = strlen( p) + 1;
	if (not (i%INCR)) {
		p = realloc( p, i+INCR);
		if (not p)
			nomem( );
	}
	p[i-1] = c;
	p[i] = '\0';
	return (p);
}


readbuf( ibuf, n)
char	ibuf[];
uint	n;
{
	static bool	initialized;
	char		*p;
	int		i,
			j,
			k,
			c;

	loop {
		if (not initialized) {
			initialized = TRUE;
			i = 0;
		}
		else {
			bigtime_t s = system_time( );
			loop {
				i = ioctl( 0, TCWAITEVENT, 0);
				if (i < 0)
					return (0);
				if (i) {
					i = read( 0, ibuf, min( i, n));
					if (i <= 0)
						return (0);
					break;
				}
				if (s+3000000 < system_time( ))
					break;
			}
		}
		unless (i) {
			ibuf[0] = '@';
			i = 1;
		}
		j = 0;
		k = 0;
		while (j < i)
			if (c = toascii( ibuf[j++]))
				ibuf[k++] = c;
		if (k) {
			write( 2, ibuf, k);
			return (k);
		}
	}
}


copybuf( dst, n)
char	*dst;
uint	n;
{
	char	*src,
		*end;

	src = dst + SCANSIZ;
	end = dst + n;
	while (src < end)
		*dst++ = *src++;
}


sendbreak( fd)
{

	ioctl( fd, TCSBRK, 0);
}


void
onsigalrm( )
{

	signal( SIGALRM, onsigalrm);
	alarm( 1);
}


bool
syntax( )
{

	return (FALSE);
}


bool
nomem( )
{

	croak( "no mem");
}


croak( mesg, arg0, arg1)
char	*mesg;
{

	fflush( stdout);
	fprintf( stderr, "ali: ");
	fprintf( stderr, mesg, arg0, arg1);
	fprintf( stderr, "\n");
	exit( 1);
}
