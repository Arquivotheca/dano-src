#include <File.h>
#include <stdio.h>
#include <MediaFormats.h>
#include <MediaTrack.h>
#include <Locker.h>
#include <Autolock.h>
#include <Application.h>
#include <Roster.h>
#include "MPEGSystemExtractor.h"

#define DEFAULT_CHUNK_SIZE	0x10000
#define SEEK_CHUNK_SIZE		0x10000

//#define DEBUG printf
#define DEBUG if (0) printf
#define SEEK(x) //printf x
#define SEQ(x) //printf x

const double FUDGE = 1.00416;

const double pictureRateCode[] = {
	-1, 23.976, 24, 25, 29.97, 30, 50, 59.94,
	60, -1, -1, -1, -1, -1, -1, -1
};

unsigned short layer1_bit_rate_11172[] = {
	0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0
};

unsigned short layer2_bit_rate_11172[] = {
	0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0
};

unsigned short layer3_bit_rate_11172[] = {
	0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
};

unsigned short layer1_bit_rate_13818[] = {
	0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0
};

unsigned short layer2_bit_rate_13818[] = {   /* also for layer 3 */
	0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0
};


char *mode_table[] = { "Stereo", "Joint Stereo", "Dual Channel", "Mono" };

extern "C" const char * mime_type_extractor = "video/mpeg";

static media_encoded_video_format::video_encoding mpeg1_video;
static media_encoded_audio_format::audio_encoding mpeg1_audio1;
static media_encoded_audio_format::audio_encoding mpeg1_audio2;
static media_encoded_audio_format::audio_encoding mpeg1_audio3;

#define HEADER_SIZE 12


BLocker locker("mpeg extractor lock");

Extractor* instantiate_extractor()
{
	BAutolock autolock(&locker);
	static bool initialized = FALSE;

	if (!initialized) {
		BMediaFormats bmf;
		media_format_description d;
		status_t err;
		media_format fmt;

		initialized = TRUE;

		bmf.Lock();

		memset(&d, 0, sizeof(d));
		d.family = B_MPEG_FORMAT_FAMILY;
		d.u.mpeg.id = B_MPEG_1_VIDEO;
		err = bmf.GetFormatFor(d, &fmt);
		if (err == B_OK) {
			mpeg1_video = fmt.u.encoded_video.encoding;
		}
		else {
			memset(&fmt, 0, sizeof(fmt));
			fmt.type = B_MEDIA_ENCODED_VIDEO;
			err = bmf.MakeFormatFor(d, fmt, &fmt);
			if (err == B_OK) {
				mpeg1_video = fmt.u.encoded_video.encoding;
			}
		}

		memset(&d, 0, sizeof(d));
		d.family = B_MPEG_FORMAT_FAMILY;
		d.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_1;
		err = bmf.GetFormatFor(d, &fmt);
		if (err == B_OK) {
			mpeg1_audio1 = fmt.u.encoded_audio.encoding;
		}
		else {
			memset(&fmt, 0, sizeof(fmt));
			fmt.type = B_MEDIA_ENCODED_AUDIO;
			err = bmf.MakeFormatFor(d, fmt, &fmt);
			if (err == B_OK) {
				mpeg1_audio1 = fmt.u.encoded_audio.encoding;
			}
		}

		memset(&d, 0, sizeof(d));
		d.family = B_MPEG_FORMAT_FAMILY;
		d.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_2;
		err = bmf.GetFormatFor(d, &fmt);
		if (err == B_OK) {
			mpeg1_audio2 = fmt.u.encoded_audio.encoding;
		}
		else {
			memset(&fmt, 0, sizeof(fmt));
			fmt.type = B_MEDIA_ENCODED_AUDIO;
			err = bmf.MakeFormatFor(d, fmt, &fmt);
			if (err == B_OK) {
				mpeg1_audio2 = fmt.u.encoded_audio.encoding;
			}
		}

		memset(&d, 0, sizeof(d));
		d.family = B_MPEG_FORMAT_FAMILY;
		d.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_3;
		err = bmf.GetFormatFor(d, &fmt);
		if (err == B_OK) {
			mpeg1_audio3 = fmt.u.encoded_audio.encoding;
		}
		else {
			memset(&fmt, 0, sizeof(fmt));
			fmt.type = B_MEDIA_ENCODED_AUDIO;
			err = bmf.MakeFormatFor(d, fmt, &fmt);
			if (err == B_OK) {
				mpeg1_audio3 = fmt.u.encoded_audio.encoding;
			}
		}

		bmf.Unlock();
	}

	return new MPEGSystemExtractor;
}

MPEGSystemExtractor::MPEGSystemExtractor()
	:	fFileSize(0),
		fAudioStartOffset(0),
		fActualAudioSizeSoFar(0),
		fAudioSize(0),
		fAudioDuration(0),
		fAudioNumFrames(0),
		fFileType(-1),
		fNumStreams(0),
		fHaveVideo(false),
		fHaveAudio(false),
		fXingHeader(NULL),
		fScanThread(-1)
{
	fSeekBuf = (uchar *)malloc(SEEK_CHUNK_SIZE);
}

MPEGSystemExtractor::~MPEGSystemExtractor()
{
	if(fScanThread != -1)
	{
		thread_id tmp=fScanThread;
		fScanThread = -1;
		status_t dummy;
		wait_for_thread(tmp,&dummy);
	}

	if (fSeekBuf)
		free(fSeekBuf);
	
	delete fXingHeader;
}




status_t MPEGSystemExtractor::GetFileFormatInfo(media_file_format *mfi)
{
	strcpy(mfi->pretty_name,    "MPEG File");
	strcpy(mfi->short_name,     "mpeg");
	strcpy(mfi->file_extension, "mpg");

	mfi->family = B_MPEG_FORMAT_FAMILY;

	mfi->capabilities = media_file_format::B_READABLE
					|   media_file_format::B_IMPERFECTLY_SEEKABLE;
	
	if (fFileType == kAudioStreamOnly) {
		strcpy(mfi->mime_type, "audio/mpeg");
		mfi->capabilities |= media_file_format::B_KNOWS_ENCODED_AUDIO;
	} else if (fFileType == kVideoStreamOnly) {
		strcpy(mfi->mime_type, "video/mpeg");
		mfi->capabilities |= media_file_format::B_KNOWS_ENCODED_VIDEO;
	} else if (fFileType == kSystemStream) {
		strcpy(mfi->mime_type, "video/mpeg");
		if (fHaveVideo)
			mfi->capabilities |= media_file_format::B_KNOWS_ENCODED_VIDEO;
		if (fHaveAudio)
			mfi->capabilities |= media_file_format::B_KNOWS_ENCODED_AUDIO;
	}
	
	return B_OK;
}



static int
is_mpeg_audio_header(unsigned char *hdr, size_t len, bool knownBitRate,
                     int recursionDepth = 0, int match_sample_rate = 0,
                     int match_layer = 0)
{
	if (len < 4) {
		return recursionDepth > 1;
	}

	int index, bit_rate, layer, sample_rate, id;
	bool padding;
	
	if (hdr[0] != 0xff || (hdr[1] & 0xe0) != 0xe0) {
		return 0;
	}
	/* set layer = 1, 2, or 3 */
	layer = 4 - ((hdr[1] & 0x6) >> 1);
	if (layer == 4) {
		return 0;
	}
	if(match_layer != 0 && match_layer != layer)
		return 0;

	/* set id */
	id = (hdr[1] & 0x018) >> 3;

	/* check that the bit_rate index is in range */
	index = (hdr[2] & 0xf0) >> 4;
	if (index == 15) {
		return 0;
	}

	/* set the bit_rate */
	if ((id&1) == 1) {    /* 11172 format tables */
		if (layer == 1)
			bit_rate = layer1_bit_rate_11172[index];
		else if (layer == 2)
			bit_rate = layer2_bit_rate_11172[index];
		else /*if (layer == 3)*/
			bit_rate = layer3_bit_rate_11172[index];
	} else {
		if (layer == 1)
			bit_rate = layer1_bit_rate_13818[index];
		else /*if (layer == 2 || layer == 3)*/
			bit_rate = layer2_bit_rate_13818[index];
	}
	if (knownBitRate && bit_rate == 0) {
		return 0;
	}
	
	/* check that the sample rate index is in range */
	index = (hdr[2] & 0x0c) >> 2;

	/* calculate sample_rate */
	switch (index) {
	case 0:
		sample_rate = ((id&1) == 0) ? 22050 : 44100;
		break;
	case 1:
		sample_rate = ((id&1) == 0) ? 24000 : 48000;
		break;
	case 2:
		sample_rate = ((id&1) == 0) ? 16000 : 32000;
		break;
	default:
		return 0;
	}
	if(!(id&2))
		sample_rate /= 2;
	if(match_sample_rate != 0 && match_sample_rate != sample_rate)
		return 0;
	
	padding = ((hdr[2] & 0x2) == 2);
	
	/* check that the emphasis field is not illegal */
	if ((hdr[3] & 3) == 2) {
		return 0;
	}

	if (bit_rate == 0) {
		/* if we get here it is likely that this is indeed an mpeg audio header */
//		printf("found audio header, bit_rate = %d, sample_rate = %d, padding = %d, layer = %d, len = %lu\n", bit_rate, sample_rate, padding, layer, len);
		return 1;
	}
	
	/* calculate the size of this frame */
	size_t frame_size;
	int pad_bytes;
	if (layer == 1) {
		pad_bytes = padding ? 4 : 0;
		frame_size = ((size_t)((12 * 1000 * bit_rate) / (float)sample_rate)) * 4 + pad_bytes;
	} else {
		pad_bytes = padding ? 1 : 0;
		if (layer == 3 && (id&1) == 0) {
			// special case for MPEG-2 layer 3
			frame_size = (size_t)((72 * 1000 * bit_rate) / (float)sample_rate) + pad_bytes;		
		} else {
			frame_size = (size_t)((144 * 1000 * bit_rate) / (float)sample_rate) + pad_bytes;				
		}
	}

	//printf("frame_size = %lu, bit_rate = %d, sample_rate = %d, padding = %d, layer = %d, len = %lu\n", frame_size, bit_rate, sample_rate, padding, layer, len);
	if (len < frame_size) {
		return recursionDepth > 1;
	} else if ((frame_size == len) || (recursionDepth > 8)) {
		// this is the last frame we can look at with this data, and it matched size
		return 1;
	} else {
		//printf("recursing - depth %ld\n", recursionDepth);
		int r = is_mpeg_audio_header(hdr + frame_size, len - frame_size, knownBitRate, recursionDepth + 1, sample_rate, layer);
		if (r == 0) {
//			printf("\tdata = %02x %02x - %02x %02x %02x %02x - %02x %02x\n",
//				hdr[frame_size-2], hdr[frame_size-1], hdr[frame_size],
//				hdr[frame_size+1], hdr[frame_size+2], hdr[frame_size+3],
//				hdr[frame_size+4], hdr[frame_size+5]);
		}
		return r;
	}
}



