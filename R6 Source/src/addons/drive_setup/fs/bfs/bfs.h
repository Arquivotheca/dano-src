//--------------------------------------------------------------------
//	
//	bfs.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef BFS_H
#define BFS_H

#include <drive_setup.h>
#include <bfs_map.h>

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

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
#define TITLE_TEXT			"Be File System"

#define LINE1_H				  5
#define LINE1_V				(TITLE_V + 20)
#define LINE1_WIDTH			(WIND_WIDTH - (2 * LINE1_H))

#define BLOCK_MENU_H		 10
#define BLOCK_MENU_V		(LINE1_V + 10)
#define BLOCK_MENU_WIDTH	(WIND_WIDTH - (2 * BLOCK_MENU_H))
#define BLOCK_MENU_TEXT		"File System Block Size"

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
