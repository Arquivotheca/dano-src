#include <stdio.h>
#include <MediaFormats.h>
#include <Encoder.h>
#include <string.h>

#include <MediaTheme.h>
#include <ParameterWeb.h>

#include <Debug.h>

#include "MP3Encoder.h"
#include "mp3_encode.h"

#if NDEBUG
 #define FPRINTF (void)
 #define assert(x) (void)0
#else
 #define FPRINTF fprintf
 #define assert(x) do { if (!(x)) { fprintf(stderr, "ASSERT FAILED!\n%s:%d: %s\n", __FILE__, __LINE__, #x); debugger("assert failed!"); } } while (0)
#endif

#define WAVE_FORMAT_MPEG 0x0055
const int32 default_bit_rate = 160000;

struct mpeg1waveformat {

	int16 wFormatTag;
	int16 nChannels;
	int32 nSamplesPerSec;
	int32 nAvgBytesPerSec;
	int16 nBlockAlign;
	int16 wBitsPerSample;
	int16 cbSize;

	int16 fwHeadLayer;
	int32 dwHeadBitrate;
	int16 fwHeadMode;
	int16 fwHeadModeExt;
	int16 wHeadEmphasis;
	int16 fwHeadFlags;
	int32 dwPTSLow;
	int32 dwPTSHigh;

};

#define MPEG_MODEXT_NONE 0x0001
#define MPEG_MODEXT_INTENSITY 0x0002
#define MPEG_MODEXT_MSSTEREO 0x0004
#define MPEG_MODEXT_INT_MS 0x0008

#define MPEG_RATE_SINGLE 0x1
#define MPEG_RATE_DUAL 0x2

static struct {
	int32 bitrate;
	uint32 caps;
} mpeg_bitrate_lay3[] = {
	{ 0, 0 },			/* free form */
	{ 32000, MPEG_RATE_SINGLE },
	{ 40000, MPEG_RATE_SINGLE },
	{ 48000, MPEG_RATE_SINGLE },
	{ 56000, MPEG_RATE_SINGLE },
	{ 64000, MPEG_RATE_SINGLE | MPEG_RATE_DUAL },
	{ 80000, MPEG_RATE_SINGLE },
	{ 96000, MPEG_RATE_SINGLE | MPEG_RATE_DUAL },
	{ 112000, MPEG_RATE_SINGLE | MPEG_RATE_DUAL },
	{ 128000, MPEG_RATE_SINGLE | MPEG_RATE_DUAL },
	{ 160000, MPEG_RATE_SINGLE | MPEG_RATE_DUAL },
	{ 192000, MPEG_RATE_SINGLE | MPEG_RATE_DUAL },
	{ 224000, MPEG_RATE_DUAL },
	{ 256000, MPEG_RATE_DUAL },
	{ 320000, MPEG_RATE_DUAL },
	-1			/* forbidden */
};

#define MPEG_EMPHASIS_NONE 0x0001
#define MPEG_EMPHASIS_5015 0x0002
#define MPEG_EMPHASIS_CCITT 0x0004

#define MPEG_LAYER1 0x0001
#define MPEG_LAYER2 0x0002
#define MPEG_LAYER3 0x0004

#define MPEG_STEREO 0x0001
#define MPEG_JOINTSTEREO 0x0002
#define MPEG_DUALCHANNEL 0x0004
#define MPEG_SINGLECHANNEL 0x0008

#define MPEG_PRIVATEBIT 0x0001
#define MPEG_COPYRIGHT 0x0002
#define MPEG_ORIGINALHOME 0x0004
#define MPEG_PROTECTIONBIT 0x0008
#define MPEG_ID_MPEG1 0x0010


static media_encoded_audio_format::audio_encoding mpgencoding;
static media_encoded_audio_format::audio_encoding wavencoding;
static media_encoded_audio_format::audio_encoding aviencoding;
static media_encoded_audio_format::audio_encoding QTencoding;

extern "C" void register_encoder(void);
extern "C" BPrivate::Encoder * instantiate_encoder(void);

BPrivate::Encoder *instantiate_encoder(void) {
	return new MP3Encoder();
}

