

#include	"calah.h"
#include	"board.h"
#include	"attr.h"


acalc( bp, ap)
board	*bp;
attr	*ap;
{
	uint	i,
		j;

	ap->mustpass = bp->mustpass;
	ap->mycalah = bp->holes[BRIGHTCALAH];
	ap->yourcalah = bp->holes[BLEFTCALAH];
	ap->mymoves = 0;
	ap->yourmoves = 0;
	ap->mycaptures = 0;
	for (i=0; i<BCALAH; ++i) {
		if (j = bp->holes[BLO+i]) {
			j += i;
			if ((j < BRIGHTCALAH)
			and (not bp->holes[BLO+j]))
				ap->mycaptures += bp->holes[BHI+j];
			++ap->mymoves;
		}
		if (bp->holes[BHI+i])
			++ap->yourmoves;
	}
	ap->mydistl = bp->holes[BLO+0] + bp->holes[BLO+1] + bp->holes[BLO+2];
	ap->mydistr = bp->holes[BLO+3] + bp->holes[BLO+4] + bp->holes[BLO+5];
	ap->yourdistl = bp->holes[BHI+0] + bp->holes[BHI+1] + bp->holes[BHI+2];
	ap->yourdistr = bp->holes[BHI+3] + bp->holes[BHI+4] + bp->holes[BHI+5];
}
