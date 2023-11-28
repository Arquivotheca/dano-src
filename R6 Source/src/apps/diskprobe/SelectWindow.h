//--------------------------------------------------------------------
//	
//	SelectWindow.h
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef SELECT_WINDOW_H
#define SELECT_WINDOW_H

#include <Box.h>
#include <Button.h>
#include <Font.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <RadioButton.h>
#include <StringView.h>
#include <Window.h>

#define	TITLE_BAR_HEIGHT	 25
#define	SELECT_WIDTH		300
#define SELECT_HEIGHT		150

#define TEXT1_X1			 25
#define TEXT1_Y1			  6
#define TEXT1_X2			(SELECT_WIDTH - TEXT1_X1)
#define TEXT1_Y2			(TEXT1_Y1 + 16)
#define TEXT1_TEXT			"Examine"B_UTF8_ELLIPSIS

#define DEVICE_X1			(TEXT1_X1 + 10)
#define DEVICE_Y1			(TEXT1_Y2 + 10)
#define DEVICE_X2			(DEVICE_X1 + 13)
#define DEVICE_Y2			(DEVICE_Y1 + 16)
#define DEVICE_TEXT			""

#define MENU_X1				(DEVICE_X2 + 4)
#define MENU_Y1				(DEVICE_Y1 - 2)
#define MENU_X2				(SELECT_WIDTH - 10)
#define MENU_Y2				DEVICE_Y2
#define MENU_TEXT			"Device"

#define TEXT2_X1			(TEXT1_X1 + 10)
#define TEXT2_Y1			(DEVICE_Y2 + 10)
#define TEXT2_X2			(SELECT_WIDTH - TEXT2_X1)
#define TEXT2_Y2			(TEXT2_Y1 + 16)
#define TEXT2_TEXT			"- or -"

#define FILE_X1				DEVICE_X1
#define FILE_Y1				(TEXT2_Y2 + 10)
#define FILE_X2				(SELECT_WIDTH - FILE_X1)
#define FILE_Y2				(FILE_Y1 + 16)
#define FILE_TEXT			"File"B_UTF8_ELLIPSIS

#define OK_BUTTON_X1		(SELECT_WIDTH - BUTTON_WIDTH - 10)
#define OK_BUTTON_Y1		(SELECT_HEIGHT - (BUTTON_HEIGHT + 14))
#define OK_BUTTON_X2		(OK_BUTTON_X1 + BUTTON_WIDTH)
#define OK_BUTTON_Y2		(OK_BUTTON_Y1 + BUTTON_HEIGHT)
#define OK_BUTTON_TEXT		"Probe"

#define CANCEL_BUTTON_X1	(OK_BUTTON_X1 - (BUTTON_WIDTH + 9))
#define CANCEL_BUTTON_Y1	OK_BUTTON_Y1
#define CANCEL_BUTTON_X2	(CANCEL_BUTTON_X1 + BUTTON_WIDTH)
#define CANCEL_BUTTON_Y2	OK_BUTTON_Y2
#define CANCEL_BUTTON_TEXT	"Cancel"

enum	SELECT				{M_DEVICE = M_OPEN_DEVICE + 64, M_FILE,
							 M_OK, M_CANCEL};


//====================================================================

class TSelectWindow : public BWindow {

private:

int32			fType;
BPopUpMenu		*fDeviceMenu;
BRadioButton	*fDevice;

public:

				TSelectWindow(BRect);
				~TSelectWindow(void);
virtual void	MessageReceived(BMessage*);
void			ScanDir(const char*, BMenu*);
};
#endif