void register_encoder(void)
{
	BMediaFormats fmts;
    media_format mediaFormat;
	media_format_description desc[1];

	memset(desc, 0, sizeof(desc));
    mediaFormat.type = B_MEDIA_ENCODED_AUDIO;
	desc[0].family = B_MPEG_FORMAT_FAMILY;
	desc[0].u.mpeg.id = B_MPEG_1_AUDIO_LAYER_3;
	status_t err = fmts.MakeFormatFor(desc, 1, &mediaFormat);
    mpgencoding = mediaFormat.u.encoded_audio.encoding;

	memset(desc, 0, sizeof(desc));
    mediaFormat.type = B_MEDIA_ENCODED_AUDIO;
    desc[0].family = B_WAV_FORMAT_FAMILY;
    desc[0].u.wav.codec = WAVE_FORMAT_MPEG;
	err = fmts.MakeFormatFor(desc, 1, &mediaFormat);
    wavencoding = mediaFormat.u.encoded_audio.encoding;

	memset(desc, 0, sizeof(desc));
    mediaFormat.type = B_MEDIA_ENCODED_AUDIO;
    desc[0].family = B_AVI_FORMAT_FAMILY;
    desc[0].u.avi.codec = 0x65610055;
	err = fmts.MakeFormatFor(desc, 1, &mediaFormat);
    aviencoding = mediaFormat.u.encoded_audio.encoding;

	memset(desc, 0, sizeof(desc));
    mediaFormat.type = B_MEDIA_ENCODED_AUDIO;
    desc[0].family = B_QUICKTIME_FORMAT_FAMILY;
    desc[0].u.quicktime.codec = '.mp3';
    desc[0].u.quicktime.vendor = 0;
	err = fmts.MakeFormatFor(desc, 1, &mediaFormat);
    QTencoding = mediaFormat.u.encoded_audio.encoding;
}

MP3Encoder::MP3Encoder()
{
	memset(&m_format, 0, sizeof(m_format));
	m_is_wav_file = false;
	m_is_avi_file = false;
	m_is_QT_file = false;
	m_buffer = 0;
	m_bufAvail = 0;
	m_chunkSize = 4*1152*4;	//	stereo, 16 bit
	m_cookie = 0;
	m_flushed = false;

	m_config = new mp3_config;
	memset(m_config, 0, sizeof(mp3_config));
	m_config->bitrate = default_bit_rate;
	
	m_web = InitParameterWeb(m_config);
	SetParameterWeb(m_web);
}

MP3Encoder::~MP3Encoder()
{
	if (m_cookie != 0)
		mp3_done(m_cookie);

	free(m_buffer);

	SetParameterWeb(0);
	delete m_config;
}

status_t
MP3Encoder::GetCodecInfo(media_codec_info *mci) const
{
	/* Fill in the encoder info struct */
	strcpy(mci->pretty_name, "Fraunhofer MP3 Encoder");
	strcpy(mci->short_name, "mp3");

	return B_NO_ERROR;
}



