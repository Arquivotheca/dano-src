//******************************************************************************
//
//	File:		GameInput.cpp
//
//	Description:	BGameInput class.
//			        Client class for game input access.
//
//	Written by:	Pierre Raynaud-Richard
//
//	Revision history
//
//	Copyright 1996, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <stdlib.h>
#include <string.h>

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _SESSION_H
#include <session.h>
#endif
#ifndef _GAME_INPUT_H
#include <GameInput.h>
#endif

static uchar Type2Axes[] = {
	0, 2, 2, 4, 4, 2, 4
};

static uchar Type2Filter[] = {
	B_NO_FILTER,
	B_ANALOG_FILTER,
	B_TIME_FILTER,
	B_ANALOG_FILTER,
	B_TIME_FILTER,
	B_TIME_FILTER,
	B_TIME_FILTER	
};

/*-------------------------------------------------------------*/

long _open_game_input_channel_(long *channel_id) {
	_BAppServerLink_ link;
	long             error;

	link.session->swrite_l(GR_OPEN_INPUT_CHANNEL);
	link.session->flush();
	link.session->sread(4, channel_id);
	link.session->sread(4, &error);
	return error;
}

long _close_game_input_channel_(long channel_id) {
	_BAppServerLink_ link;
	long             error;

	link.session->swrite_l(GR_CLOSE_INPUT_CHANNEL);
	link.session->swrite_l(channel_id);
	link.session->flush();
	link.session->sread(4, &error);
	return error;
}

long _set_game_input_channel_options_(long channel_id, gi_channel_info *info) {
	_BAppServerLink_ link;
	long             error;

	link.session->swrite_l(GR_SET_INPUT_CHANNEL);
	link.session->swrite_l(channel_id);
	link.session->swrite(sizeof(gi_channel_info), info);
	link.session->flush();
	link.session->sread(4, &error);
	return error;
}

long _get_game_input_channel_state_(long channel_id, gi_channel_input *input) {
	_BAppServerLink_ link;
	long             error;

	link.session->swrite_l(GR_GET_INPUT_CHANNEL);
	link.session->swrite_l(channel_id);
	link.session->flush();
	link.session->sread(sizeof(gi_channel_input), input);
	link.session->sread(4, &error);
	return error;
}

/*-------------------------------------------------------------*/

BGameInput::BGameInput() {
	int    i;
	
	butt_buf_size = 64;
	butt_buf = (uchar*)malloc(butt_buf_size);
	SetPollingRate(0);
	ch_info.used_channels = 0;
	_open_game_input_channel_(&ch_id);
	_set_game_input_channel_options_(ch_id, &ch_info);
	for (i=0;i<4;i++) {
		joy[i].controls = 0L;
		joy[i].type = B_NO_GAME_INPUT;
	}
}

BGameInput::~BGameInput() {
	int   i;

	for (i=0;i<4;i++) {
		if (joy[i].controls != 0L)
			free(joy[i].controls);
	}
	free(butt_buf);
	_close_game_input_channel_(ch_id);
}

void BGameInput::SetPollingRate(float rate) {
	ch_info.polling_rate = rate;
	_set_game_input_channel_options_(ch_id, &ch_info);
}

void BGameInput::SetJoystickMode(long index, ulong state) {
	if ((index < 0) || (index > 3)) return;
	if (state == B_ENABLE_JOYSTICK) {
		ch_info.used_channels |= (1<<index);
		if (joy[index].axe_count == 4)
			ch_info.used_channels |= (2<<index);
	}
	else {
		ch_info.used_channels &= (15-(1<<index));
		if (joy[index].axe_count == 4)
			ch_info.used_channels &= (15-(2<<index));
	}
	_set_game_input_channel_options_(ch_id, &ch_info);
}

void BGameInput::Read(game_input_info *info, joystick_raw_data *raw_data) {
	int     i, j, k, f_type;
	uchar   *cur_butt;
	uchar   butt;
	short   dx, dy;
	short   *filter;
	double  time;
	game_input_control *ctrl;
	
	_get_game_input_channel_state_(ch_id, &ch_input);
	if (info != 0L) {
		info->timestamp = ch_input.update_time;
		for (i=0;i<4;i++) {
			if (ch_info.used_channels & (1<<i)) {
				for (k=0;k<joy[i].axe_count;k+=2) {
					if (ch_input.detect_state & (1<<i)) {
						dx = DoCalibration(ch_input.axes[i*2+k], joy[i].calibrate+k);
						dy = DoCalibration(ch_input.axes[i*2+k+1], joy[i].calibrate+k+1);
						f_type = joy[i].filter_type;
						filter = joy[i].joy_filter;
					}
					else {
						DecodeKeyPad(joy[i].keys, &dx, &dy);
						f_type = B_TIME_FILTER;
						filter = joy[i].key_filter;
					}
					if (f_type == B_TIME_FILTER) {
						time = RepeatTime(dx, dy, joy+i+(k/2));
						info->joystick[i].axes[0+k] = DoTimeFilter(f_type, dx, filter, time);
						info->joystick[i].axes[1+k] = DoTimeFilter(f_type, dy, filter, time);
					}
					else if (f_type == B_ANALOG_FILTER) {
						info->joystick[i].axes[0+k] = DoAnalogFilter(f_type, dx, filter);
						info->joystick[i].axes[1+k] = DoAnalogFilter(f_type, dy, filter);
					}
				}
				info->joystick[i].controls = cur_butt;
				ctrl = joy[i].controls;
				for (j=0;j<joy[i].control_count;j++) {
					butt = 0;
					if (ctrl->button != 0xff) {
						if (ch_input.buttons_last & (1<<ctrl->button))
							butt |= B_IS_DOWN;
						if (ch_input.buttons_down & (1<<ctrl->button))
							butt |= B_WENT_DOWN;
						if (ch_input.buttons_up & (1<<ctrl->button))
							butt |= B_WENT_UP;
					}
					if (ctrl->key != 0) {
						if (ch_input.key_last[ctrl->key>>3] & (128>>(ctrl->key&7)))
							butt |= B_IS_DOWN;
						if (ch_input.key_down[ctrl->key>>3] & (128>>(ctrl->key&7)))
							butt |= B_WENT_DOWN;
						if (ch_input.key_up[ctrl->key>>3] & (128>>(ctrl->key&7)))
							butt |= B_WENT_UP;
					}
					*cur_butt++ = butt;
					ctrl++;
				}
			}
			info->joystick[i].type = joy[i].type;
			info->joystick[i].axe_count = joy[i].axe_count;
			info->joystick[i].filter_type = joy[i].filter_type;
			if (ch_input.detect_state & (1<<i))
				info->joystick[i].status = B_JOYSTICK_DISCONNECTED;
			else
				info->joystick[i].status = B_JOYSTICK_CONNECTED;
		}
		memcpy(info->key_states, ch_input.key_last, 16*3);
	}
	if (raw_data != 0L) {
		memcpy(raw_data->axes, ch_input.axes, 8*sizeof(short));
		raw_data->button_states = ch_input.buttons_last; 
		raw_data->button_ups = ch_input.buttons_up;
		raw_data->button_downs = ch_input.buttons_down;
		raw_data->detect_state = ch_input.detect_state;
	}
}

