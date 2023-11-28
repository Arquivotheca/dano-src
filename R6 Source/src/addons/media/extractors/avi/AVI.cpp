#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <Screen.h>
#include <OS.h>
#include <Bitmap.h>
#include <File.h>
#include <MediaFormats.h>
#include <MediaDefs.h>
#include <MediaTrack.h>

#include "Extractor.h"
#include "AVI.h"

//#define DEBUG printf
#define DEBUG if (0) printf

extern "C" const char * mime_type_sniffer = "video/avi";

extern "C" Extractor *instantiate_extractor(void)
{
	return new AVIExtractor();
}

struct split_state {
	uint32	curframe;
	int64	curbytes;
};

AVIExtractor::AVIExtractor()
{
	riff          = NULL;
	track_formats = NULL;
	//curframe      = NULL;
	//curbytes      = NULL;
	mNumTracks    = 0;
}

AVIExtractor::~AVIExtractor()
{
	//if (curframe) free(curframe);
	//if (curbytes) free(curbytes);
	//curframe = NULL;
	//curbytes = NULL;

	if (track_formats)
		delete[] track_formats;//free(track_formats)
	track_formats = NULL;

	if (riff)
		delete riff;
	riff = NULL;
}


const char *
AVIExtractor::Copyright(void)
{
	return riff->Copyright();
}

status_t
AVIExtractor::GetFileFormatInfo(media_file_format *mfi)
{
	strcpy(mfi->mime_type,      "video/x-msvideo");
	strcpy(mfi->pretty_name,    "AVI Movie File");
	strcpy(mfi->short_name,     "avi");
	strcpy(mfi->file_extension, "avi");

	mfi->family = B_AVI_FORMAT_FAMILY;

    mfi->capabilities = media_file_format::B_READABLE              |
	                    media_file_format::B_IMPERFECTLY_SEEKABLE  |
	                    media_file_format::B_PERFECTLY_SEEKABLE    |
						media_file_format::B_KNOWS_RAW_VIDEO       | 
    					media_file_format::B_KNOWS_RAW_AUDIO       |
						media_file_format::B_KNOWS_ENCODED_VIDEO   |
    					media_file_format::B_KNOWS_ENCODED_AUDIO;

	return B_OK;
}

