//	resampler_sinc.h
//	Class to implement sinc resampler

/* 0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0

	OK, now this one is a completely different sort of beast.
	
	This interpolator uses the sinc function.  The sinc function is described as:
	
	sinc(x) = 1.0						if x = 1.0
			  sin(pi * x) / (pi * x)	for all other values of x
			  
	The sinc function is the impulse response of an ideal low-pass filter.  Physically,
	this function is a much better representation of what real audio data does.  The
	other functions used in these resamplers are just useful shortcuts; they bear no
	real relation to the physics of the situation.
	
	Tbe problem with using the sinc function is that it is infinite in extent.  Since
	this code should use less than an infinite number of cycles per sample, the 
	strategy here is to only go out so far on either side of x = 0.  The further away 
	you go from x = 0, the more accurate the results but the more multiples you have
	to do.  A Blackman window is then applied to these results.
	
	The other problem is that the sinc function is expensive to evaluate all by itself;
	it's a sine function followed by a divide.  The solution used here, therefore, is
	to precompute all the sinc function values needed and put them in a lookup table.
	
	The functions used to actually build the sinc lookup table may be found in 
	sincutil.cpp.
	
  0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0 */

#ifndef _RESAMPLER_SINC_H_
#define _RESAMPLER_SINC_H_

#include <math.h>
#include <string.h>
#include <resampler.h>
#include <sincutil.h>

//************************************************************************************
//
// Class to store constants
//
//************************************************************************************

class resampler_sinc_const
{

public:

	enum
	{
		SINC_TAPS			= 17,
		MAX_SINC_TABLE_SIZE = 65536,
		MAX_INPUT_FRAMES	= 256,
		MAX_INPUT_BUFFER	= (MAX_INPUT_FRAMES *2) + 6,
		MAX_OUTPUT_FRAMES	= 4096
	};

};

//************************************************************************************
//
// sinc_state struct - holds the complete state of the sinc interpolator.  
//
//************************************************************************************

typedef struct 
{
	uint32		freq_in;
	uint32		freq_out;
	
	uint32		sinc_taps;
	float		*sinc_table;
	float		*curr_entry;			// current entry for the sinc table
	float		*sinc_table_end;
	uint32		bytes_per_table_entry;	// = sinc_taps * sizeof(float)

	int32		i_xd;

	float		*in;
	size_t		in_cnt;					// # of frames in the in_fl buffer
	void		*out;
	size_t		out_cnt;
	
} sinc_state;


//************************************************************************************
//
// Public constants for the sinc interpolator
//
//************************************************************************************

enum 
{
};


//************************************************************************************
//
// Generic C++ resampler functions
//
//************************************************************************************

//-------------------------------------------------------------------------------------
//
// Resampler without mixing
//
//-------------------------------------------------------------------------------------

template <class SampOut> void sinc(sinc_state *state)
{
	SampOut *out 		= (SampOut *) state->out;
	const float *in		= state->in;
	size_t out_cnt 		= state->out_cnt;
	size_t in_cnt 		= state->in_cnt;
	int32 i_xd 			= state->i_xd;
	int32 freq_out		= state->freq_out;	
	int32 freq_in		= state->freq_in;	
	float *tmp			= state->curr_entry;
	float *end 			= state->sinc_table_end;
	float left,right;
	size_t i;
	
	while (in_cnt > (resampler_sinc_const::SINC_TAPS - 1) && out_cnt > 0)
	{
		//
		// Make new output samples as long as xd doesn't move to a new input samplex
		// 
		while (i_xd > 0 && out_cnt > 0)
		{
			left = in[0] * tmp[0];
			
			left += in[2] * tmp[1];
			left += in[4] * tmp[2];
			left += in[6] * tmp[3];				
			left += in[8] * tmp[4];								
			left += in[10] * tmp[5];
			left += in[12] * tmp[6];
			left += in[14] * tmp[7];				
			left += in[16] * tmp[8];								
			left += in[18] * tmp[9];
			left += in[20] * tmp[10];
			left += in[22] * tmp[11];				
			left += in[24] * tmp[12];								
			left += in[26] * tmp[13];
			left += in[28] * tmp[14];
			left += in[30] * tmp[15];
			left += in[32] * tmp[16];

			*(out++) = sample_conversion<float,SampOut>::convert(left, 1.0f);
					
			right = in[1] * tmp[0];

			right += in[3] * tmp[1];
			right += in[5] * tmp[2];
			right += in[7] * tmp[3];				
			right += in[9] * tmp[4];								
			right += in[11] * tmp[5];
			right += in[13] * tmp[6];
			right += in[15] * tmp[7];				
			right += in[17] * tmp[8];								
			right += in[19] * tmp[9];
			right += in[21] * tmp[10];
			right += in[23] * tmp[11];				
			right += in[25] * tmp[12];								
			right += in[27] * tmp[13];
			right += in[29] * tmp[14];
			right += in[31] * tmp[15];				
			right += in[33] * tmp[16];								
		
			*(out++) = sample_conversion<float,SampOut>::convert(right, 1.0f);
			
			tmp += resampler_sinc_const::SINC_TAPS;
			if (tmp >= end)
				tmp = state->sinc_table;
				
			i_xd -= freq_in;
			
			out_cnt--;
		}

		//
		// Skip ahead to the next input sample position
		// 
		while (i_xd <= 0 && in_cnt > (resampler_sinc_const::SINC_TAPS - 1)) 
		{
			in += 2;
			in_cnt--;
			
			i_xd += freq_out;
		}

	}	// loop without mixing
	
	//
	// Save the state of things
	//
	state->in_cnt = in_cnt;
	state->out_cnt = out_cnt;
	state->curr_entry = tmp;
	state->i_xd = i_xd;

	//
	// At this point, in_cnt is the # of frames at the end of the in buffer
	// Move any remaining frames to the top of the buffer
	// 	
	tmp = state->in;
	for (i = 0; i < (in_cnt << 1); i++)
	{
		tmp[i] = in[i];
	}

} // sinc


