// [em 27jan00]
#include "MPEGAudioDecoder.h" 

// fraunhofer decoder
#include "mp3decifc.h"

#include <support2/Debug.h>
#include <malloc.h>

using namespace B::Media2;

namespace B {
namespace Private {

MPEGAudioDecoder::MPEGAudioDecoder()
	: BMediaNode("mpeg1audio_fh.decoder")
{
	fDecoderSem       = create_sem(1, "mpeg-audio-decoder");

	fDecoder          = 0;
	fDecoderBufferSize = DEFAULT_DECODER_BUFFER_SIZE;
	fDecoderBuffer = (uchar*)malloc(fDecoderBufferSize);

	_init_fh_decoder();
	
	fOutputBufferSize = DEFAULT_OUTPUT_BUFFER_SIZE;
	fOutputBuffer = (uchar*)malloc(fOutputBufferSize);
	fOutputBufferPos = 0;
	fOutputBufferUsed = 0;
	
	fProduceFloat      = false;
}

status_t 
MPEGAudioDecoder::Acquired(const void *id)
{
	status_t result=BMediaNode::Acquired(id);
	
	if (result<B_OK)
		return result;
	
	CreateInput();
	
	return B_OK;
}

status_t 
MPEGAudioDecoder::Released(const void *id)
{
	return BMediaNode::Released(id);
}

void 
MPEGAudioDecoder::Connected (BMediaEndpoint::arg localEndpoint,
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

		BValue audio_formats(BValue::Int32(B_AUDIO_INT16));

#if !FIXED_POINT_DECODE
		audio_formats.Overlay(BValue::Int32(B_AUDIO_FLOAT));
#endif

		c.And(B_FORMATKEY_RAW_AUDIO_TYPE,audio_formats);
				
		mOutput->SetConstraint(c);
		AddEndpoint(mOutput);
	}
	else
	{
		fProduceFloat=format[B_FORMATKEY_RAW_AUDIO_TYPE].AsInt32()==B_AUDIO_FLOAT;	
	}
}

void 
MPEGAudioDecoder::Disconnected (BMediaEndpoint::arg localEndpoint,
								IMediaEndpoint::arg)
{
	if (localEndpoint.ptr()==mInput.ptr())
	{
		if (fDecoderBuffer)
			free(fDecoderBuffer);
		
		fDecoderBuffer = NULL;
	
		if(fDecoder)
			fDecoder->Release();
	
		if (fOutputBuffer)
			free(fOutputBuffer);
	
		fOutputBuffer = NULL;
	
		delete_sem(fDecoderSem);
	
		if (mOutput!=NULL)
		{
			RemoveEndpoint(mInput);
			mInput=NULL;
		}
	}
	else
	{
		if (mInput==NULL)
		{
			RemoveEndpoint(mOutput);
			mOutput=NULL;

			CreateInput();
		}
	}
}

void 
MPEGAudioDecoder::CreateInput()
{
	mInput=new BMediaInput("audio_in");
	
	BMediaConstraint c(B_FORMATKEY_MEDIA_TYPE,BValue::Int32(B_MEDIA_ENCODED_AUDIO));
	c.And(B_FORMATKEY_CHANNEL_COUNT,BMediaConstraintItem::B_GE,BValue::Int32(1));

	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_GE,BValue::Float(0.0f));
	c.And(B_FORMATKEY_FRAME_RATE,BMediaConstraintItem::B_NE,BValue::Float(0.0f));

	c.And(B_FORMATKEY_ENCODING,BValue::String("be:mpegaudio"));

	mInput->SetConstraint(c);
	
	AddEndpoint(mInput);
}