status_t
AVIExtractor::Sniff(int32 *out_streamNum, int32 *out_chunkSize)
{
	status_t 					err;
	media_format 				*mf;
	int 						pref_chunk_size = 64 * 1024;

	BPositionIO *file = dynamic_cast<BPositionIO *>(Source());
	if (file == NULL)
		return B_IO_ERROR;

	riff = new TRIFFReader(file);
	if (riff->InitCheck() == false) {
		delete riff;
		riff = NULL;
		return B_BAD_VALUE;
	}

	mWidth  = riff->Width();
	mHeight = riff->Height();

	mNumTracks = riff->GetAVIHeader()->NumberOfStreams;

	track_formats = new(std::nothrow) media_format[mNumTracks];//(media_format *)calloc(mNumTracks*sizeof(media_format), 1);
	if (track_formats == NULL)
		return B_NO_MEMORY;
	

	if (riff->HasVideo()) {
		mf = &track_formats[0];
		size_t dummy;
		err = BMediaFormats::GetAVIFormatFor(riff->GetVIDSHeader(&dummy)->Compression,
		                               mf, B_MEDIA_ENCODED_VIDEO);
		if (err != B_OK) {
			mf->type = B_MEDIA_ENCODED_VIDEO;
			mf->u.encoded_video = media_encoded_video_format::wildcard;
		}
		
		mf->user_data_type = B_CODEC_TYPE_INFO;
		uint32 fourcc = riff->GetVIDSHeader(&dummy)->Compression;
		sprintf((char *)mf->user_data, "AVI %c%c%c%c",
			fourcc>>24, fourcc>>16, fourcc>>8, fourcc);

		mf->u.encoded_video.avg_bit_rate = 1;
		mf->u.encoded_video.max_bit_rate = 1;
		mf->u.encoded_video.frame_size = 64*1024;
		mf->u.encoded_video.output.field_rate = (1000000.0 / riff->GetAVIHeader()->TimeBetweenFrames);
		mf->u.encoded_video.output.last_active = mHeight-1;
		mf->u.encoded_video.output.interlace = 1;
		mf->u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		mf->u.encoded_video.output.pixel_width_aspect = 1;
		mf->u.encoded_video.output.pixel_height_aspect = 1;

		mf->u.encoded_video.output.display.line_width = mWidth;
		mf->u.encoded_video.output.display.line_count = mHeight;

		off_t maxsize = 0;
		AVIIndex *video_chunk;
		
		for(int i=0; i < riff->CountVideoFrames(); i++) {
			video_chunk = (AVIIndex *)riff->VideoChunkList()->ItemAt(i);
			if (video_chunk->Length > maxsize)
				maxsize = video_chunk->Length;
		}

		if (maxsize < 64*1024)
			maxsize = 64*1024;
		
		pref_chunk_size = (maxsize + 65535) & ~65535;   // round up to a 64k boundary
	}

	if (riff->HasAudio()) {
		media_raw_audio_format *raw;
		AVIAUDSHeader *auds;
		uint32		format;

		auds = riff->GetAUDSHeader();

		if (riff->HasVideo())
			mf = &track_formats[1];
		else 
			mf = &track_formats[0];
		memset(mf, 0, sizeof(*mf));

		mf->user_data_type = B_CODEC_TYPE_INFO;
		format = 0x65610000 + auds->Format;
		sprintf((char *)mf->user_data, "AVI audio %d", auds->Format);

		if (auds->Format != 1) {
			err = BMediaFormats::GetAVIFormatFor(format,
			                                     mf, B_MEDIA_ENCODED_AUDIO);
			if (err != B_OK) {
				mf->type = B_MEDIA_ENCODED_AUDIO;
				mf->u.encoded_audio = media_encoded_audio_format::wildcard;
			}

			mf->u.encoded_audio.bit_rate=auds->AvgBytesPerSec*8;
			raw = &mf->u.encoded_audio.output;
			raw->frame_rate    = riff->SamplingRate();
			raw->channel_count = riff->CountChannels();
			raw->byte_order    = riff->ByteOrder();
			raw->buffer_size   = ((riff->SamplingRate() *
								   auds->BitsPerSample *
								   riff->CountChannels()) / (20 * 8)) & ~0x3;
			/* XXXdbg -- fixme */
		} else {
	
			mf->type = B_MEDIA_RAW_AUDIO;
			raw = &mf->u.raw_audio;
	
			switch(auds->BitsPerSample) {
			case 8:
				raw->format = media_raw_audio_format::B_AUDIO_UCHAR;
				break;
	
			case 16:
				raw->format = media_raw_audio_format::B_AUDIO_SHORT;
				break;
				
			case 32:
				raw->format = media_raw_audio_format::B_AUDIO_INT;
				break;
				
			default:
				DEBUG("avi-auds: weird ass bits-per-sample field value of %d\n", auds->BitsPerSample);
				// just ignore the audio track
				mNumTracks--;
			}
	
			raw->frame_rate    = riff->SamplingRate();
			raw->channel_count = riff->CountChannels();
			raw->byte_order    = riff->ByteOrder();
			raw->buffer_size   = ((riff->SamplingRate() *
								   auds->BitsPerSample *
								   riff->CountChannels()) / (20 * 8)) & ~0x3;	
		}
	}

#if 0	
	curframe = (uint *)calloc(mNumTracks * sizeof(int), 1);
	if (curframe == NULL)
		return B_NO_MEMORY;

	curbytes = (int64 *)calloc(mNumTracks * sizeof(int64), 1);
	if (curbytes == NULL)
		return B_NO_MEMORY;
#endif
	*out_streamNum = mNumTracks;
	*out_chunkSize = pref_chunk_size;

	return B_OK;
}

