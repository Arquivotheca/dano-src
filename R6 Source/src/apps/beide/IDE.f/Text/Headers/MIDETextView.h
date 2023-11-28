//==================================================================
//	MIDETextView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MIDETEXTVIEW_H
#define _MIDETEXTVIEW_H

#include "STEngine.h"
#include "MList.h"
#include "MSyntaxStyler.h"
#include "MPrefsStruct.h"
#include "IDEConstants.h"

class MTextUndoer;
class MTypingUndoer;
class MAddOnUndoer;

class MIDETextView : public STEngine
{
	friend class MIndentUndoer;		// needs access to shift self methods
	friend class MSyntaxStyler;		// needs access to STEngine protected methods
	friend class CursorCaretState;	// needs access to STEngine protected methods

public:
								MIDETextView(
									const BRect & 	area,
									SuffixType		inSuffix);
								~MIDETextView();

	virtual void				MessageReceived(
									BMessage *message);
	virtual	void				KeyDown(
									const char *bytes, 
									int32 		numBytes);
	virtual void				Cut();
	virtual void				Copy();
	virtual	void				Paste();
	virtual	void				Clear();
	virtual void				Insert(
									const char *			inText, 
									int32 					inLength,
							   		ConstSTEStyleRangePtr 	inStyles = NULL);
	virtual void					Delete();
	void						Print();
	void						StartAddon();
	void						StopAddon();
	void						AddonInsert(
									const char* 	inText, 
									int32 			inLength);
	void						AddonDelete();

	void						BalanceWhileTyping(
									BMessage&	inMessage);

	virtual	bool				IsWordBreakChar(
									uchar inChar);

	void						Replace(
									const char * inString);

	void						AdjustUndoMenuItem(
									BMenuItem&	inMenuItem);
	void						AdjustUndoMenuItems(
									BMenuItem&	inUndoItem,
									BMenuItem&	inRedoItem);
	void						Undo();
	void						Redo();
	void						ClearUndo();
	void						ClearAllUndo();
	void						ShiftRight();
	void						ShiftLeft();
	void						DocBalance();

	void						ScrollToFunction(
									int32 	inSelStart, 
									int32 	inSelEnd);
	int32						LineStart(
									int32	inOffset);

	void						ParseText();
	void						SetSuffixType(
									SuffixType		inSuffix);
	void						SetColorSpace(
									color_space 	inColorSpace);
	void						UpdateSyntaxStyleInfo(
									const SyntaxStylePrefs&	inPrefs,
									float	inTabSize);
	void						UseSyntaxColoring(
									bool inUseIt = true);
	bool						UsesSyntaxColoring()
								{
									return fUsesSyntaxColoring;
								}

	// MTextWindow is in charge of saving the document
	// tell us when it happens please
	void						DocumentSaved();

// BTextView has em and STE don't
	int32						IndexAtPoint(float h, float v)
								{
									return PointToOffset(BPoint(h, v));
								}
	void						SetAutoindent(
									bool	inAutoIndent);
	bool						HasSelection();
	void						UpdateFontInfo(
									const font_family	inFontFamily,
									const font_style	inFontStyle,
									float				inFontSize,
									float				inTabSize,
									bool				inDoAutoIndent);
	enum	ScrollType
	{
		ScrollTop,
		ScrollMiddle
	};

	virtual void				ScrollToOffset(
									long 		inOffset)
								{
									STEngine::ScrollToOffset(inOffset);	
								}
	virtual void				ScrollToOffset(
									long 		inOffset,
									ScrollType 	inType);
// BTextView has em and STE don't

	void						SetDirty(
									bool	inDirty = true)
								{
									fDirty = inDirty;
								}
	bool						IsDirty()
								{
									return fDirty;
								}
	bool						UsesMultipleUndo()
								{
									return fUsingMultipleUndo;
								}
	static void					Init();

protected:

	MTextUndoer*				fUndoer;
	MTypingUndoer*				fTypingUndoer;
	MAddOnUndoer*				fAddOnUndoer;
	MList<MTextUndoer*>			fUndoList;
	int32						fUndoTop;
	int32						fUndoCurrent;
	int32						fUndoSaveMark;
	bool						fUsingMultipleUndo;
	int32						fAnchor;
	int32						fAnchorSelStart;
	int32						fAnchorSelEnd;
	int32						fAnchorWidth;
	bigtime_t					fFlashingDelay;		// units are microseconds
	bool						fFlashWhenTyping;
	bool						fDirty;
	bool						fAutoIndent;
	bool						fUsesSyntaxColoring;
	MSyntaxStyler				fStyler;

	int32						FindLineStart(
									int32	inIndex);
	int32						FindLineEnd(
									int32	inIndex);
	bool						GetNextSelectedLineStart(
									int32	&inStart, 
									int32	inEnd);
	void						BuildSelectionsLineStarts(
									MList<int32>& outLineStarts);

	bool						MessageDropped(
									BMessage	*inMessage,
									BPoint 		where,
									BPoint		offset);

	void						BuildTypingTask();
	void						ClearTypingTask();
	void						PushUndoer();

	void						ShiftRightSelf();
	void						ShiftLeftSelf();

	void						DoSpecialLeftRightKey(
									CommandT	inCommand);
	void						DoSpecialUpDownKey(
									CommandT	inCommand);
	void						SpecialClear(
									int32	inEndOffset);

	bool						Isletter(
									short c);

	const char *				NE_PrevLine(
									const char *text_pos, 
									const char *buffer_start);
	const char *				NE_NextLineMono(
									const char *text_pos, 
									const char *limit);

	const char *				NE_PixelToText(
									const char *	text, 
									float 			xpos);

	int32						FindPageTop(
									int32	inSelStart,
									int32	inSelEnd);

	int32						FindPageBottom(
									int32	inSelStart,
									int32	inSelEnd);

	int32						AdjustOffsetForFunctionComments(
									int32	inOffset);

	virtual void				HandleModification();
	virtual void				HandleAlphaKey(
									const char *	inBytes, 
									int32 			inNumBytes);
	virtual void				HandleBackspace();
	virtual void				HandleDelete();

	enum AccoladeType
	{
		ACCOLADES, SQUAREBRACKETS, ROUNDBRACKETS
	};

	void						FlashPreviousAccolade(AccoladeType mode);
	bool						InCommentOrString(const char* text, int32 position);
	int32						FindOpeningMatch(const char* text, int32 pos);
	int32						FindClosingMatch(const char* text, int32 pos);
	
	static BRect				GetTextRect(const BRect & area);
};

#endif
