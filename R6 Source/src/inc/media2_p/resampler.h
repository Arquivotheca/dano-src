//	resampler.h
//	Resample sound data
//	Copyright 1998 Be, Incorporated

#if !defined(resampler_h)
#define resampler_h


#include <support2/SupportDefs.h>	//	for int16 etc
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <support2/Debug.h>
#include "mixer_i586.h"

//
// Assembly conditional compilation flags
// 
#if __INTEL__
#define SRC_ASM_OPTIMIZED	1	// Set this to one if you want the Intel assembly optimized versions of the resamplers
#else
#define SRC_ASM_OPTIMIZED	0	// never set this for other platforms
#endif


//	Note that gain/conversion scaling math depends on
//	all the info values being float, even when going from
//	one integer format to another.
template<class Samp> class sample_info;

#if __MWERKS__ //mwld is brain-dead

template<> class sample_info<int16> {
public:
	static const float range;
	static const float mid_value;
	static const float max_value;
	static const float min_value;
};
//	These should be inline except for MWCC objecting
const float sample_info<int16>::range = 32767.0;
const float sample_info<int16>::mid_value = 0.0;
const float sample_info<int16>::max_value = 32767.0;
const float sample_info<int16>::min_value = -32767.0;

template<> class sample_info<int32> {
public:
	static const float range;
	static const float mid_value;
	static const float max_value;
	static const float min_value;
};
const float sample_info<int32>::range = (float)LONG_MAX;
const float sample_info<int32>::mid_value = 0.0;
const float sample_info<int32>::max_value = (float)LONG_MAX;
const float sample_info<int32>::min_value = -(float)LONG_MAX;

template<> class sample_info<float> {
public:
	static const float range;
	static const float mid_value;
	static const float max_value;
	static const float min_value;
};
const float sample_info<float>::range = 1.0;
const float sample_info<float>::mid_value = 0.0;
const float sample_info<float>::max_value = 1.0;
const float sample_info<float>::min_value = -1.0;

template<> class sample_info<uint8> {
public:
	static const float range;
	static const float mid_value;
	static const float max_value;
	static const float min_value;
};
const float sample_info<uint8>::range = 127.0;
const float sample_info<uint8>::mid_value = 128.0;
const float sample_info<uint8>::max_value = 255.0;
const float sample_info<uint8>::min_value = 1.0;

#else //not __MWERKS__

template<> class sample_info<int16> {
public:
	static const float range = 32767.0;
	static const float mid_value = 0.0;
	static const float max_value = 32767.0;
	static const float min_value = -32767.0;
};

template<> class sample_info<int32> {
public:
	static const float range = (float)LONG_MAX;
	static const float mid_value = 0.0;
	static const float max_value = (float)LONG_MAX;
	static const float min_value = -(float)LONG_MAX;
};

template<> class sample_info<float> {
public:
	static const float range = 1.0;
	static const float mid_value = 0.0;
	static const float max_value = 1.0;
	static const float min_value = -1.0;
};

template<> class sample_info<uint8> {
public:
	static const float range = 127.0;
	static const float mid_value = 128.0;
	static const float max_value = 255.0;
	static const float min_value = 1.0;
};

#endif //__MWERKS__

//------------------------------------------------------------------------------------------------------
//
// Templae for sample conversion classes
// 
// Since some of the new resamplers convert from the input format to
// float to the output format, we now need format converters that do
// gain scaling and converters that don't. 
//
//------------------------------------------------------------------------------------------------------

template<class SampIn, class SampOut> class sample_conversion;

// 
//	explicit specialization for same->same conversions
//

template<>
class sample_conversion<int16, int16> {
public:
	static inline int16 convert(int16 in, float gain)
		{
			float x = in * gain;
			if (x < -32767.0) return -32767;
			if (x > 32767.0) return 32767;
			return (int16)x;
		}

	static inline int16 convert(int16 in)
		{
			return in;
		}
};

template<>
class sample_conversion<int32, int32> {
public:
	static inline int32 convert(int32 in, float gain)
		{
			float x = in * gain;
			if (x < -LONG_MAX) return -LONG_MAX;
			if (x > LONG_MAX) return LONG_MAX;
			return (int32)x;
		}

	static inline int32 convert(int32 in)
		{
			return in;
		}
};


template<>
class sample_conversion<float, float> {
public:
	static inline float convert(float in, float gain)
		{
			return in * gain;	//	we don't clip float
		}

	static inline float convert(float in)
		{
			return in;
		}
};


template<>
class sample_conversion<uint8, uint8> {
public:
	static inline uint8 convert(uint8 in, float gain)
		{
			float x = (in - 128) * gain + 128;
			if (x < 1) return 1;
			if (x > 255) return 255;
			return (uint8)x;
		}

	static inline uint8 convert(uint8 in)
		{
			return in;
		}
};

//	We used to partially specialize for float as the output 
//	parameter, but MWCC didn't like that so we explicitly 
//	specialize for uint8, int16 and int32 to float.
//
//  mattg - added specialization for float -> all other formats
template<>
class sample_conversion<uint8, float> {
public:
	static inline float convert(uint8 in, float gain)
		{	//	don't clip
			return (in - sample_info<uint8>::mid_value) * gain;
		}

