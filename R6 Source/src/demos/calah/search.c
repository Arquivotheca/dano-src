

#include	"calah.h"
#include	"board.h"
#include	"eval.h"


double	searchAB( ),
	max( );


search( bp)
board	*bp;
{
	double	alpha,
		a;
	uint	bestm,
		m,
		n;
	board	b;

	m = mgenerate( bp);
	n = mnext( bp, m);
	m = mclear( m, n);
	bclone( &b, bp);
	move( &b, n);
	alpha = 1 - searchAB( &b, 0, sdepth, 0., 1.);
	bestm = n;
	while (mcount( m)) {
		n = mnext( bp, m);
		m = mclear( m, n);
		bclone( &b, bp);
		move( &b, n);
		a = 1 - searchAB( &b, 0, sdepth, 0., 1-alpha);
		if (a > alpha) {
			alpha = a;
			bestm = n;
		}
	}
	report( 1-alpha);
	return (bestm);
}


static double
searchAB( bp, depth, horizon, alpha, beta)
board	*bp;
double	alpha,
	beta;
{
	board	b;

	if (depth < horizon) {
		uint m = mgenerate( bp);
		if (mcount( m))
			while (mcount( m)) {
				uint n = mnext( bp, m);
				m = mclear( m, n);
				bclone( &b, bp);
				move( &b, n);
				alpha = max( alpha, 1 - searchAB( &b, depth+1, horizon, 1-beta, 1-alpha));
				if (alpha >= beta)
					break;
			}
		else
			alpha = evaluate( bp, depth);
	}
	else
		alpha = evaluate( bp, depth);
	return (alpha);
}


static
report( w)
double	w;
{
	static char *insults[] = {
		"Har, Har, Har!",
		"Haa!",
		"Right on!",
		"",
		"Grumble.",
		"Damn it.",
		"You scum."
	};
	int	i;

	i = w * nel( insults);
	if (i < 0)
		i = 0;
	unless (i < nel( insults))
		i = nel( insults) - 1;
	printf( "You've got a %8f win probability.  %s\n", w, insults[i]);
}


static double
max( a, b)
double	a,
	b;
{

	return (a<b? b: a);
}
