#include "MSADPCMDecoder.h"

#include "RIFFTypes.h"
#include "msadpcm_decode.h"

#include <support2/Debug.h>

namespace B {
namespace Private {

using namespace B::Media2;

CMSADPCMDecoder::CMSADPCMDecoder()
	: BMediaNode("msadpcm.decoder"),
	  mHeaderIn(NULL),
	  mTempOutputBuffer(NULL)
{
}

status_t 
CMSADPCMDecoder::Acquired(const void *id)
{
	status_t result=BMediaNode::Acquired(id);
	
	if (result<B_OK)
		return result;
	
	CreateInput();
	
	return B_OK;
}

status_t 
CMSADPCMDecoder::Released(const void *id)
{
	return BMediaNode::Released(id);
}

status_t 
CMSADPCMDecoder::AcceptInputConnection(IMediaOutput::arg,
										BMediaInput::arg,
										const BMediaFormat &format)
{
	const BValue &info=format[B_FORMATKEY_INFO];
	
	if (!info.IsDefined())
		return B_BAD_VALUE;
		
	if (format[B_FORMATKEY_ENCODING].AsString()=="be:wav:adpcm")
	{
		if (info.Length()<sizeof(wav_meta_data_t))
			return B_BAD_VALUE;	
	}
	else
	{
		// be:avi:adpcm

		const AVIAUDSHeader *aviaudsheader=(const AVIAUDSHeader *)info.Data();

		if (info.Length()<22)
			return B_BAD_VALUE;				
		
		int16 num_coeff=B_LENDIAN_TO_HOST_INT16(aviaudsheader->NumCoefficients);

		if (info.Length()<size_t(22+num_coeff*4))
			return B_BAD_VALUE;
			
		if (B_LENDIAN_TO_HOST_INT16(aviaudsheader->Format)!=2)
			return B_BAD_TYPE;
	}
	
	return B_OK;
}

void 
CMSADPCMDecoder::Connected(BMediaEndpoint::arg localEndpoint,
						IMediaEndpoint::arg,
						const BMediaFormat &format)
{
	if (localEndpoint.ptr()==mInput.ptr())
	{
		int32 num_coeff;
		const int16 *coeff;
		
		const BValue &info=format[B_FORMATKEY_INFO];

		if (format[B_FORMATKEY_ENCODING].AsString()=="be:wav:adpcm")
		{			
			const wav_meta_data_t *meta_data=(const wav_meta_data_t *)info.Data();
			
			num_coeff=meta_data->num_coeff;
			coeff=meta_data->coeff;
			mSamplesPerBlock=format[B_FORMATKEY_DECODED_BUFFER_SIZE].AsInt32()
								/(sizeof(int16)*format[B_FORMATKEY_CHANNEL_COUNT].AsInt32());								
		}
		else
		{
			// be:avi:adpcm
			
			const AVIAUDSHeader *aviaudsheader=(const AVIAUDSHeader *)info.Data();

			num_coeff=B_LENDIAN_TO_HOST_INT16(aviaudsheader->NumCoefficients);
			coeff=(const int16 *)aviaudsheader->Coefficients;
			mSamplesPerBlock=B_LENDIAN_TO_HOST_INT16(aviaudsheader->SamplesPerBlock);
		}
		
		mHeaderIn=(ADPCMWaveFormat *)malloc(sizeof(ADPCMWaveFormat)+sizeof(ADPCMCoefSet)*(num_coeff-1));

		float frame_rate;
		format[B_FORMATKEY_FRAME_RATE].GetFloat(&frame_rate);
		
		mHeaderIn->wfx.nSamplesPerSec=(uint32)frame_rate;
		mHeaderIn->wfx.nChannels=format[B_FORMATKEY_CHANNEL_COUNT].AsInt32();
		mHeaderIn->wSamplesPerBlock=mSamplesPerBlock;
		
		mHeaderIn->wNumCoef=num_coeff;
		
		for (int32 i=0;i<num_coeff;++i)
		{
			mHeaderIn->aCoef[i].iCoef1=B_LENDIAN_TO_HOST_INT16(coeff[2*i]);
			mHeaderIn->aCoef[i].iCoef2=B_LENDIAN_TO_HOST_INT16(coeff[2*i+1]);
		}

		mHeaderOut.wf.nChannels=format[B_FORMATKEY_CHANNEL_COUNT].AsInt32();
		mHeaderOut.wf.nSamplesPerSec=(uint32)frame_rate;
		
		mOutput=new BMediaOutput("audio_out");
		
		BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_RAW_AUDIO));
		c.And(B_FORMATKEY_CHANNEL_COUNT,format[B_FORMATKEY_CHANNEL_COUNT]);
		c.And(B_FORMATKEY_FRAME_RATE,format[B_FORMATKEY_FRAME_RATE]);
		c.And(B_FORMATKEY_BYTE_ORDER,BValue::Int32(B_MEDIA_HOST_ENDIAN));

