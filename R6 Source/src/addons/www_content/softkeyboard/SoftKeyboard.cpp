
#define DEBUG 0

#include "SoftKeyboard.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include <String.h>
#include <Message.h>
#include <Directory.h>
#include <Bitmap.h>
#include <Input.h>
#include <MessageQueue.h>
#include <Binder.h>
#include <Entry.h>

#include <TranslatorFormats.h>
#include <Autolock.h>

#include <Debug.h>
#include <util.h>

// Yuck, need to include this for get_setting().
#include <ResourceCache.h>

kbd_cell_info cellInfos[4];
soft_key_info keyInfos[2];

#define SFT_KBD_START_KBD_REPEAT	'sfk0'
#define SFT_KBD_SEND_KBD_REPEAT		'sfk1'

#define KEY_MAP_TABLE_SIZE			128
#define SFTKBD_DRAW_COLORED_BOXES	0

#define rc2index(r, c)  (((r)*(_kbdInfo.xCellCount))+(c))

// Create a new C string from a P string
static const char *
p2cstr(const char * pString)
{
	char * cString = (char *) malloc(pString[0] + 1);
	memcpy(cString, pString+1, pString[0]);
	cString[pString[0]] = '\0';
	return cString;
}

// Control Key never affects deadness in our world
// because we never display different things for control key
static uint32
Modifiers2TableMask(uint32 modifiers)
{
	if (modifiers & B_OPTION_KEY && modifiers & B_CAPS_LOCK && modifiers & B_SHIFT_KEY)
		return B_OPTION_CAPS_SHIFT_TABLE;
	else if (modifiers & B_OPTION_KEY && modifiers & B_CAPS_LOCK)
		return B_OPTION_CAPS_TABLE;
	else if (modifiers & B_OPTION_KEY && modifiers & B_SHIFT_KEY)
		return B_OPTION_SHIFT_TABLE;
	else if (modifiers & B_OPTION_KEY)
		return B_OPTION_TABLE;
	else if (modifiers & B_CAPS_LOCK && modifiers & B_SHIFT_KEY)
		return B_CAPS_SHIFT_TABLE;
	else if (modifiers & B_CAPS_LOCK)
		return B_CAPS_TABLE;
	else if (modifiers & B_SHIFT_KEY)
		return B_SHIFT_TABLE;
	else
		return B_NORMAL_TABLE;
}


SoftKeyboard::SoftKeyboard(BRect frame, const char * name, uint32 resizingMode,
							const char * configFile, const BMessage * embedParams,
							const char * relativePath)
	: BView(frame, name, resizingMode, B_WILL_DRAW)
{
	// Must be initialized first.
	_keyCharsThread = 0;
	_normalKeyCharsThread = 0;
	
	// Get the key map
	get_key_map(&_keyMap, (char **) &_keyChars);

	// Choose the dead keys
	_acuteDeadString = p2cstr(_keyChars + _keyMap->acute_dead_key[1]);
	_graveDeadString = p2cstr(_keyChars + _keyMap->grave_dead_key[1]);
	_circumflexDeadString = p2cstr(_keyChars + _keyMap->circumflex_dead_key[1]);
	_dieresisDeadString = p2cstr(_keyChars + _keyMap->dieresis_dead_key[1]);
	_tildeDeadString = p2cstr(_keyChars + _keyMap->tilde_dead_key[1]);
	
	// Set the current modifiers state
	ModifiersChanged(0);				// Do not change this from 0, or at least
	_prevModifiers = 0;					// don't make it anything that could cause
										// it to display anything other than the normal
										// key characters -- i.e. it's okay to set the numlock keys
										// etc but not the shift or option keys here.
		
	// Where do file names come from?
	_relativePath = relativePath;
	
	BString configFilePathUnesc;
	
	UnEscapePathString(&configFilePathUnesc, configFile);
	
	// Read from the Configuration Files
	ReadConfig(configFilePathUnesc.String(), embedParams);
	
	// Initialize Some Things
	_lastPushedKey = NULL;
	for (int i=0; i<16; ++i)
		_keyStates[i] = 0; 				// Everything's up
	_keyRepeatRunner = NULL;
	_ignoreNextKeyUp = false;
	_ignoreNextKeyDown = false;
	_keyRepeatCount = 0;
	_deadTableToUse = NULL;
	
	// Resize the view to the proper size.
	ResizeTo(_kbdInfo.xCellCount * _kbdInfo.cellWidth + _kbdInfo.borderWidth, _kbdInfo.yCellCount * _kbdInfo.cellHeight + _kbdInfo.borderWidth);
	
	// Make the border color take effect
	SetViewColor(_kbdInfo.borderColor);
	
}


SoftKeyboard::~SoftKeyboard()
{
	status_t retVal;
	if (_normalKeyCharsThread != 0)
	{
		wait_for_thread(_normalKeyCharsThread, &retVal);
	}
	if (_keyCharsThread != 0)
	{
		wait_for_thread(_keyCharsThread, &retVal);
	}
	
	
	int32 i;
	
	delete[] _cells;
	for (i=0; i<256; ++i)
		if (_keys[i])
			delete _keys[i];
	free((void *) _keyMap);
	free((void *) _keyChars);
	free((void *) _acuteDeadString);
	free((void *) _graveDeadString);
	free((void *) _circumflexDeadString);
	free((void *) _dieresisDeadString);
	free((void *) _tildeDeadString);
	delete _keyRepeatRunner;
}


void
SoftKeyboard::AttachedToWindow()
{
	SetDrawingMode(B_OP_ALPHA);
	SetViewColor(_kbdInfo.borderColor);
	
	RequestTarget();
}


void
SoftKeyboard::Draw(BRect updateRect)
{
	if (_normalKeyCharsThread != 0)
	{
								// Wait for the key_chars_fetcher thread to finish loading them
								// before we try to draw.
		status_t retVal;
		wait_for_thread(_normalKeyCharsThread, &retVal);
		_normalKeyCharsThread = 0;
	}

	int i;
	for (i=0; i<256; ++i)
	{
		if (_keys[i])
			_DrawKey(this, this, _keys[i]);
	}
	
	if (_keyCharsThread != 0)
		resume_thread(_keyCharsThread);
}


void
SoftKeyboard::MouseDown(BPoint pt)
{
	// Calculate the cell
	float topLeftOffset = floor(_kbdInfo.borderWidth / 2.0); // this is floor because it's the bottom right of the non existing cells
	
	int16 row = (uint8) ((pt.y - topLeftOffset) / _kbdInfo.cellHeight);
	if (row < 0 || row >= _kbdInfo.yCellCount)
		return;
	
	int16 col = (uint8) ((pt.x - topLeftOffset) / _kbdInfo.cellWidth);
	if (col < 0 || col >= _kbdInfo.xCellCount)
		return;
	
	int16 cellNum = rc2index(row, col);
	
	
	// Find the key
	Key * key = _keys[_cells[cellNum].key];
	if (!key)
		return ;
		
	// If we are in dead mode, and this is a modifier that is not a shift or caps lock key,
	// ignore it
	if (_deadTableToUse)
		if (key->_modifierKey && !(key->_modifierKey & B_SHIFT_KEY || key->_modifierKey & B_CAPS_LOCK))
			return;
	
	// Don't do this if the key is a modifier that is in the sticky down mode
	if (key->_state != Key::STICKY_DOWN)
	{
		// Tell the key in that cell to be down
		AdjustKeyState(key->_keyCode, true);
		key->_state = Key::DOWN;
		
		// Redraw that key
		_DrawKey(this, this, key);
		Flush();
		
		// Send the Message!
		SendKeyDown(key);
		
		// If it's not a modifier and we're not ignoring key downs
		if (key->_modifierKey == 0 && !_ignoreNextKeyDown)
			StartKeyRepeatNotifications();
	}
	
	// If it was a modifier that just got set down, redraw the rest of the keyboard
	// and send the Modifier Changed Event
	// If it was already down (indicated by state == STICKY_DOWN), do nothing
	if (key->_modifierKey != 0 && key->_state == Key::DOWN)
	{
		ModifiersChanged(_modifiers | key->_modifierKey);
		Draw(Bounds());
		SendModifiersChanged();
	}
	
	// Set the event mask so we get the up notification
	SetMouseEventMask(B_POINTER_EVENTS, 0);
	
	// Remember which key we just pushed so we can release it
	_lastPushedKey = key;
	
}


//void
//SoftKeyboard::MouseMoved(BPoint pt, uint32 transit, BMessage * dragMsg)
//{
	// Do not do normal button tracking of the key.
	// Applications will be expecting that if they get a key down
	// message, that they will get an up notification as well.
	// Therefore, keep the key down until they lift up the pen.
//}


void
SoftKeyboard::MouseUp(BPoint pt)
{
	StopKeyRepeatNotifications();
	
	bool alreadySentEvent = false;
	
	// If there was one of our keys down
	if (_lastPushedKey)
	{
		// If the key is a modifier
		if (_lastPushedKey->_modifierKey != 0)
		{
			if (_lastPushedKey->_state == Key::STICKY_DOWN)
			{
				// If it is STICKY_DOWN, release it
				AdjustKeyState(_lastPushedKey->_keyCode, false);
				_lastPushedKey->_state = Key::UP;
				ModifiersChanged(_modifiers & ~_lastPushedKey->_modifierKey);
				
				// Redraw Everything
				Draw(Bounds());
				
				// Send the Message! (modifiers don't repeat)
				SendModifiersChanged();
				SendKeyUp(_lastPushedKey);
			}
			else
			{
				// If it is DOWN, leave it down and set it to STICKY_DOWN
				_lastPushedKey->_state = Key::STICKY_DOWN;
			}
			_lastPushedKey = NULL;
			return;
		}
		
		// Tell the key to be up
		_lastPushedKey->_state = Key::UP;
		_DrawKey(this, this, _lastPushedKey);
		AdjustKeyState(_lastPushedKey->_keyCode, false);
		
		// If the key was not a modifier, and there are modifiers other than Caps Lock down,
		// undo all of the modifiers except Caps Lock
		if (_modifiers != 0 && _modifiers != B_CAPS_LOCK)
		{
			if (!(_ignoreNextKeyUp == _lastPushedKey->_keyCode || _lastPushedKey->_modifierKey))
			{
				SendKeyUp(_lastPushedKey);
				ReleaseModifiers();
				alreadySentEvent = true;
				Draw(Bounds());
			}
		}
		
		// Send the Message!
		if (!alreadySentEvent)
			SendKeyUp(_lastPushedKey);
		
		// Finally, forget the last key
		_lastPushedKey = NULL;
	}
}


