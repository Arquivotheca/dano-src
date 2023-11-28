#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <MediaFormats.h>
#include "MediaWriter.h"
#include "aiff.h"

using namespace BPrivate;

//#define DEBUG	printf
#define DEBUG	if (0) printf

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
	int16		padding;
	int16		channel_count;
	int32		frame_count;
	int16		sample_size;
	int16		srate_exponent;
	int32		srate_mantissa_0;
	int32		srate_mantissa_1;
} _aifc_comm_chunk;

typedef struct {
	int32		ckID;
	int32		ckSize;
	int32		offset;
	int32		blockSize;
} _aiff_sound_data_chunk;

/* Private enums */
enum {
	AIFF_CHUNK_SIZE		= 65536,
	MINIMAL_BLOC_LENGTH	= 1024,
//	AIFF_BUFFER_LENGTH	= 40000	// in us
	AIFF_BUFFER_LENGTH	= 60000	// in us
};

enum {
	U8TO8,
	L16TOB16,
	B16TOB16,
	LFLOATTOB32,
	BFLOATTOB32,
	L32TOB32,
	B32TOB32
};

const double MAX_VALUE = 1024.0*1024.0*1024.0*2.0-1.0;

enum {
	OUTPUT_BUFFER_SIZE = 2048
};

/* Code */
MediaWriter *instantiate_mediawriter(void) {
	return new AIFFWriter();
}

status_t get_mediawriter_info(media_file_format *mfi) {
	strcpy(mfi->mime_type,      "audio/x-aiff");
	strcpy(mfi->pretty_name,    "Audio IFF format (AIFF)");
	strcpy(mfi->short_name,     "AIFF");
	strcpy(mfi->file_extension, "aiff");

	mfi->capabilities = media_file_format::B_KNOWS_RAW_AUDIO |
		                media_file_format::B_WRITABLE;
	mfi->family       = B_AIFF_FORMAT_FAMILY;

	return B_OK;
}

status_t accepts_format(media_format *fmt, uint32 flags) {
	int reject_wildcards = flags & B_MEDIA_REJECT_WILDCARDS;

	if (fmt->type == B_MEDIA_ENCODED_AUDIO) {
		// we don't support AIFC for now
		return B_BAD_TYPE;
	}
	else if (fmt->type == B_MEDIA_RAW_AUDIO) {
		switch(fmt->u.raw_audio.format) {
		case 0: // wildcard
			if(reject_wildcards)
				return B_BAD_TYPE;
		case media_raw_audio_format::B_AUDIO_UCHAR:
		case media_raw_audio_format::B_AUDIO_SHORT:
		case media_raw_audio_format::B_AUDIO_FLOAT:
		case media_raw_audio_format::B_AUDIO_INT:
			break;

		default:
			return B_BAD_TYPE;
		}

		if(fmt->u.raw_audio.channel_count < 1)
			if(reject_wildcards ||
				fmt->u.raw_audio.channel_count != media_raw_audio_format::wildcard.channel_count)
			{
				return B_BAD_TYPE;
			}

		if(fmt->u.raw_audio.frame_rate < 0.01)
			if(reject_wildcards ||
				fmt->u.raw_audio.frame_rate != media_raw_audio_format::wildcard.frame_rate)
			{
				return B_BAD_TYPE;
			}
		
		return B_OK;
	}

	return B_BAD_TYPE;
}


AIFFWriter::AIFFWriter() {
	fHeader.writer = NULL;
	fHeader.copyright = NULL;
	fHeaderCommitted = false;
	fTrack = NULL;
}

AIFFWriter::~AIFFWriter() {
	if (fHeader.copyright != NULL)
		free(fHeader.copyright);
	if (fHeader.writer) {
		delete fHeader.writer;
		fHeader.writer = NULL;
	}
}


