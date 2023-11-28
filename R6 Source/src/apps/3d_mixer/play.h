#include <AudioStream.h>
#include <BufferMsgs.h>
#include <BufferStream.h>
#include <BufferStreamManager.h>
#include <MediaDefs.h>
#include <SoundFile.h>
#include <Subscriber.h>

#include "channel.h"
#include "global.h"

#ifndef	PLAY_H
#define	PLAY_H

//-----------------------------------------------------------

extern short	glb_buffer[PLAY_SAMPLE_COUNT*2];
extern 			long	data_sem;
extern 			long	buffer_sem;

//-----------------------------------------------------------

void	init_stuff();
bool	record_func(void *data, char *buffer, size_t count, void *header);

//-----------------------------------------------------------

#endif
