#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Box.h>
#include <Screen.h>

#include "BasicKeyboard.h"
#ifdef INPUT_METHOD
#include "SoftKeyboard.h"
#endif
#include "utils.h"

enum {
	msg_modifier_down = 'modd',
	msg_key_down = 'keyd'
};

const char* kEmptyStr = " ";

#ifdef ADD_ESCAPE
const char*	kEscapeKey = "Esc";
#endif
const char* kBackspaceKey = "Backspace";
const char* kTabKey = "Tab";
const char* kCapsLockKey = "Caps Lock";
const char* kEnterKey = "Enter";
const char* kShiftKey = "Shift";
const char* kControlKey = "Control";
const char* kAltKey = "Alt";
const char* kOptionKey = "Option";
const char* kSpaceKey = "Space";

// ******************************************************************************************

#if 0
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

//
//ulong
//GetMods(uchar caps_state,
//		uchar num_state,
//		uchar scroll_state,
//		uchar shift_state,
//		uchar shift_stateL,
//		uchar shift_stateR,
//		uchar cmd_state,
//		uchar cmd_stateL,
//		uchar cmd_stateR,
//		uchar ctrl_state,
//		uchar ctrl_stateL,
//		uchar ctrl_stateR,
//		uchar opt_state,
//		uchar opt_stateL,
//		uchar opt_stateR,
//		uchar menu_state
//)
//{
//	return shift_state | (cmd_state << 1) | (ctrl_state << 2) |
//		  (caps_state << 3) | (scroll_state << 4) | (num_state << 5) |
//		  (opt_state << 6) | (menu_state << 7) | (shift_stateL << 8) |
//		  (shift_stateR << 9) | (cmd_stateL << 10) | (cmd_stateR << 11) |
//		  (ctrl_stateL << 12) | (ctrl_stateR << 13) | (opt_stateL << 14) |
//		  (opt_stateR << 15);
//}

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
	
printf("process %i %i %i\n", shift_state, cmd_state, ctrl_state);
	//
	
	bool setBit = true;
	uint32 sc = r->be_keycode;

	if (!sc)
		return false;

	if (r->is_keydown) {
printf("key down\n");
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

// ******************************************************************************************

#ifdef INPUT_METHOD
TBasicKeyboardWindow::TBasicKeyboardWindow(TSoftKeyboardLooper *owner, uint32 mode)
	: BWindow(BRect(-1024, -1024, -1023, -1023), "Soft Keyboard",
#if _SUPPORTS_SOFT_KEYBOARD
		(window_look)25, B_FLOATING_ALL_WINDOW_FEEL,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE | B_NOT_ZOOMABLE |
	  		B_NOT_MINIMIZABLE | B_AVOID_FOCUS)
#else
		(window_look)25/*B_FLOATING_WINDOW_LOOK*/, B_FLOATING_ALL_WINDOW_FEEL,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE | B_NOT_ZOOMABLE |
	  		B_NOT_MINIMIZABLE | B_NOT_CLOSABLE | B_AVOID_FOCUS)
#endif
			,fOwnerLooper(owner), fMode(mode)
			, fIsActive(false)
#else
TBasicKeyboardWindow::TBasicKeyboardWindow()
	: BWindow(BRect(0,0,10,10), "Soft Keyboard", B_TITLED_WINDOW, B_NOT_RESIZABLE)
#endif
{
	AddParts();
//#ifndef _SUPPORTS_SOFT_KEYBOARD
//	CenterWindowOnScreen(this);
//#endif
}

#if _SUPPORTS_SOFT_KEYBOARD
bool
TBasicKeyboardWindow::QuitRequested()
{
	//
	//	This window is always open, but may be off-screen
	//
	MakeActive(false, false, BRect(0,0,0,0));
	
	return false;	
}
#endif

void 
TBasicKeyboardWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case 'visb':
			{
				BMessage reply('rply');
				reply.AddBool("visible", IsActive());
				message->SendReply(&reply);
			}
			break;
		case 'actv':
			{
				//
				//	sent from custom TextView, what to do and where
				bool active;
				if (message->FindBool("active", &active) == B_OK) {
					BRect frame; 
					if (active && message->FindRect("frame", &frame) == B_OK)
						MakeActive(true, true, frame);
					else
						MakeActive(false, false, BRect(0,0,0,0));
				}
			}
			break;
			
		case msg_modifier_down:	// should be sent from view
		case msg_key_down:	// should be sent from view
		case B_KEY_DOWN:
		case B_KEY_UP:
