/* ++++++++++

   FILE:  HP_Print.h
   REVS:  $Revision: 1.1 $
   NAME:  Robert Polic

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef HP_PRINT_H
#define HP_PRINT_H

#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _CHECK_BOX_H
#include <CheckBox.h>
#endif
#ifndef _LOOPER_H
#include <Looper.h>
#endif
#ifndef _MESSAGE_FILTER_H
#include <MessageFilter.h>
#endif
#ifndef _MENU_FIELD_H
#include <MenuField.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _POINT_H
#include <Point.h>
#endif
#ifndef _POP_UP_MENU_H
#include <PopUpMenu.h>
#endif
#ifndef _RADIO_BUTTON_H
#include <RadioButton.h>
#endif
#ifndef _RECT_H
#include <Rect.h>
#endif
#ifndef _TEXT_CONTROL_H
#include <TextControl.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif

#define	PRINT_WIDTH			300
#define PRINT_HEIGHT		130

#define	MENU_HEIGHT			 16

#define COPIES_H			 10
#define COPIES_V			 12
#define COPIES_WIDTH		 80
#define COPIES_HEIGHT		 16
#define COPIES_TEXT			"Copies"
#define	PRINT_COPIES		100

#define RANGE_H				 17
#define RANGE_V				 54
#define	RANGE_TEXT			"Pages"

#define ALL_H				 52
#define ALL_V				 42
#define ALL_WIDTH			 36
#define ALL_HEIGHT			 16
#define ALL_TEXT			"All"

#define SELECTION_H			120
#define SELECTION_V			ALL_V
#define SELECTION_WIDTH		 16
#define SELECTION_HEIGHT	 16
#define SELECTION_TEXT		""

#define FROM_H				(SELECTION_H + SELECTION_WIDTH + 1)
#define FROM_V				ALL_V
#define FROM_WIDTH			 73
#define FROM_HEIGHT			 16
#define FROM_TEXT			"From"

#define TO_H				(FROM_H + FROM_WIDTH + 13)
#define TO_V				FROM_V
#define TO_WIDTH			 59
#define TO_HEIGHT			FROM_HEIGHT
#define TO_TEXT				"To"

enum	RANGE				{RANGE_ALL = 200, RANGE_SELECTION, RANGE_FROM,
							 RANGE_TO};

#define QUALITY_H			  6
#define QUALITY_V			 71
#define QUALITY_TEXT		"Quality"
enum	QUALITY				{QUALITY_DRAFT = 300, QUALITY_GOOD, QUALITY_BEST};

#define PRINT_BUTTON_WIDTH	 70
#define PRINT_BUTTON_HEIGHT	 20

#define PRINT_LINE_V		(PRINT_HEIGHT - PRINT_BUTTON_HEIGHT - 28)

#define PRINT_OK_BUTTON_H		(PRINT_WIDTH - PRINT_BUTTON_WIDTH - 10)
#define PRINT_OK_BUTTON_V		(PRINT_HEIGHT - PRINT_BUTTON_HEIGHT - 11)
#define PRINT_OK_BUTTON_TEXT	"OK"

#define PRINT_SAVE_CHECKBOX_WIDTH	90
#define PRINT_SAVE_CHECKBOX_HEIGHT	20
#define PRINT_SAVE_CHECKBOX_H		(10)
#define PRINT_SAVE_CHECKBOX_V		(PRINT_HEIGHT - PRINT_SAVE_CHECKBOX_HEIGHT - 11)
#define PRINT_SAVE_CHECKBOX_TEXT	"Save To File"

#define PRINT_CANCEL_BUTTON_H	(PRINT_OK_BUTTON_H - PRINT_BUTTON_WIDTH - 12)
#define PRINT_CANCEL_BUTTON_V	PRINT_OK_BUTTON_V
#define PRINT_CANCEL_BUTTON_TEXT	"Cancel"

enum	PRINT_BUTTONS		{PRINT_OK = 1, PRINT_CANCEL,PRINT_SAVE};

class	TPrintView;

filter_result	PrintKeyFilter(BMessage*, BHandler**, BMessageFilter*);	


//====================================================================

class BPrint : public BWindow {

public:
TPrintView		*fView;

private:
char			fResult;
long			fPrintSem;
BMessage		*fPrintMessage;
BMessageFilter	*fFilter;

public:
				BPrint(BMessage*, char*);
				~BPrint(void);
virtual	void	MessageReceived(BMessage*);
		bool	QuitRequested();
		long	Go(void);
};

//--------------------------------------------------------------------

class TPrintView : public BView {

public:

BTextControl	*fCopies;
BTextControl	*fFrom;
BTextControl	*fTo;

private:

long			fPrintQuality;
BMenuField		*fQuality;
BRadioButton	*fAllButton;
BRadioButton	*fFromButton;
BCheckBox		*fSaveToFileCheckBox;

	BMessage	*fPageSettings;	
		bool	IsPrintToFile();

public:
				TPrintView(BRect, BMessage*);
virtual void	AttachedToWindow(void);
virtual	void	Draw(BRect rect);
virtual	void	MessageReceived(BMessage*);
		void	UpdateMessage(BMessage*);
		void	AddSaveToFileFlag(BMessage*);

	long	begin;
	long	end;
	long	copies;
};
#endif
