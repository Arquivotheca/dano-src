

#define	BCALAH		6

/*
 * indices for for board.holes
 *
 *	BHI+0 BHI+1 BHI+2 ... BHI+BCALAH-3 BHI+BCALAH-2 BHI+BCALAH-1
 *
 * BLEFTCALAH							BRIGHTCALAH
 *
 *	BLO+0 BLO+1 BLO+2 ... BLO+BCALAH-3 BLO+BCALAH-2 BLO+BCALAH-1
 */
#define	BLO		0			/* left-most on lo side */
#define	BHI		(BCALAH + 1)		/* left-most on hi side */
#define	BRIGHTCALAH	BCALAH			/* calah of lo side*/
#define	BLEFTCALAH	(2*BCALAH + 1)		/* calah of hi side  */

typedef struct board {
	struct board	*sibling,
			*children;
	uchar		holes[2*BCALAH+2];
	float		winprob;
	uint		move;
	bool		mustpass;
} board;


board	*bdup( ),
	*bsetup( );