static int
find_mpeg_header(unsigned char *buff, int len)
{
	int i;
	for(i=0; i < len-4; i++) {
		if (buff[i] == 0 && buff[i+1] == 0 && buff[i+2] == 1) {
			if (buff[i+3] == 0xb3 || buff[i+3] == 0xba)
				return i;
		} else if (is_mpeg_audio_header(&buff[i], len - i, false)) {
			return i;
		}
	}

	return -1;
}

static int32
StreamType(uchar type)
{
	if (type == 0xb8)
		return kAudioStream;

	if (type == 0xb9)
		return kVideoStream;

	if ((type & 0xe0) == 0xc0)
		return kAudioStream;

	if ((type & 0xf0) == 0xe0)
		return kVideoStream;

	return kInvalidStream;
}

static status_t 
DecodeTime(uchar *stream, bigtime_t *t)
{
	if (	((stream[0] & 0x01) != 0x01) ||
			((stream[2] & 0x01) != 0x01) ||
			((stream[4] & 0x01) != 0x01)) {
		DEBUG("BAD CLOCK REFERENCE\n");
		return B_ERROR;
	}

	// 90kHz clock
	*t =
		((((bigtime_t)(stream[0] & 0x0e)) << 30) |
		 (((bigtime_t)(stream[1] & 0xff)) << 22) |
		 (((bigtime_t)(stream[2] & 0xfe)) << 15) |
		 (((bigtime_t)(stream[3] & 0xff)) << 8)  |
		 (((bigtime_t)(stream[4] & 0xfe)) << 0));
	
#if 0
		(((bigtime_t)((stream[0] & 0xf) >> 1) << 29) |
		((bigtime_t)stream[1] << 22) |
		((bigtime_t)(stream[2] >> 1) << 15) |
		((bigtime_t)stream[3] << 7) |
		((bigtime_t)stream[4] >> 1))
		* (bigtime_t) 1000 / (bigtime_t) 90;
#endif

	return B_OK;
}

static int GetXingHeader(XHEADDATA *X,  unsigned char *buf, int len);

status_t
MPEGSystemExtractor::build_audio_format_info(unsigned char *hdr, int len)
{
	BAutolock autolock(&locker);

	int id, layer, protection, bit_rate, sample_rate, padding, mode,
		mode_extension, copyright, original, emphasis;
	int index;
	
	len = len;

	if (hdr[0] != 0xff || (hdr[1] & 0xe0) != 0xe0)
		return B_BAD_VALUE;

	id = (hdr[1] & 0x08) >> 3;

	switch (hdr[1] & 0x6) {
	case 6:
		layer = 1;
		break;
	case 4:
		layer = 2;
		break;
	case 2:
		layer = 3;
		break;
	case 0:
	default:
		return B_BAD_VALUE;
		break;
	}

	protection = (hdr[1] & 1);

	index = (hdr[2] & 0xf0) >> 4;
	if (index == 15)
		return B_BAD_VALUE;

	if (id == 1) {    /* 11172 format tables */
		if (layer == 1)
			bit_rate = layer1_bit_rate_11172[index];
		else if (layer == 2)
			bit_rate = layer2_bit_rate_11172[index];
		else /*if (layer == 3)*/
			bit_rate = layer3_bit_rate_11172[index];
	} else {
		if (layer == 1)
			bit_rate = layer1_bit_rate_13818[index];
		else /*if (layer == 2 || layer == 3)*/
			bit_rate = layer2_bit_rate_13818[index];
	}


	index = (hdr[2] & 0x0c) >> 2;
	
	switch (index) {
	case 0:
		sample_rate = (id == 0) ? 22050 : 44100;
		break;
	case 1:
		sample_rate = (id == 0) ? 24000 : 48000;
		break;
	case 2:
		sample_rate = (id == 0) ? 16000 : 32000;
		break;
	default:
		return B_BAD_VALUE;
	}
	
	if(!(hdr[1] & 0x10))
		sample_rate/=2;
	
	padding        = (hdr[2] & 0x02);
	mode           = (hdr[3] & 0xc0) >> 6;
	mode_extension = (hdr[3] & 0x20) >> 4;
	copyright      = (hdr[3] & 0x08);
	original       = (hdr[3] & 0x04);
	emphasis       = (hdr[3] & 0x03);

	// now that we're reasonably sure that this is in fact an mpeg audio stream,
	// let's see what Xing has to say about it
	XHEADDATA xhead;
	if(1==GetXingHeader(&xhead,hdr,len))
	{
		// found Xing header info
		fXingHeader=new XHEADDATA(xhead);
		//printf("Xing header info:\n frames: %d\n bytes: %d\n",fXingHeader->frames,fXingHeader->bytes);
	}

	// after parsing everything out of the header we now build a media_format
	fAudioFormat.type = B_MEDIA_ENCODED_AUDIO;
	if (layer == 1)
		fAudioFormat.u.encoded_audio.encoding = mpeg1_audio1;
	else if (layer == 2)
		fAudioFormat.u.encoded_audio.encoding = mpeg1_audio2;
	else if (layer == 3)
		fAudioFormat.u.encoded_audio.encoding = mpeg1_audio3;

	fAudioFormat.u.encoded_audio.bit_rate = bit_rate * 1000.0;
	fAudioFormat.u.encoded_audio.output.frame_rate = sample_rate;

	if (mode < 3)  // XXXdbg - this means dual channel mode == stereo
		fAudioFormat.u.encoded_audio.output.channel_count = 2;
	else 
		fAudioFormat.u.encoded_audio.output.channel_count = 1;

	fAudioFormat.u.encoded_audio.output.buffer_size = 4096;
	if(fXingHeader)
		fAudioDuration = (bigtime_t)((fXingHeader->frames * 1152 / sample_rate) * 1000000.0);

	fHaveAudio = true;
	
	if (layer == 1) {
		fAudioFrameSize = (12 * 1000 * bit_rate) / (float)sample_rate;
	} else {
		if (layer == 3 && id == 0) {
			fAudioFrameSize = (72 * 1000 * bit_rate) / (float)sample_rate;
		} else {
			fAudioFrameSize = (144 * 1000 * bit_rate) / (float)sample_rate;
		}
	} 
	
	return B_OK;
}

status_t
MPEGSystemExtractor::build_video_format_info(unsigned char *header, int len)
{
	BAutolock autolock(&locker);

	uchar *videoPacketStart = NULL;
	
	for (int32 i=0; i < (len - HEADER_SIZE); i++) {
		if ((header[i+0] == 0) && (header[i+1] == 0) &&
			(header[i+2] == 1) && (header[i+3] == 0xb3)) {
			videoPacketStart = header + i;
			break;
		}
	}

	if (!videoPacketStart) {
		printf("mpeg1: Unable to find video header\n");
		return B_BAD_VALUE;
	}

	fVideoFormat.type = B_MEDIA_ENCODED_VIDEO;
	fVideoFormat.u.encoded_video.encoding = mpeg1_video;
	fVideoFormat.u.encoded_video.output.display.line_width = 
		(((uint32)videoPacketStart[4]) << 4) +
		(videoPacketStart[5] >> 4);
	fVideoFormat.u.encoded_video.output.display.line_count = 
		(((uint32)videoPacketStart[5] & 0x0f) << 8) +
		videoPacketStart[6];
	
	fVideoFormat.u.encoded_video.output.interlace = 1;
	fVideoFormat.u.encoded_video.output.first_active = 0;
	fVideoFormat.u.encoded_video.output.last_active =
		fVideoFormat.u.encoded_video.output.display.line_count - 1;
	fVideoFormat.u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	fVideoFormat.u.encoded_video.output.pixel_width_aspect = 1;
	fVideoFormat.u.encoded_video.output.pixel_height_aspect = 1;
	

	DEBUG("MPEGSystemExtractor: video size is %ld x %ld\n",
		  fVideoFormat.u.encoded_video.output.display.line_width,
		  fVideoFormat.u.encoded_video.output.display.line_count);

	unsigned long prIndex = videoPacketStart[7] & 0x0f;
	if (!prIndex ||
		(prIndex >= sizeof(pictureRateCode) / sizeof(pictureRateCode[0]))) {
		DEBUG("Bad picture rate (%lx)\n", prIndex);
		return B_BAD_TYPE;
	}
	
	fVideoFormat.u.encoded_video.output.field_rate = pictureRateCode[prIndex];

	fFrameRate = pictureRateCode[prIndex];
	fHaveVideo   = true;

	return B_OK;
}


