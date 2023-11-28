#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Screen.h>
#include <OS.h>
#include <Bitmap.h>
#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaFile.h>
#include <MediaTrack.h>


#include "Extractor.h"
#include "QuickTime.h"

#include "MediaIndex.h"
#include "QTAtomHandler.h"
#include "QTCodecDetails.h"
#include "QTStructures.h"
#include "QTTrack.h"

namespace BPrivate {
	extern bool media_debug;
}

extern "C" const char * mime_type_extractor = "video/quicktime";

extern "C" Extractor *instantiate_extractor(void)
{
	return new QuickTimeExtractor();
}


QuickTimeExtractor::QuickTimeExtractor()
{
	qt            = NULL;
	track_formats = NULL;
	tracklist     = NULL;
}

QuickTimeExtractor::~QuickTimeExtractor()
{
	if (track_formats)
		free(track_formats);
	track_formats = NULL;

	tracklist = NULL;

	if (qt)
		delete qt;
	qt = NULL;
	
}


const char *
QuickTimeExtractor::Copyright(void)
{
	return qt->Copyright();
}

status_t
QuickTimeExtractor::GetFileFormatInfo(media_file_format *mfi)
{
	strcpy(mfi->mime_type,      "video/quicktime");
	strcpy(mfi->pretty_name,    "Quicktime Movie File");
	strcpy(mfi->short_name,     "quicktime");
	strcpy(mfi->file_extension, "mov");

	mfi->family = B_QUICKTIME_FORMAT_FAMILY;

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
QuickTimeExtractor::Sniff(int32 *out_streamNum, int32 *out_chunkSize)
{
	status_t 		err;

	if (BPrivate::media_debug) printf("############################### QUICKTIME SNIFF CALLED! ###\n");
	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	if (pio == NULL)
		return B_IO_ERROR;

	qt = new QTAtomHandler();
	qt->SetTo(pio);
	err = qt->Begin();
	if (err != QT_OK)
		return B_BAD_VALUE;
		
	tracklist = qt->TrackList();
	track_formats = (media_format *)calloc(tracklist->CountItems() * sizeof(media_format), 1);
	if (track_formats == NULL)
		return B_NO_MEMORY;
	
	size_t i, j, maxsize = 64*1024, framecount;
	QTTrack *track;
	for(i=0; i < tracklist->CountItems(); i++) {
		MediaIndex *mi;

		track = (QTTrack *)tracklist->ItemAt(i);
		if (track->Type() == QT_VIDEO) {
			mi         = track->VIndex();
			framecount = track->videoFrameCount;
		} else if (track->Type() == QT_AUDIO) {
			mi         = track->AIndex();
			framecount = track->stcoEntryCount;
		} else {
			continue;
		}

		for(j=0; j < framecount; j++) {
			if (mi[j].entrysize > maxsize)
				maxsize = mi[j].entrysize;
		}
	}

	maxsize = (maxsize + 65535) & ~65535;   // round up to a 64k boundary 
	
	*out_streamNum = tracklist->CountItems();
	*out_chunkSize = maxsize << 1;

	return B_OK;
}

status_t
QuickTimeExtractor::TrackInfo(int32 in_stream, media_format *out_format, void **out_info, int32 *out_size)
{
	int32						fRate;
	status_t					err;
	QTTrack 					*track;
	media_format 				*mf;
	
	if (in_stream < 0 || in_stream >= tracklist->CountItems())
		return B_BAD_INDEX;

	track = (QTTrack *)tracklist->ItemAt(in_stream);
	mf = &track_formats[in_stream];

	if (track->Type() == QT_VIDEO) {
		err = BMediaFormats::GetQuicktimeFormatFor(
			0, track->videoCodecList[0].codecID, mf, B_MEDIA_ENCODED_VIDEO);
		mf->user_data_type = B_CODEC_TYPE_INFO;
		// fill out the user_data field even if the previous call failed so
		// that we can at least identify an unhandled format
		sprintf((char*)mf->user_data, "QuickTime 0x%08x-%c%c%c%c", 0,
			track->videoCodecList[0].codecID>>24,
			track->videoCodecList[0].codecID>>16,
			track->videoCodecList[0].codecID>>8,
			track->videoCodecList[0].codecID);
		
		//printf("quicktime video codec 0x%08x, %s\n", track->videoCodecList[0].codecID, strerror(err));
		if (err != B_OK) {
			mf->type = B_MEDIA_ENCODED_VIDEO;
		}
		
		mf->u.encoded_video.avg_bit_rate = 1;
		mf->u.encoded_video.max_bit_rate = 1;
		mf->u.encoded_video.frame_size = 64*1024;
		if(qt->MaxDuration() == 0)
			mf->u.encoded_video.output.field_rate = 0;
		else
			mf->u.encoded_video.output.field_rate = (float)track->videoFrameCount * 1000000.0 /
													 (float)qt->MaxDuration();
		mf->u.encoded_video.output.interlace = 1;
		mf->u.encoded_video.output.last_active = track->videoCodecList[0].height-1;
		mf->u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		mf->u.encoded_video.output.pixel_width_aspect = 1;
		mf->u.encoded_video.output.pixel_height_aspect = 1;

		switch(track->videoCodecList[0].depth) {
			case 16:
				mf->u.encoded_video.output.display.format = B_RGB15_BIG;
				break;
			case 24:
				mf->u.encoded_video.output.display.format = B_RGB24_BIG;
				break;
			case 32:
				mf->u.encoded_video.output.display.format = B_RGB32_BIG;
				break;
			default:
				mf->u.encoded_video.output.display.format = B_NO_COLOR_SPACE;
		}
		mf->u.encoded_video.output.display.bytes_per_row =
			track->videoCodecList[0].depth / 8 * track->videoCodecList[0].width;

		// align to 2 bytes
		mf->u.encoded_video.output.display.bytes_per_row +=
			mf->u.encoded_video.output.display.bytes_per_row & 1;

		mf->u.encoded_video.output.display.line_width = track->videoCodecList[0].width;
		mf->u.encoded_video.output.display.line_count = track->videoCodecList[0].height;
	} else if (track->Type() == QT_AUDIO) {
		media_raw_audio_format *raw;
		audio_smp_details *auds = &track->audioCodecList[0];

		memset(mf, 0, sizeof(*mf));
		fRate = auds->audioSampleRate;
		if ((fRate >= 11022) && (fRate <= 11028))
			fRate = 11025;
		else if ((fRate >= 22044) && (fRate <= 22056))
			fRate = 22050;
		else if ((fRate >= 44088) && (fRate <= 44112))
			fRate = 44100;
		if ((auds->codecID == QT_raw) ||
			(auds->codecID == QT_raw00) ||
			(auds->codecID == QT_twos)) {
			mf->type = B_MEDIA_RAW_AUDIO;
			raw = &mf->u.raw_audio;

			switch(auds->bitsPerSample) {
			case 8:
				/* XXXdbg - is the data signed or unsigned? */
				if (auds->codecID == QT_twos)
					raw->format = media_raw_audio_format::B_AUDIO_CHAR;
				else
					raw->format = media_raw_audio_format::B_AUDIO_UCHAR;
				break;
			case 16:
				raw->format = media_raw_audio_format::B_AUDIO_SHORT;
				break;
			case 32:
				raw->format = media_raw_audio_format::B_AUDIO_INT;
				break;
			default:
				*out_format = *mf;
				return B_BAD_TYPE;
			}

			raw->frame_rate    = fRate;
			raw->channel_count = auds->audioChannels;
			raw->byte_order    = B_MEDIA_BIG_ENDIAN;
			raw->buffer_size   = ((fRate *
								   auds->bitsPerSample *
								   auds->audioChannels) / (20 * 8)) & ~0x3;
		} else {
			err = BMediaFormats::GetQuicktimeFormatFor(
				0, auds->codecID, mf, B_MEDIA_ENCODED_AUDIO);
			if (err != B_OK) {
				mf->type = B_MEDIA_ENCODED_AUDIO;
			}
			mf->user_data_type = B_CODEC_TYPE_INFO;
			sprintf((char*)mf->user_data, "QuickTime 0x%08x-%c%c%c%c", 0,
				auds->codecID>>24,
				auds->codecID>>16,
				auds->codecID>>8,
				auds->codecID);
			//printf("quicktime audio codec 0x%08x, %s\n", auds->codecID, strerror(err));

			mf->u.encoded_audio.output.frame_rate = fRate;
			mf->u.encoded_audio.output.channel_count = auds->audioChannels;
			mf->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
			mf->u.encoded_audio.output.byte_order =
				B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
			mf->u.encoded_audio.output.buffer_size = ((fRate *
								   			   		   auds->bitsPerSample *
								      		   		   auds->audioChannels) / (20 * 8)) & ~0x3;
			mf->u.encoded_audio.bit_rate = auds->bitsPerSample;
		}
	}
	
	*out_format = track_formats[in_stream];
	return B_OK;
}

status_t
QuickTimeExtractor::CountFrames(int32 in_stream, int64 *out_frames)
{
	QTTrack *track;

	if (in_stream < 0 || in_stream >= tracklist->CountItems())
		return B_BAD_INDEX;

	track = (QTTrack *)tracklist->ItemAt(in_stream);

	if (track->Type() == QT_VIDEO) {
		*out_frames = track->videoFrameCount;
	} else if (track->Type() == QT_AUDIO) {
		*out_frames = track->audioFrameCount;
	} else {
		return B_ERROR;
	}

	return B_OK;
}


status_t
QuickTimeExtractor::GetDuration(int32 in_stream, bigtime_t *out_expire_time)
{
	if (in_stream < 0 || in_stream >= tracklist->CountItems())
		return B_BAD_INDEX;

	*out_expire_time = qt->MaxDuration();
	return B_OK;
}

/*
status_t
QuickTimeExtractor::GetExpireTime(int32 in_stream,
								  int64 which_frame,
								  bigtime_t *out_expire_time)
{
	QTTrack *track;
	int i;

	if (in_stream < 0 || in_stream > tracklist->CountItems())
		return B_ERROR;

	track = (QTTrack *)tracklist->ItemAt(in_stream);

	if (track->Type() == QT_VIDEO) {
		if (which_frame < 0)
			which_frame = 0;
		else if (which_frame > track->videoFrameCount)
			which_frame = track->videoFrameCount-1;

		if (track->videoIndex == NULL) {
			double d;
			fprintf(stderr, "Oh shit! no media index!\n");
			d = (double)track->qt_tkhdr.duration /
				(double)track->qt_tkhdr.timescale;

			*out_expire_time = (int64) ((double)which_frame * d);
		}

		*out_expire_time = track->videoIndex[which_frame].expire_time;

	} else if (track->Type() == QT_AUDIO) {
		audio_smp_details *auds;

		auds = &track->audioCodecList[0];
		int64 num_frames = (track->dataSize / ((auds->bitsPerSample / 8) * auds->audioChannels));	
		bigtime_t total_time = (num_frames * 1000000LL) / (bigtime_t)auds->audioSampleRate;
		if (which_frame < 0)
			which_frame = 0;
		else if (which_frame >= num_frames)
			which_frame = num_frames - 1;

		*out_expire_time = (which_frame * total_time)  / num_frames;
	}
	
	return B_OK;
}
*/

status_t 
QuickTimeExtractor::AllocateCookie(int32 in_stream, void **cookieptr)
{
	*cookieptr = calloc(sizeof(int64), 1);
	if(*cookieptr == NULL)
		return B_NO_MEMORY;
	else
		return B_NO_ERROR;
}

status_t 
QuickTimeExtractor::FreeCookie(int32 in_stream, void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

status_t
QuickTimeExtractor::SplitNext(int32   in_stream,
							  void   *cookie,
							  off_t  *inout_filepos,
							  char   *in_packetPointer,
							  int32  *inout_packetLength,
							  char  **out_bufferStart,
							  int32  *out_bufferLength,
							  media_header *mh)
{
	QTTrack *track;
	off_t    pos;
	size_t   size;
	int64    frame;
	int32	 flags;
	int64    *curframeptr = (int64*)cookie;

	if (in_stream < 0 || in_stream >= tracklist->CountItems())
		return B_BAD_INDEX;

	track = (QTTrack *)tracklist->ItemAt(in_stream);

	frame = *curframeptr;
	if (track->Type() == QT_VIDEO) {
		int64 max_frames = track->videoFrameCount;
		mh->u.encoded_video.field_number = 0;	// raw header is the same
		if (frame <= 0) {
			frame = 0;
			mh->start_time = 0;
		} else if (frame >= max_frames) {
			mh->start_time = track->VIndex()[max_frames-1].start_time;
			if (max_frames >= 2)
				mh->start_time += track->VIndex()[max_frames-1].start_time - track->VIndex()[max_frames-2].start_time;
			return B_LAST_BUFFER_ERROR;
		} else {
			mh->start_time = track->VIndex()[frame].start_time;
		};
		mh->u.encoded_video.field_sequence = frame;

		pos   = track->VIndex()[frame].entrypos;
		size  = track->VIndex()[frame].entrysize;
		flags = track->VIndex()[frame].flags;
	} else if (track->Type() == QT_AUDIO) {
		int64 max_frames = track->stcoEntryCount;
		if (frame <= 0) {
			frame = 0;
			mh->start_time = 0;
		} else if (frame >= max_frames) {
			mh->start_time = track->AIndex()[max_frames-1].start_time;
			if (max_frames >= 2)
				mh->start_time += track->AIndex()[max_frames-1].start_time - track->AIndex()[max_frames-2].start_time;
			return B_LAST_BUFFER_ERROR;
		} else {
			mh->start_time = track->AIndex()[frame].start_time;
		};

		pos   = track->AIndex()[frame].entrypos;
		size  = track->AIndex()[frame].entrysize;
	} else {
		return B_ERROR;
	}

	if (pos >= *inout_filepos &&
		pos+size <= (*inout_filepos + *inout_packetLength)) {

		*out_bufferStart  = in_packetPointer + (pos - *inout_filepos);
		mh->orig_size = *out_bufferLength = size;

		if (track->Type() == QT_VIDEO) {
			if (frame+1 < track->videoFrameCount)
				*inout_filepos = track->VIndex()[frame+1].entrypos;
			else
				*inout_filepos = 0;

			mh->type = B_MEDIA_ENCODED_VIDEO;
			mh->u.encoded_video.field_flags = flags;
		} else if (track->Type() == QT_AUDIO) {
			if (frame+1 < track->stcoEntryCount)
				*inout_filepos = track->AIndex()[frame+1].entrypos;
			else
				*inout_filepos = 0;
		}

		mh->file_pos = *inout_filepos;
		
		(*curframeptr)++;
	} else {
		*out_bufferStart    = NULL;
		*inout_filepos      = pos;
		*inout_packetLength = size;
	}

	return B_OK;
}


status_t
QuickTimeExtractor::Seek(int32      in_stream,
						 void      *cookie,
						 int32      to_what,
						 int32		flags,
						 bigtime_t *inout_time,
						 int64     *inout_frame,
						 off_t     *inout_filePos,
						 char      *in_packetPointer,
						 int32     *inout_packetLength,
						 bool      *out_done)
{
	QTTrack *track;
	MediaIndex *index;
	int64 i;
	int64 which_frame = *inout_frame;
	int64 *curframeptr = (int64*)cookie;

	bool seek_forward =
		(flags & B_MEDIA_SEEK_DIRECTION_MASK) == B_MEDIA_SEEK_CLOSEST_FORWARD;

	if (in_stream < 0 || in_stream >= tracklist->CountItems())
		return B_BAD_INDEX;

	track = (QTTrack *)tracklist->ItemAt(in_stream);

	if (track->Type() == QT_VIDEO) {
		index = track->VIndex();

		if (to_what == B_SEEK_BY_TIME) {
			bigtime_t when = *inout_time;
			bigtime_t found_time = 0;
			
			for(i=0; i < track->videoFrameCount; i++) {
				found_time = index[i].start_time;
				if (when <= found_time)
					break;
			}
			if(!seek_forward && when < found_time) {
				which_frame = i-1;
			}
			else {
				which_frame = i;
			}
		}

		if (which_frame < 0)
			which_frame = 0;
		else if (which_frame >= track->videoFrameCount)
			which_frame = track->videoFrameCount-1;
				
		/* find the nearest key frame based on the flags argument */
		bool gotit = false;
		if (seek_forward) {
			for(i=0; i < 60 && which_frame+i < track->videoFrameCount; i++) {
				if (index[which_frame+i].flags == B_MEDIA_KEY_FRAME) {
					gotit = true;
					break;
				}
			}
			if (gotit)
				which_frame += i;
			
		} else {
			for(i=0; i < 60 && which_frame-i >= 0; i++) {
				if (index[which_frame-i].flags == B_MEDIA_KEY_FRAME) {
					gotit = true;
					break;
				}
			}
			if (gotit)
				which_frame -= i;
		}
		
		if(!gotit)
			return B_DEV_SEEK_ERROR;

		*inout_frame = which_frame;
		*inout_frame = (*inout_frame < 0) ? 0 : *inout_frame;
			
		if(!(flags & B_MEDIA_SEEK_PEEK))
			*curframeptr = *inout_frame;
		*inout_filePos      = index[*inout_frame].entrypos;
		*inout_packetLength = index[*inout_frame].entrysize;
		if (*inout_frame == 0)
			*inout_time = 0;
		else
			*inout_time = index[*inout_frame].start_time;

		*out_done = true;
		
	} else if (track->Type() == QT_AUDIO) {
		int64 max_chunks = track->stcoEntryCount;
		int64 frame = *inout_frame;
		audio_smp_details *auds = &track->audioCodecList[0];
		int64 num_frames = (track->dataSize / ((auds->bitsPerSample / 8) * auds->audioChannels));	
		bigtime_t total_time = (num_frames * 1000000LL) / (bigtime_t)auds->audioSampleRate;
		bigtime_t when;

		index = track->AIndex();

		if (to_what == B_SEEK_BY_FRAME) {
			when = (frame * total_time) / num_frames;
		} else {
			when = *inout_time;
		}

		for(i=0; i < track->stcoEntryCount; i++) {
			if (when <= index[i].start_time)
				break;
		}
		frame = i;

		if (frame < 0)
			frame = 0;
		else if (frame >= max_chunks)
			frame = max_chunks - 1;

		if(!(flags & B_MEDIA_SEEK_PEEK))
			*curframeptr = frame;
		*inout_filePos      = index[frame].entrypos;
		*inout_packetLength = index[frame].entrysize;
		if (frame == 0)
			*inout_time = 0;
		else
			*inout_time = index[frame - 1].start_time;

		*inout_frame = (*inout_time * num_frames) / total_time;

		*out_done = true;
	} else {
		return B_ERROR;
	}

	return B_OK;
}