		c.And(B_FORMATKEY_RAW_AUDIO_TYPE,BValue()
											.Overlay(BValue::Int32(B_AUDIO_INT16))
											.Overlay(BValue::Int32(B_AUDIO_UINT8)));
		
		mOutput->SetConstraint(c);
		AddEndpoint(mOutput);		
	}
	else
	{
		mHeaderOut.wBitsPerSample
			= (format[B_FORMATKEY_RAW_AUDIO_TYPE].AsInt32() & B_AUDIO_SIZE_MASK)*8;
			
		mOutputBlockSize=mSamplesPerBlock*(mHeaderOut.wBitsPerSample/8);

		mTempOutputBuffer=malloc(mOutputBlockSize);
		mTempOutputBufferLength=0;
	}
}

void 
CMSADPCMDecoder::Disconnected(BMediaEndpoint::arg localEndpoint,
							IMediaEndpoint::arg)
{
	if (localEndpoint.ptr()==mInput.ptr())
	{
		free(mHeaderIn);
		mHeaderIn=NULL;
		
		if (mOutput!=NULL)
		{
			RemoveEndpoint(mInput);
			mInput=NULL;
		}
	}
	else	// localEndpoint.ptr()==mOutput.ptr()
	{
		free(mTempOutputBuffer);
		mTempOutputBuffer=NULL;
		
		if (mInput==NULL)
		{
			RemoveEndpoint(mOutput);
			mOutput=NULL;
			
			CreateInput();
		}
	}
}

status_t 
CMSADPCMDecoder::HandleBuffer(BMediaInput::arg,
							BBuffer *buffer)
{
	BBuffer out_buffer;
	
	if (mOutput==NULL || mOutput->AcquireBuffer(&out_buffer)<B_OK)
	{
		buffer->ReleaseBuffer();
		return B_OK;
	}
	
	if (out_buffer.Size()<mOutputBlockSize)
	{
		mTempOutputBufferLength=adpcmDecode4Bit(mHeaderIn,(const char *)buffer->Data(),
										&mHeaderOut,(char *)mTempOutputBuffer,
										buffer->Size());
	}
	else
	{
		uint32 count=adpcmDecode4Bit(mHeaderIn,(const char *)buffer->Data(),
										&mHeaderOut,(char *)out_buffer.Data(),
										buffer->Size());

		out_buffer.SetRange(0,count);
		
		if (mOutput->SendBuffer(&out_buffer)<B_OK)
			out_buffer.ReleaseBuffer();
	}
	
	buffer->ReleaseBuffer();										
	
	while (mTempOutputBufferLength>0)
	{
		if (mOutput->AcquireBuffer(&out_buffer)<B_OK)
		{
			mTempOutputBufferLength=0;
			return B_OK;
		}
		
		size_t size=min_c(mTempOutputBufferLength,out_buffer.Size());
		memcpy(out_buffer.Data(),mTempOutputBuffer,size);
		memmove(mTempOutputBuffer,(const char *)mTempOutputBuffer+size,mTempOutputBufferLength-size);
		mTempOutputBufferLength-=size;
	}
	
	return B_OK;
}

void 
CMSADPCMDecoder::CreateInput()
{
	mInput=new BMediaInput("audio_in");
	
	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_ENCODED_AUDIO));
	c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));

	c.And(B_FORMATKEY_INFO,BMediaConstraintItem::B_NE,BValue::undefined);

	BMediaConstraint c1(B_FORMATKEY_ENCODING,BValue::String("be:avi:msadpcm"));

	BMediaConstraint c2(B_FORMATKEY_ENCODING,BValue::String("be:wav:msadpcm"));
	c2.And(B_FORMATKEY_DECODED_BUFFER_SIZE,BMediaConstraintItem::B_GE,BValue::Int32(1));
	
	c1.Or(c2);
	c.And(c1);
	
	mInput->SetConstraint(c);
	
	AddEndpoint(mInput);
}

} } // B::Private
