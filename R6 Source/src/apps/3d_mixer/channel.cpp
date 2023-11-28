#include "channel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <MediaKit.h>
#include <Application.h>
#include <Entry.h>
#include <Path.h>    
#include "track_view.h"
#include <math.h>
//-----------------------------------------------------------

#define	S0	65536
#define	S1	128
#define	NS	(S0/S1)

//-----------------------------------------------------------

Channel	*make_channel(TrackViewer *a_viewer, const char *name)
{
	Channel	*c;

	c = new Channel(name, a_viewer);

	return c;
}

//-----------------------------------------------------------

void	Channel::handle_read_request()
{
}

//-----------------------------------------------------------

void	Channel::get_sample(long p, long scale, short *vmax, short *vmin)
{
}

//-----------------------------------------------------------

void	Channel::handle_write_request()
{
}

//-----------------------------------------------------------

float	Channel::GetGain()
{
	return the_gain;
}

//-----------------------------------------------------------

void	Channel::SetGain(float a_gain)
{
	the_gain = a_gain;
}

//-----------------------------------------------------------

void	Channel::SetOffset(long offset)
{
}

//-----------------------------------------------------------


void	Channel::HandleRequest()
{
	switch(command_type) {
		case	CMD_READ :
				handle_read_request();
				break;

		case	CMD_WRITE :
				handle_write_request();
				break;
	}

	command_type = CMD_DONE;
}


//-----------------------------------------------------------

void	Channel::PrivateTask()
{
}

//-----------------------------------------------------------

long		channel_task(void *p)
{
	return 0;
}

//-----------------------------------------------------------

		Channel::~Channel()
{
	free(buffer1);
	delete_sem(todo_sem);
}


//-----------------------------------------------------------

void	Channel::SetName(const char *n)
{
	strcpy(cname, n);
}

//-----------------------------------------------------------
// Initialise the channel object.
// open the main file, error should be checked after calling
// the constructor.
// buffer positions are set to a large negative value which will
// avoid trying to reuse some part of the previous state.
// Spawn the thread which will handle IO requests for the channel.
// Initialise the channel at position 0 (this acts like a preload).
//------------------------------------------------------------


		Channel::Channel(const char *filename, TrackViewer *a_viewer)
{
	long		i;
	char		buf[256];
	long		p;
	float		freq;	
	double		v;
	double		step;

	got_data = 0;
	viewer = a_viewer;
	strcpy(cname, filename);
	buffer1 = (short *)malloc(TOTAL_SIZE + EXTRA);
	todo_sem = create_sem(0, "channel_todo");

	v = 0;

	freq = 300 + rand()%900;

	step = (2*3.1415926/AVAIL_SAMPLE)*freq;

	for (i = 0; i < AVAIL_SAMPLE; i++) {
		buffer1[i] = (int)(1000.0 * sin(v));
		v += step;
	}
}

//-----------------------------------------------------------



void	Channel::handle_mini(short *p)
{
}

//-----------------------------------------------------------

void	Channel::build_mini_file()
{
}

//-----------------------------------------------------------

int		Channel::Error()
{
	return(error);
}

//-----------------------------------------------------------

long	Channel::PrivateRead(long pos, long count, char *p)
{
	return 0;
}


//-----------------------------------------------------------

long	Channel::PrivateWrite(long pos, long count, char *p)			
{
	return 0;
}

//-----------------------------------------------------------

long	Channel::GetSize()
{
	return 32768;
}

//-----------------------------------------------------------

void	Channel::set_server(long pos)
{
}

//-----------------------------------------------------------

short	*Channel::GetBuffer()
{
	return buffer1 + (EXTRA/2);
}

//-----------------------------------------------------------

short	*Channel::SetPos(long pos)
{
	got_data = viewer->fill_buffer(buffer1, AVAIL_SAMPLE + EXTRA/2, pos - (EXTRA/2));
	return buffer1;
}

//-----------------------------------------------------------
// 0.5 means center.
// 1   means all left.
// 0   means all right.

void	Channel::SetLeftRight(float v)
{
	float	sum;
	float	tot;
	
	v = 0.12+ (v * (1.0- (0.12*2.0)));

	//v *= 1.15;
	//v -= 0.1;
	if (v > 1.0) v = 1.0;
	if (v < 0.0) v = 0;

	
	right_weight = sin(v*3.1415926/2.0);
	left_weight = cos(v*3.1415926/2.0);

	tot = right_weight + left_weight;

	right_weight /= tot;
	left_weight /= tot;
}

//-----------------------------------------------------------

float	Channel::GetReverbe_level()
{
	return 0;
}

//-----------------------------------------------------------

long	Channel::GetReverbe_distance()
{
	return 0;
}

//-----------------------------------------------------------

