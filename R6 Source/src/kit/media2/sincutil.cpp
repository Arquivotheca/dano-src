#include <support2/SupportDefs.h>
#include <math.h>
#include <resampler_sinc_lowpass.h>
#include "sincutil.h"

//**************************************************************
//
// IIR filter coefficients used by resampler_sinc_lowpass
//
//**************************************************************

const float sinc_lowpass_const::m_a[] =
{
	 0.00002252604204094672,
	-0.00028427727269862470,
	 0.00419534961980524400,
	-0.02232087857921671000,
	 0.11736641198146520000,
	-0.35754008515909850000,
	 1.00620219914172400000,
	-1.87301154512256200000,
	 3.15222848812682400000,
	-3.51421148747082500000,
	 3.51681572539862000000,
	-1.95934102812001400000
};
	
const float sinc_lowpass_const::m_b[] =
{
	 0.00250046843691883600,
	 0.01242873521106720000,
	 0.03710327253088329000,
	 0.07913782958283877000,
	 0.13132798031851780000,
	 0.17583315479906510000,
	 0.19345851682745180000,
	 0.17583315479906700000,
	 0.13132798031851500000,
	 0.07913782958284132000,
	 0.03710327253088152000,
	 0.01242873521106746000,
	 0.00250046843691886800
};

//**************************************************************
//
// get_sinc_table_size
//
//**************************************************************

uint32 get_sinc_table_size
(
	uint32 *freq_in,
	uint32 *freq_out,
	uint32 sinc_taps,
	uint32 max_table_size_bytes
)
{
	uint32	table_entries;
	uint32	max_table_entries;
	uint32	round_factor;
	uint32	freq_in_temp;
	uint32	pos;

	//
	// Determine the size of and allocate the sinc table
	// 
	table_entries = 0;
	round_factor = 5;	
	max_table_entries = max_table_size_bytes / (sinc_taps * sizeof(float));
	freq_in_temp = *freq_in;
	do	
	{
		pos = freq_in_temp;
		table_entries = 1;
		while (table_entries <= max_table_entries )
		{
			if (0 == (pos % *freq_out))
				break;
				
			table_entries++;
			pos += freq_in_temp;
		}
		
		//
		// Make sure the table is not too large
		//
		if (table_entries > max_table_entries)
		{
			//
			// Round the input rate until the table is small enough
			//
			freq_in_temp = *freq_in + (round_factor >> 1);
			freq_in_temp -= freq_in_temp % round_factor;
 			
			round_factor += 5;	// increase the rounding factor in case we have to go again
		}
	
	} while (table_entries > max_table_entries);
	
	*freq_in = freq_in_temp;
	
	return (table_entries * sinc_taps);

} // get_sinc_table_size


//**************************************************************
//
// make_sinc_table
//
//**************************************************************
float *make_sinc_table
(
	uint32 	*freq_in,
	uint32 	*freq_out,
	uint32 	sinc_taps,
	uint32 	max_table_size_bytes,
	uint32 	*table_size_floats
)
{
	float *sinc_table,*temp;
	
	//
	// Figure out the size of the table
	// 
	*table_size_floats = get_sinc_table_size(freq_in,freq_out,sinc_taps,max_table_size_bytes);

	//
	// Allocate the table
	//
	sinc_table = new float[*table_size_floats];
	
	//
	// Fill out the table
	//
	uint32 pos = 0;
	uint32 mod = 0;	
	temp = sinc_table;
	
	do
	{
		double	x,phase;
		int32	i,start,stop;
		double	A,B;
		
		A = 2.0 / ((double) sinc_taps);
		B = 2.0 * A;
		
		phase = ((double) mod) / ((double) (*freq_out) );	
		
		stop = ((int32) sinc_taps) >> 1;
		start = - stop;
		
		for (i = start; i <= stop; i++)
		{
			x = (phase - (double)i) * M_PI;
			
			if (0.0 == x)
				*(temp++) = 1.0;
			else
				*(temp++) = (sin( x ) / x) * 
							(0.42 + 0.5 * cos(A * x) + 0.08 * cos(B * x ));	// Blackman window
		}
	
		pos += *freq_in;
		mod = pos % *freq_out;

	}	while (0 != mod);
	
	return sinc_table;
	
} // make_sinc_table

