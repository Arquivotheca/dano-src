//	resampler_direct.h
//	'dummy' resampler implementation of direct mixing with no sample-rate conversion

#ifndef _RESAMPLER_DIRECT_H_
#define _RESAMPLER_DIRECT_H_

#include <resampler.h>

struct direct_state
{
	void *			out;
	const void *	in;
	const float *	gain;
	uint32			channel_count;
	uint32			frame_count;
};

typedef void (*direct_proc)(direct_state * state);

//-------------------------------------------------------------------------------------
//	general forms
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut> void direct(direct_state * state)
{
	const SampIn * in = (const SampIn*)state->in;
	SampOut * out = (SampOut*)state->out;
	float gain0 = state->gain[0] * sample_info<SampOut>::range / sample_info<SampIn>::range;
	float gain1 = state->gain[1] * sample_info<SampOut>::range / sample_info<SampIn>::range;
	uint32 channel_count = state->channel_count;
	for (uint32 n = state->frame_count; n > 0; n--)
	{
		in += state->channel_count;
		*(out++) = sample_conversion<SampIn,SampOut>::convert(in[-channel_count], gain0);
		*(out++) = sample_conversion<SampIn,SampOut>::convert(in[-1], gain1);
	}
}

template <class SampIn, class SampOut> void direct_mix(direct_state * state)
{
	const SampIn * in = (const SampIn*)state->in;
	SampOut * out = (SampOut*)state->out;
	float gain0 = state->gain[0] * sample_info<SampOut>::range / sample_info<SampIn>::range;
	float gain1 = state->gain[1] * sample_info<SampOut>::range / sample_info<SampIn>::range;
	uint32 channel_count = state->channel_count;
	for (uint32 n = state->frame_count; n > 0; n--)
	{
		in += state->channel_count;
		*(out++) += sample_conversion<SampIn,SampOut>::convert(in[-channel_count], gain0);
		*(out++) += sample_conversion<SampIn,SampOut>::convert(in[-1], gain1);
	}
}

template <class SampIn, class SampOut> class direct_info
{
public:
	static const direct_proc	proc = &direct<SampIn, SampOut>;
	static const direct_proc	proc_mix = &direct_mix<SampIn, SampOut>;
};

//-------------------------------------------------------------------------------------
//	optimized forms (these are valid for PPC builds too)
//-------------------------------------------------------------------------------------

void direct_uint8_to_float		(direct_state * state);
void direct_uint8_to_float_mix	(direct_state * state);
void direct_int16_to_float		(direct_state * state);
void direct_int16_to_float_mix	(direct_state * state);
void direct_int32_to_float		(direct_state * state);
void direct_int32_to_float_mix	(direct_state * state);
void direct_float_to_float		(direct_state * state);
void direct_float_to_float_mix	(direct_state * state);

//-------------------------------------------------------------------------------------
//	template wrappers
//-------------------------------------------------------------------------------------

template <> class direct_info <uint8, float>
{
public:
	static const direct_proc	proc = &direct_uint8_to_float;
	static const direct_proc	proc_mix = &direct_uint8_to_float_mix;
};
template <> class direct_info <int16, float>
{
public:
	static const direct_proc	proc = &direct_int16_to_float;
	static const direct_proc	proc_mix = &direct_int16_to_float_mix;
};
template <> class direct_info <int32, float>
{
public:
	static const direct_proc	proc = &direct_int32_to_float;
	static const direct_proc	proc_mix = &direct_int32_to_float_mix;
};
template <> class direct_info <float, float>
{
public:
	static const direct_proc	proc = &direct_float_to_float;
	static const direct_proc	proc_mix = &direct_float_to_float_mix;
};

//-------------------------------------------------------------------------------------
//	the resampler
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
class resampler_direct : public _resampler_base
{
private:
	direct_proc		m_proc;
	direct_proc		m_proc_mix;
	uint32			m_channel_count;
public:
	resampler_direct(uint32 channel_count);
	~resampler_direct();
	virtual void resample_gain(
			const void * in,
			size_t & in_cnt,
			void * out,
			size_t & out_cnt,
			const float *gain,
			bool mix);
};

template <class SampIn, class SampOut>
resampler_direct<SampIn, SampOut>::resampler_direct(uint32 channel_count) :
	m_channel_count(channel_count)
{
	clear();
	m_proc = direct_info<SampIn,SampOut>::proc;
	m_proc_mix = direct_info<SampIn,SampOut>::proc_mix;
}

template <class SampIn, class SampOut>
resampler_direct<SampIn, SampOut>::~resampler_direct() {}

template <class SampIn, class SampOut>
void resampler_direct<SampIn,SampOut>::resample_gain(
	const void * 	in,
	size_t & 		in_cnt,
	void * 			out,
	size_t & 		out_cnt,
	const float *	gain,
	bool 			mix)
{
	size_t to_convert = min_c(in_cnt, out_cnt);
	if (to_convert <= 0) return;
	direct_state state;
	state.in = in;
	state.out = out;
	state.gain = gain;
	state.channel_count = m_channel_count;
	state.frame_count = to_convert;
	if (mix)
	{
		m_proc_mix(&state);
	}
	else
	{
		m_proc(&state);
	}
	in_cnt -= to_convert;
	out_cnt -= to_convert;
}

#endif // _RESAMPLER_DIRECT_H_