#ifdef INPUT_METHOD
			//
			//	pass the key down data to the input method
			{
				int32 value;
				if(B_OK!=message->FindInt32("be:value",&value))
					value=0;
				BMessage* thisMessage = DetachCurrentMessage();
				thisMessage->what = value?B_KEY_DOWN:B_KEY_UP;
				fOwnerLooper->EnqueueMessage(thisMessage);
			}
#endif
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

//	BRect(l:0.0, t:0.0, r:538.0, b:173.0)
void 
TBasicKeyboardWindow::AddParts()
{
	BBox* box = new BBox(Bounds(), "box", B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER);
	AddChild(box);

	fBasicKeyboard = new TBasicKeyboard(BPoint(10,10));
	box->AddChild(fBasicKeyboard);
	
	ResizeTo(fBasicKeyboard->Frame().Width() + 20, fBasicKeyboard->Frame().Height() + 20);
}

void
TBasicKeyboardWindow::MakeActive(bool active, bool customLoc, BRect relativeFrame)
{
	if (active) {
		if (customLoc) {
			BPoint actualLoc;
			//
			//	center the window below the target view
			//
			actualLoc.x = (relativeFrame.right - relativeFrame.Width()/2)
							- Frame().Width()/2;
			actualLoc.y = relativeFrame.bottom + 10;
			
			BScreen screen;
			BRect screenFrame = screen.Frame();
			float windowWidth = Frame().Width();
			float windowHeight = Frame().Height();
			
			//
			//	make it fully visible left to right
			if (actualLoc.x < 0)
				actualLoc.x = 20;
			else if (actualLoc.x > screenFrame.right
				|| actualLoc.x + windowWidth > screenFrame.right)
				actualLoc.x = screenFrame.right - Frame().Width() - 10;
			
			//
			//	make it visible top to bottom			
			if (actualLoc.y < 0)
				actualLoc.y = 20;	
			else if (actualLoc.y > screenFrame.bottom
				|| actualLoc.y + windowHeight > screenFrame.bottom)
				actualLoc.y = screenFrame.bottom - Frame().Height() - 10;
				
			//
			//	see if it now obscures the target view
			BRect windowFrame(actualLoc.x, actualLoc.y,
				actualLoc.x + windowWidth, actualLoc.y + windowHeight);
			if (windowFrame.Intersects(relativeFrame))
				actualLoc.y = relativeFrame.top - windowHeight - 10;				
				 
			MoveTo(actualLoc);
		} else
			CenterWindowOnScreen(this);
	} else
		MoveTo(-1024, -1024);
		
	fIsActive = active;
}

// ******************************************************************************************

TBasicKeyboard::TBasicKeyboard(BPoint loc)
	: BView(BRect(loc.x, loc.y, loc.x+1, loc.y+1), "basic",
		B_FOLLOW_NONE, B_WILL_DRAW)
			, fTracking(false) 
			,fModifiers(0), fCurrentKey(0)

{
	get_key_map(&fKeyList, &fCharList);
	fButtonList = new BList(128);
}


TBasicKeyboard::~TBasicKeyboard()
{
}

const int32 kBasicRowCount = 5;
const int32 kMaxKeysPerRow = 15;
const int32 kHorizontalBuffer = 10;
const int32 kVerticalBuffer = 10;
const int32 kKeyBuffer = 2;

void 
TBasicKeyboard::GetPreferredSize(float *width, float *height)
{
	int32 count = fButtonList->CountItems();
	TKey *key = (TKey*)fButtonList->ItemAt(count-1);
	if (key) {
		*width = key->Frame().right; 
		*height = key->Frame().bottom;
	} else {
		*width = ceil((StringWidth("XX") * kMaxKeysPerRow))
			+ (kMaxKeysPerRow * (2 * kHorizontalBuffer))
			+ ((kMaxKeysPerRow-1) * kKeyBuffer);
		*height = ceil((FontHeight(be_plain_font, true) * kBasicRowCount))
			+ (kBasicRowCount * (2 * kVerticalBuffer))
			+ ((kBasicRowCount - 1) * kKeyBuffer);
	}
}

