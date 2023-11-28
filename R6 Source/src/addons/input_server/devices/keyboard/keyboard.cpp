#include "kb_mouse_driver.h"

#include "keyboard.h"

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
#include "kb_device.h"

#define Table_Size			128
#define Cntl				0
#define Option_Caps_Shift	1
#define	Option_Caps			2
#define	Option_Shift		3
#define	Option				4
#define Caps_Shift			5
#define	Caps				6
#define Shift				7
#define Normal				8


key_map	*keys = NULL;
char	*keys_buffer = NULL;

void init_from_settings(keyboard_device *kb);


KeyboardInputDevice* KeyboardInputDevice::sDevice = NULL;


static int pascal_cmp(char *str0, char *str1) 
{
	int     i;	

	if (str0[0] != str1[0])
		return -1;
	for (i=1; i<=str0[0]; i++)
		if (str0[i] != str1[i])
			return -1;
	return 0;
}


BInputServerDevice*
instantiate_input_device()
{
	return (new KeyboardInputDevice());
}


KeyboardInputDevice::KeyboardInputDevice()
	: BInputServerDevice()
{
	sDevice = this;

	init_control_alt_del();

	StartMonitoringDevice("input/keyboard/usb");

	int numKeyboards = 0;
	BList usbKeyboardList;
	int atKeyboard = open("/dev/input/keyboard/at/0", O_RDWR);
	int adbKeyboard = open("/dev/input/keyboard/adb/0", O_RDWR);

	if (atKeyboard >= 0) {
		numKeyboards++;		
	}

	if (adbKeyboard >= 0) {
		numKeyboards++;
	}
		
	struct dirent	*de = NULL;
	DIR				*dp = opendir("/dev/input/keyboard/usb");
	if (dp != NULL) {
		while (de = readdir(dp)) {
			char	port_path[PATH_MAX + 1] = "/dev/input/keyboard/usb/";
			int 	fd = -1;
			
			if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0))
				continue;
	
			strcpy(&port_path[strlen(port_path)], de->d_name);
	
			if ((fd = open(port_path, O_RDWR)) < 0)
				continue;
			
			
			keyboard_device	*kb = new usb_keyboard_device(fd, B_ERROR);
			BNode			node(port_path);
			node.GetNodeRef(&kb->node);

			usbKeyboardList.AddItem(kb);
		}

		numKeyboards += usbKeyboardList.CountItems();
		
		closedir(dp);
	}

	if (numKeyboards > 0) {
		int32				curDevice = 0;
		input_device_ref	**devices = (input_device_ref **)malloc(sizeof(input_device_ref *) * (numKeyboards + 1));
		input_device_ref	atDevice = {"AT Keyboard", B_KEYBOARD_DEVICE, NULL};
		input_device_ref	adbDevice = {"ADB Keyboard", B_KEYBOARD_DEVICE, NULL};
		
		for (curDevice = 0; curDevice < usbKeyboardList.CountItems(); curDevice++) {
			keyboard_device *kb = (keyboard_device *)usbKeyboardList.ItemAt(curDevice);
			fKeyboards.AddItem(kb);
			init_from_settings(kb);
			
			devices[curDevice] = (input_device_ref *)malloc(sizeof(input_device_ref));
			devices[curDevice]->name = (char *)malloc(32);
			sprintf(devices[curDevice]->name, "USB Keyboard %d", curDevice + 1);
			devices[curDevice]->type = B_KEYBOARD_DEVICE;
			devices[curDevice]->cookie = (void *)kb;
		}
		
		if (atKeyboard >= 0) {
			keyboard_device *kb = new at_keyboard_device(atKeyboard, B_ERROR);
			fKeyboards.AddItem(kb);
			init_from_settings(kb);
			
			atDevice.cookie = (void *)kb;
			kb->name = strdup(atDevice.name);
			
			devices[curDevice++] = &atDevice;
		}

		if (adbKeyboard >= 0) {
			keyboard_device *kb = new adb_keyboard_device(adbKeyboard, B_ERROR);
			fKeyboards.AddItem(kb);
			init_from_settings(kb);
			
			adbDevice.cookie = (void *)kb;
			kb->name = strdup(adbDevice.name);
			
			devices[curDevice++] = &adbDevice;
		}

		devices[curDevice] = NULL;
		
		RegisterDevices(devices);
		
		for (int32 i = 0; i < usbKeyboardList.CountItems(); i++)
			free(devices[i]);
		
		free(devices);
	}
}	