void
SoftKeyboard::MessageReceived(BMessage * msg)
{
	status_t	err;
	BMessenger	msngr;
	
	switch (msg->what)
	{
		// Tells us where to send event messages
		case 'msng':
		{
			err = msg->FindMessenger("messenger", &msngr);
			if (err != B_OK)
				goto DEFAULT;
			
			_target = msngr;
		}
		break;
		
		// The key repeat delay just elapsed, start sending repeat downs
		case SFT_KBD_START_KBD_REPEAT:
		{
			int32 repeatRate = GetRepeatRate();
			bigtime_t repeatTime = 1000000 / repeatRate; 
			delete _keyRepeatRunner;
			_keyRepeatRunner = new BMessageRunner(	BMessenger(this),
													new BMessage(SFT_KBD_SEND_KBD_REPEAT),
													repeatTime,
													-1);
		}
		// break;  No break because we want it to fall through
		// Send a repeat down
		case SFT_KBD_SEND_KBD_REPEAT:
		{
			// Fix for bug #20000720-20786
			// Search for other repeats or a mouse up in the message queue.
			BMessageQueue * queue = Looper()->MessageQueue();
			BMessage * queuedMessage;
			
			// If we find other repeats, drop them. 
			while ((queuedMessage = queue->FindMessage(SFT_KBD_SEND_KBD_REPEAT, 0)) != NULL)
			{
				queue->RemoveMessage(queuedMessage);
			}
			
			// If we find a mouse up, don't emit any events.
			if ((queuedMessage = queue->FindMessage(B_MOUSE_UP, 0)) == NULL)
			{
				if (_lastPushedKey)
				{
					++_keyRepeatCount;
					SendKeyDown(_lastPushedKey);
				}
			}
			
		}				
		break;
		
		default:
DEFAULT:
			BView::MessageReceived(msg);
	}
}


void
SoftKeyboard::GetKeyDisplayChars(uint8 keyCode, char displayChars[5], bool & markDead,
									int32 *& deadTable, float & stringWidth, bool ignoreControl)
{
	Key * key = _keys[keyCode];
	if (!key)
	{
		strncpy("", displayChars, 10);
		markDead = false;
		stringWidth = 0;
		return;
	}
	
	uint8 i, j;
	
	// Codes > 127 not supported yet
	if (keyCode > 127)
	{
		strncpy("", displayChars, 10);
		markDead = false;
		return;
	}
	
	int32 modifierIndex = _modifierIndex;
	if (ignoreControl && modifierIndex == CONTROL_CHAR_TABLE)
		modifierIndex = NORMAL_CHAR_TABLE;
	
	// Copy the string
	strncpy(displayChars, key->_normalChar + (modifierIndex * sizeof(key->_normalChar)), 5);
	stringWidth = *(&key->_normalCharWidth + modifierIndex);
	
	// If it's an empty string, return now
	if (displayChars[0] == '\0')
	{
		markDead = false;
		deadTable = NULL;
		return ;
	}
	
	// If the stringwidth isn't there (which is the first time you see it -- just the normal state)
	if (stringWidth == 0 && _modifierIndex == 0)
	{
		stringWidth = _defaultFont.StringWidth(key->_normalChar);
		key->_normalCharWidth = (uint8) stringWidth;
	}
	
	// Is there already a dead key pending?
	if (_deadTableToUse)
	{
		// Release everything but the shift and caps lock modifiers
		ReleaseModifiers(true);
		
		// Here Soft Keyboard is different.
		// It only displays the keys that are possible to use
		// All this crap wants to get turned into P-String compares, but I was having some problem with that. -joeo
		const char		* pDeadString;
		unsigned char	deadStringLen;
		char			cDeadString[5];
		const char		* pDeadReplace;
		char			deadReplaceLen;
		char			cDeadReplace[5];
		
		// Find This char in the even elements of the _deadTableToUes
		for (j=0; j<32; j+=2)
		{
			pDeadString = _keyChars + _deadTableToUse[j];
			if (!pDeadString)
			{
				strcpy(displayChars, "");
				markDead = false;
				deadTable = NULL;
				return ;
			}
			deadStringLen = *pDeadString++;
			
			// Convert the Pascal String into a C String
			for (i=0; i<deadStringLen && i < 4; ++i)
				cDeadString[i] = pDeadString[i];
			cDeadString[i] = '\0';
			
			if (strcmp(cDeadString, displayChars) == 0)
			{
				pDeadReplace = _keyChars + _deadTableToUse[j+1];
				deadReplaceLen = *pDeadReplace++;
				
				// Convert the Pascal String into a C String
				for (i=0; i<deadReplaceLen && i < 4; ++i)
					cDeadReplace[i] = pDeadReplace[i];
				cDeadReplace[i] = '\0';
				
				strcpy(displayChars, cDeadReplace);
				
				goto MATCHED;
			}
		}
		if (j != 35)
		{
			displayChars[0] = ' ';
			displayChars[0] = '\0';
		}
		
MATCHED:
		markDead = false;
		deadTable = NULL;
	}
	else
	{
		// Check for deadness
		uint32 modifierTable = Modifiers2TableMask(_modifiers);
		if (modifierTable & _keyMap->acute_tables && strcmp(displayChars, _acuteDeadString) == 0)
		{
			markDead = true;
			deadTable = _keyMap->acute_dead_key;
		}
		else if (modifierTable & _keyMap->grave_tables && strcmp(displayChars, _graveDeadString) == 0)
		{
			markDead = true;
			deadTable = _keyMap->grave_dead_key;
		}
		else if (modifierTable & _keyMap->circumflex_tables && strcmp(displayChars, _circumflexDeadString) == 0)
		{
			markDead = true;
			deadTable = _keyMap->circumflex_dead_key;
		}
		else if (modifierTable & _keyMap->dieresis_tables && strcmp(displayChars, _dieresisDeadString) == 0)
		{
			markDead = true;
			deadTable = _keyMap->dieresis_dead_key;
		}
		else if (modifierTable & _keyMap->tilde_tables && strcmp(displayChars, _tildeDeadString) == 0)
		{
			markDead = true;
			deadTable = _keyMap->tilde_dead_key;
		}
		else
		{
			markDead = false;
			deadTable = NULL;
		}
	}
}


void
SoftKeyboard::GetModifierIndepASCII(uint8 keyCode, char & code)
{
	// Codes > 127 not supported yet
	if (keyCode > 127)
	{
		code = 0;
		return ;
	}
	
	const char	* pString	= _keyChars + _keyMap->normal_map[keyCode];
	
	if (pString[0] <= 0)
	{
		code = 0;
		return ;
	}
	
	code = pString[1];
}


void
SoftKeyboard::CalculateDisplayChar(uint8 keyCode, const uint32 modifiers, char displayChars[5])
{
	int8	i;
	int32	* map;
	
	if (modifiers & B_CONTROL_KEY)
		map = _keyMap->control_map;
	else if (modifiers & B_OPTION_KEY && modifiers & B_CAPS_LOCK && modifiers & B_SHIFT_KEY)
		map = _keyMap->option_caps_shift_map;
	else if (modifiers & B_OPTION_KEY && modifiers & B_CAPS_LOCK)
		map = _keyMap->option_caps_map;
	else if (modifiers & B_OPTION_KEY && modifiers & B_SHIFT_KEY)
		map = _keyMap->option_shift_map;
	else if (modifiers & B_OPTION_KEY)
		map = _keyMap->option_map;
	else if (modifiers & B_CAPS_LOCK && modifiers & B_SHIFT_KEY)
		map = _keyMap->caps_shift_map;
	else if (modifiers & B_CAPS_LOCK)
		map = _keyMap->caps_map;
	else if (modifiers & B_SHIFT_KEY)
		map = _keyMap->shift_map;
	else
		map = _keyMap->normal_map;
	
	// Get the Pascal String for the Character
	const char	* 	pString	= _keyChars + map[keyCode];
	unsigned char	pStringLen	= *pString++;
	
	// Convert the Pascal String into a C String
	for (i=0; i<pStringLen && i < 4; ++i)
		displayChars[i] = pString[i];
	displayChars[i] = '\0';
}


void
SoftKeyboard::ModifiersChanged(uint32 modifiers)
{
	if (_keyCharsThread != 0)
	{
								// Wait for the key_chars_fetcher thread to finish loading them
								// Only wait the first time, in the very off chance that they
								// manage to click a modifier key before this is done loading
								// Also, this will not get run the first time this function is
								// called in the ctor, before ReadConfig, because the thread
								// will not have been spawned yet.
		status_t retVal;
		wait_for_thread(_keyCharsThread, &retVal);
		_keyCharsThread = 0;
	}
	
	// Set the modifiers
	_prevModifiers = _modifiers;
	_modifiers = modifiers;
	
	// Adjust it a bit -- for example, if we just turned off the Right Shift, but the Left shift is on,
	// we want the shift bit to still be on
	if (_modifiers & B_LEFT_SHIFT_KEY || _modifiers & B_RIGHT_SHIFT_KEY)
		_modifiers |= B_SHIFT_KEY;
	if (_modifiers & B_LEFT_COMMAND_KEY || _modifiers & B_RIGHT_COMMAND_KEY)
		_modifiers |= B_COMMAND_KEY;
	if (_modifiers & B_LEFT_CONTROL_KEY || _modifiers & B_RIGHT_CONTROL_KEY)
		_modifiers |= B_CONTROL_KEY;
	if (_modifiers & B_LEFT_OPTION_KEY || _modifiers & B_RIGHT_OPTION_KEY)
		_modifiers |= B_OPTION_KEY;
	
	
	// Set the map to use for key char lookups
	if (_modifiers & B_CONTROL_KEY)
		_modifierIndex = CONTROL_CHAR_TABLE;
	else if (_modifiers & B_OPTION_KEY && _modifiers & B_CAPS_LOCK && _modifiers & B_SHIFT_KEY)
		_modifierIndex = OPTION_CAPS_SHIFT_CHAR_TABLE;
	else if (_modifiers & B_OPTION_KEY && _modifiers & B_CAPS_LOCK)
		_modifierIndex = OPTION_CAPS_CHAR_TABLE;
	else if (_modifiers & B_OPTION_KEY && _modifiers & B_SHIFT_KEY)
		_modifierIndex = OPTION_SHIFT_CHAR_TABLE;
	else if (_modifiers & B_OPTION_KEY)
		_modifierIndex = OPTION_CHAR_TABLE;
	else if (_modifiers & B_CAPS_LOCK && _modifiers & B_SHIFT_KEY)
		_modifierIndex = CAPS_SHIFT_CHAR_TABLE;
	else if (_modifiers & B_CAPS_LOCK)
		_modifierIndex = CAPS_CHAR_TABLE;
	else if (_modifiers & B_SHIFT_KEY)
		_modifierIndex = SHIFT_CHAR_TABLE;
	else
		_modifierIndex = NORMAL_CHAR_TABLE;
}


