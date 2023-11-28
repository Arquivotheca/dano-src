#include <unistd.h>
#include <Alert.h>
#include <Box.h>
#include <Screen.h>
#include <StringView.h>
#include <Window.h>
#include "utils.h"

void
What(const char* what)
{
	BAlert* a = new BAlert("", what, "Cancel", NULL, NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT );
	a->Go();
}

void
WhatWithTimeout(const char* what, int32 seconds)
{
	BRect bounds(0,0, be_plain_font->StringWidth(what)+10, FontHeight(be_plain_font, true)+10);
	
	BWindow* window = new BWindow(bounds, "What",
		B_MODAL_WINDOW,	B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE);
	BBox* box = new BBox(bounds, "bg", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	window->AddChild(box);
	box->SetFont(be_plain_font);
	
	bounds.InsetBy(5,5);
	BStringView* string = new BStringView(bounds, "what", what, B_FOLLOW_NONE);
	box->AddChild(string);
	
	CenterWindowOnScreen(window);
	window->Show();
	
	sleep(seconds);
	
	window->Lock();
	window->Quit();
}

void
CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}

float
FontHeight(const BFont* font, bool full)
{
	font_height finfo;		
	font->GetHeight(&finfo);
	float h = finfo.ascent + finfo.descent;

	if (full)
		h += finfo.leading;
	
	return h;
}

#ifdef DEBUG
static void
dumpkeymap()
{
	key_map* keylist;
	char* charlist;
	get_key_map(&keylist, &charlist);
	
	printf("version %lx\n", keylist->version);
	printf("caps %lx, scroll %lx, num %lx\n", keylist->caps_key, keylist->scroll_key, keylist->num_key);
	printf("shift - left %lx, right %lx\n", keylist->left_shift_key, keylist->right_shift_key);
	printf("command - left %lx, right %lx\n", keylist->left_command_key, keylist->right_command_key);
	printf("control - left %lx, right %lx\n", keylist->left_control_key, keylist->right_control_key);
	printf("option - left %lx, right %lx\n", keylist->left_option_key, keylist->right_option_key);
	printf("menu %lx, lock %lx\n", keylist->menu_key, keylist->lock_settings);
	
	for (int32 i=0 ; i<128 ; i++) {
		int32 key = keylist->normal_map[i];
		int32 size = charlist[key];
		if (size > 0) {
			printf("%li %lx - %li %lx %c\n", i, i, size, key, charlist[key+size]);
		} else
			printf("%li - size of 0\n", i);
	}
}
#endif
//

#ifdef ADD_KEY_STATES
// from kb_mouse_driver.h
#define KEY_Scroll		0x0f
#define	KEY_Pause		0x10
#define KEY_Num			0x22
#define	KEY_CapsLock	0x3b
#define KEY_ShiftL		0x4b
#define KEY_ShiftR		0x56
#define KEY_ControlL	0x5c
#define KEY_CmdL		0x5d
#define KEY_AltL		0x5d
#define KEY_CmdR		0x5f
#define KEY_AltR		0x5f
#define KEY_ControlR	0x60
#define KEY_OptL		0x66
#define KEY_WinL		0x66
#define KEY_OptR		0x67
#define KEY_WinR		0x67
#define KEY_Menu		0x68
#define KEY_NumEqual	0x6a
#define KEY_Power		0x6b
#define KEY_SysRq		0x7e
#define KEY_Break		0x7f

// from kb_mouse_driver.h
typedef struct {
	bigtime_t	timestamp;
	uint32		be_keycode;
	bool		is_keydown;
} raw_key_info;

typedef struct {
	bigtime_t	time;
	uint32		rawKeyCode;
	uint32		modifiers;
	uchar		key_states[16];
	bool		keyDown;
} keyboard_io;

/* macros for dealing with key_states - do not attempt to read */
#define IS_KEY_SET(ks, k)			((ks)[(k)>>3]&(1<<((7-(k))&7)))
#define SET_KEY(ks, k)				((ks)[(k)>>3]|=(1<<((7-(k))&7)))
#define UNSET_KEY(ks, k)			((ks)[(k)>>3]&=~(1<<((7-(k))&7)))
#define TOGGLE_KEY(ks, k)			((ks)[(k)>>3]^=(1<<((7-(k))&7)))

