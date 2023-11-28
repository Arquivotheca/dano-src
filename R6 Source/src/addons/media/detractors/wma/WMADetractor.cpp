#include <File.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <Entry.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaTrack.h>
#include <Extractor.h>
#include <DataIO.h>
#include <Locker.h>
#include <Autolock.h>

#include "WMADetractor.h"


const int32 BLOCKSIZEINFRAMES = 256;


WMADetractor *currentdetractor;

class WMAAutolock: public BAutolock
{
	private:
		static BLocker lock;
	public:
		WMAAutolock(WMADetractor *which)
			: BAutolock(&lock)
		{
			currentdetractor=which;
		}
		
		~WMAAutolock()
		{
			currentdetractor=NULL;
		}
};

BLocker WMAAutolock::lock("wma_locker");


//#define DEBUG printf
#define DEBUG if (0) printf
#define SEEK(x) //printf x
#define SEQ(x) //printf x


extern "C" const char * mime_type_detractor = "audio/x-raw";

extern "C" Detractor* instantiate_detractor()
{
	return new WMADetractor;
}


WMADetractor::WMADetractor()
{
	fCurrentFrame = -1;


	unsigned char data[]= {
    	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x02, 0x03, 0x04,
	};
	memcpy(g_pmid,data,20);
}

WMADetractor::~WMADetractor()
{
}


status_t WMADetractor::SetTo(const entry_ref *ref)
{
	DEBUG("WMADetractor::SetTo(entry_ref)\n");
	return SetTo(new BFile(ref,B_READ_ONLY));
}

status_t WMADetractor::SetTo(BDataIO *source)
{
	WMAAutolock lock(this);

	DEBUG("WMADetractor(%08x)::SetTo(BDataIO) (%08x)\n",int(this),int(&g_hdrstate));
	fSource=dynamic_cast<BPositionIO*>(source);
	if(!fSource)
		return B_ERROR;
	
    tWMAFileStatus rc;

    memset ((void *)&g_hdrstate, 0, sizeof(g_hdrstate));

    rc = WMAFileIsWMA (&g_hdrstate);
    if(rc != cWMA_NoErr)
    {
        fprintf(stderr, "** The file is not a WMA file.\n");
        return B_ERROR;
    }

    /* init the decoder */

    memset ((void *)&g_state, 0, sizeof(g_state));
//    memset ((void *)&g_hdr, 0, sizeof(g_hdr));
	DEBUG("----WMAFileDecodeInit g_state=%08x, g_hdr=%08x\n",int(&g_state),int(&g_hdr));
    rc = WMAFileDecodeInit (&g_state);
    if(rc != cWMA_NoErr)
    {
        fprintf(stderr, "** Cannot initialize the WMA decoder.\n");
        return B_ERROR;
    }
    
    DEBUG("g_state now %08x\n",int(g_state));

    /* get header information */
    rc = WMAFileDecodeInfo (g_state, &g_hdr);
    if(rc != cWMA_NoErr)
    {
        fprintf(stderr, "** Failed to retrieve information.\n");
        return B_ERROR;
    }

    if(g_hdr.has_DRM)
    {
	    tWMA_U32 LicenseLength;
        WMAGetLicenseStore((tWMAFileHdrState*)g_state, &LicenseLength);

        g_lic.pPMID = (unsigned char *)&g_pmid;
        g_lic.cbPMID = sizeof(g_pmid);
        if (LicenseLength == 0) {
		  //  const char *strLic = "drmv1pm.lic";
		 	const char *strLic = "/etc/DRMv12.lic";
            g_fpLic = fopen (strLic, "rb");
            if(g_fpLic == NULL)
            {
                fprintf(stderr, "** Cannot open the license file %s.\n", strLic);
                return B_ERROR;
            }
        }

        rc = WMAFileLicenseInit (g_state, &g_lic, CHECK_ALL_LICENSE);
        if(rc != cWMA_NoErr)
        {
            fprintf(stderr, "** WMALicenseInit failed (%u).\n", rc);
            return B_ERROR;
        }

        if (LicenseLength == 0) {
            fclose(g_fpLic);
            g_fpLic = NULL;
        }
    }

    MarkerEntry *pEntry;
    int iMarkerNum;
    iMarkerNum = WMAGetMarkers(&g_hdrstate, &pEntry);

	fCurrentFrame = 0;
	fNeedToDecode = true;
	return B_OK;
}

