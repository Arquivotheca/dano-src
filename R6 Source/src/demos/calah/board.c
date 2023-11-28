

#include	"calah.h"
#include	"board.h"


board	*
bsetup( )
{
	board	*bp;
	uint	i;

	unless (bp = malloc( sizeof *bp))
		return ((board *) nomem( ));
	memset( bp, 0, sizeof *bp);
	for (i=0; i<BCALAH; ++i) {
		bp->holes[BLO+i] = 3;
		bp->holes[BHI+i] = 3;
	}
	return (bp);
}


board	*
bdup( bp)
board	*bp;
{
	board	*new;

	unless (new = malloc( sizeof *new))
		return ((board *) nomem( ));
	*new = *bp;
	return (new);
}


bclone( dst, src)
board	*dst,
	*src;
{

	memcpy( dst, src, sizeof *dst);
}


bswap( bp)
board	*bp;
{
	uint	i;

	for (i=0; i<nel( bp->holes)/2; ++i) {
		uint j = birotate( i);
		uint t = bp->holes[i];
		bp->holes[i] = bp->holes[j];
		bp->holes[j] = t;
	}
}


binext( i)
{

	if (i < BCALAH)
		return (i + 1);
	if (i == BCALAH)
		return (BHI+BCALAH-1);
	if (i == BHI)
		return (BLO);
	return (i - 1);
}


bireflect( i)
{

	if (i < BCALAH)
		return (BHI + i);
	return (BLO + BHI - i);
}


birotate( i)
{

	if (i < BCALAH)
		return (BHI+BCALAH-1-i);
	if (i == BRIGHTCALAH)
		return (BLEFTCALAH);
	if (i < BLEFTCALAH)
		return (BLO+BCALAH-1 + BHI-i);
	return (BRIGHTCALAH);
}


bprint( bp)
board	*bp;
{

	uint	i;

	if (gflag) {
		printf( "!");
		printf( "%3d", bp->holes[BLEFTCALAH]);
		for (i=0; i<BCALAH; ++i)
			printf( "%3d", bp->holes[BHI+i]);
		printf( "%3d", bp->holes[BRIGHTCALAH]);
		for (i=0; i<BCALAH; ++i)
			printf( "%3d", bp->holes[BLO+i]);
	}
	else {
		printf( "     d   c   b   a   9   8\n");
		printf( "  +-------------------------+\n");
		printf( " / ");
		for (i=0; i<BCALAH; ++i)
			printf( " %2d ", bp->holes[BHI+i]);
		printf( "  \\\n  %2d                       %2d", bp->holes[BLEFTCALAH], bp->holes[BRIGHTCALAH]);
		printf( bp->mustpass? " (e)\n": "\n");
		printf( " \\ ");
		for (i=0; i<BCALAH; ++i)
			printf( " %2d ", bp->holes[BLO+i]);
		printf( "  /\n");
		printf( "  +-------------------------+\n");
		printf( "     1   2   3   4   5   6\n");
	}
	printf( "\n");
	fflush( stdout);
}
