// ============================================================
//  STEngine.h	©1996 Hiroshi Lockheimer
// ============================================================
// 	STE Version 1.0a5

#ifndef _STENGINE_H
#define _STENGINE_H

#include <View.h>
#include "STE.h"
#include "STESupport.h"


class STEngine : public BView {
public:
						STEngine(BRect frame, const char *name, BRect textRect,
						 	 	 ConstSTEStylePtr nullStyle, uint32 resizeMask = B_FOLLOW_ALL_SIDES, 
							 	 uint32 flags = B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS | B_NAVIGABLE);
	virtual				~STEngine();

	static void			Setup();

	virtual void		AttachedToWindow();
	virtual void		Draw(BRect inRect);
	virtual	void		MouseDown(BPoint where);
	virtual	void		MouseMoved(BPoint where, uint32 code, 
							   	   const BMessage *message);
	virtual	void		WindowActivated(bool state);
	virtual	void		KeyDown(const char *bytes, int32 numBytes);
	virtual	void		Pulse();
	virtual	void		FrameResized(float width, float height);
	virtual	void		MakeFocus(bool focusState = TRUE);
	virtual void		MessageReceived(BMessage *message);
	
	void				SetText(const char* inText, int32 inLength,
								ConstSTEStyleRangePtr inStyles = NULL);
	virtual void		Insert(const char *inText, int32 inLength,
							   ConstSTEStyleRangePtr inStyles = NULL);
	virtual void		Delete();
	
	const char*			Text();
	int32				TextLength() const;
	void				GetText(char *buffer, int32 offset, int32 length) const;
	char				ByteAt(int32 offset) const;
	int32				GlyphWidth(int32 inOffset) const;

	int32				CountLines() const;
	int32				CurrentLine() const;
	void				GoToLine(int32 lineNum);
	
	virtual void		Cut();
	virtual void		Copy();
	virtual void		Paste();
	virtual void		Clear();
	void				SelectAll();

	virtual bool		CanPaste();
	virtual bool		CanDrop(const BMessage *inMessage);
			
	virtual void		Select(int32 startOffset, int32 endOffset);
	void				GetSelection(int32 *outStart, int32 *outEnd) const;

	void				SetStyle(uint32 inMode, ConstSTEStylePtr inStyle);
	void				GetStyle(int32 inOffset, STEStylePtr outStyle) const;
	void				SetStyleRange(int32 startOffset, int32 endOffset,
									  ConstSTEStyleRangePtr inStyles, bool inRefresh = TRUE);
	STEStyleRangePtr	GetStyleRange(int32 startOffset, int32 endOffset,
									  int32 *outLength = NULL) const;
	bool				IsContinuousStyle(uint32 *ioMode, STEStylePtr outStyle) const;
	
	int32				OffsetToLine(int32 offset) const;
	int32				PixelToLine(float pixel) const;
	int32				OffsetAt(int32 line) const;
	BPoint				OffsetToPoint(int32 inOffset, float *outHeight = NULL);
	int32				PointToOffset(BPoint point); 
	
	void				FindWord(int32 inOffset, int32 *outFromOffset, 
							 	 int32 *outToOffset);
	
	float				GetHeight(int32 startLine, int32 endLine);
	
	void				GetHiliteRegion(int32 startOffset, int32 endOffset,
										BRegion *outRegion);
	void				InsetRegion(
							BRegion&	inoutRegion) const;
	
	virtual void		ScrollToOffset(int32 inOffset);
	void				ScrollOffsetIntoView(int32 inOffset);
	void				ScrollToSelection();

	void				SetTextRect(BRect rect);
	BRect				TextRect() const;
	void				SetTabWidth(float width);
	float				TabWidth() const;
	void				MakeSelectable(bool selectable = TRUE);
	bool				IsSelectable() const;
	void				MakeEditable(bool editable = TRUE);
	bool				IsEditable() const;
	void				SetWordWrap(bool wrap);
	bool				DoesWordWrap() const;
	void				UseOffscreen(color_space colors);
	void				DontUseOffscreen();
	bool				DoesUseOffscreen() const;
	
protected:
	virtual void		HandleBackspace();
	virtual void		HandleArrowKey(uint32 inArrowKey);
	virtual void		HandleDelete();
	virtual void		HandlePageKey(uint32 inPageKey);
	virtual void		HandleAlphaKey(const char *	inBytes, int32 inNumBytes);
	
	void				InsertAt(const char *inText, int32 inLength,
							   	 int32 inOffset, ConstSTEStyleRangePtr inStyles = NULL);
	void				RemoveRange(int32 fromOffset, int32 toOffset);
	
	void				Refresh(int32 fromOffset, int32 toOffset, 
								bool erase, bool scroll);
	
	void				RecalLineBreaks(int32 *startLine, int32 *endLine);
	virtual int32		FindLineBreak(int32 fromOffset, float *outAscent, 
								  	  float *outDescent, float width);

	void				HideSelection();
	void				ShowSelection();
	
	virtual bool		IsWordBreakChar(uchar inChar);	
	/*virtual*/ bool		IsLineBreakChar(uchar inChar);
	
	float				StyledWidth(int32 fromOffset, int32 length,
									float *outAscent = NULL, float *outDescent = NULL);
	float				ActualTabWidth(float location);
							
	void				DrawLines(int32 startLine, int32 endLine, 
							  	  int32 startOffset = -1, bool erase = FALSE);
	void				DrawSelection(int32 startOffset, int32 endOffset);	
	void				DrawCaret(int32 offset);
	void				InvertCaret();
	void				DragCaret(int32 offset);
		
	virtual void		TrackDrag(BPoint where);
	virtual void		InitiateDrag();
	virtual	bool		MessageDropped(BMessage *inMessage, BPoint where,
									   BPoint offset);
	bool				WaitMouseUp(BPoint inPoint);

	virtual void		UpdateScrollbars();
		
	virtual void		Activate();
	virtual void		Deactivate();
	
	virtual void		HandleModification();
	
protected:
	STETextBuffer			mText;
	STELineBuffer			mLines;
	STEStyleBuffer			mStyles;
	BRect					mTextRect;
	int32					mSelStart;
	int32					mSelEnd;
	bool					mCaretVisible;
	bigtime_t				mCaretTime;
	int32					mClickOffset;
	int32					mClickCount;
	bigtime_t				mClickTime;
	int32					mDragOffset;
	bool					mDragOwner;
	bool					mActive;
	float					mTabWidth;
	bool					mSelectable;
	bool					mEditable;
	bool					mWrap;
	BBitmap* 				mOffscreen;

	static STEWidthBuffer	sWidths;

private:
	void				SetMouse(BPoint where);
};	

// ------------------------------------------------------------
// 	IsEditable
// ------------------------------------------------------------
// Return whether the text can be edited

inline bool
STEngine::IsEditable() const
{
	return (mEditable);
}

#endif