// yet more code that knows how to parse sequence headers
static int32
sequence_header_size(const uchar *buffer, int32 length)
{
	int load_intra_quantizer_matrix;
	int load_non_intra_quantizer_matrix;
	int32 size;

	// sequence_start_code (00 00 01 b3)
	// 12 + 4 since we need to look for another
	// start code following the main header
	if(buffer == NULL || length < 12 + 4 ||
		buffer[0] != 0x00 ||
		buffer[1] != 0x00 ||
		buffer[2] != 0x01 ||
		buffer[3] != 0xb3)
	{
		return 0;
	}

	// sequence header size at least 12
	size = 12;

	// check load_[non_]intra_quantizer_matrix
	load_intra_quantizer_matrix = buffer[11] & 2;
	if(load_intra_quantizer_matrix) {
		// add size for intra_quantizer_matrix
		size += 64;
		if(size > length) {
			return 0;
		}
		load_non_intra_quantizer_matrix = buffer[64+11] & 1;
	}
	else {
		load_non_intra_quantizer_matrix = buffer[11] & 1;
	}
	if(load_non_intra_quantizer_matrix) {
		size += 64;
		if(size > length) {
			return 0;
		}
	}

	// if we've read the matrices correctly, the next three
	// bytes should be a start code (00 00 01).  We have to
	// look for the optional extension blocks, so we'll need
	// at least 4 more bytes to do so.

	if(size+4 > length) {
		return 0;
	}
	if(buffer[size] != 0x00 || buffer[size+1] != 0x00 || buffer[size+2] != 0x01) {
		printf("\n!!!!!!!!!!!!SEQUENCE HEADER PROBLEMS!!!!!!!!!!!!\n");
		printf("%2x %2x [ %2x %2x %2x ] %2x %2x\n",
			buffer[size-2], buffer[size-1],
			buffer[size], buffer[size+1], buffer[size+2],
			buffer[size+3], buffer[size+4]);
		// use what we've got
		return size;
	}

	// look for extension_start_code (00 00 01 b5).  If found,
	// we need to calculate its length since it's technically
	// part of the seqence header.

	if(buffer[size+3] == 0xb5) {
		// found one.  It's terminated by another start code (00 00 01)
		// so we need to search through the stream for one.  We'll need
		// to look for a user_data_start_code (4 bytes) after this,
		// so make sure enough data is left.

		// size of extension_start_code
		size += 4;

		// eat up an integral number of bytes with 4 bytes left over
		if(size+4 > length) {
			return 0;
		}
		while(buffer[size+0] != 0x00 || buffer[size+1] != 0x00 || buffer[size+2] != 0x01) {
			size++;
			if(size+4 > length) {
				return 0;
			}
		}
	}

	// ditto for user_data_start_code (00 00 01 b2)

	if(buffer[size+3] == 0xb2) {
		// this time we only need to look for any start code (00 00 01, 3 bytes), not
		// for a specific one (4 bytes)

		// size of user_data_start_code
		size += 4;

		// eat up an integral number of bytes with 3 bytes left over
		if(size+3 > length) {
			return 0;
		}
		while(buffer[size+0] != 0x00 || buffer[size+1] != 0x00 || buffer[size+2] != 0x01) {
			size++;
			if(size+3 > length) {
				return 0;
			}
		}
	}
	
	// all good.

	return size;
}

ssize_t
MPEGSystemExtractor::ParsePack(uchar *buffer, int32 length, off_t off,
							   uchar stream_id, struct MPEG_info *info)
{
	int32 i = 0, size = 0, err;
	
	// Only set the stream_id if it isn't already set. Some streams contain data
	// for additional audio/video streams at the end of the file, which would prevent
	// files from playing if we didn't do this check.
	if(info->stream_mapping[StreamType(stream_id)] == 0)
		info->stream_mapping[StreamType(stream_id)] = stream_id;

	if(StreamType(stream_id) != kVideoStream) {
		// we don't do any parsing of non-video packs.
		return length;
	}
	
	while (i <= length - HEADER_SIZE) {
		// if we find a header, we assume the data
		// associated with it fits inside this pack
		int32 pack_len = length - i;

		if (	(buffer[i+0] != 0) || (buffer[i+1] != 0) ||
				(buffer[i+2] != 1)) {
			i++;
			continue;
		}

		if (fHaveVideo == false && buffer[i+3] == 0xb3) {

			// sequence_start_code (00 00 01 b3)
			// this has info like frame size, frame rate

			err = build_video_format_info(&buffer[i], length - i);
			if (err != B_OK)
				return err;
		}

		if ((buffer[i+3] == 0xb3) &&
			(buffer[i+10] & 0x20))
		{
			// sequence_start_code (00 00 01 b3)
			// this is important for doing seeks, so we
			// need to know exactly how long it is
			double fps;

			size = sequence_header_size(buffer+i, length-i);
			if(size <= 0) {
				return i;
			}

			// found an entire sequence block, 'size' bytes long
			// pillage it for the basic info we need

			fps = pictureRateCode[buffer[i+7] & 15];
			if (fps > 0.0f)
				info->streams[stream_id - STREAM_ID_BASE].fps = fps;
			
			pack_len = size;
		}
		else if ((buffer[i+3] == 0xb8) && (buffer[i+5] & 8)) {
			// group_start_code (00 00 01 b8)
#if 0
			// MPEG time stamps are unreliable, to put it mildly.
			// keep this around because they'll be somewhat useful
			// when we do guess-seeking
			uint32 h, m, s, f;

			h =  (buffer[i+4] & 0x7c) >> 2;
			m = ((buffer[i+4] & 0x03) << 4) | ((buffer[i+5] & 0xf0) >> 4);
			s = ((buffer[i+5] & 0x07) << 3) | ((buffer[i+6] & 0xe0) >> 5);
			f = ((buffer[i+6] & 0x1f) << 1) | ((buffer[i+7] & 0x80) >> 7);
printf("%ld:%ld:%ld:%ld/%g ", h, m, s, f, info->streams[stream_id - STREAM_ID_BASE].fps);
			info->streams[stream_id - STREAM_ID_BASE].temp.t = 
					((((h * 60) + m) * 60) + s) * 1000000LL +
					(int64)(1000000LL * f / info->streams[stream_id - STREAM_ID_BASE].fps);
			DEBUG("Group start code (%Lx): %ld:%2.2ld:%2.2ld.%2.2ld\n",
				  off + i, h, m, s, f);
#else
			if(info->streams[stream_id - STREAM_ID_BASE].fps < 0.1) {
//				printf("mpeg1: uninitialized stream 0x%x\n", stream_id);
				// skip this block.. sometimes a single audio stream group_start_code
				// will be in a file, confusing everyone later on.
				i += 7;	// min group size is 7.375 bytes
				continue;
			}
			info->streams[stream_id - STREAM_ID_BASE].temp.t = 
					(bigtime_t)(1e6 * info->streams[stream_id - STREAM_ID_BASE].f / info->streams[stream_id - STREAM_ID_BASE].fps);
#endif
			size = 7;	// min group size is 7.375 bytes
		} else if (buffer[i+3] == 0) {
			DEBUG("Picture start (%Lx) %d\n", off + i, (buffer[i+5] >> 3) & 7);

			// picture_start_code (00 00 01 00)
			// increment the frame count and keep looking

			++info->streams[stream_id - STREAM_ID_BASE].f;
			i += 7;	// actual min size is 7.75 bytes
			continue;
		} else {
			if (	(buffer[i+3] >= 0xb0) && (buffer[i+3] != 0xb2) &&
					(buffer[i+3] != 0xb5))
			{
				DEBUG("Unknown code (%x) @ %Lx\n", buffer[i+3], off + i);
			}
			i += 3;
			continue;
		}

		struct layer_header *h;
		
		h = (struct layer_header *)malloc(sizeof(*h));
		if (!h) {
//			printf("Out of core\n");
			return ENOMEM;
		}

		h->header_type = buffer[i+3];
		h->stream_id = stream_id;
		h->offset = off + i;
		h->len = pack_len;
		h->t = info->streams[stream_id - STREAM_ID_BASE].temp.t;
		h->next = NULL;

		h->f = info->streams[stream_id - STREAM_ID_BASE].f;
		if(h->header_type == 0xb3) {
			// this is the most recent sequence for this stream
			info->streams[stream_id - STREAM_ID_BASE].temp.seq = h;
		}
		h->sequence = info->streams[stream_id - STREAM_ID_BASE].temp.seq;

//printf("%x: %2x: %Ld+%4ld: %Ld  %Ld\n", h->stream_id, h->header_type, h->offset, h->len, h->t, h->f);

		if (info->streams[stream_id - STREAM_ID_BASE].h) {
			info->streams[stream_id - STREAM_ID_BASE].h_tail->next = h;
			info->streams[stream_id - STREAM_ID_BASE].h_tail = h;
		} else {
			info->streams[stream_id - STREAM_ID_BASE].h =
					info->streams[stream_id - STREAM_ID_BASE].h_tail = h;
		}

		i += size;
	}

	return i;
}

