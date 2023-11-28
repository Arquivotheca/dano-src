/******************************************************************************
 **
 **	File:		GameInput.h
 **
 **	Description:	Client class for game input access.
 **
 **	Copyright 1996-97, Be Incorporated, All Rights Reserved.
 **
 ******************************************************************************/


#ifndef	_GAME_INPUT_H
#define	_GAME_INPUT_H

#include <game_input_channel.h>

enum {
    B_ENABLE_JOYSTICK = 1,
	B_DISABLE_JOYSTICK = 0,
	B_IS_JOYSTICK_TOP_PRIMARY_ON = 0x0001,
	B_IS_JOYSTICK_TOP_SECONDARY_ON = 0x0002,
	B_IS_JOYSTICK_BOTTOM_PRIMARY_ON = 0x0004,
	B_IS_JOYSTICK_BOTTOM_SECONDARY_ON = 0x0008,
	B_JOYSTICK_TOP_PRIMARY = 0,
	B_JOYSTICK_TOP_SECONDARY = 1,
	B_JOYSTICK_BOTTOM_PRIMARY = 2,
	B_JOYSTICK_BOTTOM_SECONDARY = 3,
	
	B_JOYSTICK_CONNECTED = 1,
	B_JOYSTICK_DISCONNECTED = 0
};

enum {
    B_JOYSTICK_MAX_TYPE = 6,
	
    B_NO_GAME_INPUT = 0,
	B_JOYSTICK_2X2_ANALOG = 1,
	B_JOYSTICK_2X2_CONTACT = 2,
	B_JOYSTICK_4X4_ANALOG = 3,
	B_JOYSTICK_4X4_CONTACT = 4,
	B_KEYBOARD_2_AXES = 5,
	B_KEYBOARD_4_AXES = 6,
};

enum {
    B_NO_FILTER = 0,
	B_ANALOG_FILTER = 1,
	B_TIME_FILTER = 2
};

enum {
    B_IS_DOWN = 0x01,
	B_WENT_DOWN = 0x02,
	B_WENT_UP = 0x04
};

enum {
    B_CONTROL_NAME_SIZE = 33
};

typedef struct {
    short    axes[4];
	uchar    *controls;
    uchar    type;
	uchar    axe_count;
    uchar    status;
	uchar    filter_type;
} joystick_info;

typedef struct {
    bigtime_t      timestamp;
    joystick_info  joystick[4];
	uchar          key_states[16];
	uchar          key_ups[16];
	uchar          key_downs[16];
} game_input_info;

typedef struct {
    short     axes[8];
	uchar     button_states;
	uchar     button_ups;
	uchar     button_downs;
	uchar     detect_state;
} joystick_raw_data;

/* private struct*/
typedef struct {
	uchar        button;
	uchar        key;
	char         name[B_CONTROL_NAME_SIZE+1];
} game_input_control;

class BGameInput {
	
 public:
	BGameInput();
	~BGameInput();
	inline  float	 PollingRate();
	void             SetPollingRate(float rate);
	inline  ulong    JoysticksMode();
	void             SetJoystickMode(long index, ulong state);
	void             Read(game_input_info *info, joystick_raw_data *raw_data = 0L);
	void     SetChannel(long index, uchar type, short *calibrate, uchar *keys,
						short *key_filter, short *joy_filter, long ctrl_count,
						game_input_control *ctrl_list);
	void     GetChannel(long index, uchar *type, short *calibrate, uchar *keys,
						short *key_filter, short *joy_filter, long *ctrl_count,
						game_input_control **ctrl_list);
	
 private:
	typedef struct {            
		short        calibrate[6];
		uchar        keys[8];
		short        key_filter[8];
		short        joy_filter[8];
		game_input_control *controls;
		bigtime_t    time_filter;
		char         time_last_x;
		char         time_last_y;
		uchar        control_count;	
		uchar        joy_prev;
		uchar        type;
		uchar        axe_count;
		uchar        filter_type;
		uchar        _reserved_;
	} settings;
	
	long             ch_id;
	gi_channel_info  ch_info;
	gi_channel_input ch_input;
	long             butt_buf_size;
	uchar            *butt_buf;
	settings         joy[4];
	
	short     DoCalibration(short value, short *refs);
	void      DecodeKeyPad(uchar *keys, short *dx, short *dy);
	bigtime_t RepeatTime(short dx, short dy, settings *set);
	short     DoAnalogFilter(int type, short value, short *filter);
	short     DoTimeFilter(int type, short value, short *filter, bigtime_t time);
};

float BGameInput::PollingRate() {
    return ch_info.polling_rate;    
}

ulong BGameInput::JoysticksMode() {
    return ch_info.used_channels;    
}

#endif