KeyboardInputDevice::~KeyboardInputDevice()
{
	StopMonitoringDevice("input/keyboard/usb");

	keyboard_device *kb = NULL;
	while ((kb = (keyboard_device *)fKeyboards.RemoveItem((int32)0)) != NULL) {
		close(kb->driver);
		delete (kb);
	}		
}


status_t
KeyboardInputDevice::Start(
	const char	*device,
	void		*cookie)
{
	keyboard_device *kb = (keyboard_device *)cookie;
	
	kb->thread = spawn_thread(keyboarder, device, B_REAL_TIME_DISPLAY_PRIORITY+4, kb);
	resume_thread(kb->thread);
	
	return (B_NO_ERROR);
}


status_t
KeyboardInputDevice::Stop(
	const char	*device,
	void		*cookie)
{
	status_t dummy;
	keyboard_device *kb = (keyboard_device *)cookie;
	
	kill_thread(kb->thread);
	wait_for_thread(kb->thread, &dummy);
	kb->thread = B_ERROR;
	
	return (B_NO_ERROR);
}


status_t
KeyboardInputDevice::Control(
	const char	*device,
	void		*cookie,
	uint32		code,
	BMessage	*message)
{
	switch (code) {
		case B_KEY_MAP_CHANGED:
		case B_KEY_LOCKS_CHANGED:
		case B_KEY_REPEAT_DELAY_CHANGED:
		case B_KEY_REPEAT_RATE_CHANGED:
			init_from_settings(((keyboard_device *)cookie));
			break;

		case B_NODE_MONITOR:
			HandleNodeMonitor(message);
			break;
	}

	return (B_NO_ERROR);
}


void
KeyboardInputDevice::HandleNodeMonitor(
	BMessage	*message)
{
	int32 opcode = 0;
	if (message->FindInt32("opcode", &opcode) != B_NO_ERROR)
		return;

	input_device_ref	*devices[2] = {NULL, NULL};
	input_device_ref	usbDevice = {NULL, B_KEYBOARD_DEVICE, NULL};

	if (opcode == B_ENTRY_CREATED) {
		node_ref nodeRef;
		if (message->FindInt64("directory", &nodeRef.node) != B_NO_ERROR)
			return;
		if (message->FindInt32("device", &nodeRef.device) != B_NO_ERROR)
			return;

		BPath		path;		
		BDirectory	dir(&nodeRef);
		BEntry		entry(&dir, NULL);
		if (entry.GetPath(&path) != B_NO_ERROR)
			return;

		if (message->FindInt64("node", &nodeRef.node) != B_NO_ERROR)
			return;

		const char *devName = NULL;
		if (message->FindString("name", &devName) != B_NO_ERROR)
			return;
			
		path.Append(devName);

		int fd = open(path.Path(), O_RDWR);
		if (fd < 0)
			return;

			
		keyboard_device *kb = new usb_keyboard_device(fd, B_ERROR);
		kb->node = nodeRef;
		
		fKeyboards.AddItem(kb);
		init_from_settings(kb);
		
		usbDevice.cookie = (void *)kb;
		usbDevice.name = (char *)malloc(32);
		sprintf(usbDevice.name, "USB Keyboard %d", atoi(devName) + 1);		
		devices[0] = &usbDevice;

		kb->name = usbDevice.name;

		RegisterDevices(devices);
	}
	else {
		node_ref nodeRef;
		if (message->FindInt64("node", &nodeRef.node) != B_NO_ERROR)
			return;
		if (message->FindInt32("device", &nodeRef.device) != B_NO_ERROR)
			return;
					
		int32 numKeyboards = fKeyboards.CountItems();
		for (int32 i = 0; i < numKeyboards; i++) {
			keyboard_device *kb = (keyboard_device *)fKeyboards.ItemAt(i);

			if (kb->node != nodeRef)
				continue;

			usbDevice.name = kb->name;				
			usbDevice.cookie = (void *)kb;
			devices[0] = &usbDevice;

			UnregisterDevices(devices);
		
			close(kb->driver);
		
			fKeyboards.RemoveItem((int32)i);	
			delete (kb);
			
			break;
		}
	}
}


