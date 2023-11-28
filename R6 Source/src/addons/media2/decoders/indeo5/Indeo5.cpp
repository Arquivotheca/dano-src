#include "Indeo5.h"

#include <interface/GraphicsDefs.h>
#include <interface/Screen.h>

#include <stdio.h>

using namespace B::Media2;

namespace B {
namespace Private {

struct
{
	::color_space	beos;
	COLOR_FORMAT_LIST hive;
} color_space_map[] = {
	{ ::B_RGB32,          CF_BGRX32      },
	{ ::B_RGB32_BIG,      CF_XRGB32      },
#if __INTEL__
	{ ::B_RGB15,          CF_XRGB16_1555 },
	{ ::B_RGB16,          CF_RGB16_565   },
#endif
	{ ::B_CMAP8,          CF_CLUT_8      },
	{ ::B_RGB24,          CF_RGB24       },
	{ ::B_YCbCr422,       CF_YUY2        },
	{ ::B_NO_COLOR_SPACE, CF_UNDEFINED   },
};

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
	: BMediaNode("indeo5.decoder")
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

status_t 
Indeo5Decoder::Acquired(const void *id)
{
	status_t result=BMediaNode::Acquired(id);
	
	if (result<B_OK)
		return result;
	
	CreateInput();
	
	return B_OK;
}

status_t 
Indeo5Decoder::Released(const void *id)
{
	return BMediaNode::Released(id);
}

status_t 
Indeo5Decoder::AcceptInputConnection (IMediaOutput::arg,
										BMediaInput::arg,
										const BMediaFormat &format)
{
	const int32 width=format[B_FORMATKEY_WIDTH].AsInt32();
	const int32 height=format[B_FORMATKEY_HEIGHT].AsInt32();
	
	/* Query to see if all of this is OK */
	
	status_t err = Init();
	if(err != B_NO_ERROR)
		return err;

	PIA_RETURN_STATUS uRetVal =  DecodeQuery(&DecInst, &DecInput, &DecOutput, height, width, FALSE);
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
Indeo5Decoder::AcceptOutputConnection (BMediaOutput::arg,
										IMediaInput::arg,
										const BMediaFormat &format)
{
	const ::color_space space=::color_space(format[B_FORMATKEY_COLORSPACE].AsInt32());
	
	int32 i;
	for (i=0;color_space_map[i].beos!=::B_NO_COLOR_SPACE
				&& color_space_map[i].beos!=space;++i)
	{
	}
	
	if (color_space_map[i].hive == CoutOutput.cfOutputFormat)
		return B_OK;

	if (space==::B_YCbCr422)
		return B_OK;
				
	CoutOutput.cfOutputFormat = color_space_map[i].hive;

	PIA_RETURN_STATUS uRetVal = ColorOutQuery(&CoutInst, &CoutInput, &CoutOutput,
	                        	CoutInst.dImageDim.h, CoutInst.dImageDim.w, FALSE);

	if (uRetVal != PIA_S_OK)
	{
		printf("\n\n*** Aborting... Error Colorout Sequence Query Failed.  Error: %ld\n", uRetVal);
		return B_BAD_VALUE;
	}

	switch (CoutOutput.cfOutputFormat)
	{
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

	if(!palettevalid && space == ::B_CMAP8) {
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

void 
Indeo5Decoder::Connected (BMediaEndpoint::arg localEndpoint,
							IMediaEndpoint::arg,
							const BMediaFormat &format)
{
	if (localEndpoint.ptr()==mInput.ptr())
	{
		mOutput=new BMediaOutput("video_out");
		
		BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_RAW_VIDEO));
		c.And(B_FORMATKEY_WIDTH,format[B_FORMATKEY_WIDTH]);
		c.And(B_FORMATKEY_HEIGHT,format[B_FORMATKEY_HEIGHT]);

		BValue color_spaces;
		for (int32 i=0;color_space_map[i].beos!=::B_NO_COLOR_SPACE;++i)
			color_spaces.Overlay(BValue::Int32(color_space_map[i].beos));
					
		c.And(B_FORMATKEY_COLORSPACE,color_spaces);
		
		mOutput->SetConstraint(c);
		AddEndpoint(mOutput);
	}
}

void 
Indeo5Decoder::Disconnected (BMediaEndpoint::arg localEndpoint,
								IMediaEndpoint::arg)
{
	if (localEndpoint.ptr()==mInput.ptr())
	{
		ColorOutSequenceEnd(&CoutInst);
		ColorOutFreePrivateData(&CoutInst);
		DecodeSequenceEnd(&DecInst);
		DecodeFreePrivateData(&DecInst);
		
		if (mOutput!=NULL)
		{
			RemoveEndpoint(mInput);
			mInput=NULL;
		}
	}
	else
	{
		if (mInput==NULL)
		{
			RemoveEndpoint(mOutput);
			mOutput=NULL;

			CreateInput();
		}
	}
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
Indeo5Decoder::HandleBuffer (BMediaInput::arg, BBuffer *buffer)
{
	BBuffer out_buffer;
	
	if (mOutput==NULL || mOutput->AcquireBuffer(&out_buffer)<B_OK)
	{
		buffer->ReleaseBuffer();
		return B_OK;
	}
	
	const void *srcBuffer=buffer->Data();
	size_t bufSize=buffer->Size();

	U32 			uRetVal;			/* Error Return Value */

	RectSt			rBoundingRect;
	
	DecInput.uSizeCompressedData = bufSize;
	DecInput.pu8CompressedData = (U8*)srcBuffer;
	DecInput.uSequenceID = 0;	// may need to provide this informaion later

	DecOutput.pu8IF09BaseAddress = NULL;
	CoutOutput.pu8OutputData = (uint8*)out_buffer.Data();
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

	uRetVal = ::DecodeFrame(&DecInst, &DecInput, &DecOutput);

	buffer->ReleaseBuffer();

	if (uRetVal != PIA_S_OK) {
		printf("\n\n*** Warning... Error Decode Frame Failed.  Error: %ld\n", uRetVal);
		out_buffer.ReleaseBuffer();
		return B_ERROR;
	}
	if (DecOutput.uTag != DECODE_FRAME_OUTPUT_INFO_TAG) {
		printf("\n\n*** Warning... Decoder Output Info Struct Tag not Initialized.\n");
		out_buffer.ReleaseBuffer();
		return B_ERROR;
	}

	DecodeGetBoundingRectangle(&DecInst, &rBoundingRect);


	if(mOutput->Format()[B_FORMATKEY_COLORSPACE].AsInt32()==::B_YCbCr422) {
		toycbcr(DecOutput.pioOutputData, (uint8*)out_buffer.Data(),
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
			out_buffer.ReleaseBuffer();
			return B_ERROR; 
		}
	}
	//printf("frame done\n");

	if (mOutput->SendBuffer(&out_buffer)<B_OK)
		out_buffer.ReleaseBuffer();
	
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

void 
Indeo5Decoder::CreateInput()
{
	mInput=new BMediaInput("video_in");
	
	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_ENCODED_VIDEO));
	c.And(B_FORMATKEY_WIDTH,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_HEIGHT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_ENCODING,BValue()
								.Overlay(BValue::String("be:avi:iv50"))
								.Overlay(BValue::String("be:avi:IV50")));
	
	mInput->SetConstraint(c);
	
	AddEndpoint(mInput);
}

} } // namespace B::Private
