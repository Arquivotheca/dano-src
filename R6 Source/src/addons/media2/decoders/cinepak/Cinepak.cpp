#include "Cinepak.h"

#include <interface/GraphicsDefs.h>

int   cpDecompress(void *ptr,unsigned char *data,unsigned char *baseAddr);
void *cpDecompressInit(long real_width,long real_height,color_space cs);
void  cpDecompressCleanup(void *ptr);

using namespace B::Media2;
using namespace B::Raster2;

namespace B {
namespace Private {

CinepakDecoder::CinepakDecoder()
	: BMediaNode("cinepak.decoder")
{
	dptr = NULL;
}

status_t 
CinepakDecoder::Acquired(const void *id)
{
	status_t result=BMediaNode::Acquired(id);
	
	if (result<B_OK)
		return result;
	
	CreateInput();
	
	return B_OK;
}

status_t 
CinepakDecoder::Released(const void *id)
{
	return BMediaNode::Released(id);
}

void 
CinepakDecoder::Connected (BMediaEndpoint::arg localEndpoint,
							IMediaEndpoint::arg,
							const BMediaFormat &format)
{
	using namespace B::Raster2;
	
	if (localEndpoint.ptr()==mInput.ptr())
	{
		mOutput=new BMediaOutput("video_out");
		
		BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_RAW_VIDEO));
		c.And(B_FORMATKEY_WIDTH,format[B_FORMATKEY_WIDTH]);
		c.And(B_FORMATKEY_HEIGHT,format[B_FORMATKEY_HEIGHT]);

		BValue color_spaces;
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
		color_spaces.Overlay(BValue::Int32(B_RGB32));
		color_spaces.Overlay(BValue::Int32(B_RGB16));
		color_spaces.Overlay(BValue::Int32(B_RGB15));		
		color_spaces.Overlay(BValue::Int32(B_YCbCr422));
#else
		color_spaces.Overlay(BValue::Int32(B_RGB32_BIG));
		color_spaces.Overlay(BValue::Int32(B_RGB16_BIG));
		color_spaces.Overlay(BValue::Int32(B_RGB15_BIG));		
#endif
					
		c.And(B_FORMATKEY_COLORSPACE,color_spaces);
		
		mOutput->SetConstraint(c);
		AddEndpoint(mOutput);
	}
	else
	{
		const size_t bytes_per_pixel=format[B_FORMATKEY_COLORSPACE].AsInt32()
										== B_RGB32 ? 4 : 2;
		
		mOutputSize=bytes_per_pixel*format[B_FORMATKEY_WIDTH].AsInt32()
					*format[B_FORMATKEY_HEIGHT].AsInt32();
					
		dptr=cpDecompressInit(format[B_FORMATKEY_WIDTH].AsInt32(),
		                      (format[B_FORMATKEY_HEIGHT].AsInt32()+2)&~2,
		                      ::color_space(format[B_FORMATKEY_COLORSPACE].AsInt32()));
	}
}

void 
CinepakDecoder::Disconnected (BMediaEndpoint::arg localEndpoint,
								IMediaEndpoint::arg)
{
	if (localEndpoint.ptr()==mInput.ptr())
	{
		cpDecompressCleanup(dptr);
		
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

void 
CinepakDecoder::CreateInput()
{
	mInput=new BMediaInput("video_in");
	
	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_ENCODED_VIDEO));
	c.And(B_FORMATKEY_WIDTH,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_HEIGHT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_ENCODING,BValue()
								.Overlay(BValue::String("be:avi:cvid")));
	
	mInput->SetConstraint(c);
	
	AddEndpoint(mInput);	
}

status_t 
CinepakDecoder::HandleBuffer (BMediaInput::arg, BBuffer *buffer)
{
	BBuffer out_buffer;
	
	if (mOutput==NULL || mOutput->AcquireBuffer(&out_buffer)<B_OK)
	{
		buffer->ReleaseBuffer();
		return B_OK;
	}

	cpDecompress(dptr,(uint8 *)buffer->Data(),
					(uint8 *)out_buffer.Data());

	out_buffer.SetRange(0,mOutputSize);	
	buffer->ReleaseBuffer();
	
	if (mOutput->SendBuffer(&out_buffer)<B_OK)
		out_buffer.ReleaseBuffer();
		
	return B_OK;
}

} } // B::Private
