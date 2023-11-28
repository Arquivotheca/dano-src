//#define DEBUG 1
//--------------------------------------------------------------------
//	
//	KeymapView.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1994-95 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include "KeymapView.h"

#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Debug.h>
#include <OS.h>
#include <Resources.h>
#include <Roster.h>

unsigned char hand_cursor[] = {16, 1, 0, 6,
							   0x03, 0x00, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80,
							   0x04, 0x80, 0x04, 0x80, 0x64, 0xf0, 0x94, 0xac,
							   0x4c, 0xaa, 0x24, 0x0a, 0x24, 0x02, 0x10, 0x02,
							   0x08, 0x02, 0x08, 0x04, 0x04, 0x04, 0x04, 0x04,
							   0x03, 0x00, 0x07, 0x80, 0x07, 0x80, 0x07, 0x80,
							   0x07, 0x80, 0x07, 0x80, 0x67, 0xf0, 0xf7, 0xfc,
							   0x7f, 0xfe, 0x3f, 0xfe, 0x3f, 0xfe, 0x1f, 0xfe,
							   0x0f, 0xfe, 0x0f, 0xfc, 0x07, 0xfc, 0x07, 0xfc};

KeyRecord theKeys[NUMKEYS+1] =  {{KEY1, 0, 0, UP, LOCKED},		//   0 <null>
								 {KEY1,  15,  54, UP, FREE},	//   1 <Esc>
								 {KEY1,  51,  54, UP, LOCKED},	//   2 <F1>
								 {KEY1,  69,  54, UP, LOCKED},	//   3 <F2>
								 {KEY1,  87,  54, UP, LOCKED},	//   4 <F3>
								 {KEY1, 105,  54, UP, LOCKED},	//   5 <F4>
								 {KEY1, 132,  54, UP, LOCKED},	//   6 <F5>
								 {KEY1, 150,  54, UP, LOCKED},	//   7 <F6>
								 {KEY1, 168,  54, UP, LOCKED},	//   8 <F7>
								 {KEY1, 186,  54, UP, LOCKED},	//   9 <F8>
								 {KEY1, 213,  54, UP, LOCKED},	//  10 <F9>
								 {KEY1, 231,  54, UP, LOCKED},	//  11 <F10>
								 {KEY1, 249,  54, UP, LOCKED},	//  12 <F11>
								 {KEY1, 267,  54, UP, LOCKED},	//  13 <F12>
								 {KEY1, 302,  54, UP, LOCKED},	//  14 <Print>
								 {KEY1, 320,  54, UP, LOCKED},	//  15 <Scroll>
								 {KEY1, 338,  54, UP, LOCKED},	//  16 <Pause>
								 {KEY1,  15,  82, UP, FREE},	//  17 <`>
								 {KEY1,  33,  82, UP, FREE},	//  18 <1>
								 {KEY1,  51,  82, UP, FREE},	//  19 <2>
								 {KEY1,  69,  82, UP, FREE},	//  20 <3>
								 {KEY1,  87,  82, UP, FREE},	//  21 <4>
								 {KEY1, 105,  82, UP, FREE},	//  22 <5>
								 {KEY1, 123,  82, UP, FREE},	//  23 <6>
								 {KEY1, 141,  82, UP, FREE},	//  24 <7>
								 {KEY1, 159,  82, UP, FREE},	//  25 <8>
								 {KEY1, 177,  82, UP, FREE},	//  26 <9>
								 {KEY1, 195,  82, UP, FREE},	//  27 <0>
								 {KEY1, 213,  82, UP, FREE},	//  28 <->
								 {KEY1, 231,  82, UP, FREE},	//  29 <=>
								 {KEY3, 249,  82, UP, FREE},	//  30 <back>
								 {KEY1, 302,  82, UP, FREE},	//  31 <Insert>
								 {KEY1, 320,  82, UP, FREE},	//  32 <Home>
								 {KEY1, 338,  82, UP, FREE},	//  33 <PageUp>
								 {KEY1, 373,  82, UP, LOCKED},	//  34 <NumLock>
								 {KEY1, 391,  82, UP, FREE},	//  35 </>
								 {KEY1, 409,  82, UP, FREE},	//  36 <*>
								 {KEY1, 427,  82, UP, FREE},	//  37 <->
								 {KEY2,  15, 100, UP, FREE},	//  38 <Tab>
								 {KEY1,  42, 100, UP, FREE},	//  39 <q>
								 {KEY1,  60, 100, UP, FREE},	//  40 <w>
								 {KEY1,  78, 100, UP, FREE},	//  41 <e>
								 {KEY1,  96, 100, UP, FREE},	//  42 <r>
								 {KEY1, 114, 100, UP, FREE},	//  43 <t>
								 {KEY1, 132, 100, UP, FREE},	//  44 <y>
								 {KEY1, 150, 100, UP, FREE},	//  45 <u>
								 {KEY1, 168, 100, UP, FREE},	//  46 <i>
								 {KEY1, 186, 100, UP, FREE},	//  47 <o>
								 {KEY1, 204, 100, UP, FREE},	//  48 <p>
								 {KEY1, 222, 100, UP, FREE},	//  49 <[>
								 {KEY1, 240, 100, UP, FREE},	//  50 <]>
								 {KEY2, 258, 100, UP, FREE},	//  51 <\>
								 {KEY1, 302, 100, UP, FREE},	//  52 <Delete>
								 {KEY1, 320, 100, UP, FREE},	//  53 <End>
								 {KEY1, 338, 100, UP, FREE},	//  54 <PageDn>
								 {KEY1, 373, 100, UP, FREE},	//  55 <7>
								 {KEY1, 391, 100, UP, FREE},	//  56 <8>
								 {KEY1, 409, 100, UP, FREE},	//  57 <9>
								 {KEY6, 427, 100, UP, FREE},	//  58 <+>
								 {KEY3,  15, 118, UP, LOCKED},	//  59 <Caps>
								 {KEY1,  51, 118, UP, FREE},	//  60 <a>
								 {KEY1,  69, 118, UP, FREE},	//  61 <s>
								 {KEY1,  87, 118, UP, FREE},	//  62 <d>
								 {KEY1, 105, 118, UP, FREE},	//  63 <f>
								 {KEY1, 123, 118, UP, FREE},	//  64 <g>
								 {KEY1, 141, 118, UP, FREE},	//  65 <h>
								 {KEY1, 159, 118, UP, FREE},	//  66 <j>
								 {KEY1, 177, 118, UP, FREE},	//  67 <k>
								 {KEY1, 195, 118, UP, FREE},	//  68 <l>
								 {KEY1, 213, 118, UP, FREE},	//  69 <;>
								 {KEY1, 231, 118, UP, FREE},	//  70 <'>
								 {KEY3, 249, 118, UP, FREE},	//  71 <Enter>
								 {KEY1, 373, 118, UP, FREE},	//  72 <4>
								 {KEY1, 391, 118, UP, FREE},	//  73 <5>
								 {KEY1, 409, 118, UP, FREE},	//  74 <6>
								 {KEY4,  15, 136, UP, LOCKED},	//  75 <LShift>
								 {KEY1,  60, 136, UP, FREE},	//  76 <z>
								 {KEY1,  78, 136, UP, FREE},	//  77 <x>
								 {KEY1,  96, 136, UP, FREE},	//  78 <c>
								 {KEY1, 114, 136, UP, FREE},	//  79 <v>
								 {KEY1, 132, 136, UP, FREE},	//  80 <b>
								 {KEY1, 150, 136, UP, FREE},	//  81 <n>
								 {KEY1, 168, 136, UP, FREE},	//  82 <m>
								 {KEY1, 186, 136, UP, FREE},	//  83 <,>
								 {KEY1, 204, 136, UP, FREE},	//  84 <.>
								 {KEY1, 222, 136, UP, FREE},	//  85 </>
								 {KEY4, 240, 136, UP, LOCKED},	//  86 <RShift>
								 {KEY1, 320, 136, UP, FREE},	//  87 <UpArrow>
								 {KEY1, 373, 136, UP, FREE},	//  88 <1>
								 {KEY1, 391, 136, UP, FREE},	//  89 <2>
								 {KEY1, 409, 136, UP, FREE},	//  90 <3>
								 {KEY6, 427, 136, UP, FREE},	//  91 <Enter>
								 {KEY2,  15, 154, UP, LOCKED},	//  92 <LCtrl>
								 {KEY2,  68, 154, UP, LOCKED},	//  93 <LCmnd>
								 {KEY5,  95, 154, UP, FREE},	//  94 <space>
								 {KEY2, 214, 154, UP, LOCKED},	//  95 <RCmnd>
								 {KEY2, 258, 154, UP, LOCKED},	//  96 <RCtrl>
								 {KEY1, 302, 154, UP, FREE},	//  97 <LtArrow>
								 {KEY1, 320, 154, UP, FREE},	//  98 <DnArrow>
								 {KEY1, 338, 154, UP, FREE},	//  99 <RtArrow>
								 {KEY3, 373, 154, UP, FREE},	// 100 <0>
								 {KEY1, 409, 154, UP, FREE}		// 101 <.>
							};