void 
TBasicKeyboard::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}

void 
TBasicKeyboard::AttachedToWindow()
{	
	rgb_color color;
	if (Parent())
		color = Parent()->ViewColor();
	else
		color = ui_color(B_PANEL_BACKGROUND_COLOR);
		
	SetHighColor(color);
	SetLowColor(color);
	SetViewColor(color);

	AddParts();
#ifdef PULSE	
	int32 rate;
	get_key_repeat_rate(&rate);
	Window()->SetPulseRate(1000000/rate);
#endif

	ResizeToPreferred();
	
	fOffscreenView->ResizeTo(Bounds().Width(), Bounds().Height());
	fOffscreenView->SetViewColor(ViewColor());
	fOffscreenView->SetHighColor(ViewColor());
	fOffscreenView->SetLowColor(ViewColor());

	fOffscreenBits = new BBitmap(Bounds(), B_RGB_32_BIT, true);
	fOffscreenBits->AddChild(fOffscreenView);	
}

void
TBasicKeyboard::DetachedFromWindow()
{
	int32 count = fButtonList->CountItems()-1;
	for (int32 i=count ; i>=0 ; i--) {
		TKey* key = (TKey*)fButtonList->RemoveItem(i);
		if (key)
			delete key;
	}
	delete fButtonList;
	delete fOffscreenBits;
}

bool
TBasicKeyboard::SendEvent()
{
	BMessage* keyMessage = Window()->DetachCurrentMessage();
	if (!keyMessage)
		return false;

	int32 key;
	if (keyMessage->FindInt32("key", &key) != B_OK)
		return false;
		
	keyMessage->AddInt32("modifiers", fModifiers);

	raw_key_info rawKey;
	rawKey.timestamp = system_time();
	rawKey.be_keycode = key;
	rawKey.is_keydown = true;		
	keyboard_io theKeyInfo;
	
	if (!process_raw_key(&rawKey, &theKeyInfo, fModifiers))
		return false;

	if (fModifiers != 0) {
		uchar key_states[16];
		memcpy (key_states, theKeyInfo.key_states, sizeof (theKeyInfo.key_states));
		
		if (fModifiers & B_SHIFT_KEY)
			SET_KEY(key_states, KEY_ShiftL);
		if (fModifiers & B_COMMAND_KEY)
			SET_KEY(key_states, KEY_CmdL);
		if (fModifiers & B_CONTROL_KEY)
			SET_KEY(key_states, KEY_ControlL);
		if (fModifiers & B_OPTION_KEY)
			SET_KEY(key_states, KEY_OptL);
			
		memcpy (theKeyInfo.key_states, key_states, sizeof (theKeyInfo.key_states));
	}
	
	keyMessage->AddData("states", B_UINT8_TYPE, &theKeyInfo.key_states, 16);

	const char *str;
	if (keyMessage->FindString("bytes", &str) != B_OK)
		return false;

	int32 count = strlen(str);
	for (int32 i = 0; i < count; i++) {
		keyMessage->AddData("byte", B_INT8_TYPE, (char *)&str[i], sizeof(uchar));
	}

#ifdef INPUT_METHOD
	//	don't set it to B_KEY_DOWN here, do it in the window
//	keyMessage->what = B_KEY_DOWN;
	Window()->PostMessage(keyMessage);
#else
	keyMessage->PrintToStream();

	ssize_t size;
	const uchar* keyStates;		
	if (keyMessage->FindData("states", B_UINT8_TYPE,(const void**)&keyStates, &size) == B_OK) {
		for (int32 i=0 ; i<size ; i++) {
			printf("%x,", keyStates[i]);
		}
		printf("\n");
	}

	char* data;		
	int32 index=0;
	printf("'byte' - ");
	while(keyMessage->FindData("byte", B_INT8_TYPE, index++, (const void**)&data, &size) == B_OK) {
		data[size] = 0;
		printf("%s", data);
	}
	printf("\n");
	
	if (keyMessage->FindString("bytes", (const char**)&data) == B_OK)
		printf("'bytes' =  %s %li\n", data, strlen(data));		
#endif

	return true;
}

