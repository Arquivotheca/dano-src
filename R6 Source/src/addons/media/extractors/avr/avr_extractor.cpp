#include <stdio.h>
#include <math.h>
#include <File.h>
#include <ByteOrder.h>
#include <MediaFormats.h>

#include "avr_extractor.h"

#if DEBUG
#define PRINTF printf
#else
#define PRINTF (void)
#endif


#define AVR_HDR_SIZE     128
#define AVR_MONO_OFFSET  12	   /* offset in bytes to the mono field */

typedef struct avr_header {
	char magic[4];          /* "2BIT" is the magic number */
	char name[8];           /* null-padded sample name */ 
	ushort mono;            /* 0 = mono, 0xffff = stereo */ 
	ushort rez;             /* 8 = 8 bit, 16 = 16 bit */ 
	ushort sign;            /* 0 = unsigned, 0xffff = signed */ 
	ushort loop;            /* 0 = no loop, 0xffff = looping sample */ 
	ushort midi;            /* 0xffff = no MIDI note assigned, 
							   0xffXX = single key note assignment 
							   0xLLHH = key split, low/hi note */ 
	long rate;              /* sample frequency in hertz */ 
	long size;              /* sample length in bytes or words (see rez) */ 
	long lbeg;              /* offset to start of loop in bytes or words. 
							   set to zero if unused. */ 
	long lend;              /* offset to end of loop in bytes or words. 
							   set to sample length if unused. */ 
	short res1;             /* Reserved, MIDI keyboard split */ 
	short res2;             /* Reserved, sample compression */ 
	short res3;             /* Reserved */ 
	char ext[20];           /* Additional filename space, used 
							   if (name[7] != 0) */ 
	char user[64];          /* User defined. Typically ASCII message. */ 
}avr_header;



enum {
	AVR_CHUNK_SIZE		= 65536,
	MINIMAL_BLOC_LENGTH	= 1024,
//	AVR_BUFFER_LENGTH	= 40000	// in us
	AVR_BUFFER_LENGTH	= 60000	// in us
};


extern "C" const char * mime_type_extractor = "audio/x-avr";

extern "C" Extractor *instantiate_extractor(void)
{
	return new AVRExtractor();
}


/* code */
AVRExtractor::AVRExtractor() {
	/* format description */
	fSampleFormat = 0;
	fChannelCount = 0;
	fFrameCount = 0;
	fFrameSize = 0;
	fRate = 0.0;
	/* pseudo-bloc management */
	fBlocSize = 0;
	fFramePerBloc = 0;
	/* data access */
	fDataOffset = 0;
	fDataEnd = 0;
	fDataLength = 0;
}

AVRExtractor::~AVRExtractor()
{
}

status_t
AVRExtractor::GetFileFormatInfo(media_file_format *mfi)
{
	strcpy(mfi->mime_type,      "audio/x-avr");
	strcpy(mfi->pretty_name,    "AVR Sound File");
	strcpy(mfi->short_name,     "avr");
	strcpy(mfi->file_extension, "avr");

	mfi->family = B_AVR_FORMAT_FAMILY;

    mfi->capabilities = media_file_format::B_READABLE              |
                        media_file_format::B_PERFECTLY_SEEKABLE    |
                        media_file_format::B_KNOWS_RAW_AUDIO       |
                        media_file_format::B_KNOWS_ENCODED_AUDIO;

	return B_OK;
}

