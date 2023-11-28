// ===========================================================================
//	MTextUndoer.h					©1995 Metrowerks Inc. All rights reserved.
// ===========================================================================

#ifndef _MTEXTUNDOER_H
#define _MTEXTUNDOER_H

#include "MList.h"

class MIDETextView;
class String;
class BMenuItem;


class	MTextUndoer
{
public:
								MTextUndoer(
									MIDETextView&	inTextView);
	virtual						~MTextUndoer();
	
	virtual void				Redo();
	virtual void				Undo();
	
	virtual bool				CanRedo() const;
	virtual bool				CanUndo() const;
	
	bool						IsDone() const { return fIsDone; }

	virtual void				AdjustUndoMenuItem(
									BMenuItem&	inMenuItem);
	virtual void				SetUndoText(
									BMenuItem&	inMenuItem);
	virtual void				SetRedoText(
									BMenuItem&	inMenuItem);

protected:

	MIDETextView&		fTextView;
	bool				fIsDone;

	char*				fDeletedText;
	int32				fDeletedTextLen;
	int32				fSelStart;
	int32				fSelEnd;
	
	virtual void				UndoSelf();
	virtual void				RedoSelf() = 0;

	virtual void				AppendItemName(
									String& inName) = 0;
};

// ---------------------------------------------------------------------------

class	MCutUndoer : public MTextUndoer {
public:
								MCutUndoer(
									MIDETextView&	inTextView);
	virtual						~MCutUndoer() { }

protected:

	virtual void				RedoSelf();
	virtual void				AppendItemName(String& inName);
};

// ---------------------------------------------------------------------------

class	MPasteUndoer : public MTextUndoer {
public:
								MPasteUndoer(
									MIDETextView&	inTextView);
	virtual						~MPasteUndoer();

protected:

	char*				fPastedText;
	ssize_t				fPastedTextLen;

	virtual void				RedoSelf();
	virtual void				UndoSelf();
	virtual void				AppendItemName(String& inName);
};

// ---------------------------------------------------------------------------

class	MDragUndoer : public MTextUndoer {
public:
								MDragUndoer(
									MIDETextView&	inTextView,
									const char*		inInsertedText,
									int32			inInsertedTextLen,
									int32			inDropOffset,
									bool			inSameWindow);
	virtual						~MDragUndoer();

protected:

	char*				fInsertedText;
	int32				fInsertedTextLen;
	int32				fDropOffset;
	bool				fSameWindowDrag;

	virtual void				RedoSelf();
	virtual void				UndoSelf();
	virtual void				AppendItemName(String& inName);
};


// ---------------------------------------------------------------------------

class	MClearUndoer : public MTextUndoer {
public:
								MClearUndoer(
									MIDETextView&	inTextView,
									int32			inRealSelEnd = -1);
	virtual						~MClearUndoer() { }

protected:

	int32				fRealSelEnd;

	virtual void				UndoSelf();
	virtual void				RedoSelf();
	virtual void				AppendItemName(String& inName);
};


// ---------------------------------------------------------------------------

class	MTypingUndoer : public MTextUndoer {
public:
								MTypingUndoer(
									MIDETextView&	inTextView);
	virtual						~MTypingUndoer();
	
	virtual void				Reset();
	virtual void				InputACharacter(
									int32	inGlyphWidth = 1);
	virtual void				InputCharacters(
									int32	inHowMany);
	virtual void				BackwardErase();
	virtual void				ForwardErase();
	virtual void				Replace(
									const char *	inString);
	bool						SelectionChanged();

protected:

	char*				fTypedText;
	int32				fTypingStart;
	int32				fTypingEnd;
	int32				fUndoCount;

	virtual void				RedoSelf();
	virtual void				UndoSelf();
	virtual void				AppendItemName(String& inName);
};

// ---------------------------------------------------------------------------

class	MIndentUndoer : public MTextUndoer {
public:
								MIndentUndoer(
									MIDETextView&	inTextView,
									bool			inIndentRight);

	virtual						~MIndentUndoer();

protected:

	char*				fShiftedText;
	int32				fShiftedTextLen;
	int32				fNewSelStart;
	int32				fNewSelEnd;
	bool				fIndentToRight;

	virtual void				UndoSelf();
	virtual void				RedoSelf();
	virtual void				AppendItemName(String& inName);
};

// ---------------------------------------------------------------------------

class	MInsertUndoer : public MTextUndoer {
public:
								MInsertUndoer(
									MIDETextView&	inTextView,
									const char *	inInsertedText,
									ssize_t			inTextLength);

	virtual						~MInsertUndoer();

protected:

	char*				fInsertedText;
	ssize_t				fInsertedTextLen;

	virtual void				UndoSelf();
	virtual void				RedoSelf();
	virtual void				AppendItemName(String& inName);
};

// ---------------------------------------------------------------------------

class	MAddOnUndoer : public MTextUndoer {
public:
								MAddOnUndoer(
									MIDETextView&	inTextView);

	virtual						~MAddOnUndoer();

	void						PushUndoer(
									MTextUndoer*	inUndoer);
	int32						ActionCount()
								{
									return fUndoList.CountItems();
								}		
protected:

	MList<MTextUndoer*>		fUndoList;

	virtual void				UndoSelf();
	virtual void				RedoSelf();
	virtual void				AppendItemName(String& inName);
};

#endif