status_t
SoftKeyboard::ReadConfig(const char * filePath, const BMessage * embedParams)
{
	ssize_t					amt;
	int						fd;				// File descriptor for the config file
	kbd_file_header			fileHeader;		// The file header
	kbd_cell_info			* cells;
	soft_key_info			* keys;
	special_drawing_info	* drawing;
	int						i;
	status_t				err;
	
	
	// **** Read in the Config File *********************************************
	// Open the file
	fd = open(filePath, O_RDONLY | O_BINARY);
	if (fd < 0)
		return fd;
	
	// Read the header
	lseek(fd, 0, SEEK_SET);
	amt = read(fd, &fileHeader, sizeof(fileHeader));
	if (amt < 0)
		return amt;
	if (amt != sizeof(fileHeader) ||
			strcmp(fileHeader.magic_1, "kbdfile") != 0 ||
			strcmp(fileHeader.magic_2, "kbdfile") != 0)
		return B_BAD_VALUE;
	
	// Read the keyboard info
	lseek(fd, fileHeader.kbd_info_start, SEEK_SET);
	amt = read(fd, &_kbdInfo, sizeof(_kbdInfo));
	if (amt < 0)
		return amt;
	if (amt != (ssize_t) sizeof(_kbdInfo))
		return B_BAD_VALUE;
	
	// Read the cell infos
	cells = (kbd_cell_info *) malloc(sizeof(kbd_cell_info) * fileHeader.kbd_cell_count);
	lseek(fd, fileHeader.kbd_cell_start, SEEK_SET);
	amt = read(fd, cells, sizeof(kbd_cell_info) * fileHeader.kbd_cell_count);
	if (amt < 0)
		return amt;
	if (amt != (ssize_t) (sizeof(kbd_cell_info) * fileHeader.kbd_cell_count))
		return B_BAD_VALUE;
	
	// Read the key infos
	keys = (soft_key_info *) malloc(sizeof(soft_key_info) * fileHeader.kbd_key_count);
	lseek(fd, fileHeader.kbd_key_start, SEEK_SET);
	amt = read(fd, keys, sizeof(soft_key_info) * fileHeader.kbd_key_count);
	if (amt < 0)
		return amt;
	if (amt != (ssize_t) sizeof(soft_key_info) * fileHeader.kbd_key_count)
		return B_BAD_VALUE;
	
	// Read the special drawing infos
	drawing = (special_drawing_info *) malloc(sizeof(special_drawing_info) * fileHeader.drawing_info_count);
	lseek(fd, fileHeader.drawing_info_start, SEEK_SET);
	amt = read(fd, drawing, sizeof(special_drawing_info) * fileHeader.drawing_info_count);
	if (amt < 0)
		return amt;
	int32 blah = sizeof(special_drawing_info) * fileHeader.drawing_info_count;
	if (amt != blah)
		return B_BAD_VALUE;
	
	
	
	// **** Parse the embedParams message ***************************************
	
	// NULL is okay
	if (embedParams)
	{
		const char	* paramValStr;
		char 		* endptr;
		float		valF;
		int32		valI32;
		rgb_color	valCol;
		
		// borderWidth
		err = embedParams->FindString("borderwidth", &paramValStr);
		if (err == B_OK)
		{
			valF = (float) strtod(paramValStr, &endptr);
			if (endptr != paramValStr)
				_kbdInfo.borderWidth = valF;
		}
		
		// keyUpColor
		err = find_color(embedParams, "keyupcolor", &valCol);
		if (err == B_OK)
			_kbdInfo.keyUpColor = valCol;
		
		// keyDownColor
		err = find_color(embedParams, "keydowncolor", &valCol);
		if (err == B_OK)
			_kbdInfo.keyDownColor = valCol;
		
		// specialKeyUpColor
		err = find_color(embedParams, "specialkeyupcolor", &valCol);
		if (err == B_OK)
			_kbdInfo.specialKeyUpColor = valCol;
		
		// specialKeyDownColor
		err = find_color(embedParams, "specialkeydowncolor", &valCol);
		if (err == B_OK)
			_kbdInfo.specialKeyDownColor = valCol;
		
		// borderColor
		err = find_color(embedParams, "bordercolor", &valCol);
		if (err == B_OK)
			_kbdInfo.borderColor = valCol;
		
		// textColor
		err = find_color(embedParams, "textcolor", &valCol);
		if (err == B_OK)
			_kbdInfo.textColor = valCol;
		
		// fontFamily
		err = embedParams->FindString("fontfamily", &paramValStr);
		if (err == B_OK)
			strncpy(_kbdInfo.fontFamily, paramValStr, B_FONT_FAMILY_LENGTH + 1);
		
		// fontStyle
		err = embedParams->FindString("fontstyle", &paramValStr);
		if (err == B_OK)
			strncpy(_kbdInfo.fontStyle, paramValStr, B_FONT_STYLE_LENGTH + 1);
		
		// normalFontSize
		err = embedParams->FindString("normalfontsize", &paramValStr);
		if (err == B_OK)
		{
			valF = (float) strtod(paramValStr, &endptr);
			if (endptr != paramValStr)
				_kbdInfo.normalFontSize = valF;
		}
		
		// cellWidth
		err = embedParams->FindString("cellwidth", &paramValStr);
		if (err == B_OK)
		{
			valF = (float) strtod(paramValStr, &endptr);
			if (endptr != paramValStr)
				_kbdInfo.cellWidth = valF;
		}
		
		// cellHeight
		err = embedParams->FindString("cellheight", &paramValStr);
		if (err == B_OK)
		{
			valF = (float) strtod(paramValStr, &endptr);
			if (endptr != paramValStr)
				_kbdInfo.cellHeight = valF;
		}
		
		// drawingMode
		err = embedParams->FindString("drawingmode", &paramValStr);
		if (err == B_OK)
		{
			if (strcasecmp("normal", paramValStr) == 0)
				_kbdInfo.drawingMode = 0;
			else if (strcasecmp("rounded", paramValStr) == 0)
				_kbdInfo.drawingMode = 1;
			else if (strcasecmp("3d", paramValStr) == 0)
				_kbdInfo.drawingMode = 2;
		}
		
		// roundedRadius
		err = embedParams->FindString("roundedradius", &paramValStr);
		if (err == B_OK)
		{
			valF = (float) strtod(paramValStr, &endptr);
			if (endptr != paramValStr)
				_kbdInfo.roundedRadius = valF;
		}
		
		// specialFontFamily
		err = embedParams->FindString("fontfamily", &paramValStr);
		if (err == B_OK)
			strncpy(_kbdInfo.specialFontFamily, paramValStr, B_FONT_FAMILY_LENGTH + 1);
		
		// specialFontStyle
		err = embedParams->FindString("fontstyle", &paramValStr);
		if (err == B_OK)
			strncpy(_kbdInfo.specialFontStyle, paramValStr, B_FONT_STYLE_LENGTH + 1);
		
		// specialFontSize
		err = embedParams->FindString("specialfontsize", &paramValStr);
		if (err == B_OK)
		{
			valF = (float) strtod(paramValStr, &endptr);
			if (endptr != paramValStr)
				_kbdInfo.specialFontSize = valF;
		}
		
		// bevelWidth
		err = embedParams->FindString("bevelwidth", &paramValStr);
		if (err == B_OK)
		{
			valI32 = (int32) strtol(paramValStr, &endptr, 10);
			if (endptr != paramValStr)
				_kbdInfo.bevelWidth = valI32;
		}
		
		// bevelLeftColorUp
		err = find_color(embedParams, "bevelleftcolorup", &valCol);
		if (err == B_OK)
			_kbdInfo.bevelLeftColorUp = valCol;
		
		// bevelLeftColorDown
		err = find_color(embedParams, "bevelleftcolordown", &valCol);
		if (err == B_OK)
			_kbdInfo.bevelLeftColorDown = valCol;
		
		// bevelTopColorUp
		err = find_color(embedParams, "beveltopcolorup", &valCol);
		if (err == B_OK)
			_kbdInfo.bevelTopColorUp = valCol;
		
		// bevelTopColorDown
		err = find_color(embedParams, "beveltopcolordown", &valCol);
		if (err == B_OK)
			_kbdInfo.bevelTopColorDown = valCol;
		
		// bevelRightColorUp
		err = find_color(embedParams, "bevelrightcolorup", &valCol);
		if (err == B_OK)
			_kbdInfo.bevelRightColorUp = valCol;
		
		// bevelRightColorDown
		err = find_color(embedParams, "bevelrightcolordown", &valCol);
		if (err == B_OK)
			_kbdInfo.bevelRightColorDown = valCol;
		
		// bevelBottomColorUp
		err = find_color(embedParams, "bevelbottomcolorup", &valCol);
		if (err == B_OK)
			_kbdInfo.bevelBottomColorUp = valCol;
		
		// bevelBottomColorDown
		err = find_color(embedParams, "bevelbottomcolordown", &valCol);
		if (err == B_OK)
			_kbdInfo.bevelBottomColorDown = valCol;
		
		// pressShiftUpX
		err = embedParams->FindString("pressshiftupx", &paramValStr);
		if (err == B_OK)
		{
			valI32 = (int32) strtol(paramValStr, &endptr, 10);
			if (endptr != paramValStr)
				_kbdInfo.pressShiftUpX = valI32;
		}
		
		// pressShiftUpY
		err = embedParams->FindString("pressshiftupy", &paramValStr);
		if (err == B_OK)
		{
			valI32 = (int32) strtol(paramValStr, &endptr, 10);
			if (endptr != paramValStr)
				_kbdInfo.pressShiftUpY = valI32;
		}
		
		// pressShiftDownX
		err = embedParams->FindString("pressshiftdownx", &paramValStr);
		if (err == B_OK)
		{
			valI32 = (int32) strtol(paramValStr, &endptr, 10);
			if (endptr != paramValStr)
				_kbdInfo.pressShiftDownX = valI32;
		}
		
		// pressShiftDownY
		err = embedParams->FindString("pressshiftdowny", &paramValStr);
		if (err == B_OK)
		{
			valI32 = (int32) strtol(paramValStr, &endptr, 10);
			if (endptr != paramValStr)
				_kbdInfo.pressShiftDownY = valI32;
		}
		
		
	}
	
	
	// **** Process the Configuration and Construct Everything ******************
	
	
	// Set the default font
	_defaultFont.SetFamilyAndStyle(_kbdInfo.fontFamily, _kbdInfo.fontStyle);
	_defaultFont.SetSize(_kbdInfo.normalFontSize);
	
	// Set the default special font
	BFont defaultSpecialFont;
	defaultSpecialFont.SetFamilyAndStyle(_kbdInfo.specialFontFamily, _kbdInfo.specialFontStyle);
	defaultSpecialFont.SetSize(_kbdInfo.specialFontSize);
	
	// Choose the proper drawing function
	if (_kbdInfo.drawingMode == 1)
		_DrawKey = SoftKeyboard::DrawRounded;
	else if (_kbdInfo.drawingMode == 2)
		_DrawKey = SoftKeyboard::Draw3D;
	else
		_DrawKey = SoftKeyboard::DrawNormal;
	
	// Fill in the keys pointer array with NULLs
	for (i=0; i<256; ++i)
		_keys[i] = NULL;
	
	// Take the list of soft_key_infos and turn them into Key objects
	for (i=0; i<fileHeader.kbd_key_count; ++i)
	{
		uint8 keyCode = keys[i].keyCode;
		int8 drawInfo = keys[i].specialDrawingInfo;
		
		// Create the key object
		_keys[keyCode] = new Key(keys[i].keyCode);
		
		// Do the special drawing info
		if (drawInfo > 0 && drawInfo <= fileHeader.drawing_info_count) // <= b/c drawInfo is n+1
		{
			--drawInfo;
			// The label
			if (drawing[drawInfo].specialLabel[0] != '\0')
				_keys[keyCode]->_label = strdup(drawing[drawInfo].specialLabel);
			
			
			// The font family and style are picked in this order
			// 1. If there is a font in the config file, use that.  Any unspecified parts come from "defaultSpecial"
			// 2. If there is a special label, but nothing in the config use the "defaultSpecial"
			// 3. Otherwise, use the default / don't assign a font
			
			// Case 1: Specified in Config
			
			// The font family and style
			if (drawing[drawInfo].fontFamily[0] != '\0' &&
						drawing[drawInfo].fontStyle[0] != '\0')
			{
				_keys[keyCode]->_font = new BFont(defaultSpecialFont);
				_keys[keyCode]->_font->SetFamilyAndStyle(drawing[drawInfo].fontFamily,
															drawing[drawInfo].fontStyle);
				if (drawing[drawInfo].fontSize > 0)
					_keys[keyCode]->_font->SetSize(drawing[drawInfo].fontSize);
			}
			else if (drawing[drawInfo].fontSize > 0)
			{
				_keys[keyCode]->_font = new BFont(defaultSpecialFont);
				_keys[keyCode]->_font->SetSize(drawing[drawInfo].fontSize);
			}
			
			// Case 2: Default Special
			
			else if (_keys[keyCode]->_label)
			{
				_keys[keyCode]->_font = new BFont(defaultSpecialFont);
			}			
			
			// Case 3: Default / none

			
		}
	}
	
	// Take the list of kbd_cell_infos and turn them into Cell objects
	_cells = new Cell[fileHeader.kbd_cell_count];
	for (i=0; i<fileHeader.kbd_cell_count; ++i)
	{
		Cell * cell = _cells + i;
		
		// Set the joins for the cell
		cell->join_left = cells[i].join_left;
		cell->join_top = cells[i].join_top;
		cell->join_right = cells[i].join_right;
		cell->join_bottom = cells[i].join_bottom;
		
		// Set the rect for the cell
		int row = i / _kbdInfo.xCellCount;
		int col = i % _kbdInfo.xCellCount;
		
		float topLeftOffset = floor(_kbdInfo.borderWidth / 2.0); // this is floor because it's the bottom right of the non existing cells
		float left = col * _kbdInfo.cellWidth + topLeftOffset;
		float top = row * _kbdInfo.cellHeight + topLeftOffset;
		float right = ((col+1) * _kbdInfo.cellWidth) - 1 + topLeftOffset;
		float bottom = ((row+1) * _kbdInfo.cellHeight) - 1 + topLeftOffset;
		
		// Pull off some for the border, if appropriate
		if (!cell->join_left)
			left += ceil(_kbdInfo.borderWidth / 2.0);
		if (!cell->join_top)
			top += ceil(_kbdInfo.borderWidth / 2.0);
		if (!cell->join_right)
			right -= floor(_kbdInfo.borderWidth / 2.0);
		if (!cell->join_bottom)
			bottom -= floor(_kbdInfo.borderWidth / 2.0);
		
		cell->rect.Set(left, top, right, bottom);
		
		if (_keys[cells[i].key])
		{
			_keys[cells[i].key]->AddCell(_cells + i);
			cell->key = cells[i].key;
		}
		
	}
	
	// Calculate the corners that are normal corners, as opposed to straight edges
	// In the boring, flat drawing mode, this doesn't matter.  In rounded and 3d, it does.
	if (_DrawKey != SoftKeyboard::DrawNormal)
	{
		int row, col;
		uint8 keyCode;
		
		int rightColumn = _kbdInfo.xCellCount - 1;
		int bottomRow =  _kbdInfo.yCellCount - 1;
		
		// Left Top
		_cells[rc2index(0, 0)].corner_LT = 1;
		for (row = 1; row < _kbdInfo.yCellCount; ++row)		// Left column
			_cells[rc2index(row, 0)].corner_LT = cells[rc2index(row, 0)].key != _cells[rc2index(row-1, 0)].key ? 1 : 0;
		for (col = 1; col < _kbdInfo.xCellCount; ++col)		// Top Row
			_cells[rc2index(0, col)].corner_LT = cells[rc2index(0, col)].key != _cells[rc2index(0, col-1)].key ? 1 : 0;
		for (row = 1; row < _kbdInfo.yCellCount; ++row)
		{
			for (col = 1; col < _kbdInfo.xCellCount; ++col)
			{
				keyCode = cells[rc2index(row, col)].key;
				if (keyCode != cells[rc2index(row-1, col)].key
						&& keyCode != cells[rc2index(row, col-1)].key)
					_cells[rc2index(row, col)].corner_LT = 1;
				else
					_cells[rc2index(row, col)].corner_LT = 0;
			}
		}
		
		// Right Top
		_cells[rc2index(0, rightColumn)].corner_RT = 1;
		for (row = 1; row < _kbdInfo.yCellCount; ++row)		// Right Column
			_cells[rc2index(row, rightColumn)].corner_RT = cells[rc2index(row, rightColumn)].key != _cells[rc2index(row-1, rightColumn)].key ? 1 : 0;
		for (col = 0; col < rightColumn; ++col)				// Top  Row
			_cells[rc2index(0, col)].corner_RT = cells[rc2index(0, col)].key != _cells[rc2index(0, col+1)].key ? 1 : 0;
		for (row = 1; row < _kbdInfo.yCellCount; ++row)
		{
			for (col = 0; col < rightColumn; ++col)
			{
				keyCode = cells[rc2index(row, col)].key;
				if (keyCode != cells[rc2index(row-1, col)].key
						&& keyCode != cells[rc2index(row, col+1)].key)
					_cells[rc2index(row, col)].corner_RT = 1;
				else
					_cells[rc2index(row, col)].corner_RT = 0;
			}
		}
		
		// Left Bottom
		_cells[rc2index(bottomRow, 0)].corner_LB = 1;
		for (row = 0; row < bottomRow; ++row)				// Left column
			_cells[rc2index(row, 0)].corner_LB = cells[rc2index(row, 0)].key != _cells[rc2index(row+1, 0)].key ? 1 : 0;
		for (col = 1; col < _kbdInfo.xCellCount; ++col)		// Bottom Row
			_cells[rc2index(bottomRow, col)].corner_LB = cells[rc2index(bottomRow, col)].key != _cells[rc2index(bottomRow, col-1)].key ? 1 : 0;
		for (row = 0; row < bottomRow; ++row)
		{
			for (col = 1; col < _kbdInfo.xCellCount; ++col)
			{
				keyCode = cells[rc2index(row, col)].key;
				if (keyCode != cells[rc2index(row+1, col)].key
						&& keyCode != cells[rc2index(row, col-1)].key)
					_cells[rc2index(row, col)].corner_LB = 1;
				else
					_cells[rc2index(row, col)].corner_LB = 0;
			}
		}
		
		// Right Bottom
		_cells[rc2index(bottomRow, rightColumn)].corner_RB = 1;
		for (row = 0; row < bottomRow; ++row)				// Right column
			_cells[rc2index(row, rightColumn)].corner_RB = cells[rc2index(row, rightColumn)].key != _cells[rc2index(row+1, rightColumn)].key ? 1 : 0;
		for (col = 0; col < rightColumn; ++col)				// Bottom Row
			_cells[rc2index(bottomRow, col)].corner_RB = cells[rc2index(bottomRow, col)].key != _cells[rc2index(bottomRow, col+1)].key ? 1 : 0;
		for (row = 0; row < bottomRow; ++row)
		{
			for (col = 0; col < rightColumn; ++col)
			{
				keyCode = cells[rc2index(row, col)].key;
				if (keyCode != cells[rc2index(row+1, col)].key
						&& keyCode != cells[rc2index(row, col+1)].key)
					_cells[rc2index(row, col)].corner_RB = 1;
				else
					_cells[rc2index(row, col)].corner_RB = 0;
			}
		}
	}
	
	// Calculate the corners that are concave.
	// They require special drawing -- if there is a rounded corner, the inside
	// thingy needs to be drawn.  If there isn't, there are a few pixels that will
	// be wrong if not special cased b/c of the insetting of the rects.
	{
		int row, col;
		
		// Left Top
		for (col = 0; col < _kbdInfo.xCellCount; ++col)
			_cells[rc2index(0, col)].concave_LT = 0;
		for (row = 0; row < _kbdInfo.yCellCount; ++row)
			_cells[rc2index(row, 0)].concave_LT = 0;
		for (row = 1; row < _kbdInfo.yCellCount; ++row)
		{
			for (col = 1; col < _kbdInfo.xCellCount; ++col)
			{
				uint8 keyCode = cells[rc2index(row, col)].key;
				if (keyCode != cells[rc2index(row-1, col-1)].key
						&& keyCode == cells[rc2index(row-1, col)].key
						&& keyCode == cells[rc2index(row, col-1)].key)
				{
					_cells[rc2index(row, col)].concave_LT = 1;
				}
				else
				{
					_cells[rc2index(row, col)].concave_LT = 0;
				}
			}
		}
		
		// Right Top
		for (col = 0; col < _kbdInfo.xCellCount; ++col)
			_cells[rc2index(0, col)].concave_RT = 0;
		for (row = 0; row < _kbdInfo.yCellCount; ++row)
			_cells[rc2index(row, _kbdInfo.xCellCount-1)].concave_RT = 0;
		for (row = 1; row < _kbdInfo.yCellCount; ++row)
		{
			for (col = 0; col < _kbdInfo.xCellCount-1; ++col)
			{
				uint8 keyCode = cells[rc2index(row, col)].key;
				if (keyCode != cells[rc2index(row-1, col+1)].key
						&& keyCode == cells[rc2index(row-1, col)].key
						&& keyCode == cells[rc2index(row, col+1)].key)
				{
					_cells[rc2index(row, col)].concave_RT = 1;
				}
				else
				{
					_cells[rc2index(row, col)].concave_RT = 0;
				}
			}
		}
		
		// Right Bottom
		for (col = 0; col < _kbdInfo.xCellCount; ++col)
			_cells[rc2index(_kbdInfo.yCellCount-1, col)].concave_RB = 0;
		for (row = 0; row < _kbdInfo.yCellCount; ++row)
			_cells[rc2index(row, _kbdInfo.xCellCount-1)].concave_RB = 0;
		for (row = 0; row < _kbdInfo.yCellCount-1; ++row)
		{
			for (col=0; col < _kbdInfo.xCellCount-1; ++col)
			{
				uint8 keyCode = cells[rc2index(row, col)].key;
				if (keyCode != cells[rc2index(row+1, col+1)].key
						&& keyCode == cells[rc2index(row+1, col)].key
						&& keyCode == cells[rc2index(row, col+1)].key)
				{
					_cells[rc2index(row, col)].concave_RB = 1;
				}
				else
				{
					_cells[rc2index(row, col)].concave_RB = 0;
				}
			}
		}
		
		// Left Bottom
		for (col = 0; col < _kbdInfo.xCellCount; ++col)
			_cells[rc2index(_kbdInfo.yCellCount-1, col)].concave_LB = 0;
		for (row = 0; row < _kbdInfo.yCellCount; ++row)
			_cells[rc2index(row, 0)].concave_LB = 0;
		for (row = 0; row < _kbdInfo.yCellCount-1; ++row)
		{
			for (col = 1; col < _kbdInfo.xCellCount; ++col)
			{
				uint8 keyCode = cells[rc2index(row, col)].key;
				if (keyCode != cells[rc2index(row+1, col-1)].key
						&& keyCode == cells[rc2index(row, col-1)].key
						&& keyCode == cells[rc2index(row+1, col)].key)
				{
					_cells[rc2index(row, col)].concave_LB = 1;
				}
				else
				{
					_cells[rc2index(row, col)].concave_LB = 0;
				}
			}
		}
	}
	
	// Go tell the keys that are modifiers their key
	// Caps Lock
	i = _keyMap->caps_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_CAPS_LOCK;
	// Scroll Lock
	i = _keyMap->scroll_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_SCROLL_LOCK;
	// Num Lock
	i = _keyMap->num_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_NUM_LOCK;
	// Left Shift Key
	i = _keyMap->left_shift_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_LEFT_SHIFT_KEY | B_SHIFT_KEY;
	// Right Shift Key
	i = _keyMap->right_shift_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_RIGHT_SHIFT_KEY | B_SHIFT_KEY;
	// Left Command Key
	i = _keyMap->left_command_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_LEFT_COMMAND_KEY | B_COMMAND_KEY;
	// Right Command Key
	i = _keyMap->right_command_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_RIGHT_COMMAND_KEY | B_COMMAND_KEY;
	// Left Control Key
	i = _keyMap->left_control_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_LEFT_CONTROL_KEY | B_CONTROL_KEY;
	// Right Control Key
	i = _keyMap->right_control_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_RIGHT_CONTROL_KEY | B_CONTROL_KEY;
	// Left Option Key
	i = _keyMap->left_option_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_LEFT_OPTION_KEY | B_OPTION_KEY;
	// Right Option Key
	i = _keyMap->right_option_key;
	if (i < 256)
		if (_keys[i])
			_keys[i]->_modifierKey |= B_RIGHT_OPTION_KEY | B_OPTION_KEY;
	
	// Don't leak
	free(cells);
	free(keys);
	free(drawing);	
	close(fd);
	
	
	// **** Load the Characters and Calculate their Width ***********************
	
	// Spawn the second thread ....
	_keyCharsThread = spawn_thread(GetTheRestOfTheKeyChars, "key_chars_fetcher", B_LOW_PRIORITY, (void *) this);
	
	// Spawn and run the first ...
	_normalKeyCharsThread = spawn_thread(GetTheNormalKeyChars, "key_chars_fetcher_normal", B_DISPLAY_PRIORITY, (void *) this);
	resume_thread(_normalKeyCharsThread);
	

	// **** Look For Keys with Graphics *****************************************
	const char * keyGraphicsDir;
	
	if (embedParams) // NULL is okay
	{
		err = embedParams->FindString("graphics", &keyGraphicsDir);
		if (err == B_OK)
		{
			// Look in the graphics directory for files that fit the pattern
			// key_<keynum>_<state>.png
			// State: Up -- U -- 0x01
			// 		  Dn -- D -- 0x02
			// Only pngs are supported.  If this is a huge problem, support
			// could be added
			
			// Find the graphics directory
			BString dirPath;
			UnEscapePathString(&dirPath, keyGraphicsDir);
			if (dirPath[dirPath.Length()-2] != '/')
				dirPath += "/";
			
			InitResources(dirPath.String());
			
			BDirectory dir(dirPath.String());
			entry_ref ref;
			uint8 keyCode, states;
			
			dir.Rewind();
			
			while (B_OK == dir.GetNextRef(&ref))
			{
				if(ParseGraphicFileName(ref.name, keyCode, states))
				{
					Key * key = _keys[keyCode];
					if (key)
					{
						// Load this graphic and assign it to a key
						BBitmap * bitmap = (BBitmap *) ResourceBitmap(ref.name);
						
						// If the bitmap was read in okay
						if (bitmap)
						{
							Key * key = _keys[keyCode];
							if (states & 0x01)
							{
								key->_upBitmap = bitmap;
							}
							if (states & 0x02)
							{
								key->_downBitmap = bitmap;
							}
						}
					}
				}
			}
			
			
		}
	}
	
	return B_OK;
}