status_t AIFFWriter::SetSource(BDataIO *source) {
	int			fd;	
	
	if(fHeader.writer)
		return B_NOT_ALLOWED;

	status_t err;
	BPositionIO* posio = dynamic_cast<BPositionIO*>(source);
	if (posio == NULL)
		return B_BAD_TYPE;

DEBUG("#### SetRef In\n");

	fHeader.writer = new FileWriter(posio);	
	err = fHeader.writer->InitCheck();
	if(err < B_OK) {
		delete fHeader.writer;
		fHeader.writer = 0;
		return err;
	}
	
DEBUG("#### SetRef Out\n");

	return B_OK;
}
	
status_t AIFFWriter::AddTrack(BMediaTrack *track) {
	status_t					err;
	media_format				mf;
	BMediaFormats				formatObject;
	media_format_description	fd;

	/* Check if we didn't alreday add a audio track */
	if (fTrack != NULL)
		return B_BAD_INDEX;

	track->EncodedFormat(&mf);
	if (mf.u.raw_audio.byte_order == 0) {
		mf.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	}
	err = accepts_format(&mf, B_MEDIA_REJECT_WILDCARDS);
	if(err != B_OK) {
		return err;
	}

	if (mf.type == B_MEDIA_ENCODED_AUDIO) {
		/* Check if it's an encoded format that we know how to support */
		return B_BAD_TYPE;
	}
	else if (mf.type == B_MEDIA_RAW_AUDIO) {
		/* If it's an raw format, get all the info we will need to do the
		   conversion into the equivalent WAV format */
		fHeader.rate = mf.u.raw_audio.frame_rate;
		fHeader.channel_count = mf.u.raw_audio.channel_count;

		switch (mf.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_UCHAR :
			fHeader.bit_per_sample = 8;
			fHeader.mode = U8TO8;
			break;
		case media_raw_audio_format::B_AUDIO_SHORT :
			fHeader.bit_per_sample = 16;
			if (mf.u.raw_audio.byte_order == B_MEDIA_LITTLE_ENDIAN)
				fHeader.mode = L16TOB16;
			else
				fHeader.mode = B16TOB16;
			break;
		case media_raw_audio_format::B_AUDIO_FLOAT :
			fHeader.bit_per_sample = 32;
			if (mf.u.raw_audio.byte_order == B_MEDIA_LITTLE_ENDIAN)
				fHeader.mode = LFLOATTOB32;
			else
				fHeader.mode = BFLOATTOB32;
			break;
		case media_raw_audio_format::B_AUDIO_INT :
			fHeader.bit_per_sample = 32;
			if (mf.u.raw_audio.byte_order == B_MEDIA_LITTLE_ENDIAN)
				fHeader.mode = L32TOB32;
			else
				fHeader.mode = B32TOB32;
			break;
		default:
			return B_ERROR;
		}
	
		fHeader.frame_count = (fHeader.channel_count*fHeader.bit_per_sample)>>3;

		fTrack = track;
		return B_OK;
	}
	/* You can't add anything else than an audio track */
	return B_BAD_TYPE;
}

status_t AIFFWriter::AddChunk(int32 type, const char *data, size_t size) {
	return B_ERROR;
}

status_t AIFFWriter::AddCopyright(const char *data) {
	/* Accept only one copyright */
	if (fHeader.copyright != NULL)
		return B_ERROR;
	/* Duplicate the copyright */
	fHeader.copyright = strdup(data);
	if (fHeader.copyright == NULL)
		return B_ERROR;
	return B_OK;
}

status_t AIFFWriter::AddTrackInfo(int32 track, uint32 code, const char *data, size_t size) {
	return B_ERROR;
}

