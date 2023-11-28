//--------------------------------------------------------------------
//	
//	IconTools.h
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ICON_TOOLS_H
#define ICON_TOOLS_H

#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _FONT_H
#include <Font.h>
#endif
#ifndef _POINT_H
#include <Point.h>
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

#define TOOLWIDTH	9	// in cells
#define TOOLHEIGHT	2	// in cells
#define	TOOLSIZE	32	// in pixels

#define	NUMTOOLS	18

#define PARAM_WIDTH		232
#define PARAM_HEIGHT	120
#define TEXT_H			  4
#define TEXT_V			 12
#define TEXT_WIDTH		(PARAM_WIDTH - (TEXT_H * 2))
#define TEXT_ARC		"Arc Parameters"
#define TEXT_RRECT		"Round Rect Parameters"
#define BREAK_H			(TEXT_H - 2)
#define BREAK_V			TEXT_V + 2
#define BREAK_WIDTH		(TEXT_WIDTH + 2)
#define PARAM1_H		36
#define PARAM1_V		(BREAK_H + 26)
#define PARAM1_WIDTH	(PARAM_WIDTH - (PARAM1_H * 2))
#define PARAM1_HEIGHT	16
#define PARAM1_TEXT_ARC	"Start Angle"
#define PARAM1_TEXT_RRECT "X Radius"
#define PARAM2_H		PARAM1_H
#define PARAM2_V		(PARAM1_V + PARAM1_HEIGHT + 10)
#define PARAM2_WIDTH	PARAM1_WIDTH
#define PARAM2_HEIGHT	PARAM1_HEIGHT
#define PARAM2_TEXT_ARC	"Arc Angle"
#define PARAM2_TEXT_RRECT "Y Radius"
#define BUTTON1_H		(PARAM_WIDTH - 80)
#define BUTTON1_V		(PARAM_HEIGHT - 32)
#define BUTTON1_WIDTH	70
#define BUTTON1_HEIGHT	15
#define BUTTON1_TEXT	"OK"
#define BUTTON2_H		(BUTTON1_H - 10 - BUTTON1_WIDTH)
#define BUTTON2_V		BUTTON1_V
#define BUTTON2_WIDTH	BUTTON1_WIDTH
#define BUTTON2_HEIGHT	BUTTON1_HEIGHT
#define BUTTON2_TEXT	"Cancel"

enum TOOLTYPES		{T_MARQUEE,
					 T_LASSO,
					 T_ERASER,
					 T_PENCIL,
					 T_DROPPER,
					 T_BUCKET,
					 T_MAGNIFY,
					 T_LINE,
					 T_FILL_RECT,
					 T_RECT,
					 T_FILL_RRECT,
					 T_RRECT,
					 T_FILL_OVAL,
					 T_OVAL,
					 T_FILL_ARC,
					 T_ARC,
					 T_FILL_TRIANGLE,
					 T_TRIANGLE};

typedef struct {
	short	x;
	short	y;
	short	dir;
} last_pos;


//====================================================================

class TToolView : public BView {

private:
short			fTool;
float			fArcAngle;
float			fStartAngle;
float			fXRadius;
float			fYRadius;
BBitmap			*fToolIcons[NUMTOOLS];

public:
				TToolView(BRect);
				~TToolView(void); 
virtual	void	AttachedToWindow(void);
virtual	void	Draw(BRect);
virtual void	MouseDown(BPoint);
void			GetParameters(short);
short			GetTool(void);
BBitmap*		GetToolMap(short);
short			GetToolType(void);
void			SetTool(short);
void			Dropper(short, short, BBitmap*);
void			Fill(short, short, short, BView*, uchar*, uchar*);
void			Pen(short, short, short, short, BView*, short);
void			RecursiveFill(short, short, uchar, BView*, uchar*, uchar*);
void			Shape(short, short, short, short, BView*, short, short, short,
					  bool);
};

//--------------------------------------------------------------------

#define VIEW_COLOR			216

enum	param_messages		{PARAM1 = 128, PARAM2, OK, CANCEL};

class	TParamView;

class TParamWindow : public BWindow {

private:

TParamView		*fView;

public:

				TParamWindow(BRect, short, float*, float*);
virtual void	MessageReceived(BMessage*);
};

//--------------------------------------------------------------------

class TParamView : public BView {

private:

short			fTool;
float			*fParam1Value;
float			*fParam2Value;
BButton			*fOK;
BButton			*fCancel;
BTextControl	*fParam1;
BTextControl	*fParam2;

public:

				TParamView(BRect, short, float*, float*);
virtual	void	AttachedToWindow(void);
virtual	void	Draw(BRect);
virtual void	MessageReceived(BMessage*);
};
#endif
