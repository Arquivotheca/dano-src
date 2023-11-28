#ifndef _MSADPCM_DECODER_H
#define _MSADPCM_DECODER_H

#include <media2/MediaNode.h>
#include <msmedia.h>

namespace B {
namespace Private {

class CMSADPCMDecoder : public B::Media2::BMediaNode
{
	struct wav_meta_data_t
	{
		uint32 num_coeff;
		int16 coeff[2*MSADPCM_MAX_COEF];
	};

	B::Media2::BMediaInput::ptr mInput;
	B::Media2::BMediaOutput::ptr mOutput;
	
	ADPCMWaveFormat *mHeaderIn;
	PCMWaveFormat mHeaderOut;
	
	int32 mSamplesPerBlock;
	size_t mOutputBlockSize;
	
	void *mTempOutputBuffer;
	size_t mTempOutputBufferLength;
	
	void CreateInput();
	
	public:
		CMSADPCMDecoder();
		
		virtual status_t Acquired (const void *id);
		virtual status_t Released (const void *id);

		virtual	status_t AcceptInputConnection (B::Media2::IMediaOutput::arg remoteOutput,
										B::Media2::BMediaInput::arg input,
										const B::Media2::BMediaFormat &format);

		virtual	void Connected (B::Media2::BMediaEndpoint::arg localEndpoint,
								B::Media2::IMediaEndpoint::arg remoteEndpoint,
								const B::Media2::BMediaFormat &format);
	
		virtual	void Disconnected (B::Media2::BMediaEndpoint::arg localEndpoint,
									B::Media2::IMediaEndpoint::arg remoteEndpoint);		

		virtual	status_t HandleBuffer (B::Media2::BMediaInput::arg receiver,
										B::Media2::BBuffer *buffer);						
};

} } // B::Private

#endif