/*
FindPacks does all the work parsing the MPEG file. It has to demultiplex the
streams as well as keep track of key positions within the streams. Key points
in streams are: sequence headers, group start.
 */
ssize_t
MPEGSystemExtractor::FindPacks(	uchar		*buffer,
								int32		length,
								off_t		off,
								uchar		*stream_id,
								off_t		*stream_remaining,
								struct MPEG_info *info)
{
	int32 i = 0, size;
	
	while (i <= length - HEADER_SIZE) {

		// stream_remaining is the amount of data in
		// this pack, as established by a system
		// header (below) or passed in for simple files with
		// only one stream

		if (*stream_remaining > 0) {
			ssize_t ret;
			int32 len;
			if (*stream_remaining > length - i) {
				len = length - i;
			}
			else
				len = *stream_remaining;

			// we know from the system header that the next
			// stream_remaining bytes of the stream belong to
			// stream stream_id;  ParsePack() builds some
			// data structures based on this data.  We may
			// end up missing some information if it's too
			// close to the end of the pack, since there's
			// no buffering done.  This could be solved with
			// a stacked extractor (mpegvideo over mpegsystem
			// or something)

			ret = ParsePack(buffer + i, len, off + i, *stream_id, info);
			if (ret < 0)
				return ret;
			*stream_remaining -= len;
			i += len;
			continue;
		}

		if (	(buffer[i+0] != 0) || (buffer[i+1] != 0) ||
				(buffer[i+2] != 1)) {
			// this is a good place to find extractor bugs.
			// if we were parsing the stream correctly,
			// we should never need to search for a start code.
			i++;
			continue;
		}
		
		if (buffer[i+3] == 0xba) {
			bigtime_t t;

			if (	((buffer[i+4] & 0xf0) != 0x20) ||
					((buffer[i+11] & 1) != 1) ||
					(DecodeTime(buffer + i + 4, &t) != B_OK)) {
				DEBUG("FALSE PACK START @ %Lx\n", off + i);
				i += 4;
				continue;
			}

			DEBUG("Pack start (t == %Ld)\n", t);
			size = 12;

		} else if (buffer[i+3] == 0xbb) {
			if (	((buffer[i+6] & 0x80) != 0x80) ||
					((buffer[i+8] & 1) != 1) ||
					((buffer[i+10] & 0x20) != 0x20) ||
					(buffer[i+11] != 0xff)) {
				DEBUG("FALSE SYSTEM HEADER\n");
				i += 4;
				continue;
			}
			
			DEBUG("System header\n");
			size = buffer[i+4] * 0x100 + buffer[i+5] + 6;

		} else if (buffer[i+3] >= 0xb8) {
			size = buffer[i+4] * 0x100 + buffer[i+5] + 6;

			// Scan video and audio packs only			
			if (	(buffer[i+3] == 0xb8) || (buffer[i+3] == 0xb9) ||
					((buffer[i+3] & 0xe0) == 0xc0) ||
					((buffer[i+3] & 0xf0) == 0xe0)) {

				int32 data_offset;
				uchar nonpts_nondts = true;

				data_offset = 6;
				
				if (i + data_offset + 1 > length)
					return i;

				// stuffing bytes
				// need one byte to identify
				while (buffer[i+data_offset] == 0xff) {
					data_offset++;

					if (i + data_offset + 1 > length)
						return i;
				}

				// STD
				// need one byte to identify
				if ((buffer[i+data_offset] & 0xc0) == 0x40) {
					data_offset += 2;
				}

				if (i + data_offset + 5 > length)
					return i;

				// PTS
				// need 5 bytes to identify
				if (((buffer[i+data_offset] & 0xf1) == 0x21) &&
					((buffer[i+data_offset+2] & 1) == 1) &&
					((buffer[i+data_offset+4] & 1) == 1)) {

					/*
					int64 val;
					val = (((buffer[i+data_offset+0] & 0x0e) << 30) |
						   ((buffer[i+data_offset+1] & 0xff) << 22) |
						   ((buffer[i+data_offset+2] & 0xfe) << 15) |
						   ((buffer[i+data_offset+3] & 0xff) << 8)  |
						   ((buffer[i+data_offset+4] & 0xfe) << 0));
					*/
					
					data_offset += 5;
					nonpts_nondts = false;
				}

				if (i + data_offset + 10 > length)
					return i;

				// DTS
				// need 10 bytes to identify
				if (((buffer[i+data_offset] & 0xf1) == 0x31) &&
					((buffer[i+data_offset+2] & 1) == 1) &&
					((buffer[i+data_offset+4] & 1) == 1) &&
					((buffer[i+data_offset+5] & 0xf1) == 0x11) &&
					((buffer[i+data_offset+7] & 1) == 1) &&
					((buffer[i+data_offset+9] & 1) == 1)) {

					/*
					int64 val;
					val = (((buffer[i+data_offset+0] & 0x0e) << 30) |
						   ((buffer[i+data_offset+1] & 0xff) << 22) |
						   ((buffer[i+data_offset+2] & 0xfe) << 15) |
						   ((buffer[i+data_offset+3] & 0xff) << 8)  |
						   ((buffer[i+data_offset+4] & 0xfe) << 0));
					*/

					data_offset += 10;
					nonpts_nondts = false;

				}
				
				if (i + data_offset > length)
					return i;

				// nonPTS_nonDTS_bits
				// need one byte to identify
				if (nonpts_nondts) {
					if (buffer[i+data_offset] == 0xf)
						data_offset++;
					else
						DEBUG("NONPTS NONDTS NOT FOUND\n");
				}

				if ((buffer[i+3] & 0xe0) == 0xc0) {
					if (fHaveAudio == false) {
						if (build_audio_format_info(&buffer[i+data_offset],
													length-i-data_offset) != B_OK)
							printf("mpeg1: failed to sniff the audio format...\n");
					}

					fActualAudioSizeSoFar += size - data_offset;		// this much we're sure we have
					if(off)
						fAudioSize=int64(fFileSize)*int64(fActualAudioSizeSoFar)/int64(off);	// this we're extrapolating from the above
				//	printf("fAudioSize now estimated at %d (%d/%d)*%d\n",
				//		int(fAudioSize),
				//		int(fFileSize),
				//		int(off),
				//		int(fActualAudioSizeSoFar));
				}


				*stream_id = buffer[i+3];
				*stream_remaining = size - data_offset;
				
				DEBUG("PACK %x at %Lx %Lx (%Lx %Lx)\n",
						*stream_id, off + i + data_offset, *stream_remaining,
						off + i, off + i + size);

				struct layer *l;
				l = (struct layer *)malloc(sizeof(*l));
				if (!l)
					return ENOMEM;
				l->offset = off + i + data_offset;
				l->len = *stream_remaining;
//printf("len for %d: %d - %d = %d\n",*stream_id,size,data_offset,l->len);
				l->next = NULL;
				if (info->streams[*stream_id - STREAM_ID_BASE].l) {
					info->streams[*stream_id - STREAM_ID_BASE].l_tail->next = l;
					info->streams[*stream_id - STREAM_ID_BASE].l_tail = l;
				} else {
					info->streams[*stream_id - STREAM_ID_BASE].l =
							info->streams[*stream_id - STREAM_ID_BASE].l_tail = l;
				}

				i += data_offset;
				continue;
			}
		} else {
			DEBUG("Unknown type: %x @ %Lx\n", buffer[i+3], off + i);
			size = 3;
		}

		i += size;
	}

	return i;
}

// ID3v2 detection
// see specification on http://www.id3.org
int32 ID3Size(unsigned char *start)
{
	if(	(start[0]=='I') &&
		(start[1]=='D') &&
		(start[2]=='3') &&
		(start[3]<0xff) &&
		(start[4]<0xff) &&
		/* no constraints on flags field */
		(start[6]<0x80) &&
		(start[7]<0x80) &&
		(start[8]<0x80) &&
		(start[9]<0x80))
	{
		return 10+((start[6]<<21)|(start[7]<<14)|(start[8]<<7)|start[9]);
	}
	return 0;
}		