void
SoftKeyboard::ReleaseModifiers(bool noShift)
{
	int i;
	
	// Caps Lock
	//i = _keyMap->caps_key;
	//if (i < 256)
	//	ForceKeyRelease((uint8) i);
	
	// Scroll Lock
	i = _keyMap->scroll_key;
	if (i < 256)
		ForceKeyRelease((uint8) i);
	
	// Num Lock
	i = _keyMap->num_key;
	if (i < 256)
		ForceKeyRelease((uint8) i);
	
	if (!noShift)
	{
		// Left Shift Key
		i = _keyMap->left_shift_key;
		if (i < 256)
			ForceKeyRelease((uint8) i);
		
		// Right Shift Key
		i = _keyMap->right_shift_key;
		if (i < 256)
			ForceKeyRelease((uint8) i);
	}
		
	// Left Command Key
	i = _keyMap->left_command_key;
	if (i < 256)
		ForceKeyRelease((uint8) i);
	
	// Right Command Key
	i = _keyMap->right_command_key;
	if (i < 256)
		ForceKeyRelease((uint8) i);
	
	// Left Control Key
	i = _keyMap->left_control_key;
	if (i < 256)
		ForceKeyRelease((uint8) i);
	
	// Right Control Key
	i = _keyMap->right_control_key;
	if (i < 256)
		ForceKeyRelease((uint8) i);
	
	// Left Option Key
	i = _keyMap->left_option_key;
	if (i < 256)
		ForceKeyRelease((uint8) i);
	
	// Right Option Key
	i = _keyMap->right_option_key;
	if (i < 256)
		ForceKeyRelease((uint8) i);
	
}