KeySizes theSizes[14] =	{ {KEYMAPWIDTH, KEYMAPHEIGHT},
						  {KEY1WIDTH, KEY1HEIGHT},
						  {KEY1WIDTH, KEY1HEIGHT},
						  {KEY2WIDTH, KEY2HEIGHT},
						  {KEY2WIDTH, KEY2HEIGHT},
						  {KEY3WIDTH, KEY3HEIGHT},
						  {KEY3WIDTH, KEY3HEIGHT},
						  {KEY4WIDTH, KEY4HEIGHT},
						  {KEY4WIDTH, KEY4HEIGHT},
						  {KEY5WIDTH, KEY5HEIGHT},
						  {KEY5WIDTH, KEY5HEIGHT},
						  {KEY6WIDTH, KEY6HEIGHT},
						  {KEY6WIDTH, KEY6HEIGHT},
						  {KEY1WIDTH, KEY1HEIGHT}
						};

LEDRecord theLEDs[3] =  { {376, 57, 388, 58, OFF},	//  numLock
						  {402, 57, 414, 58, OFF},	//  capsLock
						  {428, 57, 440, 58, OFF}	//  scrollLock
						};

extern	TKeymapWindow	*keymapWind;

uchar acute[]      = {2,0xC2,0xB4};
uchar grave[]      = {1,'`'};
uchar circumflex[] = {1,'^'};
uchar diereisis[]  = {1,'"'};
uchar tilde[]      = {1,'~'};

uchar *deads[] =
{
  acute,
  grave,
  circumflex,
  diereisis,
  tilde
};

//====================================================================

TKeymapView	*keymapView;

TKeymapView::TKeymapView(BRect rect, char *title)
	: BView(rect, title, B_NOT_RESIZABLE,
		B_WILL_DRAW | B_NAVIGABLE | B_PULSE_NEEDED)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fCurKey = -1;
	fDeadKey = 0;
	fMods = 0;
	for (long loop = 0; loop <= 13; loop++)
		fKeyBits[loop] = 0;
		
	keymapView = this;
	for (int i = 0; i < 8; i++)
		fUndoDeep[i] = NULL;
	
	fUndoString = NULL;
}

static int32 pulse_me(TKeymapView *v)
{
  while (acquire_sem_etc(v->fSem,1,B_TIMEOUT,0) == B_OK) {
	if (!v->Window())
	  return 0;
	v->Window()->Lock();
	v->Pulse();
	v->Window()->Unlock();
	release_sem(v->fSem);
	snooze(20000);
  }

  return 0;
}

//--------------------------------------------------------------------

void TKeymapView::DetachedFromWindow()
{
//  long l=0;
//  if (Window()) {
//	while (Window()->IsLocked()) {
//	  Window()->Unlock();
//	  l++;
//	}
//  }
//  
//  acquire_sem(fSem);
//  int32 dummy;
//  wait_for_thread(fTid,&dummy);
//  
//  while (l--)
//	Window()->Lock();
}

TKeymapView::~TKeymapView()
{
	long	loop;

	for (loop = 0; loop <= 13; loop++) {
	  if (fKeyBits[loop])
		delete fKeyBits[loop];
	}
}

//--------------------------------------------------------------------

void TKeymapView::AttachedToWindow()
{
	fDirty = false;
	fUndoFlag = false;
	fUndoDeepFlag = false;
	fDeadKey = 0;
	fReadOnly = false;
	
	BRect tempRect(EDITLEFT, EDITTOP, EDITRIGHT, EDITBOTTOM);
	BRect textRect(tempRect);
	textRect.OffsetTo(1,1);

	fTextView =  new TEditTextView(this, tempRect, "tview", textRect, B_FOLLOW_NONE, B_WILL_DRAW);						
	AddChild(fTextView);
	
	BFont newFont;
	newFont.SetFamilyAndStyle(keymapWind->fFontName, keymapWind->fFontStyle);
	SetFont(&newFont);
	SetFontSize(12);
	SetDrawingMode(B_OP_OVER);
	
	app_info info;
	if (be_app->GetAppInfo(&info) == B_NO_ERROR)  {
		BFile 		file;
		status_t 	status = file.SetTo(&info.ref, O_RDONLY);
		if (status == B_NO_ERROR) {
			uint32		len;
			BResources 	resources(&file);
			void*		picH;
			for (int32 loop = 0; loop <= 13; loop++) {
				if ((picH = resources.FindResource('PICT', loop+5, &len)) != 0) {
					tempRect.Set(0, 0, theSizes[loop].keyWidth, theSizes[loop].keyHeight);
					fKeyBits[loop] = new BBitmap(tempRect, B_COLOR_8_BIT);
					if (fKeyBits[loop])
						fKeyBits[loop]->SetBits(picH, len, 0, B_COLOR_8_BIT);
					free(picH);
				} else {
					char	message[256];
					sprintf(message, "A resource can not be loaded: PICT - %li", (int32)loop);
					BAlert	*myAlert = new BAlert("", message, "Sorry");
					myAlert->Go();
					be_app->PostMessage(B_QUIT_REQUESTED);
					return;
				}
			}
		}
	}

//	fSem = create_sem(1,"thSem");
//	fTid = spawn_thread((thread_entry)pulse_me,"pulseThread",
//						B_NORMAL_PRIORITY,this);
//	resume_thread(fTid);

	MakeFocus(true);
}

//--------------------------------------------------------------------