status_t
MP3Encoder::SetFormat(media_file_format *mff,
					  media_format *in_fmt,
					  media_format *out_fmt)
{
	status_t err;

	if (!mff || !in_fmt || !out_fmt)
		return B_BAD_VALUE;

	in_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	in_fmt->require_flags = 0;
	out_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	out_fmt->require_flags = 0;

	if (mff->family == B_WAV_FORMAT_FAMILY)
		m_is_wav_file = true;
	else if (mff->family == B_AVI_FORMAT_FAMILY)
		m_is_avi_file = true;
	else if (mff->family == B_QUICKTIME_FORMAT_FAMILY)
		m_is_QT_file = true;
	else if (mff->family != B_MPEG_FORMAT_FAMILY)
		mff->family = B_MPEG_FORMAT_FAMILY;


	if (in_fmt->type != B_MEDIA_RAW_AUDIO) {
		in_fmt->type = B_MEDIA_RAW_AUDIO;
		in_fmt->u.raw_audio = media_raw_audio_format::wildcard;
	}

	if ((in_fmt->u.raw_audio.channel_count != 1) &&
		(in_fmt->u.raw_audio.channel_count != 2)) {

		in_fmt->u.raw_audio.channel_count = 2;
	}

	switch (in_fmt->u.raw_audio.format) {
	case media_raw_audio_format::B_AUDIO_UCHAR :
	case media_raw_audio_format::B_AUDIO_CHAR :
	case media_raw_audio_format::B_AUDIO_SHORT :
	case media_raw_audio_format::B_AUDIO_INT :
	case media_raw_audio_format::B_AUDIO_FLOAT :
		break;

	default:
		in_fmt->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
		break;
	}

	/* Fill in the encoded output format */
	out_fmt->type = B_MEDIA_ENCODED_AUDIO;
	out_fmt->u.encoded_audio = media_encoded_audio_format::wildcard;
	if (mff->family == B_MPEG_FORMAT_FAMILY) {
	    out_fmt->u.encoded_audio.encoding = mpgencoding;
	}
	else if (mff->family == B_AVI_FORMAT_FAMILY) {
	    out_fmt->u.encoded_audio.encoding = aviencoding;
	}
	else if (mff->family == B_QUICKTIME_FORMAT_FAMILY) {
	    out_fmt->u.encoded_audio.encoding = QTencoding;
	}
	else {
	    out_fmt->u.encoded_audio.encoding = wavencoding;
	}
	if(out_fmt->u.encoded_audio.encoding == 0)
	    return B_ERROR;
	
	out_fmt->type = B_MEDIA_ENCODED_AUDIO;
	out_fmt->u.encoded_audio.output.frame_rate = (int32)in_fmt->u.raw_audio.frame_rate;
	out_fmt->u.encoded_audio.output.channel_count = in_fmt->u.raw_audio.channel_count;
	out_fmt->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
   
	if (fabs(in_fmt->u.raw_audio.frame_rate/44100.0 - 1.0) < 0.01) {
		out_fmt->u.encoded_audio.output.frame_rate = 44100.0;
	}
	else if (fabs(in_fmt->u.raw_audio.frame_rate/48000.0 - 1.0) < 0.01) {
		out_fmt->u.encoded_audio.output.frame_rate = 48000.0;
	}
	else if (fabs(in_fmt->u.raw_audio.frame_rate/32000.0 - 1.0) < 0.01) {
		out_fmt->u.encoded_audio.output.frame_rate = 32000.0;
	}
	else {
		in_fmt->u.raw_audio.frame_rate = 44100.0;
		out_fmt->u.encoded_audio.output.frame_rate = 44100.0;
	}

	out_fmt->u.encoded_audio.output.channel_count = in_fmt->u.raw_audio.channel_count;
	out_fmt->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
	out_fmt->u.encoded_audio.output.byte_order = 
        B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	out_fmt->u.encoded_audio.output.buffer_size = 1024;
	out_fmt->u.encoded_audio.frame_size = 1152;
	out_fmt->u.encoded_audio.bit_rate = m_config->bitrate;

	m_format = in_fmt->u.raw_audio;
	m_format.frame_rate = out_fmt->u.encoded_audio.output.frame_rate;
	m_chunkSize = 4*1152*2*m_format.channel_count;
	return B_OK;
}


status_t
MP3Encoder::CommitHeader()
{
	if (m_is_wav_file == false && m_is_avi_file == false && m_is_QT_file == false)
		return B_OK;
	
	mpeg1waveformat header;

	header.wFormatTag = WAVE_FORMAT_MPEG;
	header.nChannels = m_format.channel_count;
	header.nSamplesPerSec = (int32)m_format.frame_rate;
	header.nAvgBytesPerSec = (int32)(m_config->bitrate/8);
	header.nBlockAlign = 1;
	header.wBitsPerSample = 0;
	header.cbSize = 0;
	AddTrackInfo('strf', (const char*)&header, 18);
	return B_OK;

#if 0
	// the extra data confused the heck out of Windows' mediaplayer and/or mp3 decoder,
	// so we'll leave it out for now
	header.cbSize = 22;
	header.fwHeadLayer = MPEG_LAYER3;
	header.dwHeadBitrate = (int32)m_config->bitrate;
	header.fwHeadMode = (m_format.channel_count != 1) ? MPEG_STEREO :
		MPEG_SINGLECHANNEL;
	header.fwHeadModeExt = 0;
	//	We might want to allow for settable emphasis?
	header.wHeadEmphasis = MPEG_EMPHASIS_NONE;
	//	We might want to add protection here (CRC)
	header.fwHeadFlags = MPEG_COPYRIGHT | MPEG_ORIGINALHOME | MPEG_ID_MPEG1;
	header.dwPTSLow = 0;
	header.dwPTSHigh = 0;

	AddTrackInfo(0, (const char*)&header, sizeof(header));
#endif
}

void
MP3Encoder::AttachedToTrack()
{

}


