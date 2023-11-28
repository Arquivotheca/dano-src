#include "CCITTDecoder.h"

#include <support2/Debug.h>

namespace B {
namespace Private {

using namespace B::Media2;

CCITTDecoder::CCITTDecoder()
	: BMediaNode("ccittadpcm.decoder")
{
}

status_t 
CCITTDecoder::Acquired(const void *id)
{
	status_t result=BMediaNode::Acquired(id);
	
	if (result<B_OK)
		return result;
	
	CreateInput();
	
	return B_OK;
}

status_t 
CCITTDecoder::Released(const void *id)
{
	return BMediaNode::Released(id);
}

void 
CCITTDecoder::Connected(BMediaEndpoint::arg localEndpoint,
						IMediaEndpoint::arg,
						const BMediaFormat &format)
{
	if (localEndpoint.ptr()==mInput.ptr())
	{
		if (format[B_FORMATKEY_ENCODING].AsString()=="be:g721_4")
			mFormatIndex=0;
		else if (format[B_FORMATKEY_ENCODING].AsString()=="be:g723_3")
			mFormatIndex=1;
		else
			mFormatIndex=2;			
		
		g72x_init_state(&mState);
		
		mOutput=new BMediaOutput("audio_out");
		
		BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_RAW_AUDIO));
		c.And(B_FORMATKEY_CHANNEL_COUNT,format[B_FORMATKEY_CHANNEL_COUNT]);
		c.And(B_FORMATKEY_FRAME_RATE,format[B_FORMATKEY_FRAME_RATE]);
		c.And(B_FORMATKEY_BYTE_ORDER,BValue::Int32(B_MEDIA_HOST_ENDIAN));
		c.And(B_FORMATKEY_RAW_AUDIO_TYPE,BValue::Int32(B_AUDIO_INT16));
		
		mOutput->SetConstraint(c);
		AddEndpoint(mOutput);		
	}
	else
	{
	}
}

void 
CCITTDecoder::Disconnected(BMediaEndpoint::arg localEndpoint,
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
CCITTDecoder::HandleBuffer(BMediaInput::arg,
							BBuffer *buffer)
{
	BBuffer out_buffer;
	
	size_t count=buffer->Size();
	
	switch (mFormatIndex)
	{
		case 0:
			count=(count*8)/4;
			break;
		
		case 1:
			count=(count*8)/3;
			//NOTE: it is assumed that (count*8)%3 == 0
			break;
		
		case 2:
			count=(count*8)/5;
			//NOTE: it is assumed that (count*8)%5 == 0
			break;
		
		default:
			TRESPASS();
			break;
	}

	uint32 value=0;
	int32 valid_bits=0;
	
	const uint8 *in_ptr=(const uint8 *)buffer->Data();

	while (count>0)
	{
		if (mOutput==NULL || mOutput->AcquireBuffer(&out_buffer)<B_OK)
		{
			buffer->ReleaseBuffer();
			return B_OK;
		}

		int16 *out_ptr=(int16 *)out_buffer.Data();
		
		const size_t copy_count=min_c(count,out_buffer.Size()/sizeof(int16));
		
		switch (mFormatIndex)
		{
			case 0:
			{
				for (size_t i=0;i<copy_count;++i)
				{
					if (valid_bits<4)
					{
						value=(*in_ptr++)<<valid_bits;
						valid_bits+=8;
					}
					
					out_ptr[i]=g721_decoder(value&0x0f,AUDIO_ENCODING_LINEAR,&mState);
					
					value>>=4;
					valid_bits-=4;
				}
				
				break;
			}

			case 1:
			{
				for (size_t i=0;i<copy_count;++i)
				{
					if (valid_bits<3)
					{
						value=(*in_ptr++)<<valid_bits;
						valid_bits+=8;
					}
					
					out_ptr[i]=g723_24_decoder(value&0x07,AUDIO_ENCODING_LINEAR,&mState);
					
					value>>=3;
					valid_bits-=3;
				}
				
				break;
			}

			case 2:
			{
				for (size_t i=0;i<copy_count;++i)
				{
					if (valid_bits<5)
					{
						value=(*in_ptr++)<<valid_bits;
						valid_bits+=8;
					}
					
					out_ptr[i]=g723_40_decoder(value&0x1f,AUDIO_ENCODING_LINEAR,&mState);
					
					value>>=5;
					valid_bits-=5;
				}
				
				break;
			}
			
			default:
				TRESPASS();
				break;
		}
		
		count-=copy_count;
		
		out_buffer.SetRange(0,copy_count*sizeof(int16));
		
		if (mOutput->SendBuffer(&out_buffer)<B_OK)
			out_buffer.ReleaseBuffer();	
	}
			
	buffer->ReleaseBuffer();
	
	return B_OK;
}

void 
CCITTDecoder::CreateInput()
{
	mInput=new BMediaInput("audio_in");
	
	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_ENCODED_AUDIO));
	c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));

	c.And(B_FORMATKEY_ENCODING,BValue()
							.Overlay(BValue::String("be:g721_4"))
							.Overlay(BValue::String("be:g723_3"))
							.Overlay(BValue::String("be:g723_5")));

	mInput->SetConstraint(c);
	
	AddEndpoint(mInput);
}

} } // B::Private