void TKeymapView::Draw(BRect where)
{
	PushState();

	BRect tempRect(WINDBORDER, WINDBORDER, WINDBORDER + KEYMAPWIDTH, WINDBORDER + KEYMAPHEIGHT);

	if (tempRect.Intersects(where)) {
		tempRect = tempRect & where;
		
		BRect r;
		r.left = tempRect.left - WINDBORDER;
		r.top = tempRect.top - WINDBORDER;
		r.right = tempRect.right - WINDBORDER;
		r.bottom = tempRect.bottom - WINDBORDER;
		DrawBitmapAsync(fKeyBits[0], r, tempRect);
	}

	tempRect.Set(EDITLEFT, EDITTOP, EDITRIGHT, EDITBOTTOM);
	if (tempRect.Intersects(where)) {
		tempRect.Set(0, 0, EDITRIGHT - EDITLEFT, EDITBOTTOM - EDITTOP);
		tempRect.Set(where.left - EDITLEFT,
					 where.top - EDITTOP,
					 where.right - EDITLEFT,
					 where.bottom - EDITTOP);
		//WAA - Fix this
		//fTextView->FillRect(tempRect, B_SOLID_LOW);
		//fTextView->Draw(tempRect);
	}

	// Set the font for drawing to be the one selected
	// from the menu.
	BFont font;
	font.SetFamilyAndStyle(keymapWind->fFontName, keymapWind->fFontStyle);
	SetFont(&font);

	//fMods = modifiers() & (B_CONTROL_KEY | B_COMMAND_KEY | B_SHIFT_KEY | B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK | B_OPTION_KEY);
	theLEDs[0].ledState = OFF;
	theLEDs[1].ledState = OFF;
	theLEDs[2].ledState = OFF;
	DrawLEDs();

	int32 theTable, theNumTable;
	FindTable(&theTable, &theNumTable);

	for (int32 loop = 1; loop <= NUMKEYS; loop++) {
		tempRect.left = theKeys[loop].keyLeft;
	  	tempRect.top = theKeys[loop].keyTop;
		tempRect.right = theKeys[loop].keyLeft + theSizes[theKeys[loop].keyType].keyWidth;
	  	tempRect.bottom = theKeys[loop].keyTop + theSizes[theKeys[loop].keyType].keyHeight;

		if (tempRect.Intersects(where)) {
			if (IsKeyPad(loop))
		  		DrawKey(loop, theNumTable);
			else
		  		DrawKey(loop, theTable);
	  	}
	}
	PopState();
}

//--------------------------------------------------------------------

void TKeymapView::MessageReceived(BMessage *theParcel)
{
	switch(theParcel->what) {
	case B_UNMAPPED_KEY_DOWN:
		KeyDown(NULL, 0);	// KeyDown doesn't look at it's arguments, so
							// don't bother to send it the correct info
		break;
	default:
		if (theParcel->WasDropped())
			ModifyKey(theParcel);
		else
			BView::MessageReceived(theParcel);
	}
}


//--------------------------------------------------------------------