status_t 
MPEGAudioDecoder::HandleBuffer (BMediaInput::arg, BBuffer *buffer)
{
	if (mOutput==NULL || mOutput->EndpointState()!=B_CONNECTED_ENDPOINT)
	{
		buffer->ReleaseBuffer();
		return B_OK;
	}
	
	const uint8 *in_ptr=(const uint8 *)buffer->Data();
	size_t in_size=buffer->Size();
	
	while (in_size>0)
	{
		int bytes_read=fDecoder->Fill(in_ptr,in_size);
		
		in_ptr+=bytes_read;
		in_size-=bytes_read;

		if (fDecoder->GetInputLeft()>=2048)
		{
			BBuffer out_buffer;
			
			if (mOutput->AcquireBuffer(&out_buffer)<B_OK)
				fDecoder->Reset();
			else
			{				
				SSC ssc;
				int decode_count;
				
				if(fProduceFloat)
				{
					ssc = fDecoder->DecodeFrame(
						(float*)out_buffer.Data(),
						out_buffer.Size(),
						&decode_count);
				}
				else
				{
					ssc = fDecoder->DecodeFrame(
						(unsigned char *)out_buffer.Data(),
						out_buffer.Size(),
						&decode_count);
				}
				
				if (SSC_SUCCESS(ssc))
				{
					out_buffer.SetRange(0,decode_count);
					
					if (mOutput->SendBuffer(&out_buffer)<B_OK)
						out_buffer.ReleaseBuffer();					
				}
				else
					out_buffer.ReleaseBuffer();
			}
		}
	}

	ASSERT(in_size==0);
	
	buffer->ReleaseBuffer();
			
	return B_OK;
}

#if 0

status_t MPEGAudioDecoder::Decode(
                  void* out_buffer,
                  int64* out_frameCount,
                  media_header* mh,
                  media_decode_info* mdi)
{
	int32	          copy_count, needed_count, amt;
	int             decode_count;
	status_t        err;
	media_header    header;
	
	// Fraunhofer return code
	SSC             ssc;

	uchar*          out_pos = (uchar*)out_buffer;
	int64           num_frames_incr = 0;
	bool	          last_input_buffer = false;

	// current chunk of encoded data from the track
	const uchar*    chunk = 0;
	size_t          chunk_size, chunk_pos;
	
	// minimum encoded data needed 
	size_t          min_decode_fill_amt = 2048;
	
	// ++++++
	const int       error_max = 16;
	int             error_count = 0;
	
	int             frame_size = (2 * fNumChannels * (fProduceFloat ? 2 : 1));
	
	*out_frameCount = 0;
	needed_count = fBlocSize;
	
	acquire_sem(fDecoderSem);

	//printf("\n### MP3 Decode()\n");
	
	while (needed_count > 0) {
		//printf("\nwe still need %d bytes of decoded data\n", needed_count);

		// first check if we have some decoded data that we can use
		amt = (fOutputBufferUsed - fOutputBufferPos);
		if (amt > 0) {
			if (amt > needed_count)
				copy_count = needed_count;
			else
				copy_count = amt;

			memcpy(out_pos, &fOutputBuffer[fOutputBufferPos], copy_count);
			//printf("copied %d bytes of previously decoded data\n", copy_count);
			
			fOutputBufferPos  += copy_count;
			needed_count      -= copy_count;
			out_pos           += copy_count;

			*out_frameCount   += copy_count / frame_size;	//	16 bit * channels
			num_frames_incr   += copy_count / frame_size;	//	16 bit * channels

			if (needed_count <= 0)   // are we all done?
				break;
		}

		// add data to the decoder's input buffer as needed
		//printf("%d bytes in input buffer%s\n", fDecoder->GetInputLeft(),
		//	last_input_buffer ? " (at last buffer)" : "");

		while((fDecoder->GetInputLeft() < min_decode_fill_amt) && !last_input_buffer) {
			// fetch a new chunk if the current one is dry
			if(!chunk || chunk_pos == chunk_size) {
				//printf("*** getting a chunk\n");
				chunk_size = 0;
				chunk_pos = 0;

				err = GetNextChunk((const void**)&chunk, &chunk_size, &header, mdi);
				if(err < B_OK) {
					if((err == B_LAST_BUFFER_ERROR) /*&& (*out_frameCount != 0)*/) {
						//printf("    --> last buffer\n");
						last_input_buffer = true;
						//break;
					}
					else {
						//printf("!!! GetNextChunk(): %s\n", strerror(err));
						release_sem(fDecoderSem);
						return err;
					}
				}
			}

			if(chunk_size > 0) {
				// add data to decoder buffer (as much as it wants)
				int fill_count = fDecoder->Fill(&chunk[chunk_pos], chunk_size-chunk_pos);
				//printf("filled %d\n", fill_count);
				chunk_pos += fill_count;
			}
		}

		//printf("chunk: at %ld of %ld%s\n", chunk_pos, chunk_size,
			//last_input_buffer ? " (at last buffer)" : "");
		
		// decode some data
		amt = fDecoder->GetInputLeft();
		if(amt == 0 && last_input_buffer && chunk_pos == chunk_size) {
			break;
		}
		
		if (amt >= min_decode_fill_amt || last_input_buffer) {

			// check if we need to make room in the output buffer
			amt = (fOutputBufferUsed - fOutputBufferPos);
			//printf("Moving %d bytes of decoded data\n", amt); 
			if (amt > 0)
				memmove(fOutputBuffer, &fOutputBuffer[fOutputBufferPos], amt);
			else
				amt = 0;
			fOutputBufferPos = 0;
			fOutputBufferUsed  = amt;

			// attempt to decode a frame
			if(fProduceFloat)
				ssc = fDecoder->DecodeFrame(
					(float*)&fOutputBuffer[fOutputBufferUsed],
					fOutputBufferSize-fOutputBufferUsed,
					&decode_count);
			else
				ssc = fDecoder->DecodeFrame(
					&fOutputBuffer[fOutputBufferUsed],
					fOutputBufferSize-fOutputBufferUsed,
					&decode_count);
			//printf("decoded %d bytes\n", decode_count);
				
			if(SSC_SUCCESS(ssc)) {
				// got it successfully
				fOutputBufferUsed += decode_count;
				error_count = 0;
			}
			else
			{
				if(last_input_buffer)
				{
					release_sem(fDecoderSem);
					return B_LAST_BUFFER_ERROR;
				}
			
				if(ssc == SSC_W_MPGA_SYNCNEEDDATA) {
					// decoder needs more input data
					size_t new_fill_amt = min_decode_fill_amt * 2;
					if(new_fill_amt >= fDecoderBufferSize)
						new_fill_amt = fDecoderBufferSize;
					if(new_fill_amt == min_decode_fill_amt) {
						// give up
						//printf("Fraunhofer too greedy, giving up\n");
						release_sem(fDecoderSem);
						return B_ERROR;
					}
					min_decode_fill_amt = new_fill_amt;				
				}
				else {
					printf("fraunhofer error: %s\n",
						mp3decGetErrorText(ssc));
					if(++error_count > error_max) { // bail after too many errors
						release_sem(fDecoderSem);
						return B_ERROR;
					}
				}
			}
		}
		
	}
	
	// no data fetched?
	if(needed_count == fBlocSize) {
		release_sem(fDecoderSem);
		return B_LAST_BUFFER_ERROR;
	}

	*mh = header;
	//	assuming 44.1 kHz
	mh->start_time = (int64)(1000000.0 * (float)fCurFrame / fSampleRate);
	fCurFrame += num_frames_incr;
	//printf("Returned %d frames\n", num_frames_incr);
	release_sem(fDecoderSem);
	return B_OK;
}

