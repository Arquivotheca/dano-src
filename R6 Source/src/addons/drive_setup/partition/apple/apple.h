//--------------------------------------------------------------------
//	
//	apple.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef APPLE_H
#define APPLE_H

#include <drive_setup.h>
#include "mac_map.h"

#include <Alert.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Directory.h>
#include <Drivers.h>
#include <Entry.h>
#include <Font.h>
#include <image.h>
#include <List.h>
#include <ListView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

#define BLOCK_SIZE			512

#define	WIND_WIDTH			423
#define WIND_HEIGHT			356
#define VIEW_COLOR			216
#define SELECT_COLOR		180

#define BUTTON_WIDTH		 70
#define BUTTON_HEIGHT		 22

#define BUTTON_PARTITION_H	(WIND_WIDTH - 10 - BUTTON_WIDTH)
#define BUTTON_PARTITION_V	(WIND_HEIGHT - 10 - BUTTON_HEIGHT)
#define BUTTON_PARTITION_TEXT	"OK"

#define BUTTON_CANCEL_H		(BUTTON_PARTITION_H - 10 - BUTTON_WIDTH)
#define BUTTON_CANCEL_V		BUTTON_PARTITION_V
#define BUTTON_CANCEL_TEXT	"Cancel"

#define BUTTON_REVERT_H		(BUTTON_CANCEL_H - 10 - BUTTON_WIDTH)
#define BUTTON_REVERT_V		BUTTON_CANCEL_V
#define BUTTON_REVERT_TEXT	"Revert"

#define BUTTON_ADD_H		 10
#define BUTTON_ADD_V		BUTTON_PARTITION_V
#define BUTTON_ADD_TEXT		"Add"

#define BUTTON_REMOVE_H		(BUTTON_ADD_H + BUTTON_WIDTH + 10)
#define BUTTON_REMOVE_V		BUTTON_ADD_V
#define BUTTON_REMOVE_TEXT	"Remove"

#define TITLE_H				 10
#define TITLE_V				  6
#define TITLE_WIDTH			(WIND_WIDTH - (2 * TITLE_H))
#define TITLE_TEXT			"Apple Style Partition Map"

#define H_LINE_H			  5
#define H_LINE_WIDTH		(WIND_WIDTH - (2 * H_LINE_H))
#define LINE1_V				(TITLE_V + 20)
#define LINE2_V				(WIND_HEIGHT - BUTTON_HEIGHT - 20)

#define LAYOUT_MENU_H		 10
#define LAYOUT_MENU_V		(LINE1_V + 10)
#define LAYOUT_MENU_WIDTH	(WIND_WIDTH - (2 * LAYOUT_MENU_H))

#define PART_100_BE		"100% BeOS Partition"
#define PART_100_HFS	"100% HFS Partition"
#define PART_50			"50% BeOS / 50% HFS"
#define PART_25			"4 25% Partitions"

#define LABEL_V				(LAYOUT_MENU_V + 32)
#define LABEL_PARTITION_H	  0
#define LABEL_PARTITION_TEXT "Partition Type"
#define LABEL_FS_H			(LABEL_PARTITION_H + (LIST_WIDTH / 3.5))
#define LABEL_FS_TEXT		"File System"
#define LABEL_VOLUME_H		(LABEL_FS_H + (LIST_WIDTH / 5.5))
#define LABEL_VOLUME_TEXT	"Volume Name"
#define LABEL_SIZE_H		(LABEL_VOLUME_H + (LIST_WIDTH / 2.65))
#define LABEL_SIZE_TEXT		"Size"

#define LIST_H				 10
#define LIST_V				(LABEL_V + 13)
#define LIST_WIDTH			(WIND_WIDTH - (2 * LIST_H) - B_V_SCROLL_BAR_WIDTH)
#define LIST_HEIGHT			100

#define PART_MENU_WIDTH		50
#define PART_MENU_H			(WIND_WIDTH - 10 - PART_MENU_WIDTH)
#define PART_MENU_V			(LIST_V + LIST_HEIGHT + 10 + 16 + 15)
#define PART_MENU_TEXT		"Types"