status_t
MP3Encoder::StartEncoder(void)
{
	if (!m_buffer) {
		m_buffer = (char *)malloc(m_chunkSize);
		if (m_buffer == NULL)
			return B_NO_MEMORY;
	}

	mp3_config curConfig = *m_config;

	curConfig.framerate = int32(m_format.frame_rate);
	curConfig.numchannels = m_format.channel_count;

	mp3_init(&curConfig, &m_cookie);
	m_flushed = false;

	return B_OK;
}



status_t
MP3Encoder::EncodeBuffer(void *src, int32 src_length, media_encode_info *mei)
{
	assert(src_length == 4*1152*m_format.channel_count*2);

	char * output = (char *)alloca(src_length);	/*	assume each packet will shrink, which is safe for the blade codec	*/
	int outputSize = mp3_encode(m_cookie, src, src_length, output);

	if (outputSize > 0) {
		return WriteChunk(output, outputSize, mei);
	}
	return B_OK;
}

status_t
MP3Encoder::Encode(const void *_in_buffer,
				   int64 num_frames,
				   media_encode_info *mei)
{
	const char *in_buffer = (const char *)_in_buffer;
	size_t src_length = num_frames * m_format.channel_count * (m_format.format & 0xf);
	status_t err;

	if (!m_cookie || m_flushed) {
		return B_NO_INIT;
	}

	mei->flags |= B_MEDIA_KEY_FRAME;

	if (m_bufAvail > 0) {
		if (m_bufAvail + xform(src_length) < m_chunkSize) {
			//	just copy into FIFO
			convert(in_buffer, src_length);
			return B_OK;
		}
		//	copy part into FIFO
		convert(in_buffer, src_length);
		//	encode FIFO
		err = EncodeBuffer(m_buffer, m_chunkSize, mei);
		//	update variables
		m_bufAvail = 0;
		if (err < 0) {
			return err;
		}
	}
	while (xform(src_length) >= m_chunkSize) {
		//	encode from buffer
		if (m_format.format == 0x2 && m_format.byte_order == B_MEDIA_HOST_ENDIAN) {
			err = EncodeBuffer(const_cast<char *>(in_buffer), m_chunkSize, mei);
			in_buffer += m_chunkSize;
			src_length -= m_chunkSize;
		}
		else {
			convert(in_buffer, src_length);
			err = EncodeBuffer(m_buffer, m_chunkSize, mei);
			m_bufAvail = 0;
		}
		if (err < 0) {
			return err;
		}
	}
	convert(in_buffer, src_length);
	return B_OK;
}

status_t
MP3Encoder::Flush()
{
	media_encode_info mei;
	mei.flags |= B_MEDIA_KEY_FRAME;
	
	if (m_cookie == 0) return B_OK;
	if (m_flushed) return B_NOT_ALLOWED;
	m_flushed = true;

	if (m_bufAvail > 0) {
		memset(m_buffer+m_bufAvail, 0, m_chunkSize-m_bufAvail);
		status_t err = EncodeBuffer(m_buffer, m_chunkSize, &mei);
		if (err < B_OK) return err;
	}
	return WriteChunk(m_buffer, mp3_encode(m_cookie, NULL, 0, m_buffer), &mei);
}

#pragma mark **** Parameter Web Stuff ****

enum _param_id
{
	P_BITRATE,
	P_QUALITY,
	P_COPYRIGHT,
	P_ORIGINAL,
	P_CRC
};

BParameterWeb *
MP3Encoder::Web()
{
	BParameterWeb* web;
	status_t status = GetParameterWeb(&web);
	return (status == B_OK) ? web : 0;
}

status_t 
MP3Encoder::GetParameterValue(int32 id, void *valu, size_t *size)
{
	bigtime_t when;
	return GetParameterValue(id, &when, valu, size);
}

status_t 
MP3Encoder::SetParameterValue(int32 id, const void *valu, size_t size)
{
	SetParameterValue(id, 0LL, valu, size);
	return B_OK;
}

BView *
MP3Encoder::GetParameterView()
{
	BView *ret=NULL;
	if(m_web)
	{
		BParameterWeb *_web;
		if(B_OK == GetParameterWeb(&_web))
		{
			if(_web)
			{
				ret=BMediaTheme::ViewFor(_web);
				if(!ret)
					delete _web;
			}
		}
	}
	return ret;
}

