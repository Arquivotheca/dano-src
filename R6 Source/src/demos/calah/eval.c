

#include	"calah.h"
#include	"board.h"
#include	"attr.h"


int	Var_mustpass;
double	Var_mycalah;
double	Var_yourcalah;
double	Var_mymoves;
double	Var_yourmoves;
double	Var_mycaptures;
double	Var_mydistl;
double	Var_mydistr;
double	Var_yourdistl;
double	Var_yourdistr;

double	Var_GTscore_OUT( );


#define	MEN		36


double
evaluate( bp, depth)
board	*bp;
{
	static char *dist[] = {
		"0", "1", "2"
	};
	attr		a;
	uint		mc,
			yc;
	double		score;

	++ecount;
	tick( );
	mc = bp->holes[BRIGHTCALAH];
	yc = bp->holes[BLEFTCALAH];
	if (mc > MEN/2)
		return (1);
	if (yc > MEN/2)
		return (0);
	if (mc+yc == MEN)
		return (.5);
	acalc( bp, &a);
	Var_mustpass = a.mustpass;
	Var_mycalah = a.mycalah;
	Var_yourcalah = a.yourcalah;
	Var_mymoves = a.mymoves;
	Var_yourmoves = a.yourmoves;
	Var_mycaptures = a.mycaptures;
	Var_mydistl = a.mydistl;
	Var_mydistr = a.mydistr;
	Var_yourdistl = a.yourdistl;
	Var_yourdistr = a.yourdistr;
	score = Var_GTscore_OUT( );
	if ((mc == MEN/2)
	and (score < .5))
		return (.5);
	if ((yc == MEN/2)
	and (score > .5))
		return (.5);
	return (score);
}
