#include <stdio.h>
#include <OS.h>
#include <assert.h>
#include <alloca.h>

#include "mp3_encode.h"

extern "C" {
#include "lame/lame.h"
}


int
mp3_init(
	BPrivate::mp3_config * config,
	void ** cookie)
{
	int ret;
	lame_global_flags *info=new lame_global_flags;

	ret = lame_init(info);

	if (ret != B_OK) {
		fprintf(stderr, "LAME: lame_init returns error\n");
		ret = B_ERROR;
	} else {
		info->in_samplerate = config->framerate;
		info->out_samplerate = config->framerate;
		info->mode = config->mode;
		if(info->mode == BPrivate::mp3_config::MODE_MONO)
			info->num_channels = 1; // one input channel
		info->brate = config->bitrate/1000;
		info->emphasis = 0;
		info->extension = 0;
		info->error_protection = config->crc;
		info->copyright = 1;
		info->original = 1;
		if(!config->psycho)
			info->quality=9; // 9 == worst quality, disables psycho, filtering, etc.

		ret = lame_init_params(info);

		if (ret != B_OK) {
			fprintf(stderr, "LAME: lame_init_params returns error\n");
			ret = B_ERROR;
		} else {
			*cookie = info;
		}
	}
	return ret;
}

int
mp3_encode(
	void * cookie,
	const void * src,
	int size,
	void * dest)
{
	if (cookie == 0) return B_BAD_VALUE;
	if (!src) {
		return lame_encode_finish((lame_global_flags*)cookie, (char*)dest, size);
	}

	int ret=B_ERROR;
	if(((lame_global_flags*)cookie)->num_channels == 2)
	{
		// LAME requires separate left and right channels, so we de-interleave the input first

		short *leftpcm=(short*)alloca(size/2);
		short *rightpcm=(short*)alloca(size/2);

		short *s=(short*)src;
		for(int i=0;i<size/4;i++) // number of int16
		{
			leftpcm[i] = *s++;
			rightpcm[i] = *s++;
		}
		ret = lame_encode_buffer((lame_global_flags*)cookie,leftpcm, rightpcm, size/4, (char*)dest, size);
	}
	else if(((lame_global_flags*)cookie)->num_channels == 1)
	{
		ret = lame_encode_buffer((lame_global_flags*)cookie, (short*)src, NULL, size/2, (char*)dest, size);
	}

	return ret;
}

int
mp3_done(
	void * cookie)
{
	if (cookie == 0) return B_BAD_VALUE;
	delete ((lame_global_flags*)cookie);
	return 0;
}

