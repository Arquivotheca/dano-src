#include "WAVExtractor.h"
#include <stdio.h>
#include <math.h>
#include <ByteOrder.h>
#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>

//#define DEBUG	printf
#define DEBUG	if (0) printf

/* WAV signatures */
#define _RIFF_MAGIC	'RIFF'
#define _WAVE_MAGIC	'WAVE'
#define _RIFF_FMT	'fmt '
#define _RIFF_DATA	'data'
#define _RIFF_FACT  'fact'

#define WAVE_MS_PCM 	0x0001
#define WAVE_MS_ADPCM 	0x0002
#define WAVE_MS_IEEE 	0x0003
#define WAVE_IMA_ADPCM	0x0011
#define WAVE_MS_EXTENSIBLE 0xfffe
#define WAVE_MPEG 0x0055

//--------------------------------------------------------			
#define WAVE_FORMAT_G721_4 0x7214
#define WAVE_FORMAT_G723_3 0x7233
#define WAVE_FORMAT_G723_5 0x7235
//--------------------------------------------------------			
/*
#define WAVE_IBM_MULAW	0x0101
#define WAVE_IBM_ALAW	0x0102
#define WAVE_IBM_ADPCM	0x0103
*/
/* Private struct */
typedef struct {
	int32 riff_magic;
	int32 chunk_size;
	int32 wav_magic;
} _wav_sound_header;

typedef struct {
	int32 magic;
	int32 size;
} _riff_chunk;

typedef struct {
	int16 format;
	int16 channel_count;
	int32 sampling_rate;
	int32 average_rate;
	int16 alignment;
} _wav_format_chunk;

typedef struct {
	int16 sample_size; // in bits
} _wav_format_pcm;

typedef struct {
} _wav_format_mulaw;

typedef struct {
} _wav_format_alaw;

typedef struct {
    uint16 sample_size;       /* Number of bits per sample of mono data */
    uint16 cb_size;	          /* How many byte in the rest of the optional struct */
	uint16 samples_per_block; /* How many samples per encoded blocs */
	uint16 num_coef;		  /* How many coefs (followed by the coef list) */
} _wav_format_adpcm;

typedef struct {
	uint16 sample_size;
	uint16 samples_per_block;
	uint16 cb_size;
} _wav_format_ima_adpcm;

typedef struct {
	uint32	one;
	uint16	two;
	uint16	three;
	uchar	four[8];
} WAV_GUID;

typedef struct {
	uint16 sample_size;
	uint16 cb_size;
	union {
		uint16 valid_bits;
		uint16 samples_per_block;
	};
	uint32 channel_mask;
	WAV_GUID guid;
} _wav_format_extensible;

static bool operator==(const WAV_GUID & a, const WAV_GUID & b) {
	return !memcmp(&a, &b, sizeof(WAV_GUID));
}

