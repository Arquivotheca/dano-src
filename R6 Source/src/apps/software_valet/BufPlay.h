#ifndef _BUFPLAY_H_
#define _BUFPLAY_H_

#include <AudioStream.h>

class BufPlay
{
public:
	BufPlay();
	~BufPlay();
	
	void Play(char *buffer, long size);
	
	static bool	dac_writer(void *arg, char *dac_buf, long count);

	char *bufP;
	long length;
	
	BDACStream	*dacSubscriber;
	
};

#endif