//-------------------------------------------------------------------------------------
//
// Resampler with mixing
//
//-------------------------------------------------------------------------------------

template <class SampOut> void sinc_mix(sinc_state *state)
{
	SampOut *out 		= (SampOut *) state->out;
	const float *in		= state->in;
	size_t out_cnt 		= state->out_cnt;
	size_t in_cnt 		= state->in_cnt;
	int32 i_xd 			= state->i_xd;
	int32 freq_out		= state->freq_out;	
	int32 freq_in		= state->freq_in;	
	float *tmp			= state->curr_entry;
	float *end 			= state->sinc_table_end;
	float left,right;
	size_t i;
		
	while (in_cnt > (resampler_sinc_const::SINC_TAPS - 1) && out_cnt > 0)
	{
		//
		// Make new output samples as long as xd doesn't move to a new input samplex
		// 
		while (i_xd > 0 && out_cnt > 0)
		{
			left = in[0] * tmp[0];
			
			left += in[2] * tmp[1];
			left += in[4] * tmp[2];
			left += in[6] * tmp[3];				
			left += in[8] * tmp[4];								
			left += in[10] * tmp[5];
			left += in[12] * tmp[6];
			left += in[14] * tmp[7];				
			left += in[16] * tmp[8];								
			left += in[18] * tmp[9];
			left += in[20] * tmp[10];
			left += in[22] * tmp[11];				
			left += in[24] * tmp[12];								
			left += in[26] * tmp[13];
			left += in[28] * tmp[14];
			left += in[30] * tmp[15];
			left += in[32] * tmp[16];

			*(out++) += sample_conversion<float,SampOut>::convert(left, 1.0f);
					
			right = in[1] * tmp[0];

			right += in[3] * tmp[1];
			right += in[5] * tmp[2];
			right += in[7] * tmp[3];				
			right += in[9] * tmp[4];								
			right += in[11] * tmp[5];
			right += in[13] * tmp[6];
			right += in[15] * tmp[7];				
			right += in[17] * tmp[8];								
			right += in[19] * tmp[9];
			right += in[21] * tmp[10];
			right += in[23] * tmp[11];				
			right += in[25] * tmp[12];								
			right += in[27] * tmp[13];
			right += in[29] * tmp[14];
			right += in[31] * tmp[15];				
			right += in[33] * tmp[16];								
		
			*(out++) += sample_conversion<float,SampOut>::convert(right, 1.0f);
			
			tmp += resampler_sinc_const::SINC_TAPS;
			if (tmp >= end)
				tmp = state->sinc_table;

			i_xd -= freq_in;
			
			out_cnt--;
		}

		//
		// Skip ahead to the next input sample position
		// 
		while (i_xd <= 0 && in_cnt > (resampler_sinc_const::SINC_TAPS - 1)) 
		{
			in += 2;
			in_cnt--;
			
			i_xd += freq_out;
		}

	}	// loop without mixing
	
	//
	// Save the state of things
	//
	state->in_cnt = in_cnt;
	state->out_cnt = out_cnt;
	state->curr_entry = tmp;
	state->i_xd = i_xd;

	//
	// At this point, in_cnt is the # of frames at the end of the in buffer
	// Move any remaining frames to the top of the buffer
	// 	
	tmp = state->in;
	for (i = 0; i < (in_cnt << 1); i++)
	{
		tmp[i] = in[i];
	}

} // sinc_mix