status_t
MPEGSystemExtractor::GetFormatInfo()
{
	off_t		pos;
	int32		start;
	uchar 		stream_id;
	uchar		header[4096];

	DEBUG("Getting Format Info\n");
	
	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	if (pio == NULL)
		return B_IO_ERROR;

	if ((pos = pio->Seek(0, SEEK_END)) <= 0)
		fFileSize = 0;
	else
		fFileSize = pos;

	ssize_t headerSize;
	if ((headerSize = pio->ReadAt(0LL, &header[0], sizeof(header))) < 4) {
		printf("mpeg1: Error reading file header\n");
		return (headerSize >= 0) ? EINVAL : headerSize;
	}

	stream_id = 0;

	int id3length=ID3Size(header);

	if(id3length>0)
	{
		// read data after ID3v2 tag
//		printf("position: %Ld\n",pio->Position());
		headerSize = pio->ReadAt(id3length, header, sizeof(header));
		if(headerSize < 4)
			return (headerSize >= 0) ? EINVAL : headerSize;
	}

	
	start = find_mpeg_header(&header[0], headerSize);
	if (start < 0)
		return B_BAD_TYPE;
	

	if (	(header[start+0] == 0) && (header[start+1] == 0) &&
			(header[start+2] == 1) && (header[start+3] == 0xb3)) {
		fFileType  = kVideoStreamOnly;
		fHaveAudio = false;

		stream_id = 0xb9;
	} else if (is_mpeg_audio_header(&header[start], headerSize - start, false)) {
		fFileType  = kAudioStreamOnly;
		fHaveVideo = false;
		fHaveAudio = true;
		fAudioStartOffset = start + id3length;
		fAudioSize = fFileSize - fAudioStartOffset;
		if (fAudioSize < 0)
			fAudioSize = 0;

//		printf("audio start offset = %Ld, audio size = %Ld\n", fAudioStartOffset, fAudioSize);

		// a bit ugly, but it allows us to make an EXP based "devkit" that will
		// play mp3s from within Wagner, while still properly skipping the id3
		// tag in other cases.
		app_info ai;
		if(B_OK == be_app->GetAppInfo(&ai) &&
			0 != strcmp("application/x-vnd.Web",ai.signature))
		{
			/* look for ID3 v1 tag */
			uchar buff[128];
			if (fAudioSize > 128 && pio->ReadAt(fFileSize - 128, buff, 128) == 128) {
				if (buff[0] == 'T' && buff[1] == 'A' && buff[2] == 'G') {
					fAudioSize -= 128;
					buff[127] = '\0';
					DEBUG("IDT3v1 info: %s\n", buff+3);
				}
			}
		}
		
		/* no need to parse the whole stream for audio */
		build_audio_format_info(&header[start], headerSize - start);
	} else {
		fFileType = kSystemStream;     
		/* it's a system stream but we don't know what's in it yet */
	}

	if(fFileType != kAudioStreamOnly) {
		// start scanning the entire file for entrypoints in the background
		fScanThread=spawn_thread(backgroundscanner,"mpegscanner",B_NORMAL_PRIORITY,this);
		fDataStart=start;
		fStreamID=stream_id;
		resume_thread(fScanThread);
		while(fActualAudioSizeSoFar < 200000 && fScanThread != -1)
			snooze(20000); // give the background scanner time to do an initial estimate of the size
	}

	if(fFileType != kVideoStreamOnly) {
		// for system streams, fAudioSize is estimated while the stream is still being scanned in the background

		if(fAudioDuration == 0) {
			fAudioDuration = (bigtime_t)(((double)fAudioSize /
				(fAudioFormat.u.encoded_audio.bit_rate / 8.0)) * 1000000.0);
		}
		fAudioNumFrames = (int64)((fAudioDuration / 1000000.0) *
			fAudioFormat.u.encoded_audio.output.frame_rate);
		//printf("guestimated duration: %Ld / %Ld (%f)\n",fAudioDuration,fAudioNumFrames,fAudioFormat.u.encoded_audio.output.frame_rate);
	}

	return B_OK;
}


long MPEGSystemExtractor::backgroundscanner(void *self)
{
	return ((MPEGSystemExtractor*)self)->BackgroundScanner();
}

long MPEGSystemExtractor::BackgroundScanner()
{
	off_t pos = fDataStart;
	int32 len;
	off_t stream_remaining = 0LL;
	uchar stream_id=fStreamID;
	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	while (fScanThread != -1 && (len = pio->ReadAt(pos, fSeekBuf, SEEK_CHUNK_SIZE)) >= HEADER_SIZE){
		ssize_t ret;
		if (fFileType == kVideoStreamOnly) {
			struct layer *l;
			l = (struct layer *)malloc(sizeof(*l));
			if (!l)
				break;
			l->offset = pos;
			l->len = len;
			l->next = NULL;
			if (mpeg.streams[stream_id - STREAM_ID_BASE].l) {
				mpeg.streams[stream_id - STREAM_ID_BASE].l_tail->next = l;
				mpeg.streams[stream_id - STREAM_ID_BASE].l_tail = l;
			} else {
				mpeg.streams[stream_id - STREAM_ID_BASE].l =
					mpeg.streams[stream_id - STREAM_ID_BASE].l_tail = l;
			}
			stream_remaining = len;
		}
		ret = FindPacks(fSeekBuf, len, pos, &stream_id, &stream_remaining, &mpeg);
		if (ret > 0)
			pos += ret;
		else
			break;
	}
	fAudioSize=fActualAudioSizeSoFar;
	fScanThread=-1;
	//printf("background scan done\n");
	return B_OK; // not necessarily true
}


status_t MPEGSystemExtractor::Sniff(int32 *out_streamNum, int32 *out_chunkSize)
{

	int pos;
	unsigned char start[4096];
	ssize_t read_size;

	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	if (pio == NULL)
		return B_IO_ERROR;

	read_size = pio->ReadAt(0LL, start, sizeof(start));
	if(read_size < 4) {
		if(read_size >= 0)
			return B_BAD_TYPE;
		return read_size;
	}

	/* test for some other formats until mp3 detection is better */
	if(*((uint32*)start) == B_HOST_TO_BENDIAN_INT32('RIFF'))
		return B_BAD_TYPE;
	if(*((uint32*)start+1) == B_HOST_TO_BENDIAN_INT32('moov'))
		return B_BAD_TYPE;
	if(*((uint32*)start+1) == B_HOST_TO_BENDIAN_INT32('mdat'))
		return B_BAD_TYPE;
	if(*((uint32*)(start+2)) == B_HOST_TO_BENDIAN_INT32('-lh5'))
		return B_BAD_TYPE;

	int id3length=ID3Size(start);
	if(id3length>0)
	{
		read_size = pio->ReadAt(id3length, start, sizeof(start));
		if(read_size < 4)
		{
			if(read_size >= 0)
				return B_BAD_TYPE;
			return read_size;
		}
	}

	pos = find_mpeg_header(&start[0], read_size);

	if (pos < 0)
		return B_BAD_TYPE;
	
	if (start[pos+0] == 0 && start[pos+1] == 0 && start[pos+2] == 1) {
		if (start[pos+3] == 0xb3)
			*out_streamNum = 1;
		else if (start[pos+3] == 0xba)  
			*out_streamNum = 2;  // XXXdbg assume system streams have 2 tracks
		else
			return B_BAD_TYPE;
	} else if (is_mpeg_audio_header(&start[pos], sizeof(start) - pos, false)) {
		*out_streamNum = 1;
//		fAudioStartOffset = pos;
	}

	*out_chunkSize = DEFAULT_CHUNK_SIZE;

	return B_OK;
}


status_t 
MPEGSystemExtractor::TrackInfo(int32 in_stream, media_format *out_format,
	void **out_info, int32 *out_size)
{
	status_t					err;

	// shut up the compiler
	out_info = out_info;
	out_size = out_size;

	if (fSeekBuf == NULL)
		return B_NO_MEMORY;

	// if we have parsed anything yet, go do it 
	if (!fHaveVideo && !fHaveAudio) {
		err = GetFormatInfo();
		if (err != B_OK)
			return err;
	}

	DEBUG("TrackInfo (in_stream %ld ## audio %d video %d)\n", in_stream,
		   fHaveAudio, fHaveVideo);

	if (fHaveVideo) {
		if (in_stream == kVideoStream)
			*out_format = fVideoFormat;
		else if (in_stream == kAudioStream && fHaveAudio) {
			*out_format = fAudioFormat;
		} else {
			return B_BAD_INDEX;
		}
	} else if (fHaveAudio && in_stream == 0) {
			*out_format = fAudioFormat;
	} else {
		return B_BAD_INDEX;
	}

	return B_OK;
}

// Xing's seekpoint-calculation
static int SeekPoint(unsigned char TOC[100], int file_bytes, float percent);