void
SoftKeyboard::ForceKeyRelease(uint8 keyCode)
{
	Key * key = _keys[keyCode];
	
	if (key && key->_state != Key::UP)
	{
		key->_state = Key::UP;
		AdjustKeyState(keyCode, false);
		if (key->_modifierKey != 0)
		{
			ModifiersChanged(_modifiers & ~key->_modifierKey);
			SendModifiersChanged();
		}
		SendKeyUp(key);
	}
}

void
SoftKeyboard::RequestTarget()
{
	// Find the device
	BInputDevice * device = find_input_device("SoftKeyboardDevice");
	if (!device)
		return ;
	
	// Create the BMessage to send it
	BMessage request;
	BMessage reply('msng');
	request.AddMessage("message", &reply);
	request.AddMessenger("messenger", BMessenger(this));
	
	// Send it
	device->Control('Fmgr', &request);
	
	// A reply will come in the MessageReceived
}


// If the target is invalid, it will ask for another one
// This should keep a queue of events and flush that when
// the input server device is reconnected / back
void
SoftKeyboard::SendMessageToTarget(BMessage * msg)
{
	if (!_target.IsValid())
		RequestTarget();
	else
		_target.SendMessage(msg);
}


void
SoftKeyboard::SendKeyDown(Key * key)
{
	BMessage	msg('enqu');
	BMessage	event(B_KEY_DOWN);
	bool		isDead;
	int32		* deadTableToUse;
	const char	* displayStr;
	
	// Fill in the crap
	CreateGenericKeyMessage(key->_keyCode, &event, isDead, deadTableToUse);
	
	// Key Repeats
	if (_keyRepeatCount > 0)
		event.AddInt32("be:key_repeat", _keyRepeatCount);
	
	// Add event to msg
	msg.AddMessage("message", &event);
	
	if (_deadTableToUse && _ignoreNextKeyDown)
	{
		// There is a pending dead key
		_ignoreNextKeyDown = 0;
	}
	else if (_deadTableToUse && !_ignoreNextKeyDown)
	{
		event.FindString("bytes", &displayStr);
		if (displayStr && displayStr[0])	
		{
			SendMessageToTarget(&msg);
		}
	}
	else if (isDead)
	{
		// There is no pending dead key, but this is one
		// So postpone this event, and redraw everything
		_deadTableToUse = deadTableToUse;
		
		_ignoreNextKeyDown = key->_keyCode;		// On the next key down, don't send it
		_ignoreNextKeyUp = key->_keyCode;		// Don't send the next key up, but after that,
												// we want both key ups and downs
		
		Draw(Bounds());
	}
	else
	{
		SendMessageToTarget(&msg);
	}
}


void
SoftKeyboard::SendKeyUp(Key * key)
{
	if (_ignoreNextKeyUp != key->_keyCode)
	{
		BMessage msg('enqu'), event(B_KEY_UP);
		bool isDead;
		int32 * deadTableToUse;
		const char	* displayStr;
		
		// Fill in the crap
		CreateGenericKeyMessage(key->_keyCode, &event, isDead, deadTableToUse);
		
		// Add event to msg
		msg.AddMessage("message", &event);
		
		// if we're in dead mode and we're not that modifier up
		if (_deadTableToUse && _ignoreNextKeyUp == 0)
		{
			// if it's not a shift or caps lock key
			if (!(key->_modifierKey & B_SHIFT_KEY || key->_modifierKey & B_CAPS_LOCK))
			{
				// Get out of dead mode
				_deadTableToUse = NULL;
				_ignoreNextKeyUp = 0;
				_ignoreNextKeyDown = 0;
				Draw(Bounds());
			}
			
			// If the key isn't mapped, don't send the event
			event.FindString("bytes", &displayStr);
			if (!(displayStr && displayStr[0]))	
				return ;
		}
		
		SendMessageToTarget(&msg);
		
		return ;
	}
	else
	{
		_ignoreNextKeyUp = 0;
		_ignoreNextKeyDown = 0;
	}
}


void
SoftKeyboard::SendModifiersChanged()
{
	BMessage msg('enqu'), event(B_MODIFIERS_CHANGED);
	
	// The time
	event.AddInt64("when", real_time_clock_usecs());
	
	// The current modifiers
	event.AddInt32("modifiers", _modifiers);
	
	// The previous modifiers
	event.AddInt32("be:old_modifiers", _prevModifiers);
	
	// A bitmap of all of the states of all of the keys
	event.AddData("states", B_UINT8_TYPE, &_keyStates, 16);
	
	// Add event to msg
	msg.AddMessage("message", &event);
	
	// Send the message
	SendMessageToTarget(&msg);
}


