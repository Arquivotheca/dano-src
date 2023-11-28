/* ++++++++++
	FILE:	kb_device.cpp
	NAME:	herold
	DATE:	May 12, 1999
	Copyright (c) 1999 by Be Incorporated.  All Rights Reserved.

	Base class implementation for the keyboard_device classs, plus
	implementations for various subclasses.
+++++ */

#include "kb_mouse_driver.h"
#include "priv_syscalls.h"

#include "kb_device.h"

#if SUPPORTS_TASK_MANAGER
#include "TeamMonitorWindow.h"
#endif

#include <unistd.h>
#include <dirent.h>

#define DEBUG 1
#include <Debug.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <stdlib.h>
#include <string.h>


#if SUPPORTS_TASK_MANAGER
static TeamMonitorWindow *sWindow = NULL;
#endif

/* ----------
	Constructor
----- */

keyboard_device::keyboard_device(int d, thread_id t, bool u)
{
	driver = d;
	thread = t;
	usb = u;
	name = NULL;

	memset (key_states, 0, sizeof (key_states));
	locking_caps_key = FALSE; /* true if it locks (eg Macs) */
	caps_key = KEY_CapsLock;/* default value */
	caps_state = 0;			/* caps-lock state flag */
	caps_up = TRUE;			/* flag when caps goes up */
	num_key = KEY_Num;		/* default value */
	num_state = 0;			/* num-lock state flag */
	num_up = TRUE;			/* flag when num goes up */
	scroll_key = KEY_Scroll;/* default value */
	scroll_state = 0;		/* scroll-lock state flag */
	scroll_up = TRUE;		/* flag when scroll goes up */
	shift_state = 0;		/* either shift-key down flag */
	shiftl_key = KEY_ShiftL;/* default value */
	shift_stateL = 0;		/* state of left shift key */
	shiftr_key = KEY_ShiftR;/* default value */
	shift_stateR = 0;		/* state of right shift key */
	cmd_state = 0;			/* either cmd-key down flag */
	cmdl_key = KEY_CmdL;	/* default value */
	cmd_stateL = 0;			/* state of left cmd key */
	cmdr_key = KEY_CmdR;	/* default value */
	cmd_stateR = 0;			/* state of right cmd key */
	ctrl_state = 0;			/* either cntrl-key down flag */
	ctrll_key = KEY_ControlL;/* default value */
	ctrl_stateL = 0;		/* state of left ctrl key */
	ctrlr_key = KEY_ControlR;/* default value */
	ctrl_stateR = 0;		/* state of right ctrl key */
	opt_state = 0;			/* either MS option-key down */
	optl_key = KEY_OptL;	/* default value */
	opt_stateL = 0;			/* state of left MS cmnd key */
	optr_key = KEY_OptR;	/* default value */
	opt_stateR = 0;			/* state of right MS cmnd key */
	menu_key = KEY_Menu;	/* default value */
	menu_state = 0;			/* MS menu-key down */
}
	
/* ----------
	Destructor
----- */

keyboard_device::~keyboard_device()
{
	if (name)
		free(name);
}


/* ----------
	init_control_keys - given a key map, set up what should be viewed as
	control keys.
----- */

void
keyboard_device::init_control_keys (key_map *keys)
{
	system_info	sysinfo;

	if (get_system_info(&sysinfo) == B_OK)
		locking_caps_key = (sysinfo.platform_type == B_MAC_PLATFORM);
 
	caps_key = keys->caps_key;
	scroll_key = keys->scroll_key;
	num_key = keys->num_key;
	shiftl_key = keys->left_shift_key;
	shiftr_key = keys->right_shift_key;
	cmdl_key = keys->left_command_key;
	cmdr_key = keys->right_command_key;
	ctrll_key = keys->left_control_key;
	ctrlr_key = keys->right_control_key;
	optl_key = keys->left_option_key;
	optr_key = keys->right_option_key;
	menu_key = keys->menu_key;

}

/* ----------
	set_non_repeating_key - make a key non-repeating
----- */

status_t
keyboard_device::set_non_repeating_key (uint32 key)
{
	return ioctl(driver, KB_SET_KEY_NONREPEATING, &key);
}

/* ----------
	set_repeating_key - make a key repeating
----- */

status_t
keyboard_device::set_repeating_key (uint32 key)
{
	return ioctl(driver, KB_SET_KEY_REPEATING, &key);
}


/* ----------
	set_key_repeat_rate - set the key repeat rate (duh)
----- */

status_t
keyboard_device::set_key_repeat_rate (int32 keys_per_second)
{
	return ioctl(driver, KB_SET_KEY_REPEAT_RATE, &keys_per_second);
}


