//--------------------------------------------------------------------
//	
//	IconView.h
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ICON_VIEW_H
#define ICON_VIEW_H
#ifndef ICON_WINDOW_H
#include "IconWindow.h"
#endif

#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _FILE_H
#include <File.h>
#endif
#ifndef _FILE_PANEL_H
#include <FilePanel.h>
#endif
#ifndef _FONT_H
#include <Font.h>
#endif
#ifndef _RESOURCES_H
#include <Resources.h>
#endif
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _POINT_H
#include <Point.h>
#endif
#ifndef _RECT_H
#include <Rect.h>
#endif
#ifndef _SCROLL_VIEW_H
#include <ScrollView.h>
#endif
#include <TextControl.h>
#ifndef _TEXT_VIEW_H
#include <TextView.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif

enum	VIEWMESSAGES	{M_ICON_DROPPED = 512, M_CLOSE_TEXT_VIEW};

#define	WINDBORDER		3	// in pixels

#define	FATBITS			8	// in pixels
#define	LARGEICON		32	// icon size in pixles
#define	SMALLICON		16	// small icon size in pixels
#define	ICONBORDER		2	// in pixels

#define FAT32LEFT		3
#define FAT32TOP		3
#define FAT32RIGHT		FAT32LEFT + (LARGEICON * FATBITS) + 8
#define	FAT32BOTTOM		FAT32TOP + (LARGEICON * FATBITS) + 8

#define	LIST32LEFT		FAT32RIGHT + 4
#define LIST32TOP		FAT32TOP
#define	LIST32RIGHT		LIST32LEFT + LARGEICON + 9
#define LIST32BOTTOM	LIST32TOP + LARGEICON + 9
#define LISTOFFSET		33

#define	ICON32LEFT		FAT32RIGHT - (2 * (LARGEICON + 4) + 5)
#define ICON32TOP		FAT32BOTTOM + 7
#define	ICON32RIGHT		ICON32LEFT + LARGEICON + 3
#define ICON32BOTTOM	ICON32TOP + LARGEICON + 3

#define	HILITE32LEFT	ICON32RIGHT + 4
#define	HILITE32TOP		ICON32TOP
#define HILITE32RIGHT	HILITE32LEFT + LARGEICON + 3
#define HILITE32BOTTOM	ICON32BOTTOM

#define FAT16LEFT		FAT32LEFT
#define FAT16TOP		FAT32BOTTOM + 4
#define FAT16RIGHT		FAT16LEFT + (SMALLICON * FATBITS) + 8
#define FAT16BOTTOM		FAT16TOP + (SMALLICON * FATBITS) + 8

#define	ICON16LEFT		FAT16RIGHT + 4
#define ICON16TOP		FAT16TOP + 3
#define	ICON16RIGHT		ICON16LEFT + SMALLICON + 3
#define ICON16BOTTOM	ICON16TOP + SMALLICON + 3

#define	HILITE16LEFT	FAT16RIGHT + 4
#define	HILITE16TOP		ICON16BOTTOM + 4
#define HILITE16RIGHT	HILITE16LEFT + SMALLICON + 3
#define HILITE16BOTTOM	HILITE16TOP + SMALLICON + 3

typedef struct {
	char 	mimeSignature[255];
	long	id;
	BBitmap	*icon32;
	BBitmap	*icon16;
} icon_info;

class TEditTextView;


//====================================================================

class TIconView : public BView {

public:
#ifdef APPI_EDIT
bool			fCreateAppi;
#endif

bool			fDirty;
bool			fMarqueeFlag;
bool			fUndo16;
bool			fUndo16Marquee;
bool			fUndo32;
bool			fUndo32Marquee;
short			fEditField;
short			fIconCount;
short			fSelectedIcon;
short			fUndoTool;
long			fPenMode;
long			fPenSize;
long			fResult;
long			fTextField;
long			fHiliteColor[256];
ulong			fAppFlags;
ulong			fSignature;
ulong			fType;
BBitmap			*fDrag16Icon;
BBitmap			*fEdit16Icon;
BBitmap			*fFatBitScreen16;
BBitmap			*fHilite16Icon;
BBitmap			*fOffScreen16;
BBitmap			*fUndo16Drag;
BBitmap			*fUndo16Icon;
BBitmap			*fDrag32Icon;
BBitmap			*fEdit32Icon;
BBitmap			*fFatBitScreen32;
BBitmap			*fHilite32Icon;
BBitmap			*fOffScreen32;
BBitmap			*fUndo32Drag;
BBitmap			*fUndo32Icon;
BList			*fIconList;
BRect			fDragRect;
BRect			fMarqueeRect;
BRect			fUndoDragRect;
BRect			fUndoMarqueeRect;
BFile			*fFile;
BScrollView		*fScrollView;
BView			*fOffView16;
BView			*fFatBitView16;
BView			*fOffView32;
BView			*fFatBitView32;
TEditTextView	        *fTextView;
entry_ref		*fRef;

				TIconView(BRect, entry_ref*);
				~TIconView(void); 
virtual void	AttachedToWindow(void);
virtual	void	Draw(BRect);
virtual void	KeyDown(const char*, int32);
virtual void	MessageReceived(BMessage*);
virtual void	MouseDown(BPoint);
				void	DoSetCursor(BPoint);
virtual void	MouseMoved(BPoint, ulong, const BMessage*);
virtual void	Pulse(void);
void			Clear(void);
void			CloseTextView(bool);
void			Copy(void);
void			Cut(void);
void			DeleteIcon(void);
void			DrawIcon(BRect, short);
void			DrawSignature(short);
void			IconDropped(BMessage*);
void			HandleIconDrop(BMessage *msg);
void			MakeComposite(short, void*);
void			NewIcon(void);
long			OpenFile(entry_ref*);
void			Paste(void);
void			PrepareUndo(short);
bool			SaveChanges(void);
void			RemoveAllIcons(void);
long			SaveFile(void);
void			SaveRequested(BMessage*);
void			SelectAll(void);
void			SetEditField(short);
void			SetMode(long);
void			SetPen(long);
void			SetSelected(short);
void			TrackMouse(short, ulong, BPoint, BRect);
void			Undo(void);
};

//====================================================================

class TEditTextView : public BTextView {
public:
				TEditTextView(BRect, BRect);
virtual bool	AcceptsChar(ulong) const;
};
#endif
