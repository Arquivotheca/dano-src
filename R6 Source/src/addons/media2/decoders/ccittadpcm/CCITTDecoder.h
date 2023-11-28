#ifndef _CCITT_DECODER_H
#define _CCITT_DECODER_H

#include <media2/MediaNode.h>
#include "g72x.h"

namespace B {
namespace Private {

class CCITTDecoder : public B::Media2::BMediaNode
{
	B::Media2::BMediaInput::ptr mInput;
	B::Media2::BMediaOutput::ptr mOutput;

	int32 mFormatIndex;
	g72x_state mState;
	
	void CreateInput();
	
	public:
		CCITTDecoder();
		
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

} } // B::Private

#endif