#if !__INTEL__
//**************************************************************
//
// lowpass_iir
//
//**************************************************************

void lowpass_iir(sinc_lowpass_state *state)
{
	uint32 i;
	size_t iir_cnt = state->out_cnt << 1;	// caller wants this many frames (need << 1 to decimate)
	size_t max = state->over_cnt - (sinc_lowpass_const::IIR_TAPS - 1);
	
	if (iir_cnt > max)
	{
		iir_cnt = max;		// but may only get this many
	}
	
	max = state->hist_size - state->hist_cnt;
	
	if (iir_cnt > max)
	{
		iir_cnt = max;
	}
	
	float *over = state->over;
	float *new_hist = state->hist + (state->hist_cnt << 1);
	float *hist = new_hist - ((sinc_lowpass_const::IIR_TAPS - 1) << 1);
	const float *a = state->a;
	const float *b = state->b;

	for (i = 0; i < iir_cnt; i++)
	{
		float s;
			
		// filter left channel over inputs
		s =  b[ 0] * over[ 0];
		s += b[ 1] * over[ 2];
		s += b[ 2] * over[ 4];
		s += b[ 3] * over[ 6];
		s += b[ 4] * over[ 8];
		s += b[ 5] * over[10];
		s += b[ 6] * over[12];						
		s += b[ 7] * over[14];
		s += b[ 8] * over[16];
		s += b[ 9] * over[18];
		s += b[10] * over[20];						
		s += b[11] * over[22];
		s += b[12] * over[24];
		
		// filter left channel over outputs
		s -= a[ 0] * hist[ 0];
		s -= a[ 1] * hist[ 2];		
		s -= a[ 2] * hist[ 4];
		s -= a[ 3] * hist[ 6];		
		s -= a[ 4] * hist[ 8];
		s -= a[ 5] * hist[10];		
		s -= a[ 6] * hist[12];
		s -= a[ 7] * hist[14];		
		s -= a[ 8] * hist[16];
		s -= a[ 9] * hist[18];		
		s -= a[10] * hist[20];
		s -= a[11] * hist[22];		

		*(new_hist++) = s;
		
		// filter right channel over inputs
		s =  b[ 0] * over[ 1];
		s += b[ 1] * over[ 3];
		s += b[ 2] * over[ 5];
		s += b[ 3] * over[ 7];
		s += b[ 4] * over[ 9];
		s += b[ 5] * over[11];
		s += b[ 6] * over[13];						
		s += b[ 7] * over[15];
		s += b[ 8] * over[17];
		s += b[ 9] * over[19];
		s += b[10] * over[21];						
		s += b[11] * over[23];
		s += b[12] * over[25];
		
		// filter right channel over outputs
		s -= a[ 0] * hist[ 1];
		s -= a[ 1] * hist[ 3];		
		s -= a[ 2] * hist[ 5];
		s -= a[ 3] * hist[ 7];		
		s -= a[ 4] * hist[ 9];
		s -= a[ 5] * hist[11];		
		s -= a[ 6] * hist[13];
		s -= a[ 7] * hist[15];		
		s -= a[ 8] * hist[17];
		s -= a[ 9] * hist[19];		
		s -= a[10] * hist[21];
		s -= a[11] * hist[23];		

		*(new_hist++) = s;

		over += 2;
		hist += 2;
	}
	
	state->hist_cnt += iir_cnt;
	
	//
	// Move the data at the end of the over buffer to the beginning
	//
	state->over_cnt -= iir_cnt;
	float *dest = state->over;
	
	for (i = 0; i < (state->over_cnt << 1); i++)
	{
		dest[i] = over[i];	
	}

} // lowpass_iir
#endif  // !__INTEL__
