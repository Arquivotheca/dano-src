#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <FileWriter.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <MediaFormats.h>
#include "MediaWriter.h"
#include "wav.h"

//#define DEBUG printf
#define DEBUG if (0) printf

/* Private struct */
typedef struct {
	int32	riff_magic;
	int32	chunk_size;
	int32	wav_magic;
} _wav_sound_header;

typedef struct {
	int32	magic;
	int32	size;
} _riff_chunk;

typedef struct {
	int16	format;
	int16	channel_count;
	int32	sampling_rate;
	int32	average_rate; // unused
	int16	alignment;
	int16	sample_size;
} _wav_format_chunk;

typedef struct {
	uint32	one;
	uint16	two;
	uint16	three;
	uchar	four[8];
} WAV_GUID;

typedef struct {
	uint16 cb_size;
	union {
		uint16 valid_bits;
		uint16 samples_per_block;
	};
	uint32 channel_mask;
	WAV_GUID guid;
} _wav_format_extensible;

static WAV_GUID wav_format_pcm_ex = { 0x1, 0x0, 0x10, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static WAV_GUID wav_format_ieee_ex = { 0x3, 0x0, 0x10, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static WAV_GUID wav_format_ambisonic_pcm = { 0x1, 0x721, 0x11d3, { 0x86, 0x44, 0xc8, 0xc1, 0xca, 0x00, 0x00, 0x00 } };
static WAV_GUID wav_format_ambisonic_ieee = { 0x3, 0x721, 0x11d3, { 0x86, 0x44, 0xc8, 0xc1, 0xca, 0x00, 0x00, 0x00 } };

/* WAV signatures */
#define _RIFF_MAGIC	'RIFF'
#define _WAVE_MAGIC	'WAVE'
#define _RIFF_FMT	'fmt '
#define _RIFF_DATA	'data'

#define WAVE_MS_PCM 	0x0001
#define WAVE_MS_ADPCM 	0x0002
#define WAVE_MS_EXTENSIBLE	0xfffe
/*
#define WAVE_IBM_MULAW	0x0101
#define WAVE_IBM_ALAW	0x0102
#define WAVE_IBM_ADPCM	0x0103
*/

/* Private enums */
enum {
	S8TOU8,
	U8TOU8,
	L16TOL16,
	B16TOL16,
	LFLOATTOL32,
	BFLOATTOL32,
	L32TOL32,
	B32TOL32,
	ENCODED
};

const double MAX_VALUE = 1024.0*1024.0*1024.0*2.0-1.0;

enum {
	OUTPUT_BUFFER_SIZE = 2048
};

/* Code */
MediaWriter *instantiate_mediawriter(void) {
	return new WAVWriter();
}

status_t get_mediawriter_info(media_file_format *mfi) {
	strcpy(mfi->mime_type,       "audio/x-wav");
	strcpy(mfi->pretty_name,    "RIFF Audio File Format (WAV)");
	strcpy(mfi->short_name,     "wav");
	strcpy(mfi->file_extension, "wav");

	mfi->capabilities = media_file_format::B_KNOWS_RAW_AUDIO |
		                media_file_format::B_KNOWS_ENCODED_AUDIO |
		                media_file_format::B_WRITABLE;
	mfi->family       = B_WAV_FORMAT_FAMILY;

	return B_OK;
}

status_t accepts_format(media_format *fmt, uint32 flags) {
	status_t					err;
	BMediaFormats				formatObject;
	media_format_description	fd;

	if (fmt->type == B_MEDIA_ENCODED_AUDIO) {
		err = formatObject.GetCodeFor(*fmt, B_WAV_FORMAT_FAMILY, &fd);

		if (err != B_OK)
			return err;
		if (fd.family != B_WAV_FORMAT_FAMILY)
			return B_BAD_TYPE;
		return B_OK;
	}
	else if (fmt->type == B_MEDIA_RAW_AUDIO) {
		switch(fmt->u.raw_audio.format) {
		case 0: // wildcard
			if(flags & B_MEDIA_REJECT_WILDCARDS)
				return B_BAD_TYPE;
		case media_raw_audio_format::B_AUDIO_CHAR:
		case media_raw_audio_format::B_AUDIO_UCHAR:
		case media_raw_audio_format::B_AUDIO_SHORT:
		case media_raw_audio_format::B_AUDIO_FLOAT:
		case media_raw_audio_format::B_AUDIO_INT:
			return B_OK;
		
		default:
			return B_BAD_TYPE;
		}
	}
	return B_BAD_TYPE;
}


WAVWriter::WAVWriter() {
	fHeader.writer = NULL;
	fHeader.meta_data = NULL;
	fHeader.meta_data_length = 0;
	fHeader.copyright = NULL;
	fTrack = NULL;
	fHeader.channel_mask = 0;
	fHeader.valid_bits = 0;
	fHeader.matrix_mask = 0;
	fHeaderCommitted = false;
}

WAVWriter::~WAVWriter() {
	if (fHeader.meta_data != NULL)
		free(fHeader.meta_data);
	if (fHeader.copyright != NULL)
		free(fHeader.copyright);
	if (fHeader.writer) {
		delete fHeader.writer;
		fHeader.writer = NULL;
	}
}


status_t WAVWriter::SetSource(BDataIO *source) {
	int			fd;	

	if(fHeader.writer)
		return B_NOT_ALLOWED;

	BPositionIO* posio = dynamic_cast<BPositionIO*>(source);
	if (!posio)
		return B_BAD_TYPE;

DEBUG("#### SetRef In\n");

	fHeader.writer = new FileWriter(posio);
	status_t err = fHeader.writer->InitCheck();
	if(err < B_OK) {
		delete fHeader.writer;
		fHeader.writer = NULL;
	}
DEBUG("#### SetRef Out\n");
	fHeaderCommitted = false;
	return B_OK;
}
	
status_t WAVWriter::AddTrack(BMediaTrack *track) {
	status_t					err;
	media_format				mf;
	BMediaFormats				formatObject;
	media_format_description	fd;

	/* Check if we didn't alreday add a audio track */
DEBUG("#### AddTrack In\n");
	if (fTrack != NULL)
		return B_BAD_INDEX;
	if(fHeaderCommitted)
		return B_NOT_ALLOWED;
	track->EncodedFormat(&mf);
	/* Check if it's an encoded format that we know how to support */
	if (mf.type == B_MEDIA_ENCODED_AUDIO) {
		formatObject.Lock();
		err = formatObject.GetCodeFor(mf, B_WAV_FORMAT_FAMILY, &fd);
		formatObject.Unlock();

		if (err != B_OK) {
DEBUG("#### AddTrack Out 10\n");
			return err;
		}
		if (fd.family != B_WAV_FORMAT_FAMILY) {
DEBUG("#### AddTrack Out 11\n");
			return B_BAD_TYPE;
		}
			
		fHeader.codec = fd.u.wav.codec;
		fHeader.mode = ENCODED;
		fHeader.raw_format = mf.u.encoded_audio.output.format;
		fHeader.rate = (uint16)mf.u.encoded_audio.output.frame_rate;
		fHeader.channel_count = mf.u.encoded_audio.output.channel_count;
		fHeader.bit_per_sample = (uint16)mf.u.encoded_audio.bit_rate;
		fHeader.frame_size = mf.u.encoded_audio.frame_size;

		fTrack = track;
DEBUG("#### AddTrack Out 0\n");
		return B_OK;
	}
	/* If it's an raw format, get all the info we will need to do the
	   conversion into the equivalent WAV format */
	else if (mf.type == B_MEDIA_RAW_AUDIO) {
		if (mf.u.raw_audio.byte_order == 0) {
			mf.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
		}
		fHeader.codec = WAVE_MS_PCM;	// raw is encoded using WAVE_MS_PCM format
		fHeader.raw_format = mf.u.raw_audio.format;
		fHeader.rate = mf.u.raw_audio.frame_rate;
		fHeader.channel_count = mf.u.raw_audio.channel_count;
		fHeader.frame_size = mf.u.raw_audio.buffer_size;
		fHeader.channel_mask = mf.u.raw_audio.channel_mask;
		fHeader.valid_bits = mf.u.raw_audio.valid_bits;
		fHeader.matrix_mask = mf.u.raw_audio.matrix_mask;

		if (	(fHeader.channel_mask &&		//	assigned channels, and they're not L-R ?
					((fHeader.channel_count != 2) ||
						(fHeader.channel_mask != (B_CHANNEL_LEFT | B_CHANNEL_RIGHT))) ||
				fHeader.matrix_mask ||			//	assigned matrix format ?
				(fHeader.channel_count > 2) ||	//	greater than stereo ?
				(fHeader.valid_bits &&			//	not all bits valid ?
					(fHeader.valid_bits != (mf.u.raw_audio.format & 0xf)*8)))) {
			//	then we need MS_EXTENSIBLE (which doesn't work on W98SE)
			fHeader.codec = WAVE_MS_EXTENSIBLE;
DEBUG("Extensible: channel mask %d, valid bits %d, matrix mask %d\n",
		fHeader.channel_mask, fHeader.valid_bits, fHeader.matrix_mask);
		}

		switch (fHeader.raw_format) {
		case media_raw_audio_format::B_AUDIO_CHAR :
			fHeader.bit_per_sample = 8;
			fHeader.mode = S8TOU8;
			break;
		case media_raw_audio_format::B_AUDIO_UCHAR :
			fHeader.bit_per_sample = 8;
			fHeader.mode = U8TOU8;
			break;
		case media_raw_audio_format::B_AUDIO_SHORT :
			fHeader.bit_per_sample = 16;
			if (mf.u.raw_audio.byte_order == B_MEDIA_LITTLE_ENDIAN)
				fHeader.mode = L16TOL16;
			else
				fHeader.mode = B16TOL16;
			break;
		case media_raw_audio_format::B_AUDIO_FLOAT :
			fHeader.bit_per_sample = 32;
			if (mf.u.raw_audio.byte_order == B_MEDIA_LITTLE_ENDIAN)
				fHeader.mode = LFLOATTOL32;
			else
				fHeader.mode = BFLOATTOL32;
			break;
		case media_raw_audio_format::B_AUDIO_INT :
			fHeader.bit_per_sample = 32;
			if (mf.u.raw_audio.byte_order == B_MEDIA_LITTLE_ENDIAN)
				fHeader.mode = L32TOL32;
			else
				fHeader.mode = B32TOL32;
			break;
		}

		fTrack = track;
DEBUG("#### AddTrack Out 1\n");
		return B_OK;
	}
	/* You can't add anything else than an audio track */
DEBUG("#### AddTrack Out 2\n");
	return B_BAD_TYPE;
}

status_t WAVWriter::AddChunk(int32 type, const char *data, size_t size) {
	return B_ERROR;
}

status_t WAVWriter::AddCopyright(const char *data) {
	if(fHeaderCommitted)
		return B_NOT_ALLOWED;
	/* Accept only one copyright */
	if (fHeader.copyright != NULL)
		return B_ERROR;
	/* Duplicate the copyright */
	fHeader.copyright = strdup(data);
	if (fHeader.copyright == NULL)
		return B_ERROR;
	return B_OK;
}

status_t WAVWriter::AddTrackInfo(int32 track, uint32 code, const char *data, size_t size) {
	if(fHeaderCommitted)
		return B_NOT_ALLOWED;
	/* Accept only one block of meta-data */
	if (fHeader.meta_data != NULL)
		return B_ERROR;
	/* Allocate a buffer and copy the block */ 	
	fHeader.meta_data = (char*)malloc(size);
	if (fHeader.meta_data == NULL)
		return B_ERROR;
	fHeader.meta_data_length = size;
	memcpy(fHeader.meta_data, data, size);
	return B_OK;
}

status_t WAVWriter::CommitHeader(void) {
	uint16					sample_size;
	_riff_chunk				fmt_chunk_head, data_chunk_head;
	_wav_sound_header		riff_head;
	_wav_format_chunk		fmt_chunk;
	_wav_format_extensible	x_chunk;

DEBUG("#### CommitHeader In\n");
	/* Do we really have a track ? */
	if (fTrack == NULL) {
DEBUG("#### CommitHeader Out 0\n");
		return B_ERROR;
	}
	if (!fHeader.writer) {
DEBUG("#### CommitHeader Out 1\n");
		return B_ERROR;
	}
	if(fHeaderCommitted)
		return B_NOT_ALLOWED;

	/* Get the magic signature ready */
	riff_head.riff_magic = B_HOST_TO_BENDIAN_INT32(_RIFF_MAGIC);
	riff_head.wav_magic = B_HOST_TO_BENDIAN_INT32(_WAVE_MAGIC);
	fHeader.offset_total_length = fHeader.writer->Position() + 4;
	fHeader.total_length = 4;
	if (fHeader.writer->Write(&riff_head, sizeof(_wav_sound_header)) < B_OK) {
DEBUG("#### CommitHeader Out 2\n");
		return B_ERROR;
	}

	/* Write the format chunk */
	fmt_chunk_head.magic = B_HOST_TO_BENDIAN_INT32(_RIFF_FMT);
	if (fHeader.meta_data) {
		fmt_chunk_head.size = B_HOST_TO_LENDIAN_INT32(fHeader.meta_data_length);
	}
	else {
		if (fHeader.codec == WAVE_MS_PCM) {
			fmt_chunk_head.size = B_HOST_TO_LENDIAN_INT32(sizeof(fmt_chunk));
		}
		else {
			assert(fHeader.codec == WAVE_MS_EXTENSIBLE);
			fmt_chunk_head.size = B_HOST_TO_LENDIAN_INT32(sizeof(fmt_chunk)+sizeof(x_chunk));
		}
	}
	fHeader.total_length += sizeof(_riff_chunk);
	if (fHeader.writer->Write(&fmt_chunk_head, sizeof(_riff_chunk)) < B_OK) {
DEBUG("#### CommitHeader Out 3\n");
		return B_ERROR;
	}

	/* codec specific part */
	if (fHeader.meta_data) {
		fHeader.total_length += fHeader.meta_data_length;
		if (fHeader.writer->Write(fHeader.meta_data, fHeader.meta_data_length) < B_OK) {
DEBUG("#### CommitHeader Out 4\n");
			return B_ERROR;
		}
	}
	/* or default raw format part */
	else {
		fmt_chunk.format = B_HOST_TO_LENDIAN_INT16(fHeader.codec);
		fmt_chunk.channel_count = B_HOST_TO_LENDIAN_INT16(fHeader.channel_count);
		fmt_chunk.sampling_rate = B_HOST_TO_LENDIAN_INT32(fHeader.rate);
		fmt_chunk.average_rate =
			B_HOST_TO_LENDIAN_INT32((fHeader.rate*fHeader.bit_per_sample*fHeader.channel_count)/8);
		fmt_chunk.alignment = B_HOST_TO_LENDIAN_INT16((fHeader.bit_per_sample*fHeader.channel_count)/8);
		fmt_chunk.sample_size = B_HOST_TO_LENDIAN_INT16(fHeader.bit_per_sample);
		fHeader.total_length += sizeof(fmt_chunk);
		if (fHeader.writer->Write(&fmt_chunk, sizeof(fmt_chunk)) < B_OK) {
DEBUG("#### CommitHeader Out 5\n");
			return B_IO_ERROR;
		}
		if (fHeader.codec == WAVE_MS_EXTENSIBLE) {
DEBUG("EXTENSIBLE channels 0x%x, bits %d, matrix 0x%x\n", fHeader.channel_mask,
		fHeader.valid_bits, fHeader.matrix_mask);
			x_chunk.cb_size = B_HOST_TO_LENDIAN_INT16(22);
			if (!fHeader.valid_bits) fHeader.valid_bits = fHeader.bit_per_sample;
			x_chunk.valid_bits = B_HOST_TO_LENDIAN_INT16(fHeader.valid_bits);
			x_chunk.channel_mask = B_HOST_TO_LENDIAN_INT32(fHeader.channel_mask);
			if (fHeader.matrix_mask & B_MATRIX_AMBISONIC_WXYZ) {
				x_chunk.guid = wav_format_ambisonic_pcm;
			}
			else {
				x_chunk.guid = wav_format_pcm_ex;
			}
			x_chunk.guid.one = B_HOST_TO_LENDIAN_INT32(x_chunk.guid.one);
			x_chunk.guid.two = B_HOST_TO_LENDIAN_INT16(x_chunk.guid.two);
			x_chunk.guid.three = B_HOST_TO_LENDIAN_INT16(x_chunk.guid.three);
			if (fHeader.writer->Write(&x_chunk, sizeof(x_chunk)) < B_OK) {
				return B_IO_ERROR;
			}
		}
		fHeader.total_length += sizeof(x_chunk);
	}

	/* Write the data chunk header */
	data_chunk_head.magic = B_HOST_TO_BENDIAN_INT32(_RIFF_DATA);
	fHeader.offset_data_length = fHeader.writer->Position() + 4;
	fHeader.data_length = 0;
	fHeader.total_length += sizeof(_riff_chunk);
	if (fHeader.writer->Write(&data_chunk_head, sizeof(_riff_chunk)) < B_OK) {
DEBUG("#### CommitHeader Out 6\n");
		return B_ERROR;
	}
DEBUG("#### CommitHeader Out 7\n");
	fHeaderCommitted = true;
	return B_OK;
}
	
status_t WAVWriter::WriteData(	int32 				tracknum,
							 	media_type 			type,
					 			const void 			*data,
								size_t 				size,
					 			media_encode_info	*info) {
	char		tmp_buffer[OUTPUT_BUFFER_SIZE];
	char		*dest;
	uint32		i, buf_size, count4, count2;
	const char	*data_ptr = (const char *)data;

	if (tracknum != 0)
		return B_BAD_INDEX;
	if(!fHeaderCommitted || !fHeader.writer)
		return B_NOT_ALLOWED;

DEBUG("#### WriteData In\n");
	fHeader.data_length += size;

	switch (fHeader.mode) {
	case ENCODED :
	case U8TOU8 :
	case L16TOL16 :
	case L32TOL32 :
		if (fHeader.writer->Write(data_ptr, size) < B_OK) {
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
			case S8TOU8:
				if (buf_size & 3)
					for (i=0; i<buf_size; i++)
						dest[i] = data_ptr[i] + 128;
				else {
					count4 = buf_size>>2;
					for (i=0; i<count4; i++)
						((int32*)dest)[i] = ((int32*)data_ptr)[i] ^ 0x80808080;
				}
				break;
			case B16TOL16 :
				count2 = buf_size>>1;
				for (i=0; i<count2; i++)
					((int16*)dest)[i] = B_SWAP_INT16(((int16*)data_ptr)[i]);
				break;
			case LFLOATTOL32 :
				count4 = buf_size>>2;
				for (i=0; i<count4; i++)
					((int32*)dest)[i] = B_HOST_TO_LENDIAN_INT32(
						(int32)(MAX_VALUE*B_LENDIAN_TO_HOST_FLOAT(((float*)data_ptr)[i])));
				break;
			case BFLOATTOL32 :
				count4 = buf_size>>2;
				for (i=0; i<count4; i++)
					((int32*)dest)[i] = B_HOST_TO_LENDIAN_INT32(
						(int32)(MAX_VALUE*B_BENDIAN_TO_HOST_FLOAT(((float*)data_ptr)[i])));
				break;
			case B32TOL32 :
				count4 = buf_size>>2;
				for (i=0; i<count4; i++)
					((int32*)dest)[i] = B_SWAP_INT32(((int32*)data_ptr)[i]);
				break;
			default :
				return B_BAD_TYPE;
			}
			if (fHeader.writer->Write(tmp_buffer, buf_size) < B_OK)
				return B_ERROR;
			data_ptr += buf_size;
			size -= buf_size;
		}
	}

DEBUG("#### WriteData Out 2\n");
	return B_OK;
}


status_t WAVWriter::CloseFile(void) {
	uint32		length;

	if(!fHeaderCommitted || !fHeader.writer)
		return B_NOT_ALLOWED;
DEBUG("#### CloseFile In\n");
	/* Go back and patch the data length */
	length = B_HOST_TO_LENDIAN_INT32(fHeader.data_length);
	if (fHeader.writer->Seek(fHeader.offset_data_length, SEEK_SET) < B_OK)
		return B_ERROR;
	if (fHeader.writer->Write(&length, sizeof(length)) < B_OK)
		return B_ERROR;
	/* Go back and patch the total length */
	length = B_HOST_TO_LENDIAN_INT32(fHeader.total_length+fHeader.data_length);
	if (fHeader.writer->Seek(fHeader.offset_total_length, SEEK_SET) < B_OK)
		return B_ERROR;
	if (fHeader.writer->Write(&length, sizeof(length)) < B_OK)
		return B_ERROR;
	if (fHeader.writer->Flush() < B_OK)
		return B_ERROR;

	delete fHeader.writer;
	fHeader.writer = NULL;
	
DEBUG("#### CloseFile Out\n");
	return B_OK;
}