	static inline float convert(uint8 in)
		{	//	don't clip
			return (in - sample_info<uint8>::mid_value);
		}
};

template<>
class sample_conversion<int16, float> {
public:
	static inline float convert(int16 in, float gain)
		{	//	don't clip
			return in * gain;
		}

	static inline float convert(int16 in)
		{	//	don't clip
			return in;
		}
};


template<>
class sample_conversion<int32, float> {
public:
	static inline float convert(int32 in, float gain)
		{	//	don't clip
			return in * gain;
		}

	static inline float convert(int32 in)
		{	//	don't clip
			return in;
		}
};


template<>
class sample_conversion<float, int16> {
public:
	static inline int16 convert(float in, float gain)
		{
			float x = in * gain;
			if (x < -32767.0) return -32767;
			if (x > 32767.0) return 32767;
			return (int16)x;
		}

	static inline int16 convert(float in)
		{
			return (int16) (in * 32767.0);
		}
};


template<>
class sample_conversion<float, int32> {
public:
	static inline int32 convert(float in, float gain)
		{
			float x = in * gain;
			if (x < -LONG_MAX) return -LONG_MAX;
			if (x > LONG_MAX) return LONG_MAX;
			return (int32)x;
		}

	static inline int32 convert(float in)
		{
			return (int32) (in * LONG_MAX);
		}
};

template<>
class sample_conversion<float, uint8> {
public:
	static inline uint8 convert(float in, float gain)
		{
			float x = in * gain + 128;
			if (x < 1) return 1;
			if (x > 255) return 255;
			return (uint8)x;
		}

	static inline uint8 convert(float in)
		{
			return (uint8) (in * 127.0 + 128.0);
		}
};



//	If everything else fails (it shouldn't in our usage) we
//	fall back to this generic, but not optimally efficient, template.
template<class SampIn, class SampOut>
class sample_conversion {
public:
	static inline SampOut convert(SampIn in, float gain)
		{
			float x = (in - sample_info<SampIn>::mid_value) *
				gain + sample_info<SampOut>::mid_value;
			if (x < sample_info<SampOut>::min_value) return (SampOut)sample_info<SampOut>::min_value;
			if (x > sample_info<SampOut>::max_value) return (SampOut)sample_info<SampOut>::max_value;
			return static_cast<SampOut>(x);
		}

	static inline SampOut convert(SampIn in)
		{
			float x;
			
			x = (in - sample_info<SampIn>::mid_value);
			x *= sample_info<SampOut>::range / sample_info<SampIn>::range;
			x += sample_info<SampOut>::mid_value;
			
			if (x < sample_info<SampOut>::min_value) return (SampOut)sample_info<SampOut>::min_value;
			if (x > sample_info<SampOut>::max_value) return (SampOut)sample_info<SampOut>::max_value;
			return static_cast<SampOut>(x);
		}

};

//------------------------------------------------------------------------------------------------------
//
// Templae for block sample conversion classes
// 
// These classes convert blocks of frames. They exist to take advantage of the
// format conversion assembly routines in the media lib.
//
// These classes always create stereo float frames from some other input format
//
//------------------------------------------------------------------------------------------------------

template <class SampIn> void block_convert_mono
(
	float *dest,
	SampIn *source,
	float *gain,
	int frames
)
{
	float	sample;
	float	gain0 = gain[0]*(1.0/sample_info<SampIn>::range);
	float	gain1 = gain[1]*(1.0/sample_info<SampIn>::range);
	
	float	*temp;

	temp = dest;
	
	for (int i = 0; i < frames; i++)
	{
		sample = sample_conversion<SampIn, float>::convert(*(source++));

		*(dest++) = sample * gain0;
		*(dest++) = sample * gain1;
	}
	
}  // block_convert_mono

template <class SampIn> void block_convert_stereo
(
	float *dest,
	SampIn *source,
	float *gain,
	int frames
)
{
	float	gain0 = gain[0]*(1.0/sample_info<SampIn>::range);
	float	gain1 = gain[1]*(1.0/sample_info<SampIn>::range);
	
	for (int i = 0; i < frames; i++)
	{
		*(dest++) = sample_conversion<SampIn, float>::convert(*(source++), gain0);
		*(dest++) = sample_conversion<SampIn, float>::convert(*(source++), gain1);
	}

}  // block_convert_stereo

typedef void (*block_convert_proc)(float *dest,void *source,float *gain,int frames);

template <class SampIn> class block_convert_info
{
	public:
	
	static const block_convert_proc 	mono = (block_convert_proc) &block_convert_mono <SampIn>;
	static const block_convert_proc		stereo = (block_convert_proc) &block_convert_stereo <SampIn>;
};


#if __INTEL__

template<> class block_convert_info<uint8> 
{
	public:
	
	static const block_convert_proc	mono = (block_convert_proc) &unsignedByteToFloat1to2;
	static const block_convert_proc	stereo = (block_convert_proc) &unsignedByteToFloat2;
};

