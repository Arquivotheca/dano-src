#ifndef SOFTKEYBOARD_H
#define SOFTKEYBOARD_H

#include <View.h>
#include "Key.h"
#include "SoftKeyboardStructs.h"
#include <Messenger.h>
#include <MessageRunner.h>
#include <experimental/ResourceSet.h>

class SoftKeyboard : public BView
{
public:
					// configFile is the path to the file
					// embedParams is the message from the embed tag (optional)
					// any fields in embedParams overrides the ones in configFile
					SoftKeyboard(	BRect frame,
									const char * name,
									uint32 resizingMode,
									const char * configFile,
									const BMessage * embedParams = NULL,
									const char * relativePath = NULL);
	virtual			~SoftKeyboard();
	
	virtual void	AttachedToWindow();
	virtual void	Draw(BRect updateRect);
	virtual void	MouseDown(BPoint pt);
	//virtual void	MouseMoved(BPoint pt, uint32 transit, BMessage * dragMsg); See comment in SoftKeyboard.cpp
	virtual void	MouseUp(BPoint pt);
	virtual void	MessageReceived(BMessage * msg);
	
					// puts the string to draw into displayChars
					// if this key should be drawn as a dead key, markDead is set to true
					// displayChars will get the UTF8 string, NULL terminated
	void			GetKeyDisplayChars(uint8 keyCode, char displayChars[5],
										bool & markDead, int32 *& deadTable,
										float & stringWidth, bool ignoreControl = true);
	
					// Get the ASCII code for the key, as if no modifiers were pressed (for making the event messages)
					// Return 0 if the key is not valid for some reason
	void			GetModifierIndepASCII(uint8 keyCode, char & code);
	
					// Look up the string to draw in the keymap.  Called during setup of the keys.
	void			CalculateDisplayChar(uint8 keyCode, const uint32 modifiers, char displayChars[5]);
	
					// Whenever the modifiers are changed, call this
	void			ModifiersChanged(uint32 modifiers);
	
private:
					// Reads the config from the file in filePath
	status_t		ReadConfig(const char * filePath, const BMessage * embedParams);
	
					// Releases all of the modifiers except Caps Lock
	void			ReleaseModifiers(bool noShift = false);
	void			ForceKeyRelease(uint8 keyCode);
	
					// Sends a message to the Input Server Device asking for where we
					// should send our event messages to
	void			RequestTarget();
	void			SendMessageToTarget(BMessage * msg);
	
					// Send the messages to _target for the events
	void			SendKeyDown(Key * key);
	void			SendKeyUp(Key * key);
	void			SendModifiersChanged();
	
					// Fill in all that crap for the B_KEY_DOWN, etc. messages
	void			CreateGenericKeyMessage(uint8 keyCode, BMessage * event,
												bool & isDead, int32 *& deadTable);
	
					// Set/Unset the bit for the key in the big array
	void			AdjustKeyState(int32 rawkey, bool pressed);
	
					// Start and Stop Key Repeat Notifications
	void			StartKeyRepeatNotifications();
	void			StopKeyRepeatNotifications();
	
					// Drawing Functions
					// One of these three functions is patched into _DrawKey
					// when the config is loaded
	typedef void	(* draw_func)(BView * view, SoftKeyboard * kbd, Key * key);
	draw_func		_DrawKey;
	static void		DrawNormal(BView * view, SoftKeyboard * kbd, Key * key);
	static void		DrawRounded(BView * view, SoftKeyboard * kbd, Key * key);
	static void		Draw3D(BView * view, SoftKeyboard * kbd, Key * key);
	
					// Convert a possibly relative / escaped path to a real file path
	void			UnEscapePathString(BString * out, const char * in);
	status_t		UnEscapePathStringHelper(BString * out, const char * in);
	
	bool			ParseGraphicFileName(const char * name, uint8 & keyCode, uint8 & states);
	
					// Image Resource Management
	const BBitmap	* ResourceBitmap(const char * name);
	void			InitResources(const char * resourceDir);

					// Thread func to fetch the key characters
	static int32	GetTheNormalKeyChars(void * _keyboard);	
	thread_id		_normalKeyCharsThread;
	static int32	GetTheRestOfTheKeyChars(void * _keyboard);	
	thread_id		_keyCharsThread;
	
					// Find Repeat and delay rate
	bigtime_t		GetRepeatDelay();
	int32			GetRepeatRate();
	
	// Drawing and general info
	kbd_info		_kbdInfo;
	Cell			* _cells;
	Key				* _keys[256];
	key_map			* _keyMap;
	const char		* _keyChars;
	BFont			_defaultFont;
	BFont			_defaultSpecialFont;
	Key				* _lastPushedKey;
	
	// Dead Key Strings (cached) Keys that look like this are dead
	const char		* _acuteDeadString;
	const char		* _graveDeadString;
	const char		* _circumflexDeadString;
	const char		* _dieresisDeadString;
	const char		* _tildeDeadString;
	
	// Dead Key State
	int32			* _deadTableToUse;
	uint8			_ignoreNextKeyDown;
	uint8			_ignoreNextKeyUp;
	
	// Key events and stuff
	BMessenger		_target;
	BMessageRunner	* _keyRepeatRunner;
	int32			_keyRepeatCount;
	
	// Key States
	uint32			_modifiers;				// The modifiers that are down now
	uint32			_prevModifiers;			// The modifiers that were down before the ones that are down now
	int32			_modifierIndex;
	uint8			_keyStates[16];
	
	// Where do we find resources
	const char		* _relativePath;
	BResourceSet	_resources;
	
	// Friends
	friend Cell;
	friend Key;
};
					



#endif // SOFTKEYBOARD_H
