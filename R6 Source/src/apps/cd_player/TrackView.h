//--------------------------------------------------------------------
//	
//	TrackView.h
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef TRACK_VIEW_H
#define TRACK_VIEW_H
#ifndef SAVE_WINDOW_H
#include <SaveWindow.h>
#endif
#ifndef EDIT_TEXT_VIEW_H
#include <EditTextView.h>
#endif

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _LIST_H
#include <List.h>
#endif
#ifndef _LOOPER_H
#include <Looper.h>
#endif
#ifndef _RECT_H
#include <Rect.h>
#endif
#ifndef _TEXT_VIEW_H
#include <TextView.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#include <scsi.h>

#define HEADER		 21
#define TRAILER		 17
#define MARGIN		 62
#define CELL_HEIGHT	 16
#define CELL_WIDTH	455
#define TRACK_POS	(MARGIN + 5)
#define TITLE_POS	(MARGIN + 26)
#define TIME_POS	(MARGIN + 405)


//====================================================================

class TTrackView : public BView {

private:
int				fCDID;
int32			fField;
int32			fTracks;
BList			*fList;
scsi_toc		*fTOC;

public:
int32			fTrack;
BTextView		*fTextView;
BWindow			*fWindow;

				TTrackView(BRect, char*, scsi_toc*, BList*, BWindow*, int); 
				~TTrackView(void);
virtual	void	AttachedToWindow(void);
virtual	void	Draw(BRect);
virtual void	MouseDown(BPoint);
virtual void	Pulse(void);
void			DrawTrack(int32, bool);
void			KillEditField(bool);
void			NextField(bool);
void			OpenEditField(int32);
};
#endif
