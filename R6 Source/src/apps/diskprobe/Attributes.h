//--------------------------------------------------------------------
//	
//	Attributes.h
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include <Button.h>
#include <Font.h>
#include <Rect.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>
#include <Box.h>

#define ATTRIBUTE_WIDTH		300
#define ATTRIBUTE_HEIGHT	150

#define TYPE_X1				  8
#define TYPE_Y1				  7
#define TYPE_X2				(ATTRIBUTE_WIDTH - TYPE_X1)
#define TYPE_Y2				(TYPE_Y1 + 10)
#define TYPE_TEXT			"Type: "

#define NAME_X1				TYPE_X1
#define NAME_Y1				(TYPE_Y2 + 4)
#define NAME_X2				TYPE_X2
#define NAME_Y2				(NAME_Y1 + 10)
#define NAME_TEXT			"Name: "

#define FIELD_X1			  8
#define FIELD_Y1			(NAME_Y2 + 5)
#define FIELD_X2			(ATTRIBUTE_WIDTH - FIELD_X1)
#define FIELD_Y2			(ATTRIBUTE_HEIGHT - (BUTTON_HEIGHT + 12) - 12)

#define FIELD1_X1			FIELD_X1
#define FIELD1_Y1			(NAME_Y2 + 20)
#define FIELD1_X2			((ATTRIBUTE_WIDTH / 2) - FIELD1_X1)
#define FIELD1_Y2			(FIELD1_Y1 + 16)

#define FIELD2_X1			(FIELD1_X2 + (FIELD1_X1 * 2))
#define FIELD2_Y1			FIELD1_Y1
#define FIELD2_X2			(ATTRIBUTE_WIDTH - FIELD1_X1)
#define FIELD2_Y2			FIELD1_Y2

#define FIELD3_X1			FIELD1_X1
#define FIELD3_Y1			(FIELD1_Y2 + 8)
#define FIELD3_X2			FIELD1_X2
#define FIELD3_Y2			(FIELD3_Y1 + 16)

#define FIELD4_X1			FIELD2_X1
#define FIELD4_Y1			FIELD3_Y1
#define FIELD4_X2			FIELD2_X2
#define FIELD4_Y2			FIELD3_Y2

#define OK_BUTTON_X1		(ATTRIBUTE_WIDTH - BUTTON_WIDTH - 9)
#define OK_BUTTON_Y1		(ATTRIBUTE_HEIGHT - (BUTTON_HEIGHT + 13))
#define OK_BUTTON_X2		(OK_BUTTON_X1 + BUTTON_WIDTH)
#define OK_BUTTON_Y2		(OK_BUTTON_Y1 + BUTTON_HEIGHT)
#define OK_BUTTON_TEXT		"Done"

#define CANCEL_BUTTON_X1	(OK_BUTTON_X1 - BUTTON_WIDTH - 9)
#define CANCEL_BUTTON_Y1	OK_BUTTON_Y1
#define CANCEL_BUTTON_X2	(CANCEL_BUTTON_X1 + BUTTON_WIDTH)
#define CANCEL_BUTTON_Y2	OK_BUTTON_Y2
#define CANCEL_BUTTON_TEXT	"Cancel"

#define REVERT_BUTTON_X1	 9
#define REVERT_BUTTON_Y1	OK_BUTTON_Y1
#define REVERT_BUTTON_X2	(REVERT_BUTTON_X1 + BUTTON_WIDTH)
#define REVERT_BUTTON_Y2	OK_BUTTON_Y2
#define REVERT_BUTTON_TEXT	"Revert"

#define M_OK				'OK  '
#define M_CANCEL			'CNCL'
#define M_DIRTY				'DIRT'
#define M_REVERT			'REVR'
#define M_CHANGED			'CHNG'
#define M_MODIFIED			'MODF'

enum						{LEFT = 0, TOP, RIGHT, BOTTOM};
enum						{X = 0, Y};

class	TAttributesView;
class	TProbeWindow;
class	TTextView;


//====================================================================

class TAttributesWindow : public BWindow {

private:

TAttributesView	*fAttributesView;

public:

				TAttributesWindow(BRect, attribute*, TProbeWindow*, bool);
virtual	void	MessageReceived(BMessage*);
virtual bool	QuitRequested(void);
};


//====================================================================

class TAttributesView : public BBox {

private:

bool			fDirty;
bool			fReadOnly;
BButton			*fCancelButton;
BButton			*fOKButton;
BButton			*fRevertButton;
BTextControl	*fFields[4];
BView			*fView;
TTextView		*fTextView;
TProbeWindow	*fWindow;
attribute		*fAttr;

public:
				TAttributesView(BRect, attribute*, TProbeWindow*, bool);
virtual	void	AttachedToWindow(void);
virtual	void	MessageReceived(BMessage*);
virtual	void	Draw(BRect);
bool			CheckData(void);
status_t		GetData(void);
bool			Quit(int32 = 0);
status_t		SetData(void);
};


//====================================================================

class TTextView : public BTextView {

private:

bool			*fDirty;

public:
				TTextView(BRect, BRect, bool*);
virtual	void	InsertText(const char*, int32, int32, const text_run_array*);
virtual	void	DeleteText(int32, int32);
};
#endif
