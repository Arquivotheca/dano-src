

/*
 * ("I" refers to the player on the move)
 */
typedef struct {
	bool	mustpass;	/* I must make E move */
	uint	mycalah,	/* # stones in my calah */
		yourcalah,	/* # stones in your calah */
		mymoves,	/* # moves I would have ignoring E */
		yourmoves,	/* # moves you would have, if it was your turn */
		mycaptures,	/* # of your stones vunerable to capture */
		mydistl,	/* # stones exist on my left side */
		mydistr,	/* # stones exist on my right side */
		yourdistl,	/* # stones exist on your left side */
		yourdistr;	/* # stones exist on your right side */
} attr;

#define	AFORMAT	"GTscore N mustpass B mycalah N yourcalah N mymoves N yourmoves N mycaptures N mydistl N mydistr N yourdistl N yourdistr N"