void TKeymapView::ModifyKey(BMessage* theParcel)
{
	int32	theTable;
	int32	theNumTable;
	int32	count = 0;	
	char*	data;
		
	if (!fReadOnly) {
		FindTable(&theTable, &theNumTable);      
	
		if (fCurKey != -1) {
			//if (theKeys[loop].keyFlags != LOCKED) 
			if (theKeys[fCurKey].keyFlags != LOCKED) {
				if (theParcel->HasData("control", B_RAW_TYPE, count)) {
					uint8 length;
					ssize_t size;

					//printf("	deep\n");
					// Free the old undo values.
					for (int i = 0; i < 8; i++) {
						if (fUndoDeep[i] != NULL) {
							free(fUndoDeep[i]);
							fUndoDeep[i] = NULL;
						}
					}

					length = *(uint8*)fExtKeyMap->control_map[fCurKey];
					if (length > 0) {
						fUndoDeep[0] = (char*) malloc(length + 1);
						memcpy(fUndoDeep[0], fExtKeyMap->control_map[fCurKey], length+1);
						
						if (fExtKeyMap->control_map[fCurKey] != NULL)
							free (fExtKeyMap->control_map[fCurKey]);

						if (theParcel->FindData( (const char*)"control",
												 (type_code)B_RAW_TYPE,
												 (int32)count,(const void**) &data,
												 (ssize_t*) &size) == B_NO_ERROR) {
							fExtKeyMap->control_map[fCurKey] = (char*)malloc(size);
							memcpy(fExtKeyMap->control_map[fCurKey], data, size);
						}
					} else {
						fUndoDeep[0] = (char*) malloc(2);
						fUndoDeep[0][0] = 0;
					}
					
					length = *(uint8*)fExtKeyMap->option_caps_shift_map[fCurKey];					
					if (length > 0){
						fUndoDeep[1] = (char*) malloc(length + 1);
						memcpy(fUndoDeep[1], fExtKeyMap->option_caps_shift_map[fCurKey], length+1);
						if (fExtKeyMap->option_caps_shift_map[fCurKey] != NULL)
							free (fExtKeyMap->option_caps_shift_map[fCurKey]);
						theParcel->FindData("option_caps_shift", B_RAW_TYPE, count, (const void**)&data, &size);
						fExtKeyMap->option_caps_map[fCurKey] = (char*)malloc(size);
						memcpy(fExtKeyMap->option_caps_map[fCurKey], data, size);
					} else {
						fUndoDeep[1] = (char*) malloc(2);
						fUndoDeep[1][0] = 0;
					}

					length = *(uint8*)fExtKeyMap->option_caps_map[fCurKey];
					if (length > 0) {
						fUndoDeep[2] = (char*) malloc(length + 1);
						memcpy(fUndoDeep[2], fExtKeyMap->option_caps_map[fCurKey], length+1);
						if (fExtKeyMap->option_caps_map[fCurKey] != NULL)
							free (fExtKeyMap->option_caps_map[fCurKey]);
						theParcel->FindData("option_caps", B_RAW_TYPE, count, (const void**)&data, &size);
						fExtKeyMap->option_caps_map[fCurKey] = (char*)malloc(size);
						memcpy(fExtKeyMap->option_caps_map[fCurKey], data, size);
					} else {
						fUndoDeep[2] = (char*) malloc(2);
						fUndoDeep[2][0] = 0;
					}

					length = *(uint8*)fExtKeyMap->option_shift_map[fCurKey];
					if (length > 0) {
						fUndoDeep[3] = (char*) malloc(length + 1);
						memcpy(fUndoDeep[3], fExtKeyMap->option_shift_map[fCurKey], length+1);
						if (fExtKeyMap->option_shift_map[fCurKey] != NULL)
							free (fExtKeyMap->option_shift_map[fCurKey]);
						theParcel->FindData("option_shift", B_RAW_TYPE, count, (const void**)&data, &size);
						fExtKeyMap->option_shift_map[fCurKey] = (char*)malloc(size);
						memcpy(fExtKeyMap->option_shift_map[fCurKey], data, size);
					} else {
						fUndoDeep[3] = (char*) malloc(2);
						fUndoDeep[3][0] = 0;
					}

					length = *(uint8*)fExtKeyMap->option_map[fCurKey];
					if ( length > 0) {
						fUndoDeep[4] = (char*) malloc(length + 1);
						memcpy(fUndoDeep[4], fExtKeyMap->option_map[fCurKey], length+1);
						if (fExtKeyMap->option_map[fCurKey] != NULL)
							free (fExtKeyMap->option_map[fCurKey]);
						theParcel->FindData("option", B_RAW_TYPE, count, (const void**)&data, &size);
						fExtKeyMap->option_map[fCurKey] = (char*)malloc(size);
						memcpy(fExtKeyMap->option_map[fCurKey], data, size);
					} else {
						fUndoDeep[4] = (char*) malloc(2);
						fUndoDeep[4][0] = 0;
					}

					length = *(uint8*)fExtKeyMap->caps_shift_map[fCurKey];						
					if (length > 0) {
						fUndoDeep[5] = (char*) malloc(length + 1);
						memcpy(fUndoDeep[5], fExtKeyMap->caps_shift_map[fCurKey], length+1);
						if (fExtKeyMap->caps_shift_map[fCurKey] != NULL)
							free (fExtKeyMap->caps_shift_map[fCurKey]);
						theParcel->FindData("caps_shift", B_RAW_TYPE, count, (const void**)&data, &size);
						fExtKeyMap->caps_shift_map[fCurKey] = (char*)malloc(size);
						memcpy(fExtKeyMap->caps_shift_map[fCurKey], data, size);
					} else {
						fUndoDeep[5] = (char*) malloc(2);
						fUndoDeep[5][0] = 0;
					}
					
					length = *(uint8*)fExtKeyMap->caps_map[fCurKey];					
					if (length > 0) {
						fUndoDeep[6] = (char*) malloc(length + 1);
						memcpy(fUndoDeep[6], fExtKeyMap->caps_map[fCurKey], length+1);
						if (fExtKeyMap->caps_map[fCurKey] != NULL)
							free (fExtKeyMap->caps_map[fCurKey]);
						theParcel->FindData("caps", B_RAW_TYPE, count, (const void**)&data, &size);
						fExtKeyMap->caps_map[fCurKey] = (char*)malloc(size);
						memcpy(fExtKeyMap->caps_map[fCurKey], data, size);
					} else {
						fUndoDeep[6] = (char*) malloc(2);
						fUndoDeep[6][0] = 0;
					}
					
					length = fExtKeyMap->shift_map[fCurKey][0];
					if (length > 0) {
						//printf("Shift: %s, len %d\n", fExtKeyMap->shift_map[fCurKey],
						//       length);
						fUndoDeep[7] = (char*) malloc(length + 1);
						memcpy(fUndoDeep[7], fExtKeyMap->shift_map[fCurKey], length+1);
						//printf("Shift Undo is%s\n", fUndoDeep[7]);
						if (fExtKeyMap->shift_map[fCurKey] != NULL)
							free (fExtKeyMap->shift_map[fCurKey]);
						theParcel->FindData("shift", B_RAW_TYPE, count, (const void**)&data, &size);
						fExtKeyMap->shift_map[fCurKey] = (char*)malloc(size);
						memcpy(fExtKeyMap->shift_map[fCurKey], data, size);
					} else {
						fUndoDeep[7] = (char*) malloc(2);
						fUndoDeep[7][0] = 0;
					}
					
					length = *(uint8*)fExtKeyMap->normal_map[fCurKey];					
					if (length > 0) {
						fUndoDeep[8] = (char*) malloc(length + 1);
						memcpy(fUndoDeep[8], fExtKeyMap->normal_map[fCurKey], length+1);
						if (fExtKeyMap->normal_map[fCurKey] != NULL)
							free (fExtKeyMap->normal_map[fCurKey]);
						theParcel->FindData("normal", B_RAW_TYPE, count,(const void**) &data, &size);
						#if DEBUG
						{
							char gorp[1000];
							memcpy (gorp, data+1, size -1);
							gorp[size -1] = 0;
							PRINT(("String is %s, size is %ld, strlen is %d\n", gorp, size, data[0]));
						}
						#endif

						fExtKeyMap->normal_map[fCurKey] = (char*)malloc(size);
						memcpy(fExtKeyMap->normal_map[fCurKey], data, size);
					} else {
						fUndoDeep[8] = (char*) malloc(2);
						fUndoDeep[8][0] = 0;
					}
					
					fUndoKey = fCurKey;
					fCurKey = -1;
					DrawKey(fUndoKey, theTable);
					fUndoDeepFlag = TRUE;
				} else  {
					uint8		length;
					ssize_t		size;
					void*		data;
					long		tbl;
					
					if ((theParcel->FindData("text/plain", B_MIME_TYPE, count,
											 (const void**) &data, &size) == B_NO_ERROR)) {
						PRINT(("	shallow\n"));

						if (IsKeyPad(fCurKey)) {
							fUndoTable = theNumTable + fCurKey;
							tbl = theNumTable;
						} else {
							fUndoTable = theTable + fCurKey;
							tbl = theTable;
						}
						
						length = *(uint8*)fExtKeyMap->control_map[fUndoTable];
						if (fUndoString != NULL) {
							free(fUndoString);
						}
							
						fUndoString = (char*)malloc(length + 1);
						memcpy(fUndoString, fExtKeyMap->control_map[fUndoTable], length+1);
						if (fExtKeyMap->control_map[fUndoTable] != NULL ) {
							free(fExtKeyMap->control_map[fUndoTable]);
						}
							
						fExtKeyMap->control_map[fUndoTable] = (char*)malloc(size+1);
						memcpy(&(fExtKeyMap->control_map[fUndoTable][1]), data, size);
						fExtKeyMap->control_map[fUndoTable][0] = size;
						fUndoDeepFlag = false;
						PRINT(("Updating key\n"));
						fUndoKey = fCurKey;
						fCurKey = -1;
						DrawKey(fUndoKey, tbl);
					}
				}
				
				fUndoFlag = TRUE;
				fDirty = TRUE;
				keymapWind->SetMenuEnable(MENU_UNDO, TRUE);
				keymapWind->SetMenuEnable(MENU_SAVE, TRUE);
			} //else printf("Key is locked.\n");
		}
	
		for (int i = 0; i < 8; i++) {
			if (fUndoDeep[i] != NULL) {
				char cStr[2048];
				memcpy(cStr, &(fUndoDeep[i][1]), fUndoDeep[i][0]);
				cStr[fUndoDeep[i][0]] = 0;
				//printf("%i: %s\n", i, cStr);
			} //else printf("NULL");
		}
	} else {
		BAlert* alert = new BAlert( "Map is read only",
									"The system keymap files are read-only. To edit, save a copy in the user area and edit the copy.\n", "OK");
		alert->Go();
	}
	
	return;
}

void			
TKeymapView::MouseMoved(BPoint where,uint32 code,const BMessage *a_message)
{
  int32 loop;
  BRect tempRect;
  int32 lastKey = fCurKey;
  fCurKey = -1;
  if (a_message != NULL && code != 2)
    {
      for (loop = 1; loop <= NUMKEYS; loop++) 
	{
	  tempRect.left = theKeys[loop].keyLeft;
	  tempRect.top = theKeys[loop].keyTop;
	  tempRect.right = theKeys[loop].keyLeft +
	    theSizes[theKeys[loop].keyType].keyWidth;
	  tempRect.bottom = theKeys[loop].keyTop +
	    theSizes[theKeys[loop].keyType].keyHeight;
	  //printf("intersect  amount is %d\n", intersectPct);
	  if (tempRect.Contains(where))
	    {
	      // There's a key under the meese.
	      if (theKeys[loop].keyFlags != LOCKED) 
		fCurKey = loop;
	      break;
	    }
	}
    }

  if (fCurKey != lastKey )
    {
      
      int32 theTable, theNumTable;
      FindTable(&theTable, &theNumTable);
      if (fCurKey != -1 )
	{
	  if (IsKeyPad(fCurKey))
	    DrawKey(fCurKey,theNumTable );	
	  else  
	    DrawKey(fCurKey,theTable );	
	}
      if (lastKey != -1 )
	{
	  if (IsKeyPad(lastKey))
	    DrawKey(lastKey,theNumTable );	
	  else  
	    DrawKey(lastKey,theTable );	
	}
    }

  //printf("curKey = %ld\n", fCurKey);
  BView::MouseMoved(where, code, a_message);
}