void
SoftKeyboard::CreateGenericKeyMessage(uint8 keyCode, BMessage * event, bool & isDead, int32 *& deadTableToUse)
{
	char	displayChars[10];
	char	rawChar;
	float	stringWidth;
	char	* p;
	
	GetKeyDisplayChars(keyCode, displayChars, isDead, deadTableToUse, stringWidth, false);
	GetModifierIndepASCII(keyCode, rawChar);
	
	// The time
	event->AddInt64("when", real_time_clock_usecs());
	
	// Which Modifiers are down
	event->AddInt32("modifiers", _modifiers);
	
	// The Be hardware independent key code
	event->AddInt32("key", keyCode);
	
	// A bitmap of all of the states of all of the keys
	event->AddData("states", B_UINT8_TYPE, &_keyStates, 16);
	
	if (displayChars[0] == '\0')
	{
		// Unmapped Key
		if (event->what == B_KEY_DOWN)
			event->what = B_UNMAPPED_KEY_DOWN;
		else if (event->what == B_KEY_UP)
			event->what = B_UNMAPPED_KEY_UP;
	}
	else
	{
		// Three differnt ways of representing the same data
		event->AddInt32("raw_char", rawChar);
		event->AddString("bytes", displayChars);
		for (p = displayChars; *p; ++p)
			event->AddInt8("byte", *p);
	}	
}


void
SoftKeyboard::AdjustKeyState(int32 rawkey, bool pressed)
{
	int32 byte, bitmask;
	// 0 is the value used for unassigned keys
	if(rawkey!=0){
		byte = rawkey>>3;
		bitmask = 1 << (7 - (rawkey&7));
		if(pressed){
			_keyStates[byte] |= bitmask;
		}else{
			_keyStates[byte] &= ~bitmask;
		}
	}
}


void
SoftKeyboard::StartKeyRepeatNotifications()
{
	if (_keyRepeatRunner)
		return ;
	
	bigtime_t repeatDelay = GetRepeatDelay();
	_keyRepeatRunner = new BMessageRunner(	BMessenger(this),
											new BMessage(SFT_KBD_START_KBD_REPEAT),
											repeatDelay,
											1);
}

void
SoftKeyboard::StopKeyRepeatNotifications()
{
	delete _keyRepeatRunner;
	
	_keyRepeatCount = 0;
	_keyRepeatRunner = NULL;
}


void
SoftKeyboard::DrawNormal(BView * view, SoftKeyboard * kbd, Key * key)
{
	// Rect that is the union of all the cell rects
	BRect keyRectUnion;
	BRect cornerSpace;
	rgb_color bkgColor;
	
	// Get the info about what to draw (assuming that there's no special label)
	char	displayChars[10];
	bool	markDead;
	int32	* deadTable;
	float	stringWidth;
	
	kbd->GetKeyDisplayChars(key->_keyCode, displayChars, markDead, deadTable, stringWidth);
	
	// Choose the colors depending on the state
	if (key->_label || markDead)				// Use special color
		bkgColor = key->_state == Key::UP ? kbd->_kbdInfo.specialKeyUpColor : kbd->_kbdInfo.specialKeyDownColor;
	else										// Use Normal Color
		bkgColor = key->_state == Key::UP ? kbd->_kbdInfo.keyUpColor : kbd->_kbdInfo.keyDownColor;
	
	// For each cell that we own
	int8 i;
	for (i=0; i<key->_cellCount; ++i)
	{
		
		// Fill in the background
		BRect r(key->_cells[i]->rect);

		view->SetHighColor(bkgColor);
		view->FillRect(r);
		
		if (i == 0)
			keyRectUnion = r;
		else
			keyRectUnion = keyRectUnion | r;
		

		// Check for concave corners
		float borderCeil = ceil(kbd->_kbdInfo.borderWidth / 2.0) - 1;
		float borderFloor = floor(kbd->_kbdInfo.borderWidth / 2.0) - 1;
		
		// Left Top		
		if (key->_cells[i]->concave_LT)
		{
			cornerSpace.Set(r.left, r.top, r.left + borderCeil, r.top + borderCeil);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
		}
		
		// Right Top
		if (key->_cells[i]->concave_RT)
		{
			cornerSpace.Set(r.right - borderFloor, r.top, r.right, r.top + borderCeil);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
		}
		
		// Right Bottom
		if (key->_cells[i]->concave_RB)
		{
			cornerSpace.Set(r.right - borderFloor, r.bottom - borderFloor, r.right, r.bottom);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
		}
		
		// Left Bottom
		if (key->_cells[i]->concave_LB)
		{
			cornerSpace.Set(r.left, r.bottom - borderFloor, r.left + borderCeil, r.bottom);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
		}
	}
	
	BBitmap * bitmap = key->_state == Key::UP ? key->_upBitmap : key->_downBitmap;;
	
	if (bitmap)
	{
		
		BRect drawInto;
		BRect bitmapBounds = bitmap->Bounds();
		
		drawInto.left = keyRectUnion.left + (keyRectUnion.Width() / 2.0) - (bitmapBounds.Width() / 2.0);
		drawInto.top = keyRectUnion.top + (keyRectUnion.Height() / 2.0) - (bitmapBounds.Height() / 2.0);
		drawInto.right = drawInto.left + bitmapBounds.Width();
		drawInto.bottom = drawInto.top + bitmapBounds.Height();
		
		view->DrawBitmapAsync(bitmap, drawInto);
	}
	else if (key->_label)
	{
		// Draw the special label
		BPoint pt;
		BRect viewBounds;
		
		// If we have a font, use it, otherwise, use the default
		view->SetFont(key->_font ? key->_font : &(kbd->_defaultFont));
		
		// pt.y is the same as for the normal one
		pt.y = keyRectUnion.bottom - (kbd->_kbdInfo.cellHeight * 0.2);
		
		viewBounds = kbd->Bounds();
		pt.x = keyRectUnion.left + keyRectUnion.Width() / 2.0;
		if (pt.x < viewBounds.left + viewBounds.Width() / 2.0)
		{
			pt.x = keyRectUnion.left + (kbd->_kbdInfo.cellWidth * 0.4);
		}
		else
		{
			pt.x = keyRectUnion.right - (kbd->_kbdInfo.cellWidth * 0.4);
			pt.x -= view->StringWidth(key->_label);
		}
		
		view->SetHighColor(kbd->_kbdInfo.textColor);
		view->DrawString(key->_label, pt);
	}
	else
	{
		// Draw the Glyph
		BPoint	pt;
		
		pt.x = keyRectUnion.left + keyRectUnion.Width() / 2.0;
		pt.y = keyRectUnion.bottom - (kbd->_kbdInfo.cellHeight * 0.2);
		
		view->SetFont(&(kbd->_defaultFont));
		
		pt.x -= stringWidth / 2.0;
		
		pt.x = floor(pt.x)+1;
		pt.y = floor(pt.y);
		
		view->SetHighColor(kbd->_kbdInfo.textColor);
		view->DrawString(displayChars, pt);
	}
	
}


