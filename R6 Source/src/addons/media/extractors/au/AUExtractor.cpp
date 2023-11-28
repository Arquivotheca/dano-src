#include "AUExtractor.h"
#include <stdio.h>
#include <math.h>
#include <MediaFormats.h>
#include <ByteOrder.h>
#include <File.h>
#include <Debug.h>

//#define DEBUG	printf
#define DEBUG	if (0) printf

/* AU signatures */
#define _AU_MAGIC	'.snd'

/* Private struct */
typedef struct {
	int32	dataLocation;        /* offset or pointer to the data */ 
	int32	dataSize;            /* number of bytes of data */ 
	int32	dataFormat;          /* the data format code */ 
	int32	samplingRate;        /* the sampling rate */ 
	int32	channelCount;        /* the number of channels */ 
} _au_sound_header;

enum {
	AU_CHUNK_SIZE		= 65536,
	MINIMAL_BLOC_LENGTH	= 1024,
	AU_BUFFER_LENGTH	= 60000	// in us
};

enum {
	SND_FORMAT_UNSPECIFIED			= 0,	// unspecified format 
	SND_FORMAT_MULAW_8				= 1,	// 8-bit mu-law samples 
	SND_FORMAT_LINEAR_8				= 2,	// 8-bit linear samples 
	SND_FORMAT_LINEAR_16			= 3,	// 16-bit linear samples 
	SND_FORMAT_LINEAR_24			= 4,	// 24-bit linear samples 
	SND_FORMAT_LINEAR_32			= 5,	// 32-bit linear samples 
	SND_FORMAT_FLOAT				= 6,	// floating-point samples 
	SND_FORMAT_DOUBLE				= 7,	// double-precision float samples 
	SND_FORMAT_INDIRECT				= 8,	// fragmented sampled data 
	SND_FORMAT_NESTED				= 9,	// ? 
	SND_FORMAT_DSP_CORE				= 10,	// DSP program 
	SND_FORMAT_DSP_DATA_8			= 11,	// 8-bit fixed-point samples 
	SND_FORMAT_DSP_DATA_16			= 12,	// 16-bit fixed-point samples 
	SND_FORMAT_DSP_DATA_24			= 13,	// 24-bit fixed-point samples 
	SND_FORMAT_DSP_DATA_32			= 14,	// 32-bit fixed-point samples 
	SND_FORMAT_DISPLAY				= 16,	// non-audio display data 
	SND_FORMAT_MULAW_SQUELCH		= 17,	// ? 
	SND_FORMAT_EMPHASIZED			= 18,	// 16-bit linear with emphasis 
	SND_FORMAT_COMPRESSED			= 19,	// 16-bit linear with compression 
	SND_FORMAT_COMPRESSED_EMPHASIZED = 20,	// A combination of the two above 
	SND_FORMAT_DSP_COMMANDS			= 21,	// Music Kit DSP commands 
	SND_FORMAT_DSP_COMMANDS_SAMPLES	= 22,	// ? 
	SND_FORMAT_SUN_G721				= 23,	// CCITT G.721 4-bits ADPCM
	SND_FORMAT_SUN_G723_3			= 25,	// CCITT G.723 3-bits ADPCM
	SND_FORMAT_SUN_G723_5			= 26	// CCITT G.723 5-bits ADPCM
};

extern "C" const char *mime_types_extractor[] = { "audio/basic", NULL };

extern "C" Extractor *instantiate_extractor(void)
{
	return new AUExtractor();
}

/* Code */
AUExtractor::AUExtractor() {
	/* format description */
	fSampleFormat = 0;
	fChannelCount = 0;
	fMediaType = 0;
	fRate = 0.0;
	fBlocSizeOut = 0;
	/* pseudo-bloc management */
	fBlocSize = 0;
	fFramePerBloc = 0;
	fFrameCount = 0;
	fFrameSize = 0;
	/* data access */
	fDataOffset = 0;
	fDataEnd = 0;
}

AUExtractor::~AUExtractor() {
}