status_t AIFFWriter::CommitHeader(void) {
	int16					exponent;
	uint16					sample_size;
	uint32					mantissa0, mantissa1;
	_aifc_comm_chunk		comm;
	_aiff_chunk_header		chunk_head;
	_aiff_format_chunk		form;
	_aiff_sound_data_chunk	ssnd;

	status_t err;

DEBUG("#### CommitHeader In\n");
	/* Do we really have a track ? */
	if (fTrack == NULL) {
DEBUG("#### CommitHeader Out 0\n");
		return B_ERROR;
	}
	if (fHeader.writer == NULL) {
DEBUG("#### CommitHeader Out 1\n");
		return B_ERROR;
	}

	/* Get the magic signature ready */
	form.magic = B_HOST_TO_BENDIAN_INT32(_AIFF_MAGIC);
	form.data_size = -1L;	//	Will get fixed up later
	form.file_type = B_HOST_TO_BENDIAN_INT32(_AIFF_TYPE);
	fHeader.offset_total_length = fHeader.writer->Position() + 4;
	fHeader.total_length = 0;
	err = fHeader.writer->Write(&form, sizeof(form));
	if(err < B_OK) {
DEBUG("#### CommitHeader Out 2\n");
		return err;
	}

	/* Write the COMM chunk header */
	chunk_head.magic = B_HOST_TO_BENDIAN_INT32(_AIFF_COMMON);
	chunk_head.zappo = B_HOST_TO_BENDIAN_INT32(18);
	fHeader.total_length += sizeof(chunk_head);
	err = fHeader.writer->Write(&chunk_head, sizeof(chunk_head));
	if(err < B_OK) {
DEBUG("#### CommitHeader Out 3\n");
		return err;
	}

	/* Write the COMM chunk */
	ConvertFloatTo80Bits(fHeader.rate, &exponent, &mantissa0, &mantissa1);
	comm.channel_count = B_HOST_TO_BENDIAN_INT16(fHeader.channel_count); 
	comm.frame_count = -1L;	//	Will get fixed up later
	comm.sample_size = B_HOST_TO_BENDIAN_INT16(fHeader.bit_per_sample);
	comm.srate_exponent = B_HOST_TO_BENDIAN_INT16(exponent);
	comm.srate_mantissa_0 = B_HOST_TO_BENDIAN_INT32(mantissa0);
	comm.srate_mantissa_1 = B_HOST_TO_BENDIAN_INT32(mantissa1);
	fHeader.offset_frame_count = fHeader.writer->Position() + 2;
	fHeader.total_length += sizeof(comm)-sizeof(comm.padding);
	err = fHeader.writer->Write(&comm.channel_count, sizeof(comm)-sizeof(comm.padding));
	if(err < B_OK) {
DEBUG("#### CommitHeader Out 4\n");
		return err;
	}

	/* Write the SSND chunk header */
	ssnd.ckID = B_HOST_TO_BENDIAN_INT32(_AIFF_DATA);
	ssnd.ckSize = sizeof(ssnd);
	ssnd.offset = 0;
	ssnd.blockSize = 0;
	
	fHeader.offset_data_length = fHeader.writer->Position() + 4;
	fHeader.data_length = 0;
	fHeader.total_length = sizeof(ssnd);
	err = fHeader.writer->Write(&ssnd, sizeof(ssnd));
	if(err < B_OK) {
		DEBUG("#### CommitHeader Out 5\n");
		return err;
	}

	fHeaderCommitted = true;

DEBUG("#### CommitHeader Out 6\n");
	return B_OK;
}
	
void AIFFWriter::ConvertFloatTo80Bits(float f, int16 *exp, uint32 *m0, uint32 *m1) {
	int16		exp0;

	// Don't handle negative or smaller than 1.0
	if (f <= 1.0) {
		*exp = 0;
		*m0 = 0;
		*m1 = 0;
		return;
	}
	// For the other ones.
	exp0 = 0;
	while (f >= 2.0) {
		exp0++;
		f *= 0.5;
	}
	*exp = exp0+16383;
	*m0 = (uint32)(f*(16.0*1024.0*1024.0))<<7;
	*m1 = 0;
}	
	
