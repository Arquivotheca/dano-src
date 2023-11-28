/* lib headers */
#include <string.h>
#include <Debug.h>
#include <File.h>
#include <Screen.h>
#include <MediaFormats.h>

#include "Decoder.h"
#include "MediaTrack.h"
#include "Extractor.h"

#include "Indeo5.h"


struct {
	color_space	beos;
	COLOR_FORMAT_LIST hive;
} color_space_map[] = {
	{ B_RGB32,          CF_BGRX32      },
	{ B_RGB32_BIG,      CF_XRGB32      },
#if __INTEL__
	{ B_RGB15,          CF_XRGB16_1555 },
	{ B_RGB16,          CF_RGB16_565   },
#endif
	{ B_CMAP8,          CF_CLUT_8      },
	{ B_RGB24,          CF_RGB24       },
	{ B_YCbCr422,       CF_YUY2        },
	{ B_NO_COLOR_SPACE, CF_UNDEFINED   },
};

media_encoded_video_format::video_encoding my_encoding;
media_format mediaFormat;

void register_decoder(const media_format ** out_format, int32 * out_count)
{
	//printf("Indeo5Decoder loaded\n");
	status_t 					err;
	media_format_description	formatDescription[5];
	const int					formatDescription_count =
		sizeof(formatDescription) / sizeof(media_format_description);

	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	memset(&formatDescription, 0, sizeof(media_format_description));
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

	err = formatObject.MakeFormatFor(formatDescription, formatDescription_count, &mediaFormat);
	if(err != B_NO_ERROR) {
		//printf("Indeo5Decoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	my_encoding = mediaFormat.u.encoded_video.encoding;
	*out_format = &mediaFormat;
	*out_count = 1;
}

Decoder *instantiate_decoder(void)
{
	if(my_encoding == 0) {
		return NULL;
	}
	Indeo5Decoder *decoder = new Indeo5Decoder();
	return decoder;
}

Indeo5Decoder::IndeoEnvironment::IndeoEnvironment()
{
	decoder_ready = false;
	colorout_ready = false;

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

Indeo5Decoder::IndeoEnvironment::~IndeoEnvironment()
{
	//printf("Indeo5Decoder::IndeoEnvironment::~IndeoEnvironment()\n");
	if(decoder_ready)
		DecodeShutdown();
	if(colorout_ready)
		ColorOutShutdown();
}

ENVIRONMENT_INFO *
Indeo5Decoder::IndeoEnvironment::GetEnvironment()
{
	U32 			uRetVal;			/* Error Return Value */
	if(!decoder_ready) {
		uRetVal = DecodeStartup();
		if (uRetVal != PIA_S_OK) {
			printf("\n\n*** Aborting... Encode Startup Failed.  Error: %ld\n", uRetVal);
			return NULL;
		}
		decoder_ready = true;
	}
	if(!colorout_ready) {
		uRetVal = ColorOutStartup();
		if (uRetVal != PIA_S_OK) {
			printf("\n\n*** Aborting... ColorOut Startup Failed.  Error: %ld\n", uRetVal);
			return NULL;
		}
		colorout_ready = true;
	}
	return &eienvironment;
}

Indeo5Decoder::IndeoEnvironment Indeo5Decoder::indeoEnvironment;

Indeo5Decoder::Indeo5Decoder()
{
	decoder_inst_ready = false;
	colorout_inst_ready = false;

	memset(&DecInst, 0, sizeof(DecInst));
	memset(&DecInput, 0, sizeof(DecInput));
	memset(&DecOutput, 0, sizeof(DecOutput));

	memset(&CoutInst, 0, sizeof(CoutInst));
	memset(&CoutInput, 0, sizeof(CoutInput));
	memset(&CoutOutput, 0, sizeof(CoutOutput));
}


Indeo5Decoder::~Indeo5Decoder()
{
	ColorOutSequenceEnd(&CoutInst);
	ColorOutFreePrivateData(&CoutInst);
	DecodeSequenceEnd(&DecInst);
	DecodeFreePrivateData(&DecInst);
}

status_t
Indeo5Decoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Indeo-5 Compression");
	strcpy(mci->short_name, "indeo5");
	return B_OK;
}



//	Sniff() is called when looking for a Decoder for a BMediaTrack.
//				it should return an error if the Decoder cannot handle
//				this BMediaTrack. Otherwise, it should initialize the object.

status_t 
Indeo5Decoder::Sniff(const media_format *in_format,
                     const void *in_info, size_t in_size)
{
	U32 			uRetVal;			/* Error Return Value */
	status_t		err;

	if (in_format->type != B_MEDIA_ENCODED_VIDEO)
		return B_BAD_INDEX;

	if (in_format->u.encoded_video.encoding != my_encoding)
		return B_BAD_TYPE;

	output_format = in_format->u.encoded_video.output;
	int32 width  = in_format->u.encoded_video.output.display.line_width;
	int32 height = in_format->u.encoded_video.output.display.line_count;

	/* Query to see if all of this is OK */
	
	err = Init();
	if(err != B_NO_ERROR)
		return err;

	uRetVal =  DecodeQuery(&DecInst, &DecInput, &DecOutput, height, width, FALSE);
	if (uRetVal != PIA_S_OK) {
		printf("\n\n*** Aborting... Error Decode Sequence Query Failed.  Error: %ld\n", uRetVal);
		return B_BAD_VALUE;
	}
	DecInst.dImageDim.w = width;
	DecInst.dImageDim.h = height;

	uRetVal = DecodeImageDimInit(&DecInst, &DecInput, &DecOutput);
	if (uRetVal != PIA_S_OK) {
		printf("\n\n*** Aborting... Error Decode Frame Dim Init Failed.  Error: %ld\n", uRetVal);
		return B_BAD_VALUE;
	}
	
	/* Sequence Setup needs to be done */
			
	uRetVal = DecodeSequenceSetup(&DecInst, &DecInput, &DecOutput);
	if (uRetVal != PIA_S_OK) {
		printf("\n\n*** Aborting... Encode Sequence Setup Failed.  Error: %ld\n", uRetVal);
		return B_BAD_VALUE;
	}

	CoutInst.coscSequenceControlsUsed |= COUT_BOTTOM_UP_OUTPUT;
	CoutInput.cfInputFormat = DecOutput.cfOutputFormat;

	CoutOutput.odDestination =  DEST_BUFFER ;
	ColorOutSetTransparencyKind(&CoutInst, TK_IGNORE);
	/* Set Frame Size in the instance structure, then */
	/* do frame size dependant decoder initialization */

	CoutInput.dInputDim.w = CoutInst.dImageDim.w = width;
	CoutInput.dInputDim.h = CoutInst.dImageDim.h = height;

	CoutOutput.cfOutputFormat = CF_UNDEFINED;

	uRetVal = ColorOutImageDimInit(&CoutInst, &CoutInput, &CoutOutput);
	if (uRetVal != PIA_S_OK) {
		printf("\n\n*** Aborting... Error ColorOut Frame Dim Init Failed.  Error: %ld\n", uRetVal);
		return B_BAD_VALUE;
	}
	return B_OK;
}

status_t 
Indeo5Decoder::Init()
{
	U32 			uRetVal;			/* Error Return Value */

	if(DecInst.peiEnvironment == NULL) {
		DecInst.peiEnvironment = indeoEnvironment.GetEnvironment();
		if(DecInst.peiEnvironment == NULL)
			return B_ERROR;
	}

	if(!decoder_inst_ready) {
		DecInput.cfInputFormat =  CF_IV50;
		uRetVal = DecodeConstantInit(&DecInst);
		if (uRetVal != PIA_S_OK) {
			printf("\n\n*** Aborting... Decoder Const Init Failed.  Error: %ld\n", uRetVal);
			return B_ERROR;
		}
		DecInst.dcfcFrameControlsUsed = DC_FRAME_FLAGS_VALID | DC_TRANSPARENCY;
		DecInst.dcscSequenceControlsUsed = DC_SEQUENCE_FLAGS_VALID | DC_ACCESS_KEY;
	
		/* Specifies Output color format */
	
		DecOutput.cfOutputFormat = CF_PLANAR_YVU9_8BIT;
		DecOutput.cfOutputFormat = CF_PLANAR_YVU9_8BIT;
		DecOutput.odDestination = DEST_BUFFER;
		decoder_inst_ready = true;
	}
	if(!colorout_inst_ready) {
		uRetVal = ColorOutConstantInit(&CoutInst);
		if (uRetVal != PIA_S_OK) {
			printf("\n\n*** Aborting... Decoder Const Init Failed.  Error: %ld\n", uRetVal);
			return B_ERROR;
		}
		CoutInst.cofcFrameControlsUsed =
			COUT_FRAME_FLAGS_VALID |
			COUT_TRANSPARENCY_STREAM_MASK |
			COUT_USE_FIXED_PALETTE;
		CoutInst.coscSequenceControlsUsed = COUT_SEQUENCE_FLAGS_VALID;
		if(!(CoutInst.cociInfo.cofcSupportedFrameControls & COUT_USE_FIXED_PALETTE)) {
			printf(" COUT_USE_FIXED_PALETTE not set, %08ld\n", CoutInst.cociInfo.cofcSupportedFrameControls);
			return B_ERROR;
		}
		palettevalid = false;
		colorout_inst_ready = true;
	}
	return B_NO_ERROR;
}

status_t 
Indeo5Decoder::InitColor(media_raw_video_format *rvf)
{
	U32 			uRetVal;			/* Error Return Value */
	
	int i;
	for(i = 0; color_space_map[i].beos != B_NO_COLOR_SPACE; i++) {
		if(color_space_map[i].beos == rvf->display.format)
			break;
	}

	if(color_space_map[i].beos == B_NO_COLOR_SPACE) {
		//printf("unsupported color_space\n");
		i = 0;
		rvf->display.format = color_space_map[i].beos;
		rvf->display.bytes_per_row =
			media_raw_video_format::wildcard.display.bytes_per_row;
	}

	if(color_space_map[i].hive == CoutOutput.cfOutputFormat &&
	   CoutOutput.iOutputStride == (int32)rvf->display.bytes_per_row) {
		return B_NO_ERROR;
	}

	//printf("InitColor: cs = 0x%x\n", cs);
	output_format.display.format = rvf->display.format;

	if(rvf->display.format == B_YCbCr422) {
		rvf->display.bytes_per_row = 2 * CoutInst.dImageDim.w;
		//printf("using YCbCr\n");
		return B_NO_ERROR;
	}
	//printf("using Cout\n");

	//CoutOutput.cfOutputFormat = CF_XRGB32;
	//CoutOutput.cfOutputFormat = CF_BGRX32;
	CoutOutput.cfOutputFormat = color_space_map[i].hive;

#if 0
	printf("u16NumberOfAlgorithms: %d\n", CoutInst.cociInfo.listOutputFormats.u16NumberOfAlgorithms);
	for(int i = 0; i < CoutInst.cociInfo.listOutputFormats.u16NumberOfAlgorithms; i++) {
		printf("color convertion: %d\n", CoutInst.cociInfo.listOutputFormats.eList[i]);
	}
#endif
	//printf("looking for color format %d\n", CoutOutput.cfOutputFormat);
	uRetVal = ColorOutQuery(&CoutInst, &CoutInput, &CoutOutput,
	                        CoutInst.dImageDim.h, CoutInst.dImageDim.w, FALSE);
	if (uRetVal != PIA_S_OK) {
		printf("\n\n*** Aborting... Error Colorout Sequence Query Failed.  Error: %ld\n", uRetVal);
		return B_BAD_VALUE;
	}

	if(rvf->display.bytes_per_row ==
	   media_raw_video_format::wildcard.display.bytes_per_row) {
		switch (CoutOutput.cfOutputFormat) {
	 	case CF_BGRX32:
	 	case CF_XRGB32:
			CoutOutput.iOutputStride = CoutInst.dImageDim.w*4;
			break;
	
	 	case CF_RGB24:
			CoutOutput.iOutputStride = CoutInst.dImageDim.w*3;
			break;
	
		case CF_XRGB16_1555:
		case CF_RGB16_565:
			CoutOutput.iOutputStride = CoutInst.dImageDim.w*2;
			break;
	
		case CF_CLUT_8:
		case CF_GRAY_8:
			CoutOutput.iOutputStride = CoutInst.dImageDim.w;
			break;
	
		case CF_GRAY_1:
			CoutOutput.iOutputStride = CoutInst.dImageDim.w/8;
			break;
		}
		rvf->display.bytes_per_row = CoutOutput.iOutputStride;
	}
	else {
		CoutOutput.iOutputStride = rvf->display.bytes_per_row;
	}

	if(!palettevalid && rvf->display.format == B_CMAP8) {
		BScreen screen;
		if(!screen.IsValid()) {
			printf("BScreen invalid\n");
		}
		else {
			BGR_PALETTE bgrpalette;
			BGR_ENTRY bgrcolors[256];
			bgrpalette.u16Tag = BGR_PALETTE_TAG;
			bgrpalette.u16NumberOfEntries = 256;
			bgrpalette.pbgrTable = &bgrcolors[0];
			memcpy(bgrcolors, screen.ColorMap()->color_list, 256*4);
    		for (int i = 0 ; i < 256 ; i++) {
    			uint8 u8Value = bgrcolors[i].u8B;
    			bgrcolors[i].u8B = bgrcolors[i].u8R;
    			bgrcolors[i].u8R = u8Value;
    		}
			//memset(bgrcolors, 0, 256*4);
			PIA_Boolean f;
			uRetVal = ColorOutUseFixedPalette(&CoutInst, &bgrpalette);
			if (uRetVal != PIA_S_OK) {
				printf("ColorOutUseFixedPalette failed: %ld\n", uRetVal);
			}
			uRetVal = ColorOutUseThisPalette(&CoutInst, &bgrpalette, &f);
			if (uRetVal != PIA_S_OK) {
				printf("ColorOutUseThisPalette failed: %ld\n", uRetVal);
			}
			else {
				palettevalid = true;
			}
		}
	}
	ColorOutSequenceEnd(&CoutInst);
	uRetVal = ColorOutSequenceSetup(&CoutInst, &CoutInput, &CoutOutput);
	if (uRetVal != PIA_S_OK) {
		printf("\n\n*** Aborting... ColorOut Sequence Setup Failed.  Error: %ld\n", uRetVal);
		return B_ERROR;
	}

	return B_NO_ERROR;
}


//	Format() gets upon entry the format that the application wants and
//				returns that format untouched if it can output it, or the
//				closest match if it cannot output that format.

status_t 
Indeo5Decoder::Format(media_format *inout_format)
{
	status_t err;

	if(inout_format->type != B_MEDIA_RAW_VIDEO) {
		inout_format->type = B_MEDIA_RAW_VIDEO;
		inout_format->u.raw_video = media_raw_video_format::wildcard;
	}

	media_raw_video_format *rvf = &inout_format->u.raw_video;

	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;

	if(rvf->field_rate == media_raw_video_format::wildcard.field_rate) {
		rvf->field_rate = output_format.field_rate;
	}
	rvf->interlace = output_format.interlace;
	rvf->first_active = output_format.first_active;
	rvf->last_active = output_format.last_active;
	rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
	rvf->pixel_width_aspect = output_format.pixel_width_aspect;
	rvf->pixel_height_aspect = output_format.pixel_height_aspect;
	rvf->display.line_width = output_format.display.line_width;
	rvf->display.line_count = output_format.display.line_count;
	rvf->display.pixel_offset = 0;
	rvf->display.line_offset = 0;

	err = InitColor(rvf);
	if(err != B_NO_ERROR) {
		return err;
	}
	return B_OK;
}

static void
toycbcr(PLANAR_IO src, uint8 *dest, int width, int height)
{
	uint8 *Y = src.pu8Y;
	uint8 *Cb = src.pu8U;
	uint8 *Cr = src.pu8V;
	
	//printf("toycbcr %p %p %p -> %p (%d*%d)\n", Y, Cb, Cr, dest, width, height);
	//printf("uYPitch = %d\n", src.uYPitch);
	//printf("uVUPitch = %d\n", src.uVUPitch);
	
	for(int y=0; y<height; y++) {
		for(int x=0; x<width; x+=4) {
			*dest++ = *Y++;
			*dest++ = *Cb;
			*dest++ = *Y++;
			*dest++ = *Cr;
			*dest++ = *Y++;
			*dest++ = *Cb++;
			*dest++ = *Y++;
			*dest++ = *Cr++;
		}
		Y += src.uYPitch-width;
		if((y & 3) != 3) {
			Cb -= width/4;
			Cr -= width/4;
		}
		else {
			Cb += src.uVUPitch - width/4;
			Cr += src.uVUPitch - width/4;
		}
	}
}


status_t 
Indeo5Decoder::Decode(void *out_buffer, int64 *frame_count, media_header *mh,
                      media_decode_info *info)
{
	const void		*srcBuffer;
	size_t			bufSize;
	status_t		err;

	err = GetNextChunk(&srcBuffer, &bufSize, mh, info);
	if (err != B_OK)
		return err;

	ASSERT(srcBuffer);
	ASSERT(out_buffer);
	
	U32 			uRetVal;			/* Error Return Value */

	RectSt			rBoundingRect;
	
	DecInput.uSizeCompressedData = bufSize;
	DecInput.pu8CompressedData = (U8*)srcBuffer;
	DecInput.uSequenceID = 0;	// may need to provide this informaion later

	DecOutput.pu8IF09BaseAddress = NULL;
	CoutOutput.pu8OutputData = (uint8*)out_buffer;
	{
		RectSt vr;
		vr.r =	0;
		vr.c =	0;
		vr.w =	DecInst.dImageDim.w;
		vr.h =	DecInst.dImageDim.h;
		DecodeSetViewRect(&DecInst, vr);
		ColorOutSetViewRect(&CoutInst, vr);
    }

	DecOutput.uTag = CoutOutput.uTag = 0;

	if(bufSize > 0) {

		uRetVal = ::DecodeFrame(&DecInst, &DecInput, &DecOutput);
	
		if (uRetVal != PIA_S_OK) {
			printf("\n\n*** Warning... Error Decode Frame Failed.  Error: %ld\n", uRetVal);
			return B_ERROR;
		}
		if (DecOutput.uTag != DECODE_FRAME_OUTPUT_INFO_TAG) {
			printf("\n\n*** Warning... Decoder Output Info Struct Tag not Initialized.\n");
			return B_ERROR;
		}
	}
	DecodeGetBoundingRectangle(&DecInst, &rBoundingRect);


	if(output_format.display.format == B_YCbCr422) {
		toycbcr(DecOutput.pioOutputData, (uint8*)out_buffer,
		        DecInst.dImageDim.w, DecInst.dImageDim.h);
	}
	else {
		ColorOutSetBoundingRectangle(&CoutInst, rBoundingRect);
		ColorOutSetTransparencyKind(&CoutInst, TK_IGNORE);
		CoutInst.eMode = DecInst.eMode;
		CoutInput.pioInputData = DecOutput.pioOutputData;
		uRetVal = ColorOutFrame(&CoutInst, &CoutInput, &CoutOutput);
		if (uRetVal != PIA_S_OK) {
			printf("\n\n*** Warning... Error ColorOut Frame Failed.  Error: %ld\n", uRetVal);
			return B_ERROR; 
		}
	}
	//printf("frame done\n");

	*frame_count = 1;
	
	return B_OK;
}

