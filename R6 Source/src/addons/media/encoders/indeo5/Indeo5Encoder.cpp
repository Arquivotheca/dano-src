#include <MediaTrack.h>
#include <MediaFormats.h>
#include "Decoder.h"
#include "Extractor.h"

#include "Indeo5Encoder.h"

/* lib headers */

#include <string.h>
#include <Debug.h>
#include <File.h>
#include <Screen.h>
#include <MediaFormats.h>

media_encoded_video_format::video_encoding iv50_encoding;

void register_encoder(void)
{
	//printf("Indeo Encoder loaded\n");
	status_t 					err;
	media_format				mediaFormat;
	media_format_description	formatDescription[5];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'IV50';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'IV50';
	formatDescription[2].family = B_AVI_FORMAT_FAMILY;
	formatDescription[2].u.avi.codec = 'iv50';
	formatDescription[3].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[3].u.quicktime.codec = 'IV50';
	formatDescription[3].u.quicktime.vendor = 0;
	formatDescription[4].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[4].u.quicktime.codec = 'iv50';
	formatDescription[4].u.quicktime.vendor = 0;

	err = formatObject.MakeFormatFor(formatDescription, 5, &mediaFormat,
	                                 BMediaFormats::B_SET_DEFAULT);
	if(err != B_NO_ERROR) {
		//printf("Indeo5rtEncoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	iv50_encoding = mediaFormat.u.encoded_video.encoding;
}

Encoder *instantiate_encoder(void)
{
	if(iv50_encoding == 0)
		return NULL;
	return new Indeo5Encoder();
}

Indeo5Encoder::IndeoEnvironment::IndeoEnvironment()
{
	encoder_ready = false;
	colorin_ready = false;

	memset(&eienvironment, 0, sizeof(eienvironment));
	eienvironment.bHasTSC = true;
	eienvironment.uTimerScale = 1000;
	
#if __INTEL__
	{
		cpuid_info ci;
		get_cpuid(&ci, 1, 0);
		if(ci.eax_1.features & (1<<23)) {
			//printf("using mmx\n");
			eienvironment.bMMXavailable = true;
		}
		eienvironment.eFamily = ci.eax_1.family;
		eienvironment.uModel = ci.eax_1.model;
		eienvironment.uStepping = ci.eax_1.stepping;
	}
#elif __POWERPC__
	eienvironment.eFamily = CPU_PPC603;
#else
#error unknown processor
#endif
}

Indeo5Encoder::IndeoEnvironment::~IndeoEnvironment()
{
	//printf("Indeo5Decoder::IndeoEnvironment::~IndeoEnvironment()\n");
	if(encoder_ready)
		EncodeShutdown();
	if(colorin_ready)
		ColorInShutdown();
}

ENVIRONMENT_INFO *
Indeo5Encoder::IndeoEnvironment::GetEnvironment()
{
	U32 			uRetVal;			/* Error Return Value */
	if(!encoder_ready) {
		uRetVal = EncodeStartup();
		if (uRetVal != PIA_S_OK) {
			// printf("\n\n*** Aborting... Encode Startup Failed.  Error: %d\n", uRetVal);
			return NULL;
		}
		encoder_ready = true;
	}
	if(!colorin_ready) {
		uRetVal = ColorInStartup();
		if (uRetVal != PIA_S_OK) {
			// printf("\n\n*** Aborting... ColorOut Startup Failed.  Error: %d\n", uRetVal);
			return NULL;
		}
		colorin_ready = true;
	}
	return &eienvironment;
}

Indeo5Encoder::IndeoEnvironment Indeo5Encoder::indeoEnvironment;

Indeo5Encoder::Indeo5Encoder()
{
	colorin_inst_ready = false;
	encoder_inst_ready = false;
	out_buffer = NULL;
	//printf("init indeo\n");

	memset(&EncInst, 0, sizeof(EncInst));
	memset(&EncInput, 0, sizeof(EncInput));
	memset(&EncOutput, 0, sizeof(EncOutput));

	memset(&CinInst, 0, sizeof(CinInst));
	memset(&CinInput, 0, sizeof(CinInput));
	memset(&CinOutput, 0, sizeof(CinOutput));

	//printf("init indeo done\n");
}


Indeo5Encoder::~Indeo5Encoder()
{
	if(colorin_inst_ready) {
		ColorInSequenceEnd(&CinInst);
		ColorInFreePrivateData(&CinInst);
	}
	if(encoder_inst_ready) {
		EncodeSequenceEnd(&EncInst);
		EncodeFreePrivateData(&EncInst);
	}
	if(out_buffer)
		free(out_buffer);
}

status_t 
Indeo5Encoder::Init()
{
	U32 			uRetVal;			/* Error Return Value */
	/* Query to see if all of this is OK */

	EncInst.uFrameTime = 33333;
	EncInst.uFrameScale = 1;
	EncInst.keyKeyFrameRateKind = KEY_FIXED;
	// KEY_AVERAGE, KEY_MAXIMUM_FRAMES_BETWEEN, KEY_FIXED, KEY_NONE
	EncInst.uKeyFrameRate = 15;
	//EncInst.uTargetDataRate = ;
	EncInst.ecscSequenceControlsUsed = EC_FLAGS_VALID | EC_ABSOLUTE_QUALITY_SLIDER;
	EncInput.uAbsoluteQuality = 7500; // 0 - 10000

	EncOutput.cfOutputFormat =  CF_IV50;
	if(EncInst.peiEnvironment == NULL) {
		EncInst.peiEnvironment = indeoEnvironment.GetEnvironment();
		if(EncInst.peiEnvironment == NULL)
			return B_ERROR;
	}
	if(!encoder_inst_ready) {
		uRetVal = EncodeConstantInit(&EncInst);
		if (uRetVal != PIA_S_OK) {
			// printf("\n\n*** Aborting... Encoder Const Init Failed.  Error: %d\n", uRetVal);
			return B_ERROR;
		}
		encoder_inst_ready = true;
	}
	if(!colorin_inst_ready) {
		uRetVal = ColorInConstantInit(&CinInst);
		if (uRetVal != PIA_S_OK) {
			// printf("\n\n*** Aborting... ColorIn Const Init Failed.  Error: %d\n", uRetVal);
			return B_ERROR;
		}
		colorin_inst_ready = true;
	}
	CinOutput.cfOutputFormat = EncInput.cfSrcFormat = CF_PLANAR_YVU9_8BIT;
	return B_NO_ERROR;
}

status_t 
Indeo5Encoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Indeo5 Video");
	strcpy(mci->short_name, "iv50");
	return B_NO_ERROR;
}