void
SoftKeyboard::DrawRounded(BView * view, SoftKeyboard * kbd, Key * key)
{
	// Rect that is the union of all the cell rects
	BRect keyRectUnion;
	BRect cornerSpace;
	rgb_color bkgColor;
	
	float curveRadius = kbd->_kbdInfo.roundedRadius;
	
	// Get the info about what to draw (assuming that there's no special label)
	char	displayChars[10];
	bool	markDead;
	int32	* deadTable;
	float	stringWidth;
	kbd->GetKeyDisplayChars(key->_keyCode, displayChars, markDead, deadTable, stringWidth);

	// Choose the colors depending on the state
	if (key->_label || markDead)				// Use special color
		bkgColor = key->_state == Key::UP ? kbd->_kbdInfo.specialKeyUpColor : kbd->_kbdInfo.specialKeyDownColor;
	else										// Use Normal Color
		bkgColor = key->_state == Key::UP ? kbd->_kbdInfo.keyUpColor : kbd->_kbdInfo.keyDownColor;
	
	// For each cell that we own
	int8 i;
	for (i=0; i<key->_cellCount; ++i)
	{
		// Fill in the background
		BRect r(key->_cells[i]->rect);
		view->SetHighColor(bkgColor);
		view->FillRect(r);
		
		if (i == 0)
			keyRectUnion = r;
		else
			keyRectUnion = keyRectUnion | r;
		

		// Check for concave corners
		// Concave corners have the rounded edge drawn into the opposing cell's area
		// Draw the negative space knockout
		float borderCeil = ceil(kbd->_kbdInfo.borderWidth / 2.0) - 1;
		float borderFloor = floor(kbd->_kbdInfo.borderWidth / 2.0) - 1;
		BRect cornerRect;
		
		float twiceCurveRadius = curveRadius + curveRadius;
		
		// Left Top
		if (key->_cells[i]->concave_LT)
		{
			// The pixels that don't match because of the rect thing
			cornerSpace.Set(r.left, r.top, r.left + borderCeil, r.top + borderCeil);
			
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
			
			// The negative space corner
			view->SetHighColor(bkgColor);
			cornerSpace.Set(r.left-curveRadius+2, r.top-curveRadius+2, r.left+1, r.top+1);
			view->FillRect(cornerSpace);
			
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			cornerSpace.Set(r.left-twiceCurveRadius+1, r.top-twiceCurveRadius+2, r.left+1, r.top+1);
			view->FillArc(cornerSpace, 270, 90);
		}
		
		// Right Top
		if (key->_cells[i]->concave_RT)
		{
			// The pixels that don't match because of the rect thing
			cornerSpace.Set(r.right - borderFloor, r.top, r.right, r.top + borderCeil);
			
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
			
			// The negative space corner
			view->SetHighColor(bkgColor);
			cornerSpace.Set(r.right, r.top-curveRadius+2, r.right+curveRadius-1, r.top+1);
			view->FillRect(cornerSpace);
			
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			cornerSpace.Set(r.right, r.top-twiceCurveRadius+2, r.right+twiceCurveRadius-1, r.top+1);
			view->FillArc(cornerSpace, 180, 90);
		}
		
		// Right Bottom
		if (key->_cells[i]->concave_RB)
		{
			// The pixels that don't match because of the rect thing
			cornerSpace.Set(r.right - borderFloor, r.bottom - borderFloor, r.right, r.bottom);
			
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
			
			// The negative space corner
			view->SetHighColor(bkgColor);
			cornerSpace.Set(r.right, r.bottom, r.right+curveRadius-1, r.bottom+curveRadius-1);
			view->FillRect(cornerSpace);
			
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			cornerSpace.Set(r.right, r.bottom, r.right+twiceCurveRadius-1, r.bottom+twiceCurveRadius-1);
			view->FillArc(cornerSpace, 90, 90);
		}
		
		// Left Bottom
		if (key->_cells[i]->concave_LB)
		{
			// The pixels that don't match because of the rect thing
			cornerSpace.Set(r.left, r.bottom - borderFloor, r.left + borderCeil, r.bottom);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
		
			// The negative space corner
			view->SetHighColor(bkgColor);
			cornerSpace.Set(r.left-curveRadius+2, r.bottom, r.left+1, r.bottom+curveRadius-1);
			view->FillRect(cornerSpace);
			
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			cornerSpace.Set(r.left-twiceCurveRadius+2, r.bottom, r.left+1, r.bottom+twiceCurveRadius-1);
			view->FillArc(cornerSpace, 0, 90);
		}



		// Check for normal corners
		// Normal corners have the rounded edge drawn into the cell area
		twiceCurveRadius = curveRadius + curveRadius + 1;
		// Left Top
		if (key->_cells[i]->corner_LT)
		{
			cornerRect.Set(r.left, r.top, r.left+curveRadius, r.top+curveRadius);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerRect);
			
			cornerRect.Set(r.left, r.top, r.left+twiceCurveRadius, r.top+twiceCurveRadius);
			view->SetHighColor(bkgColor);
			view->FillArc(cornerRect, 90, 90);
		}
		
		// Right Top
		if (key->_cells[i]->corner_RT)
		{
			cornerRect.Set(r.right-curveRadius, r.top, r.right, r.top+curveRadius);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerRect);
			
			cornerRect.Set(r.right-twiceCurveRadius, r.top, r.right, r.top+twiceCurveRadius);
			view->SetHighColor(bkgColor);
			view->FillArc(cornerRect, 0, 90);
		}
		
		// Right Bottom
		if (key->_cells[i]->corner_RB)
		{
			cornerRect.Set(r.right-curveRadius, r.bottom-curveRadius, r.right, r.bottom);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerRect);
			
			cornerRect.Set(r.right-twiceCurveRadius, r.bottom-twiceCurveRadius, r.right, r.bottom);
			view->SetHighColor(bkgColor);
			view->FillArc(cornerRect, 270, 90);
		}
		
		// Left Bottom
		if (key->_cells[i]->corner_LB)
		{
			cornerRect.Set(r.left, r.bottom-curveRadius, r.left+curveRadius, r.bottom);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerRect);
			
			cornerRect.Set(r.left, r.bottom-twiceCurveRadius, r.left+twiceCurveRadius, r.bottom);
			view->SetHighColor(bkgColor);
			view->FillArc(cornerRect, 180, 90);
		}
	}
	
	BBitmap * bitmap = key->_state == Key::UP ? key->_upBitmap : key->_downBitmap;;
	
	if (bitmap)
	{
		
		BRect drawInto;
		BRect bitmapBounds = bitmap->Bounds();
		
		drawInto.left = keyRectUnion.left + (keyRectUnion.Width() / 2.0) - (bitmapBounds.Width() / 2.0);
		drawInto.top = keyRectUnion.top + (keyRectUnion.Height() / 2.0) - (bitmapBounds.Height() / 2.0);
		drawInto.right = drawInto.left + bitmapBounds.Width();
		drawInto.bottom = drawInto.top + bitmapBounds.Height();
		
		view->DrawBitmapAsync(bitmap, drawInto);
	}
	else if (key->_label)
	{
		// Draw the special label
		BPoint pt;
		BRect viewBounds;
		
		// If we have a font, use it, otherwise, use the default
		view->SetFont(key->_font ? key->_font : &(kbd->_defaultFont));

		// pt.y is the same as for the normal one
		pt.y = keyRectUnion.bottom - (kbd->_kbdInfo.cellHeight * 0.2);
		
		viewBounds = kbd->Bounds();
		pt.x = keyRectUnion.left + keyRectUnion.Width() / 2.0;
		if (pt.x < viewBounds.left + viewBounds.Width() / 2.0)
		{
			pt.x = keyRectUnion.left + (kbd->_kbdInfo.cellWidth * 0.4);
		}
		else
		{
			pt.x = keyRectUnion.right - (kbd->_kbdInfo.cellWidth * 0.4);
			pt.x -= view->StringWidth(key->_label);
		}
		
		view->SetHighColor(kbd->_kbdInfo.textColor);
		view->DrawString(key->_label, pt);
	}
	else
	{
		// Draw the Glyph
		BPoint	pt;
		
		pt.x = keyRectUnion.left + keyRectUnion.Width() / 2.0;
		pt.y = keyRectUnion.bottom - (kbd->_kbdInfo.cellHeight * 0.2);
		
		view->SetFont(&(kbd->_defaultFont));
		
		pt.x -= stringWidth / 2.0;
		
		pt.x = floor(pt.x)+1;
		pt.y = floor(pt.y);
		
		view->SetHighColor(kbd->_kbdInfo.textColor);
		view->DrawString(displayChars, pt);
	}
}


static void
OffsetPoint(BPoint & pt, float x, float y)
{
	pt.x += x;
	pt.y += y;
}


static float
CornerPointOffset(uint32 normal, float multiplier)
{
	if (normal)
		return multiplier;
	return 0;
}


#define LEFT_EDGE_MULTIPLIER	1
#define RIGHT_EDGE_MULTIPLIER	-1
#define TOP_EDGE_MULTIPLIER		1
#define BOTTOM_EDGE_MULTIPLIER	-1


void
SoftKeyboard::Draw3D(BView * view, SoftKeyboard * kbd, Key * key)
{
	// Rect that is the union of all the cell rects
	BRect		keyRectUnion;
	rgb_color	bkgColor;
	rgb_color	leftColor, topColor, rightColor, bottomColor;
	
	int32		bevelWidth = kbd->_kbdInfo.bevelWidth;
	BPoint		keyCapShift;
	
	// Get the info about what to draw (assuming that there's no special label)
	char	displayChars[10];
	bool	markDead;
	int32	* deadTable;
	float	stringWidth;
	kbd->GetKeyDisplayChars(key->_keyCode, displayChars, markDead, deadTable, stringWidth);

	// Choose the background colors depending on the state
	if (key->_label || markDead)				// Use special color
		bkgColor = key->_state == Key::UP ? kbd->_kbdInfo.specialKeyUpColor : kbd->_kbdInfo.specialKeyDownColor;
	else										// Use Normal Color
		bkgColor = key->_state == Key::UP ? kbd->_kbdInfo.keyUpColor : kbd->_kbdInfo.keyDownColor;
	
	// Choose the bevel colors depeneding on the state
	if (key->_state == Key::UP)
	{
		leftColor = kbd->_kbdInfo.bevelLeftColorUp;
		topColor = kbd->_kbdInfo.bevelTopColorUp;
		rightColor = kbd->_kbdInfo.bevelRightColorUp;
		bottomColor = kbd->_kbdInfo.bevelBottomColorUp;
		keyCapShift.Set(kbd->_kbdInfo.pressShiftUpX, kbd->_kbdInfo.pressShiftUpY);
	}
	else
	{
		leftColor = kbd->_kbdInfo.bevelLeftColorDown;
		topColor = kbd->_kbdInfo.bevelTopColorDown;
		rightColor = kbd->_kbdInfo.bevelRightColorDown;
		bottomColor = kbd->_kbdInfo.bevelBottomColorDown;
		keyCapShift.Set(kbd->_kbdInfo.pressShiftDownX, kbd->_kbdInfo.pressShiftDownY);
	}
	
	// For filling in concave corners
	float borderCeil = ceil(kbd->_kbdInfo.borderWidth / 2.0) - 1;
	float borderFloor = floor(kbd->_kbdInfo.borderWidth / 2.0) - 1;
	
	// For each cell that we own
	int8 i;
	for (i=0; i<key->_cellCount; ++i)
	{
		// Fill in the background
		Cell * c = key->_cells[i];
		BRect r(c->rect);
		view->SetHighColor(bkgColor);
		view->FillRect(r);
		
		if (i == 0)
			keyRectUnion = r;
		else
			keyRectUnion = keyRectUnion | r;
		

		// Draw the lines
		view->SetPenSize(1);
		BPoint p1, p2;
		
		// Right
		view->SetHighColor(rightColor);
		p1.Set(r.right, r.top);
		p2.Set(r.right, r.bottom);
		if (!c->join_right)
		{
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, -1, CornerPointOffset(c->corner_RT, 1));
				OffsetPoint(p2, -1, CornerPointOffset(c->corner_RB, -1));
			}
		}
		
		// Bottom
		view->SetHighColor(bottomColor);
		p1.Set(r.left, r.bottom);
		p2.Set(r.right, r.bottom);
		if (!c->join_bottom)
		{
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, CornerPointOffset(c->corner_LB, 1), -1);
				OffsetPoint(p2, CornerPointOffset(c->corner_RB, -1), -1);
			}
		}
		
		// Left
		view->SetHighColor(leftColor);
		p1.Set(r.left, r.top);
		p2.Set(r.left, r.bottom);
		if (!c->join_left)
		{
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, 1, CornerPointOffset(c->corner_LT, 1));
				OffsetPoint(p2, 1, CornerPointOffset(c->corner_LB, -1));
			}
		}
		
		// Top
		view->SetHighColor(topColor);
		p1.Set(r.left, r.top);
		p2.Set(r.right, r.top);
		if (!c->join_top)
		{
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, CornerPointOffset(c->corner_LT, 1), 1);
				OffsetPoint(p2, CornerPointOffset(c->corner_RT, -1), 1);
			}
		}
		
		BRect cornerSpace;
		
		// Concave Left Top
		if (c->concave_LT)
		{
			cornerSpace.Set(r.left, r.top, r.left + borderCeil, r.top + borderCeil);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
			
			view->SetHighColor(topColor);
			p1.Set(r.left, r.top + borderCeil + 1);
			p2.Set(r.left + borderCeil, r.top + borderCeil + 1);
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, 0, 1);
				OffsetPoint(p2, 1, 1);
			}
			
			view->SetHighColor(leftColor);
			p1.Set(r.left + borderCeil + 1, r.top);
			p2.Set(r.left + borderCeil + 1, r.top + borderCeil + 1);
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, 1, 0);
				OffsetPoint(p2, 1, 1);
			}
		}
		
		// Concave Right Top
		if (c->concave_RT)
		{
			cornerSpace.Set(r.right - borderFloor, r.top, r.right, r.top + borderCeil);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
			
			view->SetHighColor(topColor);
			p1.Set(r.right - borderFloor - 1, r.top + borderCeil + 1);
			p2.Set(r.right, r.top + borderCeil + 1);
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, -1, 1);
				OffsetPoint(p2, 0, 1);
			}
			
			view->SetHighColor(rightColor);
			p1.Set(r.right - borderFloor - 1, r.top);
			p2.Set(r.right - borderFloor - 1, r.top + borderCeil + 1);
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, -1, 0);
				OffsetPoint(p2, -1, 1);
			}
		}
		
		// Concave Right Bottom
		if (c->concave_RB)
		{
			cornerSpace.Set(r.right - borderFloor, r.bottom - borderFloor, r.right, r.bottom);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
			
			view->SetHighColor(bottomColor);
			p1.Set(r.right - borderFloor - 1, r.bottom - borderFloor - 1);
			p2.Set(r.right, r.bottom - borderFloor - 1);
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, -1, -1);
				OffsetPoint(p2, 0, -1);
			}
			
			view->SetHighColor(rightColor);
			p1.Set(r.right - borderFloor - 1, r.bottom - borderFloor - 1);
			p2.Set(r.right - borderFloor - 1, r.bottom);
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, -1, -1);
				OffsetPoint(p2, -1, 0);
			}
		}
		
		// Concave Left Bottom
		if (c->concave_LB)
		{
			cornerSpace.Set(r.left, r.bottom - borderFloor, r.left + borderCeil, r.bottom);
			view->SetHighColor(kbd->_kbdInfo.borderColor);
			view->FillRect(cornerSpace);
			
			view->SetHighColor(bottomColor);
			p1.Set(r.left, r.bottom - borderFloor - 1);
			p2.Set(r.left + borderCeil + 1, r.bottom - borderFloor - 1);
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, 0, -1);
				OffsetPoint(p2, 1, -1);
			}
			
			view->SetHighColor(leftColor);
			p1.Set(r.left + borderCeil + 1, r.bottom - borderFloor - 1);
			p2.Set(r.left + borderCeil + 1, r.bottom);
			for (int32 j=0; j < bevelWidth; ++j)
			{
				view->StrokeLine(p1, p2);
				OffsetPoint(p1, 1, -1);
				OffsetPoint(p2, 1, 0);
			}
		}
	}
	
	BBitmap * bitmap = key->_state == Key::UP ? key->_upBitmap : key->_downBitmap;;
	
	if (bitmap)
	{
		// If the up and down bitmaps are the same, then we want to shift the bitmap by
		// the shift pixel amount.  This is so they can provide one bitmap and have it go
		// up and down just like the other keys.  But if the two bitmaps are different, then
		// we let the OEM deal with the pixel shifting themselves.  This provides the opportunity
		// to do something funky.  If this turns out to not go nicely, we can remove this check
		
		BRect drawInto;
		BRect bitmapBounds = bitmap->Bounds();
		
		drawInto.left = keyRectUnion.left + (keyRectUnion.Width() / 2.0) - (bitmapBounds.Width() / 2.0);
		drawInto.top = keyRectUnion.top + (keyRectUnion.Height() / 2.0) - (bitmapBounds.Height() / 2.0);
		drawInto.right = drawInto.left + bitmapBounds.Width();
		drawInto.bottom = drawInto.top + bitmapBounds.Height();
		
		
		// Do the up/down offset
		if (key->_upBitmap == key->_downBitmap)
			drawInto.OffsetBy(keyCapShift);
		
		view->DrawBitmapAsync(bitmap, drawInto);
	}
	else if (key->_label)
	{
		// Draw the special label
		BPoint pt;
		BRect viewBounds;
		
		// If we have a font, use it, otherwise, use the default
		view->SetFont(key->_font ? key->_font : &(kbd->_defaultFont));
		
		// pt.y is the same as for the normal one
		pt.y = keyRectUnion.bottom - ((kbd->_kbdInfo.cellHeight - bevelWidth) * 0.2);
		pt.y -= bevelWidth;
		
		viewBounds = kbd->Bounds();
		pt.x = keyRectUnion.left + keyRectUnion.Width() / 2.0;
		if (pt.x < viewBounds.left + viewBounds.Width() / 2.0)
		{
			pt.x = keyRectUnion.left + ((kbd->_kbdInfo.cellWidth - bevelWidth) * 0.4);
			pt.x += bevelWidth;
		}
		else
		{
			pt.x = keyRectUnion.right - ((kbd->_kbdInfo.cellWidth - bevelWidth) * 0.4);
			pt.x -= view->StringWidth(key->_label);
			pt.x -= bevelWidth;
		}
		
		// Do the up/down offset
		pt += keyCapShift;
		
		view->SetHighColor(kbd->_kbdInfo.textColor);
		view->DrawString(key->_label, pt);
	}
	else
	{
		// Draw the Glyph
		BPoint	pt;
		
		pt.x = keyRectUnion.left + keyRectUnion.Width() / 2.0;
		pt.y = keyRectUnion.bottom - (kbd->_kbdInfo.cellHeight * 0.2);
		
		view->SetFont(&(kbd->_defaultFont));
		
		pt.x -= stringWidth / 2.0;
		
		pt.x = floor(pt.x)+1;
		pt.y = floor(pt.y);
		
		// Do the up/down offset
		pt += keyCapShift;
		
		view->SetHighColor(kbd->_kbdInfo.textColor);
		view->DrawString(displayChars, pt);
	}
}