status_t AIFFWriter::WriteData(	int32 				tracknum,
							 	media_type 			type,
					 			const void 			*data,
								size_t 				size,
					 			media_encode_info	*info) {
	char		tmp_buffer[OUTPUT_BUFFER_SIZE];
	char		*dest;
	uint32		i, buf_size, count4, count2;
	const char	*data_ptr = (const char *)data;

	status_t err;
	if (tracknum != 0)
		return B_BAD_INDEX;
	
	if(!fHeaderCommitted) {
		return B_NOT_ALLOWED;
	}

DEBUG("#### WriteData In\n");
	fHeader.data_length += size;

	switch (fHeader.mode) {
	case B16TOB16 :
	case B32TOB32 :
		err = fHeader.writer->Write(data_ptr, size);
		if(err < B_OK) {
			DEBUG("#### WriteData Out 0\n");
			return B_ERROR;
		}
		break;
	default :
		while (size > 0) {
			if (size > OUTPUT_BUFFER_SIZE)
				buf_size = OUTPUT_BUFFER_SIZE;
			else
				buf_size = size;
			dest = (char*)tmp_buffer;
			
			switch (fHeader.mode) {
			case U8TO8 :
				count4 = buf_size>>2;
				for (i=0; i<count4; i++)
					((uint32*)dest)[i] = ((uint32*)data_ptr)[i] ^ 0x80808080L;
				for (i=count4<<2; i<buf_size; i++)
					dest[i] = data_ptr[i] ^ 0x80;
				break;
			case L16TOB16 :
				count2 = buf_size>>1;
				for (i=0; i<count2; i++)
					((int16*)dest)[i] = B_SWAP_INT16(((int16*)data_ptr)[i]);
				break;
			case LFLOATTOB32 :
				count4 = buf_size>>2;
				for (i=0; i<count4; i++)
					((int32*)dest)[i] = B_HOST_TO_BENDIAN_INT32(
						(int32)(MAX_VALUE*B_LENDIAN_TO_HOST_FLOAT(((float*)data_ptr)[i])));
				break;
			case BFLOATTOB32 :
				count4 = buf_size>>2;
				for (i=0; i<count4; i++)
					((int32*)dest)[i] = B_HOST_TO_BENDIAN_INT32(
						(int32)(MAX_VALUE*B_BENDIAN_TO_HOST_FLOAT(((float*)data_ptr)[i])));
				break;
			case L32TOB32 :
				count4 = buf_size>>2;
				for (i=0; i<count4; i++)
					((int32*)dest)[i] = B_SWAP_INT32(((int32*)data_ptr)[i]);
				break;
			default :
				return B_BAD_TYPE;
			}
			err = fHeader.writer->Write(tmp_buffer, buf_size);
			if(err < B_OK)
				return err;
			data_ptr += buf_size;
			size -= buf_size;
		}
	}

DEBUG("#### WriteData Out 2\n");
	return B_OK;
}


status_t AIFFWriter::CloseFile(void) {
	uint32		length;

	if(!fHeaderCommitted) {
		return B_ERROR;
	}

DEBUG("#### CloseFile In\n");
	/* Go back and patch the data length */
	length = B_HOST_TO_BENDIAN_INT32(fHeader.data_length);
	if (fHeader.writer->Seek(fHeader.offset_data_length, SEEK_SET) < B_OK)
		return B_ERROR;
	if (fHeader.writer->Write(&length, sizeof(length)) < B_OK)
		return B_ERROR;
	/* Go hack the frame count */
	length = B_HOST_TO_BENDIAN_INT32(fHeader.data_length/fHeader.frame_count-2);
	if (fHeader.writer->Seek(fHeader.offset_frame_count, SEEK_SET) < B_OK)
		return B_ERROR;
	if (fHeader.writer->Write(&length, sizeof(length)) < B_OK)
		return B_ERROR;
	/* Go back and patch the total length */
	length = B_HOST_TO_BENDIAN_INT32(fHeader.total_length+fHeader.data_length);
	if (fHeader.writer->Seek(fHeader.offset_total_length, SEEK_SET) < B_OK)
		return B_ERROR;
	if (fHeader.writer->Write(&length, sizeof(length)) < B_OK)
		return B_ERROR;
	if (fHeader.writer->Flush() < B_OK)
		return B_ERROR;
	/* Just close the file... */
	delete fHeader.writer;
	fHeader.writer = NULL;

DEBUG("#### CloseFile Out\n");
	return B_OK;
}