status_t 
Indeo5Encoder::InitFormat(media_file_format *mfi,
                          media_format *in_format,
                          media_format *out_format)
{
	in_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	in_format->require_flags = 0;
	out_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	out_format->require_flags = 0;

	if(in_format->type != B_MEDIA_RAW_VIDEO) {
		in_format->type = B_MEDIA_RAW_VIDEO;
		in_format->u.raw_video = media_raw_video_format::wildcard;
	}

	memset(out_format, 0, sizeof(media_format));
	out_format->type = B_MEDIA_ENCODED_VIDEO;
	out_format->u.encoded_video = media_encoded_video_format::wildcard;
	out_format->u.encoded_video.encoding = iv50_encoding;
	out_format->u.encoded_video.output = in_format->u.raw_video;
	
	return B_NO_ERROR;
}

status_t 
Indeo5Encoder::InitColor(color_space *cs)
{
	U32 			uRetVal;			/* Error Return Value */

	if(*cs == B_RGB15) {
		// this is 16 bit in the decoder, but 15 bit here
		CinInput.cfSrcFormat = CF_RGB16_565;
		CinInput.iInputStride = CinInst.dImageDim.w*2;
	}
	else if(*cs == B_RGB32) {
		// this is big endian in the decoder, but little endian here
		CinInput.cfSrcFormat = CF_XRGB32;
		CinInput.iInputStride = CinInst.dImageDim.w*4;
	}
	else {
		*cs = B_RGB32;
		CinInput.cfSrcFormat = CF_XRGB32;
		CinInput.iInputStride = CinInst.dImageDim.w*4;
	}

	//printf("looking for color in format %d size %d %d\n",
	//       CinInput.cfSrcFormat, CinInst.dImageDim.w, CinInst.dImageDim.h);
	uRetVal = ColorInQuery(&CinInst, &CinInput, &CinOutput,
	                       CinInst.dImageDim.h, CinInst.dImageDim.w, FALSE);
	if (uRetVal != PIA_S_OK) {
		// printf("\n\n*** Aborting... Error Colorin Sequence Query Failed.  Error: %d\n", uRetVal);
		return B_ERROR;
	}

	ColorInSequenceEnd(&CinInst);
	//printf("ColorInSequenceSetup ...\n");
	uRetVal = ColorInSequenceSetup(&CinInst, &CinInput, &CinOutput);
	//printf("ColorInSequenceSetup done\n");
	if (uRetVal != PIA_S_OK) {
		//printf("\n\n*** Aborting... ColorIn Sequence Setup Failed.  Error: %d\n", uRetVal);
		return B_ERROR;
	}
	return B_NO_ERROR;
}

