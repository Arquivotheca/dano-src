#ifndef _MEDIA2_AUDIORESAMPLER_H_
#define _MEDIA2_AUDIORESAMPLER_H_

#include <media2/MediaDefs.h>

// +++ TO DO: make the following into MFLAGS

// at least one of these must be enabled
#define _MEDIA2_RESAMPLER_ENABLE_INT16_OUTPUT 1
#define _MEDIA2_RESAMPLER_ENABLE_FLOAT_OUTPUT 1

// 16bit input is default and may not be disabled
#define _MEDIA2_RESAMPLER_ENABLE_UINT8_INPUT 1
#define _MEDIA2_RESAMPLER_ENABLE_INT32_INPUT 1
#define _MEDIA2_RESAMPLER_ENABLE_FLOAT_INPUT 1

// drop-sample algorithm is default and may not be disabled
#define _MEDIA2_RESAMPLER_ENABLE_LINEAR 1
#define _MEDIA2_RESAMPLER_ENABLE_TRILINEAR 1
#define _MEDIA2_RESAMPLER_ENABLE_CUBIC 1
#define _MEDIA2_RESAMPLER_ENABLE_SINC 1

namespace B {
namespace Media2 {

class BAudioResampler
{
public:
		enum resampler_type
		{
			B_ZERO = 0,		// zeroth-order converter (aka drop-sample)
			B_LINEAR,		// linear interpolator
			B_TRILINEAR,	// tri-linear interpolator
			B_CUBIC,		// cubic interpolator
			B_SINC,			// sin(x)/x interpolator
			B_SINC_LOWPASS	// sin(x)/x interpolator with lowpass filter
							// (only useful for downsampling)
		};
	
	// factory method
	// - input and output frame rates are rounded to the nearest integer;
	//   B_SINC and B_SINC_LOWPASS resamplers may further modify the input frame
	//   rate to produce a reasonably small sinc table.

	static	status_t				Instantiate(
										resampler_type type,
										media_multi_audio_format inputFormat,
										media_multi_audio_format outputFormat,
										BAudioResampler ** outResampler);

	// float-to-integer conversion
	// - the resampler produces only floating-point output, so these functions may
	//   come in handy.  note that sampleCount is indeed the number of samples,
	//   not frames, to be converted.
	
	static status_t 				ConvertFloatToUInt8(
										const float *	input,
										uint8 *			output,
										size_t			sampleCount,
										float			scale = 1.0f);

	static status_t					ConvertFloatToInt16(
										const float *	input,
										int16 *			output,
										size_t			sampleCount,
										float			scale = 1.0f);

	static status_t					ConvertFloatToInt32(
										const float *	input,
										int32 *			output,
										size_t			sampleCount,
										float			scale = 1.0f);

	// instance methods

	virtual ~BAudioResampler();

	resampler_type Type() const;
	media_multi_audio_format InputFormat() const;
	media_multi_audio_format OutputFormat() const;

	// clear resampler state
	void Reset();

	status_t 						Resample(
										const void *	input,
										size_t *		ioInputFrames,
										void *			output,
										size_t *		ioOutputFrames,
										const float *	gain = 0,
										bool			mix = false);

private:
	class resampler_impl;
	BAudioResampler(resampler_impl* impl);

	resampler_impl *				mImpl;
	float *							mUnityGain;
};

} } // namespace B::Media2
#endif //_MEDIA2_AUDIORESAMPLER_H_
