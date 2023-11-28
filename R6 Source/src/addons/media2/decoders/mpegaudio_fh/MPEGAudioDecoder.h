// [em 25jan00] fraunhofer decoder wrapper
#ifndef _MPEG_AUDIO_DECODER_H
#define _MPEG_AUDIO_DECODER_H

#include <media2/MediaNode.h>

class IMpgaDecoder;

namespace B {
namespace Private {

// the Fraunhofer decoder

class MPEGAudioDecoder : public B::Media2::BMediaNode
{
	enum {
		DEFAULT_DECODER_BUFFER_SIZE = 16384,
		DEFAULT_OUTPUT_BUFFER_SIZE = 16384
	};

	::B::Media2::BMediaInput::ptr mInput;
	::B::Media2::BMediaOutput::ptr mOutput;

	sem_id        fDecoderSem;

	IMpgaDecoder* fDecoder;
	
	bool					fProduceFloat;
	
	// this buffer is effectively internal to the decoder
	// +++++rtalloc!
	uchar*        fDecoderBuffer;
	int           fDecoderBufferSize;
	
	// outbound-data buffer
	// +++++rtalloc!
	uchar*        fOutputBuffer;
	int           fOutputBufferSize;
	int           fOutputBufferPos;
	int           fOutputBufferUsed;

	status_t _init_fh_decoder();

	void CreateInput();
	
	public:
		MPEGAudioDecoder();

		virtual status_t Acquired (const void *id);
		virtual status_t Released (const void *id);

		virtual	void Connected (::B::Media2::BMediaEndpoint::arg localEndpoint,
								::B::Media2::IMediaEndpoint::arg remoteEndpoint,
								const ::B::Media2::BMediaFormat &format);
	
		virtual	void Disconnected (::B::Media2::BMediaEndpoint::arg localEndpoint,
									::B::Media2::IMediaEndpoint::arg remoteEndpoint);		

		virtual	status_t HandleBuffer (::B::Media2::BMediaInput::arg receiver,
										::B::Media2::BBuffer *buffer);		
};

} } // B::Private

#endif
