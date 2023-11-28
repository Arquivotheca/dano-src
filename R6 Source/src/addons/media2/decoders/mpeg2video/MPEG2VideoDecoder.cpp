#include "MPEG2VideoDecoder.h"

#include <support2/Debug.h>
#include <interface/GraphicsDefs.h>

#include <media2/MediaConstraint.h>
#include <support2/StdIO.h>

using namespace B::Media2;

CMPEG2VideoDecoder::CMPEG2VideoDecoder()
	: BMediaNode("mpeg2video"),
	  mDecoderImpl(NULL)
{
}

CMPEG2VideoDecoder::~CMPEG2VideoDecoder()
{
}

status_t 
CMPEG2VideoDecoder::Acquired(const void *)
{
	fInput=new BMediaInput("in");
	
	BMediaConstraint c(B_FORMATKEY_WIDTH,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_WIDTH,BMediaConstraintItem::B_MULTIPLE_OF,BValue::Int32(16));
	c.And(B_FORMATKEY_HEIGHT,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_HEIGHT,BMediaConstraintItem::B_MULTIPLE_OF,BValue::Int32(16));
	c.And(B_FORMATKEY_MEDIA_TYPE,BMediaConstraintItem::B_EQ,BValue::Int32(B::Media2::B_MEDIA_ENCODED_VIDEO));

	c.And(B_FORMATKEY_ENCODING,BValue()
								.Overlay(BValue::String("mpeg1"))
								.Overlay(BValue::String("mpeg2")));
								
	fInput->SetConstraint(c);
	
	AddEndpoint(fInput);
	
	fOutput=new BMediaOutput("out");	
	fOutput->SetConstraint(BMediaConstraint(false));	
	AddEndpoint(fOutput);
	
	return B_OK;
}

status_t
CMPEG2VideoDecoder::Released(const void *)
{
	return B_OK;
}

void 
CMPEG2VideoDecoder::Connected (BMediaEndpoint::arg localEndpoint,
								IMediaEndpoint::arg remoteEndpoint,
								const BMediaFormat &format)
{
	BMediaNode::Connected(localEndpoint,remoteEndpoint,format);
	
	if (localEndpoint.ptr()==fInput.ptr())
	{
		fInputFormat=format;
		
		mDecoderImpl=mpeg2video_create(fInputFormat[B_FORMATKEY_WIDTH].AsInt32(),
										fInputFormat[B_FORMATKEY_HEIGHT].AsInt32(),
										YUV420,
										AcquireOutputBufferCB,
										SendBufferCB,
										this);
										
		BMediaConstraint c(B_FORMATKEY_WIDTH,
							BMediaConstraintItem::B_EQ,
							fInputFormat[B_FORMATKEY_WIDTH]);
		
		c.And(B_FORMATKEY_HEIGHT,
				BMediaConstraintItem::B_EQ,
				fInputFormat[B_FORMATKEY_HEIGHT]);

		c.And(B_FORMATKEY_COLORSPACE,BMediaConstraintItem::B_EQ,
							BValue::Int32(::B_YUV9));

		c.And(B_FORMATKEY_MEDIA_TYPE,BMediaConstraintItem::B_EQ,
							BValue::Int32(B::Media2::B_MEDIA_RAW_VIDEO));

		fOutput->SetConstraint(c);		
	}
	else
	{
		ASSERT(localEndpoint.ptr()==fOutput.ptr());
		
		fOutputFormat=format;
	}
}

void 
CMPEG2VideoDecoder::Disconnected (BMediaEndpoint::arg localEndpoint,
									IMediaEndpoint::arg remoteEndpoint)
{
	if (localEndpoint.ptr()==fInput.ptr())
	{
		mpeg2video_destroy(mDecoderImpl);
		mDecoderImpl=NULL;
	}
	
	BMediaNode::Disconnected(localEndpoint,remoteEndpoint);
}

status_t 
CMPEG2VideoDecoder::HandleBuffer(BMediaInput::arg, B::Media2::BBuffer *buffer)
{
	buffer_t *buf=(buffer_t *)malloc(sizeof(buffer_t));
	buf->data=buffer->Data();
	buf->size=buffer->Size();
	buf->has_start_time=buffer->Info()["StartTime"].IsSpecified();
	buf->start_time=buffer->Info()["StartTime"].AsTime();
	buf->cookie=new BBuffer(*buffer);
	buf->acquire_ref=AcquireBufferCB;
	buf->release_ref=ReleaseBufferCB;
	buf->internal_ref_count=1;
	
	mpeg2video_decode (mDecoderImpl,buf);

	return B_OK;
}

status_t 
CMPEG2VideoDecoder::PropagateMessage (const BMessage &message,
										IMediaEndpoint::arg from,
										media_endpoint_type direction,
										BMediaEndpointVector *visited)
{
	switch (message.What())
	{
		case B_MEDIA_FLUSH_ENDPOINT:
			mpeg2video_flush(mDecoderImpl);
			break;

		case B_MEDIA_ENDPOINT_LATE:
			mpeg2video_late(mDecoderImpl,0 /*unused*/);
			break;
	
		case B_MEDIA_REPEAT_LAST_FRAME:
			mpeg2video_repeat_last_picture(mDecoderImpl);
			break;

		default:
			return BMediaNode::PropagateMessage(message,from,direction,visited);
			break;
	}				

	return B_OK;
}

void 
CMPEG2VideoDecoder::AcquireBufferCB(buffer_t *me)
{
	static_cast<B::Media2::BBuffer *>(me->cookie)->AcquireBuffer();
	atomic_add(&me->internal_ref_count,1);
}

void 
CMPEG2VideoDecoder::ReleaseBufferCB(buffer_t *me)
{
	static_cast<B::Media2::BBuffer *>(me->cookie)->ReleaseBuffer();
	
	if (atomic_add(&me->internal_ref_count,-1)==1)
	{
		delete static_cast<B::Media2::BBuffer *>(me->cookie);
		free(me);
	}
}

status_t 
CMPEG2VideoDecoder::AcquireOutputBufferCB(void *cookie, buffer_t **buf_ptr)
{
	B::Media2::BBuffer buffer;
	status_t result=static_cast<CMPEG2VideoDecoder *>(cookie)->fOutput->AcquireBuffer(&buffer);
	
	if (result<B_OK)
		return result;
		
	buffer_t *buf=(buffer_t *)malloc(sizeof(buffer_t));
	buf->data=buffer.Data();
	buf->size=buffer.Size();
	buf->has_start_time=false;
	buf->cookie=new BBuffer(buffer);
	buf->acquire_ref=AcquireBufferCB;
	buf->release_ref=ReleaseBufferCB;
	buf->internal_ref_count=1;
	
	*buf_ptr=buf;
	
	return B_OK;
}

status_t 
CMPEG2VideoDecoder::SendBufferCB(void *cookie, buffer_t *buf)
{
	B::Media2::BBuffer *buffer=static_cast<B::Media2::BBuffer *>(buf->cookie);

	buffer->EditInfo().RemoveItem(value_ref(BValue::String("StartTime")));
	
	if (buf->has_start_time)
		buffer->EditInfo().Overlay(BValue::String("StartTime"),
									BValue::Time(buf->start_time));
									
	status_t result=static_cast<CMPEG2VideoDecoder *>(cookie)->fOutput->SendBuffer(buffer);
	
	if (result>=B_OK)
	{
		if (atomic_add(&buf->internal_ref_count,-1)==1)
		{
			delete buffer;
			free(buf);
		}		
	}
	
	return result;
}