// internal
status_t 
MP3Encoder::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *ioSize)
{
	int32& ival = *(int32*)value;
	bool& bval = *(bool*)value;

	switch(id)
	{
	case P_BITRATE:
		if(*ioSize < 4)
			return B_NO_MEMORY;
		ival = m_config->bitrate;
		*ioSize=4;
		break;	

	case P_QUALITY:
		if(*ioSize < 4)
			return B_NO_MEMORY;
		ival = m_config->quality;
		*ioSize=4;
		break;

	case P_COPYRIGHT:
		if(*ioSize < 1)
			return B_NO_MEMORY;
		bval = m_config->copyright;
		*ioSize=1;
		break;	

	case P_ORIGINAL:
		if(*ioSize < 1)
			return B_NO_MEMORY;
		bval = m_config->original;
		*ioSize=1;
		break;	

	case P_CRC:
		if(*ioSize < 1)
			return B_NO_MEMORY;
		bval = m_config->crc;
		*ioSize=1;
		break;	

	default:
		return B_BAD_INDEX;
	}
	
	return B_OK;
}

// internal
void 
MP3Encoder::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
	const int32& val = *(int32*)value;
	const int32& bval = *(bool*)value;

printf("setting param %d to %d\n",id,val);	
	switch(id)
	{
	case P_BITRATE:
		if(size < 4)
			return;
		m_config->bitrate = val;
		break;
		
	case P_QUALITY:
		if(size < 4)
			return;
		m_config->quality = mp3EncQualityMode(val);
		break;
		
	case P_COPYRIGHT:
		if(size < 1)
			return;
		m_config->copyright = bval;
		break;
		
	case P_ORIGINAL:
		if(size < 1)
			return;
		m_config->original = bval;
		break;
		
	case P_CRC:
		if(size < 1)
			return;
		m_config->crc = bval;
		break;
	}
}

BParameterWeb *
MP3Encoder::InitParameterWeb(mp3_config* config)
{
	BParameterWeb* web = new BParameterWeb();
	BParameterGroup* main = web->MakeGroup("main");
	BParameterGroup* g;
	BDiscreteParameter* p;
	
	g = main->MakeGroup("Format");
	p = g->MakeDiscreteParameter(
		P_BITRATE, B_MEDIA_NO_TYPE, "Bitrate", B_BITRATE);
	p->AddItem(32000, "32000 kbits");
	p->AddItem(40000, "40000 kbits");
	p->AddItem(48000, "48000 kbits");
	p->AddItem(56000, "56000 kbits");
	p->AddItem(64000, "64000 kbits");
	p->AddItem(80000, "80000 kbits");
	p->AddItem(96000, "96000 kbits");
	p->AddItem(112000, "112000 kbits");
	p->AddItem(128000, "128000 kbits");
	p->AddItem(160000, "160000 kbits");
	p->AddItem(192000, "192000 kbits");
	p->AddItem(224000, "224000 kbits");
	p->AddItem(256000, "256000 kbits");
	p->AddItem(320000, "320000 kbits");
	for(int32 n = 0; n < p->CountItems(); n++)
		if(p->ItemValueAt(n) == config->bitrate)
		{
			p->SetValue(&n, sizeof(n), 0LL);
			break;
		}

	p = g->MakeDiscreteParameter(
		P_QUALITY, B_MEDIA_NO_TYPE, "Quality", B_GENERIC);
	p->AddItem(mp3EncQualityFast, "Fast");
	p->AddItem(mp3EncQualityMedium, "Medium");
	p->AddItem(mp3EncQualityHighest, "Highest");
	p->SetValue(&config->quality, sizeof(int32), 0LL);
	
	g = main->MakeGroup("Options");
	
	p = g->MakeDiscreteParameter(
		P_CRC, B_MEDIA_NO_TYPE, "CRC error protection", B_ENABLE);
	p->AddItem(0, "");
	p->AddItem(1, "");
	p->SetValue(&config->crc, sizeof(bool), 0LL);
	
	p = g->MakeDiscreteParameter(
		P_COPYRIGHT, B_MEDIA_NO_TYPE, "Label 'Copyrighted'", B_ENABLE);
	p->AddItem(0, "");
	p->AddItem(1, "");
	p->SetValue(&config->copyright, sizeof(bool), 0LL);

	p = g->MakeDiscreteParameter(
		P_ORIGINAL, B_MEDIA_NO_TYPE, "Label 'Original'", B_ENABLE);
	p->AddItem(0, "");
	p->AddItem(1, "");
	p->SetValue(&config->original, sizeof(bool), 0LL);

	return web;
}
