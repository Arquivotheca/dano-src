#ifndef C_AC3_DECODER_H

#define C_AC3_DECODER_H

#include <media2/MediaNode.h>
#include "ac3audio.h"

class CAC3Decoder : public B::Media2::BMediaNode
{
	CAC3Decoder (const CAC3Decoder &);
	CAC3Decoder &operator= (const CAC3Decoder &);
	
	B::Media2::BMediaInput::ptr mInput;
	B::Media2::BMediaOutput::ptr mOutput;

	ac3audio_decoder_t *mDecoderImpl;
	
	static void AcquireBufferCB (buffer_t *me);
	static void ReleaseBufferCB (buffer_t *me);	

	static status_t AcquireOutputBufferCB (void *cookie, buffer_t **);
	static status_t SendBufferCB (void *cookie, buffer_t *);

	public:
		CAC3Decoder();
		virtual ~CAC3Decoder();
		
		virtual status_t Acquired(const void *id);
		virtual status_t Released(const void *id);

		virtual	status_t HandleBuffer (B::Media2::BMediaInput::arg receiver,
										B::Media2::BBuffer *buffer);

		virtual	void Connected (B::Media2::BMediaEndpoint::arg localEndpoint,
								B::Media2::IMediaEndpoint::arg remoteEndpoint,
								const B::Media2::BMediaFormat &format);
	
		virtual	void Disconnected (B::Media2::BMediaEndpoint::arg localEndpoint,
									B::Media2::IMediaEndpoint::arg remoteEndpoint);

		virtual status_t	PropagateMessage (const B::Support2::BMessage &message,
												B::Media2::IMediaEndpoint::arg from,
												B::Media2::media_endpoint_type direction,
												B::Media2::BMediaEndpointVector *visited);
};

#endif