status_t
AVIExtractor::TrackInfo(int32 in_stream, media_format *out_format,
						void **out_info, int32 *out_size)
{
	media_format *mf;
	
	if (in_stream < 0 || in_stream > mNumTracks) {
DEBUG("bogus stream number %d\n", in_stream);
		return B_BAD_INDEX;
	}

	mf = &track_formats[in_stream];
	*out_format = track_formats[in_stream];

	if (mf->type == B_MEDIA_ENCODED_VIDEO || mf->type == B_MEDIA_RAW_VIDEO) {
		*out_info = riff->GetVIDSHeader((size_t*)out_size);
	} else if (mf->type == B_MEDIA_ENCODED_AUDIO || mf->type == B_MEDIA_RAW_AUDIO) {
		*out_info = riff->GetAUDSHeader();
		*out_size = sizeof(AVIAUDSHeader);
	}
	
	return B_OK;
}

status_t
AVIExtractor::CountFrames(int32 in_stream, int64 *out_frames)
{
	if (in_stream < 0 || in_stream > mNumTracks) {
DEBUG("bogus stream number %d\n", in_stream);
		return B_BAD_INDEX;
	}

	if (track_formats[in_stream].type == B_MEDIA_RAW_VIDEO ||
		track_formats[in_stream].type == B_MEDIA_ENCODED_VIDEO) {
		*out_frames = riff->CountVideoFrames();
	} else if (track_formats[in_stream].type == B_MEDIA_RAW_AUDIO ||
			   track_formats[in_stream].type == B_MEDIA_ENCODED_AUDIO) {
		*out_frames = riff->CountAudioFrames();
	}
		
	return B_OK;
}


status_t
AVIExtractor::GetDuration(int32 in_stream,
						  bigtime_t *out_expire_time)
{
	if (in_stream < 0 || in_stream > mNumTracks) {
DEBUG("bogus stream number %d\n", in_stream);
		return B_BAD_INDEX;
	}

	if (track_formats[in_stream].type == B_MEDIA_RAW_VIDEO ||
		track_formats[in_stream].type == B_MEDIA_ENCODED_VIDEO) {

		*out_expire_time = (bigtime_t)riff->CountVideoFrames() *
			               (bigtime_t)riff->GetAVIHeader()->TimeBetweenFrames;
	
	} else if (track_formats[in_stream].type == B_MEDIA_RAW_AUDIO ||
			   track_formats[in_stream].type == B_MEDIA_ENCODED_AUDIO) {

		AVIAUDSHeader *auds = riff->GetAUDSHeader();
		int64 num_frames = (int64)(auds->ByteCount / ((double)(auds->BitsPerSample * auds->Channels) / 8.0));
		bigtime_t total_time = (num_frames * 1000000LL) / (bigtime_t)auds->SamplesPerSec;
		*out_expire_time = total_time;
	}
	
	return B_OK;
}

status_t 
AVIExtractor::AllocateCookie(int32 in_stream, void **cookieptr)
{
	*cookieptr = calloc(sizeof(split_state), 1);
	if(*cookieptr == NULL)
		return B_NO_MEMORY;
	else
		return B_NO_ERROR;
}

