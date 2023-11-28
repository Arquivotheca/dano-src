
#include <media2/AudioResampler.h>

#include "resampler_direct.h"
#include "resampler_zero.h"
#include "resampler_linear.h"
#include "resampler_trilinear.h"
#include "resampler_cubic.h"
#include "resampler_sinc.h"
#include "resampler_sinc_lowpass.h"

#include <support2/Debug.h>
#include <support2/StdIO.h>

#define checkpoint \
berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Media2 {

class BAudioResampler::resampler_impl
{
public:
	resampler_impl(
		BAudioResampler::resampler_type _type,
		const media_multi_audio_format& _inputFormat,
		const media_multi_audio_format& _outputFormat,
		_resampler_base *				_resampler) :
		type(_type), inputFormat(_inputFormat), outputFormat(_outputFormat), resampler(_resampler)
	{
		ASSERT(resampler);
	}
	~resampler_impl()
	{
		delete resampler;
	}
	const BAudioResampler::resampler_type	type;
	const media_multi_audio_format			inputFormat;
	const media_multi_audio_format			outputFormat;
	_resampler_base	* const					resampler;
};

// ------------------------------------------------------------------------ //

status_t 
BAudioResampler::Instantiate(
	resampler_type type,
	media_multi_audio_format inputFormat,
	media_multi_audio_format outputFormat,
	BAudioResampler ** outResampler)
{
	// sanity checks
	if (type < B_ZERO || type > B_SINC_LOWPASS) return B_BAD_VALUE;
	if (inputFormat.frame_rate < 1.0f) return B_MEDIA_BAD_FORMAT;
	if (outputFormat.frame_rate < 1.0f) return B_MEDIA_BAD_FORMAT;
	// + only float & short output supported
	// (32 bit integer support would be nice too)
	if (outputFormat.format != B_AUDIO_FLOAT &&
		outputFormat.format != B_AUDIO_INT16) return B_MEDIA_BAD_FORMAT;
	// + channel split/mix not supported, except for 1->2
	if (inputFormat.channel_count != outputFormat.channel_count &&
		(inputFormat.channel_count != 1 && outputFormat.channel_count != 2)) return B_MEDIA_BAD_FORMAT;
	if (inputFormat.channel_count < 1) return B_MEDIA_BAD_FORMAT;
	// + surround formats not supported
	if (inputFormat.channel_count > 2) return B_MEDIA_BAD_FORMAT;
	
	uint32 inputRate = uint32(inputFormat.frame_rate + 0.5f);
	uint32 outputRate = uint32(outputFormat.frame_rate + 0.5f);

	_resampler_base * r = 0;

	if (inputRate == outputRate)
	{
		// create a 'direct' resampler (format conversion only)
		switch (outputFormat.format)
		{
			case B_AUDIO_FLOAT:
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_OUTPUT
			switch (inputFormat.format)
			{
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
				case B_AUDIO_UINT8:
					r = new resampler_direct<uint8, float>(	inputFormat.channel_count);
					break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
				case B_AUDIO_INT32:
					r = new resampler_direct<int32, float>(	inputFormat.channel_count);
					break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
				case B_AUDIO_FLOAT:
					r = new resampler_direct<float, float>(	inputFormat.channel_count);
					break;
#endif
				case B_AUDIO_INT16:
					r = new resampler_direct<int16, float>(	inputFormat.channel_count);
					break;

				default:
					return B_UNSUPPORTED;
			}
			break;
#else
			return B_UNSUPPORTED;
#endif
			case B_AUDIO_INT16:
#if _MEDIA2_RESAMPLER_ENABLE_INT16_OUTPUT
			switch (inputFormat.format)
			{
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
				case media_raw_audio_format::B_AUDIO_UCHAR:
					r = new resampler_direct<uint8, int16>(	inputFormat.channel_count);
					break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
				case B_AUDIO_INT32:
					r = new resampler_direct<int32, int16>(	inputFormat.channel_count);
					break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
				case B_AUDIO_FLOAT:
					r = new resampler_direct<float, int16>(	inputFormat.channel_count);
					break;
#endif
				case B_AUDIO_INT16:
					r = new resampler_direct<int16, int16>(	inputFormat.channel_count);
					break;

				default:
					return B_UNSUPPORTED;
			}
			break;
#else
			return B_UNSUPPORTED;
#endif
		}
	}
	else
	{
#if _MEDIA2_RESAMPLER_ENABLE_SINC
		// + resampler_sinc doesn't allow access to the modified input frame rate, so
		//   calculate it first:
		if (type == B_SINC_LOWPASS && inputRate < outputRate) type = B_SINC;
		if (type == B_SINC_LOWPASS)
		{
			uint32 oversampledOutputRate = outputRate << sinc_lowpass_const::OVERSAMPLE_SHIFT_FACTOR;
			get_sinc_table_size(
				&inputRate, &oversampledOutputRate,
				resampler_sinc_const::SINC_TAPS,
				resampler_sinc_const::MAX_SINC_TABLE_SIZE);
		}
		else
		if (type == B_SINC)
		{
			get_sinc_table_size(
				&inputRate, &outputRate,
				resampler_sinc_const::SINC_TAPS,
				resampler_sinc_const::MAX_SINC_TABLE_SIZE);
		}
#endif //_MEDIA2_RESAMPLER_ENABLE_SINC

		switch (outputFormat.format)
		{
			case B_AUDIO_FLOAT:
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_OUTPUT
			// xxx -> float resamplers
			switch (type)
			{
				case B_ZERO :
				default :
				{		
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_zero<uint8, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_zero<int32, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_zero<float, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_zero<int16, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}
					break;
				}
#if _MEDIA2_RESAMPLER_ENABLE_LINEAR
				case B_LINEAR :
				{		
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_linear<uint8, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_linear<int32, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_linear<float, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_linear<int16, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}			
					break;
				}
#endif //_MEDIA2_RESAMPLER_ENABLE_LINEAR

#if _MEDIA2_RESAMPLER_ENABLE_TRILINEAR
				case B_TRILINEAR :
				{		
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_trilinear<uint8, float>(	inputRate,
																		outputRate,
																		inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_trilinear<int32, float>(	inputRate,
																		outputRate,
																		inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_trilinear<float, float>(	inputRate,
																		outputRate,
																		inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_trilinear<int16, float>(	inputRate,
																		outputRate,
																		inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}				
					break;
				}
#endif //_MEDIA2_RESAMPLER_ENABLE_TRILINEAR

#if _MEDIA2_RESAMPLER_ENABLE_CUBIC
				case B_CUBIC :
				{		
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_cubic<uint8, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_cubic<int32, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_cubic<float, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_cubic<int16, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}			
					break;
				}
#endif //_MEDIA2_RESAMPLER_ENABLE_CUBIC

#if _MEDIA2_RESAMPLER_ENABLE_SINC
				case B_SINC :
				{			
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_sinc<uint8, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_sinc<int32, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_sinc<float, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_sinc<int16, float>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}
					break;
				}
				case B_SINC_LOWPASS :
				{			
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_sinc_lowpass<uint8, float>(	inputRate,
																			outputRate,
																			inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_sinc_lowpass<int32, float>(	inputRate,
																			outputRate,
																			inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_sinc_lowpass<float, float>(	inputRate,
																			outputRate,
																			inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_sinc_lowpass<int16, float>(	inputRate,
																			outputRate,
																			inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}
					break;
				}
#endif //_MEDIA2_RESAMPLER_ENABLE_SINC
			} // switch(...
			break;
#else
			// xxx -> float resamplers disabled
			return B_UNSUPPORTED;
#endif

			case B_AUDIO_INT16:
#if _MEDIA2_RESAMPLER_ENABLE_INT16_OUTPUT
			// xxx -> int16 resamplers
			switch (type)
			{
				case B_ZERO :
				default :
				{		
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_zero<uint8, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_zero<int32, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_zero<float, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_zero<int16, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}
					break;
				}
#if _MEDIA2_RESAMPLER_ENABLE_LINEAR
				case B_LINEAR :
				{		
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_linear<uint8, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_linear<int32, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_linear<float, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_linear<int16, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}			
					break;
				}
#endif //_MEDIA2_RESAMPLER_ENABLE_LINEAR

#if _MEDIA2_RESAMPLER_ENABLE_TRILINEAR
				case B_TRILINEAR :
				{		
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_trilinear<uint8, int16>(	inputRate,
																		outputRate,
																		inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_trilinear<int32, int16>(	inputRate,
																		outputRate,
																		inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_trilinear<float, int16>(	inputRate,
																		outputRate,
																		inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_trilinear<int16, int16>(	inputRate,
																		outputRate,
																		inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}				
					break;
				}
#endif //_MEDIA2_RESAMPLER_ENABLE_TRILINEAR

#if _MEDIA2_RESAMPLER_ENABLE_CUBIC
				case B_CUBIC :
				{		
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_cubic<uint8, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_cubic<int32, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_cubic<float, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_cubic<int16, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}			
					break;
				}
#endif //_MEDIA2_RESAMPLER_ENABLE_CUBIC

#if _MEDIA2_RESAMPLER_ENABLE_SINC
				case B_SINC :
				{			
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_sinc<uint8, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_sinc<int32, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_sinc<float, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_sinc<int16, int16>(	inputRate,
																	outputRate,
																	inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}
					break;
				}
				case B_SINC_LOWPASS :
				{			
					switch(inputFormat.format){
#if _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT
						case B_AUDIO_UINT8:
							r = new resampler_sinc_lowpass<uint8, int16>(	inputRate,
																			outputRate,
																			inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT
						case B_AUDIO_INT32:
							r = new resampler_sinc_lowpass<int32, int16>(	inputRate,
																			outputRate,
																			inputFormat.channel_count);
							break;
#endif
#if _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT
						case B_AUDIO_FLOAT:
							r = new resampler_sinc_lowpass<float, int16>(	inputRate,
																			outputRate,
																			inputFormat.channel_count);
							break;
#endif
						case B_AUDIO_INT16:
							r = new resampler_sinc_lowpass<int16, int16>(	inputRate,
																			outputRate,
																			inputFormat.channel_count);
							break;
						default:
							return B_UNSUPPORTED;
					}
					break;
				}
#endif //_MEDIA2_RESAMPLER_ENABLE_SINC
			} // switch(type)
#else
			// xxx -> int16 resamplers disabled
			return B_UNSUPPORTED;
#endif
		}	
	}
	inputFormat.frame_rate = inputRate;
	outputFormat.frame_rate = outputRate;
	*outResampler = new BAudioResampler(new resampler_impl(type, inputFormat, outputFormat, r));
	return B_OK;
}

status_t 
BAudioResampler::ConvertFloatToUInt8(const float *input, uint8 *output, size_t sampleCount, float scale)
{
	if (!sampleCount) return B_OK;
	if (sampleCount >= 4)
	{
		size_t toConvert = sampleCount & 0xfffffffc;
		convertBufferFloatToUByte((int32*)output, const_cast<float*>(input), scale, toConvert);
		sampleCount -= toConvert;
		if (sampleCount > 0)
		{
			input += toConvert;
			output += toConvert;
		}
	}
	while (sampleCount > 0)
	{
		*output++ = sample_conversion<float,uint8>::convert(*input++, scale);	
	}
	return B_OK;
}

status_t 
BAudioResampler::ConvertFloatToInt16(const float *input, int16 *output, size_t sampleCount, float scale)
{
	if (!sampleCount) return B_OK;
	if (sampleCount >= 4)
	{
		size_t toConvert = sampleCount & 0xfffffffc;
		convertBufferFloatToWord((int32*)output, const_cast<float*>(input), scale, toConvert);
		sampleCount -= toConvert;
		if (sampleCount > 0)
		{
			input += toConvert;
			output += toConvert;
		}
	}
	while (sampleCount > 0)
	{
		*output++ = sample_conversion<float,int16>::convert(*input++, scale);	
	}
	return B_OK;
}

status_t 
BAudioResampler::ConvertFloatToInt32(const float *input, int32 *output, size_t sampleCount, float scale)
{
	if (!sampleCount) return B_OK;
	if (sampleCount >= 4)
	{
		size_t toConvert = sampleCount & 0xfffffffc;
		convertBufferFloatToInt(output, const_cast<float*>(input), scale, toConvert);
		sampleCount -= toConvert;
		if (sampleCount > 0)
		{
			input += toConvert;
			output += toConvert;
		}
	}
	while (sampleCount > 0)
	{
		*output++ = sample_conversion<float,int32>::convert(*input++, scale);	
	}
	return B_OK;
}

// ------------------------------------------------------------------------ //

BAudioResampler::~BAudioResampler()
{
	delete mImpl;
	if (mUnityGain) delete [] mUnityGain;
}

BAudioResampler::resampler_type 
BAudioResampler::Type() const
{
	return mImpl->type;
}

media_multi_audio_format 
BAudioResampler::InputFormat() const
{
	return mImpl->inputFormat;
}

media_multi_audio_format 
BAudioResampler::OutputFormat() const
{
	return mImpl->outputFormat;
}

void 
BAudioResampler::Reset()
{
	mImpl->resampler->clear();
}

status_t 
BAudioResampler::Resample(
	const void *input, size_t *ioInputFrames,
	void *output, size_t *ioOutputFrames,
	const float *gain,
	bool mix)
{
	float * _gain = const_cast<float*>(gain);
	if (!_gain)
	{
		if (!mUnityGain)
		{
			mUnityGain = new float[mImpl->inputFormat.channel_count];
			for (size_t n = 0; n < mImpl->inputFormat.channel_count; n++) mUnityGain[n] = 1.0f;
		}
		_gain = mUnityGain;
	}
	mImpl->resampler->resample_gain(input, *ioInputFrames, output, *ioOutputFrames, _gain, mix);
	return B_OK;
}

BAudioResampler::BAudioResampler(resampler_impl *impl)
{
	ASSERT(impl);
	mImpl = impl;
	mUnityGain = 0;
}

} } // B::Media2