int32 mod_array[] =
{
  B_CAPS_LOCK,
  B_SCROLL_LOCK,
  B_NUM_LOCK,
  B_SHIFT_KEY,
  B_SHIFT_KEY,
  B_COMMAND_KEY,
  B_COMMAND_KEY,
  B_CONTROL_KEY,
  B_CONTROL_KEY,
  B_OPTION_KEY,
  B_OPTION_KEY
};
  
int32 TKeymapView::TransformMods(int32 keyCode, bool state,
								 int32 iMods, bool *isModifier)
{
  int32 t=0,i;
  uint32 *cmp_array = &fExtKeyMap->caps_key;
  *isModifier = false;

  //  printf("TransformMods(%d,%d,0x%08x)\n",keyCode,state,iMods);
  
  for (i=0;i<11;i++) {
	//	//printf("Checking against %d...\n",cmp_array[i]);
	if (cmp_array[i] == (uint32)keyCode) {
	  t = mod_array[i];
	  *isModifier = true;
	  break;
	}
  }

#if 0
  if (i < 3) {
	/* These are the locking states.  Releasing the key does nothing. */
	if (!state)
	  return iMods;
	/* Pressing it toggles the state */
	if (iMods & t)
	  iMods = iMods & (~t);
	else
	  iMods = iMods | t;
	return iMods;
  }
#endif
  return state?(iMods|t):(iMods&(~t));
}

int32 TKeymapView::IsDeadKey(int32 keyCode)
{
  int32 theTable, theNumTable, table;
  FindTable(&theTable, &theNumTable);
  if (IsKeyPad(keyCode))
	table = theNumTable;
  else
	table = theTable;

  int32 deadTables, usedTable;
  char *keyString, *deadString;
  for (int loop = 0; loop < 5; loop++) {
	deadTables = *(&fExtKeyMap->acute_tables + loop);
	usedTable = 1 << (table / 128);
	keyString = GetCurKeyString(keyCode);
	deadString = fExtKeyMap->acute_dead_key[loop * 32 + 1];
	if (keyString) {
	  if ((deadTables & usedTable) &&
		  (keyString[0] == deadString[0]) &&
		  (strncmp(keyString+1,deadString+1,keyString[0]) == 0)) {
		return loop+1;
	  }
	}
  }

  return 0;
}

void TKeymapView::PrintKey(int32 keyCode)
{
  if (!Window()->IsActive())
	return;
  char *keyString = GetCurKeyString(keyCode);
  if ((keyString != NULL) && (keyString[0] != 0))
	fTextView->FakeKeyDown(&(keyString[1]), keyString[0]);
}

void TKeymapView::KeyDown(const char* , int32 )
{
  BMessage *m = Window()->CurrentMessage();
  int32 keyCode = m->FindInt32("key");
  if (theKeys[keyCode].keyState)
	PrintKey(keyCode);
  else
	KeyState(keyCode,true);
}

void TKeymapView::KeyState(int32 keyCode, bool state)
{
  key_info theKeyStates;
  uint32 mods;
  bool isModifier = false;

  if (keyCode && (state == theKeys[keyCode].keyState))
	return;

  MakeFocus(true);
  
  if (!keyCode) {
	get_key_info(&theKeyStates);
	mods = theKeyStates.modifiers &
	  (B_CONTROL_KEY|B_COMMAND_KEY|B_SHIFT_KEY|B_OPTION_KEY|
	   B_CAPS_LOCK|B_NUM_LOCK|B_SCROLL_LOCK);
  } else {
	mods = TransformMods(keyCode, state, fMods, &isModifier);
  }
  if ((mods != fMods) && !keyCode) {
	fMods = mods;
	DrawLEDs();
	//DrawMods();
	ReDraw();
  }

  if (!keyCode)
	return;
  
  theKeys[keyCode].keyState = state;
  if (state)
	fKeyStates[keyCode>>3] |= 1 << (7-(keyCode&0x07));
  else
	fKeyStates[keyCode>>3] &= ~(1 << (7-(keyCode&0x07)));
  
  int32 theTable, theNumTable, table;
  FindTable(&theTable, &theNumTable);
  table = theTable;
  if (IsKeyPad(keyCode))
	table = theNumTable;
  
  DrawKey(keyCode, table);

  if ((state == true) && !isModifier) {
	if (fDeadKey) {
	  PrintKey(keyCode);
	  fDeadKey = 0;
	  ReDraw();
	} else if ((fDeadKey = IsDeadKey(keyCode)) != 0) 
	  ReDraw();
	else 
	  PrintKey(keyCode);
  }
}

//--------------------------------------------------------------------

