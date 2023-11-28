/* test.c */

#include <malloc.h>	// for heapcheck
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "WMA_Dec_Emb_x86.h"

// Define DISCARD_OUTPUT for profiling.
//#define DISCARD_OUTPUT

// comment out DUMP_WAV define below to output a raw .pcm file rather than a .wav file.
// #define DUMP_WAV


#undef DUMP_WAV
#ifdef DUMP_WAV
#include "wavfileio.h"
#endif  /* DUMP_WAV */

//#define _MARKERDEMO_
//#define TEST_SPEED

// HEAP_DEBUG_CHECK is the same as the defines in msaudio.h 
// Although it is normally bad form to copy something out a .h instead of 
// including the .h, in this test program, we want to make sure we do not 
// inadvertently use anything from msaudio.h 
#if defined(HEAP_DEBUG_TEST) && defined(_DEBUG)
void HeapDebugCheck();
#define HEAP_DEBUG_CHECK HeapDebugCheck()
#else
#define HEAP_DEBUG_CHECK
#endif

/* global */

static tWMAFileHdrState g_hdrstate;
static tHWMAFileState g_state;
static tWMAFileHeader g_hdr;
static tWMAFileLicParams g_lic;

#ifndef TEST_SPEED
static FILE *g_fp = NULL;
static FILE *g_fpLic = NULL;
#endif /* TEST_SPEED */

#ifdef TEST_SPEED
unsigned char *g_pBuffer = NULL;
#else  /* TEST_SPEED */
const int MAX_BUFSIZE = WMA_MAX_DATA_REQUESTED;
unsigned char g_pBuffer[WMA_MAX_DATA_REQUESTED];
#endif /* TEST_SPEED */
tWMA_U32 g_cbBuffer = 0;

#ifdef TEST_SPEED
#else  /* TEST_SPEED */
unsigned char g_pBufLic[WMA_MAX_DATA_REQUESTED];
#endif /* TEST_SPEED */
tWMA_U32 g_cbBufLic = 0;

unsigned long g_ulStartFirstSec, g_ulEndSec;
unsigned long g_ulFullSec = 0;
unsigned long g_ulOutputSamples = 0;
unsigned int  g_SampleRate;

/* Portable Media ID
 *
 * This is different for each portable medium (PM).  Should be
 * provided by the application by reading an identification from
 * the PD
 */
unsigned char g_pmid[20] =
{
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x02, 0x03, 0x04,
};

//#define MAX_SAMPLES 256
#define MAX_SAMPLES 2048
short g_pLeft [MAX_SAMPLES * 2];
short *g_pRight = NULL;

#define STRING_SIZE 256
unsigned char g_szTitle[STRING_SIZE];
unsigned char g_szAuthor[STRING_SIZE];
unsigned char g_szCopyright[STRING_SIZE];
unsigned char g_szDescription[STRING_SIZE];
unsigned char g_szRating[STRING_SIZE];

#if 0
extern "C" void
SerialSendString(char *pcString)
{
    fprintf(stderr, pcString);
}

extern "C" void
SerialPrintf(const char *format, ...)
{
    va_list a;
    char s[256];
    va_start( a, format );
    vsprintf(s, format, a);
    va_end(a);
    SerialSendString(s);
}
#endif /* 0 */

/**************************************************/

void
WStrToStrN(unsigned char *pStr,
           unsigned char *pWStr,
           int n)
{
    if(pStr == NULL || pWStr == NULL)
    {
        return;
    }

    while(n-- > 0)
    {
        *pStr++ = *pWStr;
        pWStr += 2;
    }
    *pStr = 0;
}

void WMADebugMessage(const char* pszFmt, ... )
{
    va_list vargs;
    va_start(vargs, pszFmt);
    vfprintf(stderr, pszFmt, vargs );
    va_end(vargs);
}


/* WMAFileCBGetData */