static uint8 keypad_mask[16] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03,
	0x00, 0x07, 0x00, 0x07, 0x30, 0x00, 0x00, 0x00
};

static uint8 modifier_mask[16] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x40, 0xB0, 0xC1, 0x01, 0x00, 0x00
};

void
KeyboardInputDevice::SendEvent(
	keyboard_io *theKey,
	char		*str,
	char		*n_str, 
	int32		type,
	int32		repeat_count)
{
	char		bytes[256];
	int32		count, i;
	BMessage	*event;

	event = new BMessage(type);
	event->AddInt64("when", theKey->time);
	event->AddInt32("modifiers", theKey->modifiers);
	event->AddInt32("key", theKey->rawKeyCode);
	event->AddData("states", B_UINT8_TYPE, &theKey->key_states, 16);
	if ((type == B_KEY_DOWN) || (type == B_KEY_UP)) {
		if (n_str[0] > 0) event->AddInt32("raw_char", n_str[1]);
		count = str[0];
		if (count > 0) {
			for (i = 1; i <= count; i++) {
				event->AddData("byte", B_INT8_TYPE, str+i, sizeof(uchar));
				bytes[i - 1] = str[i];
			}
			bytes[count] = 0;
			event->AddString("bytes", (char *)bytes);
		}
	}		
	if (repeat_count > 0)
		event->AddInt32("be:key_repeat", repeat_count);
	
	sDevice->EnqueueMessage(event);
};

