

#include	<SupportDefs.h>
#include	<stdlib.h>
#include	"rico.h"
extern "C" {
#include	"reg.h"
}
#include	"XString.h"
#include	"Finder.h"


#define	max( a, b)	((a)<(b)? (b): (a))


Finder::Finder( ushort *p, bool f1, bool f2, uint c)
{

	line = ~0;
	col = c;
	forward = f1;
	fold = f2;
	status = SEARCHING;
	unless (pattern = regcomp( p))
		status = regerror=='s'? BADSYNTAX: NOMEM;
}


Finder::~Finder( )
{

	if (pattern)
		free( pattern);
}


XString
Finder::getfindstr( XString x, bool fold, bool regex, bool word)
{
	XString	y;
	uint	w,
		wlast;

	w = x.getw( );
	if (word)
		unless ((regex)
		and (w == '^' || w == '~'))
			y.putw( '~');
	loop {
		switch (w) {
		case 0:
			if (word)
				unless ((regex)
				and (wlast == '$' || wlast == '~'))
					y.putw( '~');
			y.putw( w);
			return (y);
		case '\\':
		case '[':
		case '*':
		case '.':
		case '^':
		case '$':
		case '~':
			unless (regex)
				y.putw( '\\');
			y.putw( w);
			break;
		default:
			if ((fold)
			and ('A'<=w && w<='Z'))
				w += 'a' - 'A';
			y.putw( w);
		}
		wlast = w;
		w = x.getw( );
	}
}


bool
Finder::search( ushort *s, uint l)
{

	unless (status == SEARCHING)
		return (TRUE);
	if (fold)
		s = foldcase( s);
	unless (regfind( pattern, s)) {
		if (l == line) {
			status = FAILED;
			return (TRUE);
		}
		if (line == ~0)
			line = l;
		return (FALSE);
	}

	uint c = reglp( 0) - s;
	uint n = regrp( 0) - reglp( 0);
	if (forward) {
		if (line == ~0) {
			line = l;
			while (c <= col) {
				unless (regstep( ))
					return (FALSE);
				c = reglp( 0) - s;
				n = regrp( 0) - reglp( 0);
			}
		}
	}
	else
		if (line == ~0) {
			line = l;
			unless (c < col)
				return (FALSE);
			while ((regstep( ))
			and (reglp( 0) < s+col)) {
				c = reglp( 0) - s;
				n = regrp( 0) - reglp( 0);
			}
		}
		else if (l == line)
			while (regstep( )) {
				c = reglp( 0) - s;
				n = regrp( 0) - reglp( 0);
			}
		else
			while (regstep( )) {
				c = reglp( 0) - s;
				n = regrp( 0) - reglp( 0);
			}
	line = l;
	col = c;
	len = n;
	status = FOUND;
	return (TRUE);
}


ushort	*
Finder::foldcase( ushort *s)
{

	loop {
		uint w = *s++;
		if ('A'<=w && w<='Z')
			w += 'a' - 'A';
		x.putw( w);
		unless (w)
			return ((ushort *) x.getm( x.count( )));
	}
}
