#include "AC3Decoder.h"

#include <malloc.h>

using namespace B::Media2;

CAC3Decoder::CAC3Decoder()
	: BMediaNode("AC3Decoder"),
	  mInput(NULL),
	  mOutput(NULL),
	  mDecoderImpl(NULL)
{
}

CAC3Decoder::~CAC3Decoder()
{
} 

status_t 
CAC3Decoder::Acquired(const void *)
{
	mInput=new BMediaInput("in");
	
	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BMediaConstraintItem::B_EQ,BValue::Int32(B::Media2::B_MEDIA_ENCODED_AUDIO));	
	c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_ENCODING,BValue::String("be:ac3audio"));
	
	mInput->SetConstraint(c);
	
	AddEndpoint(mInput);
	
	return B_OK;
}

status_t
CAC3Decoder::Released(const void *)
{
	return B_OK;
}

void 
CAC3Decoder::Connected (BMediaEndpoint::arg localEndpoint,
								IMediaEndpoint::arg remoteEndpoint,
								const BMediaFormat &format)
{
	BMediaNode::Connected(localEndpoint,remoteEndpoint,format);
	
	if (localEndpoint.ptr()==mInput.ptr())
	{
		mOutput=new BMediaOutput("out");
		
		BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BMediaConstraintItem::B_EQ,BValue::Int32(B::Media2::B_MEDIA_RAW_AUDIO));	

		c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_ONE_OF,
				BValue().Overlay(BValue::Int32(1))
						.Overlay(BValue::Int32(2))
						.Overlay(format[B_FORMATKEY_CHANNEL_COUNT]));
		
		c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_EQ,
						format[B_FORMATKEY_FRAME_RATE]);
						
		c.And(B_FORMATKEY_BUFFER_FRAMES,BMediaConstraintItem::B_EQ,BValue::Int32(6*256));
		c.And(B_FORMATKEY_BYTE_ORDER,BMediaConstraintItem::B_EQ,BValue::Int32(B::Media2::B_MEDIA_HOST_ENDIAN));
		c.And(B_FORMATKEY_RAW_AUDIO_TYPE,BMediaConstraintItem::B_EQ,BValue::Int32(B_AUDIO_INT16));

		mOutput->SetConstraint(c);
		
		AddEndpoint(mOutput);
	}
	else
	{
		output_audio_coding_mode_t output_coding_mode;
		
		switch (format[B_FORMATKEY_CHANNEL_COUNT].AsInteger())
		{
			case 1:
				output_coding_mode=C_MONO;
				break;

			case 2:
				output_coding_mode=C_CONVENTIONAL_STEREO;
				break;

			default:
				output_coding_mode=C_PASS_THROUGH;
				break;				
		}
		
		mDecoderImpl=ac3audio_create(output_coding_mode,
										AcquireOutputBufferCB,
										SendBufferCB,
										this);
	}
}

void 
CAC3Decoder::Disconnected (BMediaEndpoint::arg localEndpoint,
									IMediaEndpoint::arg remoteEndpoint)
{
	if (localEndpoint.ptr()==mOutput.ptr())
	{
		ac3audio_destroy(mDecoderImpl);
		mDecoderImpl=NULL;
	}
	
	BMediaNode::Disconnected(localEndpoint,remoteEndpoint);
}

status_t 
CAC3Decoder::HandleBuffer(BMediaInput::arg, B::Media2::BBuffer *buffer)
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
	
	ac3audio_decode (mDecoderImpl,buf);

	return B_OK;
}

void 
CAC3Decoder::AcquireBufferCB(buffer_t *me)
{
	static_cast<B::Media2::BBuffer *>(me->cookie)->AcquireBuffer();
	atomic_add(&me->internal_ref_count,1);
}

void 
CAC3Decoder::ReleaseBufferCB(buffer_t *me)
{
	static_cast<B::Media2::BBuffer *>(me->cookie)->ReleaseBuffer();
	
	if (atomic_add(&me->internal_ref_count,-1)==1)
	{
		delete static_cast<B::Media2::BBuffer *>(me->cookie);
		free(me);
	}
}

status_t 
CAC3Decoder::AcquireOutputBufferCB(void *cookie, buffer_t **buf_ptr)
{
	B::Media2::BBuffer buffer;
	status_t result=static_cast<CAC3Decoder *>(cookie)->mOutput->AcquireBuffer(&buffer);
	
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
CAC3Decoder::SendBufferCB(void *cookie, buffer_t *buf)
{
	B::Media2::BBuffer *buffer=static_cast<B::Media2::BBuffer *>(buf->cookie);
	
	if (buf->has_start_time)
		buffer->EditInfo().Overlay(BValue::String("StartTime"),
									BValue::Time(buf->start_time));
									

	status_t result=static_cast<CAC3Decoder *>(cookie)->mOutput->SendBuffer(buffer);
	
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

status_t 
CAC3Decoder::PropagateMessage (const BMessage &message,
										IMediaEndpoint::arg from,
										media_endpoint_type direction,
										BMediaEndpointVector *visited)
{
	switch (message.What())
	{
		case B_MEDIA_FLUSH_ENDPOINT:
			ac3audio_flush(mDecoderImpl);
			break;

		default:
			return BMediaNode::PropagateMessage(message,from,direction,visited);
			break;
	}
	
	return B_OK;
}