static WAV_GUID wav_format_pcm_ex = { 0x1, 0x0, 0x10, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static WAV_GUID wav_format_ieee_ex = { 0x3, 0x0, 0x10, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static WAV_GUID wav_format_ambisonic_pcm = { 0x1, 0x721, 0x11d3, { 0x86, 0x44, 0xc8, 0xc1, 0xca, 0x00, 0x00, 0x00 } };
static WAV_GUID wav_format_ambisonic_ieee = { 0x3, 0x721, 0x11d3, { 0x86, 0x44, 0xc8, 0xc1, 0xca, 0x00, 0x00, 0x00 } };

static bool in_ms_family(const WAV_GUID & a) {
	return (a.two == 0x0) && (a.three == 0x10) && !memcmp(a.four, wav_format_pcm_ex.four, 8);
}

enum {
	WAV_CHUNK_SIZE		= 65536,
	MINIMAL_BLOC_LENGTH	= 1024,
	WAV_BUFFER_LENGTH	= 60000	// in us
};

extern "C" const char *mime_types_extractor[] = { "audio/wav", "audio/x-wav", NULL };

extern "C" Extractor *instantiate_extractor(void)
{
	return new WAVExtractor();
}

/* Code */
WAVExtractor::WAVExtractor() {
	/* format description */
	fSampleFormat = 0;
	// init the channel count to 1 in case it is used before ReadFmt()
	// is called (which would cause a divide by zero).  not the best
	// solution, but better than crashing.
	fChannelCount = 1;
	fMediaType = 0;
	fValidBits = 0;
	fChannelMask = 0;
	fOutputFrameRate = 0.0;
	md.fNumCoef = 0;
	/* pseudo-bloc management */
	fBlocSize = 0;
	fFramePerBloc = 0;
	fFrameCount = 0;
	fFrameSize = 0;
	/* data access */
	fDataOffset = 0;
	fDataEnd = 0;
}

WAVExtractor::~WAVExtractor() {
}

status_t WAVExtractor::GetFileFormatInfo(media_file_format *mfi)
{
	strcpy(mfi->mime_type,      "audio/x-wav");
	strcpy(mfi->pretty_name,    "WAV Sound File");
	strcpy(mfi->short_name,     "wave");
	strcpy(mfi->file_extension, "wav");
	mfi->capabilities = media_file_format::B_READABLE |
		media_file_format::B_IMPERFECTLY_SEEKABLE |
		media_file_format::B_PERFECTLY_SEEKABLE |
		media_file_format::B_KNOWS_RAW_AUDIO |
		media_file_format::B_KNOWS_ENCODED_AUDIO;
	mfi->family = B_WAV_FORMAT_FAMILY;
	mfi->version = B_BEOS_VERSION;

	return B_OK;
}


/* Initialisation/Setup call before taking other the data extraction */
status_t WAVExtractor::Sniff(int32 *out_streamNum, int32 *out_chunkSize) {
	status_t			err;
	_wav_sound_header	h;

	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	if (pio == NULL)
		return B_IO_ERROR;

	/* Check the magic signature for WAV. */
	pio->Seek(0, SEEK_SET);
	if (pio->Read(&h, sizeof(h)) != sizeof(h))
		return B_IO_ERROR;
	if (B_BENDIAN_TO_HOST_INT32(h.riff_magic) != _RIFF_MAGIC ||
		B_BENDIAN_TO_HOST_INT32(h.wav_magic) != _WAVE_MAGIC)
		return B_BAD_TYPE;

	/* Read the header and set internal states */
	err = ReadHeader();
	if (err != B_OK) {
DEBUG("ReadHeader returned error\n");
		return err;
	}


DEBUG("$######## End of sniffing :fSampleFormat = %ld\n", fSampleFormat);
DEBUG("$######## End of sniffing :fChannelCount = %ld\n", fChannelCount);
DEBUG("$######## End of sniffing :fMediaType = %ld\n", fMediaType);
DEBUG("$######## End of sniffing :fOutputFrameRate = %f\n", fOutputFrameRate);
DEBUG("$######## End of sniffing :fBlocSizeOut = %ld\n", fBlocSizeOut);
DEBUG("$######## End of sniffing :md.fNumCoef = %ld\n", md.fNumCoef);
DEBUG("$######## End of sniffing :fBlocSize = %ld\n", fBlocSize);
DEBUG("$######## End of sniffing :fFramePerBloc = %ld\n", fFramePerBloc);
DEBUG("$######## End of sniffing :fFrameCount = %ld\n", fFrameCount);
DEBUG("$######## End of sniffing :fFrameSize = %ld\n", fFrameSize);
DEBUG("$######## End of sniffing :fDataOffset = %ld\n", fDataOffset);
DEBUG("$######## End of sniffing :fDataEnd = %ld\n", fDataEnd);
DEBUG("$######## End of sniffing :fChannelMask = %ld\n", fChannelMask);
DEBUG("$######## End of sniffing :fValidBits = %d\n", fValidBits);
DEBUG("$######## End of sniffing :fMatrixMask = %d\n", fMatrixMask);
	/* Exit values */
	*out_streamNum = 1;
	*out_chunkSize = WAV_CHUNK_SIZE;
	return B_OK;
}

/* Read header and extract useful information */
status_t WAVExtractor::ReadHeader() {
	bool				got_fmt, got_data;
	int32 				chunk_size, chunk_start, full_bloc, nibble_left;
	status_t			err;
	_riff_chunk			chunk_head;

	/* Flags for things to find during the init process */	
	got_fmt = false;
	got_data = false;

	BPositionIO *file = dynamic_cast<BPositionIO *>(Source());
	if (file == NULL)
		return B_IO_ERROR;

	/* Seek through the header tables, looking for the key infos */
	while (file->Read(&chunk_head, sizeof(chunk_head)) == sizeof(chunk_head)) {
		/* size and offset of a table */
		chunk_size = B_LENDIAN_TO_HOST_INT32(chunk_head.size);
		chunk_start = file->Seek(0, SEEK_CUR);
		if (chunk_start < 0)
			return B_IO_ERROR;
		/* Special processing for some table */
DEBUG("Header 0x%08lx [%c%c%c%c]\n", B_BENDIAN_TO_HOST_INT32(chunk_head.magic),
	  				((char*)&chunk_head.magic)[0], ((char*)&chunk_head.magic)[1],
	  				((char*)&chunk_head.magic)[2], ((char*)&chunk_head.magic)[3]);
		switch (B_BENDIAN_TO_HOST_INT32(chunk_head.magic)) {
		case _RIFF_FMT:
DEBUG("Got format\n");
			err = ReadFmt();
			if (err != B_OK)
				return err;
			got_fmt = true;
			break;
		case _RIFF_FACT:
			err = ReadFact();
			if (err != B_OK)
				return err;
			break;
DEBUG("Got Data\n");
		case _RIFF_DATA:
			fDataOffset = chunk_start;
			fDataEnd = chunk_start + chunk_size;
			got_data = true;
			break;
		}
		/* We got all we want */
		if (got_data)
			break;
		/* Test various cases of failure */
		off_t size = file->Seek(0, SEEK_END);
		if (size < B_OK)
			return size;
		if (chunk_start + chunk_size > size)
			return B_IO_ERROR;
		if (file->Seek(chunk_start + chunk_size, SEEK_SET) < 0)
			return B_IO_ERROR;
	}

	/* Did we complete the init process ? */
	if (!got_fmt || !got_data)
		return B_BAD_VALUE;

	/* Calculate the FrameCount */
	switch (fMediaType) {
	case B_MEDIA_RAW_AUDIO :
		fFrameCount = (fDataEnd - fDataOffset) / fFrameSize;
DEBUG("Raw audio: total %ld frames\n", fFrameCount);
		break;
	case B_MEDIA_ENCODED_AUDIO :
		switch (fSampleFormat) {
		case WAVE_MPEG:
			// read some mp3 from the data chunk here to check if this is actually
			// an mpeg version that we can handle
			{
				unsigned char _mp3header[2];
				if(file->Read(_mp3header,2)!=2)
					return B_IO_ERROR; // incomplete file
				if(_mp3header[0]!=0xff || (_mp3header[1]&0xe0)!=0xe0) // allow mpeg 2.5
					return B_BAD_TYPE;
			
				// If the average bytes/sec is set, use it to calculate the length of the file.
				// Information from the fact chunk has been found unreliable in practice.
				if(fRawDataRate)
					fFrameCount = (uint32)((fDataEnd - fDataOffset) * fOutputFrameRate / fRawDataRate);
			}
			break;
			
		case WAVE_MS_ADPCM :
			/* first, let's count the complete blocs */
DEBUG("Total:%ld, BlocSize:%ld\n", (fDataEnd - fDataOffset), fBlocSize);
			full_bloc = (fDataEnd - fDataOffset) / fBlocSize;
DEBUG("Full bloc : %ld\n", full_bloc);
			fFrameCount = full_bloc * fFramePerBloc;
			/* correction for the last bloc */
			nibble_left = ((fDataEnd - fDataOffset) - full_bloc * fBlocSize) * 2;
DEBUG("nibble_left : %ld\n", nibble_left);
			if (nibble_left > 0)
				fFrameCount += 2 + (nibble_left / fChannelCount - 14);

			break;

		case WAVE_IMA_ADPCM:
DEBUG("Total:%ld, BlocSize:%ld\n", (fDataEnd - fDataOffset), fBlocSize);
			full_bloc = (fDataEnd - fDataOffset) / fBlocSize;
DEBUG("Full bloc : %ld\n", full_bloc);
			fFrameCount = full_bloc * fFramePerBloc;
			/* correction for the last bloc */
			nibble_left = ((fDataEnd - fDataOffset) - full_bloc * fBlocSize) * 2;
DEBUG("nibble_left : %ld\n", nibble_left);
			if (nibble_left > 0)
				fFrameCount += (1 + (nibble_left - 16*fChannelCount) / fChannelCount);
			break;
		/*
			the WAVE Extractor needs to know the number of frame contained in the file
		*/
		case WAVE_FORMAT_G721_4:
			{
				fFrameCount = fDataEnd - fDataOffset; 	//in bytes
				fFrameCount *= 8; 						//in bits
				fFrameCount /= 4; 						//in code = in sample
				fFrameCount /= fChannelCount;			//in frame
			}
			break;
		case WAVE_FORMAT_G723_3:
			{
				fFrameCount = fDataEnd - fDataOffset; 	//in bytes
				fFrameCount *= 8; 						//in bits
				fFrameCount /= 3; 						//in code = in sample
				fFrameCount /= fChannelCount;			//in frame
			}
			break;
		case WAVE_FORMAT_G723_5:
			{
				fFrameCount = fDataEnd - fDataOffset; 	//in bytes
				fFrameCount *= 8; 						//in bits
				fFrameCount /= 5; 						//in code = in sample
				fFrameCount /= fChannelCount;			//in frame
			}
			break;

		default :
			return B_BAD_TYPE;
		}
		break;
	default :
		return B_BAD_TYPE;
	}
DEBUG("ReadHeader() returns B_OK\n");
	return B_OK;
}  

/* Read fmt table and extract useful information */
status_t WAVExtractor::ReadFmt() {
	int32				bps, i, block_per_blockout;
	int16				list_coef[2*MSADPCM_MAX_COEF];
	_wav_format_pcm		wfp;
	_wav_format_alaw	wfa;	
	_wav_format_mulaw	wfm;	
	_wav_format_adpcm	wfadp;
	_wav_format_ima_adpcm	wfima;
	_wav_format_chunk	wfc;

	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	if (pio == NULL)
		return B_IO_ERROR;

	if (pio->Read(&wfc.format, 2) != 2)
		return B_IO_ERROR;
	if (pio->Read(&wfc.channel_count, 2) != 2)
		return B_IO_ERROR;
	if (pio->Read(&wfc.sampling_rate, 4) != 4)
		return B_IO_ERROR;
	if (pio->Read(&wfc.average_rate, 4) != 4)
		return B_IO_ERROR;
	if (pio->Read(&wfc.alignment, 2) != 2)
		return B_IO_ERROR;
	fOutputFrameRate = (float)B_LENDIAN_TO_HOST_INT32(wfc.sampling_rate);
	if ((fOutputFrameRate >= 11022.0) && (fOutputFrameRate <= 11028.0))
		fOutputFrameRate = 11025.0;
	else if ((fOutputFrameRate >= 22044.0) && (fOutputFrameRate <= 22056.0))
		fOutputFrameRate = 22050.0;
	else if ((fOutputFrameRate >= 44088.0) && (fOutputFrameRate <= 44112.0))
		fOutputFrameRate = 44100.0;

	fRawDataRate = (float)B_LENDIAN_TO_HOST_INT32(wfc.average_rate);
	fChannelCount = B_LENDIAN_TO_HOST_INT16(wfc.channel_count);

DEBUG("######### Format : 0x%04x\n", B_LENDIAN_TO_HOST_INT16(wfc.format));
	bool isFloat = false;
	switch (B_LENDIAN_TO_HOST_INT16(wfc.format)) {
	/* Non compressed audio, standard format */
	case WAVE_MS_IEEE:
		isFloat = true;
	case WAVE_MS_PCM :
		if (pio->Read(&wfp.sample_size, 2) != 2)
			return B_IO_ERROR;
		fMediaType = B_MEDIA_RAW_AUDIO;
		bps = B_LENDIAN_TO_HOST_INT16(wfp.sample_size);
		if ((!isFloat) && (bps <= 8)) {
			fSampleFormat = media_raw_audio_format::B_AUDIO_UCHAR;
			fFrameSize = fChannelCount;
		} else if ((!isFloat) && (bps <= 16)) {
			fSampleFormat = media_raw_audio_format::B_AUDIO_SHORT;
			fFrameSize = fChannelCount*2;
		} else if ((!isFloat) && (bps <= 24)) {
			//fixme	we should left-justify these samples when reading
			//	and do it for all sizes > 16 <= 32
			return B_BAD_TYPE;
		} else if ((!isFloat) && (bps <= 32)) {
			fSampleFormat = media_raw_audio_format::B_AUDIO_INT;
			fFrameSize = fChannelCount*4;
		} else if (isFloat && (bps == 32)) {
			fSampleFormat = media_raw_audio_format::B_AUDIO_FLOAT;
			fFrameSize = fChannelCount*4;
		}
		else
			return B_BAD_TYPE;
		{
			int32 f = (int32)(fOutputFrameRate*(float)WAV_BUFFER_LENGTH*1e-6);
			fFramePerBloc = 128;
			while (fFramePerBloc < f) fFramePerBloc<<=1;
			fBlocSize = (fFramePerBloc*fFrameSize) & ~7;
			while (fBlocSize < MINIMAL_BLOC_LENGTH) {
				fBlocSize <<= 1;
			}
			while (fBlocSize > WAV_CHUNK_SIZE/2) {
				fBlocSize >>= 1;
			}
			fFramePerBloc = fBlocSize/fFrameSize;
		}
		break;
	/* Adapatative Differential MS compressed audio */	
	case WAVE_MS_ADPCM :
		if (pio->Read(&wfadp.sample_size, 2) != 2)
			return B_IO_ERROR;

		if (pio->Read(&wfadp.cb_size, 2) != 2)
			return B_IO_ERROR;

		if (pio->Read(&wfadp.samples_per_block, 2) != 2)
			return B_IO_ERROR;

		if (pio->Read(&wfadp.num_coef, 2) != 2)
			return B_IO_ERROR;

		// Check that the encoded format use 4 bits per sample
		if (B_LENDIAN_TO_HOST_INT16(wfadp.sample_size) != 4)
			return B_BAD_VALUE;

		// Encoded bloc size
		fFramePerBloc = B_LENDIAN_TO_HOST_INT16(wfadp.samples_per_block);
		fBlocSize = ((14+fFramePerBloc-2)*fChannelCount+1)/2;
		block_per_blockout = (int32)((fOutputFrameRate*(float)WAV_BUFFER_LENGTH*1e-6)/(float)fFramePerBloc + 0.5);
		if (block_per_blockout < 1)
			block_per_blockout = 1;
		fBlocSizeOut = block_per_blockout*fFramePerBloc*2*fChannelCount;

		// Read the coefs
		md.fNumCoef = (B_LENDIAN_TO_HOST_INT16(wfadp.cb_size)-4) / 4;
		if (md.fNumCoef != B_LENDIAN_TO_HOST_INT16(wfadp.num_coef))
			return B_BAD_VALUE;
		if (md.fNumCoef > MSADPCM_MAX_COEF)
			return B_BAD_VALUE;
		if (pio->Read(list_coef, (2 * sizeof(int16)) * md.fNumCoef) != (2 * sizeof(int16)) *
			md.fNumCoef)
			return B_IO_ERROR;
		
		for (i=0; i<md.fNumCoef*2; i++)
			md.fListCoef[i] = B_LENDIAN_TO_HOST_INT16(list_coef[i]);
		
		fMediaType = B_MEDIA_ENCODED_AUDIO;
		fSampleFormat = WAVE_MS_ADPCM;
		break;
	/*
		Information needed by the WAV Extractor (for pseudo-bloc management, ...)
	*/
	case WAVE_FORMAT_G721_4:
		{
			fMediaType = B_MEDIA_ENCODED_AUDIO;
			fBlocSize  = 1;					// size of block the WAV Extractor will dispatch to the decoder
			fBlocSize *= 512;				// must be divisible by 4             (because of assumption in decoder)
			fBlocSize *= fChannelCount; 	// must be divisible by channel count (because of assumption in decoder)
			
			fFramePerBloc  = 1;				//number of frame in each bloc
			fFramePerBloc *= fBlocSize;		//in bytes		
			fFramePerBloc *= 8;				//in bits
			fFramePerBloc /= 4;				//in code = in sample
			fFramePerBloc /= fChannelCount;	//in frame
			
			fBlocSizeOut = 4096;			//must be a power of two
			
			fMediaType = B_MEDIA_ENCODED_AUDIO;
			fSampleFormat = WAVE_FORMAT_G721_4;
		}
		break;
	case WAVE_FORMAT_G723_3:
		{
			fBlocSize  = 1;					// size of block the WAV Extractor will dispatch to the decoder
			fBlocSize *= 510;				// must be divisible by 3             (because of assumption in decoder)
			fBlocSize *= fChannelCount; 	// must be divisible by channel count (because of assumption in decoder)
			
			fFramePerBloc  = 1;				//number of frame in each bloc
			fFramePerBloc *= fBlocSize;		//in bytes		
			fFramePerBloc *= 8;				//in bits
			fFramePerBloc /= 3;				//in code = in sample
			fFramePerBloc /= fChannelCount;	//in frame
			
			fBlocSizeOut = 4096;			//must be a power of two
			
			fMediaType = B_MEDIA_ENCODED_AUDIO;
			fSampleFormat = WAVE_FORMAT_G723_3;
		}
		break;
	case WAVE_FORMAT_G723_5:
		{
			fBlocSize  = 1;					// size of block the WAV Extractor will dispatch to the decoder
			fBlocSize *= 510;				// must be divisible by 5             (because of assumption in decoder)
			fBlocSize *= fChannelCount; 	// must be divisible by channel count (because of assumption in decoder)
			
			fFramePerBloc  = 1;				//number of frame in each bloc
			fFramePerBloc *= fBlocSize;		//in bytes		
			fFramePerBloc *= 8;				//in bits
			fFramePerBloc /= 5;				//in code = in sample
			fFramePerBloc /= fChannelCount;	//in frame
			
			fBlocSizeOut = 4096;			//must be a power of two
			
			fMediaType = B_MEDIA_ENCODED_AUDIO;
			fSampleFormat = WAVE_FORMAT_G723_5;
		}
		break;
		
	// Intel-style ADPCM
	case WAVE_IMA_ADPCM :
		DEBUG("### WAVE_IMA_ADPCM\n");
		if (pio->Read(&wfima.sample_size, 2) != 2)
			return B_IO_ERROR;

		if (pio->Read(&wfima.cb_size, 2) != 2)
			return B_IO_ERROR;

		if (pio->Read(&wfima.samples_per_block, 2) != 2)
			return B_IO_ERROR;

		// Check that the encoded format use 4 bits per sample
		if (B_LENDIAN_TO_HOST_INT16(wfima.sample_size) != 4)
			return B_BAD_VALUE;

		// Encoded bloc size
		fBlocSize = B_LENDIAN_TO_HOST_INT16(wfc.alignment);
		fFramePerBloc = ((fBlocSize - 4*fChannelCount)*2)/fChannelCount + 1;
		block_per_blockout = (int32)((fOutputFrameRate*(float)WAV_BUFFER_LENGTH*1e-6)/(float)fFramePerBloc + 0.5);
		if (block_per_blockout < 1)
			block_per_blockout = 1;
		fBlocSizeOut = block_per_blockout*fFramePerBloc*2*fChannelCount;

		fMediaType = B_MEDIA_ENCODED_AUDIO;
		fSampleFormat = WAVE_IMA_ADPCM;
		break;

	case WAVE_MPEG:
		fMediaType = B_MEDIA_ENCODED_AUDIO;
		fSampleFormat = WAVE_MPEG;

		// Note that 'frame' and 'channel' are kind of misleading.  This is
		// actually raw decoded data which we just pass on to the decoder.
		fBlocSize = 128;
		fBlocSizeOut = 128;
		break;

	case WAVE_MS_EXTENSIBLE: {
			_wav_format_extensible wfe;
			if (pio->Read(&wfe.sample_size, sizeof(wfe.sample_size)) != sizeof(wfe.sample_size)) {
				return B_IO_ERROR;
			}
			wfe.sample_size = (uint16)B_LENDIAN_TO_HOST_INT16(wfe.sample_size);
			if (pio->Read(&wfe.cb_size, sizeof(wfe.cb_size)) != sizeof(wfe.cb_size)) {
				return B_IO_ERROR;
			}
			wfe.cb_size = (uint16)B_LENDIAN_TO_HOST_INT16(wfe.cb_size);
			if (pio->Read(&wfe.valid_bits, sizeof(wfe.valid_bits)) != sizeof(wfe.valid_bits)) {
				return B_IO_ERROR;
			}
			wfe.valid_bits = (uint16)B_LENDIAN_TO_HOST_INT16(wfe.valid_bits);
			if (pio->Read(&wfe.channel_mask, sizeof(wfe.channel_mask)) != sizeof(wfe.channel_mask)) {
				return B_IO_ERROR;
			}
			wfe.channel_mask = (uint32)B_LENDIAN_TO_HOST_INT32(wfe.channel_mask);
			if (pio->Read(&wfe.guid, sizeof(wfe.guid)) != sizeof(wfe.guid)) {
				return B_IO_ERROR;
			}
			wfe.guid.one = (uint32)B_LENDIAN_TO_HOST_INT32(wfe.guid.one);
			wfe.guid.two = (uint16)B_LENDIAN_TO_HOST_INT16(wfe.guid.two);
			wfe.guid.three = (uint16)B_LENDIAN_TO_HOST_INT16(wfe.guid.three);

#if 1
fprintf(stderr, "GUID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
	wfe.guid.one, wfe.guid.two, wfe.guid.three, wfe.guid.four[0],
	wfe.guid.four[1], wfe.guid.four[2], wfe.guid.four[3], wfe.guid.four[4],
	wfe.guid.four[5], wfe.guid.four[6], wfe.guid.four[7]);
#endif
#if 0	//	this is done in the outer scan loop
			if (wfe.cb_size != 22) {
				if (pio->Seek(wfe.cb_size-22, 1) < 0) {
					return B_IO_ERROR;
				}
			}
#endif
			fMediaType = B_MEDIA_RAW_AUDIO;
			bps = (uint16)B_LENDIAN_TO_HOST_INT16(wfe.sample_size);
			if (bps <= 8) {
				fSampleFormat = media_raw_audio_format::B_AUDIO_UCHAR;
				fFrameSize = fChannelCount;
			} else if (bps <= 16) {
				fSampleFormat = media_raw_audio_format::B_AUDIO_SHORT;
				fFrameSize = fChannelCount*2;
			} else if (bps <= 24) {
				//fixme	we should left-justify these samples when reading
				//	and do it for all sizes > 16 <= 32
DEBUG("bps = %d, bailing\n", bps);
				return B_BAD_TYPE;
			} else if (bps <= 32) {
				fSampleFormat = media_raw_audio_format::B_AUDIO_INT;
				fFrameSize = fChannelCount*4;
			} else {
DEBUG("bps = %d, bailing\n", bps);
				return B_BAD_TYPE;
			}
			if (fFrameSize != (uint16)B_LENDIAN_TO_HOST_INT16(wfc.alignment)) {
DEBUG("fFrameSize = %d, alignment = %d, bailing\n", fFrameSize, wfc.alignment);
				return B_MEDIA_BAD_FORMAT;
			}
			if (wfe.guid == wav_format_pcm_ex) {
				fValidBits = wfe.valid_bits;
				fChannelMask = wfe.channel_mask;
				fMatrixMask = 0;
			}
			else if (wfe.guid == wav_format_ieee_ex) {
				fValidBits = 32;
				fChannelMask = wfe.channel_mask;
				fMatrixMask = 0;
				fSampleFormat = media_raw_audio_format::B_AUDIO_FLOAT;
			}
			else if (wfe.guid == wav_format_ambisonic_pcm) {
				fValidBits = wfe.valid_bits;
				fChannelMask = 0;
				fMatrixMask = B_MATRIX_AMBISONIC_WXYZ;
			}
			else if (wfe.guid == wav_format_ambisonic_ieee) {
				fValidBits = wfe.valid_bits;
				fChannelMask = 0;
				fMatrixMask = B_MATRIX_AMBISONIC_WXYZ;
				fSampleFormat = media_raw_audio_format::B_AUDIO_FLOAT;
			}
			else {
DEBUG("GUID not known, bailing!\n");
				return B_MEDIA_BAD_FORMAT;
			}
			{
				int32 f = (int32)(fOutputFrameRate*(float)WAV_BUFFER_LENGTH*1e-6);
				fFramePerBloc = 128;
				while (fFramePerBloc < f) fFramePerBloc<<=1;
				fBlocSize = (fFramePerBloc * fFrameSize) & ~7;
				while (fBlocSize < MINIMAL_BLOC_LENGTH) {
					fBlocSize <<= 1;
				}
				while (fBlocSize > WAV_CHUNK_SIZE/2) {
					fBlocSize >>= 1;
				}
				fFramePerBloc = fBlocSize/fFrameSize;
			}
		}
		break;

/*
	case WAVE_IBM_MULAW :
		if (pio->Read(&wfm, sizeof(wfm)) != sizeof(wfm))
			return B_ERROR;
		fMediaType = B_MEDIA_FIRST_USER_TYPE+1;
		break;
		
	case WAVE_IBM_ALAW :
		if (pio->Read(&wfa, sizeof(wfa)) != sizeof(wfa))
			return B_ERROR;
		fMediaType = B_MEDIA_FIRST_USER_TYPE+2;
		break;

	case WAVE_IBM_ADPCM :
		if (pio->Read(&wfadp, sizeof(wfadp)) != sizeof(wfadp))
			return B_ERROR;
		fMediaType = B_MEDIA_FIRST_USER_TYPE+3;
		fSampleFormat = media_raw_audio_format::B_AUDIO_UCHAR;
		fFrameSize = 1;
		if (fChannelCount != 1)
			return B_ERROR;
		break;
*/		

	default :
		return B_BAD_TYPE;
	}

DEBUG("ReadFmt() returns OK\n");
	return B_OK;
}

status_t WAVExtractor::ReadFact()
{
	BPositionIO *pio = dynamic_cast<BPositionIO *>(Source());
	if (pio == NULL)
		return B_IO_ERROR;

	unsigned int sampleCount;
	if (pio->Read(&sampleCount, 4) != 4)
		return B_IO_ERROR;

	fFrameCount = B_LENDIAN_TO_HOST_INT32(sampleCount) / fChannelCount / 2;
	return B_OK;
}

/* Provide information about output format */
status_t WAVExtractor::TrackInfo(int32 in_stream, media_format *out_format, void **out_info, int32 *out_size) {
	int32						i;
	status_t					err;
	BMediaFormats				formatObject;
	media_format_description	fd;

	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* fills the media_format descriptor */
	out_format->type = (media_type)fMediaType;
	out_format->user_data_type = B_CODEC_TYPE_INFO;
	sprintf((char *)out_format->user_data, "%d", fSampleFormat);
	switch (fMediaType) {
	case B_MEDIA_RAW_AUDIO :
		out_format->u.raw_audio.frame_rate = fOutputFrameRate;
		out_format->u.raw_audio.channel_count = fChannelCount;
		out_format->u.raw_audio.format = fSampleFormat;
		out_format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
		out_format->u.raw_audio.buffer_size = fBlocSize;
		out_format->u.raw_audio.channel_mask = fChannelMask;
		out_format->u.raw_audio.valid_bits = fValidBits;
		out_format->u.raw_audio.matrix_mask = fMatrixMask;
DEBUG("############# Rate : %f\n", fOutputFrameRate);
DEBUG("############# ChannelCount : %d\n", fChannelCount);
DEBUG("############# SampleFormat : %d\n", fSampleFormat);
DEBUG("############# BufferSize : %d (%x)\n", fBlocSize, fBlocSize);
		break;
	case B_MEDIA_ENCODED_AUDIO :
		switch (fSampleFormat) {
		case WAVE_MS_ADPCM :
		case WAVE_IMA_ADPCM:
			{
			memset(out_format, 0, sizeof(media_format));	
			memset(&fd, 0, sizeof(fd));
			fd.family = B_WAV_FORMAT_FAMILY;
			fd.u.wav.codec = fSampleFormat;
			formatObject.Lock();
			err = formatObject.GetFormatFor(fd, out_format);
			formatObject.Unlock();
			if (err != B_OK)
			{
				DEBUG("BMediaFormats::GetFormatFor() failed: %s\n", strerror(err));
				return err;
			}

			out_format->type = B_MEDIA_ENCODED_AUDIO;
			out_format->u.encoded_audio.output.frame_rate = fOutputFrameRate;
			out_format->u.encoded_audio.output.channel_count = fChannelCount;
			out_format->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
			out_format->u.encoded_audio.output.byte_order =
				B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
			out_format->u.encoded_audio.output.buffer_size = fBlocSizeOut;
			out_format->u.encoded_audio.bit_rate = 4;
			out_format->u.encoded_audio.frame_size = fBlocSize;
			if(fSampleFormat == WAVE_MS_ADPCM)
			{
				// private msadpcm infos
				*out_info = (void*)&md;
				*out_size = sizeof(md);
			}
			}
			break;
			
		/*
			filling some track informations
		*/
		case WAVE_FORMAT_G721_4:
		case WAVE_FORMAT_G723_3:
		case WAVE_FORMAT_G723_5:
			{
				memset(out_format, 0, sizeof(media_format));	
				memset(&fd, 0, sizeof(fd));
				fd.family = B_WAV_FORMAT_FAMILY;
				fd.u.wav.codec = fSampleFormat;
				formatObject.Lock();
				err = formatObject.GetFormatFor(fd, out_format);
				formatObject.Unlock();
				if (err != B_OK)
				{
					DEBUG("BMediaFormats::GetFormatFor() failed: %s\n", strerror(err));
					return err;
				}
	
				out_format->type = B_MEDIA_ENCODED_AUDIO;
				out_format->u.encoded_audio.output.frame_rate = fOutputFrameRate;
				out_format->u.encoded_audio.output.channel_count = fChannelCount;
				out_format->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
				out_format->u.encoded_audio.output.byte_order =
					B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
				out_format->u.encoded_audio.output.buffer_size = fBlocSizeOut;
				out_format->u.encoded_audio.frame_size = fBlocSize;
			}
			break;

		case WAVE_MPEG:
			{
			memset(out_format, 0, sizeof(media_format));	
			memset(&fd, 0, sizeof(fd));
			fd.family = B_MPEG_FORMAT_FAMILY;
			fd.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_3;
			formatObject.Lock();
			err = formatObject.GetFormatFor(fd, out_format);
			formatObject.Unlock();
			if (err != B_OK)
				return err;

			out_format->type = B_MEDIA_ENCODED_AUDIO;
			out_format->u.encoded_audio.output.frame_rate = fOutputFrameRate;
			out_format->u.encoded_audio.output.channel_count = fChannelCount;
			out_format->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_UCHAR;
			out_format->u.encoded_audio.output.byte_order =
				B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
			out_format->u.encoded_audio.output.buffer_size = fBlocSizeOut;
			out_format->u.encoded_audio.frame_size = fBlocSize;
			}
			break;

		default :
DEBUG("Should never happen (Internal error 0)\n");
			return B_ERROR;
		}
		break;
	default :
DEBUG("Should never happen (Internal error 1)\n");
		return B_ERROR;
	}
	return B_OK;
}

status_t WAVExtractor::CountFrames(int32 in_stream, int64 *out_frames) {
	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* return the frame count */
	*out_frames = (int64)fFrameCount;
	return B_OK;
}

status_t WAVExtractor::GetDuration(int32 in_stream, bigtime_t *out_duration) {
	/* Check the stream id */
	if (in_stream != 0)
		return B_BAD_INDEX;
	/* return the duration */
	*out_duration = (bigtime_t)(((double)fFrameCount*1000000.0)/fOutputFrameRate);
	return B_OK;
}

bigtime_t WAVExtractor::StartTime(uint32 offset)
{
	int32		frame_count;

	frame_count = 0;
	switch (fMediaType) {
	case B_MEDIA_RAW_AUDIO :
		frame_count = (offset-fDataOffset)/fFrameSize;
		break;
	case B_MEDIA_ENCODED_AUDIO :
		switch (fSampleFormat) {
		case WAVE_MPEG:
			frame_count = (int32) ((offset - fDataOffset) * fOutputFrameRate / fRawDataRate);
			break;
		case WAVE_MS_ADPCM :
		case WAVE_IMA_ADPCM:
			frame_count = ((offset-fDataOffset)/fBlocSize)*fFramePerBloc;
			break;
		/*
			calculate frame count between data start and offset
		*/
		case WAVE_FORMAT_G721_4:
			{
				frame_count = (offset-fDataOffset);	//in bytes
				frame_count *= 8;					//in bits
				frame_count /= 4;					//in code = in sample
				frame_count /= fChannelCount;		// in frame
			}
			break;
		case WAVE_FORMAT_G723_3:
			{
				frame_count = (offset-fDataOffset);
				frame_count *= 8;
				frame_count /= 3;
				frame_count /= fChannelCount;
			}
			break;
		case WAVE_FORMAT_G723_5:
			{
				frame_count = (offset-fDataOffset);
				frame_count *= 8;
				frame_count /= 5;
				frame_count /= fChannelCount;
			}
			break;
		}
		break;
	}
	return (bigtime_t)((float)frame_count * 1000000.0 / fOutputFrameRate);
}

status_t 
WAVExtractor::AllocateCookie(int32 in_stream, void **cookieptr)
{
	*cookieptr = malloc(sizeof(uint32));
	if(*cookieptr == NULL)
		return B_NO_MEMORY;
	*((uint32*)*cookieptr) = fDataOffset;
	return B_NO_ERROR;
}

status_t 
WAVExtractor::FreeCookie(int32 in_stream, void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

status_t WAVExtractor::SplitNext(	int32	in_stream,
									void	*cookie,
									off_t	*inout_filepos,
								  	char	*in_packetPointer,
								  	int32	*inout_packetLength,
								  	char	**out_bufferStart,
								  	int32	*out_bufferLength,
									media_header *mh)
{
	int32 local_offset, local_end, real_end;
	uint32 *read_from_ptr = (uint32 *)cookie;

DEBUG("WAVExtractor::SplitNext\n");

	/* Check the stream id */
	if (in_stream != 0) {
DEBUG("Bad stream index\n");
		return B_BAD_INDEX;
	}

	/* Check if we are not reading to far ahead */
	local_offset = *read_from_ptr - (uint32)(*inout_filepos);
	if (local_offset < 0) {
DEBUG("local_offset < 0, returning\n");
		*out_bufferStart = (char*)NULL;
		*inout_filepos = *read_from_ptr;
		return B_OK;
	}
	
	/* Check if we are not reading to far back */
	local_end = local_offset + fBlocSize;
	if (local_end > WAV_CHUNK_SIZE) {
DEBUG("local_end > WAV_CHUNK_SIZE, returning\n");
		*out_bufferStart = (char*) NULL;
		*inout_filepos = *read_from_ptr;
		return B_OK;
	}

	/* Check if we arrived near the end */
	real_end = (int32) fDataEnd - (int32) (*inout_filepos);
	if (local_end > real_end)
		local_end = real_end;

	if (local_end == local_offset) {
DEBUG("local_end == local_offset\n");
		mh->start_time = StartTime(fDataEnd);
		return B_LAST_BUFFER_ERROR;
	};

	/* Check if we read far enough */
	if (local_end > (int32) *inout_packetLength) {
DEBUG("local_end > (int32) *inout_packetLength\n");
		*out_bufferStart = (char*) NULL;
		*inout_packetLength = local_end;
		return B_OK;
	}
	
	/* Do the split and offset the pointers for the next one... */
DEBUG("SplitNext: [%lld %d]\n", *inout_filepos+local_offset, local_end-local_offset);
	*out_bufferStart = in_packetPointer + local_offset;
	mh->orig_size = *out_bufferLength = local_end-local_offset;
	mh->start_time = StartTime(*read_from_ptr);
	*read_from_ptr += local_end - local_offset;
	mh->file_pos = *inout_filepos = *read_from_ptr;
	return B_OK;
}

/* Seeking functions */
status_t WAVExtractor::Seek(int32		in_stream,
							 void		*cookie,
							 int32		in_towhat,
							 int32		flags,
							 bigtime_t	*inout_time,
							 int64		*inout_frame,
							 off_t		*inout_filePos,
							 char		*in_packetPointer,
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
		frame_index = (uint32)((*inout_time)*fOutputFrameRate*0.000001+0.5);
		goto seek_from_index;
	case B_SEEK_BY_FRAME :
		frame_index = *inout_frame;
seek_from_index:
		/* Do the clamping */
		if (frame_index < 0)
			frame_index = 0;
		else if (frame_index > fFrameCount)
			frame_index = fFrameCount;
		/* do the bloc specific rounding, if needed */
		switch (fMediaType) {
		case B_MEDIA_RAW_AUDIO :
			*read_from_ptr = fDataOffset+frame_index*fFrameSize;
			break;
		case B_MEDIA_ENCODED_AUDIO :
			switch (fSampleFormat) {
			case WAVE_MPEG:
				*read_from_ptr = fDataOffset + (int32) (frame_index * fRawDataRate / fOutputFrameRate);
				break;
			/*
				seek only at bloc boundary
			*/
			case WAVE_FORMAT_G721_4:
			case WAVE_FORMAT_G723_3:
			case WAVE_FORMAT_G723_5:
			case WAVE_MS_ADPCM:
			case WAVE_IMA_ADPCM:
				frame_index = (frame_index / fFramePerBloc) * fFramePerBloc;
				*read_from_ptr = fDataOffset + fBlocSize * (frame_index / fFramePerBloc);
				break;
			}
		}
		/* adjust other returned values */
		*inout_time = (bigtime_t)(((double)frame_index*1000000.0) / fOutputFrameRate);
		*inout_frame = frame_index;
		*inout_filePos = *read_from_ptr;
		length = fBlocSize;
		real_length = (int32)fDataEnd - *read_from_ptr;
		if (real_length < length)
			length = real_length;
		*inout_packetLength = length;
		*out_done = true;
		return B_OK;
	default :
		return B_BAD_VALUE;
	}
}
