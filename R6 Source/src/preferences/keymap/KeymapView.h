//--------------------------------------------------------------------
//	
//	KeymapView.h
//
//	Written by: Robert Polic
//	
//	Copyright 1994 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef KEYMAP_VIEW_H
#define KEYMAP_VIEW_H

#include "KeymapWindow.h"

#include <Bitmap.h>
#include <Message.h>
#include <Point.h>
#include <Rect.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

#define NOP_CHAR		-1

#define CURSORSIZE		 68	// cursor size in bytes
#define	WINDBORDER		 10	// in pixels
#define	KEYSMENU		 14	// in pixels

#define NUMKEYS			101 // number of keys

#define	KEYMAPWIDTH	452	// in pixels
#define KEYMAPHEIGHT	180	// in pixels

#define KEY1WIDTH		 16
#define KEY1HEIGHT		 16
#define KEY2WIDTH		 25
#define KEY2HEIGHT		 16
#define KEY3WIDTH		 34
#define KEY3HEIGHT		 16
#define KEY4WIDTH		 43
#define KEY4HEIGHT		 16
#define KEY5WIDTH		117
#define KEY5HEIGHT		 16
#define KEY6WIDTH		 16
#define KEY6HEIGHT		 34

#define EDITLEFT		 24
#define EDITTOP			 26
#define EDITRIGHT		451
#define EDITBOTTOM		 40

#define KEY1	 1
#define KEY2	 3
#define KEY3	 5
#define KEY4	 7
#define KEY5	 9
#define KEY6	11

#define DOWN	 1
#define UP		 0

#define FREE	 0
#define LOCKED	 1

#define ON		TRUE
#define OFF		FALSE

const int32 KEY_MSG = 'CHKY';
struct KeyRecord
	{
	short	keyType;
	long	keyLeft;
	long	keyTop;
	short	keyState;
	long	keyFlags;
	};

struct KeySizes
	{
	long	keyWidth;
	long	keyHeight;
	};

struct LEDRecord
	{
	long	ledLeft;
	long	ledTop;
	long	ledRight;
	long	ledBottom;
	bool	ledState;
	};

//====================================================================
class TKeymapView;

class TEditTextView : public BTextView 
{
public:
		TEditTextView(TKeymapView* parent, BRect frame, const char *name, BRect textRect,
			  long resizeMode, long flags);
virtual	void	AttachedToWindow();
virtual void    FakeKeyDown(const char* key, int32 size);
virtual void    KeyDown(const char* key, int32 size);
protected:
                TKeymapView* mParent;
};
	
//====================================================================

struct ext_keymap;

class TKeymapView : public BView 
{
public:
  sem_id    fSem;
  thread_id fTid;
  uchar		fKeyStates[16];
  long		fUndoKey;
  char*		fUndoDeep[9];
  char*		fUndoString;
  long		fUndoTable;
  ulong		fMods;
  long		fDeadKey;
  bool		fDirty;
  bool		fUndoFlag;
  bool		fUndoDeepFlag;
  bool          fReadOnly;
  BBitmap	*fKeyBits[14];
  TEditTextView	*fTextView;
  ext_keymap	*fExtKeyMap;
  int32         fCurKey;   // Current key when dragging & dropping.
  int32         fCurTable; // Current table when D&D.
			
		TKeymapView(BRect frame, char *name);
	        ~TKeymapView();

// Overrides from BView
     virtual void    MouseMoved(BPoint where,uint32 code,const BMessage *a_message);
virtual	void	AttachedToWindow();
virtual	void	DetachedFromWindow();
virtual	void	Draw(BRect updateRect);
virtual void	MessageReceived(BMessage *theParcel);
virtual void	ModifyKey(BMessage* theParcel);
virtual void	MouseDown(BPoint thePoint);
virtual void	Pulse();

// Our new methods
void			DeadKey(long theKey);
char *          GetCurKeyString(int32 theKey);
char *          GetKeyString(int32 theKey, int32 table, bool *isDead);
virtual void    KeyDown(const char* key, int32 size);
int32           TransformMods(int32 keyCode, bool state,
							  int32 iMods, bool *isModifier);
int32           IsDeadKey(int32 keyCode);
void            PrintKey(int32 keyCode);
void            KeyState(int32 keyCode, bool state);
void			DrawKey(int32 theKey, long table);
void			DrawLEDs();
bool			IsKeyPad(long theKey);
void			Key();
void			SetExtKeyMap(ext_keymap *theMap);
void			FindTable(long *theTable, long *theNumTable);
void			ReDraw();
void			Undo();
};

#endif





