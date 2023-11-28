//--------------------------------------------------------------------
//	
//	Header.h
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef HEADER_H
#define HEADER_H

#include <Bitmap.h>
#include <Point.h>
#include <Rect.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>

#define HEADER_HEIGHT		 43

#define ICON_X1				 10
#define ICON_Y1				  8
#define ICON_X2				(ICON_X1 + B_LARGE_ICON - 1)
#define ICON_Y2				(ICON_Y1 + B_LARGE_ICON - 1)

#define LABEL_FIELD_X1		 50
#define LABEL_FIELD_Y1		  7
#define LABEL_FIELD_Y2		(LABEL_FIELD_Y1 + 12)
#define DEVICE_TEXT			"Device: "
#define FILE_TEXT			"File: "

#define BLOCK_TEXT			"Block:"
#define BLOCK_FIELD_X1		LABEL_FIELD_X1
#define BLOCK_FIELD_Y1		(LABEL_FIELD_Y2 + 8)
#define BLOCK_FIELD_Y2		(BLOCK_FIELD_Y1 + 12)

#define EDIT_FIELD_Y1		BLOCK_FIELD_Y1
#define EDIT_FIELD_Y2		BLOCK_FIELD_Y2

#define TOTAL_FIELD_X1		(EDIT_FIELD_X1 + 10)
#define TOTAL_FIELD_Y1		BLOCK_FIELD_Y1
#define TOTAL_FIELD_X2		(TOTAL_FIELD_X1 + 50)
#define TOTAL_FIELD_Y2		BLOCK_FIELD_Y2

#define BLOCK_OFFSET_TEXT	"Offset: "
#define DEVICE_OFFSET_TEXT	"Device Offset: "
#define FILE_OFFSET_TEXT	"File Offset: "


//====================================================================

class THeaderView : public BView {

private:

int32			fBase;
off_t			fBlock;
off_t			fSize;
BBitmap			*fIcon;
BStringView		*fName;
BStringView		*fOffset1;
BStringView		*fOffset2;
BStringView		*fTotal;

public:

BTextControl	*fPosition;

				THeaderView(BRect, int32, int32);
virtual void	Draw(BRect);
virtual void	MessageReceived(BMessage*);
void			GetSelection(int32*, int32*);
void			SetBase(int32);
void			SetBlock(off_t, bool total = false);
void			SetIcon(BBitmap*);
void			SetInfo(char*, off_t, bool reset_block = true);
void			SetOffset(off_t, off_t);
};
#endif
