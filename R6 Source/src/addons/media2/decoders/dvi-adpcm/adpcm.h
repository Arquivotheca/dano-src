/*
** adpcm.h - include file for adpcm coder.
**
** Version 1.0, 7-Jul-92.
*/

#ifndef C_ADPCM_H

#define C_ADPCM_H

#ifdef __cplusplus
extern "C" {
#endif

struct adpcm_state {
    short	valprev;	/* Previous output value */
    char	index;		/* Index into stepsize table */
};

#ifdef __STDC__
#define ARGS(x) x
#else
#define ARGS(x) ()
#endif

void adpcm_coder ARGS((short [], char [], int, struct adpcm_state *));
void adpcm_decoder ARGS((char [], short [], int, struct adpcm_state *));

#ifdef __cplusplus
}
#endif

#endif