status_t AUExtractor::GetFileFormatInfo(media_file_format *mfi) {
	strcpy(mfi->mime_type,      "audio/basic");
	strcpy(mfi->pretty_name,    "AU Encoded Sound File");
	strcpy(mfi->short_name,     "au");
	strcpy(mfi->file_extension, "au");

	mfi->family = B_MISC_FORMAT_FAMILY;

    mfi->capabilities = media_file_format::B_READABLE              |
                        media_file_format::B_PERFECTLY_SEEKABLE    |
                        media_file_format::B_KNOWS_RAW_AUDIO       |
                        media_file_format::B_KNOWS_ENCODED_AUDIO;

	return B_OK;
}


/* Initialisation/Setup call before taking other the data extraction */
status_t AUExtractor::Sniff(int32 *out_streamNum, int32 *out_chunkSize) {
	int32		magic;
	status_t	err;

	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	if (pio == NULL)
		return B_IO_ERROR;

	/* Check the magic signature for AU. */
	pio->Seek(0, SEEK_SET);
	if (pio->Read(&magic, sizeof(magic)) != sizeof(magic))
		return B_IO_ERROR;
	if (B_BENDIAN_TO_HOST_INT32(magic) != _AU_MAGIC)
		return B_BAD_TYPE;

	/* Read the header and set internal states */
	err = ReadHeader();
	if (err != B_OK)
		return err;

	/* Complete initialisation of internal states */
	*out_streamNum = 1;
	*out_chunkSize = AU_CHUNK_SIZE;
	
DEBUG("fSampleFormat: %d\n", fSampleFormat);
DEBUG("fChannelCount: %d\n", fChannelCount);
DEBUG("fMediaType: %d\n", fMediaType);
DEBUG("fRate: %f\n", fRate);
DEBUG("fBlocSizeOut: %d\n", fBlocSizeOut);
DEBUG("fBlocSize: %d\n", fBlocSize);
DEBUG("fFramePerBloc: %d\n", fFramePerBloc);
DEBUG("fFrameCount: %d\n", fFrameCount);
DEBUG("fFrameSize: %d\n", fFrameSize);
DEBUG("fDataOffset: %d\n", fDataOffset);
DEBUG("fDataEnd: %d\n", fDataEnd);

	return B_OK;
}