int32
KeyboardInputDevice::keyboarder(
	void	*arg)
{
	keyboard_device		*kb = (keyboard_device *)arg;
	int					theKeyboardDriver = kb->driver;
	
	int 		deadKey;
	char        *a_str, *n_str, *tmp_str, *lastDeadKey_a_str, *lastDeadKey_n_str;
	long		result, theTable, loop, lastRawKeyDownCode;
	ulong       *dead_table;
	ulong		a_char, n_char, mods, fake_mods, prevMods, lastLocks, newLocks;
	uchar       key_status[128];
	uint32		autoRepeatCount;
	keyboard_io	theKey, lastDeadKey;

	memset (&theKey, 0, sizeof(theKey));
	lastLocks = keys->lock_settings & (CAPS_LOCK | SCROLL_LOCK | NUM_LOCK);
	theKey.modifiers = lastLocks;
	prevMods = lastLocks;
	lastRawKeyDownCode = -1;

	// deadKey encoding :
	// 1 to 5 : index of the active deadKey table
	// 0 : no deadKey pending
	// -1 : deadKey Up pending.
	deadKey = 0;

	// status encoding :
	// Bit 0 : the next keyUp should generate a keyUp event.
	// Bit 1 : the next keyUp should generate a deadKeyUp event.
	// Bit 2 : the key is currently down (no related with event).

	memset (key_status, 0, sizeof(key_status));
	
	while (true) {
		result = kb->get_next_key (&theKey);
		// process a valid key event
		if (result != B_OK) {
			if (result < B_OK && kb->usb)
				return 0;
			continue;
		}

		a_char = theKey.rawKeyCode;
		mods = theKey.modifiers;
		//SERIAL_PRINT(("cooked: %s scan=%.8x mods=%.8x lastLocks=%.8x\n", theKey.keyDown ? "DOWN" : "UP", theKey.rawKeyCode, theKey.modifiers, lastLocks));

		// generates the B_MODIFIERS_CHANGED mesages.
		if (mods != prevMods) {
			BMessage *event = new BMessage(B_MODIFIERS_CHANGED);
			event->AddInt64("when", theKey.time);
			event->AddInt32("modifiers", mods);
			event->AddInt32("be:old_modifiers", prevMods);
			event->AddData("states", B_UINT8_TYPE, &theKey.key_states, 16);
			sDevice->EnqueueMessage(event);
			prevMods = mods;
		}

		#ifdef	DEBUG_LOG
		if ((a_char == 0x11) && (mods & CONTROL_KEY)) {
			if (!in_debug) {
				in_debug = 0x01;
				start_debugger();
			}
		}
		#endif

		/* ---
			hack for extended keyboards with CD-control keys and the like:
			These keys are mapped well beyond the 128 limit, so we just
		    special-case them here.  When Hiroshi rewrites all this, I am
		    sure he will clean this up.  This whole loop could use some
		    parcelling out into subroutines.  I just add to the mess here.
			rwh 5/16/99.
		--- */

		if (a_char > 127) { 	/* extended key from crazy keyboards? */
			if (theKey.keyDown) {
				if (lastRawKeyDownCode == theKey.rawKeyCode)
					autoRepeatCount++;
				else
					autoRepeatCount = 0;
				KeyboardInputDevice::SendEvent(&theKey, NULL, NULL, B_UNMAPPED_KEY_DOWN, autoRepeatCount);
				lastRawKeyDownCode = theKey.rawKeyCode;
			} else {
				KeyboardInputDevice::SendEvent(&theKey, NULL, NULL, B_UNMAPPED_KEY_UP, 0);
				lastRawKeyDownCode = -1;
			}
			continue;
		}

		// select the right keymap table based on the modifiers state
		fake_mods = mods;
		// NUM_LOCK invert the shift key if we are using the keyPad.
		if ((keypad_mask[a_char>>3] & (1<<(a_char&7))) && (fake_mods & NUM_LOCK))
			fake_mods ^= SHIFT_KEY;
			
		if ((fake_mods & CONTROL_KEY) && !(fake_mods & COMMAND_KEY))
			theTable = Cntl;
		else if (fake_mods & OPTION_KEY) {
			if (fake_mods & SHIFT_KEY) {
				if (fake_mods & CAPS_LOCK)
					theTable = Option_Caps_Shift;
				else
					theTable = Option_Shift;
			}
			else {
				if (fake_mods & CAPS_LOCK)
					theTable = Option_Caps;
				else
					theTable = Option;
			}
		}
		else if (fake_mods & CAPS_LOCK) {
			if (fake_mods & SHIFT_KEY)
				theTable = Caps_Shift;
			else
				theTable = Caps;
		}
		else if (fake_mods & SHIFT_KEY)
			theTable = Shift;
		else
			theTable = Normal;

		// get the keymap string corresponding to the key without modifiers
		n_char = keys->control_map[(Table_Size * Normal) + a_char];
		n_str = keys_buffer+n_char;
		// get the keymap string corresponding to the key with the modifiers
		a_char = keys->control_map[(Table_Size * theTable) + a_char];
		a_str = keys_buffer+a_char;

		// if the key_code is mapped to something, then we need to process the
		// dead keys and the auto-repeat filters.
		if (a_str[0] != 0) {
			// process keyDown
			if (theKey.keyDown) {
			// if no deadKey yet, check if it's a new one
				if (deadKey == 0) {
					for (loop = 0; loop < 5; loop++) {
						tmp_str = keys_buffer+keys->acute_dead_key[loop*32+1];
						if (((*(&keys->acute_tables + loop)) & (1 << theTable)) &&
							(pascal_cmp(tmp_str, a_str) == 0)) {
							deadKey = loop + 1;
							lastDeadKey = theKey;
							goto not_a_basic_keydown;
						}
					}
				}
			// if there is already a dead key pending, check if it's a valid
			// combo.
				else if (deadKey > 0) {
					dead_table = (ulong *)keys->acute_dead_key+(deadKey-1)*32;
					for (loop = 0; loop < 16; loop++) {
						tmp_str = keys_buffer+dead_table[2*loop];
						if (pascal_cmp(tmp_str, a_str) == 0) {
							a_str = keys_buffer+dead_table[2*loop+1];
							key_status[theKey.rawKeyCode] = 2;
							KeyboardInputDevice::SendEvent(&theKey, a_str, n_str, B_KEY_DOWN, 0);
							lastDeadKey_a_str = a_str;
							lastDeadKey_n_str = n_str;
							deadKey = -1;
							goto not_a_basic_keydown;
						}
					}
				// It's not a valid combo. We need to flush out the
				// deadKey KEY_DOWN, and perhaps the KEY_UP too...
					KeyboardInputDevice::SendEvent(&lastDeadKey, keys_buffer + dead_table[1], n_str,
							  B_KEY_DOWN, 0);
					if (!(key_status[lastDeadKey.rawKeyCode] & 4))
						KeyboardInputDevice::SendEvent(&lastDeadKey, keys_buffer + dead_table[1], n_str,
								  B_KEY_UP, 0);
					else key_status[lastDeadKey.rawKeyCode] |= 1;
					deadKey = 0;
				}
			// Then it's just a basic key_down...
				if (lastRawKeyDownCode == theKey.rawKeyCode)
					autoRepeatCount++;
				else
					autoRepeatCount = 0;
				KeyboardInputDevice::SendEvent(&theKey, a_str, n_str, B_KEY_DOWN, autoRepeatCount);
				key_status[theKey.rawKeyCode] = 1;
				
			not_a_basic_keydown:;
				lastRawKeyDownCode = theKey.rawKeyCode;
				key_status[theKey.rawKeyCode] |= 4;
			}
		// process keyUp
			else {
			// just a regular keyUp...
				if (key_status[theKey.rawKeyCode] & 1) {
					KeyboardInputDevice::SendEvent(&theKey, a_str, n_str, B_KEY_UP, 0);
					key_status[theKey.rawKeyCode] &= ~1;
				}
			// keyUp corresponding to the last deadKeyDown.
				else if (key_status[theKey.rawKeyCode] & 2) {
					KeyboardInputDevice::SendEvent(&theKey, lastDeadKey_a_str, lastDeadKey_n_str, B_KEY_UP, 0);
					key_status[theKey.rawKeyCode] &= ~2;
					deadKey = 0;
				}
				
				lastRawKeyDownCode = -1;
				key_status[theKey.rawKeyCode] &= ~4;
			}
		}
		else {
		// process unmapped keyDown
			if (theKey.keyDown) {
				if (lastRawKeyDownCode == theKey.rawKeyCode) {
					if (modifier_mask[lastRawKeyDownCode>>3] & (1<<(lastRawKeyDownCode&7)))
						goto ignored_keydown;
					autoRepeatCount++;
				}
				else
					autoRepeatCount = 0;
				KeyboardInputDevice::SendEvent(&theKey, NULL, NULL, B_UNMAPPED_KEY_DOWN, autoRepeatCount);
				lastRawKeyDownCode = theKey.rawKeyCode;
		ignored_keydown:;
			}
		// process unmapped keyUp
			else {
				KeyboardInputDevice::SendEvent(&theKey, NULL, NULL, B_UNMAPPED_KEY_UP, 0);
				lastRawKeyDownCode = -1;
			}
		}
			
		// update the state of the lock lights if necessary
		newLocks = mods & (CAPS_LOCK | SCROLL_LOCK | NUM_LOCK);
		if (newLocks != lastLocks) {
			lastLocks = newLocks;
			kb->set_leds (
				((newLocks & NUM_LOCK) != 0),
				((newLocks & CAPS_LOCK) != 0),
				((newLocks & SCROLL_LOCK) != 0)
			);
		}
	}

	return (B_NO_ERROR);
}