tWMA_U32 WMAFileCBGetData (
    tHWMAFileState hstate,
    tWMA_U32 offset,
    tWMA_U32 num_bytes,
    unsigned char **ppData)
/*
tWMA_U32 WMAFileCBGetData (
    tHWMAFileState *state,
    tWMA_U32 offset,
    tWMA_U32 num_bytes,
    unsigned char **ppData)
*/
{
    tWMA_U32 ret;

#ifdef TEST_SPEED

    if(offset >= g_cbBuffer)
    {
        *ppData = g_pBuffer + g_cbBuffer;
        ret = 0;
    }
    else
    {
        *ppData = g_pBuffer + offset;

        if(offset + num_bytes > g_cbBuffer)
        {
            ret = g_cbBuffer - offset;
        }
        else
        {
            ret = num_bytes;
        }
    }

#else  /* TEST_SPEED */

    tWMA_U32 nWanted = num_bytes <= (tWMA_U32) MAX_BUFSIZE ? num_bytes : (tWMA_U32) MAX_BUFSIZE;
    if(num_bytes != nWanted)
    {
        fprintf(stderr, "** WMAFileCBGetData: Requested too much (%lu).\n",
                num_bytes);
    }

    fseek (g_fp, offset, SEEK_SET);
    ret = fread (g_pBuffer, 1, (size_t)nWanted, g_fp);

    g_cbBuffer = ret;

    *ppData = g_pBuffer;

#endif /* TEST_SPEED */

    fprintf(stderr, "++ WMAFileCBGetData: %lu bytes from %lu.\n",
            ret, offset);

    return ret;
}


/* WMAFileCBGetLicenseData */
//g_plicData = WMAGetLicenseStore(g_state,g_pLicenseLength);

tWMA_U32 WMAFileCBGetLicenseData (
    tHWMAFileState *state,
    tWMA_U32 offset,
    tWMA_U32 num_bytes,
    unsigned char **ppData)
{
    tWMA_U32        ret;
    unsigned char  *g_plicData;
    tWMA_U32        g_LicenseLength;
    tWMA_U32        nWanted;
    g_plicData = WMAGetLicenseStore(g_state, &g_LicenseLength);

#ifdef TEST_SPEED

    if(offset >= g_cbBufLic)
    {
        *ppData = g_pBufLic + g_cbBufLic;
        ret = 0;
    }
    else
    {
        *ppData = g_pBufLic + offset;

        if(offset + num_bytes > g_cbBufLic)
        {
            ret = g_cbBufLic - offset;
        }
        else
        {
            ret = num_bytes;
        }
    }

#else  /* TEST_SPEED */

    nWanted = num_bytes <= (tWMA_U32) MAX_BUFSIZE ? num_bytes : (tWMA_U32) MAX_BUFSIZE;
    if(num_bytes != nWanted)
    {
        fprintf(stderr, "** WMAFileCBGetLicenseData: Requested too much (%lu).\n",
                num_bytes);
    }


    if (g_LicenseLength) {
        g_cbBufLic = ((offset +nWanted) > g_LicenseLength) ? (g_LicenseLength - offset): nWanted;
        ret = g_cbBufLic;
        *ppData = g_plicData + offset;
    }
    else {
        fseek (g_fpLic, offset, SEEK_SET);
        ret = fread (g_pBufLic, 1, (size_t)nWanted, g_fpLic);

        g_cbBufLic = ret;

        *ppData = g_pBufLic;

    }



#endif /* TEST_SPEED */

    fprintf(stderr, "++ WMAFileCBGetLicenseData: %lu bytes from %lu.\n",
            ret, offset);

    return ret;
}


/* main */