status_t 
Indeo5Encoder::SetFormat(media_file_format *mfi,
						 media_format *in_format, media_format *out_format)
{
	U32 		uRetVal;			/* Indeo Error Return Value */
	status_t	err;
	int retry_count = 1;

	err = InitFormat(mfi, in_format, out_format);
	if(err != B_NO_ERROR)
		return err;

	err = Init();
	if(err != B_NO_ERROR)
		return err;

retry:	
	int width  = in_format->u.raw_video.display.line_width;
	int height = in_format->u.raw_video.display.line_count;
	
	uRetVal =  EncodeQuery(&EncInst, &EncInput, &EncOutput, height, width, FALSE);
	if (uRetVal != PIA_S_OK) {
		if(retry_count > 0) {
			in_format->u.raw_video.display.line_width = 320;
			in_format->u.raw_video.display.line_count = 240;
			retry_count--;
			goto retry;
		}
		// printf("\n\n*** Aborting... Error Encode Sequence Query Failed.  Error: %d\n", uRetVal);
		return B_ERROR;
	}
	CinInst.dImageDim.w = width;
	CinInst.dImageDim.h = height;
	CinOutput.iOutputStride = CinInst.dImageDim.w;

	CinInput.cfSrcFormat = CF_UNDEFINED;

	uRetVal = ColorInImageDimInit(&CinInst, &CinInput, &CinOutput);
	if (uRetVal != PIA_S_OK) {
		if(retry_count > 0) {
			in_format->u.raw_video.display.line_width = 320;
			in_format->u.raw_video.display.line_count = 240;
			retry_count--;
			goto retry;
		}
		// printf("\n\n*** Aborting... Error ColorIn Frame Dim Init Failed.  Error: %d\n", uRetVal);
		return B_ERROR;
	}
	
	err = InitColor(&in_format->u.raw_video.display.format);
	if(err != B_NO_ERROR)
		return err;
	
	EncInst.dImageDim.w = width;
	EncInst.dImageDim.h = height;

	return B_NO_ERROR;
}

status_t 
Indeo5Encoder::StartEncoder()
{
	U32 		uRetVal;			/* Indeo Error Return Value */
	status_t	err;

	uRetVal = EncodeImageDimInit(&EncInst, &EncInput, &EncOutput);
	if (uRetVal != PIA_S_OK) {
		//printf("\n\n*** Aborting... Error Encode Frame Dim Init Failed.  Error: %d\n", uRetVal);
		return B_ERROR;
	}
	
	uRetVal = EncodeSequenceSetup(&EncInst, &EncInput, &EncOutput);
	if (uRetVal != PIA_S_OK) {
		//printf("\n\n*** Aborting... Encode Sequence Setup Failed.  Error: %d\n", uRetVal);
		return B_ERROR;
	}

#if 0
	CinOutput.iOutputStride = CinInst.dImageDim.w;

	CinInput.cfSrcFormat = CF_UNDEFINED;

	uRetVal = ColorInImageDimInit(&CinInst, &CinInput, &CinOutput);
	if (uRetVal != PIA_S_OK) {
		printf("\n\n*** Aborting... Error ColorIn Frame Dim Init Failed.  Error: %d\n", uRetVal);
		return B_ERROR;
	}
	
	err = InitColor(in_format->u.raw_video.display.format);
	if(err != B_NO_ERROR)
		return err;
#endif

	size_t out_buffer_size = 0;
	uRetVal = EncodeGetMaxCompressedSize(EncInst.dImageDim.w,
	                                     EncInst.dImageDim.h, &out_buffer_size);
	if (uRetVal != PIA_S_OK) {
		//printf("\n\n*** Aborting... Error EncodeGetMaxCompressedSize Failed.  Error: %d\n", uRetVal);
		return B_ERROR;
	}

	if(out_buffer)
		free(out_buffer);	// size may have changed
	out_buffer = malloc(out_buffer_size);
	if(out_buffer == NULL)
		return B_NO_MEMORY;

	return B_NO_ERROR;
}

