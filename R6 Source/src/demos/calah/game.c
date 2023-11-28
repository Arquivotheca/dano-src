

#include	"calah.h"
#include	"board.h"


bool	humanplay( ),
	machineplay( );


bool
game( )
{
	board	*bp;

	unless (bp = bsetup( ))
		return (FALSE);
	while ((humanplay( bp))
	and (machineplay( bp)))
		;
	if (feof( stdin))
		return (FALSE);
	showresults( bp);
	return (TRUE);
}


static
showresults( bp)
board	*bp;
{
	uint	rcalah,
		lcalah,
		i;

	for (i=0; i<BCALAH; ++i) {
		bp->holes[BRIGHTCALAH] += bp->holes[BLO+i];
		bp->holes[BLO+i] = 0;
		bp->holes[BLEFTCALAH] += bp->holes[bireflect( i)];
		bp->holes[bireflect( i)] = 0;
	}
	rcalah = bp->holes[BRIGHTCALAH];
	lcalah = bp->holes[BLEFTCALAH];
	if (rcalah < lcalah)
		printf( "Game over -- you lost %d-%d.\n", rcalah, lcalah);
	else if (rcalah > lcalah)
		printf( "Game over -- you won %d-%d.\n", rcalah, lcalah);
	else
		printf( "The game is a draw.\n");
}


static bool
humanplay( bp)
board	*bp;
{
	char	line[80];
	uint	m;

	bprint( bp);
	m = mgenerate( bp);
	switch (mcount( m)) {
	case 0:
		return (FALSE);
	case 1:
		if ((mempty( m))
		or (mfini( m))) {
			move( bp, m);
			return (TRUE);
		}
	default:
		loop {
			printf( "Please enter your move.\n> ");
			fflush( stdout);
			unless (fgets( line, sizeof line, stdin))
				return (FALSE);
			printf( "\n");
			if (line[0] == 'd')
				sdepth = atoi( line+1);
			else {
				m = mscan( line);
				if ((mcount( m))
				and (mlegal( bp, m))) {
					printf( "You move");
					mprint( m);
					printf( "\n");
					move( bp, m);
					return (TRUE);
				}
				printf( "illegal move (try one of");
				mprint( mgenerate( bp));
				printf( ")\n");
			}
		}
	}
}


static bool
machineplay( bp)
board	*bp;
{
	uint	m,
		n;
	int	r;

	bswap( bp);
	bprint( bp);
	bswap( bp);
	m = mgenerate( bp);
	unless (n = mcount( m)) {
		bswap( bp);
		return (FALSE);
	}
	if (n > 1) {
		tickstart( );
		m = search( bp);
		tickreport( );
	}
	if (mempty( m))
		printf( "You have another move.\n");
	else unless (mfini( m)) {
		printf( "I move");
		mprintother( m);
		printf( ".\n");
	}
	move( bp, m);
	return (TRUE);
}
