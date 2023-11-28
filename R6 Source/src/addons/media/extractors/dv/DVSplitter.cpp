
#include "DVSplitter.h"
#include <MediaFormats.h>
#include <string.h>

#if 0
#include <stdio.h>
#define INFO(x) printf x
#define ERROR(x) printf x
#else
#define INFO(x)
#define ERROR(x)
#endif

media_encoded_video_format::video_encoding dvstream_encoding;
media_encoded_video_format::video_encoding dvvideo_encoding;
media_encoded_audio_format::audio_encoding dvaudio_encoding;
static media_format mediaFormat[3];

void register_extractor(const media_format ** out_formats, int32 * out_count)
{
	INFO(("DVSplitter loaded\n"));
	status_t 					err;
	media_format_description	formatDescription[5];
	BMediaFormats				formatObject;

	mediaFormat[0].type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat[0].u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));

	formatDescription[0].family = B_AVI_FORMAT_FAMILY;
	formatDescription[0].u.avi.codec = 'cdvc';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'CDVC';
	formatDescription[2].family = B_AVI_FORMAT_FAMILY;
	formatDescription[2].u.avi.codec = 'dvsd';
	formatDescription[3].family = B_AVI_FORMAT_FAMILY;
	formatDescription[3].u.avi.codec = 'pcdv';
	formatDescription[4].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[4].u.quicktime.vendor = 0;
	formatDescription[4].u.quicktime.codec  = 'dvc ';

	err = formatObject.MakeFormatFor(formatDescription, 5, &mediaFormat[0]);
	if(err != B_NO_ERROR) {
		ERROR(("DVSplitter: MakeFormatFor failed, %s\n", strerror(err)));
	}
	dvstream_encoding = mediaFormat[0].u.encoded_video.encoding;
	
	mediaFormat[1].type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat[1].u.encoded_video = media_encoded_video_format::wildcard;
	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'dv_v';
	err = formatObject.MakeFormatFor(formatDescription, 1, &mediaFormat[1]);
	if(err != B_NO_ERROR) {
		ERROR(("DVSplitter: MakeFormatFor failed, %s\n", strerror(err)));
	}
	dvvideo_encoding = mediaFormat[1].u.encoded_video.encoding;
	
	mediaFormat[2].type = B_MEDIA_ENCODED_AUDIO;
	mediaFormat[2].u.encoded_audio = media_encoded_audio_format::wildcard;
	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'dv_a';
	err = formatObject.MakeFormatFor(formatDescription, 1, &mediaFormat[2]);
	if(err != B_NO_ERROR) {
		ERROR(("DVSplitter: MakeFormatFor failed, %s\n", strerror(err)));
	}
	dvaudio_encoding = mediaFormat[2].u.encoded_audio.encoding;

	*out_formats = mediaFormat;
	*out_count = 3;
}

Extractor *
instantiate_extractor()
{
	if ((dvstream_encoding == 0) && (dvvideo_encoding == 0) &&
			(dvaudio_encoding == 0))
		return 0;
	return new DVSplitter();
}


DVSplitter::DVSplitter()
{
	fAudioFramesPerVideoFrame = 1470;
}

status_t 
DVSplitter::GetFileFormatInfo(media_file_format *mfi)
{
	strcpy(mfi->mime_type,      "video/x-dv");
	strcpy(mfi->pretty_name,    "DV Stream");
	strcpy(mfi->short_name,     "dv");
	strcpy(mfi->file_extension, "dv");

	mfi->family = B_BEOS_FORMAT_FAMILY;

    mfi->capabilities = media_file_format::B_READABLE              |
                        media_file_format::B_PERFECTLY_SEEKABLE    |
                        media_file_format::B_KNOWS_ENCODED_VIDEO   |
                        media_file_format::B_KNOWS_ENCODED_AUDIO;

	return B_OK;
}

