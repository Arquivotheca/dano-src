#ifndef C_MPEG2_EXTRACTOR_H

#define C_MPEG2_EXTRACTOR_H

#include <Extractor.h>

#include "MPEG2Track.h"
#include "PointerList.h"

class CBitStream;
class CMemoryBitStream;

class CMPEG2Extractor : public BPrivate::Extractor
{
	status_t SniffSystemHeader (CBitStream &bs);
	status_t SniffPackHeader (CBitStream &bs);

	status_t SniffPESPacket (CBitStream &bs, bool scan_tracks,
								uint8 *which_stream=NULL,
								uint8 *which_substream=NULL,
								size_t *chunk_length=NULL);

	status_t GetStreamData(CMemoryBitStream &bs, uint8 stream_id,
													uint8 substream_id,
													size_t *chunk_length);

	status_t ScanProgramStream(CBitStream &bs);
	status_t Sniff();
	
	CPointerList<CMPEG2Track> fTracks;	
	size_t fChunkSize;

	void *fBuffer;
	size_t fBufferSize;
	
	public:
		CMPEG2Extractor();
		virtual ~CMPEG2Extractor();
		
		virtual status_t Sniff (int32 *out_streamNum, int32 *out_chunkSize);

		virtual status_t GetFileFormatInfo (media_file_format *mfi);

		virtual status_t TrackInfo (int32 in_stream, media_format *out_format,
									void **out_info, int32 *out_infoSize);
									
		virtual status_t CountFrames (int32 in_stream, int64 *out_frames);
		virtual status_t GetDuration (int32 in_stream, bigtime_t *out_duration);
	
		virtual status_t AllocateCookie (int32 in_stream, void **cookieptr);
		virtual	status_t FreeCookie (int32 in_stream, void *cookie);
	
		virtual status_t SplitNext (int32 in_stream, void *cookie,
									  off_t *inout_filepos,
									  char *in_packetPointer,
									  int32 *inout_packetLength,
									  char **out_bufferStart,
									  int32 *out_bufferLength,
									  media_header *out_mh);								

		virtual status_t Seek (int32 in_stream, void *cookie,
		                         int32 in_towhat, int32 flags,
								 bigtime_t *inout_time, int64 *inout_frame,
								 off_t *inout_filePos, char *in_packetPointer,
								 int32 *inout_packetLength, bool *out_done);
};

#endif
