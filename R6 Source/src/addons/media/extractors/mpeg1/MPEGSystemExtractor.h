#ifndef MPEGSYSTEMEXTRACTOR_H
#define MPEGSYSTEMEXTRACTOR_H

#include "Extractor.h"

using namespace BPrivate;

/* copied/adapted from Xing's VBR SDK */
// A Xing header may be present in the ancillary
// data field of the first frame of an mp3 bitstream
// The Xing header (optionally) contains
//      frames      total number of audio frames in the bitstream
//      bytes       total number of bytes in the bitstream
//      toc         table of contents

// toc (table of contents) gives seek points
// for random access
// the ith entry determines the seek point for
// i-percent duration
// seek point in bytes = (toc[i]/256.0) * total_bitstream_bytes
// e.g. half duration seek point = (toc[50]/256.0) * total_bitstream_bytes


#define XING_FRAMES_FLAG     0x0001
#define XING_BYTES_FLAG      0x0002
#define XING_TOC_FLAG        0x0004
#define XING_VBR_SCALE_FLAG  0x0008

// this code isn't finished yet
#define GUESS_SEEK 0

#define XING_FRAMES_AND_BYTES (XING_FRAMES_FLAG | XING_BYTES_FLAG)

// structure to receive extracted header
typedef struct {
    int h_id;       		// from MPEG header, 0=MPEG2, 1=MPEG1
    int samprate;   		// determined from MPEG header
    int flags;      		// from Xing header data
    int frames;     		// total bit stream frames from Xing header data
    int bytes;      		// total bit stream bytes from Xing header data
    int vbr_scale;  		// encoded vbr scale from Xing header data
    unsigned char toc[100];	// seekpoints for this file
}   XHEADDATA;
/* end of Xing stuff */

const int32 kInvalidStream = -1;
const int32 kVideoStream = 0;
const int32 kAudioStream = 1;

#if 1
struct layer_header {
	struct layer_header *sequence;
	struct layer_header *next;
	off_t		offset;
	bigtime_t	t;
	int64		f;
	int32		len; // remaining bytes in the packet
	uchar		header_type;
	uchar		stream_id;
};
#else
struct layer_header {
	uchar		header_type;
	uchar		stream_id;
	off_t		offset;
	int32		len; // remaining bytes in the packet
	bigtime_t	t;
	struct layer_header *next;
};
#endif

struct layer {
	off_t offset;
	struct layer *next;
	int32 len;
};

struct stream {
	double fps;
	int64 f;
	struct {
		bigtime_t t;
		struct layer_header *seq;	// most-recent sequence header
	} temp;		// temporary state
	struct {
		struct layer_header *header;
		struct layer_header *recent_layer_header;
		struct layer *recent_layer;
		int state;
	} seek;		// seeking state

	struct layer_header *h, *h_tail;
	struct layer *l, *l_tail;
};

#define STREAM_ID_BASE 0xb8

struct MPEG_info {
	int32 stream_mapping[2];
	struct stream streams[0x100 - STREAM_ID_BASE];

	MPEG_info() {
		memset(stream_mapping, 0, sizeof(stream_mapping));
		memset(streams, 0, sizeof(streams));
	};
	
	~MPEG_info() {
		for (int32 i=0;i<0x100 - STREAM_ID_BASE;i++) {
			for (;streams[i].h;streams[i].h=streams[i].h_tail) {
				streams[i].h_tail = streams[i].h->next;
				free(streams[i].h);
			}
			for (;streams[i].l;streams[i].l=streams[i].l_tail) {
				streams[i].l_tail = streams[i].l->next;
				free(streams[i].l);
			}
		}
	}
};

class MPEGSystemExtractor : public Extractor {
public:

	MPEGSystemExtractor();
	~MPEGSystemExtractor();

	virtual status_t GetFileFormatInfo(media_file_format *mfi);
	virtual status_t Sniff(int32 *out_streamNum, int32 *out_chunkSize);
	virtual status_t TrackInfo(int32 in_stream, media_format *out_format, void **out_info,
						int32 *out_size);

	virtual status_t AllocateCookie(int32 in_stream, void **cookieptr);
	virtual status_t FreeCookie(int32 in_stream, void *cookie);

	virtual status_t Seek(int32      in_stream,
	                      void      *cookie,
					      int32      to_what,
					      int32      flags,
					      bigtime_t *inout_time,
					      int64     *inout_frame,
					      off_t     *inout_filePos,
					      char      *in_packetPointer,
					      int32     *inout_packetLength,
					      bool      *out_done);
	virtual status_t SplitNext(int32 in_stream,
	                           void  *cookie,
							   off_t *inout_filepos,
							   char *in_packetPointer,
							   int32 *inout_packetLength,
							   char **out_bufferStart,
							   int32 *out_bufferLength,
							   media_header *mh);
	virtual status_t CountFrames(int32 in_stream, int64 *out_frames);
	virtual status_t GetDuration(int32 in_stream, bigtime_t *out_expireTime);

private:
	status_t		 GetFormatInfo();

	status_t         build_audio_format_info(unsigned char *header, int len);
	status_t         build_video_format_info(unsigned char *header, int len);
	status_t         mpeg_audio_seek(bigtime_t time, off_t *new_fpos, bool seekBack) const;

	ssize_t          ParsePack(uchar *buffer, int32 length, off_t off,
							   uchar stream_id, struct MPEG_info *info);
	ssize_t          FindPacks(uchar *buffer, int32 length, off_t off,
							   uchar *stream_id, off_t *stream_remaining,
							   struct MPEG_info *info);

#if GUESS_SEEK
	int64            SeekForwardFrom(int64 where, uint8 type, uint8 mask, int32 *length);
#endif
	static long      backgroundscanner(void *self);
	long             BackgroundScanner();

	struct MPEG_info mpeg;

	off_t	  		 fFileSize;
	off_t			 fAudioStartOffset;	/* offset of the first audio frame */
	off_t            fActualAudioSizeSoFar; /* actual size of audio as determined by background scanner so far */
	off_t			 fAudioSize;		/* (totalfilesize/parsedsize)*fActualAudioSizeSoFar */
	float			 fAudioFrameSize;		/* avg. size of an MPEG audio frame
										   FrameSize = 144 * BitRate / SampleRate */
	
	bigtime_t        fAudioDuration;    /* usecs (an approximation) */
	int64            fAudioNumFrames;   /* output frames (approximation) */
	//off_t            fAudioCurPos;

	double	 		 fFrameRate;
	uchar     		*fSeekBuf;

	int32            fFileType;    /* 0 = audio, 1 = video, 2 = system */
	int32            fNumStreams;
	
	bool             fHaveVideo;
	media_format 	 fVideoFormat;

	bool             fHaveAudio;
	media_format 	 fAudioFormat;

	XHEADDATA       *fXingHeader;

	off_t            fDataStart;
	uchar            fStreamID;
	thread_id        fScanThread;
};

const int32  kAudioStreamOnly = 0;
const int32  kVideoStreamOnly = 1;
const int32  kSystemStream    = 2;


#endif