template<> class block_convert_info<int16> 
{
	public:
	
	static const block_convert_proc	mono = (block_convert_proc) &wordToFloat1to2;
	static const block_convert_proc	stereo = (block_convert_proc) &wordToFloat2;	
};

template<> class block_convert_info<int32> 
{
	public:
	
	static const block_convert_proc	mono = (block_convert_proc) &intToFloat1to2;
	static const block_convert_proc	stereo = (block_convert_proc) &intToFloat2;
};

template<> class block_convert_info<float> 
{
	public:
	
	static const block_convert_proc	mono = (block_convert_proc) &floatToFloat1to2;
	static const block_convert_proc	stereo = (block_convert_proc) &floatToFloat2;
};

#endif // __INTEL__


//------------------------------------------------------------------------------------------------------
//
// _resampler_base
//
//
// If SRC profiling is turned on, the SRC will print the percentage of CPU
// time used by the SRC inner loop roughly once per second.  
//
//------------------------------------------------------------------------------------------------------

#define SRC_PROFILE 1

#ifdef SRC_PROFILE

#include <kernel/OS.h>

#define SRC_PROFILE_START(x)	profile_start(x)
#define SRC_PROFILE_END(x)		profile_end(x)
#define SRC_HOWDY(string)		printf##string

#else

#define SRC_PROFILE_START(v1)
#define SRC_PROFILE_END(v1)
#define SRC_HOWDY(string)

#endif	// SRC_PROFILE


class _resampler_base {

public:
	_resampler_base()
	{
	}

	virtual ~_resampler_base() 
	{ 
	}
	
	virtual void clear() 
	{ 
#ifdef SRC_PROFILE
		m_total_time = 0;
		m_last_total_time = 0;
		m_total_samples = 0;
		m_last_total_samples = 0;
		
		m_next_print_count = (bigtime_t) m_freq_out;
#endif
	}
	
	virtual void resample_gain(const void *, size_t &, void *, size_t &, const float *, bool)
		{
			assert(!"where am I and what am I doing in this handbasket?");
		}
		
protected:

#ifdef SRC_PROFILE
	uint32 		m_freq_in;
	uint32 		m_freq_out;
	bigtime_t	m_total_time;
	bigtime_t	m_last_total_time;
	bigtime_t	m_total_samples;
	bigtime_t	m_last_total_samples;
	bigtime_t	m_next_print_count;

	bigtime_t	m_start_time;
	uint32		m_initial_out_cnt;
	
	virtual inline void profile_start(size_t out_cnt)
	{
		m_initial_out_cnt = out_cnt;
		m_start_time = system_time();
	}
	
	virtual inline void profile_end(size_t out_cnt)
	{
		uint32 out_samples_made;

		out_samples_made = m_initial_out_cnt - out_cnt;
		
		if (0 != out_samples_made)
		{
			m_total_time += system_time() - m_start_time;
			m_total_samples += out_samples_made;
		
			if (m_total_samples >= m_next_print_count)
			{
				float sample_delta,time_delta,real_time_percent;
				
				time_delta = (float) (m_total_time - m_last_total_time);
				sample_delta = (float) (m_total_samples - m_last_total_samples);
				real_time_percent = time_delta / (sample_delta / ((float) m_freq_out) * 1000000.0) * 100.0;
				
				printf("----- %.4f%%\n",real_time_percent);
				
				m_next_print_count += (bigtime_t) m_freq_out;
				m_last_total_samples = m_total_samples;
				m_last_total_time = m_total_time;
			}
		}
	
	} // profile_done
	
#endif	// SRC_PROFILE

};

//------------------------------------------------------------------------------------------------------
//
// Sample rate conversion types
//
//------------------------------------------------------------------------------------------------------

enum
{
	RESAMPLER_ZERO = 0,		// zeroth-order converter (aka drop-sample)
	RESAMPLER_LINEAR,		// linear interpolator
	RESAMPLER_TRILINEAR,	// tri-linear interpolator
	RESAMPLER_CUBIC,		// cubic interpolator
	RESAMPLER_SINC,			// sin(x)/x interpolator
	RESAMPLER_SINC_LOWPASS,	// sinc interpolator followed by lowpass filter; for downsampling only
	RESAMPLER_LAST,
	RESAMPLER_MAX = RESAMPLER_LAST - 1 
};

//------------------------------------------------------------------------------------------------------
//
// Useful values for converting to float
//
// The value for ASM_GAIN_SCALE_UINT8_TO_FLOAT is 1.0/32767.0 because the assembly routines
// first convert 8 bit unsigned to 16 bit signed, then to float.
//
//------------------------------------------------------------------------------------------------------

#define ASM_GAIN_SCALE_UINT8_TO_FLOAT	(1.0/32767.0)
#define GAIN_SCALE_INT16_TO_FLOAT		(1.0/32767.0)
#define GAIN_SCALE_INT32_TO_FLOAT		4.65661287308e-10

#endif // resampler_h
