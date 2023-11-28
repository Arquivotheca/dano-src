#ifndef INDEO5_H
#define INDEO5_H

/* indeo headers */

#include <media2/MediaNode.h>

extern "C"
{
	#include "datatype.h"
	#include "pia_main.h"
	#include "pia_dec.h"
	#include "pia_cout.h"
}

namespace B {
namespace Private {

class Indeo5Decoder : public ::B::Media2::BMediaNode
{
	::B::Media2::BMediaInput::ptr mInput;
	::B::Media2::BMediaOutput::ptr mOutput;
	
	class IndeoEnvironment
	{
		public:
			IndeoEnvironment();
			~IndeoEnvironment();
			ENVIRONMENT_INFO *GetEnvironment();
		private:
			bool decoder_ready;
			bool colorout_ready;
			ENVIRONMENT_INFO eienvironment;
	};

	static IndeoEnvironment indeoEnvironment;

	bool decoder_inst_ready;
	bool colorout_inst_ready;
	bool palettevalid;

	DEC_INST		DecInst;		/* Decoder Instance Structure */
	DECODE_FRAME_INPUT_INFO DecInput;	/* Decoder Input Struct */
	DECODE_FRAME_OUTPUT_INFO DecOutput;	/* Decoder Output Struct */
	
	CCOUT_INST		CoutInst;		/* Decoder Instance Structure */
	COLOROUT_FRAME_INPUT_INFO CoutInput;	/* Decoder Input Struct */
	COLOROUT_FRAME_OUTPUT_INFO CoutOutput;	/* Decoder Output Struct */

	status_t Init();
	
	void CreateInput();
	
	public:
		Indeo5Decoder();
		
		virtual status_t Acquired (const void *id);
		virtual status_t Released (const void *id);

		virtual	status_t AcceptInputConnection (::B::Media2::IMediaOutput::arg remoteOutput,
												::B::Media2::BMediaInput::arg input,
												const ::B::Media2::BMediaFormat &format);

		virtual	status_t AcceptOutputConnection (::B::Media2::BMediaOutput::arg output,
												::B::Media2::IMediaInput::arg remoteInput,
												const ::B::Media2::BMediaFormat &format);

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
