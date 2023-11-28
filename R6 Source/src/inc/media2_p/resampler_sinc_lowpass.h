//	resampler_sinc_lowpass.h
//	Class to implement sinc resampler with lowpass filtering.

/* 0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0

	This is meant to be used exclusively for downsampling.  The sinc resampler is
	used to oversample the input signal to twice the output rate.  The oversampled
	signal is then filtered and decimated by two, leaving you with a lowpass-filtered
	signal at the desired output rate
	
	Here's the data flow:
	
	-----------------------
	 caller's input buffer
	-----------------------
	         |
	         | (sinc resampler)
	         V
	-----------------------
       oversample buffer
	-----------------------
	         |
	         | (IIR lowpass filter)
	         V
	-----------------------
     output history buffer
	-----------------------
	         |
	         | (decimate by two)
	         V
	-----------------------
    caller's output buffer
	-----------------------
   
	         	 
	
  0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0 */

#ifndef _RESAMPLER_SINC_LOWPASS_H_
#define _RESAMPLER_SINC_LOWPASS_H_

#include <math.h>
#include <string.h>
#include <resampler.h>
#include <sinc_lowpass_state.h>
#include <resampler_sinc.h>

//************************************************************************************
//
// Class to store the filter coefficients and constants
//
//************************************************************************************

class sinc_lowpass_const
{

public:

	enum
	{
		OVERSAMPLE_SHIFT_FACTOR = 1,
		MAX_OUTPUT_FRAMES = 128,
		IIR_TAPS = 13,
		MAX_OVER_FRAMES = MAX_OUTPUT_FRAMES + IIR_TAPS - 1,
		MAX_HIST_FRAMES = MAX_OUTPUT_FRAMES + IIR_TAPS - 2
	};

protected:

	static const float m_a[];
	static const float m_b[];	
	
};


//************************************************************************************
//
// Generic C++ decimate functions
//
//************************************************************************************

//-------------------------------------------------------------------------------------
//
// Decimate without mixing
//
//-------------------------------------------------------------------------------------

template <class SampOut> void decimate(sinc_lowpass_state *state)
{
	uint32 i;
	
	//
	// Decimate the output history data
	//
	size_t decimate_cnt = state->out_cnt << 1;
		
	if (decimate_cnt > (state->hist_cnt - (sinc_lowpass_const::IIR_TAPS - 2)))
	{
		decimate_cnt = state->hist_cnt - (sinc_lowpass_const::IIR_TAPS - 2);
	}
	
	decimate_cnt &= 0xfffffffe;
	
	SampOut *temp = (SampOut *) state->out;
	float *hist = state->hist;
	
	for (i = 0; i < decimate_cnt; i += 2)
	{
		*(temp++) = sample_conversion<float,SampOut>::convert( hist[0], 1.0f );
		*(temp++) = sample_conversion<float,SampOut>::convert( hist[1], 1.0f );			
		
		hist += 4;
	}
	
	state->hist_cnt -= decimate_cnt;

	//
	// Move the data at the end of the output history buffer to the beginning
	//
	float *dest = state->hist;
	
	for (i = 0; i < (state->hist_cnt << 1); i++)
	{
		dest[i] = hist[i];	
	}

	//
	// Handle the return value	
	//
	state->out_cnt -= decimate_cnt >> 1;

} // decimate

//-------------------------------------------------------------------------------------
//
// Decimate with mixing
//
//-------------------------------------------------------------------------------------

template <class SampOut> void decimate_mix(sinc_lowpass_state *state)
{
	uint32 i;
	
	//
	// Decimate the output history data
	//
	size_t decimate_cnt = state->out_cnt << 1;
		
	if (decimate_cnt > (state->hist_cnt - (sinc_lowpass_const::IIR_TAPS - 2)))
	{
		decimate_cnt = state->hist_cnt - (sinc_lowpass_const::IIR_TAPS - 2);
	}
	
	decimate_cnt &= 0xfffffffe;
	
	SampOut *temp = (SampOut *) state->out;
	float *hist = state->hist;
	
	for (i = 0; i < decimate_cnt; i += 2)
	{
		*(temp++) += sample_conversion<float,SampOut>::convert( hist[0], 1.0f );
		*(temp++) += sample_conversion<float,SampOut>::convert( hist[1], 1.0f );			
		
		hist += 4;
	}
	
	state->hist_cnt -= decimate_cnt;

	//
	// Move the data at the end of the output history buffer to the beginning
	//
	float *dest = state->hist;
	
	for (i = 0; i < (state->hist_cnt << 1); i++)
	{
		dest[i] = hist[i];	
	}

	//
	// Handle the return value	
	//
	state->out_cnt -= decimate_cnt >> 1;

} // decimate_mix


//************************************************************************************
//
// Wrapper templates for resampler functions
//
//************************************************************************************

typedef void (*decimate_proc)(sinc_lowpass_state *state);