static void
FindSettingString(const char * name, BString & out)
{
	out = configSettings[name].String();
}


void
SoftKeyboard::UnEscapePathString(BString * out, const char * in)
{
	// Is the file name relative?
	BString unescapedStr(in);
	if (_relativePath && !(in[0] == '/' || in[0] == '$'))
	{
		unescapedStr.Prepend("/");
		unescapedStr.Prepend(_relativePath);
	}
	
	UnEscapePathStringHelper(out, unescapedStr.String());
}


// Taken mostly from BResourceSet::expand_string
status_t
SoftKeyboard::UnEscapePathStringHelper(BString * out, const char * in)
{
	
	const char* start = in;
	while( *in ) {
		if( *in == '$' ) {
			if( start < in ) out->Append(start, (int32)(in-start));
			
			in++;
			char variableName[1024];
			size_t i = 0;
			if( *in == '{' ) {
				in++;
				while( *in && *in != '}' && i<sizeof(variableName)-1 )
					variableName[i++] = *in++;
				if( *in ) in++;
			} else {
				while( isalnum(*in) || *in == '_' && i<sizeof(variableName)-1 )
					variableName[i++] = *in++;
			}
			
			start = in;
			
			variableName[i] = '\0';
			
			BString val;
			
			FindSettingString(variableName, val);
			
			if( !( val.Length() > 0 ) ) {
				return B_NAME_NOT_FOUND;
			}
			
			status_t err = UnEscapePathStringHelper(out, val.String());
			if( err != B_OK ) return err;
			
		} else if( *in == '\\' ) {
			if( start < in ) out->Append(start, (int32)(in-start));
			in++;
			start = in;
			in++;
			
		} else
			in++;
	}

	if( start < in ) out->Append(start, (int32)(in-start));
	
	
	return B_OK;
}


bool
SoftKeyboard::ParseGraphicFileName(const char * name, uint8 & keyCode, uint8 & states)
{
	
	if (0 != strncmp(name, "key_", 4))
		return false;

	int len = strlen(name);
	
	
	if (len <= 4)
		return false;
	
	const char * p = name + 4;
	
	// Find the number
	uint8 keyCodeTmp = 0;
	while (*p && *p != '_')
	{
		if (!isdigit(*p))
			return false;
		
		keyCodeTmp = keyCodeTmp * 10;
		keyCodeTmp += *p - '0';
		++p;
	}
	if (*p++ != '_')
		return false;
	
	states = 0;
	while (*p == 'U' || *p == 'D')
	{
		if (*p == 'U')
			states |= 0x01;
		if (*p == 'D')	
			states |= 0x02;
		++p;
	}
	
	if (strcmp(p, ".png") != 0)
		return false;
	
	keyCode = keyCodeTmp;
	return true;
}



/*  More or less from BootkmarkList.cpp				*/
/* This code needs to be fixed because the bitmaps want to be leaked.
    We keep it from leaking by casting away the const in the ReadConfig
    function.  I think it would be better to load it ourselves
    here, and not use the ResourceSet.
*/
const BBitmap *
SoftKeyboard::ResourceBitmap(const char * name)
{
	return _resources.FindBitmap(B_PNG_FORMAT, name);
}
void
SoftKeyboard::InitResources(const char * resourceDir)
{
	// Only allows for one resource dir.  Could be expanded
	_resources.AddDirectory(resourceDir, true);
	
}


// This is done in a separate thread, because there is some I/O
// involved in loading the pictures.  So maybe we can do this then
// Either way, we can't draw until this is done.  Draw will
// load the widths for the normal state the first time.
int32
SoftKeyboard::GetTheNormalKeyChars(void * _keyboard)
{
	SoftKeyboard * kbd = (SoftKeyboard *) _keyboard;
	
	int32		i;
	
	// Get strings
	for (i=0; i<256; ++i)
	{
		Key * key = kbd->_keys[i];
		if (key)
		{
			kbd->CalculateDisplayChar(key->_keyCode, 0, key->_normalChar);
			key->_normalCharWidth = 0;
		}
	}
	
	return B_OK;
}


int32
SoftKeyboard::GetTheRestOfTheKeyChars(void * _keyboard)
{
	SoftKeyboard * kbd = (SoftKeyboard *) _keyboard;
	
	int32		i;
	BFont		* font = &kbd->_defaultFont;
	
	// Get strings
	for (i=0; i<256; ++i)
	{
		Key * key = kbd->_keys[i];
		if (key)
		{
			kbd->CalculateDisplayChar(key->_keyCode, B_SHIFT_KEY, key->_shiftChar);
			kbd->CalculateDisplayChar(key->_keyCode, B_CAPS_LOCK, key->_capsChar);
			kbd->CalculateDisplayChar(key->_keyCode, B_CAPS_LOCK | B_SHIFT_KEY, key->_capsShiftChar);
			kbd->CalculateDisplayChar(key->_keyCode, B_OPTION_KEY, key->_optionChar);
			kbd->CalculateDisplayChar(key->_keyCode, B_OPTION_KEY | B_SHIFT_KEY, key->_optionShiftChar);
			kbd->CalculateDisplayChar(key->_keyCode, B_OPTION_KEY | B_CAPS_LOCK, key->_optionCapsChar);
			kbd->CalculateDisplayChar(key->_keyCode, B_OPTION_KEY | B_CAPS_LOCK | B_SHIFT_KEY, key->_optionCapsShiftChar);
			kbd->CalculateDisplayChar(key->_keyCode, B_CONTROL_KEY, key->_controlChar);
			
			key->_shiftCharWidth = (uint8) font->StringWidth(key->_shiftChar);
			key->_capsCharWidth = (uint8) font->StringWidth(key->_capsChar);
			key->_capsShiftCharWidth = (uint8) font->StringWidth(key->_capsShiftChar);
			key->_optionCharWidth = (uint8) font->StringWidth(key->_optionChar);
			key->_optionShiftCharWidth = (uint8) font->StringWidth(key->_optionShiftChar);
			key->_optionCapsCharWidth = (uint8) font->StringWidth(key->_optionCapsChar);
			key->_optionCapsShiftCharWidth = (uint8) font->StringWidth(key->_optionCapsShiftChar);
			key->_controlCharWidth = (uint8) 0;
		}
	}
	
	return B_OK;
}


bigtime_t
SoftKeyboard::GetRepeatDelay()
{
	BinderNode::property repeatDelay = BinderNode::Root() / "service"
		/ "softkeyboard" / "repeat_delay";
	
	return static_cast<bigtime_t> (repeatDelay.Number());
}


int32
SoftKeyboard::GetRepeatRate()
{
	BinderNode::property repeatRate = BinderNode::Root() / "service"
		/ "softkeyboard" / "repeat_rate";
	
	return static_cast<int32> (repeatRate.Number());
}
