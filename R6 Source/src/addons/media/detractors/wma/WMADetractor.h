
#ifndef _WMADETRACTOR_H
#define _WMADETRACTOR_H

#include <Detractor.h>
#include <DataIO.h>
#include "WMA_Dec_Emb_x86.h"

class WMADetractor: public Detractor
{
	public:
		WMADetractor();
		virtual ~WMADetractor();

	// this is the "mediafile-part", needed to support the
	// (read only) BMediaFile calls.
		virtual status_t SetTo(const entry_ref *ref);
		virtual status_t SetTo(BDataIO *source);

		virtual status_t InitCheck() const;

		virtual status_t GetFileFormatInfo(media_file_format *mfi) const;
		virtual const char* Copyright(void) const;
		virtual int32 CountTracks() const;

	// this is the "mediatrack-part"
		virtual status_t GetCodecInfo(int32 tracknum, media_codec_info *mci) const;
		virtual status_t EncodedFormat(int32 tracknum, media_format *out_format) const;
		virtual status_t DecodedFormat(int32 tracknum, media_format *inout_format);
		virtual int64    CountFrames(int32 tracknum) const;
		virtual bigtime_t Duration(int32 tracknum) const;
		virtual int64    CurrentFrame(int32 tracknum) const;
		virtual bigtime_t CurrentTime(int32 tracknum) const;
		virtual status_t ReadFrames(int32 tracknum, void *out_buffer, int64 *out_frameCount,
							   media_header *mh = NULL);
		virtual status_t SeekTo(int32 tracknum, int32 to_what, bigtime_t *inout_time, int64 *inout_frame, int32 flags=0);
		virtual status_t FindKeyFrameForFrame(int32 tracknum, int64 *inout_frame, int32 flags=0) const;

		tWMA_U32 WMAFileCBGetData(
		    tHWMAFileState hstate,
		    tWMA_U32 offset,
		    tWMA_U32 num_bytes,
		    unsigned char **ppData);

		tWMA_U32 WMAFileCBGetLicenseData(
		    tHWMAFileState *state,
		    tWMA_U32 offset,
		    tWMA_U32 num_bytes,
		    unsigned char **ppData);

private:
		float SampleRate() const;

		int64 fCurrentFrame;
		BPositionIO *fSource;

		tWMAFileHdrState g_hdrstate;
		tHWMAFileState g_state;
		tWMAFileHeader g_hdr;
		unsigned char g_pBuffer[WMA_MAX_DATA_REQUESTED];
		tWMA_U32 g_cbBuffer;
		tWMA_U32 g_cbBufLic;
		unsigned char g_pBufLic[WMA_MAX_DATA_REQUESTED];
		tWMAFileLicParams g_lic;
		unsigned char g_pmid[20];
		FILE *g_fpLic;
		
		bool fNeedToDecode;
};

extern WMADetractor* currentdetractor;


#endif