void 
TBasicKeyboard::MessageReceived(BMessage *message)
{
	switch (message->what) 
	{
		case msg_modifier_down:
			int32 current, value;
			if (	message->FindInt32("raw_char", &current) == B_OK
				&& 	message->FindInt32("be:value", &value) == B_OK) 
			{
				if (value == 0)
					fModifiers &= (~current);
				else
					fModifiers |= current;

				SendEvent();				
				Draw(Bounds());
			}
			break;
			
		case msg_key_down:
			{
				if (SendEvent()) 
				{
					//
					//	only the caps lock key is sticky
					if (fModifiers != 0) 
					{
						int32 mods=fModifiers;
						mods &= B_CAPS_LOCK; // clear everything but capslock
						bool resetit=(mods!=fModifiers);
						fModifiers=mods;
						if(resetit) // only update if the modifiers changed
							ResetModifiers(false);
					}
				}
			}
			break;

		default:
			BView::MessageReceived(message);
	}
}

TKey*
TBasicKeyboard::KeyAtPoint(BPoint pt)
{
	TKey* key=NULL;
	bool found=false;
	int32 count = fButtonList->CountItems();
	for (int32 i=0 ; i<count ; i++) {
		key = (TKey*)fButtonList->ItemAt(i);
		if (key->Frame().Contains(pt)) {
			found = true;
			break;
		}
	}
	if (found)
		return key;
	else
		return NULL;
}

void
TBasicKeyboard::SetKey(TKey* key, bool state)
{
	if (!key)
		return;
		
	if (key->IsModifierKey()) {
		key->SetValue(!key->Value());
	} else {
		key->SetValue(state ? 1 : 0);
	}

#if 0
//	Draw(key->Frame());
#else
	BRect keyframe=key->Frame();
	if (!fOffscreenBits->Lock())
		return;
	fOffscreenView->PushState();	
	fOffscreenView->FillRect(keyframe);
	key->Draw();
	fOffscreenView->PopState();
	fOffscreenView->Sync();
	DrawBitmap(fOffscreenBits,keyframe,keyframe);
	fOffscreenBits->Unlock();
#endif
}

void
TBasicKeyboard::MouseDown(BPoint pt)
{
	TKey* key = KeyAtPoint(pt);
	if (!key || fCurrentKey == key)
		return;
	//	current key set to NULL on mouseup	
	fCurrentKey = key;

	SetKey(fCurrentKey, true);
	SetTracking(true);

//#ifdef PULSE		
//	fFirstClickTime = system_time();
//#endif
	fCurrentKey->Invoke();		
}

void
TBasicKeyboard::MouseUp(BPoint pt)
{
	fCurrentKey = KeyAtPoint(pt);
	if (!fCurrentKey)
		return;
	
	//
	//	mod keys are sticky, don't follow mouseup		
	if (!fCurrentKey->IsModifierKey())
		SetKey(fCurrentKey, false);
	
	//
	//	always turn tracking off	
	SetTracking(false);
	fCurrentKey->Invoke();		// send up-event
	fCurrentKey = NULL;
}

void
TBasicKeyboard::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	uint32 buttons;
	BPoint loc;
	GetMouse(&loc, &buttons);
#if 0
// enable this block to be able to drag the mouse over keys to activate them
	if(KeyAtPoint(where)==fCurrentKey)
		return;
	if (fCurrentKey && !fCurrentKey->IsModifierKey())
		SetKey(fCurrentKey, false);
	SetTracking(false);
	MouseDown(where);
	return;
