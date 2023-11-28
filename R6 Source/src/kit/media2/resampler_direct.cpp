#include "resampler_direct.h"
#include <mixer_i586.h>

void direct_uint8_to_float(direct_state * state)
{
	float* out = (float*)state->out;
	uint8* in = (uint8*)state->in;
	uint32 frame_count = state->frame_count;
	uint32 f = frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4)
	{
		if (state->channel_count == 2)
		{
			unsignedByteToFloat2(out, in, const_cast<float*>(state->gain), f);
		}
		else
		{
			ASSERT(state->channel_count == 1);
			unsignedByteToFloat1to2(out, in, const_cast<float*>(state->gain), f);
		}
		frame_count -= f;
	}
	if (frame_count > 0)
	{
		state->in = in + f * state->channel_count;
		state->out = out + f * 2;
		state->frame_count = frame_count;
		direct<uint8,float>(state);
	}
}

void direct_uint8_to_float_mix(direct_state * state)
{
	float* out = (float*)state->out;
	uint8* in = (uint8*)state->in;
	uint32 frame_count = state->frame_count;
	uint32 f = frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4)
	{
		if (state->channel_count == 2)
		{
			unsignedByteToFloatAccum2(out, in, const_cast<float*>(state->gain), f);
		}
		else
		{
			ASSERT(state->channel_count == 1);
			unsignedByteToFloatAccum1to2(out, in, const_cast<float*>(state->gain), f);
		}
		frame_count -= f;
	}
	if (frame_count > 0)
	{
		state->in = in + f * state->channel_count;
		state->out = out + f * 2;
		state->frame_count = frame_count;
		direct_mix<uint8,float>(state);
	}
}

void direct_int16_to_float(direct_state * state)
{
	float* out = (float*)state->out;
	int16* in = (int16*)state->in;
	uint32 frame_count = state->frame_count;
	uint32 f = state->frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4)
	{
		if (state->channel_count == 2)
		{
			wordToFloat2(out, in, const_cast<float*>(state->gain), f);
		}
		else
		{
			ASSERT(state->channel_count == 1);
			wordToFloat1to2(out, in, const_cast<float*>(state->gain), f);
		}
		frame_count -= f;
	}
	if (frame_count > 0)
	{
		state->in = in + f * state->channel_count;
		state->out = out + f * 2;
		state->frame_count = frame_count;
		direct_mix<int16,float>(state);
	}
}

void direct_int16_to_float_mix(direct_state * state)
{
	float* out = (float*)state->out;
	int16* in = (int16*)state->in;
	uint32 frame_count = state->frame_count;
	uint32 f = state->frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4)
	{
		if (state->channel_count == 2)
		{
			wordToFloatAccum2(out, in, const_cast<float*>(state->gain), f);
		}
		else
		{
			ASSERT(state->channel_count == 1);
			wordToFloatAccum1to2(out, in, const_cast<float*>(state->gain), f);
		}
		frame_count -= f;
	}
	if (frame_count > 0)
	{
		state->in = in + f * state->channel_count;
		state->out = out + f * 2;
		state->frame_count = frame_count;
		direct_mix<int16,float>(state);
	}
}

void direct_int32_to_float(direct_state * state)
{
	float* out = (float*)state->out;
	int32* in = (int32*)state->in;
	uint32 frame_count = state->frame_count;
	uint32 f = state->frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4)
	{
		if (state->channel_count == 2)
		{
			intToFloat2(out, in, const_cast<float*>(state->gain), f);
		}
		else
		{
			ASSERT(state->channel_count == 1);
			intToFloat1to2(out, in, const_cast<float*>(state->gain), f);
		}
		frame_count -= f;
	}
	if (frame_count > 0)
	{
		state->in = in + f * state->channel_count;
		state->out = out + f * 2;
		state->frame_count = frame_count;
		direct_mix<int32,float>(state);
	}
}

void direct_int32_to_float_mix(direct_state * state)
{
	float* out = (float*)state->out;
	int32* in = (int32*)state->in;
	uint32 frame_count = state->frame_count;
	uint32 f = state->frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4)
	{
		if (state->channel_count == 2)
		{
			intToFloatAccum2(out, in, const_cast<float*>(state->gain), f);
		}
		else
		{
			ASSERT(state->channel_count == 1);
			intToFloatAccum1to2(out, in, const_cast<float*>(state->gain), f);
		}
		frame_count -= f;
	}
	if (frame_count > 0)
	{
		state->in = in + f * state->channel_count;
		state->out = out + f * 2;
		state->frame_count = frame_count;
		direct_mix<int32,float>(state);
	}
}

void direct_float_to_float(direct_state * state)
{
	float* out = (float*)state->out;
	float* in = (float*)state->in;
	uint32 frame_count = state->frame_count;
	uint32 f = state->frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4)
	{
		if (state->channel_count == 2)
		{
			floatToFloat2(out, in, const_cast<float*>(state->gain), f);
		}
		else
		{
			ASSERT(state->channel_count == 1);
			floatToFloat1to2(out, in, const_cast<float*>(state->gain), f);
		}
		frame_count -= f;
	}
	if (frame_count > 0)
	{
		state->in = in + f * state->channel_count;
		state->out = out + f * 2;
		state->frame_count = frame_count;
		direct_mix<float,float>(state);
	}
}

void direct_float_to_float_mix(direct_state * state)
{
	float* out = (float*)state->out;
	float* in = (float*)state->in;
	uint32 frame_count = state->frame_count;
	uint32 f = state->frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4)
	{
		if (state->channel_count == 2)
		{
			floatToFloatAccum2(out, in, const_cast<float*>(state->gain), f);
		}
		else
		{
			ASSERT(state->channel_count == 1);
			floatToFloatAccum1to2(out, in, const_cast<float*>(state->gain), f);
		}
		frame_count -= f;
	}
	if (frame_count > 0)
	{
		state->in = in + f * state->channel_count;
		state->out = out + f * 2;
		state->frame_count = frame_count;
		direct_mix<float,float>(state);
	}
}

