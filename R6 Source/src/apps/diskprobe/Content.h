//--------------------------------------------------------------------
//	
//	Content.h
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef CONTENT_H
#define CONTENT_H

#include <Font.h>
#include <Point.h>
#include <Rect.h>
#include <Region.h>
#include <View.h>

enum	FIELDS			{F_HEX = 0, F_ASCII, F_OTHER, F_ALL};


//====================================================================

class TContentView : public BView {

private:

bool			fOnes;
bool			fDirty;
bool			fReadOnly;
int32			fBase;
int32			fField;
int32			fFontSize;
int32			fLastOffset;
int32			fLength;
int32			fOffset;
int32			fOldField;
int32			fRange;
uchar			*fBlock;

public:

				TContentView(BRect, uchar*, int32, int32, bool, int32); 
virtual	void	Draw(BRect);
virtual void	KeyDown(const char*, int32);
virtual void	FrameResized(float, float);
virtual void	MakeFocus(bool);
virtual void	MessageReceived(BMessage*);
virtual void	MouseDown(BPoint);
void			Activated(bool);
bool			Dirty(void);
void			DrawSelection(BRect, bool, int32 = F_ALL);
void			FillRegion(BRegion*);
void			GetSelection(int32*, int32*);
void			GetSize(int32*, int32*);
int32			LineHeight(void);
void			Scroll(int32);
void			SetBase(int32);
void			SetBlock(uchar*, int32, bool, bool dirty = false);
void			SetDirty(bool);
void			SetFontSize(int32);
void			SetSelection(int32, int32);
void			StrokeRegion(BRegion*);
void			Update(int32);
};
#endif