#endif
	if (buttons == 0) {
		if (fCurrentKey && !fCurrentKey->IsModifierKey())
			SetKey(fCurrentKey, false);
		SetTracking(false);
		return;
	}
				
	switch (code) {
		case B_ENTERED_VIEW:
		case B_INSIDE_VIEW:
			{
				TKey* key = KeyAtPoint(where);
				//
				//	no current key or match, do nothing
				if (!key)
					break;
				
				//
				//	if a key is down and its not a modifier key
				// 	and the current key is not the last key
				//	then reset the last key
				//
				if (fCurrentKey && !fCurrentKey->IsModifierKey()
					&& key != fCurrentKey) {
					SetKey(fCurrentKey, false);
					SetTracking(false);
					fCurrentKey = NULL;
					break;
				}
//				//
//				//	if the last key was not a mod key, then simulate a mouseup		
//				if (fCurrentKey && !fCurrentKey->IsModifierKey()) {
//					SetKey(fCurrentKey, false);
//					SetTracking(false);
//				}
//				
//				//
//				//	set to new key
//				fCurrentKey = key;
//				if (fCurrentKey && !fCurrentKey->IsModifierKey()) {
//					SetKey(fCurrentKey, true);	
//					SetTracking(true);
//					fCurrentKey->Invoke();
//				}
			}
			break;

		case B_EXITED_VIEW:
			if (fCurrentKey && !fCurrentKey->IsModifierKey()) {
				SetKey(fCurrentKey, false);
				SetTracking(false);
				fCurrentKey = NULL;
			}
			break;
		
		default:
			break;
	}
	
	BView::MouseMoved(where, code, message);
}

#ifdef PULSE	
void
TBasicKeyboard::Pulse()
{
//	ulong buttons;
//	BPoint loc;
//	GetMouse(&loc, &buttons);
//	
//	if (Bounds().Contains(loc) && IsTracking() && buttons!=0
//		&& system_time() - fFirstClickTime > fKeyDelay) {
//		Invoke();
//	} else {
//		if (Value() && buttons==0 && !fIsModifierKey) {
//			SetValue(0);
//			SetTracking(false);
//		}
//	}
	BControl::Pulse();
}
#endif

void
TBasicKeyboard::Draw(BRect updateRect)
{	
	if (!fOffscreenBits->Lock())
		return;
		
	fOffscreenView->PushState();	
	fOffscreenView->FillRect(Bounds());

	for (int32 i=0 ;; i++) 
	{
		TKey* key = (TKey*)fButtonList->ItemAt(i);
		if (!key)
			break;
		key->Draw();
	}
	
	fOffscreenView->PopState();
	fOffscreenView->Sync();
	
	DrawBitmapAsync(fOffscreenBits);
	fOffscreenBits->Unlock();
}

const int32 kRowOneKeyMap[] = {
#ifdef ADD_ESCAPE
	0x01,
#endif
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x0
};
const int32 kRowTwoKeyMap[] = {
	0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x0,0x0
};
const int32 kRowThreeKeyMap[] = {
	0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x0,0x0,0x0
};
const int32 kRowFourKeyMap[] = {
	0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x0,0x0,0x0,0x0
};
const int32 kRowFiveKeyMap[] = {
	0x5c,0x0,0x5d,0x0,0x5e,0x0,0x0,0x0,0x0,0x0,0x5f,0x0,0x60,0x0,0x0,0x0
};

const int32*
TBasicKeyboard::GetKeyMapping(int32 row)
{
	const int32* keymap=NULL;
	switch(row) {
		case 1:
			keymap = kRowOneKeyMap;
			break;
		case 2:
			keymap = kRowTwoKeyMap;
			break;
		case 3:
			keymap = kRowThreeKeyMap;
			break;
		case 4:
			keymap = kRowFourKeyMap;
			break;
		case 5:
			keymap = kRowFiveKeyMap;
			break;

		default:
			keymap = 0;
			break;
	}
	
	return keymap;
}

