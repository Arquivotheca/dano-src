#include "DVIADPCMDecoder.h"

#include <support2/Debug.h>

using namespace B::Media2;

CDVIADPCMDecoder::CDVIADPCMDecoder()
	: BMediaNode("dvi-adpcm.decoder")
{
}

status_t 
CDVIADPCMDecoder::Acquired(const void *id)
{
	status_t result=BMediaNode::Acquired(id);
	
	if (result<B_OK)
		return result;
	
	CreateInput();
	
	return B_OK;
}

status_t 
CDVIADPCMDecoder::Released(const void *id)
{
	return BMediaNode::Released(id);
}

void 
CDVIADPCMDecoder::Connected(BMediaEndpoint::arg localEndpoint,
						IMediaEndpoint::arg,
						const BMediaFormat &format)
{
	if (localEndpoint.ptr()==mInput.ptr())
	{
		mOutput=new BMediaOutput("audio_out");
		
		BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_RAW_AUDIO));
		c.And(B_FORMATKEY_CHANNEL_COUNT,format[B_FORMATKEY_CHANNEL_COUNT]);
		c.And(B_FORMATKEY_FRAME_RATE,format[B_FORMATKEY_FRAME_RATE]);
		c.And(B_FORMATKEY_BYTE_ORDER,BValue::Int32(B_MEDIA_HOST_ENDIAN));
		c.And(B_FORMATKEY_RAW_AUDIO_TYPE,BValue::Int32(B_AUDIO_INT16));
		
		mOutput->SetConstraint(c);
		AddEndpoint(mOutput);
		
		memset(&mADPCMState,0,sizeof(mADPCMState));
	}
}

void 
CDVIADPCMDecoder::Disconnected(BMediaEndpoint::arg localEndpoint,
							IMediaEndpoint::arg)
{
	if (localEndpoint.ptr()==mInput.ptr())
	{
		if (mOutput!=NULL)
		{
			RemoveEndpoint(mInput);
			mInput=NULL;
		}
	}
	else	// localEndpoint.ptr()==mOutput.ptr()
	{
		if (mInput==NULL)
		{
			RemoveEndpoint(mOutput);
			mOutput=NULL;
			
			CreateInput();
		}
	}
}

status_t 
CDVIADPCMDecoder::HandleBuffer(BMediaInput::arg,
							BBuffer *buffer)
{
	BBuffer out_buffer;
	if (mOutput==NULL || mOutput->AcquireBuffer(&out_buffer)<B_OK)
	{
		buffer->ReleaseBuffer();
		return B_OK;
	}

	ASSERT(out_buffer.Size()>=4*buffer->Size());
	
	adpcm_decoder((char *)buffer->Data(),(short *)out_buffer.Data(),
					2*buffer->Size(),&mADPCMState);
	
	out_buffer.SetRange(0,4*buffer->Size());
	buffer->ReleaseBuffer();
	
	if (mOutput->SendBuffer(&out_buffer)<B_OK)
		out_buffer.ReleaseBuffer();					
		
	return B_OK;
}

void 
CDVIADPCMDecoder::CreateInput()
{
	mInput=new BMediaInput("audio_in");
	
	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_ENCODED_AUDIO));
	c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));

	c.And(B_FORMATKEY_ENCODING,BValue::String("be:dvi_adpcm"));
		
	mInput->SetConstraint(c);
	
	AddEndpoint(mInput);
}