//  seeks to the closest position in the file to the time specified by tpos
status_t 
MPEGSystemExtractor::mpeg_audio_seek(bigtime_t tpos, off_t *new_fpos, bool seekBack) const
{
	int32 i;
	off_t pos;

//	printf("mpeg_audio_seek\n");

	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	if (pio == NULL)
		return B_IO_ERROR;

	if(fAudioDuration == 0)
		if(tpos == 0) {
			*new_fpos = fAudioStartOffset;
			return B_OK;
		}
		else
			return B_DEV_SEEK_ERROR;

	// calculate estimate file-offset for the given time-offset
	if(fXingHeader && (fXingHeader->flags&XING_TOC_FLAG))
	{
		pos=SeekPoint(fXingHeader->toc,fXingHeader->bytes,double(tpos*100)/(double)fAudioDuration);
	}
	else
	{
		// note: we also end up here if it's a VBR file which doesn't contain a TOC
		pos = (off_t)(((double)tpos / (double)fAudioDuration) * (double) fAudioSize);
	}
	//printf("Seek offset: %d (%d%%)\n",int(pos),int(double(tpos*100)/(double)fAudioDuration));

	if (fAudioFrameSize > 0) {
		if (seekBack) {
			pos = (off_t)(floor((float)pos / fAudioFrameSize) * fAudioFrameSize);
			pos += 4;
		} else {
			pos = (off_t)(ceil((float)pos / fAudioFrameSize) * fAudioFrameSize);
			pos -= 4;
		}
	}
	i = SEEK_CHUNK_SIZE / 2;
	pos = pos - i + fAudioStartOffset;
	size_t read_size = SEEK_CHUNK_SIZE;
	//printf("i = %d, pos = %Ld, end = %Ld\n", i, pos, fAudioStartOffset + fAudioSize);
	if (pos < fAudioStartOffset) {
		i -= fAudioStartOffset - pos;
		pos = fAudioStartOffset;
	} else if (pos > fAudioStartOffset + fAudioSize - SEEK_CHUNK_SIZE) {
		if (!seekBack) {
			return B_ERROR;
		}
		if(fAudioStartOffset + fAudioSize - SEEK_CHUNK_SIZE < 0) {
			read_size = fAudioSize;
		}
		else {
			i += pos - (fAudioStartOffset + fAudioSize - SEEK_CHUNK_SIZE);
			pos = fAudioStartOffset + fAudioSize - SEEK_CHUNK_SIZE;
		}
	}
		
	ssize_t sizeRead = pio->ReadAt(pos, fSeekBuf, read_size);
	if (sizeRead < 4) {
//		printf("short read - %ld @ %lld\n", sizeRead, pos);
		if (sizeRead < 0) {
			return sizeRead;
		} else {
			return B_ERROR;
		}
	}
	
	if (i < 0) {
		i = 0;
	} else if (i >= sizeRead - 4) {
		i = sizeRead - 5;
	}
	//printf("got i = %d, pos = %Ld, size = %d\n", i, pos, sizeRead);

	int start_i = i;
	while ((i >= 0) && (i < sizeRead - 4)) {
		if (is_mpeg_audio_header(&fSeekBuf[i], sizeRead - i, fAudioFrameSize > 0)) {
			*new_fpos = pos + i;
			//printf("MPEGSystemExtractor:: seeking to %lld, found audio header @ %lld, [%d]\n", pos + start_i, *new_fpos, i - start_i);
			return B_OK;
		}
		if (seekBack) {
			i--;
		} else {
			i++;
		}
	}

	printf("MPEGSystemExtractor: no header in %lld - %lld\n", pos + start_i, pos + i);

	return B_ERROR;
}


status_t 
MPEGSystemExtractor::AllocateCookie(int32 /*in_stream*/, void **cookieptr)
{
	*cookieptr = calloc(sizeof(off_t), 1);
	if(*cookieptr == 0)
		return B_NO_MEMORY;
	else
		return B_NO_ERROR;
}

