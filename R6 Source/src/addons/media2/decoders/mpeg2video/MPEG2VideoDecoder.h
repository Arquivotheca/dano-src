#ifndef C_MPEG2_VIDEO_DECODER_NODE_H

#define C_MPEG2_VIDEO_DECODER_NODE_H

#include <media2/MediaNode.h>
#include <media2/MediaEndpoint.h>

#include "mpeg2video.h"

class CMPEG2VideoDecoder : public B::Media2::BMediaNode
{	
	B::Media2::BMediaInput::ptr fInput;
	B::Media2::BMediaOutput::ptr fOutput;
	
	B::Media2::BMediaFormat fInputFormat;
	B::Media2::BMediaFormat fOutputFormat;
	
	mpeg2video_decoder_t *mDecoderImpl;
	
	static void AcquireBufferCB (buffer_t *me);
	static void ReleaseBufferCB (buffer_t *me);
	
	static status_t AcquireOutputBufferCB (void *cookie, buffer_t **);
	static status_t SendBufferCB (void *cookie, buffer_t *);
		
	public:
		CMPEG2VideoDecoder();							
		virtual ~CMPEG2VideoDecoder();

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