template <class SampOut> class decimate_info 
{
	public:
	
	static const decimate_proc proc = &decimate <SampOut> ;
	static const decimate_proc proc_mix = &decimate_mix <SampOut>;
};


#if __INTEL__&&SRC_ASM_OPTIMIZED

//-------------------------------------------------------------------------------------
//
// assembly function prototypes
//
//-------------------------------------------------------------------------------------

extern "C" void decimate_float_to_float(sinc_lowpass_state *state);
extern "C" void decimate_float_to_float_mix(sinc_lowpass_state *state);

//-------------------------------------------------------------------------------------
//
// assembly template wrapper
//
//-------------------------------------------------------------------------------------
template <> class decimate_info <float>
{
	public:
	
	static const decimate_proc proc = &decimate_float_to_float;
	static const decimate_proc proc_mix = &decimate_float_to_float_mix;

};

#endif

//************************************************************************************
//
// Resampler template
//
//************************************************************************************

template <class SampIn, class SampOut>
class resampler_sinc_lowpass 
: public _resampler_base, public sinc_lowpass_const
{

private:

	resampler_sinc <SampIn,float> 	m_sinc;
	
	float							m_over[MAX_OVER_FRAMES * 2];	// buffer holding oversampled data
	float							m_hist[MAX_HIST_FRAMES * 2];

	sinc_lowpass_state				m_state;
	
	decimate_proc					m_proc;
	decimate_proc					m_proc_mix;
	
public:
	resampler_sinc_lowpass(int32 freq_in,int32 freq_out,int channels); 
	~resampler_sinc_lowpass();
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
resampler_sinc_lowpass<SampIn,SampOut>::resampler_sinc_lowpass
(
	int32 freq_in,
	int32 freq_out,
	int channels
) : m_sinc(freq_in,freq_out << 1,channels)
	
{
	assert(channels == 1 || channels == 2);

	SRC_HOWDY(("New sinc+lowpass interpolator, %ld to %ld\n",freq_in,freq_out));
	
#ifdef SRC_PROFILE
	m_freq_in = freq_in;
	m_freq_out = freq_out;
#endif

	clear();
	
	m_state.over = 		m_over;
	m_state.hist = 		m_hist;
	m_state.a = 		m_a;
	m_state.b = 		m_b;
	m_state.hist_size = MAX_HIST_FRAMES;
	
	m_proc = decimate_info<SampOut>::proc;
	m_proc_mix = decimate_info<SampOut>::proc_mix;

} // constructor


//-------------------------------------------------------------------------------------
//
// Destructor
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
resampler_sinc_lowpass<SampIn,SampOut>::~resampler_sinc_lowpass()
{
} // destructor


//-------------------------------------------------------------------------------------
//
// clear
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_sinc_lowpass<SampIn,SampOut>::clear()
{

	m_state.over_cnt = IIR_TAPS - 1;
	m_state.hist_cnt = IIR_TAPS - 2;
	
	_resampler_base::clear();
	
	m_sinc.clear();
	
} // clear


//-------------------------------------------------------------------------------------
//
// resample_gain
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_sinc_lowpass<SampIn,SampOut>::resample_gain
(
	const void * 	in,
	size_t & 		in_cnt,
	void * 			out,
	size_t & 		out_cnt,
	const float *	gain,
	bool 			mix
)
{
	SRC_PROFILE_START(out_cnt);
	
	//
	// Figure out number of oversampled frames to request
	//
	size_t over_cnt = out_cnt << 1;	// oversample by two, so << 1
	size_t over_max = MAX_OVER_FRAMES - m_state.over_cnt;
	if (over_cnt > over_max)
	{
		over_cnt = over_max;
	}
	
	//-------------------------------------------------------------------
	// Oversample the data via the sinc interpolator
	//-------------------------------------------------------------------
	size_t over_cnt_temp = over_cnt;
	
	m_sinc.resample_gain
	(
		in,
		in_cnt,
		m_over + (m_state.over_cnt << 1),
		over_cnt_temp,
		gain,
		false	// no mixing at this stage
	);
	
	//
	// Figure out how many frames in the over buffer 
	//
	m_state.over_cnt += over_cnt - over_cnt_temp;
	
	//-------------------------------------------------------------------	
	// Filter and decimate the oversampled data
	//-------------------------------------------------------------------
	
	if ((m_state.over_cnt >= IIR_TAPS) && (m_state.hist_cnt >= (IIR_TAPS - 2)))
	{
		m_state.out = out;
		m_state.out_cnt = out_cnt;

		// run the lowpass filter
		lowpass_iir(&m_state);
		
		// decimate by two
		if (mix)
		{
			m_proc_mix(&m_state);
		}
		else
		{
			m_proc(&m_state);
		}

		out_cnt = m_state.out_cnt;	
	}
	
	SRC_PROFILE_END(out_cnt);
	
} // resample_gain


#endif // _RESAMPLER_SINC_LOWPASS_H_