/* Read header and extract useful information */
status_t AUExtractor::ReadHeader() {
	_au_sound_header	h;

	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	if (pio == NULL)
		return B_IO_ERROR;

	/* Seek through the header tables, looking for the key infos */
	if (pio->Read(&h, sizeof(h)) != sizeof(h))
		return B_IO_ERROR;

	/* Read channel count and data descriptor */
	fChannelCount = B_BENDIAN_TO_HOST_INT32(h.channelCount);
	fDataOffset = B_BENDIAN_TO_HOST_INT32(h.dataLocation);
	fDataEnd = fDataOffset + B_BENDIAN_TO_HOST_INT32(h.dataSize);
	if (B_BENDIAN_TO_HOST_INT32(h.dataSize) == 0xffffffff)
		fDataEnd = pio->Seek(0, SEEK_END);

	/* Check the sample rate value */
	switch (B_BENDIAN_TO_HOST_INT32(h.samplingRate)) {
	case 8012 :
		fRate = 8012.821;
		break;
	default :
		fRate = (float)B_BENDIAN_TO_HOST_INT32(h.samplingRate);
		break;
	}
	if ((fRate >= 11022.0) && (fRate <= 11028.0))
		fRate = 11025.0;
	else if ((fRate >= 22044.0) && (fRate <= 22056.0))
		fRate = 22050.0;
	else if ((fRate >= 44088.0) && (fRate <= 44112.0))
		fRate = 44100.0;
	
	/* Check the format */
	switch (B_BENDIAN_TO_HOST_INT32(h.dataFormat)) {
	case SND_FORMAT_LINEAR_8 :
		fMediaType = B_MEDIA_RAW_AUDIO;
		fSampleFormat = media_raw_audio_format::B_AUDIO_CHAR;
		fFrameSize = fChannelCount*8;
		goto fixed_frame_size;
		
	case SND_FORMAT_LINEAR_16 :
		fMediaType = B_MEDIA_RAW_AUDIO;
		fSampleFormat = media_raw_audio_format::B_AUDIO_SHORT;
		fFrameSize = fChannelCount*16;
		goto fixed_frame_size;

	case SND_FORMAT_LINEAR_32 :
		fMediaType = B_MEDIA_RAW_AUDIO;
		fSampleFormat = media_raw_audio_format::B_AUDIO_INT;
		fFrameSize = fChannelCount*32;
		goto fixed_frame_size;

	case SND_FORMAT_FLOAT :
		fMediaType = B_MEDIA_RAW_AUDIO;
		fSampleFormat = media_raw_audio_format::B_AUDIO_FLOAT;
		fFrameSize = fChannelCount*32;
		goto fixed_frame_size;

	case SND_FORMAT_MULAW_8 :
		fMediaType = B_MEDIA_ENCODED_AUDIO;
		fSampleFormat = SND_FORMAT_MULAW_8;
		fFrameSize = fChannelCount*8;
fixed_frame_size :
		fFrameCount = (8*(fDataEnd-fDataOffset))/fFrameSize;
		fFramePerBloc = (int32)(fRate*(float)AU_BUFFER_LENGTH*0.000001);
		fBlocSize = (fFramePerBloc*fFrameSize)/8;
		fBlocSizeOut = fBlocSize;
		break;

	case SND_FORMAT_SUN_G721 :
DEBUG("##### SND_FORMAT_SUN_G721 !!\n");
		fFrameSize = 4*fChannelCount;
		goto adpcm_setup;
	case SND_FORMAT_SUN_G723_3 :
		fFrameSize = 3*fChannelCount;
		goto adpcm_setup;
	case SND_FORMAT_SUN_G723_5 :
		fFrameSize = 5*fChannelCount;
adpcm_setup:
		fMediaType = B_MEDIA_ENCODED_AUDIO;
		fSampleFormat = B_BENDIAN_TO_HOST_INT32(h.dataFormat);
		fFrameCount = ((fDataEnd-fDataOffset)*8)/fFrameSize;
		fFramePerBloc = (int32)(fRate*(float)AU_BUFFER_LENGTH*0.000000125)*8;
		fBlocSize = (fFrameSize*fFramePerBloc)/8;
		fBlocSizeOut = 2*fChannelCount*fFramePerBloc;
		break;

	case SND_FORMAT_UNSPECIFIED :
	case SND_FORMAT_LINEAR_24 :
	case SND_FORMAT_DOUBLE :
	case SND_FORMAT_INDIRECT :
	case SND_FORMAT_NESTED :
	case SND_FORMAT_DSP_CORE :
	case SND_FORMAT_DSP_DATA_8 :
	case SND_FORMAT_DSP_DATA_16 :
	case SND_FORMAT_DSP_DATA_24 :
	case SND_FORMAT_DSP_DATA_32 :
	case SND_FORMAT_DISPLAY :
	case SND_FORMAT_MULAW_SQUELCH :
	case SND_FORMAT_EMPHASIZED :
	case SND_FORMAT_COMPRESSED :
	case SND_FORMAT_COMPRESSED_EMPHASIZED :
	case SND_FORMAT_DSP_COMMANDS :
	case SND_FORMAT_DSP_COMMANDS_SAMPLES :
DEBUG("Known but unsupported AU encoding (%d)\n", B_BENDIAN_TO_HOST_INT32(h.dataFormat));
		return B_BAD_TYPE;
	default :
DEBUG("Unsknown AU encoding (%d)\n", B_BENDIAN_TO_HOST_INT32(h.dataFormat));
		return B_BAD_TYPE;
	}

	fTimeFactor = 8e6/((float)fFrameSize*fRate);
	return B_OK;
}  