char*
TBasicKeyboard::GetKeyInfo(int32 key, int32 modifiers)
{
	int32* map=NULL;
	if (modifiers == 0)
		map = fKeyList->normal_map;
	else if (modifiers & B_OPTION_KEY && modifiers & B_SHIFT_KEY && modifiers & B_CAPS_LOCK)
		map = fKeyList->option_caps_shift_map;
	else if (modifiers & B_CAPS_LOCK && modifiers & B_SHIFT_KEY)
		map = fKeyList->caps_shift_map;
	else if (modifiers & B_OPTION_KEY && modifiers & B_SHIFT_KEY)
		map = fKeyList->option_shift_map;
	else if (modifiers & B_OPTION_KEY && modifiers & B_CAPS_LOCK)
		map = fKeyList->option_caps_map;
	else if (modifiers & B_SHIFT_KEY)
		map = fKeyList->shift_map;
	else if (modifiers & B_CAPS_LOCK)
		map = fKeyList->caps_map;
	else if (modifiers & B_OPTION_KEY)
		map = fKeyList->option_map;
	else if (modifiers & B_CONTROL_KEY)
		map = fKeyList->control_map;
	else
		return NULL;

	//	
	//	for any map
	//	get the offset into the chars array at the index of the key	
	int32 offset = map[key];
	//
	//	pascal strings, first byte is the size
	int32 size = fCharList[offset];
	char* string=NULL;
	if (size > 0) {
		//
		//	key will own this string
		string = (char*)malloc(size+1);
		//
		//	step past the length byte
		strncpy(string, &fCharList[offset+1], size);
		string[size] = 0;
	} else
		string = strdup(kEmptyStr);
	
	return string;
}

void
TBasicKeyboard::GetKeyStrings(int32 key, char** keychars)
{
	int32 modifiers;
	for (int32 modindex=0 ; modindex<9 ; modindex++) {
		switch(modindex) {
			case 0:	modifiers = 0; break;
			case 1:	modifiers = B_SHIFT_KEY; break;
			case 2:	modifiers = B_CAPS_LOCK; break;
			case 3:	modifiers = B_CAPS_LOCK + B_SHIFT_KEY; break;
			case 4:	modifiers = B_OPTION_KEY; break;
			case 5:	modifiers = B_OPTION_KEY + B_SHIFT_KEY; break;
			case 6:	modifiers = B_OPTION_KEY + B_CAPS_LOCK; break;
			case 7:	modifiers = B_OPTION_KEY + B_SHIFT_KEY + B_CAPS_LOCK; break;
			case 8:	modifiers = B_CONTROL_KEY; break;
			default: modifiers=0; break;
		}

		keychars[modindex] = GetKeyInfo(key, modifiers);
		if (!keychars[modindex])
			keychars[modindex] = strdup(kEmptyStr);
	}
}

bool
TBasicKeyboard::GetKeyInfo(int32 row, int32 column,
	float *size, int32 *rawChar,
	const char** label, bool *useLabel, bool *isModifier)
{
	bool done=false;
	
	switch (row) {
		case 1:
#ifdef ADD_ESCAPE
			if (column == 0) {
				*size = 1;
				*rawChar = B_ESCAPE;
				*label = kEscapeKey;
				*useLabel = true;
			} else
#endif
			if (column == 13) {
				*size = 2;
				*rawChar = B_BACKSPACE;
				*label = kBackspaceKey;
				*useLabel = true;
			}
			break;
		case 2:
			if (column == 0) {
				*size = 2;
				*rawChar = B_TAB;
				*label  = kTabKey;
				*useLabel = true;
			}
			break;
		case 3:
			if (column == 0) {
				*size = 2;
				*rawChar = B_CAPS_LOCK;
				*label = kCapsLockKey;
				*useLabel = true;
				*isModifier = true;
			} else if (column == 12) {
				*size = 2;
				*rawChar = B_ENTER;
				*label = kEnterKey;
				*useLabel = true;
				done = true;
			}
			break;
		case 4:
			if (column == 0 || column == 11) {
				*size = 2.5;
				if (column == 0)
					*rawChar = B_LEFT_SHIFT_KEY | B_SHIFT_KEY;
				else if (column == 11) {
					*rawChar = B_RIGHT_SHIFT_KEY | B_SHIFT_KEY;
					done = true;
				}
				*label = kShiftKey;
				*useLabel = true;
				*isModifier = true;
			}
			break;
		case 5:
			*size = 2;
			*useLabel = true;
			switch(column) {
				case 0:
					*rawChar = B_CONTROL_KEY | B_LEFT_CONTROL_KEY;
					*label = kControlKey;
					*isModifier = true;
					break;
				case 1:
					*rawChar = B_COMMAND_KEY | B_LEFT_COMMAND_KEY;
					*label = kAltKey;
					*isModifier = true;
					break;
				case 2:
					*size = 7;
					*rawChar = B_SPACE;
					*label = kSpaceKey;
					break;
				case 3:
					*rawChar = B_COMMAND_KEY | B_RIGHT_COMMAND_KEY;
					*label = kAltKey;
					*isModifier = true;
					break;
				case 4:
#ifdef SHOW_OPTION_KEY  
					*rawChar = B_OPTION_KEY | B_RIGHT_OPTION_KEY;
					*label = kOptionKey;
#else
					*rawChar = B_CONTROL_KEY | B_RIGHT_CONTROL_KEY;
					*label = kControlKey;
#endif
					*isModifier = true;
					done = true;
					break;
			}
			break;
	}
	
	return done;
}