status_t 
MPEGSystemExtractor::FreeCookie(int32 /*in_stream*/, void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

#if GUESS_SEEK
int64 MPEGSystemExtractor::SeekForwardFrom(int64 where, uint8 type, uint8 mask, int32 *packetlength)
{
	int64 data_offset=6;
	uchar buffer[100];
	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	int32 numread;
	while(1)
	{
		numread=pio->ReadAt(where,&buffer[0],100);
		if(numread!=100)
			return B_BAD_INDEX; // sorry, we're out of options here
		if(
			 buffer[0]			== 0x00 &&
			 buffer[1]			== 0x00 &&
			 buffer[2] 			== 0x01 &&
			(buffer[3] & mask)	== type )
		{
			// parse header until we find the actual packet data
			// this is largely copied from the FindPacks() function

			// stuffing bytes
			// need one byte to identify
			while (buffer[data_offset] == 0xff) {
				data_offset++;
				if (data_offset  > 16+6)
					return -1;  // only 16 stuffing bytes allowed
			}
printf("dataoffset is %d after stuffing\n",data_offset);
			// STD
			// need one byte to identify
			if ((buffer[data_offset] & 0xc0) == 0x40) {
				data_offset += 2;
			}
printf("dataoffset is %d after STD\n",data_offset);

			// PTS
			// need 5 bytes to identify
			bool nonpts_nondts=true;
			if (((buffer[data_offset] & 0xf1) == 0x21) &&
				((buffer[data_offset+2] & 1) == 1) &&
				((buffer[data_offset+4] & 1) == 1))
			{
				data_offset += 5;
				nonpts_nondts = false;
			}
printf("dataoffset is %d after PTS\n",data_offset);

			// DTS
			// need 10 bytes to identify
			if (((buffer[data_offset] & 0xf1) == 0x31) &&
				((buffer[data_offset+2] & 1) == 1) &&
				((buffer[data_offset+4] & 1) == 1) &&
				((buffer[data_offset+5] & 0xf1) == 0x11) &&
				((buffer[data_offset+7] & 1) == 1) &&
				((buffer[data_offset+9] & 1) == 1))
			{	
				data_offset += 10;
				nonpts_nondts = false;
			}
printf("dataoffset is %d after DTS\n",data_offset);
		
			// nonPTS_nonDTS_bits
			// need one byte to identify
			if (nonpts_nondts) {
				if (buffer[data_offset] == 0xf)
					data_offset++;
				else
					DEBUG("NONPTS NONDTS NOT FOUND\n");
			}
			*packetlength = (int32(buffer[4])<<8) + buffer[5] - data_offset + 6;
printf("packetlength: %d, dataoffset %d\n",*packetlength,data_offset);
			return where+data_offset;
		}
		for(int i=1;i<100;i++)
		{
			where++;
			if(buffer[i]==0)
				break;
		}
	}
}
#endif


status_t 
MPEGSystemExtractor::Seek(int32 in_stream, void *cookie, int32 to_what,
                          int32 flags, bigtime_t *inout_time,
                          int64 *inout_frame, off_t *inout_filePos,
                          char *in_packetPointer, int32 *inout_packetLength,
                          bool *out_done)
{
	struct layer_header *c;
	struct layer_header *group, *prev_group, *next_group;
	off_t        *curpos = (off_t *)cookie;

	// shut up the compiler 
	bool seek_forward =
		flags & B_MEDIA_SEEK_CLOSEST_FORWARD;
	in_packetPointer = in_packetPointer;
	
	if (in_stream < 0 || in_stream >= 2)
		return B_BAD_INDEX;


//SEEK(("%ld: mpeg1 seek %s to ", in_stream, seek_forward ? "fwd" : "back"));
	if (to_what == B_SEEK_BY_FRAME) {
//SEEK(("frame %Ld -> ", *inout_frame));
		if (*inout_frame < 0)
			*inout_frame = 0;
	} else if (to_what == B_SEEK_BY_TIME) {
//SEEK(("time %Ld -> ", *inout_time));
		if (*inout_time < 0)
			*inout_time = 0;
	} else {
		return B_BAD_INDEX;
	}

	// check if we just have a raw audio stream...
	if (fHaveAudio && fHaveVideo == false) {
		bigtime_t tpos;
		off_t     fpos;
		
		if (to_what == B_SEEK_BY_FRAME) {
			tpos = (bigtime_t)(
				((double)*inout_frame / (double)fAudioNumFrames) *
				(double)fAudioDuration);
		} else /*if (to_what == B_SEEK_BY_TIME)*/ {
			tpos = *inout_time;
		}
		
//		printf("mpeg_audio_seek(tpos = %Ld, fpos = %Ld)\n", tpos, fpos);
		if (mpeg_audio_seek(tpos, &fpos, !seek_forward) != B_OK)
			return B_ERROR;
//		printf("  mpeg_audio_seek(tpos = %Ld, fpos = %Ld)\n", tpos, fpos);


		*inout_time = (bigtime_t)(((double)fpos /
			(fAudioFormat.u.encoded_audio.bit_rate / 8.0)) * 1000000.0);
		*inout_frame = (int64)((*inout_time / 1000000.0) * 
			(fAudioFormat.u.encoded_audio.output.frame_rate));
		*inout_filePos = *curpos = fpos;
		*inout_packetLength = 4096;
		
		if (fpos + 4096 > fAudioStartOffset + fAudioSize) {
			*inout_packetLength = (fAudioStartOffset + fAudioSize) - fpos;
		}

		DEBUG("MP3 SEEK: found a sync token @ fpos %Ld (%Ld %Ld)\n", fpos,
			   *inout_time, *inout_frame);

		*out_done = true;
		return B_OK;
	}
	
	c = mpeg.streams[mpeg.stream_mapping[in_stream] - STREAM_ID_BASE].h;

	// if no layer_header, it must be an audio track which seeks differently
	if (!c) {
		double        bitrate = fAudioFormat.u.encoded_audio.bit_rate;
		double        tmp;
		bigtime_t     tpos, sum;
		struct layer *l;
		
		if (fHaveAudio == false)
			return B_BAD_INDEX;

		if (to_what == B_SEEK_BY_FRAME) {
			tpos = (bigtime_t)(((double)*inout_frame / (double)fAudioNumFrames) *
				(double)fAudioDuration);
		} else /*if (to_what == B_SEEK_BY_TIME)*/ {
			tpos = *inout_time;
		}

		l = mpeg.streams[mpeg.stream_mapping[in_stream] - STREAM_ID_BASE].l;
SEEK(("extractor: seeking audio to %Ld\n",tpos));

		for(sum=0; l; l=l->next) {
			sum += l->len;

			tmp = ((double)sum / (bitrate / 8.0)) * 1000000.0 / FUDGE; // the need for the fudge factor is unknown...
			if (tmp > tpos)
				break;
		}
SEEK(("extractor: seeked audio to %Ld\n",int64(tmp)));

		if (l == NULL)
		{
			if(fScanThread<0)
				return B_BAD_INDEX; // scan-thread is done, and we're still out of range
			// scan-thread is running, so estimate the fileposition
#if GUESS_SEEK
			int64 where=tpos*fFileSize/fAudioDuration; // this should be pretty close
			int32 packetlength;
			
			where=SeekForwardFrom(where,0xc0,0xe0,&packetlength);
			if(where<0)
				return B_BAD_INDEX;

			sum = (tpos * FUDGE * (bitrate / 8.0))/1000000.0; // ideally we are here
			*inout_filePos = where;
			*inout_packetLength = packetlength;
printf("guessed audio offset to be %d\n",int(*inout_filePos));
#else
			// wait for background-scan to complete then try again
			long dummy;
			wait_for_thread(fScanThread,&dummy);
			return Seek(in_stream, cookie, to_what, flags, inout_time, inout_frame, inout_filePos, 
							in_packetPointer, inout_packetLength, out_done);
#endif
		}
		else
		{
			sum -= l->len;
			*inout_filePos = l->offset;
			*inout_packetLength = l->len;
//printf("calculated audio offset\n");
		}

		tmp = ((double)sum / (bitrate / 8.0)) * 1000000.0 / FUDGE;
SEEK(("extractor: final position %Ld\n",int64(tmp)));

		*inout_time = (bigtime_t)tmp;
		*inout_frame = (int64)((tmp / 1000000.0) *
			(fAudioFormat.u.encoded_audio.output.frame_rate));
		*out_done = true;

		DEBUG("audio seek: time %Ld frame %Ld fpos %Ld packetlen %ld\n",
			  *inout_time, *inout_frame, *inout_filePos, *inout_packetLength);

		return B_OK;
	}

	if (to_what == B_SEEK_BY_FRAME) {
		*inout_time = (bigtime_t)(1e6 * *inout_frame / fFrameRate);
SEEK(("%Ld -> ", *inout_time));
	}

	group = mpeg.streams[mpeg.stream_mapping[in_stream] - STREAM_ID_BASE].seek.recent_layer_header;
	if(group != NULL && group->t <= *inout_time) {
		c = group;
	}

	group = prev_group = next_group = c;

	while(c) {
		if(c->header_type == 0xb8) {
			prev_group = next_group;
			next_group = c;
		}
		if(next_group->t >= *inout_time) {
			if(seek_forward || next_group->t == *inout_time) {
				group = next_group;
			}
			else {
				group = prev_group;
			}
			break;
		}
		c = c->next;
	}
	if(!c) {	// watch the end
#if GUESS_SEEK
		if(fScanThread>=0)
		{
			printf("video seek reached end\n");
			// scan-thread is running, so estimate the fileposition
			int64 where=(*inout_time)*fFileSize/fAudioDuration; // this should be pretty close
			int32 packetlength;
			
			where=SeekForwardFrom(where,0xe0,0xf0,&packetlength);
			if(where>=0)
			{
				*inout_filePos = where;
				*inout_frame = (*inout_time) * fFrameRate * 1000000.0;
				*inout_packetLength = packetlength;
				*out_done = true;
				printf("guessed video offset to be %d\n",int(*inout_filePos));
				mpeg.streams[mpeg.stream_mapping[in_stream] - STREAM_ID_BASE].seek.state = 1;
				return B_OK;
			}
		}
		group = next_group;
#else
			if(fScanThread<0)
				return B_BAD_INDEX; // really past the end
			// wait for background-scan to complete then try again
			long dummy;
			wait_for_thread(fScanThread,&dummy);
			return Seek(in_stream, cookie, to_what, flags, inout_time, inout_frame, inout_filePos, 
							in_packetPointer, inout_packetLength, out_done);
#endif
	}

	*inout_time = group->t;
	*inout_frame = group->f;
	*inout_filePos = group->sequence->offset;
	*inout_packetLength = group->sequence->len;
	*out_done = true;
SEQ(("patching sequence (%Ld + %d) before group (%Ld + %d)\n", *inout_filePos, *inout_packetLength, group->offset, group->len));

	mpeg.streams[mpeg.stream_mapping[in_stream] - STREAM_ID_BASE].seek.recent_layer_header = group;
	mpeg.streams[mpeg.stream_mapping[in_stream] - STREAM_ID_BASE].seek.header = group;
	mpeg.streams[mpeg.stream_mapping[in_stream] - STREAM_ID_BASE].seek.state = 1;

	DEBUG("SEEK out: t = %Lx f = %Lx off = %Lx\n", *inout_time, *inout_frame, *inout_filePos);
SEEK(("t%Ld/f%Ld\n", *inout_time, *inout_frame));

	return B_OK;
}


status_t MPEGSystemExtractor::SplitNext(int32 in_stream,
                                        void *cookie,
										off_t *inout_filepos,
										char *in_packetPointer,
										int32 *inout_packetLength,
										char **out_bufferStart,
										int32 *out_bufferLength,
										media_header *h)
{
	struct layer *p;
	struct stream *st;
	int32         len;
	off_t         start;
	off_t        *curpos = (off_t *)cookie;

	in_packetPointer = in_packetPointer;  /* shuts up the compiler */

//	printf("SplitNext for stream %lx (%Lx %lx)\n", in_stream, *inout_filepos, *inout_packetLength);
//	printf("SplitNext: curpos: %Ld\n",*curpos);
	if (in_stream < 0 || in_stream >= 2)
		return B_BAD_INDEX;

	if (fHaveAudio && !fHaveVideo) {
		if ((*curpos < *inout_filepos) ||
			(*curpos >= (*inout_filepos + *inout_packetLength))) {

			*out_bufferStart = NULL;
			*inout_filepos = *curpos;
			if (fAudioSize > 0 && *curpos + 4096 > fAudioStartOffset + fAudioSize)
				*inout_packetLength = (fAudioStartOffset + fAudioSize) - *curpos;
			else 
				*inout_packetLength = 4096;

//			printf("split next pos %Ld, len %ld\n", *inout_filepos, *out_bufferLength);
			return B_OK;
		}

		if ((fAudioSize > 0) && (*curpos + 4096 > fAudioStartOffset + fAudioSize)) 
			len = (fAudioStartOffset + fAudioSize) - *curpos;
		else 
			len = 4096;

		if(len <= 0)
			return B_LAST_BUFFER_ERROR;
		
		if (len > *inout_filepos + *inout_packetLength - *curpos)
			len = *inout_filepos + *inout_packetLength - *curpos;

		*out_bufferStart = in_packetPointer + (*curpos - *inout_filepos);
		*out_bufferLength = len;
		if(*inout_filepos<fAudioStartOffset)
		{
			// inputbuffer contains ID3v2 data (and possibly mp3 data as well)

			if((*inout_filepos + *inout_packetLength) < fAudioStartOffset)
			{
				// this buffer doesn't contain any mp3 data at all
				*out_bufferStart=NULL;
				*inout_filepos=fAudioStartOffset;
				if((fAudioSize>0) && (*curpos+4096 > fAudioStartOffset+fAudioSize))
					*inout_packetLength = (fAudioStartOffset + fAudioSize) - *curpos;
				else 
					*inout_packetLength = 4096;
			}
			else
			{
				*out_bufferStart+=(fAudioStartOffset-*inout_filepos);
				*out_bufferLength-=(fAudioStartOffset-*inout_filepos);
			}
		}
		*inout_filepos = *curpos + len;

		h->type = B_MEDIA_ENCODED_AUDIO;
		h->start_time = 0;
		h->orig_size = len;
		h->file_pos = *curpos;
		
		*curpos += len;

	//	printf("split next pos %Ld, len %d, bp %p (%p)\n", *inout_filepos, *out_bufferLength,
	//	   in_packetPointer, *out_bufferStart);
		
		return B_OK;
	}
	

	st = &mpeg.streams[mpeg.stream_mapping[in_stream] - STREAM_ID_BASE];

	// We want to be able to seek to groups of pictures (for finer-grained seeking),
	// but we need the information in the sequence header associated with it.
	// alter the stream so the first block read after a seek is a sequence_header
	// even if it doesn't appear there in the file.  (Seek() sets up this state)
	if(st->seek.header != NULL) {
		if(st->seek.state == 0) {
			// change the offset to point to the proper group header
			*inout_filepos = st->seek.header->offset;
			*inout_packetLength = st->seek.header->len;
SEQ(("jumping to group (%Ld + %d)\n", *inout_filepos, *inout_packetLength));
			st->seek.header = NULL;
			*out_bufferStart = NULL;
			return B_OK;
		}
		else {
			if(*inout_filepos == st->seek.header->sequence->offset) {
				// we need to reset the length of the sequence header
				// so the decoder doesn't get the wrong data
				*inout_packetLength = st->seek.header->sequence->len;
			}
			else {
				// someone called SplitNext with a different offset before 
				// we could finish our seek;  bail out.
				st->seek.header = NULL;
			}
			st->seek.state = 0;
SEQ(("SplitNext to (%Ld + %d) during pending seek\n", *inout_filepos, *inout_packetLength));
		}
	}

	p = st->seek.recent_layer;
	if(p == NULL || p->offset > *inout_filepos) {
		p = st->l;
	}
	if (!p) {
//printf("SPlitNext error 1\n");
		return B_BAD_INDEX;
	}

	while (p) {
		if (p->offset + p->len > *inout_filepos)
			break;
		p = p->next;
	}
	if (!p) {
		/* nothing found */
		//printf("Last buffer already sent, seek state %d\n",st->seek.state);
		return B_LAST_BUFFER_ERROR;
	}
	st->seek.recent_layer = p;
	
	if (*inout_filepos + *inout_packetLength <= p->offset) {
		*out_bufferStart = NULL;
		*inout_filepos = p->offset;
		*inout_packetLength = p->len;
		return B_OK;
	}

	start = (p->offset > *inout_filepos) ? p->offset : *inout_filepos;
	len = p->offset + p->len - start;
	if (len > *inout_filepos + *inout_packetLength - start)
		len = *inout_filepos + *inout_packetLength - start;

	*out_bufferStart = in_packetPointer + (start - *inout_filepos);
	*out_bufferLength = len;
	*inout_filepos = start + len;


	h->type = B_MEDIA_ENCODED_VIDEO;
	h->start_time = 0LL;
	h->orig_size = len;
	h->file_pos = start;
    h->u.encoded_video.field_number = 0;
	h->u.encoded_video.pulldown_number = 0;
	h->u.encoded_video.field_flags = 0;

	DEBUG("SplitNext: %Lx %lx\n", h->file_pos, h->orig_size);

//	if (!(p->next) && (*inout_filepos == p->offset + p->len)) {
//		*inout_filepos = fFileSize;
//	}

	return B_OK;
}


status_t MPEGSystemExtractor::CountFrames(int32 in_stream, int64 *out_frames)
{
	struct stream *s;

	if(fScanThread!=-1) // scan-thread is still busy, provide the running estimate
	{
		*out_frames=(bigtime_t)(((double)fAudioSize /
				(fAudioFormat.u.encoded_audio.bit_rate / 8.0)) * fFrameRate);
		printf("CountFrames: %Ld\n",*out_frames);
		return B_OK;
	}

	DEBUG("CountFrames - in_stream %ld\n", in_stream);
	if (fHaveVideo) {
		if (in_stream == kVideoStream) {
			s = mpeg.streams + mpeg.stream_mapping[in_stream] - STREAM_ID_BASE;
			if (!s->h) {
				printf("MPEGExtractor::CountFrames(): Not yet initialized\n");
				return B_ERROR;
			}

			*out_frames = s->f;
		} else if (in_stream == kAudioStream && fHaveAudio) {
			*out_frames = fAudioNumFrames;

		} else {
			return B_BAD_INDEX;
		}
	} else if (fHaveAudio) {
		if (in_stream != 0)
			return B_BAD_INDEX;

		*out_frames = fAudioNumFrames;
	} else {
		return B_BAD_INDEX;
	}
printf("number of frames: %Ld\n",*out_frames);
	return B_OK;
}

status_t MPEGSystemExtractor::GetDuration(int32 in_stream, bigtime_t *out_expireTime)
{
	struct stream *s;

	DEBUG("GetDuration - in_stream %ld\n", in_stream);

	if(fScanThread!=-1) // scan-thread is still busy, provide the running estimate
	{
		*out_expireTime=(bigtime_t)(((double)fAudioSize /
				(fAudioFormat.u.encoded_audio.bit_rate / 8.0)) * 1000000.0);
		printf("GetDuration: %Ld\n",*out_expireTime);
		return B_OK;
	}

	if (fHaveVideo) {
		if (in_stream == kVideoStream) {
			s = mpeg.streams + mpeg.stream_mapping[in_stream] - STREAM_ID_BASE;
			if (!s->h) {
				printf("MPEGExtractor::GetDuration(): Not yet initialized\n");
				return B_ERROR;
			}

			*out_expireTime = (bigtime_t)(1e6 * s->f / fFrameRate);
		
			DEBUG("MPEG video duration: %2.2Ld:%2.2Ld.%2.2Ld (%Ld)\n",
				  *out_expireTime / 1000000LL / 60,
				  (*out_expireTime / 1000000LL) % 60,
				  (*out_expireTime / 10000LL) % 100, *out_expireTime);
		} else if (in_stream == kAudioStream && fHaveAudio) {
			*out_expireTime = fAudioDuration;
		} else {
			return B_BAD_INDEX;
		}
	} else if (fHaveAudio) {
		if (in_stream != 0)
			return B_BAD_INDEX;

		*out_expireTime = fAudioDuration;
	} else {
		return B_BAD_INDEX;
	}

	return B_OK;
}


/* more stuff from Xing's VBR SDK */
#include <stdlib.h>
#include <float.h>
#include <math.h>

static int ExtractI4(unsigned char *buf)
{
	int x;
	// big endian extract
	
	x = buf[0];
	x <<= 8;
	x |= buf[1];
	x <<= 8;
	x |= buf[2];
	x <<= 8;
	x |= buf[3];
	
	return x;
}

static int GetXingHeader(XHEADDATA *X,  unsigned char *buf, int len)
{
	int i, head_flags;
	int h_id, h_mode, h_sr_index;
	static int sr_table[4] = { 44100, 48000, 32000, 99999 };
	
	// get Xing header data
	X->flags = 0;     // clear to null incase fail
	
	// get selected MPEG header data
	if(len<4) return 0; // not enough data left
	h_id       = (buf[1] >> 3) & 1;
	h_sr_index = (buf[2] >> 2) & 3;
	h_mode     = (buf[3] >> 6) & 3;
	
	// determine offset of header
	int skip;
	if( h_id ) {        // mpeg1
	    if( h_mode != 3 ) skip=(32+4);
	    else              skip=(17+4);
	}
	else {      // mpeg2
	    if( h_mode != 3 ) skip=(17+4);
	    else              skip=(9+4);
	}
	buf+=skip;
	len-=skip;
	if(len<4) return 0; // not enough data left
	
	if( buf[0] != 'X' ) return 0;    // fail
	if( buf[1] != 'i' ) return 0;    // header not found
	if( buf[2] != 'n' ) return 0;
	if( buf[3] != 'g' ) return 0;
	buf+=4; len-=4;
	
	X->h_id = h_id;
	X->samprate = sr_table[h_sr_index];
	if( h_id == 0 ) X->samprate >>= 1;
	
	if(len<4) return 0; // not enough data left
	head_flags = X->flags = ExtractI4(buf); buf+=4; len-=4;     // get flags
	
	if( head_flags & XING_FRAMES_FLAG )
	{
		if(len<4) return 0; // not enough data left
		X->frames   = ExtractI4(buf); buf+=4; len-=4;
	}
	if( head_flags & XING_BYTES_FLAG )
	{
		if(len<4) return 0; // not enough data left
		X->bytes = ExtractI4(buf); buf+=4; len-=4;
	}
	
	if( head_flags & XING_TOC_FLAG )
	{
		if(len<100) return 0; // not enough data left
        for(i=0;i<100;i++) X->toc[i] = buf[i];
	    buf+=100; len-=100;
	}
	
	X->vbr_scale = -1;
	if( head_flags & XING_VBR_SCALE_FLAG )
	{
		if(len<4) return 0; // not enough data left
		X->vbr_scale = ExtractI4(buf); buf+=4; len-=4;
	}
	
	//for(i=0;i<100;i++) {
	//    if( (i%10) == 0 ) printf("\n");
	//    printf(" %3d", (int)(X->toc[i]));
	//}
	
	return 1;       // success
}

static int SeekPoint(unsigned char TOC[100], int file_bytes, float percent)
{
	// interpolate in TOC to get file seek point in bytes
	int a, seekpoint;
	float fa, fb, fx;
	
	
	if( percent < 0.0f )   percent = 0.0f;
	if( percent > 100.0f ) percent = 100.0f;
	
	a = (int)percent;
	if( a > 99 ) a = 99;
	fa = TOC[a];
	if( a < 99 ) {
	    fb = TOC[a+1];
	}
	else {
	    fb = 256.0f;
	}
	
	
	fx = fa + (fb-fa)*(percent-a);
	
	seekpoint = (int)((1.0f/256.0f)*fx*file_bytes); 
	
	return seekpoint;
}