/* Provide information about output format */
status_t AUExtractor::TrackInfo(int32 in_stream, media_format *out_format, void **out_info, int32 *out_size) {
	status_t		err;

	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* fills the media_format descriptor */
	out_format->type = (media_type)fMediaType;
	switch (out_format->type) {
	case B_MEDIA_RAW_AUDIO :
		out_format->u.raw_audio.frame_rate = fRate;
		out_format->u.raw_audio.channel_count = fChannelCount;
		out_format->u.raw_audio.format = fSampleFormat;
		out_format->u.raw_audio.byte_order = B_MEDIA_BIG_ENDIAN;
		out_format->u.raw_audio.buffer_size = fBlocSize;
		break;
	case B_MEDIA_ENCODED_AUDIO :
		switch (fSampleFormat) {
		case SND_FORMAT_MULAW_8 :
			{
			BMediaFormats				formatObject;
			media_format_description	fd;

			memset(out_format, 0, sizeof(media_format));	
			memset(&fd, 0, sizeof(fd));
			fd.family = B_MISC_FORMAT_FAMILY;
			fd.u.misc.file_format = '.snd';
			fd.u.misc.codec = 'ulaw';
			formatObject.Lock();
			err = formatObject.GetFormatFor(fd, out_format);
			formatObject.Unlock();
			if (err != B_OK)
				return err;
	
			out_format->type = B_MEDIA_ENCODED_AUDIO;
			out_format->u.encoded_audio.output.frame_rate = fRate;
			out_format->u.encoded_audio.output.channel_count = fChannelCount;
			out_format->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
			out_format->u.encoded_audio.output.byte_order =
				B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
			out_format->u.encoded_audio.output.buffer_size = fBlocSize*2;
			}
			break;
		case SND_FORMAT_SUN_G721 :
		case SND_FORMAT_SUN_G723_3 :
		case SND_FORMAT_SUN_G723_5 :
			{
			BMediaFormats				formatObject;
			media_format_description	fd;

			memset(out_format, 0, sizeof(media_format));	
			memset(&fd, 0, sizeof(fd));
			fd.family = B_MISC_FORMAT_FAMILY;
			fd.u.misc.file_format = '.snd';
			if (fSampleFormat == SND_FORMAT_SUN_G721)
				fd.u.misc.codec = '7214';
			else if (fSampleFormat == SND_FORMAT_SUN_G723_3)
				fd.u.misc.codec = '7233';
			else
				fd.u.misc.codec = '7235';
			formatObject.Lock();
			err = formatObject.GetFormatFor(fd, out_format);
			formatObject.Unlock();
			if (err != B_OK)
				return err;

			out_format->type = B_MEDIA_ENCODED_AUDIO;
			out_format->u.encoded_audio.output.frame_rate = fRate;
			out_format->u.encoded_audio.output.channel_count = fChannelCount;
			out_format->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
			out_format->u.encoded_audio.output.byte_order =
				B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
			out_format->u.encoded_audio.output.buffer_size = fBlocSizeOut;
			if (fSampleFormat == SND_FORMAT_SUN_G721)
				out_format->u.encoded_audio.bit_rate = 4;
			else if (fSampleFormat == SND_FORMAT_SUN_G723_3)
				out_format->u.encoded_audio.bit_rate = 3;
			else
				out_format->u.encoded_audio.bit_rate = 5;
			out_format->u.encoded_audio.frame_size = fBlocSize;
			}
			break;
		default :
DEBUG("Should never happen (internal error) 0\n");
			return B_ERROR;
		}
		break;
	default :
DEBUG("Should never happen (internal error) 1\n");
		return B_ERROR;
	}
	return B_OK;
}

status_t AUExtractor::CountFrames(int32 in_stream, int64 *out_frames) {
	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* return the frame count */
	*out_frames = (int64)fFrameCount;
	return B_OK;
}

status_t AUExtractor::GetDuration(int32 in_stream, bigtime_t *out_duration) {
	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* return the duration */
	*out_duration = (bigtime_t)(((double)fFrameCount*1000000.0)/fRate);
	return B_OK;
}

status_t 
AUExtractor::AllocateCookie(int32 in_stream, void **cookieptr)
{
	*cookieptr = malloc(sizeof(uint32));
	if(*cookieptr == NULL)
		return B_NO_MEMORY;
	*((uint32*)*cookieptr) = fDataOffset;
	return B_NO_ERROR;
}

