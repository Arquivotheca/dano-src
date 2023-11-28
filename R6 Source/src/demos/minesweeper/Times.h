//--------------------------------------------------------------------
//	
//	Times.h
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef TIMES_H
#define TIMES_H

#include <Button.h>
#include <File.h>
#include <Point.h>
#include <Rect.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

#define BEST_TEXT1			"Fastest Mine Sweepers"
#define BEST_TEXT2			"Beginner:"
#define BEST_TEXT3			"Intermediate:"
#define BEST_TEXT4			"Expert:"
#define BEST_TEXT5			"seconds"
#define BEST_BUTTON1		"Reset"
#define BEST_BUTTON2		"OK"

#define BEST_WIND_WIDTH		380
#define BEST_WIND_HEIGHT	145
#define BEST_LINE1_V		 19
#define BEST_LINE2_V		 44
#define BEST_LINE3_V		 68
#define BEST_LINE4_V		 92
#define BEST_CAT			 17
#define BEST_TIME			128
#define BEST_NAME			251
#define BEST_RESET_H		209
#define BEST_OK_H			294
#define BEST_BUTTON_V		110
#define BEST_BUTTON_WIDTH	 75
#define BEST_BUTTON_HEIGHT	 23

#define FAST_TEXT1			"You have the fastest time"
#define FAST_TEXT2			"Please type your name."
#define FAST_TEXT3			"for beginner level."
#define FAST_TEXT4			"for intermediate level."
#define FAST_TEXT5			"for expert level."
#define FAST_BUTTON_TEXT	"OK"
#define FAST_DEFAULT_NAME	"Anonymous"

#define	FAST_WIND_WIDTH		200
#define FAST_WIND_HEIGHT	114
#define FAST_LINE1_V		 15
#define FAST_LINE2_V		 28
#define FAST_LINE3_V		 41
#define FAST_TEXT_H			  4
#define FAST_TEXT_V			 50
#define FAST_TEXT_WIDTH		(FAST_WIND_WIDTH - (2 * FAST_TEXT_H))
#define FAST_TEXT_HEIGHT	 17
#define FAST_BUTTON_WIDTH	 75
#define FAST_BUTTON_HEIGHT	 23
#define FAST_BUTTON_H		((FAST_WIND_WIDTH - FAST_BUTTON_WIDTH) / 2)
#define FAST_BUTTON_V		(FAST_TEXT_V + FAST_TEXT_HEIGHT + 14)


class	TBestView;
class	TFastView;


//====================================================================

class TBestWindow : public BWindow {

private:

public:

TBestView*		fView;

				TBestWindow(BRect, char*);
virtual void	MessageReceived(BMessage*);
virtual bool	QuitRequested();
};

//--------------------------------------------------------------------

class TBestView : public BView {

private:

public:

				TBestView(BRect, char*); 
virtual	void	Draw(BRect);
};

//--------------------------------------------------------------------

class TFastWindow : public BWindow {

private:
char*			fName;
BTextControl*	fText;

public:
int32			fTime;
int32			fType;

TFastView*		fView;

				TFastWindow(BRect, char*, int32, char*, int32);
virtual void	MessageReceived(BMessage*);
};

//--------------------------------------------------------------------

class TFastView : public BView {

private:

int32			fType;
BTextControl*	fText;

public:

				TFastView(BRect, char*, int32, BTextControl*);
virtual	void	Draw(BRect);
};
#endif