#endif

#if 0

status_t MPEGAudioDecoder::Reset(int32 to_what,
								 int64 requiredFrame, int64 *inout_frame,
								 bigtime_t requiredTime, bigtime_t *inout_time)
{
	if (to_what == B_SEEK_BY_FRAME) {
		if (*inout_frame < 0)
			*inout_frame = 0;

		fCurFrame = *inout_frame;
	} else if (to_what == B_SEEK_BY_TIME) {
		if (*inout_time < 0)
			*inout_time = 0;
		
		fCurFrame = (int64)(((double)*inout_time / 1000000.0) * fSampleRate);
	} else {
		return B_BAD_VALUE;
	}

	fDecoder->Reset();

	fOutputBufferPos = 0;
	fOutputBufferUsed = 0;

	return B_OK;
}

#endif

status_t MPEGAudioDecoder::_init_fh_decoder() {
	if(fDecoder)
		fDecoder->Release();
		
	ASSERT(fDecoderBuffer);

#if FIXED_POINT_DECODE
	SSC ssc = mp3decCreateObjectExtBuf(
		fDecoderBuffer, fDecoderBufferSize,
		0, 0, 0,
		true,
		&fDecoder);
#else
	SSC ssc = mp3decCreateObjectExtBuf(
		fDecoderBuffer, fDecoderBufferSize,
		0, 0, 0,
		false,
		&fDecoder);
#endif		

	return SSC_SUCCESS(ssc) ? B_OK : B_ERROR;
}

} } // B::Private
// END -- MpegAudioDecoder.cpp --