void BGameInput::SetChannel(long               index,
							uchar              type,
							short              *calibrate,
							uchar              *keys,
							short              *key_filter,
							short              *joy_filter,
							long               ctrl_count,
							game_input_control *ctrl_list) {
	if ((index < 0) || (index > 3)) return;
	if (type <= B_JOYSTICK_MAX_TYPE) {
		joy[index].type = type;
		joy[index].axe_count = Type2Axes[type];
		joy[index].filter_type = Type2Filter[type];
	}
	memcpy(joy[index].calibrate, calibrate, sizeof(short)*6);
	memcpy(joy[index].keys, keys, sizeof(uchar)*8);
	memcpy(joy[index].key_filter, key_filter, sizeof(short)*8);
	memcpy(joy[index].joy_filter, joy_filter, sizeof(short)*8);
	if (ctrl_count > 0) {
		if (joy[index].controls != 0L)
			free(joy[index].controls);
		joy[index].controls =
			(game_input_control*)malloc(sizeof(game_input_control)*ctrl_count);
		joy[index].control_count = ctrl_count;
		memcpy(joy[index].controls, ctrl_list, sizeof(game_input_control)*ctrl_count);
	}
}

void BGameInput::GetChannel(long               index,
							uchar              *type,
							short              *calibrate,
							uchar              *keys,
							short              *key_filter,
							short              *joy_filter,
							long               *ctrl_count,
							game_input_control **ctrl_list) {
	if ((index < 0) || (index > 3)) return;
	*type = joy[index].type;
	memcpy(calibrate, joy[index].calibrate, sizeof(short)*6);
	memcpy(keys, joy[index].keys, sizeof(uchar)*8);
	memcpy(key_filter, joy[index].key_filter, sizeof(short)*8);
	memcpy(joy_filter, joy[index].joy_filter, sizeof(short)*8);
	*ctrl_count = joy[index].control_count;
	*ctrl_list = joy[index].controls;
}

short BGameInput::DoAnalogFilter(int type, short value, short *filter) {
	return value;
}

short BGameInput::DoTimeFilter(int type, short value, short *filter, double time) {
	return value;
}

short BGameInput::DoCalibration(short value, short *refs) {
	if (value > refs[2]) {
		if (value >= refs[4])
			return 32767;
		else
			return (((long)(value-refs[2]))*32768)/(long)(refs[4]-refs[2]);
	}
	else {
		if (value <= refs[0])
			return -32768;
		else
			return (((long)(value-refs[2]))*32768)/(long)(refs[2]-refs[0]);
	}
}

static char DeltaH[8] = { -1, 0 , 1, 1, 1, 0, -1 ,-1 };
static char DeltaV[8] = { -1, -1, -1, 0 , 1, 1, 1, 0 };

void BGameInput::DecodeKeyPad(uchar *keys, short *dx, short *dy) {
	int      i;
	int      dh, dv;

	dh = dv = 0;
	for (i=0;i<8;i++)
		if (ch_input.key_last[keys[i]>>3] & (0x80 >> (keys[i]&7))) {
			dh += DeltaH[i];
			dv += DeltaV[i];
		}
	if (dh == 0)     *dx = 0;
	else if (dh < 0) *dx = -32768;
	else             *dx = 32767;
	if (dv == 0)     *dy = 0;
	else if (dv < 0) *dy = -32768;
	else             *dy = 32767;
}

double BGameInput::RepeatTime(short dx, short dy, settings *set) {
	int      new_x, new_y;

	if (dx < -1024) new_x = -1;
	else if (dx > 1024) new_x = 1;
	else new_x = 0;
	if (dy < -1024) new_y = -1;
	else if (dy > 1024) new_y = 1;
	else new_y = 0;
	if ((new_x != set->time_last_x) || (new_y != set->time_last_y)) {
		set->time_last_x = new_x;
		set->time_last_y = new_y;
		set->time_filter = ch_input.update_time;
	}
	return (ch_input.update_time - set->time_filter);
}











