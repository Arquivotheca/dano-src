#include <stdio.h>
#include <math.h>
#include <storage2/File.h>
#include <support2/ByteOrder.h>
#include <media2/MediaFormats.h>

#include <support2/StdIO.h>

#include "AIFFExtractor.h"

#define checkpoint \
//berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Media2 {

#if DEBUG
#define PRINTF printf
#else
#define PRINTF (void)
#endif

/* AIFF tags */
#define _AIFF_MAGIC		'FORM'
#define _AIFF_TYPE		'AIFF'
#define _AIFC_TYPE		'AIFC'
#define _8SVX_TYPE		'8SVX'

#define _AIFC_VERSION	'FVER'
#define _AIFF_COMMON	'COMM'
#define _AIFF_DATA		'SSND'
#define _IFF_VERSION	'VHDR'
#define _IFF_CHANNEL	'CHAN'
#define _IFF_DATA		'BODY'

/* AIFF file format structs */
typedef struct {
	int32		magic;
	int32		zappo;
} _aiff_chunk_header;

typedef struct {
	int32		magic;
	int32		data_size;
	int32		file_type;
} _aiff_format_chunk;

typedef struct {
	int32		timestamp;
} _aifc_version_chunk;

typedef struct {	
	int32		compression_type;
	char		*compression_name;
} _aiff_compression_chunk;

typedef struct {
	int32		offset;
	int32		block_size;
} _aiff_data_chunk;

enum {
	AIFF_CHUNK_SIZE		= 65536,
	MINIMAL_BLOC_LENGTH	= 1024,
//	AIFF_BUFFER_LENGTH	= 40000	// in us
	AIFF_BUFFER_LENGTH	= 60000	// in us
};


extern "C" const char *mime_types_extractor[] = { "audio/aiff", "audio/x-aiff", NULL };


extern "C" B::Private::Extractor *instantiate_extractor(void)
{
	return new AIFFExtractor();
}


