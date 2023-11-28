#include "channel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <MediaKit.h>
#include <Application.h>
#include <Entry.h>
#include <Path.h>    


//-----------------------------------------------------------

#define	S0	65536
#define	S1	128
#define	NS	(S0/S1)

//-----------------------------------------------------------

void	Channel::handle_read_request()
{
	memcpy(buffer2, buffer1 + BACK_BUF + (AVAIL_BUF - BACK_BUF), BACK_BUF);
	PrivateRead(new_pos+BACK_BUF, AVAIL_BUF, buffer2+BACK_BUF);
	buffer_2_pos = new_pos;
}

//-----------------------------------------------------------

void	Channel::get_sample(long p, long scale, short *vmax, short *vmin)
{
	long	result;
	short	v;
	long	scale2;
	long	max;
	long	min;
	long	i;
	long	m;
	long	db;


	if (scale >= (S1*2)) {
		db = p;
		p = p / S1;
		p = p * 2;
again0:;
		if ((p >= fast_buffer_pos) && (p < (fast_buffer_pos + FBS - 1))) {
			*vmax = fast_buffer[p-fast_buffer_pos];
			*vmin = fast_buffer[p-fast_buffer_pos + 1];
			return;
		} 
		
		lseek(mini_file_id, (p) * sizeof(short), SEEK_SET);
		memset(fast_buffer, 0, sizeof(fast_buffer));
		result = read(mini_file_id, &fast_buffer, sizeof(fast_buffer));
		fast_buffer_pos = p;
		goto again0;
	}
	
again:;

	scale2 = scale / 2;
	if (scale2 == 0)
		scale2 = 1;
		
	if (((p - scale2) >= fast_buffer_pos) && ((p + scale2) < (fast_buffer_pos + (FBS)))) {

		max = -65000;
		min = 65535;
		m = (p - fast_buffer_pos) - scale2;
		
		for (i = m; i < (m + scale2); i++) {
			if (fast_buffer[i] > max) {
				max = fast_buffer[i];
			}
			if (fast_buffer[i] < min) {
				min = fast_buffer[i];
			}
		}	
		*vmax = max;
		*vmin = min;
		return;
	}
	
	lseek(file_id, (p - scale2) *sizeof(short), SEEK_SET);
	result = read(file_id, &fast_buffer, sizeof(fast_buffer));
	fast_buffer_pos = p - scale2;
	goto again;
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
	if (a_gain < 0)
		a_gain = 0;
		
	the_gain = a_gain;
}

//-----------------------------------------------------------

void	Channel::SetOffset(long offset)
{
	the_offset = offset;
	SetPos(0);
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
	while(1) {
		acquire_sem(todo_sem);
		HandleRequest();
	}
}

//-----------------------------------------------------------

long		channel_task(void *p)
{
	Channel	*c;

	c = (Channel *)p;
	c->PrivateTask();
	return 0;
}