int main (int argc, char *argv[])
{
    MarkerEntry *pEntry;
    int iMarkerNum;
    tWMA_U32 msSeekTo;
#ifdef _MARKERDEMO_
    tWMA_U32 msReturned;
#endif
    tWMA_U32 LicenseLength;

    tWMAFileStatus rc;
//    const char *strOut = "d:\\test\\output.pcm";
    const char *strLic = "drmv1pm.lic";
    tWMAFileContDesc desc;
    unsigned char szTemp [STRING_SIZE];
#ifndef DISCARD_OUTPUT
#ifdef DUMP_WAV
    WavFileIO *pwfioOut = wfioNew ();
    WAVEFORMATEX wfx;
#else   /* DUMP_WAV */
    FILE *pfOutPCM = NULL;
#endif /* DUMP_WAV */
#endif // !DISCARD_OUTPUT
	int iRV = 1;	// assume error exit return value
//	FUNCTION_PROFILE(fpDecode);

    if (argc < 3) {
        fprintf(stderr, "** Too few arguments.\n");
        goto lexit;
    }

	g_fp = fopen ("/tmp/test","rb");

    g_fp = fopen (argv [1], "rb");
    if (g_fp == NULL) {
        fprintf(stderr, "** Cannot open %s.\n", argv [1]);
        goto lexit;
    }

    /* init struct */
    memset ((void *)&g_hdrstate, 0, sizeof(g_hdrstate));
    memset ((void *)&g_state, 0, sizeof(g_state));
    memset ((void *)&g_hdr, 0, sizeof(g_hdr));

    /* test the checking API */

    rc = WMAFileIsWMA (&g_hdrstate);
    if(rc != cWMA_NoErr)
    {
        fprintf(stderr, "** The file is not a WMA file.\n");
        goto lexit;
    }

    /* init the decoder */

    rc = WMAFileDecodeInit (&g_state);
    if(rc != cWMA_NoErr)
    {
        fprintf(stderr, "** Cannot initialize the WMA decoder.\n");
        goto lexit;
    }

    /* get header information */

    rc = WMAFileDecodeInfo (g_state, &g_hdr);
    if(rc != cWMA_NoErr)
    {
        fprintf(stderr, "** Failed to retrieve information.\n");
        goto lexit;
    }

    /* set up the content description struct */

    memset((void *)&desc, 0, sizeof(desc));
    desc.title_len = STRING_SIZE/2;
    desc.pTitle    = (unsigned char *)g_szTitle;
    desc.author_len = STRING_SIZE/2;
    desc.pAuthor    = (unsigned char *)g_szAuthor;
    desc.copyright_len = STRING_SIZE/2;
    desc.pCopyright    = (unsigned char *)g_szCopyright;
    desc.description_len = STRING_SIZE/2;
    desc.pDescription    = (unsigned char *)g_szDescription;
    desc.rating_len = STRING_SIZE/2;
    desc.pRating    = (unsigned char *)g_szRating;
fprintf(stderr,"g_state: %08x\n",g_state);
#if 0
    /* get content description */

    rc = WMAFileContentDesc (g_state, &desc);
    if(rc != cWMA_NoErr)
    {
        fprintf(stderr, "** Failed to retrieve content description.\n");
        goto lexit;
    }

    /* display information */

    WStrToStrN(szTemp, desc.pTitle, desc.title_len);
    printf("++            Song title: %s\n", szTemp);
    WStrToStrN(szTemp, desc.pAuthor, desc.author_len);
    printf("++                Author: %s\n", szTemp);
    WStrToStrN(szTemp, desc.pCopyright, desc.copyright_len);
    printf("++             Copyright: %s\n", szTemp);
    WStrToStrN(szTemp, desc.pDescription, desc.description_len);
    printf("++           Description: %s\n", szTemp);
    WStrToStrN(szTemp, desc.pRating, desc.rating_len);
    printf("++                Rating: %s\n", szTemp);
#endif
    printf("++ WMA bitstream version: %d\n", g_hdr.version);
    printf("++         sampling rate: ");
    switch(g_hdr.sample_rate)
    {
    case cWMA_SR_08kHz:
        printf("8000 Hz\n");
		g_SampleRate = 8000;
        break;
    case cWMA_SR_11_025kHz:
        printf("11025 Hz\n");
		g_SampleRate = 11025;
        break;
    case cWMA_SR_16kHz:
        printf("16000 Hz\n");
		g_SampleRate = 16000;
        break;
    case cWMA_SR_22_05kHz:
        printf("22050 Hz\n");
		g_SampleRate = 22050;
        break;
    case cWMA_SR_32kHz:
        printf("32000 Hz\n");
		g_SampleRate = 32000;
        break;
    case cWMA_SR_44_1kHz:
        printf("44100 Hz\n");
		g_SampleRate = 44100;
        break;
    case cWMA_SR_48kHz:
        printf("48000 Hz\n");
		g_SampleRate = 48000;
        break;
    default:
        printf("Unknown??? [%d]\n", g_hdr.sample_rate);
		g_SampleRate = g_hdr.sample_rate;
        break;
    }
    printf("++         # of channels: %d\n", g_hdr.num_channels);
    printf("++              bit-rate: %ld bps\n", g_hdr.bitrate);
    printf("++              duration: %ld ms\n", g_hdr.duration);
    printf("++           DRM content: %s\n", g_hdr.has_DRM ? "Yes" : "No");

    /* if DRM, init with the license file */

    if(g_hdr.has_DRM)
    {
        WMAGetLicenseStore(g_state, &LicenseLength);

        g_lic.pPMID = (unsigned char *)&g_pmid;
        g_lic.cbPMID = sizeof(g_pmid);
        if (LicenseLength == 0) {
            g_fpLic = fopen (strLic, "rb");
            if(g_fpLic == NULL)
            {
                fprintf(stderr, "** Cannot open the license file %s.\n", strLic);
                goto lexit;
            }
        }

        rc = WMAFileLicenseInit (g_state, &g_lic, CHECK_ALL_LICENSE);
        if(rc != cWMA_NoErr)
        {
            fprintf(stderr, "** WMALicenseInit failed (%u).\n", rc);
            goto lexit;
        }

        if (LicenseLength == 0) {
            fclose(g_fpLic);
            g_fpLic = NULL;
        }
    }

#ifndef DISCARD_OUTPUT
#ifdef DUMP_WAV
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nSamplesPerSec  = g_SampleRate;
    wfx.nChannels       = g_hdr.num_channels;
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = ((wfx.wBitsPerSample + 7) / 8) * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
    wfx.cbSize          = 0;

    if(wfioOpen (pwfioOut, argv [2], &wfx, sizeof(wfx), wfioModeWrite) != 0) {
        fprintf(stderr, "Can't create file\n");
        exit(1);
    }
#else   /* DUMP_WAV */
    pfOutPCM = fopen (argv [2], "wb");
    if (pfOutPCM == NULL) {
        fprintf(stderr, "** Cannot open output file %s.\n", argv [2]);
        goto lexit;
    }
#endif // DUMP_WAV
#endif // !DISCARD_OUTPUT

#if 0
    /* testing the seek */

    {
        tWMA_U32 msSeekTo = 3000;
        tWMA_U32 msReturned = WMAFileSeek(&g_state, 3000);
        fprintf(stderr, "++ Seek to %d and actually gotten to %d\n",
                msSeekTo, msReturned);
    }
#endif /* 0 */

	HEAP_DEBUG_CHECK;

    /* decoding loop */
#ifdef PROFILE
    Profiler_init(_T("profile.txt"));
	FUNCTION_PROFILE_START(&fpDecode,MSAC_DECODE_PROFILE);
#endif  // PROFILE
	g_ulStartFirstSec = time(NULL);	

    iMarkerNum = WMAGetMarkers(&g_hdrstate, &pEntry);

    msSeekTo = 0;

    do
    {
        rc = WMAFileDecodeData (g_state);
#ifdef _MARKERDEMO_
        if (msSeekTo==0) 
            msSeekTo ++;
        if (msSeekTo == 1) {
            msSeekTo = 2;
            msSeekTo = pEntry[3].m_dwSendTime;
//            msSeekTo = pEntry[1].m_qtime.dwLo/10000;
            msReturned = WMAFileSeek(g_state, msSeekTo);
        }
#endif //_MARKERDEMO_

        if(rc != cWMA_NoErr)
        {
			g_ulEndSec = time(NULL);
			if ( rc == cWMA_NoMoreFrames || rc == cWMA_Failed )
				iRV = 0;		// normal exit
			else
				iRV = 2;		// error decoding data
            break;
        }

        do
        {
            tWMA_U32 num_samples;
//            short *pL = g_pLeft;
//            short *pR = g_pRight;

			HEAP_DEBUG_CHECK;
            num_samples = WMAFileGetPCM (g_state, g_pLeft, g_pRight, MAX_SAMPLES);
            if (num_samples == 0)
            {
                /* no more, so on with the decoding... */
                break;
            }

#ifndef DISCARD_OUTPUT
//			FUNCTION_PROFILE_STOP(&fpDecode);
#ifdef DUMP_WAV
            wfioWrite (pwfioOut, (U8*) g_pLeft, num_samples * g_hdr.num_channels * sizeof (short));
#else   /* DUMP_WAV */
            fwrite (g_pLeft, sizeof (short), num_samples * g_hdr.num_channels, pfOutPCM);
#endif // DUMP_WAV
//			FUNCTION_PROFILE_START(&fpDecode,MSAC_DECODE_PROFILE);
#endif // !DISCARD_OUTPUT
			g_ulOutputSamples += num_samples;
        } while (1);

    } while (1);

	g_ulFullSec   = g_ulEndSec - g_ulStartFirstSec;
//	FUNCTION_PROFILE_STOP(&fpDecode);

	HEAP_DEBUG_CHECK;

//    fprintf(stderr,"Full Read and Decode took %d s.\r\nPlay Time would be %d s.\r\n",
//		g_ulFullSec, (int)((float)g_ulOutputSamples / (float)g_SampleRate ) );

#ifdef PROFILE
	{
		char szProfileMsg[400];
		sprintf( szProfileMsg, "%d bps\r\n%d Hz,%d chans\r\n%d ms\r\n%s\r\n%s\r\n%s\r\nTrack = %d\r\nClock = %d\r\nIdle  = %d\r\n\r\n", 
			g_hdr.bitrate, g_SampleRate, g_hdr.num_channels, g_hdr.duration,
			desc.pTitle, desc.pAuthor, desc.pDescription, 
			0, g_ulFullSec, 0
			);
		Profiler_closeEX((unsigned int)(g_hdr.duration),g_ulFullSec*1000,szProfileMsg);
	}
#endif  // PROFILE

lexit:

    /* clean up */

	HEAP_DEBUG_CHECK;

    if (g_fpLic) {
        fclose (g_fpLic);
        g_fpLic = NULL;
    }

#ifndef DISCARD_OUTPUT
#ifdef DUMP_WAV
    if (pwfioOut)
        wfioDelete (pwfioOut);
#else   /* DUMP_WAV */
    if (pfOutPCM) {
        fclose (pfOutPCM);
        pfOutPCM = NULL;
    }
#endif // DUMP_WAV
#endif // !DISCARD_OUTPUT

    if (g_fp) {
        fclose (g_fp);
        g_fp = NULL;
    }

	WMAFileDecodeClose (&g_state);

	HEAP_DEBUG_CHECK;
	
	return(iRV);
}

// Some compile time warning messages
#ifndef PLATFORM_SPECIFIC_COMPILER_MESSAGE
#	define COMPILER_MESSAGE(x)         message(x)
#endif
#ifdef PROFILE
#	pragma COMPILER_MESSAGE(__FILE__ "(555) : Warning - PROFILE Enabled.")
#endif
#ifdef DISCARD_OUTPUT
#	pragma COMPILER_MESSAGE(__FILE__ "(558) : Warning - DISCARD_OUTPUT Enabled.")
#endif