#define PART_NAME_H			 10
#define PART_NAME_V			(LIST_V + LIST_HEIGHT + 15)
#define PART_NAME_WIDTH		(PART_MENU_H - (2 * PART_NAME_H))
#define PART_NAME_TEXT		"Partition Name"

#define PART_TYPE_H			PART_NAME_H
#define PART_TYPE_V			PART_MENU_V
#define PART_TYPE_WIDTH		PART_NAME_WIDTH
#define PART_TYPE_TEXT		"Partition Type"

#define SLIDER_H			(PART_NAME_H + 3)
#define SLIDER_V			(PART_TYPE_V + 10 + 10)
#define SLIDER_WIDTH		(WIND_WIDTH - (2 * SLIDER_H))
#define SLIDER_HEIGHT		 30
#define SLIDER_TEXT_H		  0 
#define SLIDER_TEXT			"Partition Size"

#define BUTTON_UPDATE_H		BUTTON_ADD_H
#define BUTTON_UPDATE_V		(SLIDER_V + SLIDER_HEIGHT + 10)
#define BUTTON_UPDATE_TEXT	"Update"

#define BUTTON_ITEM_H		BUTTON_REMOVE_H
#define BUTTON_ITEM_V		BUTTON_UPDATE_V
#define BUTTON_ITEM_TEXT	"Revert"

enum MESSAGES				{M_OK = 128, M_CANCEL, M_ADD, M_REMOVE,
							 M_REVERT, M_LAYOUT, M_LIST_INVOKED,
							 M_LIST_SELECTED, M_PART_MENU, M_PART_NAME,
							 M_PART_TYPE, M_SLIDER, M_KB, M_MB, M_GB,
							 M_UPDATE, M_ITEM};

class TListView;
class TListItem;
class TSliderView;


//====================================================================

class ApplePartWindow : public BWindow {

private:

bool			fReadOnly;
int32			fBlockSize;
int32			fLogicalSize;
int32			fDevice;
uint64			fFree;
uint64			fOffset;
uint64			fSize;
BButton			*fAdd;
BButton			*fItem;
BButton			*fModify;
BButton			*fRemove;
BButton			*fRevert;
BButton			*fUpdate;
BList			*fOrig;
BMessage		*fMessage;
BPopUpMenu		*fPartMenu;
BTextControl	*fPartName;
BTextControl	*fPartType;
TListView		*fList;
TSliderView		*fSlider;
Block0			fSB;

public:

				ApplePartWindow(BRect, BMessage*);
				~ApplePartWindow(void);
virtual void	MessageReceived(BMessage*);
void			ClearList(void);
void			CheckFree(void);
void			UpdateList(void);
int32			WriteMap(void);
};

//--------------------------------------------------------------------

class TListView : public BListView {

private:

public:

				TListView(BRect); 
virtual void	Draw(BRect);
};


//--------------------------------------------------------------------

class TListItem : public BListItem {

private:

int32			fBlockSize;
float			fHeight;
partition_data	*fPartition;

public:

				TListItem(partition_data*, int32); 
				~TListItem(void);
virtual void	DrawItem(BView*, BRect, bool);
virtual void	Update(BView*, const BFont*);
partition_data*	PartitionInfo(void);
};


//====================================================================

class AppleBox : public BBox {

private:

public:
				AppleBox(BRect);
virtual void	Draw(BRect);
};


//--------------------------------------------------------------------

class TSliderView : public BView {

private:

bool			fEnabled;
bool			fFocus;
uint64			fMag;
uint64			fNew;
uint64			fMin, fMax;
float			fWidth;
float			fOffset;
BBitmap			*fSlider;
BView			*fOffView;

public:

				TSliderView(BRect, uint64, uint64, uint64); 
				~TSliderView(void);
virtual void	AttachedToWindow(void);
virtual	void	Draw(BRect);
virtual void	KeyDown(const char*, int32);
virtual void	MakeFocus(bool);
virtual void	MouseDown(BPoint);
void			DrawSlider(void);
void			DrawRange(bool);
void			SetEnabled(bool);
void			SetRange(uint64, uint64);
void			SetValue(uint64);
uint64			Value(void);
};


//====================================================================

void fit_string(BView*, char*, float);
#endif
