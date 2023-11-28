//--------------------------------------------------------------------
//	
//	dos.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef DOS_H
#define DOS_H

#include <drive_setup.h>

#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _BOX_H
#include <Box.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _MENU_FIELD_H
#include <MenuField.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _POP_UP_MENU_H
#include <PopUpMenu.h>
#endif
#ifndef _STRING_VIEW_H
#include <StringView.h>
#endif
#ifndef TEXT_CONTROL_H_
#include <TextControl.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif

#include <String.h>


#define WIND_WIDTH	250
#define WIND_HEIGHT	130

#define BUTTON_WIDTH		 70
#define BUTTON_HEIGHT		 24

#define BUTTON_OK_H			(WIND_WIDTH - 10 - BUTTON_WIDTH)
#define BUTTON_OK_V			(WIND_HEIGHT - 10 - BUTTON_HEIGHT)
#define BUTTON_OK_TEXT		"Initialize"

#define BUTTON_CANCEL_H		(BUTTON_OK_H - 10 - BUTTON_WIDTH)
#define BUTTON_CANCEL_V		BUTTON_OK_V
#define BUTTON_CANCEL_TEXT	"Cancel"

#define TITLE_H				  7
#define TITLE_V				  6
#define TITLE_WIDTH			(WIND_WIDTH - (2 * TITLE_H))
#define TITLE_TEXT			"DOS File System"

#define LINE1_H				  5
#define LINE1_V				(TITLE_V + 20)
#define LINE1_WIDTH			(WIND_WIDTH - (2 * LINE1_H))

#define BLOCK_MENU_H		 10
#define BLOCK_MENU_V		(LINE1_V + 10)
#define BLOCK_MENU_WIDTH	(WIND_WIDTH - (2 * BLOCK_MENU_H))
#define BLOCK_MENU_TEXT		"FAT entry size"

#define VOLUME_NAME_H		 10
#define VOLUME_NAME_V		(BLOCK_MENU_V + 30)
#define VOLUME_NAME_WIDTH	(WIND_WIDTH - (2 * VOLUME_NAME_H))
#define VOLUME_NAME_TEXT	"Volume Name"

enum MESSAGES				{M_OK = 128, M_CANCEL, M_MENU, M_TEXT};


//====================================================================

class InitWindow : public BWindow {

private:

BString			fDevice;
int32			fBlockSize;
int32			fSize;
BMessage		*fMessage;
BTextControl	*fVolumeName;

public:

				InitWindow(BRect, BMessage*);
virtual void	MessageReceived(BMessage*);
status_t		Initialize(void);
};

#endif