//************************************************************************************
//
// Wrapper templates for resampler functions
//
//************************************************************************************

typedef void (*sinc_proc)(sinc_state *state);

template <class SampOut> class sinc_info 
{
	public:
	
	static const sinc_proc	proc = &sinc <SampOut> ;
	static const sinc_proc	proc_mix = &sinc_mix <SampOut>;
};


#if __INTEL__&&SRC_ASM_OPTIMIZED

//-------------------------------------------------------------------------------------
//
// assembly function prototypes
//
//-------------------------------------------------------------------------------------

extern "C" void sinc_float_to_float(sinc_state *state);
extern "C" void sinc_float_to_float_mix(sinc_state *state);


//-------------------------------------------------------------------------------------
//
// assembly template wrapper
//
//-------------------------------------------------------------------------------------

template <> class sinc_info <float>
{
	public:

	static const sinc_proc	proc = &sinc_float_to_float;	
	static const sinc_proc	proc_mix = &sinc_float_to_float_mix;
};

#endif // __INTEL__&&SRC_ASM_OPTIMIZED


//************************************************************************************
//
// Resampler template
//
//************************************************************************************

template <class SampIn, class SampOut>
class resampler_sinc : public _resampler_base, public resampler_sinc_const
{

private:

	uint32				m_table_size;
	float				m_in_fl[MAX_INPUT_BUFFER];	// floating point copy of input buffer

	int32 				m_channels;

	sinc_state			m_state;	
	
	sinc_proc			m_proc;
	sinc_proc			m_proc_mix;
	block_convert_proc	m_convert;
	

public:
	resampler_sinc(int32 freq_in,int32 freq_out,int channels); 
	~resampler_sinc();
	void clear();
	virtual void resample_gain(
			const void * 	in,
			size_t & 		in_cnt,
			void * 			out,
			size_t & 		out_cnt,
			const float *	gain,
			bool 			mix);
};


//-------------------------------------------------------------------------------------
//
// Constructor
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
resampler_sinc<SampIn,SampOut>::resampler_sinc
(
	int32 freq_in,
	int32 freq_out,
	int channels
) : m_channels(channels)
{
	assert(m_channels == 1 || m_channels == 2);

#ifdef SRC_PROFILE
	m_freq_in = freq_in;
	m_freq_out = freq_out;
#endif

	m_state.freq_in = freq_in;
	m_state.freq_out = freq_out;
	
	m_proc = sinc_info<SampOut>::proc;
	m_proc_mix = sinc_info<SampOut>::proc_mix;
	
	m_state.in = m_in_fl;
	
	if (m_channels == 1)
	{
		m_convert = block_convert_info<SampIn>::mono;
	}
	else
	{
		m_convert = block_convert_info<SampIn>::stereo;
	}

	//
	// Make the sinc table
	//
	m_state.sinc_taps = SINC_TAPS;
	m_state.sinc_table = make_sinc_table(	&(m_state.freq_in),
											&(m_state.freq_out),
											SINC_TAPS,
											MAX_SINC_TABLE_SIZE,
											&m_table_size);
	m_state.sinc_table_end = m_state.sinc_table + m_table_size;
	m_state.bytes_per_table_entry = SINC_TAPS * sizeof(float);

	clear();

	SRC_HOWDY(("New sinc interpolator, %ld to %ld\n",freq_in,freq_out));
	
} // constructor


//-------------------------------------------------------------------------------------
//
// Destructor
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
resampler_sinc<SampIn,SampOut>::~resampler_sinc()
{
	delete m_state.sinc_table;

} // destructor


//-------------------------------------------------------------------------------------
//
// clear
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_sinc<SampIn,SampOut>::clear()
{
	m_state.curr_entry = m_state.sinc_table;
	m_state.i_xd = m_state.freq_out;

	memset(m_in_fl,0,((SINC_TAPS - 1) << 1) * sizeof(float));
		
	m_state.in_cnt = SINC_TAPS - 1;
		
	_resampler_base::clear();

} // clear