status_t
AVRExtractor::ReadHeader()
{
	char        hdrbuff[128];
	int16       tmpshort;
	int32       tmpint;
	avr_header	hdr;

	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	if (pio == NULL)
		return B_IO_ERROR;


	/* Check the magic signature for AVR */
	pio->Seek(0, SEEK_SET);	
	if (pio->Read(hdrbuff, AVR_HDR_SIZE) != AVR_HDR_SIZE)
		return B_IO_ERROR;

	if (strncmp(hdrbuff, "2BIT", 4) != 0)
		return B_BAD_TYPE;

    char *ptr = &hdrbuff[AVR_MONO_OFFSET];

    memcpy((void *)&tmpshort, ptr, sizeof(short));
    hdr.mono = B_BENDIAN_TO_HOST_INT16(tmpshort);
	ptr += 2;

    memcpy((void *)&tmpshort, ptr, sizeof(short));
    hdr.rez = B_BENDIAN_TO_HOST_INT16(tmpshort);
	ptr += 2;

    memcpy((void *)&tmpshort, ptr, sizeof(short));
    hdr.sign = B_BENDIAN_TO_HOST_INT16(tmpshort);
	ptr += 2;

    memcpy((void *)&tmpshort, ptr, sizeof(short));
    hdr.loop = B_BENDIAN_TO_HOST_INT16(tmpshort);
	ptr += 2;

    memcpy((void *)&tmpshort, ptr, sizeof(short));
    hdr.midi = B_BENDIAN_TO_HOST_INT16(tmpshort);
	ptr += 2;

    memcpy((void *)&tmpint, ptr, sizeof(int));
    hdr.rate = B_BENDIAN_TO_HOST_INT32(tmpint);
	ptr += 4;

    memcpy((void *)&tmpint, ptr, sizeof(int));
    hdr.size = B_BENDIAN_TO_HOST_INT32(tmpint);
	ptr += 4;

	/* currently we don't care about the rest of it... */

	if (hdr.mono == 0 && hdr.rez == 8 && hdr.sign == 0) {
		fSampleFormat = media_raw_audio_format::B_AUDIO_UCHAR;
		fChannelCount = 1;
		fFrameSize    = 1;
	} else if (hdr.mono == 0 && hdr.rez == 8 && hdr.sign == 0xffff) {
		fSampleFormat = media_raw_audio_format::B_AUDIO_CHAR;
		fChannelCount = 1;
		fFrameSize    = 1;
	} else if (hdr.mono == 0 && hdr.rez == 16 && hdr.sign == 0) {
		fSampleFormat = media_raw_audio_format::B_AUDIO_SHORT; /* XXXdbg flip sign */
		fChannelCount = 1;
		fFrameSize    = 2;
	} else if (hdr.mono == 0 && hdr.rez == 16 && hdr.sign == 0xffff) {
		fSampleFormat = media_raw_audio_format::B_AUDIO_SHORT;
		fChannelCount = 1;
		fFrameSize    = 2;
	} else if (hdr.mono == 0xffff && hdr.rez == 8 && hdr.sign == 0) {
		fSampleFormat = media_raw_audio_format::B_AUDIO_UCHAR;
		fChannelCount = 2;
		fFrameSize    = 1;
	} else if (hdr.mono == 0xffff && hdr.rez == 8 && hdr.sign == 0xffff) {
		fSampleFormat = media_raw_audio_format::B_AUDIO_CHAR;
		fChannelCount = 2;
		fFrameSize    = 1;
	} else if (hdr.mono == 0xffff && hdr.rez == 16 && hdr.sign == 0) {
		fSampleFormat = media_raw_audio_format::B_AUDIO_SHORT; /* XXXdbg flip sign */
		fChannelCount = 2;
		fFrameSize    = 2;
	} else if (hdr.mono == 0xffff && hdr.rez == 16 && hdr.sign == 0xffff) {
		fSampleFormat = media_raw_audio_format::B_AUDIO_SHORT;
		fChannelCount = 2;
		fFrameSize    = 2;
    } else {
		printf("AVR: unknown format... (mono 0x%x rez 0x%x sign 0x%x)\n",
			   hdr.mono, hdr.rez, hdr.sign);
		return B_BAD_TYPE;
	}

	fRate = hdr.rate;
	if ((fRate >= 11022.0) && (fRate <= 11028.0))
		fRate = 11025.0;
	else if ((fRate >= 22044.0) && (fRate <= 22056.0))
		fRate = 22050.0;
	else if ((fRate >= 44088.0) && (fRate <= 44112.0))
		fRate = 44100.0;

	fFrameCount = hdr.size;

	fDataOffset = AVR_HDR_SIZE;
	fDataLength = fFrameCount * fFrameSize * fChannelCount;

	fFramePerBloc = (int32)(fRate*(float)AVR_BUFFER_LENGTH*1e-6);
	fBlocSize = fFramePerBloc*fFrameSize;

	uint32 bs = MINIMAL_BLOC_LENGTH;
	while (bs < fBlocSize) {
		bs <<= 1;
	}
	fBlocSize = bs;
	fFramePerBloc = bs/fFrameSize;

	fDataEnd = fDataOffset+fDataLength;
	fTimeFactor = 1e6/((float)fFrameSize*fRate);

	return B_OK;
}


