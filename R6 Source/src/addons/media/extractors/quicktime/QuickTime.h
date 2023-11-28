#ifndef _QUICKTIME_H_
#define _QUICKTIME_H_

#include "Extractor.h"
#include "MediaIndex.h"
#include "QTAtomHandler.h"
#include "QTCodecDetails.h"
#include "QTStructures.h"
#include "QTTrack.h"

using namespace BPrivate;


class QuickTimeExtractor : public Extractor
{
public:
	QuickTimeExtractor();
	~QuickTimeExtractor();


	const char  *Copyright(void);
	status_t	GetFileFormatInfo(media_file_format *mfi);
	status_t	Sniff(int32 *out_streamNum, int32 *out_chunkSize);
	status_t	TrackInfo(int32 in_stream, media_format *out_format, void **out_info, int32 *out_size);
	status_t	CountFrames(int32 in_stream, int64 *out_frames);
	status_t    GetDuration(int32 in_stream, bigtime_t *out_expireTime);

	status_t	AllocateCookie(int32 in_stream, void **cookieptr);
	status_t	FreeCookie(int32 in_stream, void *cookie);

	status_t	SplitNext(int32   in_stream,
	                      void   *cookie,
						  off_t  *inout_filepos,
						  char   *in_packetPointer,
						  int32  *inout_packetLength,
						  char  **out_bufferStart,
						  int32  *out_bufferLength,
						  media_header *mh);
	status_t     Seek(int32      in_stream,
	                  void      *cookie,
					  int32      to_what,
					  int32		 flags,
					  bigtime_t *inout_time,
					  int64     *inout_frame,
					  off_t     *inout_filePos,
					  char      *in_packetPointer,
					  int32     *inout_packetLength,
					  bool      *out_done);

private:
	QTAtomHandler *qt;
	BList         *tracklist;

	media_format  *track_formats;   /* an array, 1 per track */
};


#endif /* _QUICKTIME_H_ */