status_t WMADetractor::InitCheck() const
{
	DEBUG("WMADetractor::InitCheck: %d\n",fCurrentFrame>=0);
	return (fCurrentFrame>=0);
}

status_t WMADetractor::GetFileFormatInfo(media_file_format *mfi) const
{
	DEBUG("WMADetractor::GetFileFormatInfo\n");
    strcpy(mfi->mime_type,      "audio/x-wma");
    strcpy(mfi->pretty_name,    "Windows Media");
    strcpy(mfi->short_name,     "WMA");
    strcpy(mfi->file_extension, "wma");

    mfi->family = B_ANY_FORMAT_FAMILY;

    mfi->capabilities = media_file_format::B_READABLE              |
                        media_file_format::B_IMPERFECTLY_SEEKABLE  |
                        media_file_format::B_PERFECTLY_SEEKABLE    |
                        media_file_format::B_KNOWS_RAW_AUDIO       |
                        media_file_format::B_KNOWS_ENCODED_AUDIO;

	return B_OK;
}

const char* WMADetractor::Copyright(void) const
{
	DEBUG("WMADetractor::Copyright\n");
	return "Copyright 2000 Be, Inc.";
}

int32 WMADetractor::CountTracks() const
{
	DEBUG("WMADetractor::CountTracks\n");
	return 1;
}

status_t WMADetractor::GetCodecInfo(int32 tracknum, media_codec_info *mci) const
{
	DEBUG("WMADetractor::GetCodecInfo\n");
	if(tracknum!=0) return B_ERROR;
	sprintf(mci->pretty_name, "Windows Media Audio, %d kbps", int(g_hdr.bitrate/1000));
	strcpy(mci->short_name, "Windows Media");
	return B_OK;
}

status_t WMADetractor::EncodedFormat(int32 tracknum, media_format *out_format) const
{
	DEBUG("WMADetractor::EncodedFormat\n");
	if(tracknum!=0) return B_ERROR;
#if 0
	out_format->type=B_MEDIA_RAW_AUDIO;
	out_format->u.raw_audio.frame_rate = SampleRate();
	out_format->u.raw_audio.channel_count = g_hdr.num_channels;
	out_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	out_format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	out_format->u.raw_audio.buffer_size = BLOCKSIZEINFRAMES*2*g_hdr.num_channels;
#else
	out_format->type = B_MEDIA_ENCODED_AUDIO;
    out_format->u.encoded_audio.output.frame_rate = SampleRate();
	out_format->u.encoded_audio.output.channel_count = g_hdr.num_channels;
	out_format->u.encoded_audio.encoding = (media_encoded_audio_format::audio_encoding)0;
    out_format->u.encoded_audio.bit_rate = g_hdr.bitrate;
    out_format->u.encoded_audio.output.buffer_size = BLOCKSIZEINFRAMES*2*g_hdr.num_channels;
#endif
	return B_OK;
}

status_t WMADetractor::DecodedFormat(int32 tracknum, media_format *inout_format)
{
	DEBUG("WMADetractor::DecodedFormat\n");
	if(tracknum!=0) return B_ERROR;
	inout_format->type=B_MEDIA_RAW_AUDIO;
	inout_format->u.raw_audio.frame_rate = SampleRate();
	inout_format->u.raw_audio.channel_count = g_hdr.num_channels;
	inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	inout_format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	inout_format->u.raw_audio.buffer_size = BLOCKSIZEINFRAMES*2*g_hdr.num_channels;
	return B_OK;
}

int64    WMADetractor::CountFrames(int32 tracknum) const
{
	DEBUG("WMADetractor::CountFrames\n");
	if(tracknum!=0) return B_ERROR;

	return int64(double(g_hdr.duration)*SampleRate()/1000.0);
}

bigtime_t WMADetractor::Duration(int32 tracknum) const
{
	DEBUG("WMADetractor::Duration\n");
	if(tracknum!=0) return B_ERROR;

	return bigtime_t(g_hdr.duration*1000.0);
}

int64    WMADetractor::CurrentFrame(int32 tracknum) const
{
	DEBUG("WMADetractor::CurrentFrame\n");
	if(tracknum!=0) return B_ERROR;

	return fCurrentFrame;
}

