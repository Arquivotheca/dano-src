#ifndef	_GAME_INPUT_CHANNEL_H
#define	_GAME_INPUT_CHANNEL_H

#include <SupportDefs.h>

enum {
	B_JOYSTICK_1A = 0,
	B_JOYSTICK_1B = 1,
	B_JOYSTICK_2A = 2,
	B_JOYSTICK_2B = 3,
	B_JOYSTICK_1A_ON = 0x0001,
	B_JOYSTICK_1B_ON = 0x0002,
	B_JOYSTICK_2A_ON = 0x0004,
	B_JOYSTICK_2B_ON = 0x0008
};
	
typedef struct {
	float     polling_rate;
	ulong     used_channels;
} gi_channel_info;

typedef struct {
	bigtime_t update_time;
	uchar     key_last[16];
	uchar     key_up[16];
	uchar     key_down[16];
	uchar     buttons_last;
	uchar     buttons_up;
	uchar     buttons_down;
	uchar     detect_state;
	short     axes[8];
} gi_channel_input;

#endif