/* Initialisation/Setup call before taking other the data extraction */
status_t AVRExtractor::Sniff(int32 *out_streamNum, int32 *out_chunkSize)
{
	status_t		err;

	err = ReadHeader();
	if (err != B_OK)
		return err;

	*out_streamNum = 1;
	*out_chunkSize = AVR_CHUNK_SIZE;

	return B_OK;
}



/* Provide information about output format */
status_t AVRExtractor::TrackInfo(int32 in_stream, media_format *out_format,
								 void **out_info, int32 *out_size)
{
	/* fills the media_format descriptor */
	status_t					err;
	media_format_description	formatDescription;
	BMediaFormats				formatObject;

	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* fake encoded signed 8 bits format */
	if (fSampleFormat == 0x1) {	
		memset(out_format, 0, sizeof(media_format));
		memset(&formatDescription, 0, sizeof(media_format_description));

#if 0
		formatDescription.family = B_AIFF_FORMAT_FAMILY;
		formatDescription.u.aiff.codec = 'sgn8';
#else
		formatDescription.family = B_AVR_FORMAT_FAMILY;
		formatDescription.u.avr.id = 'sgn8';
#endif

		formatObject.Lock();
		err = formatObject.GetFormatFor(formatDescription, out_format);
		formatObject.Unlock();	
		if (err != B_OK)
			return err;

		out_format->type = B_MEDIA_ENCODED_AUDIO;
		out_format->u.encoded_audio.output.channel_count = fChannelCount;
		out_format->u.encoded_audio.output.buffer_size = fBlocSize;
		out_format->u.encoded_audio.output.frame_rate = fRate;
		out_format->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_UCHAR;
		out_format->u.encoded_audio.output.byte_order =
			B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
		out_format->u.encoded_audio.bit_rate = 8;
		out_format->u.encoded_audio.frame_size = fBlocSize;
	}
	else {
		out_format->type = B_MEDIA_RAW_AUDIO;
		out_format->u.raw_audio.frame_rate = fRate;
		out_format->u.raw_audio.channel_count = fChannelCount;
		out_format->u.raw_audio.format = fSampleFormat;
		out_format->u.raw_audio.byte_order = B_MEDIA_BIG_ENDIAN;
		out_format->u.raw_audio.buffer_size = fBlocSize;
	}
	return B_OK;
}

status_t AVRExtractor::CountFrames(int32 in_stream, int64 *out_frames)
{
	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* return the frame count */
	*out_frames = (int64)fFrameCount;
	return B_OK;
}

status_t AVRExtractor::GetDuration(int32 in_stream, bigtime_t *out_duration)
{
	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* return the duration */
	*out_duration = (bigtime_t)(((double)fFrameCount*1000000.0)/fRate);
	return B_OK;
}

status_t 
AVRExtractor::AllocateCookie(int32 in_stream, void **cookieptr)
{
	*cookieptr = malloc(sizeof(uint32));
	if(*cookieptr == NULL)
		return B_NO_MEMORY;
	*((uint32*)*cookieptr) = fDataOffset;
	return B_NO_ERROR;
}

status_t 
AVRExtractor::FreeCookie(int32 in_stream, void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

status_t AVRExtractor::SplitNext(	int32	in_stream,
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
	if (local_end > AVR_CHUNK_SIZE) {
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
status_t AVRExtractor::Seek(int32		in_stream,
							 void		*cookie,
							 int32		in_towhat,
							 int32		flags,
							 bigtime_t	*inout_time,
							 int64		*inout_frame,
							 off_t		*inout_filePos,
							 char		*in_packetPointer,
							 int32		*inout_packetLength,
							 bool		*out_done)
{
	int32			frame_index;
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
		*read_from_ptr = fDataOffset+frame_index*fFrameSize;
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
