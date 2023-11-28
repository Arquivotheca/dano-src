#ifndef C_DVI_ADPCM_DECODER_H

#define C_DVI_ADPCM_DECODER_H

#include <media2/MediaNode.h>

#include "adpcm.h"

class CDVIADPCMDecoder : public B::Media2::BMediaNode
{
	B::Media2::BMediaInput::ptr mInput;
	B::Media2::BMediaOutput::ptr mOutput;
	
	adpcm_state mADPCMState;
	
	void CreateInput();
	
	public:
		CDVIADPCMDecoder();
		
		virtual status_t Acquired (const void *id);
		virtual status_t Released (const void *id);

		virtual	void Connected (B::Media2::BMediaEndpoint::arg localEndpoint,
								B::Media2::IMediaEndpoint::arg remoteEndpoint,
								const B::Media2::BMediaFormat &format);
	
		virtual	void Disconnected (B::Media2::BMediaEndpoint::arg localEndpoint,
									B::Media2::IMediaEndpoint::arg remoteEndpoint);		

		virtual	status_t HandleBuffer (B::Media2::BMediaInput::arg receiver,
										B::Media2::BBuffer *buffer);						
};

#endif
