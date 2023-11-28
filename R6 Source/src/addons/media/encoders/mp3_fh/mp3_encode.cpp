#include <stdio.h>
#include <OS.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "mp3_encode.h"

#include "library/mp3encode.h"

// set this to 1 to enable extra code to add extra buffering in case
// the decoder can't consume all of the data (perhaps if it is not
// a multiple of 2304?)
#define ENABLE_BUFFERING_CODE 0

class mp3handle
{
	public:
		mp3EncHandle fh_handle;
		char *buffer;
		int realbuffersize;
		int usedbuffersize;

		mp3handle()
		{
			fh_handle = NULL;
			buffer = NULL;
			realbuffersize = 0;
			usedbuffersize = 0;
		}
		~mp3handle()
		{
			free(buffer);
		}
};


int
mp3_init(
	BPrivate::mp3_config * config,
	void ** cookie)
{
	int ret;

	mp3handle *h = new mp3handle;
	h->fh_handle = mp3EncOpen(config->framerate,config->numchannels,config->quality);

	mp3EncOutputFormat format;
	format.sampleRate=config->framerate;
	format.numChannels=config->numchannels;
	format.bitRate=config->bitrate;
	mp3EncSetOutputFormat(h->fh_handle,&format);
	
	mp3EncConfigurationPtr cfg = mp3EncGetCurrentConfiguration(h->fh_handle);
	cfg->copyrightBit=config->copyright;
	cfg->writeCRCheck=config->crc;
	mp3EncSetConfiguration(h->fh_handle,cfg);

	*cookie = (void*)h;
	return B_OK;
}

int
mp3_encode(
	void * cookie,
	const void * src,
	int size,
	void * dest)
{
	if (cookie == 0) return B_BAD_VALUE;
	mp3handle *h=(mp3handle*)cookie;

	char *realsrc = (char*)src;
	unsigned int realsize = size;
#if ENABLE_BUFFERING_CODE
	if(h->buffer && h->usedbuffersize)
	{
		if(size > h->realbuffersize - h->usedbuffersize)
		{
			printf("reallocating because %d > %d - %d\n",size, h->realbuffersize, h->usedbuffersize);
			h->buffer = (char*)realloc(h->buffer, h->usedbuffersize+size);
			h->realbuffersize = h->usedbuffersize+size;
		}
		printf("memcpy1(%08x, %08x, %d)\n",h->buffer+h->usedbuffersize+size, src, size);
		memcpy(h->buffer + h->usedbuffersize, src, size);
		h->usedbuffersize += size;
		
		realsrc = h->buffer;
		realsize = h->usedbuffersize;
	}
#endif
	mp3EncRetval ret;
	unsigned int bytesConsumed = 0;
	char *rsrc = (char*)realsrc;
	unsigned int rsiz = realsize;
	char *rdst = (char*)dest;
	unsigned int outputspace = size;
	unsigned int bc;
	ret = 0;
	do {
//		printf("mp3EncEncode(%08x, %08x, %d, %08x, %08x, %d\n",
//			h->fh_handle, (void*)rsrc, rsiz, &bc, (void*)rdst, outputspace);
		int outdata;
		outdata = mp3EncEncode (h->fh_handle, (void*)rsrc, rsiz, &bc, (void*)rdst, outputspace);
//		printf("%d of %d bytes consumed, return code %d\n",bc, rsiz,ret);
		if(ret<0)
			break;
		bytesConsumed += bc;
		rsrc += bc;
		rsiz -= bc;
		rdst += outdata;
		outputspace -= outdata;
		ret += outdata;
	} while(bc>0 && rsiz>0);
//	printf("total of %d of %d bytes consumed\n",bytesConsumed, realsize);
#if ENABLE_BUFFERING_CODE
	if(h->buffer && h->usedbuffersize)
	{
		// adjust the buffer that we just used
		h->usedbuffersize -= bytesConsumed;
		if(h->usedbuffersize != 0)
		{
			memmove(h->buffer,h->buffer + bytesConsumed, h->usedbuffersize);
		}
	}
	else if(bytesConsumed != realsize)
	{
		// we didn't use a buffer (either because there was none, or because it was empty),
		// but now we need a buffer
		if(!h->buffer)
		{
			h->realbuffersize = realsize - bytesConsumed;
			h->usedbuffersize = 0;
			h->buffer = (char*)malloc(h->realbuffersize);
		}
		h->usedbuffersize += realsize - bytesConsumed;
//		printf("memcpy2(%08x, %08x, %d)\n",h->buffer, realsrc + bytesConsumed, h->usedbuffersize);
		memcpy(h->buffer, realsrc + bytesConsumed, realsize - bytesConsumed);
	}
#else
	if(bytesConsumed != realsize)
		debugger("need to enable buffering code\n");
#endif
//	printf("%d of %d bytes consumed, return code %d (%08x, %d/%d)\n",
//		bytesConsumed, realsize, ret,
//		h->buffer,h->usedbuffersize,h->realbuffersize);
	return ret;
}

int
mp3_done(
	void * cookie)
{
	if (cookie == 0) return B_BAD_VALUE;
	delete (mp3handle*)cookie;
	return 0;
}

