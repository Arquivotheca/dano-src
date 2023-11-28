/* ++++++++++
	FILE:	kb_device.h
	NAME:	herold
	DATE:	May 12, 1999
	Copyright (c) 1999 by Be Incorporated.  All Rights Reserved.

	Pithy comment goes here
+++++ */

#ifndef _KB_DEVICE_H
#define _KB_DEVICE_H

#include <SupportDefs.h>
#include <InterfaceDefs.h>
#include <NodeMonitor.h>
#include <OS.h>
#include "ktable.h"

#define SHIFT_KEY			0x00000001
#define COMMAND_KEY			0x00000002
#define CONTROL_KEY			0x00000004
#define CAPS_LOCK			0x00000008
#define SCROLL_LOCK			0x00000010
#define NUM_LOCK			0x00000020
#define OPTION_KEY			0x00000040
#define MENU_KEY			0x00000080

typedef struct {
	bigtime_t	time;
	uint32		rawKeyCode;
	uint32		modifiers;
	uchar		key_states[16];
	bool		keyDown;
} keyboard_io;


void init_control_alt_del();


class keyboard_device {
public:
						keyboard_device(int d, thread_id t, bool u = false);
	virtual				~keyboard_device();
	virtual status_t	get_next_key (keyboard_io *io) = 0;
	virtual status_t	set_key_repeat_rate (int32 keys_per_second);
	virtual status_t	set_key_repeat_delay (bigtime_t usecs_before_repeat);
	virtual status_t	set_leds (bool num, bool caps, bool scroll);
	virtual status_t	set_repeating_key(uint32 key);
	virtual status_t	set_non_repeating_key(uint32 key);
	void				init_control_keys (key_map *keys);

	int					driver;
	thread_id			thread;
	bool				usb;
	node_ref			node;
	char				*name;
	

protected:
	ulong				GetMods(void);
	bool				process_raw_key (raw_key_info *r, keyboard_io *k);
private:
	uint8				key_states[32];
	bool				locking_caps_key; 	/* true if it locks (eg Macs) */
	uint32				caps_key;			/* default value */
	uchar				caps_state;			/* caps-lock state flag */
	bool				caps_up;			/* flag when caps goes up */
	uint32				num_key;			/* default value */
	uchar				num_state;			/* num-lock state flag */
	bool				num_up;				/* flag when num goes up */
	uint32				scroll_key;			/* default value */
	uchar				scroll_state;		/* scroll-lock state flag */
	bool				scroll_up;			/* flag when scroll goes up */
	uchar				shift_state;		/* either shift-key down flag */
	uint32				shiftl_key;			/* default value */
	uchar				shift_stateL;		/* state of left shift key */
	uint32				shiftr_key;			/* default value */
	uchar				shift_stateR;		/* state of right shift key */
	uchar				cmd_state;			/* either cmd-key down flag */
	uint32				cmdl_key;			/* default value */
	uchar				cmd_stateL;			/* state of left cmd key */
	uint32				cmdr_key;			/* default value */
	uchar				cmd_stateR;			/* state of right cmd key */
	uchar				ctrl_state;			/* either cntrl-key down flag */
	uint32				ctrll_key;			/* default value */
	uchar				ctrl_stateL;		/* state of left ctrl key */
	uint32				ctrlr_key;			/* default value */
	uchar				ctrl_stateR;		/* state of right ctrl key */
	uchar				opt_state;			/* either MS option-key down */
	uint32				optl_key;			/* default value */
	uchar				opt_stateL;			/* state of left MS cmnd key */
	uint32				optr_key;			/* default value */
	uchar				opt_stateR;			/* state of right MS cmnd key */
	uint32				menu_key;			/* default value */
	uchar				menu_state;			/* MS menu-key down */
};
	
class at_keyboard_device : public keyboard_device {
public:
					at_keyboard_device (int d, thread_id t);
					~at_keyboard_device ();
	status_t		get_next_key (keyboard_io *io);
	status_t		set_repeating_key(uint32 key);
	status_t		set_non_repeating_key(uint32 key);
private:
	static const uint8	scancodes[];
	uint32			key_to_scancode (uint32 key);
	ktable			*table;		/* maps scancodes to Be keycodes */
};

class adb_keyboard_device : public keyboard_device {
public:
				adb_keyboard_device (int d, thread_id t);
	status_t	get_next_key (keyboard_io *io);
};

class usb_keyboard_device : public keyboard_device {
public:
				usb_keyboard_device (int d, thread_id t);
	status_t	get_next_key (keyboard_io *io);
};

#endif