/* code */
AIFFExtractor::AIFFExtractor() {

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

AIFFExtractor::~AIFFExtractor() {
}

status_t AIFFExtractor::GetFileFormatInfo(media_file_format *mfi) {
	strcpy(mfi->mime_type,       "audio/x-aiff");
	strcpy(mfi->pretty_name,    "AIFF Sound File");
	strcpy(mfi->short_name,     "aiff");
	strcpy(mfi->file_extension, "aiff");

	mfi->family = B_AIFF_FORMAT_FAMILY;

	return B_OK;
}

/* Initialisation/Setup call before taking other the data extraction */
status_t AIFFExtractor::Sniff(int32 *out_streamNum, int32 *out_chunkSize) {
	status_t				err;
	_aiff_format_chunk		form;

	IByteInput::ptr in = SourceStream();
	IByteSeekable::ptr seek = SourceSeekable();
	if (!in.ptr() || !seek.ptr()) return B_IO_ERROR;
	
	/* Check the magic signature for AIFF */
	seek->Seek(0, SEEK_SET);	
	if (in->Read(&form, sizeof(form)) != sizeof(form))
		return B_IO_ERROR;
	if (B_BENDIAN_TO_HOST_INT32(form.magic) != _AIFF_MAGIC)
		return B_BAD_TYPE;

	/* Check the file type */
	switch (B_BENDIAN_TO_HOST_INT32(form.file_type)) {
	case _AIFF_TYPE :
		/* Standard non-compressed AIFF file */
		err = ReadHeader();
		if (err != B_OK)
			return err;
		*out_streamNum = 1;
		*out_chunkSize = AIFF_CHUNK_SIZE;
		return B_OK;
	case _AIFC_TYPE :
		/* Compressed file - not supported for now */
		return B_BAD_TYPE;
	case _8SVX_TYPE :
		/* Standard non-compressed IFF file */
		fChannelCount = 1;
		err = ReadHeader();
		if (err != B_OK)
			return err;
		*out_streamNum = 1;
		*out_chunkSize = AIFF_CHUNK_SIZE;
		return B_OK;
	default :
		/* Unknown type - not supported */
		return B_BAD_TYPE;
	}
}

/* Read header and extract useful information */
status_t AIFFExtractor::ReadHeader() {
	int8					codec;
	int16					rate;
	int32					skip, pos, offset, lastpos, length, channels, blockSize;
	status_t				err;
	_aiff_chunk_header		chunk_head;

	IByteInput::ptr in = SourceStream();
	IByteSeekable::ptr seek = SourceSeekable();
	if (!in.ptr() || !seek.ptr()) return B_IO_ERROR;
	
	while (in->Read(&chunk_head, sizeof(chunk_head)) == sizeof(chunk_head)) {	
PRINTF("Header 0x%08x [%c%c%c%c]\n", B_BENDIAN_TO_HOST_INT32(chunk_head.magic),
	  				((char*)&chunk_head.magic)[0], ((char*)&chunk_head.magic)[1],
	  				((char*)&chunk_head.magic)[2], ((char*)&chunk_head.magic)[3]);
		switch (B_BENDIAN_TO_HOST_INT32(chunk_head.magic)) {
		case _AIFF_COMMON :
			err = ReadCommon();
			if (err != B_OK)
				return err;
			break;
		case _AIFC_VERSION :
			err = ReadVersion();
			if (err != B_OK)
				return err;
			break;
		case _AIFF_DATA:
		case _IFF_DATA :
			if (in->Read(&offset, 4) != 4)
				return B_IO_ERROR;
			if (in->Read(&blockSize, 4) != 4)
				return B_IO_ERROR;
			fDataLength = B_BENDIAN_TO_HOST_INT32(chunk_head.zappo)-8;
			lastpos = seek->Position();
			
			fDataOffset = lastpos+B_BENDIAN_TO_HOST_INT32(offset);
			pos = seek->Seek(0, SEEK_END);
			length = pos-fDataOffset;
			if( length < 0 )
			{
				fDataOffset = lastpos;
				length = pos-lastpos;
			}
			if (length < (int32)fDataLength)
				fDataLength = length;
			break;
		case _IFF_VERSION :
			if (seek->Seek(12, SEEK_CUR) < 0)
				return B_IO_ERROR;
			if (in->Read(&rate, 2) != 2)
				return B_IO_ERROR;
			if (seek->Seek(1, SEEK_CUR) < 0)
				return B_IO_ERROR;
			if (in->Read(&codec, 1) != 1)
				return B_IO_ERROR;
			if (seek->Seek(4, SEEK_CUR) < 0)
				return B_IO_ERROR;
			fSampleFormat = 0x1;
			if (codec != 0)
				return B_BAD_TYPE;
			fRate = B_BENDIAN_TO_HOST_INT16(rate);
			break;
		case _IFF_CHANNEL :
			if (in->Read(&channels, 4) != 4)
				return B_IO_ERROR;
			fChannelCount = B_BENDIAN_TO_HOST_INT32(channels);
			fChannelCount = (fChannelCount & 1) +
							((fChannelCount & 2)>>1) +
							((fChannelCount & 4)>>2) +
							((fChannelCount & 8)>>3);
			break;
		default:
			skip = B_BENDIAN_TO_HOST_INT32(chunk_head.zappo);
			if (seek->Seek(skip, SEEK_CUR) < 0)
				return B_IO_ERROR;
			break;
		}
	}
	
	/* Complete initialisation of internal state */
	switch (fSampleFormat) {
	case 0x1 :
		fFrameSize = fChannelCount;
		break;
	case media_raw_audio_format::B_AUDIO_SHORT :
		fFrameSize = fChannelCount*2;
		break;
	case media_raw_audio_format::B_AUDIO_INT :
		fFrameSize = fChannelCount*4;
		break;
	}
	fFramePerBloc = (int32)(fRate*(float)AIFF_BUFFER_LENGTH*1e-6);
	fBlocSize = fFramePerBloc*fFrameSize;

	uint32 bs = MINIMAL_BLOC_LENGTH;
	while (bs < fBlocSize) {
		bs <<= 1;
	}
	fBlocSize = bs;
	fFramePerBloc = bs/fFrameSize;

	// calculate frame count; round up to include truncated final frame
	fFrameCount = fDataLength/fFrameSize;
	if(fDataLength % fFrameSize)
		fFrameCount++;
	fDataEnd = fDataOffset+fDataLength;
	fTimeFactor = 1e6/((float)fFrameSize*fRate);

	return B_OK;
}

/* Read "common" header */
inline double ul2d(uint32 ul) {
	return (ul & (uint32)0x80000000L)?(2147483648.0 + (ul & 0x7FFFFFFFL)):ul;
}

static float ReckonRate(int16 e, uint32 m0, uint32 m1) {
	bool			is_neg = (e < 0);
	double			srate, dm0, dm1;

  	if (is_neg)
  		e &= 0x7FFF;
	dm0 = ul2d(m0);
	dm1 = ul2d(m1);

	if (!is_neg && (m0 == 0) && (m1 == 0) && (e == 0))
		return 0;

	srate = dm0 * pow(2.0, -31.0);
	srate += dm1 * pow(2.0, -63.0);
	srate *= pow(2.0, (double) (e - 16383));
	srate *= is_neg ? -1.0 : 1.0;
	return (float)((int32) floor(srate));
}

status_t AIFFExtractor::ReadCommon() {
	int16		channel_count, sample_size, srate_exponent;
	int32		frame_count;
	uint32		srate_mantissa_0, srate_mantissa_1;	

	IByteInput::ptr in = SourceStream();
	IByteSeekable::ptr seek = SourceSeekable();
	if (!in.ptr() || !seek.ptr()) return B_IO_ERROR;
	
	// Read by small fragment to avoid any struct padding issue.
	if (in->Read(&channel_count, 2) != 2)
		return B_IO_ERROR;
	if (in->Read(&frame_count, 4) != 4)
		return B_IO_ERROR;
	if (in->Read(&sample_size, 2) != 2)
		return B_IO_ERROR;
	if (in->Read(&srate_exponent, 2) != 2)
		return B_IO_ERROR;
	if (in->Read(&srate_mantissa_0, 4) != 4)
		return B_IO_ERROR;
	if (in->Read(&srate_mantissa_1, 4) != 4)
		return B_IO_ERROR;

	switch (B_BENDIAN_TO_HOST_INT16(sample_size)) {
	case 8 :
		fSampleFormat = 0x1; // for the undefined B_AUDIO_CHAR
		break;
	case 16 :
		fSampleFormat = media_raw_audio_format::B_AUDIO_SHORT;
		break;
	case 32 :
		fSampleFormat = media_raw_audio_format::B_AUDIO_INT;
		break;
	default:
		return B_BAD_TYPE;
	}
	fChannelCount = B_BENDIAN_TO_HOST_INT16(channel_count);
	if ((fChannelCount != 1) && (fChannelCount != 2))
		return B_BAD_VALUE;
	fFrameCount = B_BENDIAN_TO_HOST_INT32(frame_count);
	fRate = ReckonRate(B_BENDIAN_TO_HOST_INT16(srate_exponent),
					   B_BENDIAN_TO_HOST_INT32(srate_mantissa_0),
					   B_BENDIAN_TO_HOST_INT32(srate_mantissa_1));
	if ((fRate >= 11022.0) && (fRate <= 11028.0))
		fRate = 11025.0;
	else if ((fRate >= 22044.0) && (fRate <= 22056.0))
		fRate = 22050.0;
	else if ((fRate >= 44088.0) && (fRate <= 44112.0))
		fRate = 44100.0;
	return B_OK;
}

/* Read "version" header */
status_t AIFFExtractor::ReadVersion() {
	return B_OK;
}

/* Provide information about output format */
status_t AIFFExtractor::TrackInfo(int32 in_stream, media_format *out_format, void **, int32 *) {
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
		formatDescription.family = B_AIFF_FORMAT_FAMILY;
		formatDescription.u.aiff.codec = 'sgn8';

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

status_t AIFFExtractor::CountFrames(int32 in_stream, int64 *out_frames) {
	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* return the frame count */
	*out_frames = (int64)fFrameCount;
	return B_OK;
}

status_t AIFFExtractor::GetDuration(int32 in_stream, bigtime_t *out_duration) {
	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* return the duration */
	*out_duration = (bigtime_t)(((double)fFrameCount*1000000.0)/fRate);
	return B_OK;
}

status_t 
AIFFExtractor::AllocateCookie(int32, void **cookieptr)
{
	*cookieptr = malloc(sizeof(uint32));
	if(*cookieptr == NULL)
		return B_NO_MEMORY;
	*((uint32*)*cookieptr) = fDataOffset;
	return B_NO_ERROR;
}

status_t 
AIFFExtractor::FreeCookie(int32, void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

status_t AIFFExtractor::SplitNext(	int32	in_stream,
									void	*cookie,
									off_t	*inout_filepos,
								  	char	*in_packetPointer,
								  	int32	*inout_packetLength,
								  	char	**out_bufferStart,
								  	int32	*out_bufferLength,
									media_header *mh) {
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
	if (local_end > AIFF_CHUNK_SIZE) {
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
status_t AIFFExtractor::Seek(int32		in_stream,
							 void		*cookie,
							 int32		in_towhat,
							 int32,
							 bigtime_t	*inout_time,
							 int64		*inout_frame,
							 off_t		*inout_filePos,
							 char		*,
							 int32		*inout_packetLength,
							 bool		*out_done) {
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
		else if (frame_index > (int32)fFrameCount)
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

} } // B::Media2
