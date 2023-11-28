#include <fcntl.h>  
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

// This is an implementation of the RLE PackBits algorithm extended
// to handle runs longer that 127 bytes.


#define	MAXINBYTES	127
#define	INITIAL		0
#define	LITERAL		1
#define	UNDECIDED	2

char CalcRaw (short n, char *lpIn, char *rawrunbuf)
{
	char ncounts = 0;
	char thisbyte;
	char cnt = 1;
	char runbyte = *lpIn++;

	while (--n) {
		thisbyte = *lpIn++;
		if (thisbyte == runbyte) {
			cnt++;
		} else { /* write prev raw run, & start a new one */
			*rawrunbuf++ = cnt;
			ncounts++;
			cnt = 1;
			runbyte = thisbyte;
		}
	}
	*rawrunbuf = cnt;
	return (++ncounts);
}

/* this should be a Mac ToolBox lookalike
 * n must be <= 127
 */

void PackBits (char **plpIn, char **plpOut, short n)
{
	char	*lpIn = *plpIn;
	char	*lpOut = *plpOut;
	char	runcount = 0;
	char	rawrunbuf[MAXINBYTES];
	char	*pRaw;
	char	nraw;
	char	state;
	char	rawcount;
	char	twins;
	int		err = 0;

	if (n <= 0 || n > 127) {
		err = -1;
		goto cu0;
	}

	/* calculate raw run counts
	 */
	nraw = CalcRaw (n, lpIn, rawrunbuf);
	if (nraw <= 0 || nraw > 127) {
		err = -1;
		goto cu0;
	}

	/* initialize a few things
	 */
	pRaw = rawrunbuf;
	state = INITIAL;

	/* go through the raw run count array
	 */
	while (nraw--) {

		rawcount = *pRaw++;

		if (rawcount < 1 || rawcount > 127) {
			err = -1;
			goto cu0;
		}

		if (state == INITIAL) {
			if (rawcount == 1) {
				state = LITERAL;
				runcount = 1;
			} else if (rawcount == 2) {
				state = UNDECIDED;
				runcount = 2;
			} else {	/* rawcount >= 3 */
				/* state = INITIAL; */
				/* write replicate run and update ptrs
				 */
				*lpOut++ = -(rawcount - 1);
				*lpOut++ = *lpIn;
				lpIn += rawcount;
			}

		} else if (state == LITERAL) {
			if (rawcount < 3) {
				runcount += rawcount;
			} else {
				state = INITIAL;
				/* write literal run and update ptrs
				 */
				*lpOut++ = runcount - 1;
				if (runcount < 1 || runcount > 127) {
					goto cu0;
				}
				memcpy (lpOut, lpIn, runcount);
				lpOut += runcount;
				lpIn += runcount;
				/* write replicate run and update ptrs
				 */
				*lpOut++ = -(rawcount - 1);
				*lpOut++ = *lpIn;
				lpIn += rawcount;
			}

		} else {	/* state = UNDECIDED */
			if (rawcount == 1) {
				state = LITERAL;
				runcount++;
			} else if (rawcount == 2) {
				/* state = UNDECIDED */
				runcount += 2;
			} else {	/* rawcount >= 3 */
				state = INITIAL;
				if (runcount < 1 || runcount > 127) {
					goto cu0;
				}
				/* write out runcount/2 twin replicate runs */
				for (twins = (runcount>>1); twins--; ) {
					*lpOut++ = -1;
					*lpOut++ = *lpIn;
					lpIn += 2;
				}
				/* write out this replicate run
				 */
				*lpOut++ = -(rawcount - 1);
				*lpOut++ = *lpIn;
				lpIn += rawcount;
			}
		} /* end of UNDECIDED case */

	} /* end of main for loop */

	/* clean up hanging states
	 */
	if (state == LITERAL) {
		if (runcount < 1 || runcount > 127) {
			goto cu0;
		}
		/* write out literal run
		 */
		*lpOut++ = runcount - 1;
		memcpy (lpOut, lpIn, runcount);
		lpOut += runcount;
		lpIn += runcount;
	}
	else if (state == UNDECIDED) {
		if (runcount < 1 || runcount > 127) {
			goto cu0;
		}
		/* write out runcount/2 twin replicate runs
		 */
		for (twins = (runcount>>1); twins--; ) {
			*lpOut++ = -1;
			*lpOut++ = *lpIn;
			lpIn += 2;
		}
	}

	/* set up return values
	 */
	*plpIn = lpIn;
	*plpOut = lpOut;

cu0: return;

} /* that's all, folks */


/* if you might have more than 127 input bytes, call this routine instead
 * of the basic PackBits
 */

void BigPackBits (char **plpIn, char **plpOut, long	n)
{
		short	topack;

		while (n) {
			topack = (n < MAXINBYTES) ? n : MAXINBYTES;
			PackBits (plpIn, plpOut, topack);
			n -= topack;
		}
}

int	compress(char *outrow, char *inrow, long bytes)
{
	char	*tmp_out;

	tmp_out = outrow;

	BigPackBits(&inrow, &tmp_out, bytes);
	return(tmp_out-outrow);
}
