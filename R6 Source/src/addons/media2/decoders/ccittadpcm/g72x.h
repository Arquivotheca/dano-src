/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use.  Users may copy or modify this source code without
 * charge.
 *
 * SUN SOURCE CODE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING
 * THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS SOFTWARE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * g72x.h
 *
 * Header file for CCITT conversion routines.
 *
 */
#ifndef _G72X_H
#define	_G72X_H

#include <SupportDefs.h>

#define	AUDIO_ENCODING_ULAW	(1)	/* ISDN u-law */
#define	AUDIO_ENCODING_ALAW	(2)	/* ISDN A-law */
#define	AUDIO_ENCODING_LINEAR	(3)	/* PCM 2's-complement (0-center) */

/*
 * The following is the definition of the state structure
 * used by the G.721/G.723 encoder and decoder to preserve their internal
 * state between successive calls.  The meanings of the majority
 * of the state structure fields are explained in detail in the
 * CCITT Recommendation G.721.  The field names are essentially indentical
 * to variable names in the bit level description of the coding algorithm
 * included in this Recommendation.
 */
struct g72x_state {
	int32	yl;		/* Locked or steady state step size multiplier. */
	int16	yu;		/* Unlocked or non-steady state step size multiplier. */
	int16	dms;	/* Short term energy estimate. */
	int16	dml;	/* Long term energy estimate. */
	int16	ap;		/* Linear weighting coefficient of 'yl' and 'yu'. */

	int16	a[2];	/* Coefficients of pole portion of prediction filter. */
	int16	b[6];	/* Coefficients of zero portion of prediction filter. */
	int16	pk[2];	/*
					 * Signs of previous two samples of a partially
					 * reconstructed signal.
					 */
	int16	dq[6];	/*
					 * Previous 6 samples of the quantized difference		
					 * signal represented in an internal floating point
					 * format.
					 */
	int16	sr[2];	/*
					 * Previous 2 samples of the quantized difference
					 * signal represented in an internal floating point
					 * format.
					 */
	char	td;		/* delayed tone detect, new in 1988 version */
};

/* External function definitions. */

void	g72x_init_stat(g72x_state *state_ptr);
int32	g721_encoder(int32 sample, int32 in_coding, g72x_state *state_ptr);
int32	g721_decoder(int32 code, int32 out_coding, g72x_state *state_ptr);
int32	g723_24_encoder(int32 sample, int32 in_coding, g72x_state *state_ptr);
int32	g723_24_decoder(int32 code, int32 out_coding, g72x_state *state_ptr);
int32	g723_40_encoder(int32 sample, int32 in_coding, g72x_state *state_ptr);
int32	g723_40_decoder(int32 code, int32 out_coding, g72x_state *state_ptr);

int32	quan(int32 val, int16 table, int32 size);
int32	fmult(int32 an, int32 srn);
void	g72x_init_state(g72x_state *state_ptr);
int32	predictor_zero(g72x_state *state_ptr);
int32	predictor_pole(g72x_state *state_ptr);
int32	step_size(g72x_state *state_ptr);
int32	quantize(int32 d, int32 y, int16 *table, int32 size);
int32	reconstruct(int32 sign, int32 dqln, int32 y);
void	update(int32 code_size, int32 y, int32 wi, int32 fi, int32 dq, int32 sr, int32 dqsez, g72x_state *state_ptr);
int32	tandem_adjust_alaw(int32 sr, int32 se, int32 y, int32 i, int32 sign, int16 *qtab);
int32	tandem_adjust_ulaw(int32 sr, int32 se, int32 y, int32 i, int32 sign, int16 *qtab);

uint8	linear2alaw(int32 pcm_val);
int32	alaw2linear(uint8 a_val);
uint8	linear2ulaw(int32 pcm_val);
int32	ulaw2linear(uint8 u_val);
uint8	alaw2ulaw(uint8 aval);
uint8	ulaw2alaw(uint8 uval);

#endif /* !_G72X_H */