void TKeymapView::MouseDown(BPoint )
{
  ulong			buttons=0;
  ulong			theLock=0;
  ulong			lastKey = 0;
  char*			lastString;
  ulong			loop;
  long			theTable;
  long			theNumTable;
  long			theIndex;
  bool			gotOne;
  bool			doDrag = false;
  bool			doDeepDrag = false;
  BBitmap*		dragImage;
  BMessage*		msg;
  BPoint		where;
  BRect			tempRect;
  BView			*tmp_view;
  
  PRINT(("TKeymapView::MouseDown\n"));
  
  GetMouse(&where, &buttons);
  if (buttons & 4)
    doDrag = TRUE;
  else if (buttons & 2)
    doDeepDrag = TRUE;
  
  FindTable(&theTable, &theNumTable);
  
  do 
    {
      GetMouse(&where, &buttons);
      gotOne = false;
      
      // Figure out which key was pressed.
      for (loop = 1; loop <= NUMKEYS; loop++) 
	{
	  tempRect.left = theKeys[loop].keyLeft;
	  tempRect.top = theKeys[loop].keyTop;
	  tempRect.right = theKeys[loop].keyLeft +
	    theSizes[theKeys[loop].keyType].keyWidth;
	  tempRect.bottom = theKeys[loop].keyTop +
	    theSizes[theKeys[loop].keyType].keyHeight;
	  if (tempRect.Contains(where)) {
	    gotOne = TRUE;
	    if ((lastKey) && (loop != lastKey)) 
	      {
		if (lastKey == fExtKeyMap->scroll_key)
		  theLock = B_SCROLL_LOCK;
		else if (lastKey == fExtKeyMap->num_key)
		  theLock = B_NUM_LOCK;
		else if (lastKey == fExtKeyMap->caps_key)
		  theLock = B_CAPS_LOCK;
		else
		  theLock = 0;
		
		if (theLock) 
		  {
		    if (theLock & fMods) 
		      {
			if (theKeys[lastKey].keyState == UP) 
			  {
			    theKeys[lastKey].keyState = DOWN;
			    DrawKey(lastKey, theTable);
			  }
		      }
		    else 
		      {
			if (theKeys[lastKey].keyState == DOWN) 
			  {
			    theKeys[lastKey].keyState = UP;
			    DrawKey(lastKey, theTable);
			  }
		      }
		  }
		else 
		  {
		    theKeys[lastKey].keyState = UP;
		    DrawKey(lastKey, theTable);
		  }
	      }
	    lastKey = loop;
	    if (IsKeyPad(loop))
	      lastString = fExtKeyMap->control_map[theNumTable + loop];
	    else
	      lastString = fExtKeyMap->control_map[theTable + loop];
	    
	    if ((doDrag) || (doDeepDrag)) 
	      {
		//if (lastChar == NOP_CHAR)
		//  return;
		PRINT(("	creating DragMessage\n"));
		msg = new BMessage(KEY_MSG);
		msg->AddData("string", B_RAW_TYPE, lastString, lastString[0]+1);
		PRINT(("string key is %c\n", lastString[1]));
		msg->AddInt32("key", loop);
		msg->AddInt32("modifiers", modifiers());
		theIndex = 2;
		if (doDeepDrag) 
		  {
		    PRINT(("		deep DragMessage\n"));
		    lastString = fExtKeyMap->control_map[loop];
		    //theText[0] = lastChar;
		    msg->AddData("control", B_RAW_TYPE, lastString, lastString[0]+1);

		    lastString = fExtKeyMap->option_caps_shift_map[loop];
		    //theText[1] = lastChar;
		    msg->AddData("option_caps_shift", B_RAW_TYPE, lastString, lastString[0]+1);

		    lastString = fExtKeyMap->option_caps_map[loop];
		    //theText[2] = lastChar;
		    msg->AddData("option_caps", B_RAW_TYPE, lastString, lastString[0]+1);

		    lastString = fExtKeyMap->option_shift_map[loop];
		    //theText[3] = lastChar;
		    msg->AddData("option_shift", B_RAW_TYPE, lastString, lastString[0]+1);

		    lastString = fExtKeyMap->option_map[loop];
		    //theText[4] = lastChar;
		    msg->AddData("option", B_RAW_TYPE, lastString, lastString[0]+1);

		    lastString = fExtKeyMap->caps_shift_map[loop];
		    //theText[5] = lastChar;
		    msg->AddData("caps_shift", B_RAW_TYPE, lastString, lastString[0]+1);

		    lastString = fExtKeyMap->caps_map[loop];
		    //theText[6] = lastChar;
		    msg->AddData("caps", B_RAW_TYPE, lastString, lastString[0]+1);

		    lastString = fExtKeyMap->shift_map[loop];
		    //theText[7] = lastChar;
		    msg->AddData("shift", B_RAW_TYPE, lastString, lastString[0]+1);

		    lastString = fExtKeyMap->normal_map[loop];
		    //theText[8] = lastChar;
		    msg->AddData("normal", B_RAW_TYPE, lastString, lastString[0]+1);


		    //theText[9] = 0;
		    //msg->AddData("text", B_ASCII_TYPE, theText, 10);
		    //theIndex = 13;
		    PRINT(("Here's the deep outgoing message:\n"));
		    //msg->PrintToStream();
		  }
		BRect keyRect = tempRect;
		keyRect.Set(0, 0, theSizes[theIndex].keyWidth, theSizes[theIndex].keyHeight);
		dragImage = new BBitmap(keyRect, B_COLOR_8_BIT, TRUE);
		dragImage->Lock();
		dragImage->AddChild(tmp_view = new BView(keyRect, "", B_FOLLOW_ALL, B_WILL_DRAW));
		dragImage->SetBits(fKeyBits[theIndex]->Bits(), dragImage->BitsLength(), 0, B_COLOR_8_BIT);
		dragImage->Unlock();
		PRINT(("Doing drag...\n"));

		BPoint dragOff;
		dragOff.x = where.x - tempRect.left;
		dragOff.y = where.y - tempRect.top;
		//dragOff.PrintToStream();
		DragMessage(msg, tempRect); //, dragImage, dragOff);
		//DragMessage(msg, dragImage, BPoint(0, (theSizes[theIndex].keyHeight)), Window());
		return;
	      }
	    
	    if (lastKey == fExtKeyMap->scroll_key)
	      theLock = B_SCROLL_LOCK;
	    else if (lastKey == fExtKeyMap->num_key)
	      theLock = B_NUM_LOCK;
	    else if (lastKey == fExtKeyMap->caps_key)
	      theLock = B_CAPS_LOCK;
	    else
	      theLock = 0;
	    if (theLock) 
	      {
		if (theLock & fMods) 
		  {
		    if (theKeys[lastKey].keyState == DOWN) 
		      {
			theKeys[lastKey].keyState = UP;
			DrawKey(lastKey, theTable);
		      }
		  }
		else 
		  {
		    if (theKeys[lastKey].keyState == UP) 
		      {
			theKeys[lastKey].keyState = DOWN;
			DrawKey(lastKey, theTable);
		      }
		  }
	      }
	    else
	      if (theKeys[loop].keyState == UP) 
		{
		  theKeys[loop].keyState = DOWN;
		  DrawKey(lastKey, theTable);
		}
	    break;
	  }
	}
      if ((!gotOne) && (lastKey)) 
	{
	  if (theLock) 
	    {
	      if (theLock & fMods) 
		{
		  if (theKeys[lastKey].keyState == UP) 
		    {
		      theKeys[lastKey].keyState = DOWN;
		      DrawKey(lastKey, theTable);
		    }
		}
	      else 
		{
		  if (theKeys[lastKey].keyState == DOWN) 
		    {
		      theKeys[lastKey].keyState = UP;
		      DrawKey(lastKey, theTable);
		    }
		}
	    }
	  else
	    if (theKeys[lastKey].keyState == DOWN) 
	      {
		theKeys[lastKey].keyState = UP;
		DrawKey(lastKey, theTable);
	      }
	}
      snooze(50000);
    } while((buttons) && !(doDrag));
  
  if (gotOne) 
    {
      if (theLock) 
	{
	  if ((fMods & theLock) == (modifiers() & theLock)) 
	    {
	      fMods = modifiers();
	      set_keyboard_locks(fMods ^ theLock);
	      Pulse();
	    }
	}
      else 
	{
	  theKeys[lastKey].keyState = UP;
	  DrawKey(lastKey, theTable);
	}
      
      {
	char*  keyString = GetCurKeyString(lastKey);
	if (keyString != NULL && keyString[0] > 0)
	  {
	    char*  cStr  = (char*)malloc(keyString[0] + 1);
	    int32  len = keyString[0];
	    // Convert to C string
	    memcpy(cStr, &keyString[1], len);
	    cStr[len] = '\0';
	    //printf("Key string is %s\n", cStr);
	    fTextView->FakeKeyDown(cStr, len);
	    free(cStr);
	  }
      }
    }
}

//--------------------------------------------------------------------

void TKeymapView::Pulse()
{
	char*		keyStates;
	uchar		keyBits;
	uchar		keyChangedBits;
	int32		loop;
	int32		bitLoop;
	int32		rawKey;
	key_info	theKeyStates;

	get_key_info(&theKeyStates);
	keyStates = (char *)&theKeyStates.key_states;
	theKeyStates.modifiers &=
	  (B_CONTROL_KEY|B_COMMAND_KEY|B_SHIFT_KEY|B_OPTION_KEY|
	   B_CAPS_LOCK|B_NUM_LOCK|B_SCROLL_LOCK);
	if (fMods != theKeyStates.modifiers)
	  KeyState(0,true);

	for (loop = 0; loop < 16; loop++) {
	  keyBits = keyStates[loop];
	  if ((keyChangedBits = fKeyStates[loop] ^ keyBits) != 0) {
		for (bitLoop = 0; bitLoop < 8; bitLoop++) {
		  if ((keyChangedBits & 1) &&
			  ((rawKey = loop * 8 + 7 - bitLoop) <= NUMKEYS)) {
			if ((keyBits & 1) && (theKeys[rawKey].keyState == UP))
			  KeyState(rawKey,true);
			else if (!(keyBits & 1) && (theKeys[rawKey].keyState == DOWN))
			  KeyState(rawKey,false);			
		  }
		  keyChangedBits = keyChangedBits >> 1;
		  keyBits = keyBits >> 1;
		}
		fKeyStates[loop] = keyStates[loop];
	  }
	}
	
	/*
	if (keyStates[14] != fDeadKey) {
	  if (!keyStates[14])
		Key();
	  else
		DeadKey((long)keyStates[14]);
	}
	*/
}