bigtime_t WMADetractor::CurrentTime(int32 tracknum) const
{
	DEBUG("WMADetractor::CurrentTime\n");
	if(tracknum!=0) return B_ERROR;
	return bigtime_t(fCurrentFrame*1000000/SampleRate());
}

status_t WMADetractor::ReadFrames(int32 tracknum, void *out_buffer, int64 *out_frameCount, media_header *mh = NULL)
{
//	DEBUG("WMADetractor::ReadFrames\n");
	if(tracknum!=0) return B_ERROR;
	WMAAutolock lock(this);
    tWMAFileStatus rc;

	*out_frameCount = 0;
	short *output=(short*)out_buffer;

	if(mh)
		mh->start_time = bigtime_t(fCurrentFrame * 1000000.0 / g_hdr.sample_rate);

	do {
		if(fNeedToDecode)
		{
			fNeedToDecode = false;
			rc = WMAFileDecodeData (g_state);
			if(rc != cWMA_NoErr)
			{
				printf("decode error: %d\n",rc);
				break;
			}
		}
	
		do {
			tWMA_U32 num_samples;
			uint32 samplestodecode = max_c(BLOCKSIZEINFRAMES-*out_frameCount,0);
			num_samples = WMAFileGetPCM (g_state, output, NULL, samplestodecode);
			
			fCurrentFrame += num_samples;
			*out_frameCount += num_samples;
			output+=num_samples*2;

			if(num_samples<samplestodecode)
			{
				fNeedToDecode = true;
				break;
			}

		} while (*out_frameCount < BLOCKSIZEINFRAMES);
	} while(*out_frameCount < BLOCKSIZEINFRAMES);
//	printf("returning %Ld frames\n",*out_frameCount);
	return B_OK;
}
							   
status_t WMADetractor::SeekTo(int32 tracknum, int32 to_what, bigtime_t *inout_time, int64 *inout_frame, int32 /*flags*/)
{
	DEBUG("WMADetractor::SeekTo\n");
	if(tracknum!=0) return B_ERROR;
	WMAAutolock lock(this);

	int32 seektime;

	if(to_what == B_SEEK_BY_FRAME)
		seektime = int32(*inout_frame * 1000 / SampleRate());
	else
		seektime = int32(*inout_time / 1000);

	DEBUG("seeking to %ld\n",seektime);
	seektime = WMAFileSeek (g_state, seektime);
	DEBUG("now at %ld\n",seektime);
	fCurrentFrame = int64(seektime * SampleRate() / 1000);
	*inout_frame = fCurrentFrame;
	*inout_time = seektime * 1000;
	fNeedToDecode = true;
	return B_OK;
}

status_t WMADetractor::FindKeyFrameForFrame(int32 tracknum, int64* /* inout_frame*/, int32 /*flags*/) const
{
	DEBUG("WMADetractor::FindKeyFrameForFrame\n");
	if(tracknum!=0) return B_ERROR;
	return B_OK;
}

// ========================================================================

float WMADetractor::SampleRate() const
{
	float g_SampleRate;

    switch(g_hdr.sample_rate)
    {
    case cWMA_SR_08kHz:
        DEBUG("8000 Hz\n");
		g_SampleRate = 8000;
        break;
    case cWMA_SR_11_025kHz:
        DEBUG("11025 Hz\n");
		g_SampleRate = 11025;
        break;
    case cWMA_SR_16kHz:
        DEBUG("16000 Hz\n");
		g_SampleRate = 16000;
        break;
    case cWMA_SR_22_05kHz:
        DEBUG("22050 Hz\n");
		g_SampleRate = 22050;
        break;
    case cWMA_SR_32kHz:
        DEBUG("32000 Hz\n");
		g_SampleRate = 32000;
        break;
    case cWMA_SR_44_1kHz:
        DEBUG("44100 Hz\n");
		g_SampleRate = 44100;
        break;
    case cWMA_SR_48kHz:
        DEBUG("48000 Hz\n");
		g_SampleRate = 48000;
        break;
    default:
        DEBUG("Unknown??? [%d]\n", g_hdr.sample_rate);
		g_SampleRate = g_hdr.sample_rate;
        break;
    }
    return g_SampleRate;
}

