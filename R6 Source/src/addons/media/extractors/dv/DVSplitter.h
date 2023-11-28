#ifndef DVSPLITTER_H
#define DVSPLITTER_H

#include <Extractor.h>

using namespace BPrivate;

class DVSplitter : public Extractor {
	public:
		DVSplitter();
		
		status_t	GetFileFormatInfo(media_file_format *mfi);
		status_t	SniffFormat(const media_format *input_format,
		        	            int32 *out_streamNum, int32 *out_chunkSize);
		status_t	TrackInfo(int32 stream, media_format *format,
		                      void **info, int32 *infoSize);
		status_t	CountFrames(int32 in_stream, int64 *out_frames);
		status_t	GetDuration(int32 in_stream, bigtime_t *out_duration);
		status_t	AllocateCookie(int32 in_stream, void **cookieptr);
		status_t	FreeCookie(int32 in_stream, void *cookie);

		status_t	SplitNext(int32 in_stream, void *cookie,
							  off_t *inout_filepos, char *in_packetPointer,
							  int32 *inout_packetLength,
							  char **out_bufferStart,
							  int32 *out_bufferLength,
							  media_header *out_mh);
		status_t	Seek(int32 in_stream, void *cookie,
		        	     int32 in_towhat, int32 flags,
		        	     bigtime_t *inout_time, int64 *inout_frame,
		        	     off_t *inout_filePos, char *in_packetPointer,
		        	     int32 *inout_packetLength, bool *out_done);
	private:
		media_format	video_format;
		media_format	audio_format;
		int64			fAudioFramesPerVideoFrame;
};

#endif