status_t 
AUExtractor::FreeCookie(int32 in_stream, void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

status_t AUExtractor::SplitNext(	int32	in_stream,
									void	*cookie,
									off_t	*inout_filepos,
								  	char	*in_packetPointer,
								  	int32	*inout_packetLength,
								  	char	**out_bufferStart,
								  	int32	*out_bufferLength,
									media_header *mh)
{
	int32			local_offset, local_end, real_end;
	uint32			*read_from_ptr = (uint32 *)cookie;

	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* Check if we are not reading to far ahead */
	local_offset = *read_from_ptr-(uint32)(*inout_filepos);
	if (local_offset < 0) {
		*out_bufferStart = (char*)NULL;
		*inout_filepos = *read_from_ptr;
		return B_OK;
	}
	/* Check if we are not reading to far back */
	local_end = local_offset + fBlocSize;
	if (local_end > AU_CHUNK_SIZE) {
		*out_bufferStart = (char*)NULL;
		*inout_filepos = *read_from_ptr;
		return B_OK;
	}
	/* Check if we arrived near the end */
	real_end = (int32)fDataEnd-(int32)(*inout_filepos);
	if (local_end > real_end)
		local_end = real_end;

	if (local_end == local_offset) {
		mh->start_time = (bigtime_t)((float)(fDataEnd-fDataOffset)*fTimeFactor);
		return B_LAST_BUFFER_ERROR;
	};
	/* Check if we read far enough */
	if (local_end > (int32)*inout_packetLength) {
		*out_bufferStart = (char*)NULL;
		*inout_packetLength = local_end;
		return B_OK;
	}
	/* Do the split and offset the pointers for the next one... */
	*out_bufferStart = in_packetPointer+local_offset;
	mh->orig_size = *out_bufferLength = local_end-local_offset;
	mh->start_time = (bigtime_t)((float)(*read_from_ptr-fDataOffset)*fTimeFactor);
	*read_from_ptr += local_end-local_offset;
	mh->file_pos = *inout_filepos = *read_from_ptr;
	return B_OK;
}

/* Seeking functions */
status_t AUExtractor::Seek(int32		in_stream,
							 void		*cookie,
							 int32		in_towhat,
							 int32		flags,
							 bigtime_t	*inout_time,
							 int64		*inout_frame,
							 off_t		*inout_filePos,
							 char		*in_packetPointer,
							 int32		*inout_packetLength,
							 bool		*out_done) {
	int32			frame_index, cur_frame;
	int32			length, real_length;
	uint32			*read_from_ptr = (uint32 *)cookie;

	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* split between the different cases */
	switch (in_towhat) {
	case B_SEEK_BY_TIME :
		/* Calculate the frame index equivalent, based on time */
		frame_index = (uint32)((*inout_time)*fRate*0.000001+0.5);
		goto seek_from_index;
	case B_SEEK_BY_FRAME :
		/* Calculate the frame index equivalent */
		frame_index = *inout_frame;
seek_from_index:
		if (frame_index < 0)
			frame_index = 0;
		else if (frame_index > fFrameCount)
			frame_index = fFrameCount;
		/* Seeking is poorly supported in adpcm mode */
		if (fFrameSize < 8) {
			cur_frame = (8*(*read_from_ptr-fDataOffset))/fFrameSize;
			if (cur_frame > frame_index)
				frame_index = 0;
			else 
				frame_index = cur_frame;
		}
		*read_from_ptr = fDataOffset+(frame_index*fFrameSize)/8;
		*inout_time = (bigtime_t)(((double)frame_index*1000000.0)/fRate);
		*inout_frame = frame_index;
		*inout_filePos = *read_from_ptr;
		length = fBlocSize;
		real_length = (int32)fDataEnd-*read_from_ptr;
		if (real_length < length)
			length = real_length;
		*inout_packetLength = length;
		*out_done = true;
		return B_OK;
	default :
		return B_BAD_VALUE;
	}
}