bool
process_raw_key (raw_key_info *r, keyboard_io *k, uint32 modifiers)
{
	//	ugly
	uint8 key_states[32];
	memset (key_states, 0, sizeof (key_states));
	
	bool locking_caps_key = false; /* true if it locks (eg Macs) */
	uint32 caps_key = KEY_CapsLock;/* default value */
	uchar caps_state = modifiers & B_CAPS_LOCK;	//0;			/* caps-lock state flag */
	bool caps_up = true;			/* flag when caps goes up */
	
	uint32 num_key = KEY_Num;		/* default value */
	uchar num_state = modifiers & B_NUM_LOCK;	//0;			/* num-lock state flag */
	bool num_up = true;			/* flag when num goes up */
	
	uint32 scroll_key = KEY_Scroll;/* default value */
	uchar scroll_state = modifiers & B_SCROLL_LOCK;	//0;		/* scroll-lock state flag */
	bool scroll_up = true;		/* flag when scroll goes up */
	
	uchar shift_state = modifiers & B_SHIFT_KEY;	//0;		/* either shift-key down flag */
	uint32 shiftl_key = KEY_ShiftL;/* default value */
	uchar shift_stateL = modifiers & B_SHIFT_KEY;	//0;		/* state of left shift key */
	uint32 shiftr_key = KEY_ShiftR;/* default value */
	uchar shift_stateR = modifiers & B_SHIFT_KEY;	//0;		/* state of right shift key */
	
	uchar cmd_state = modifiers & B_COMMAND_KEY;	//			/* either cmd-key down flag */
	uint32 cmdl_key = KEY_CmdL;	/* default value */
	uchar cmd_stateL = modifiers & B_COMMAND_KEY;	///* state of left cmd key */
	uint32 cmdr_key = KEY_CmdR;	/* default value */
	uchar cmd_stateR = modifiers & B_COMMAND_KEY;	///* state of right cmd key */
	
	uchar ctrl_state = modifiers & B_CONTROL_KEY;	//0;			/* either cntrl-key down flag */
	uint32 ctrll_key = KEY_ControlL;/* default value */
	uchar ctrl_stateL = modifiers & B_CONTROL_KEY;	//0;		/* state of left ctrl key */
	uint32 ctrlr_key = KEY_ControlR;/* default value */
	uchar ctrl_stateR = modifiers & B_CONTROL_KEY;	//0;		/* state of right ctrl key */
	
	uchar opt_state = modifiers & B_OPTION_KEY;	//0;			/* either MS option-key down */
	uint32 optl_key = KEY_OptL;	/* default value */
	uchar opt_stateL = modifiers & B_OPTION_KEY;	//0;			/* state of left MS cmnd key */
	uint32 optr_key = KEY_OptR;	/* default value */
	uchar opt_stateR = modifiers & B_OPTION_KEY;	//0;			/* state of right MS cmnd key */
	
	uint32 menu_key = KEY_Menu;	/* default value */
	uchar menu_state = modifiers & B_MENU_KEY;	//0;			/* MS menu-key down */
	
	//printf("process %i %i %i\n", shift_state, cmd_state, ctrl_state);
	//
	
	bool setBit = true;
	uint32 sc = r->be_keycode;

	if (!sc)
		return false;

	if (r->is_keydown) {
		//printf("key down\n");
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
					caps_up = false;
					caps_state ^= 1;
				}
				else
					return false;
			}
		}
		else if (sc == scroll_key) {
			if (scroll_up) {
				scroll_up = false;
				scroll_state ^= 1;
			}
			else
				return false;
		}
		else if (sc == num_key) {
			if (num_up) {
				num_up = false;
				num_state ^= 1;
			}
			else
				return false;
		}

		if ((sc == num_key) || (sc == scroll_key) ||
			((sc == caps_key) && !locking_caps_key) ||
			(sc == KEY_Pause) || (sc == KEY_Break))
			TOGGLE_KEY (key_states, sc);
		else if (sc < 128)
			SET_KEY (key_states, sc);

		/* check for PC-style 3 finger salute (ctl-alt-del) */
//		if (((sc == 52) || (sc == 101)) &&
//		     ctrl_state && cmd_state)
//			do_control_alt_del(driver);
		    
		k->keyDown = true;
		k->rawKeyCode = sc;
		k->modifiers = modifiers;
		k->time = r->timestamp;
		memcpy (k->key_states, key_states, sizeof (k->key_states));

		return true;
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
				caps_up = true;
				setBit = false;
			}
		}
		else if (sc == num_key) {
			num_up = true;
			setBit = false;
		}
		else if (sc == scroll_key) {
			scroll_up = true;
			setBit = false;
		}

		if (setBit && sc < 128)
			UNSET_KEY (key_states, sc);

		k->keyDown = false;
		k->rawKeyCode = sc;
		k->modifiers = modifiers;
		k->time = r->timestamp;
		memcpy (k->key_states, key_states, sizeof (k->key_states));
		
		return true;
	}
	return false;
}
#endif