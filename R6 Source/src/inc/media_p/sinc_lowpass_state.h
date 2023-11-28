// sinc_lowpass_state.h

#ifndef SINC_LOWPASS_STATE_H
#define SINC_LOWPASS_STATE_H

//************************************************************************************
//
// sinc_lowpass_state struct - holds the complete state of 
// the sinc_lowpass resampler.
//
//************************************************************************************

typedef struct 
{
	float		*over;		// buffer of oversampled data
	uint32		over_cnt;	// number of frames in over buffer

	float		*hist;		// buffer of IIR output history data
	uint32		hist_cnt; 	// number of frames stored in hist buffer
	uint32		hist_size;	// size of hist buffer in frames
	
	const float	*a;			// Pointer to "a" coefficients for IIR
	const float	*b;			// Pointer to "b" coefficients for IIR

	void		*out;
	size_t		out_cnt;
	
} sinc_lowpass_state;

#endif // SINC_LOWPASS_STATE_H