//--------------------------------------------------------------------
void TKeymapView::SetExtKeyMap(ext_keymap *theMap)
{
	fExtKeyMap = theMap;
}

void TKeymapView::DeadKey(long theKey)
{
  long		theTable;
  long		theNumTable;
  long		loop;
  
  fDeadKey = theKey;
  
  FindTable(&theTable, &theNumTable);
  
  Window()->Lock();
  for (loop = 1; loop <= NUMKEYS; loop++) {
	if (IsKeyPad(loop)) {
	  DrawKey(loop, theNumTable);
	} else {
	  DrawKey(loop, theTable);
	}
  }
  Window()->Unlock();
}

char * TKeymapView::GetCurKeyString(int32 theKey)
{
  long theTable;
  long theNumTable;
  bool isDead;
  FindTable(&theTable, &theNumTable);

  if (IsKeyPad(theKey))
	return GetKeyString(theKey,theNumTable,&isDead);
  else
	return GetKeyString(theKey,theTable,&isDead);
}

char * TKeymapView::GetKeyString(int32 theKey, int32 table, bool *isDead)
{
  char* keyString = NULL;

  keyString = fExtKeyMap->control_map[table+theKey];
  
  *isDead = false;
  if ((fDeadKey) && (keyString != NULL) && (keyString[0] != 0)) {
	for (int loop = 0; loop < 16; loop++) {
	  char *deadString =
		fExtKeyMap->acute_dead_key[(fDeadKey - 1) * 32 + (loop * 2)];
	  if ((keyString[0] == deadString[0]) &&
		  (strncmp(keyString+1,deadString+1,keyString[0])==0)) {
		keyString =
		  fExtKeyMap->acute_dead_key[(fDeadKey - 1) * 32 + (loop * 2) + 1];
		*isDead = true;
		break;
	  }
	}
  }

  if (keyString != NULL && keyString[0] == 0)
	  keyString = NULL;
  
  return keyString;
}

//--------------------------------------------------------------------

void TKeymapView::DrawKey(int32 theKey, int32 table)
{
  BRect		tempRect;
  BRect		deadRect;
  int32		state;
  uchar*        charCodeString;
  bool          isDead;
  char          cStr[256];
  
  if (theKey > 0 && theKey < NUMKEYS && table)
    {
      if (theKey == fCurKey)
	{
	  SetHighColor(0,0,255);
	}
      
      state = !theKeys[theKey].keyState;
      tempRect.left = theKeys[theKey].keyLeft + 7;
      tempRect.top = theKeys[theKey].keyTop + 7;
      tempRect.right = theKeys[theKey].keyLeft +
	theSizes[theKeys[theKey].keyType].keyWidth + 7;
      tempRect.bottom = theKeys[theKey].keyTop +
	theSizes[theKeys[theKey].keyType].keyHeight + 7;
      DrawBitmap(fKeyBits[theKeys[theKey].keyType + state],
		 BPoint(tempRect.left, tempRect.top));
      
      charCodeString = (uchar*)GetKeyString(theKey,table,&isDead);
      
      if (isDead) {
	SetHighColor(255,0,0);
	deadRect = tempRect;
	StrokeRect(deadRect);
	deadRect.InsetBy(1, 1);
	StrokeRect(deadRect);
	SetHighColor(0,0,0);
      }
      
      int32 dead = IsDeadKey(theKey);
      if (dead) 
		charCodeString = deads[dead-1];
      
      if (charCodeString &&
	  (charCodeString[1] >= 32) &&
	  (charCodeString[1] != 127)) 
	{
	  
	  
	  int32 charCodeLength = *charCodeString;
	  charCodeString++;
	  
	  if ((charCodeLength >0) && (*(unsigned char*)charCodeString > 0x20) &&
	      charCodeString != NULL)
	    {
	      memcpy(cStr, charCodeString, charCodeLength);
	      cStr[charCodeLength] = 0;
	      BPoint p(tempRect.left + (4 - state), tempRect.bottom - (5 - state));
	      DrawString(cStr,p);
	    }
	}
      
      if (IsDeadKey(theKey)) {
	SetHighColor(255, 255, 0);
	StrokeRect(tempRect);
	tempRect.InsetBy(1, 1);
	StrokeRect(tempRect);
	SetHighColor(0, 0, 0);
      }
      
      if (theKey == fCurKey)
	{
	  StrokeRect(tempRect);
	  SetHighColor(0, 0, 0);
	}
    }
}

//--------------------------------------------------------------------

void TKeymapView::DrawLEDs()
{
  BRect	tempRect;
  
  tempRect.Set(theLEDs[0].ledLeft + 7, theLEDs[0].ledTop + 7,
			   theLEDs[0].ledRight + 7, theLEDs[0].ledBottom + 7);
  if ((fMods & B_NUM_LOCK) && (theLEDs[0].ledState == OFF)) {
	theLEDs[0].ledState = ON;
	SetHighColor(0, 178, 0);
	FillRect(tempRect);
  } else if (!(fMods & B_NUM_LOCK) && (theLEDs[0].ledState == ON)) {
	theLEDs[0].ledState = OFF;
	SetHighColor(0, 55, 0);
	FillRect(tempRect);
  }
  
  tempRect.Set(theLEDs[1].ledLeft + 7, theLEDs[1].ledTop + 7,
			   theLEDs[1].ledRight + 7, theLEDs[1].ledBottom + 7);
  if ((fMods & B_CAPS_LOCK) && (theLEDs[1].ledState == OFF)) {
	theLEDs[1].ledState = ON;
	SetHighColor(0, 178, 0);
	FillRect(tempRect);
  } else if (!(fMods & B_CAPS_LOCK) && (theLEDs[1].ledState == ON)) {
	theLEDs[1].ledState = OFF;
	SetHighColor(0, 55, 0);
	FillRect(tempRect);
  }
  
  tempRect.Set(theLEDs[2].ledLeft + 7, theLEDs[2].ledTop + 7,
			   theLEDs[2].ledRight + 7, theLEDs[2].ledBottom + 7);
  if ((fMods & B_SCROLL_LOCK) && (theLEDs[2].ledState == OFF)) {
	theLEDs[2].ledState = ON;
	SetHighColor(0, 178, 0);
	FillRect(tempRect);
  } else if (!(fMods & B_SCROLL_LOCK) && (theLEDs[2].ledState == ON)) {
	theLEDs[2].ledState = OFF;
	SetHighColor(0, 55, 0);
	FillRect(tempRect);
  }
  SetHighColor(0, 0, 0);
}

//--------------------------------------------------------------------

