#ifndef _AIFF_EXTRACTOR_H
#define _AIFF_EXTRACTOR_H

#include "Extractor.h"

namespace B {
namespace Media2 {

class AIFFExtractor : public B::Private::Extractor {

public:
						AIFFExtractor();
	virtual				~AIFFExtractor();

	virtual status_t	GetFileFormatInfo(media_file_format *mfi);
	virtual status_t	Sniff(int32 *out_streamNum, int32 *out_chunkSize);
	virtual status_t	TrackInfo(int32 in_stream, media_format *out_format, void **out_info, int32 *out_size);
	virtual status_t	CountFrames(int32 in_stream, int64 *out_frames);
	virtual status_t	GetDuration(int32 in_stream, bigtime_t *out_duration);
	virtual status_t	AllocateCookie(int32 in_stream, void **cookieptr);
	virtual status_t	FreeCookie(int32 in_stream, void *cookie);
	virtual status_t	SplitNext(int32 in_stream, void *cookie, off_t *inout_filepos,
								  char *in_packetPointer, int32 *inout_packetLength,
								  char **out_bufferStart, int32 *out_bufferLength,
								  media_header *mh);
	virtual status_t	Seek(int32 in_stream, void *cookie, int32 in_towhat, int32 flags,
							 bigtime_t *inout_time,
							 int64 *inout_frame, off_t *inout_filePos,
							 char *in_packetPointer, int32 *inout_packetLength,
							 bool *out_done);

private:
	status_t			ReadHeader();
	status_t			ReadCommon();
	status_t			ReadData();
	status_t			ReadVersion();

	/* format description */
	uint32				fSampleFormat;
	uint32				fChannelCount;
	uint32				fFrameCount;
	uint32				fFrameSize;
	float				fRate;
	/* pseudo-bloc management */
	uint32				fBlocSize;
	uint32				fFramePerBloc;
	float				fTimeFactor;
	/* data access */
	uint32				fDataOffset;
	uint32				fDataEnd;
	uint32				fDataLength;
};

} } // B::Media2
#endif