//-----------------------------------------------------------

		Channel::~Channel()
{
	if (file_id < 0)
		return;
		
	while(command_type != CMD_DONE)
		snooze(4000);
	free(buffer1);
	free(buffer2);
	kill_thread(task);
	delete_sem(todo_sem);
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


		Channel::Channel(char *filename)
{
	long	i;
	char	buf[256];
	long	p;
	
	
	app_info	info;
	entry_ref	eref;
	BEntry 		*entry;
	BPath		path;
	char		tmpp[256];

    be_app->GetAppInfo(&info);
    eref = info.ref;
    entry = new BEntry(&eref, FALSE);
    entry->GetPath(&path);
    path.GetParent(&path);
    strcpy(tmpp, path.Path());

    delete entry;                  
	
	fast_buffer_pos = -1024*1024;
	strcpy(cname, filename);
	sprintf(buf, "%s/%s", tmpp, filename);
	file_id = open(buf, O_BINARY | O_RDWR);
	if (file_id < 0) {
		printf("Could not open %s\n", buf);
		printf("Channel will be invalid\n");
		error = -1;
	}	
	else {
		the_offset = 0;
		the_gain = 1.0;
		buffer_1_pos = -1024*1024;
		buffer_2_pos = -1024*1024;
		buffer1 = (char *)malloc(TOTAL_SIZE*2);
		buffer2 = (char *)malloc(TOTAL_SIZE*2);
		todo_sem = create_sem(0, "channel_todo");
		left_weight = 0.5;
		right_weight = 0.5;
		task = spawn_thread(channel_task,
							"channel_task",
							B_REAL_TIME_PRIORITY,
							this);
		resume_thread(task);
		command_type = CMD_DONE;
		for (i= 0; i < 8; i++)
			filter_raw[i] = 0;
		filter_raw[7] = 1;
		NormalizeFilter();
		SetPos(0);
	}
	sprintf(buf, "%s/%s.mini", tmpp, cname);
	mini_file_id = open(buf, O_BINARY | O_RDWR);
	if (mini_file_id < 0) { 
		mini_file_id = open(buf, O_BINARY | O_RDWR | O_CREAT);
		build_mini_file();
	}	
}

//-----------------------------------------------------------



void	Channel::handle_mini(short *p)
{
	short	min;
	short	max;
	long	i;
	long	v;
	
	min = 32767;
	max = -32767;
	
	for (i = 0; i < S1; i++) {
		v = *p++;
	
		if (v < min) 
			min = v;
		if (v > max)
			max = v;
	}
	write(mini_file_id, (void *)&min, sizeof(min));
	write(mini_file_id, (void *)&max, sizeof(min));
}

//-----------------------------------------------------------

void	Channel::build_mini_file()
{
	long	size;
	short	*buffer;
	long	steps;	
	long	i;
	long	k;
	
	buffer = (short *)malloc(S0 * sizeof(short));
	size = GetSize();

	steps = size / (S0 * sizeof(short));


	for (i = 0; i < steps; i++) {
		printf("%ld\n", i);
		lseek(file_id, i * S0 * sizeof(short), SEEK_SET);
		memset(buffer, 0, S0 * sizeof(short));
		read(file_id, (void *)buffer, S0 * sizeof(short));

		for (k = 0; k < NS; k++) {
			handle_mini(buffer + (k * S1));
		}
	}
	free((char *)buffer);
}

//-----------------------------------------------------------

int		Channel::Error()
{
	return(error);
}

//-----------------------------------------------------------

long	Channel::PrivateRead(long pos, long count, char *p)
{
	long	result;

	if (pos == -(BACK_BUF)) {
			memset((void *)buffer2, 0, BACK_BUF);
			lseek(file_id, 0, SEEK_SET);
			//printf("read %ld at %ld\n", count, pos);
			result = read(file_id, (void *)(p), count-BACK_BUF);
	}
	else {
		lseek(file_id, pos, SEEK_SET);
		//printf("read %ld at %ld\n", count, pos);
		result = read(file_id, (void *)(p), count);
	}
}


//-----------------------------------------------------------

long	Channel::PrivateWrite(long pos, long count, char *p)			
{
	return(write(file_id, (void *)p, count));
}

//-----------------------------------------------------------

long	Channel::GetSize()
{
	long	p;

	if (file_id < 0)
		return 0;
		
	p = lseek(file_id, 0, SEEK_END);
	return p;
}

//-----------------------------------------------------------

void	Channel::set_server(long pos)
{
	new_pos = pos;
	command_type = CMD_READ;
	release_sem(todo_sem);
}

//-----------------------------------------------------------

char	*Channel::GetBuffer()
{
	return cur_buffer;
}

//-----------------------------------------------------------
char	*Channel::SetPos(long pos)
{
	char	*tmp;
	long	tmpp;

	pos += the_offset;
	while(command_type != CMD_DONE) {
		snooze(3000);
	}


	if (buffer_2_pos == pos) {
		tmp = buffer1;
		buffer1 = buffer2;
		buffer2 = tmp;
		buffer_1_pos = buffer_2_pos;
		set_server(pos + AVAIL_BUF);
		cur_buffer = (buffer1 + BACK_BUF);
		return(cur_buffer);
	}	
	else {
		//printf("no %ld, %ld\n", pos, buffer_2_pos);
		PrivateRead(pos - BACK_BUF, BACK_BUF + AVAIL_BUF, buffer2);			
		tmp = buffer1;
		buffer1 = buffer2;
		buffer2 = tmp;
	 
		set_server(pos + AVAIL_BUF);

		cur_buffer = (buffer1 + BACK_BUF);
		return(cur_buffer);
	}
}

//-----------------------------------------------------------
// 0.5 means center.
// 1   means all left.
// 0   means all right.

void	Channel::SetLeftRight(float v)
{
	float	sum;
	
	v *= 1.15;
	v -= 0.1;
	if (v > 1.0) v = 1.0;
	if (v < 0.0) v = 0;
	
	right_weight = sin(v*3.1415926/2.0);
	left_weight = cos(v*3.1415926/2.0);
}

//-----------------------------------------------------------

float	Channel::GetReverbe_level()
{
	return rev_level;
}

//-----------------------------------------------------------

long	Channel::GetReverbe_distance()
{
	return rev_dist;
}

//-----------------------------------------------------------

void	Channel::SetFilter(long index, long value)
{
	filter_raw[index] = value;
}

//-----------------------------------------------------------

void	Channel::NormalizeFilter()
{
	float	sum;
	float	k;
	int		i;
	
	sum = 0;
	
	for (i = 0; i < 7; i++) {
		sum += filter_raw[i];
	}
	k = 32768.0 - sum;
	for (i = 0; i < 7; i++) {
		filter_clean[i] = filter_raw[i];
	}

	filter_clean[7] = -k;
}