void
init_from_settings(
	keyboard_device *kb)
{
	key_map	*oldKeys = keys;
	char	*oldKeysBuffer = keys_buffer;
	uint32		i;
	static uchar nr_keys[] = {
		KEY_Scroll,
		KEY_Num,
		KEY_CapsLock,
		KEY_ShiftL,
		KEY_ShiftR,
		KEY_ControlL,
		KEY_CmdL,
		KEY_CmdR,
		KEY_ControlR,
		KEY_OptL,
		KEY_OptR,
		KEY_Menu
	};

	get_key_map(&keys, &keys_buffer);

	if (oldKeys) free(oldKeys);
	if (oldKeysBuffer) free(oldKeysBuffer);

	kb->init_control_keys (keys);
	ulong lastLocks = keys->lock_settings;
	kb->set_leds (
		((lastLocks & NUM_LOCK) != 0),
		((lastLocks & CAPS_LOCK) != 0),
		((lastLocks & SCROLL_LOCK) != 0)
	);

	/* make all keys repeating, then set non-repeating keys.  Ideally the
	   non-repeating key list would also be taken from the key map.  At
	   least this is not hardwired in the driver anymore!
	   Caveat Hackor. rwh 5/10/99 */

	for (i = 0; i < 255; i++)
		kb->set_repeating_key (i);

	for (i = 0; i < sizeof (nr_keys) / sizeof (nr_keys[0]); i++)
		kb->set_non_repeating_key (nr_keys[i]);

	int32 rate = 0;
	if (get_key_repeat_rate(&rate) == B_NO_ERROR)
		kb->set_key_repeat_rate (rate);

	bigtime_t delay = 0;
	if (get_key_repeat_delay(&delay) == B_NO_ERROR)
		kb->set_key_repeat_delay (delay);
}