status_t 
Indeo5Encoder::Encode(const void *in_buffer, int64 num_frames,
                      media_encode_info *info)
{
	U32 			uRetVal;			/* Error Return Value */

	ASSERT(in_buffer);
	ASSERT(out_buffer);

	if(num_frames != 1)
		return B_ERROR;

	CinInput.uTag = COLORIN_INPUT_INFO_TAG;
	CinOutput.uTag = 0;

	//printf("Indeo5EncoderInstance::Encode %p to %p\n", in_buffer, out_buffer);

	CinInput.pu8InputData = (PU8)in_buffer;

	uRetVal = ColorInFrame(&CinInst, &CinInput, &CinOutput);
	if (uRetVal != PIA_S_OK) {
		//printf("\n\n*** Warning... Error ColorIn Frame Failed.  Error: %d\n", uRetVal);
		return B_ERROR; 
	}
	if (CinOutput.uTag != COLORIN_OUTPUT_INFO_TAG) {
		//printf("\n\n*** Warning... ColorIn Output Info Struct Tag not Initialized.\n");
		return B_ERROR;
	}

	EncInput.pu8UncompressedData = CinOutput.pu8OutputData;
	//printf("CinOutput.pu8OutputData %p\n", CinOutput.pu8OutputData);
	EncInput.iStride = CinOutput.iOutputStride;
	
	EncInput.uTag = ENCODE_FRAME_INPUT_INFO_TAG;
	//printf("EncOutput.uFrameSize %d\n", EncOutput.uFrameSize);
	//EncOutput.pu8CompressedData = malloc(4096);
	//EncOutput.uFrameSize = 4096;
	EncOutput.pu8CompressedData = (PU8)out_buffer;
	
	EncInput.uFrameNumber++;
	//printf("EncInput.uFrameNumber: %d\n", EncInput.uFrameNumber);
	
	uRetVal = EncodeFrame(&EncInst, &EncInput, &EncOutput);
	if (uRetVal != PIA_S_OK) {
		//printf("\n\n*** Warning... Error Encode Frame Failed.  Error: %d\n", uRetVal);
		return B_ERROR;
	}
	//printf("EncOutput.uFrameSize %d\n", EncOutput.uFrameSize);
	
	if(EncOutput.bIsKeyFrame)
		info->flags |= B_MEDIA_KEY_FRAME;
	else
		info->flags &= ~B_MEDIA_KEY_FRAME;

	return WriteChunk((char*)out_buffer, EncOutput.uFrameSize, info);
}

status_t 
Indeo5Encoder::GetEncodeParameters(encode_parameters *parameters) const
{
	memset(parameters, 0, sizeof(encode_parameters));
	parameters->quality = EncInput.uAbsoluteQuality / 10000.0;
	return B_NO_ERROR;
}

status_t 
Indeo5Encoder::SetEncodeParameters(encode_parameters *parameters)
{
	if(parameters->quality < 0.0)
		parameters->quality = 0.0;
	EncInput.uAbsoluteQuality = (U32)(parameters->quality * 10000);
	if(EncInput.uAbsoluteQuality > 10000)
		EncInput.uAbsoluteQuality = 10000;
	return B_NO_ERROR;
}

/* HIVE callback */

PIA_RETURN_STATUS HiveEncodeProgressFunc(
                PTR_ENC_INST            pEncInst,
                PROGRESS_CALL_TYPE      pctType,
                U32                                     uPercentDone)
{
#if 0
	char *t[7] = {
		"PCT_UNDEFINED",
		"PCT_STATUS",
		"PCT_YIELD",
		"PCT_START",
		"PCT_END"
	};
	if(pctType > 0 && pctType < 5)
		printf("indeo encode progress %s: %d\n", t[pctType], uPercentDone);
	else
		printf("indeo encode progress %d\n", uPercentDone);
#endif
	return PIA_S_OK;
}