void TKeymapView::FindTable(long *theTable, long *theNumTable)
{
	bool	numLock = false;

	if (fMods & B_NUM_LOCK)
		numLock = true;

	if ((fMods & B_CONTROL_KEY) && !(fMods & B_COMMAND_KEY))
		*theTable = *theNumTable = 0 * 128;
	else
	if ((fMods & B_OPTION_KEY) && (fMods & B_SHIFT_KEY) && (fMods & B_CAPS_LOCK)) {
		*theTable = 1 * 128;
		if (numLock)
			*theNumTable = 2 * 128;
		else
			*theNumTable = *theTable;
	}
	else
	if ((fMods & B_OPTION_KEY) && (fMods & B_CAPS_LOCK)) {
		*theTable = 2 * 128;
		if (numLock)
			*theNumTable = 1 * 128;
		else
			*theNumTable = *theTable;
	}
	else
	if ((fMods & B_OPTION_KEY) && (fMods & B_SHIFT_KEY)) {
		*theTable = 3 * 128;
		if (numLock)
			*theNumTable = 4 * 128;
		else
			*theNumTable = *theTable;
	}
	else
	if (fMods & B_OPTION_KEY) {
		*theTable = 4 * 128;
		if (numLock)
			*theNumTable = 3 * 128;
		else
			*theNumTable = *theTable;
	}
	else
	if ((fMods & B_CAPS_LOCK) && (fMods & B_SHIFT_KEY)) {
		*theTable = 5 * 128;
		if (numLock)
			*theNumTable = 6 * 128;
		else
			*theNumTable = *theTable;
	}
	else
	if (fMods & B_CAPS_LOCK) {
		*theTable = 6 * 128;
		if (numLock)
			*theNumTable = 5 * 128;
		else
			*theNumTable = *theTable;
	}
	else
	if (fMods & B_SHIFT_KEY) {
		*theTable = 7 * 128;
		if (numLock)
			*theNumTable = 8 * 128;
		else
			*theNumTable = *theTable;
	}
	else {
		*theTable = 8 * 128;
		if (numLock)
			*theNumTable = 7 * 128;
		else
			*theNumTable = *theTable;
	}
}

//--------------------------------------------------------------------

bool TKeymapView::IsKeyPad(long theKey)
{
	if ((theKey == 55) || (theKey == 56) || (theKey == 57) ||
		(theKey == 72) || (theKey == 73) || (theKey == 74) ||
		(theKey == 88) || (theKey == 89) || (theKey == 90) ||
		(theKey == 100) || (theKey == 101))
		return true;
	else
		return false;
}

//--------------------------------------------------------------------

void TKeymapView::Key()
{
  long		theTable;
  long		theNumTable;
  long		loop;
  long		lastDead;
  
  if (fDeadKey) {
	lastDead = fDeadKey;
	fDeadKey = 0;
	FindTable(&theTable, &theNumTable);
	//WAA - get_key_map(&theKeyMap, &keymapbuffer);
	
	for (loop = 1; loop <= NUMKEYS; loop++) {
	  // if (theCharCode != NOP_CHAR) 
	  // {
	  //    for (deadLoop = 0; deadLoop < 16; deadLoop++)
	  //    if (fExtKeyMap->acute_dead_key[(lastDead - 1) * 32 + (deadLoop * 2)] == theCharCode) 
	  //	{
	  Window()->Lock();
	  if (IsKeyPad(loop)) {
		DrawKey(loop, theNumTable);
	  } else {
		DrawKey(loop, theTable);
	  }
	  Window()->Unlock();
	  break;
	}
  }
}

//--------------------------------------------------------------------

void TKeymapView::ReDraw()
{
	long		loop;
	long		theTable;
	long		theNumTable;

	Window()->Lock();
	BFont font;
	font.SetFamilyAndStyle(keymapWind->fFontName, keymapWind->fFontStyle);
	SetFont(&font);

	fTextView->GetFont(&font);
	font.SetFamilyAndStyle(keymapWind->fFontName, keymapWind->fFontStyle);
	fTextView->SetFontAndColor(&font);
	fTextView->Invalidate();
	
	FindTable(&theTable, &theNumTable);

	for (loop = 1; loop <= NUMKEYS; loop++) {
	  if (IsKeyPad(loop))
		DrawKey(loop,theNumTable);
	  else
		DrawKey(loop,theTable);
	}
	Window()->Unlock();
}

//--------------------------------------------------------------------

void TKeymapView::Undo()
{
  char*		tempString;
  long		theTable;
  long		theNumTable;
  
  if (fUndoFlag) 
    {
      if (fUndoDeepFlag) 
	{
	  //printf("DEEP\n");
	  tempString = fExtKeyMap->control_map[fUndoKey];
	  fExtKeyMap->control_map[fUndoKey] = fUndoDeep[0];
	  fUndoDeep[0] = tempString;
	  tempString = fExtKeyMap->option_caps_shift_map[fUndoKey];
	  fExtKeyMap->option_caps_shift_map[fUndoKey] = fUndoDeep[1];
	  fUndoDeep[1] = tempString;
	  tempString = fExtKeyMap->option_caps_map[fUndoKey];
	  fExtKeyMap->option_caps_map[fUndoKey] = fUndoDeep[2];
	  fUndoDeep[2] = tempString;
	  tempString = fExtKeyMap->option_shift_map[fUndoKey];
	  fExtKeyMap->option_shift_map[fUndoKey] = fUndoDeep[3];
	  fUndoDeep[3] = tempString;
	  tempString = fExtKeyMap->option_map[fUndoKey];
	  fExtKeyMap->option_map[fUndoKey] = fUndoDeep[4];
	  fUndoDeep[4] = tempString;
	  tempString = fExtKeyMap->caps_shift_map[fUndoKey];
	  fExtKeyMap->caps_shift_map[fUndoKey] = fUndoDeep[5];
	  fUndoDeep[5] = tempString;
	  tempString = fExtKeyMap->caps_map[fUndoKey];
	  fExtKeyMap->caps_map[fUndoKey] = fUndoDeep[6];
	  fUndoDeep[6] = tempString;
	  tempString = fExtKeyMap->shift_map[fUndoKey];
	  fExtKeyMap->shift_map[fUndoKey] = fUndoDeep[7];
	  fUndoDeep[7] = tempString;
	  tempString = fExtKeyMap->normal_map[fUndoKey];
	  fExtKeyMap->normal_map[fUndoKey] = fUndoDeep[8];
	  fUndoDeep[8] = tempString;
	}
      else 
	{
	  tempString = fExtKeyMap->control_map[fUndoTable];
	  fExtKeyMap->control_map[fUndoTable] = fUndoString;
	  fUndoString = tempString;
	}
      FindTable(&theTable, &theNumTable);
      //printf("Drawing key %ld\n", fUndoKey);
      if (IsKeyPad(fUndoKey))
	{
	  DrawKey(fUndoKey, theNumTable);
	}
      else
	{
	  DrawKey(fUndoKey, theTable);
	}
      fDirty = true;
      keymapWind->SetMenuEnable(MENU_SAVE, true);
    }
}

//====================================================================

TEditTextView::TEditTextView(TKeymapView* parent, BRect frame, const char *name, BRect textRect, long resizeMode, long flags) :
		BTextView(frame, name, textRect, resizeMode, flags)
{
  mParent = parent;
}

//--------------------------------------------------------------------

void TEditTextView::AttachedToWindow()
{
	BTextView::AttachedToWindow();
	//	SetFontSize(12);
	SetLowColor(168, 168, 168);
	SetWordWrap(true);
	//	SetFlags(Flags() & ~B_NAVIGABLE);
}

void TEditTextView::FakeKeyDown(const char* key, int32 size)
{
  BTextView::KeyDown(key,size);
}

void TEditTextView::KeyDown(const char* , int32 )
{
  
}