/* ----------
	set_key_repeat_delay - set the key repeat delay, in microseconds
----- */

status_t
keyboard_device::set_key_repeat_delay (bigtime_t usecs_before_repeat)
{
	return ioctl(driver, KB_SET_KEY_REPEAT_DELAY, &usecs_before_repeat);
}


/* ----------
	set_leds - set the numlock, capslock and scrolllock LEDS
----- */

status_t
keyboard_device::set_leds (bool num, bool caps, bool scroll)
{
	led_info	led;
	
	led.num_lock = num;
	led.caps_lock = caps;
	led.scroll_lock = scroll;

	num_state = (uchar)num;
	caps_state = (uchar)caps;
	scroll_state = (uchar)scroll;

	return ioctl (driver, KB_SET_LEDS, &led);
}


/* --------------------------------------------------------------------
/	calculate the modifier bit mask.
/ */

ulong
keyboard_device::GetMods(void)
{
	return shift_state | (cmd_state << 1) | (ctrl_state << 2) |
		  (caps_state << 3) | (scroll_state << 4) | (num_state << 5) |
		  (opt_state << 6) | (menu_state << 7) | (shift_stateL << 8) |
		  (shift_stateR << 9) | (cmd_stateL << 10) | (cmd_stateR << 11) |
		  (ctrl_stateL << 12) | (ctrl_stateR << 13) | (opt_stateL << 14) |
		  (opt_stateR << 15);
}


/* macros for dealing with key_states - do not attempt to read */
#define IS_KEY_SET(ks, k)			((ks)[(k)>>3]&(1<<((7-(k))&7)))
#define SET_KEY(ks, k)				((ks)[(k)>>3]|=(1<<((7-(k))&7)))
#define UNSET_KEY(ks, k)			((ks)[(k)>>3]&=~(1<<((7-(k))&7)))
#define TOGGLE_KEY(ks, k)			((ks)[(k)>>3]^=(1<<((7-(k))&7)))


void
init_control_alt_del()
{
#if SUPPORTS_TASK_MANAGER
	uint8		*threadMonitor = NULL;
	system_info	si;
	get_system_info(&si);

	create_area("Team Monitor", 
				(void **)&threadMonitor, 
				B_ANY_ADDRESS, 
				((si.max_threads * (sizeof(thread_id) + sizeof(uint8)) + B_PAGE_SIZE - 1) & -B_PAGE_SIZE) + B_PAGE_SIZE,
				B_FULL_LOCK,
				B_WRITE_AREA | B_READ_AREA);

	sWindow = new TeamMonitorWindow(threadMonitor, si.max_threads);
#endif
}


static void
do_control_alt_del(int driver)
{
#if SUPPORTS_TASK_MANAGER
	if (!sWindow->IsEnabled()) {
		sWindow->Enable(driver);
	}
#endif
}


/* ----------
	process_raw_key - process a raw_key_info into a keyboard_io.  This base
	class deals with raw_key_info where the scancode has already been
	converted to Be KeyCode by the driver (since Bob ran out of time and
	could not convert the USB and ADB drivers to pass up raw hardware
	specific scancodes).

	We keep a bit mask representing the state of the entire standard
	keyboard (keys w/Be keycodes < 128).  We also track the state of
	the control keys.
----- */