const int32 kNormalMap = 0;

void 
TBasicKeyboard::AddParts()
{	
	fOffscreenView = new BView(Bounds(), "", B_FOLLOW_NONE, 0);

	float width = ceil(StringWidth("XX")) + (2 * kHorizontalBuffer);
	float height = ceil(FontHeight(be_plain_font, true)) + (2 * kVerticalBuffer);

	for (int32 row=1 ; row<=5 ; row++) {
		const int32* keymap = GetKeyMapping(row);
		
		float top = (row-1) * height + (row-1) * kKeyBuffer;
		BRect frame(0, top, 1, top + height);
		
		for (int32 column=0 ; column<kMaxKeysPerRow ; column++) {
			//
			//	get the corresponding key value for the current row
			int32 key = keymap[column];

			//
			//	get array of nine strings for this key
			char* keychars[9];
			GetKeyStrings(key, keychars);
				
			//
			//	raw char is first byte of normal map, should be single byte			
			float size=1;
			int32 rawChar = *keychars[kNormalMap];
			const char* label=NULL;
			bool useLabel=false;
			bool isModifier=false;
			
			//
			//	get the layout info
			bool done = GetKeyInfo(row, column, &size, &rawChar,
				&label, &useLabel, &isModifier);
			if (!useLabel)
				label = keychars[kNormalMap];
			
			frame.right = frame.left + (width * size);
			if (size > 1)
				frame.right += kKeyBuffer * (size - 1);

			TKey* button = new TKey(frame, label,
				isModifier
					? new BMessage(msg_modifier_down)
					: new BMessage(msg_key_down),
				keychars, rawChar, key, isModifier, useLabel,
				fOffscreenView, this);

			button->SetTarget(this, Window());
			
			fButtonList->AddItem(button);	
				
			frame.left = frame.right + kKeyBuffer;
			
			if (done)
				break;
		}		
	}	
}

void
TBasicKeyboard::ResetModifiers(bool state)
{
	int32 count = fButtonList->CountItems();
	for (int32 i=0 ; i<count ; i++) {		
		TKey *key = (TKey*)fButtonList->ItemAt(i);
		if (key && key->IsModifierKey() && key->ModifierKey() != B_CAPS_LOCK) 
		{
			int32 oldvalue=key->Value();
			int32 newvalue=(state ? 1 : 0);
			key->SetValue(newvalue);
//			if(newvalue!=oldvalue)
				key->Invoke(); // need key-up events
		}
	}
}

// ******************************************************************************************

TKey::TKey(BRect frame, const char* label, BMessage *message,
	char** strings, int32 rawChar, int32 key, bool isModifier,
	bool useLabel, BView* parent, TBasicKeyboard* realParent)
	: BInvoker(message, NULL),
		fRawChar(rawChar),
		fKey(key),
		fButtonState(0),
		fIsModifierKey(isModifier),
		fUseLabel(useLabel),
		fRealParent(realParent),
		fParent(parent),
		fFrame(frame),
		fValue(0)
{
	fLabel = strdup(label);
	//
	//	!! truncate label if wider than bounds of button
	float fh = FontHeight(be_plain_font, true);
	
	fTextLoc.x = fFrame.right - Bounds().Width()/2 - fParent->StringWidth(label)/2;
	fTextLoc.y = fFrame.bottom - Bounds().Height()/2 + fh/2;
	
	//
	//	!! should be dynamic based on actual label
	fTextFrame.Set(frame.left+2, fTextLoc.y-fh-2, frame.right-2, fTextLoc.y+2);

	get_key_repeat_delay(&fKeyDelay);

	for (int32 kindex=0 ; kindex<9 ; kindex++)
		fStrings[kindex] = strdup(strings[kindex]);

	fHiliteColor = tint_color(fParent->ViewColor(), B_DARKEN_2_TINT);
}

