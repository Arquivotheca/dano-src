//----------------------------------------------------------------------
//
// sincutil.h
//
//----------------------------------------------------------------------

#include <sinc_lowpass_state.h>

uint32 get_sinc_table_size
(
	uint32 *freq_in,
	uint32 *freq_out,
	uint32 sinc_taps,
	uint32 max_table_size_bytes
);

float *make_sinc_table
(
	uint32 	*freq_in,
	uint32 	*freq_out,
	uint32 	sinc_taps,
	uint32 	max_table_size_bytes,
	uint32 	*table_size_floats
);

#if __INTEL__

extern "C" void lowpass_iir(sinc_lowpass_state *state);

#else

void lowpass_iir
(
	sinc_lowpass_state *state
);

#endif