bool
keyboard_device::process_raw_key (raw_key_info *r, keyboard_io *k)
{
	bool		setBit = TRUE;
	uint32		sc;

	sc = r->be_keycode;

	if (!sc)
		return FALSE;

	if (r->is_keydown) {	
		if (sc == ctrll_key) {
			ctrl_stateL = 1;
			ctrl_state = 1;
		}
		else if (sc == ctrlr_key) {
			ctrl_stateR = 1;
			ctrl_state = 1;
		}
		else if (sc == cmdl_key) {
			cmd_stateL = 1;
			cmd_state = 1;
		}
		else if (sc == cmdr_key) {
			cmd_stateR = 1;
			cmd_state = 1;
		}
		else if (sc == shiftl_key) {
			shift_stateL = 1;
			shift_state = 1;
		}
		else if (sc == shiftr_key) {
			shift_stateR = 1;
			shift_state = 1;
		}
		else if (sc == optl_key) {
			opt_stateL = 1;
			opt_state = 1;
		}
		else if (sc == optr_key) {
			opt_stateR = 1;
			opt_state = 1;
		}
		else if (sc == menu_key)
			menu_state = 1;
		else if (sc == caps_key) {
			if (locking_caps_key) {
				caps_state = 1;
			} else {
				if (caps_up) {
					caps_up = FALSE;
					caps_state ^= 1;
				}
				else
					return FALSE;
			}
		}
		else if (sc == scroll_key) {
			if (scroll_up) {
				scroll_up = FALSE;
				scroll_state ^= 1;
			}
			else
				return FALSE;
		}
		else if (sc == num_key) {
			if (num_up) {
				num_up = FALSE;
				num_state ^= 1;
			}
			else
				return FALSE;
		}

		if ((sc == num_key) || (sc == scroll_key) ||
			((sc == caps_key) && !locking_caps_key) ||
			(sc == KEY_Pause) || (sc == KEY_Break))
			TOGGLE_KEY (key_states, sc);
		else if (sc < 128)
			SET_KEY (key_states, sc);

		/* check for PC-style 3 finger salute (ctl-alt-del) */
		if (((sc == 52) || (sc == 101)) &&
		     ctrl_state && cmd_state)
			do_control_alt_del(driver);
		    
		k->keyDown = TRUE;
		k->rawKeyCode = sc;
		k->modifiers = GetMods();
		k->time = r->timestamp;
		memcpy (k->key_states, key_states, sizeof (k->key_states));

		return TRUE;
	} else {
		if (sc == ctrll_key) {
			ctrl_stateL = 0;
			ctrl_state = ctrl_stateL | ctrl_stateR;
		}
		else if (sc == ctrlr_key) {
			ctrl_stateR = 0;
			ctrl_state = ctrl_stateL | ctrl_stateR;
		}
		else if (sc == cmdl_key) {
			cmd_stateL = 0;
			cmd_state = cmd_stateL | cmd_stateR;
		}
		else if (sc == cmdr_key) {
			cmd_stateR = 0;
			cmd_state = cmd_stateL | cmd_stateR;
		}
		else if (sc == shiftl_key) {
			shift_stateL = 0;
			shift_state = shift_stateL | shift_stateR;
		}
		else if (sc == shiftr_key) {
			shift_stateR = 0;
			shift_state = shift_stateL | shift_stateR;
		}
		else if (sc == optl_key) {
			opt_stateL = 0;
			opt_state = opt_stateL | opt_stateR;
		}
		else if (sc == optr_key) {
			opt_stateR = 0;
			opt_state = opt_stateL | opt_stateR;
		}
		else if (sc == menu_key)
			menu_state = 0;
		else if (sc == caps_key) {
			if (locking_caps_key)
				caps_state = 0;
			else {
				caps_up = TRUE;
				setBit = FALSE;
			}
		}
		else if (sc == num_key) {
			num_up = TRUE;
			setBit = FALSE;
		}
		else if (sc == scroll_key) {
			scroll_up = TRUE;
			setBit = FALSE;
		}

		if (setBit && sc < 128)
			UNSET_KEY (key_states, sc);

		k->keyDown = FALSE;
		k->rawKeyCode = sc;
		k->modifiers = GetMods();
		k->time = r->timestamp;
		memcpy (k->key_states, key_states, sizeof (k->key_states));
		

		return TRUE;
	}
	return FALSE;
}


/* ----------
	ADB keyboard device contructor - uses base class
----- */

adb_keyboard_device::adb_keyboard_device (int d, thread_id t)
	: keyboard_device (d, t)
{
}

/* ----------
	ADB keyboard device contructor - uses base class
----- */

usb_keyboard_device::usb_keyboard_device (int d, thread_id t)
	: keyboard_device (d, t, TRUE)
{
}


/* ----------
	build-in table for converting AT keyboard scancodes into Be KeyCodes
----- */

const uint8 at_keyboard_device::scancodes [] = {
    /*   0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
/* 0 */ 0x00, 0x01, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x26,
/* 1 */ 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x47, 0x5c, 0x3c, 0x3d,
/* 2 */ 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x11, 0x4b, 0x33, 0x4c, 0x4d, 0x4e, 0x4f,
/* 3 */ 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x24, 0x5d, 0x5e, 0x3b, 0x02, 0x03, 0x04, 0x05, 0x06,
/* 4 */ 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x22, 0x0f, 0x37, 0x38, 0x39, 0x25, 0x48, 0x49, 0x4a, 0x3a, 0x58,
/* 5 */ 0x59, 0x5a, 0x64, 0x65, 0x7e, 0x00, 0x69, 0x0c, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 6 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 7 */ 0x6e, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x6c, 0x00, 0x6a, 0x00, 0x00,
/* extended codes start here - we added 0x80 to the scancode */
/* 8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 9 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5b, 0x60, 0x00, 0x00,
/* a */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* b */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x0e, 0x5f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* c */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x20, 0x57, 0x21, 0x00, 0x61, 0x00, 0x63, 0x00, 0x35,
/* d */ 0x62, 0x36, 0x1f, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x67, 0x68, 0x00, 0x00
};