TKey::~TKey()
{
	free(fLabel);
	for (int32 kindex=0 ; kindex<9 ; kindex++)
		free(fStrings[kindex]);
}

const rgb_color kBlack = {0,0,0,255};
const rgb_color kDarkBorder = {120,120,120,255};
const rgb_color kLightBorder = {230,230,230,255};
const rgb_color kInterior = {200,200,200,255};
const rgb_color kInteriorHilite = {128,128,128,255};
const float kButtonRadius = 4.0;

void 
TKey::Draw()
{
	if (!fParent)
		return;
		
	rgb_color interiorColor;
	//
	//	fill the interior
	if (Value()) {
		interiorColor = kInteriorHilite;
	} else {
		interiorColor = kInterior;
	}
	fParent->SetLowColor(interiorColor);
	fParent->SetHighColor(interiorColor);

	
	BRect frame(Bounds());
	frame.InsetBy(2,2);
	frame.right--;
	frame.bottom--;
	fParent->FillRoundRect(frame, kButtonRadius, kButtonRadius);

	//
	//	draw the border
	frame.InsetBy(-2,-2);
	
	frame.OffsetBy(1,1);
	fParent->SetHighColor(kLightBorder);
	fParent->StrokeRoundRect(frame, kButtonRadius, kButtonRadius);
	frame.OffsetBy(-1,-1);
	fParent->SetHighColor(kDarkBorder);
	fParent->StrokeRoundRect(frame, kButtonRadius, kButtonRadius);

	//
	//	draw the text
	const char* label = GetLabel();
	if (label && strlen(label) > 0) {
		fParent->SetHighColor(kBlack);
		fParent->MovePenTo(fTextLoc);		
		fParent->DrawString(label);
	}
}

status_t
TKey::Invoke(BMessage* message)
{
	if (!message)
		message = Message();
	if (!message)
		return B_BAD_VALUE;

	BMessage clone(*message);

	clone.AddInt64("when", system_time());
	clone.AddInt32("be:value",Value());
	//
	clone.AddInt32("key", fKey);
	clone.AddInt32("raw_char", fRawChar);

	const char* label = GetLabel();
	if (!label)
		return B_ERROR;
		
	if (	strcmp(kTabKey, label) == 0
		|| 	strcmp(kSpaceKey, label) == 0
		|| 	strcmp(kBackspaceKey, label) == 0
		|| 	strcmp(kEnterKey, label) == 0) 
	{
		char str[2];
		str[0] = fRawChar;
		str[1] = 0;
		clone.AddString("bytes", str);
	}
	else if(!IsModifierKey())
		clone.AddString("bytes", label);
	else
		clone.AddString("bytes", "");

	status_t err = BInvoker::Invoke(&clone);
	
	return err;
}

const char*
TKey::GetLabel()
{
	if (fUseLabel)
		return Label();
		
	const char* label=kEmptyStr;
	int32 modifiers = fRealParent->Modifiers();

	if (modifiers == 0 || modifiers & B_COMMAND_KEY)
		label = fStrings[0];
	else if (modifiers & B_OPTION_KEY && modifiers & B_SHIFT_KEY && modifiers & B_CAPS_LOCK)
		label = fStrings[7];
	else if (modifiers & B_OPTION_KEY && modifiers & B_SHIFT_KEY)
		label = fStrings[5];
	else if (modifiers & B_OPTION_KEY && modifiers & B_CAPS_LOCK)
		label = fStrings[6];
	else if (modifiers & B_CAPS_LOCK && modifiers & B_SHIFT_KEY)
		label = fStrings[3];
	else if (modifiers & B_SHIFT_KEY)
		label = fStrings[1];
	else if (modifiers & B_CAPS_LOCK)
		label = fStrings[2];
	else if (modifiers & B_OPTION_KEY)
		label = fStrings[4];
	else if (modifiers & B_CONTROL_KEY)
		label = fStrings[8];
	else
		//printf("** no key label for modifiers %lx **\n", modifiers);
		label = kEmptyStr;
		
	return label;
}