status_t 
DVSplitter::SniffFormat(const media_format *input_format,
                        int32 *out_streamNum, int32 *out_chunkSize)
{
	if (input_format->type != B_MEDIA_ENCODED_VIDEO)
		return B_BAD_TYPE;

	if (input_format->u.encoded_video.encoding != dvstream_encoding)
		return B_BAD_TYPE;

	video_format = *input_format;
	video_format.u.encoded_video.encoding = dvvideo_encoding;
	if (video_format.u.encoded_video.output.last_active == 575) {
		video_format.u.encoded_video.frame_size = 144000;
	} else if (video_format.u.encoded_video.output.last_active == 479) {
		video_format.u.encoded_video.frame_size = 120000;
	}
	video_format.user_data_type = B_CODEC_TYPE_INFO;
	strcpy((char *)video_format.user_data, "DV Video Stream");

	audio_format.type = B_MEDIA_ENCODED_AUDIO;
	audio_format.u.encoded_audio = media_encoded_audio_format::wildcard;
	audio_format.u.encoded_audio.encoding = dvaudio_encoding;
	audio_format.user_data_type = B_CODEC_TYPE_INFO;

#if 0
	audio_format.type = B_MEDIA_RAW_AUDIO;
	audio_format.u.raw_audio = media_raw_audio_format::wildcard;
	audio_format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_UCHAR;
	audio_format.u.raw_audio.frame_rate = 44100;
	audio_format.u.raw_audio.channel_count = 2;
	audio_format.u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	audio_format.u.raw_audio.buffer_size = 64*1024;
#endif
	
	audio_format.user_data_type = B_CODEC_TYPE_INFO;

	strcpy((char *)audio_format.user_data, "DV Audio Stream");
	
	*out_streamNum = 2;
	*out_chunkSize = 512*1024;
	
	return B_NO_ERROR;
}

status_t 
DVSplitter::TrackInfo(int32 stream, media_format *format,
                      void **, int32 *)
{
	if(stream == 0)
		*format = video_format;
	else if(stream == 1)
		*format = audio_format;
	else
		return B_BAD_INDEX;
	return B_NO_ERROR;
}

status_t 
DVSplitter::CountFrames(int32 stream, int64 *out_frames)
{
	if(fSourceExtractor == NULL)
		return B_NOT_ALLOWED;
	if(stream == 0)
		return fSourceExtractor->CountFrames(fSourceStream, out_frames);
	else if(stream == 1) {
		int64 frames;
		status_t err;
		err = fSourceExtractor->CountFrames(fSourceStream, &frames);
		if(err == B_NO_ERROR)
			*out_frames = frames * fAudioFramesPerVideoFrame;
		return err;
	}
	else
		return B_BAD_INDEX;
}

status_t 
DVSplitter::GetDuration(int32 stream, bigtime_t *out_duration)
{
	if(fSourceExtractor == NULL)
		return B_NOT_ALLOWED;
	if(stream == 0 || stream == 1)
		return fSourceExtractor->GetDuration(fSourceStream, out_duration);
	else
		return B_BAD_INDEX;
}

status_t 
DVSplitter::AllocateCookie(int32, void **cookieptr)
{
	if(fSourceExtractor == NULL)
		return B_NOT_ALLOWED;
	return fSourceExtractor->AllocateCookie(fSourceStream, cookieptr);
}

status_t 
DVSplitter::FreeCookie(int32, void *cookie)
{
	if(fSourceExtractor == NULL)
		return B_NOT_ALLOWED;
	return fSourceExtractor->FreeCookie(fSourceStream, cookie);
}

status_t 
DVSplitter::SplitNext(int32 stream, void *cookie, off_t *inout_filepos,
                      char *in_packetPointer, int32 *inout_packetLength,
                      char **out_bufferStart, int32 *out_bufferLength,
                      media_header *out_mh)
{
	status_t err;
	if(fSourceExtractor == NULL)
		return B_NOT_ALLOWED;

	if(stream < 0 || stream > 1)
		return B_BAD_INDEX;

	err = fSourceExtractor->SplitNext(fSourceStream, cookie, inout_filepos,
	                                  in_packetPointer, inout_packetLength,
	                                  out_bufferStart, out_bufferLength,
	                                  out_mh);

	if(stream == 1) {
		out_mh->type = B_MEDIA_ENCODED_AUDIO;
		// ....
	}

	//inout_filepos += *inout_packetLength;
	//*out_bufferStart = in_packetPointer;
	//*out_bufferLength = *inout_packetLength;
	return err;
}

status_t 
DVSplitter::Seek(int32 stream, void *cookie, int32 towhat, int32 flags,
                 bigtime_t *inout_time, int64 *inout_frame,
                 off_t *inout_filePos, char *in_packetPointer,
                 int32 *inout_packetLength, bool *out_done)
{
	if(fSourceExtractor == NULL)
		return B_NOT_ALLOWED;
	if(stream == 0) {
		return fSourceExtractor->Seek(fSourceStream, cookie, towhat, flags, inout_time,
		                              inout_frame, inout_filePos,
		                              in_packetPointer, inout_packetLength,
		                              out_done);
	}
	else if(stream == 1) {
		status_t err;
		int64 frame = *inout_frame / fAudioFramesPerVideoFrame;

		err = fSourceExtractor->Seek(fSourceStream, cookie, towhat, flags, inout_time,
		                             &frame, inout_filePos,
		                             in_packetPointer, inout_packetLength,
		                             out_done);
		*inout_frame = frame * fAudioFramesPerVideoFrame;
		return err;
	}
	else
		return B_BAD_INDEX;
}