//-------------------------------------------------------------------------------------
//
// resample_gain
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_sinc<SampIn,SampOut>::resample_gain
(
	const void * 	in,
	size_t & 		in_cnt,
	void * 			out,
	size_t & 		out_cnt,
	const float *	gain,
	bool 			mix
)
{
	size_t	out_cnt_temp = out_cnt;
	float	*in_fl;
	uint32	i;
	uint32	in_fl_copy;			// number of frames copied to in_fl buffer	
	
	SRC_PROFILE_START(out_cnt);
	
	//
	// Clamp the number of output frames so that the calculation of in_fl_copy below
	// doesn't overflow 32 bit int
	//
	if (out_cnt > MAX_OUTPUT_FRAMES)
	{
		out_cnt_temp = MAX_OUTPUT_FRAMES;
	}

	//-----------------------------------------------------------------------------------------
	//
	// Convert and scale the input samples into the in_fl buffer.  This code is
	// set up to take advantage of the Intel assembly library routines for this sort of
	// thing, but should build and run correctly without those routines.
	//
	//-----------------------------------------------------------------------------------------		

	if (in_cnt < 4)
	{
		SampIn 	*in_tmp = (SampIn *) in;
		
		//-----------------------------------------------------------------------------------------
		//
		// The assembly routines work on multiples of four frames.  If the input buffer 
		// containes fewer than four frames, copy these frames the old-fashioned way
		// and return
		//
		//-----------------------------------------------------------------------------------------		
		in_fl = m_in_fl + (m_state.in_cnt << 1);
		
		if (2 == m_channels)
		{
			for (i = 0; i < in_cnt; i++)
			{
				*(in_fl++) = sample_conversion<SampIn, float>::convert(*(in_tmp++), gain[0]);
				*(in_fl++) = sample_conversion<SampIn, float>::convert(*(in_tmp++), gain[1]);
			}
		
		}
		else
		{
			float	sample;
			
			for (i = 0; i < in_cnt; i++)
			{
				sample = sample_conversion<SampIn, float>::convert(*(in_tmp++));
	
				*(in_fl++) = sample * gain[0];
				*(in_fl++) = sample * gain[1];
			}
		}

		m_state.in_cnt += in_cnt;
		in_cnt = 0;
	}
	else
	{	
		//----------------------------------------------------------------------------------
		//
		// Convert, scale, and copy input samples into temp buffer
		//
		//---------------------------------------------------------------------------------- 
	
		//
		// determine # of input samples needed according to # of output samples wanted
		//
		in_fl_copy = ((out_cnt_temp * m_state.freq_in + m_state.i_xd + (m_state.freq_out - 1)) 
						/ m_state.freq_out) 
					+ (SINC_TAPS - 1);
		
		//
		// less any samples already in the in_fl buffer
		//
		in_fl_copy -= m_state.in_cnt;
		
		//
		// rounded up to the nearest four frames
		//
		in_fl_copy = (in_fl_copy + 3) & 0xfffffffc;
		
		//
		// make sure there are in_fl_copy frames in the input buffer
		//
		if (in_fl_copy > in_cnt)
		{
			in_fl_copy = in_cnt & 0xfffffffc;
		}
	
		//
		// make sure the copy won't overflow the in_fl buffer	
		//
		if ((in_fl_copy + m_state.in_cnt) > MAX_INPUT_FRAMES)
		{
			in_fl_copy = (MAX_INPUT_FRAMES - m_state.in_cnt) & 0xfffffffc;
		}
	
		//
		// convert and scale the samples
		//
		in_fl = m_in_fl + (m_state.in_cnt << 1);
		m_convert(in_fl,(void *) in,(float *) gain,in_fl_copy);
		//sample_conversion_block<SampIn>::convert(in_fl,(const SampIn *)in,m_channels,gain,in_fl_copy);
		
		//
		// m_state.in_cnt is how many input samples the SRC loop has to chew on
		//
		m_state.in_cnt += in_fl_copy;
		
		//
		// may as well do this return value here...
		//
		in_cnt -= in_fl_copy;

	}	// if there are less than four frames in the input buffer
	
	//
	// Do the SRC from the in_fl buffer -> out buffer
	// 
	m_state.out = out;
	m_state.out_cnt = out_cnt_temp;
	
	if (mix)
	{
		m_proc_mix(&m_state);	
	}
	else
	{
		m_proc(&m_state);
	}
	
	//
	// Write the out_cnt return value
	//	
	out_cnt -= out_cnt_temp - m_state.out_cnt;
	
	SRC_PROFILE_END(out_cnt);
	
} // resample_gain


#endif // _RESAMPLER_SINC_H_