status_t 
AVIExtractor::FreeCookie(int32 in_stream, void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

status_t
AVIExtractor::SplitNext(int32   in_stream,
                        void *cookie,
						off_t  *inout_filepos,
						char   *in_packetPointer,
						int32  *inout_packetLength,
						char  **out_bufferStart,
						int32  *out_bufferLength,
						media_header *mh)
{
	off_t    pos;
	size_t   size;
	int32 	 flags;
	int64    frame;
	int64    next_frame;
	AVIIndex *video_chunk, *next_video = NULL;
	AVIIndex *audio_chunk, *next_audio = NULL;
	split_state *state = (split_state *)cookie;

	if (in_stream < 0 || in_stream > mNumTracks) {
DEBUG("bogus stream number %d\n", in_stream);
		return B_BAD_INDEX;
	}

	frame = state->curframe;

	if (track_formats[in_stream].type == B_MEDIA_RAW_VIDEO ||
		track_formats[in_stream].type == B_MEDIA_ENCODED_VIDEO) {

		mh->u.encoded_video.field_number = 0;	// raw header is the same
		if (frame <= 0) {
			frame = 0;
			mh->start_time = 0;
		} else if (frame >= riff->CountVideoFrames()) {
			mh->start_time = frame * (bigtime_t)riff->UsPerFrame();
			return B_LAST_BUFFER_ERROR;
		} else
			mh->start_time = frame * (bigtime_t)riff->UsPerFrame();

		mh->u.encoded_video.field_sequence = frame;

		video_chunk = (AVIIndex *)riff->VideoChunkList()->ItemAt(frame);
		if (video_chunk == NULL) {
DEBUG("error retrieving video frame %Ld\n", frame);
			return B_ERROR;
		}
		
		pos   = video_chunk->Offset;
		size  = video_chunk->Length;
		flags = video_chunk->Flags;
		
		next_frame = frame+1;
		if(next_frame < riff->CountVideoFrames())
			next_video = (AVIIndex *)riff->VideoChunkList()->ItemAt(next_frame);
		while(next_video && next_video->Length == 0) {
			next_frame++;
			if(next_frame < riff->CountVideoFrames())
				next_video = (AVIIndex *)riff->VideoChunkList()->ItemAt(next_frame);
			else
				next_video = NULL;
		}

	} else if (track_formats[in_stream].type == B_MEDIA_RAW_AUDIO ||
			   track_formats[in_stream].type == B_MEDIA_ENCODED_AUDIO) {

//DEBUG("split audio frame %Ld / %d\n", frame, riff->CountAudioChunks());
		if (frame < 0)
			frame = 0;
		else if (frame >= riff->CountAudioChunks()) {
			mh->start_time = frame * (bigtime_t)riff->UsPerFrame();
			return B_LAST_BUFFER_ERROR;
		}

		audio_chunk = (AVIIndex *)riff->AudioChunkList()->ItemAt(frame);
		if (audio_chunk == NULL) {
DEBUG("error retrieving audio frame %Ld / %d\n", frame, riff->CountAudioChunks());
			return B_ERROR;
		}

		pos   = audio_chunk->Offset;
		size  = audio_chunk->Length;
		flags = audio_chunk->Flags;
		
		next_frame = frame+1;
		if (next_frame < riff->CountAudioChunks())
			next_audio = (AVIIndex *)riff->AudioChunkList()->ItemAt(next_frame);
	}
	

//DEBUG("SPLIT: IN: fpos %Ld pktlen %d ## ", *inout_filepos, *inout_packetLength);
	if (size == 0 || (pos >= *inout_filepos &&
		pos+size <= (*inout_filepos + *inout_packetLength))) {

//DEBUG("OUT: %Ld size %d\n", pos+size, size);
		*out_bufferStart  = in_packetPointer + (pos - *inout_filepos);
		mh->orig_size = *out_bufferLength = size;

		if (next_video) {
			*inout_filepos = next_video->Offset;
		} else if (next_audio) {
			*inout_filepos = next_audio->Offset;
		} else {
			*inout_filepos = 0;
		}

		if (track_formats[in_stream].type == B_MEDIA_RAW_AUDIO ||
			track_formats[in_stream].type == B_MEDIA_ENCODED_AUDIO) {
			AVIAUDSHeader *auds = riff->GetAUDSHeader();
			double bytes_per_frame = ((double)auds->BitsPerSample * auds->Channels) / 8;
			mh->start_time = (bigtime_t)(((state->curbytes * 1000000LL) / bytes_per_frame) / auds->SamplesPerSec);
		} else {
			mh->type = B_MEDIA_ENCODED_VIDEO;
			if (flags & AVIIF_KEYFRAME)
				mh->u.encoded_video.field_flags = B_MEDIA_KEY_FRAME;
			else
				mh->u.encoded_video.field_flags = 0;
		}

		mh->file_pos = *inout_filepos;

		state->curframe = next_frame;
		state->curbytes += size;
	} else {
//DEBUG("OUT-ERR: pos %Ld sz %d\n", pos, size);
		*out_bufferStart    = NULL;
		*inout_filepos      = pos;
		*inout_packetLength = size;
	}

	return B_OK;
}

status_t
AVIExtractor::Seek(int32 in_stream,
                   void *cookie,
				   int32 to_what,
				   int32 flags,
				   bigtime_t *inout_time,
				   int64 *inout_frame,
				   off_t *inout_filePos,
				   char *in_packetPointer,
				   int32 *inout_packetLength,
				   bool *out_done)
{
	int i;
	int64 frame;
	int64 which_frame = *inout_frame;
	off_t pos;
	size_t size;
	bool seek_forward =
		(flags & B_MEDIA_SEEK_DIRECTION_MASK) == B_MEDIA_SEEK_CLOSEST_FORWARD;
	split_state *state = (split_state *)cookie;

	if (in_stream < 0 || in_stream > mNumTracks) {
DEBUG("bogus stream number %d\n", in_stream);
		return B_BAD_INDEX;
	}

	if (track_formats[in_stream].type == B_MEDIA_RAW_VIDEO ||
		track_formats[in_stream].type == B_MEDIA_ENCODED_VIDEO) {
		bigtime_t	frame_duration = (bigtime_t)riff->UsPerFrame();
		bigtime_t	target_time = 0;

		AVIIndex *video_chunk;

		if (to_what == B_SEEK_BY_TIME) {
			target_time = *inout_time;
			which_frame = (*inout_time / frame_duration);
		}
		else {
			target_time = which_frame * frame_duration;
		}

		if (which_frame < 0)
			which_frame = 0;
		else if (which_frame >= riff->CountVideoFrames())
			which_frame = riff->CountVideoFrames() - 1;

		
		int fwd=riff->CountVideoFrames(), back=-1, max, min;
		int forward_frames = 0;
		AVIIndex *idx;
				
		/* search for key frames */
		idx = riff->GetIndex();
		for(int i=0, currframe = 0; i < riff->IndexCount(); i++) {
			uint16 id = idx[i].ChunkID & 0xffff;
			if(id  == 'db' || id  == 'dc' || id  == 'iv' ||
			   (idx[i].ChunkID >> 16 == '00' && id != 'wb')) {
//DEBUG("got %c%c\n", (idx[i].ChunkID & 0xff00) >> 8, idx[i].ChunkID & 0xff);
				if (idx[i].Flags & AVIIF_KEYFRAME) {
					if (to_what == B_SEEK_BY_TIME) {
						bigtime_t currtime = currframe * frame_duration;
						if(target_time >= currtime) {
							back = currframe;
						}
						if(target_time <= currtime) {
							fwd = currframe;
							break;
						}
					}
					else {
						if(which_frame >= currframe) {
							back = currframe;
						}
						if(which_frame <= currframe) {
							fwd = currframe;
							break;
						}
					}
				}
				currframe++;
			}
		}
				
		// What is all this stuff?  --geh
/*
		if(back >= 0 && fwd - back < 150 &&
		   (back > 0 || fwd < riff->CountVideoFrames())) {
			if (fwd < riff->CountVideoFrames() &&
				(fwd - which_frame + forward_frames) <
				(which_frame - back))
				frame = fwd;
			else
				frame = back;
			// printf("frame: %6d keyframe %6d     \r", last_frame, frame); fflush(stdout);
		}
		else {
			frame = which_frame-forward_frames;
			if(frame < 0)
				frame = 0;
		}
*/

		if(seek_forward)  {
			//printf("used forward, %d\n", fwd);
			frame = fwd;
		} else {
			//printf("used backward, %d\n", back);
			frame = back;
			if(frame < 0)
				frame = 0;
		}
		

		video_chunk = (AVIIndex *)riff->VideoChunkList()->ItemAt(frame);
		if (video_chunk == NULL)
			return B_ERROR;
		
		pos  = video_chunk->Offset;
		size = video_chunk->Length;

		*inout_frame = frame;

		if(!(flags & B_MEDIA_SEEK_PEEK))
			state->curframe = *inout_frame;
		*inout_filePos      = pos;
		*inout_packetLength = size;
		*inout_time         = (bigtime_t)riff->UsPerFrame() * frame;

		*out_done = true;

	} else if (track_formats[in_stream].type == B_MEDIA_RAW_AUDIO ||
			   track_formats[in_stream].type == B_MEDIA_ENCODED_AUDIO) {

		BList *chunks = riff->AudioChunkList();
		AVIIndex *achunk;
		bigtime_t time_sum;
		AVIAUDSHeader *auds = riff->GetAUDSHeader();;
		bigtime_t when, last;
		int32 length;
		int32 target_length;
		int32 last_length;

		//int64 num_frames = (int64)(auds->ByteCount / ((double)(auds->BitsPerSample * auds->Channels) / 8.0));	
		//bigtime_t total_time = (num_frames * 1000000LL) / (bigtime_t)auds->SamplesPerSec;

		int64 num_frames = (int64)(auds->ByteCount * auds->SamplesPerSec / ((double)(auds->AvgBytesPerSec)));	
		bigtime_t total_time = (int64)(auds->ByteCount * 1000000LL / auds->AvgBytesPerSec); 
//printf("num_frames: %Ld\n",num_frames);
//printf("total_time: %Ld\n",total_time);
		int64 which_chunk;
		double bytes_per_frame = ((double)auds->AvgBytesPerSec / auds->SamplesPerSec);
//printf("bytes_per_frame: %f\n",bytes_per_frame);
		
		if (to_what == B_SEEK_BY_TIME) {
			when = *inout_time;
		} else {
			int64 frame = *inout_frame;
			when = (frame * total_time) / num_frames;
		}
		
		time_sum = 0;
DEBUG("seek audio to: %Ld (%d chunks)\n", when,riff->CountAudioChunks());
		length = 0;
		target_length = (int32)(when * bytes_per_frame * riff->SamplingRate() / 1000000LL);
		for(i=0; i < riff->CountAudioChunks(); i++) {
DEBUG("chunk %d\n",i);
			achunk = (AVIIndex *)chunks->ItemAt(i);
			last_length = achunk->Length;
			if(length + last_length > target_length)
				break;
			length += last_length;
		}
		time_sum = (bigtime_t)(((double)length * 1000000LL) /
		           (double)riff->SamplingRate() / bytes_per_frame);
		which_chunk = i;
DEBUG("seek audio got: %Ld\n", time_sum);

		if (which_chunk < 0 || which_chunk >= riff->CountAudioChunks())
			return B_DEV_SEEK_ERROR;

		achunk = (AVIIndex *)chunks->ItemAt(which_chunk);
		pos  = achunk->Offset;
		size = achunk->Length;
		
		if(!(flags & B_MEDIA_SEEK_PEEK)) {
			state->curframe = which_chunk;
			state->curbytes = length;
		}

		*inout_filePos      = pos;
		*inout_packetLength = size;
		*inout_time         = time_sum;
		*inout_frame        = (int64)(length/bytes_per_frame);
DEBUG("seek audio got frame: %Ld\n", *inout_frame);
		*out_done = true;
		
	}

	return B_OK;
}