/* ----------
	AT keyboard device contructor - looks for a scancode mapping file, and
	initializes a mapping table with it if found.  Otherwise, sets up to
	use the default mapping table herein.
----- */

at_keyboard_device::at_keyboard_device (int d, thread_id t)
	: keyboard_device (d, t)
{
	table = new ktable ();
	if (!table)
		return;

	/* populate scancode mapping table from keyboard info file */
	BPath	path;
	if (find_directory (B_COMMON_SETTINGS_DIRECTORY, &path, FALSE) == B_OK) {
		path.Append ("AT_Keyboard_Info");
		BEntry entry (path.Path(), TRUE);
		if (entry.Exists()) {
			SERIAL_PRINT (("setting up scancode mapping table from %s\n", path.Path()));
			table->InitFromFile (path.Path());	
			return;
		}
	}

	SERIAL_PRINT (("!!no scancode file - use built in table\n"));
	/* no keyboard info file - use built-in fast table lookup */
	delete table;
	table = NULL;
}


/* ----------
	AT keyboard device destructor - destroy the scancode mapping table if
	there was one.
----- */

at_keyboard_device::~at_keyboard_device ()
{
	if (table)
		delete table;
}


/* ----------
	AT keyboard device set_non_repeating_key
----- */

status_t
at_keyboard_device::set_non_repeating_key (uint32 key)
{
	uint32	scancode;

	scancode = key_to_scancode (key);
	return ioctl(driver, KB_SET_KEY_NONREPEATING, &scancode);
}


/* ----------
	AT keyboard device set_repeating_key
----- */

status_t
at_keyboard_device::set_repeating_key (uint32 key)
{
	uint32	scancode;

	scancode = key_to_scancode (key);
	return ioctl(driver, KB_SET_KEY_REPEATING, &scancode);
}


/* ----------
	AT keyboard device key_to_scancode - converts a Be KeyCode into
	a hardware specific scancode.
----- */

uint32
at_keyboard_device::key_to_scancode (uint32 k)
{
	if (table)
		return table->ScanCode (k);
	for (size_t i = 0; i < sizeof (scancodes) / sizeof (scancodes[0]); i++)
		if (scancodes[i] == k)
			return i;

	return 0;
}


/* ----------
	AT keyboard device get_next_key - gets the key and converts it
	to a Be KeyCode.
----- */

status_t
at_keyboard_device::get_next_key (keyboard_io *kb)
{
	at_kbd_io		io;
	status_t		err;
	raw_key_info	raw;


	err = ioctl(driver, KB_READ, &io);
	if (err < B_OK)
		return err;

	raw.is_keydown = io.is_keydown;
	raw.timestamp = io.timestamp;
	if (table) 
		raw.be_keycode = table->KeyCode(io.scancode);
	else if (io.scancode < sizeof (scancodes)/sizeof(scancodes[0]))
		raw.be_keycode = scancodes[io.scancode];
	else
		raw.be_keycode = 0;

	if(raw.be_keycode == 0)
		raw.be_keycode = 0x80000000|io.scancode;

	//SERIAL_PRINT(("raw: %s scan=%.2x bescan=%.2x\n", raw.is_keydown ? "DOWN" : "UP", (int32) io.scancode, (int32) raw.be_keycode));
	return process_raw_key (&raw, kb) ? B_OK : B_OK + 1;
}


/* ----------
	USB keyboard device get_next_key - gets the Be KeyCode directly
	from the driver.
----- */

status_t
usb_keyboard_device::get_next_key (keyboard_io *kb)
{
	raw_key_info	rawKey;
	status_t		result;

	result = ioctl(driver, KB_READ, &rawKey);

	if (result < B_OK)
		return result;

	//SERIAL_PRINT(("raw: %s scan=%.2x prevMods=%.8x\n", rawKey.is_keydown ? "DOWN" : "UP", rawKey.be_keycode, prevMods));
	return process_raw_key (&rawKey, kb) ? B_OK : B_OK + 1;
}

/* ----------
	ADB keyboard device get_next_key - gets the Be KeyCode directly
	from the driver.
----- */

status_t
adb_keyboard_device::get_next_key (keyboard_io *kb)
{
	raw_key_info	rawKey;
	status_t		result;

	result = ioctl(driver, KB_READ, &rawKey);

	if (result < B_OK)
		return result;

	//SERIAL_PRINT(("raw: %s scan=%.2x prevMods=%.8x\n", rawKey.is_keydown ? "DOWN" : "UP", rawKey.be_keycode, prevMods));
	return process_raw_key (&rawKey, kb) ? B_OK : B_OK + 1;
}


