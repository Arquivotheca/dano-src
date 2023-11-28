// ===========================================================================
//	MTextUndoer.cpp				©1995 Metrowerks Inc. All rights reserved.
// ===========================================================================
//	This code is based on the PP LAction/LUndoer/LTETextAction classes.
//	It has been modified somewhat to remove Mac and TextEdit dependendencies.
//	These classes don't deal properly with the clipboard.  The Mac IDE also
//	doesn't deal correctly with the clipboard.  When any edit action changes
//	the clipboard its contents should be saved and later restored on an undo.
//	For example Cut modifies the clipboard.  An undo-cut should restore the
//	clipboard's contents.
//	BDS

#include <string.h>

#include "MTextUndoer.h"
#include "MIDETextView.h"
#include "IDEMessages.h"
#include "CString.h"
#include "Utils.h"
#include "ProjectCommands.h"

#include <Clipboard.h>
#include <MenuItem.h>

// ---------------------------------------------------------------------------
//		MTextUndoer
// ---------------------------------------------------------------------------
//	Constructor

MTextUndoer::MTextUndoer(
	MIDETextView&	inTextView) :
	fTextView(inTextView)
{
	// Save current selection range and the selected text
	fTextView.GetSelection(&fSelStart, &fSelEnd);
	fDeletedTextLen = fSelEnd - fSelStart;
	fDeletedText = new char[fDeletedTextLen];
	memcpy(fDeletedText, fSelStart + fTextView.Text(), fDeletedTextLen);
	fIsDone = true;
}

// ---------------------------------------------------------------------------
//		~MTextUndoer
// ---------------------------------------------------------------------------
//	Destructor

MTextUndoer::~MTextUndoer()
{
	delete[] fDeletedText;
}

// ---------------------------------------------------------------------------
//		AdjustUndoMenuItem
// ---------------------------------------------------------------------------
//	We Adjust the menu item here by setting the label and enabling it.
//	This assumes no multiple undo.

void
MTextUndoer::AdjustUndoMenuItem(
	BMenuItem&	inMenuItem)
{
	String		label;

	if (IsDone())
	{
		label = "Undo ";
		SetCommand(inMenuItem, cmd_Undo);
	}
	else
	{
		label = "Redo ";
		SetCommand(inMenuItem, cmd_Redo);
	}

	AppendItemName(label);

	inMenuItem.SetLabel(label);
	inMenuItem.SetEnabled(true);
}

// ---------------------------------------------------------------------------
//		SetUndoText
// ---------------------------------------------------------------------------
//	We Adjust the menu item here by setting the label and enabling it.
//	This assumes multiple undo.

void
MTextUndoer::SetUndoText(
	BMenuItem&	inUndoItem)
{
	String		label = "Undo ";

	AppendItemName(label);

	inUndoItem.SetLabel(label);
	inUndoItem.SetEnabled(true);
}

// ---------------------------------------------------------------------------
//		SetRedoText
// ---------------------------------------------------------------------------
//	We Adjust the menu item here by setting the label and enabling it.
//	This assumes multiple undo.

void
MTextUndoer::SetRedoText(
	BMenuItem&	inRedoItem)
{
	String		label = "Redo ";

	AppendItemName(label);

	inRedoItem.SetLabel(label);
	inRedoItem.SetEnabled(true);
}

// ---------------------------------------------------------------------------
//		Redo
// ---------------------------------------------------------------------------

void
MTextUndoer::Redo()
{
	if (CanRedo()) {
		RedoSelf();
	}
	
	fIsDone = true;
}

// ---------------------------------------------------------------------------
//		Undo
// ---------------------------------------------------------------------------

void
MTextUndoer::Undo()
{
	if (CanUndo()) {
		UndoSelf();
	}
		
	fIsDone = false;
}

// ---------------------------------------------------------------------------
//		CanRedo
// ---------------------------------------------------------------------------

bool
MTextUndoer::CanRedo() const
{
	return (!IsDone());
}

// ---------------------------------------------------------------------------
//		CanUndo
// ---------------------------------------------------------------------------

bool
MTextUndoer::CanUndo() const
{
	return (IsDone());
}

// ---------------------------------------------------------------------------
//		UndoSelf
// ---------------------------------------------------------------------------
//	This function is used by several of the derived classes for undo.

void
MTextUndoer::UndoSelf()
{
	// Restore deleted text
	fTextView.Select(fSelStart, fSelStart);
	fTextView.Insert(fDeletedText, fDeletedTextLen);

	// Restore original selection
	fTextView.Select(fSelStart, fSelEnd);
}


// ===========================================================================
//	MCutUndoer
// ===========================================================================

// ---------------------------------------------------------------------------
//		MCutUndoer
// ---------------------------------------------------------------------------
//	Constructor

MCutUndoer::MCutUndoer(
	MIDETextView&	inTextView)
		: MTextUndoer(inTextView)
{
}

// ---------------------------------------------------------------------------
//		RedoSelf
// ---------------------------------------------------------------------------

void
MCutUndoer::RedoSelf()
{
	// Delete selected text
	fTextView.Select(fSelStart, fSelEnd);
	fTextView.Delete();

	// Put deleted text on clipboard
	if (be_clipboard->Lock())
	{
		be_clipboard->Clear();
		be_clipboard->Data()->AddData(kTextPlain, B_MIME_TYPE, fDeletedText, fDeletedTextLen);
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
}

// ---------------------------------------------------------------------------
//		AppendItemName
// ---------------------------------------------------------------------------

void
MCutUndoer::AppendItemName(
	String& inName)
{
	inName += "Cut";
}


// ===========================================================================
//	MPasteUndoer
// ===========================================================================

// ---------------------------------------------------------------------------
//		MPasteUndoer
// ---------------------------------------------------------------------------
//	Constructor

MPasteUndoer::MPasteUndoer(
	MIDETextView&	inTextView)
		: MTextUndoer(inTextView)
{
	fPastedText = nil;
	fPastedTextLen = 0;

	// Save the text from the clipboard
	if (be_clipboard->Lock())
	{
		const char*		clipText;
	
		if (B_NO_ERROR == be_clipboard->Data()->FindData(kTextPlain, B_MIME_TYPE, (const void**) &clipText, &fPastedTextLen))
		{
			fPastedText = new char[fPastedTextLen];
			memcpy(fPastedText, clipText, fPastedTextLen);
		}
	
		be_clipboard->Unlock();
	}
}

// ---------------------------------------------------------------------------
//		~MPasteUndoer
// ---------------------------------------------------------------------------
//	Destructor

MPasteUndoer::~MPasteUndoer()
{
	delete[] fPastedText;
}

// ---------------------------------------------------------------------------
//		RedoSelf
// ---------------------------------------------------------------------------

void
MPasteUndoer::RedoSelf()
{
	// Delete selected text
	fTextView.Select(fSelStart, fSelEnd);
	fTextView.Delete();
	
	// Restore the text that was on the clipboard
	fTextView.Insert(fPastedText, fPastedTextLen);
	fTextView.Select(fSelStart, fSelStart + fPastedTextLen);
}

// ---------------------------------------------------------------------------
//		UndoSelf
// ---------------------------------------------------------------------------

void
MPasteUndoer::UndoSelf()
{
	// Delete text that was pasted
	fTextView.Select(fSelStart, fSelStart + fPastedTextLen);
	fTextView.Delete();
	
	// Restore text deleted by the paste
	fTextView.Insert(fDeletedText, fDeletedTextLen);

	// Restore selection
	fTextView.Select(fSelStart, fSelEnd);
}

// ---------------------------------------------------------------------------
//		AppendItemName
// ---------------------------------------------------------------------------

void
MPasteUndoer::AppendItemName(
	String& inName)
{
	inName += "Paste";
}

// ===========================================================================
//	MDragUndoer
// ===========================================================================

// ---------------------------------------------------------------------------
//		MDragUndoer
// ---------------------------------------------------------------------------
//	Constructor

MDragUndoer::MDragUndoer(
	MIDETextView&	inTextView,
	const char*		inInsertedText,
	int32			inInsertedTextLen,
	int32			inDropOffset,
	bool			inSameWindow)
		: MTextUndoer(inTextView),
		fInsertedTextLen(inInsertedTextLen),
		fDropOffset(inDropOffset),
		fSameWindowDrag(inSameWindow)
{
	ASSERT(inInsertedTextLen > 0);

	// Save the text from the drag message
	if (inInsertedText)
	{
		fInsertedText = new char[fInsertedTextLen];
		memcpy(fInsertedText, inInsertedText, fInsertedTextLen);
	}
}

// ---------------------------------------------------------------------------
//		~MDragUndoer
// ---------------------------------------------------------------------------
//	Destructor

MDragUndoer::~MDragUndoer()
{
	delete[] fInsertedText;
}

// ---------------------------------------------------------------------------
//		RedoSelf
// ---------------------------------------------------------------------------

void
MDragUndoer::RedoSelf()
{
	int32		dropOffset = fDropOffset;

	// Delete the text that was moved
	if (fSameWindowDrag)
	{
		fTextView.Select(fSelStart, fSelEnd);
		fTextView.Delete();

		// Adjust the offset if it comes after the selection
		if (dropOffset > fSelEnd)
			dropOffset -= fSelEnd - fSelStart;
	}

	// Restore the text that was dropped
	fTextView.Select(dropOffset, dropOffset);
	fTextView.Insert(fInsertedText, fInsertedTextLen);
	fTextView.Select(dropOffset, dropOffset + fInsertedTextLen);
}

// ---------------------------------------------------------------------------
//		UndoSelf
// ---------------------------------------------------------------------------

void
MDragUndoer::UndoSelf()
{
	int32		dropOffset = fDropOffset;

	if (fSameWindowDrag)
	{
		// Adjust the offset if it comes after the selection
		if (dropOffset > fSelEnd)
			dropOffset -= fSelEnd - fSelStart;
	}

	// Delete text that was dropped
	fTextView.Select(dropOffset, dropOffset + fInsertedTextLen);
	fTextView.Delete();
	
	// Restore the text that was moved
	if (fSameWindowDrag)
	{
		fTextView.Select(fSelStart, fSelStart);
		fTextView.Insert(fInsertedText, fInsertedTextLen);
	}

	// Restore selection
	fTextView.Select(fSelStart, fSelEnd);
}

// ---------------------------------------------------------------------------
//		AppendItemName
// ---------------------------------------------------------------------------

void
MDragUndoer::AppendItemName(
	String& inName)
{
	inName += "Drag";
}

// ===========================================================================
//	MClearUndoer
// ===========================================================================
//	Supports normal clears and also delete to end of line and delete to 
//	end of file.  These special clears require saving the real selection
//	end so it can be restored since the selection end is modified to do the
//	clear.

// ---------------------------------------------------------------------------
//		MClearUndoer
// ---------------------------------------------------------------------------
//	Constructor

MClearUndoer::MClearUndoer(
	MIDETextView&	inTextView,
	int32			inSelEnd)
		: MTextUndoer(inTextView), fRealSelEnd(inSelEnd)
{
}

// ---------------------------------------------------------------------------
//		UndoSelf
// ---------------------------------------------------------------------------
//	This function is used by several of the derived classes for undo.

void
MClearUndoer::UndoSelf()
{
	// Restore deleted text
	fTextView.Select(fSelStart, fSelStart);
	fTextView.Insert(fDeletedText, fDeletedTextLen);

	// Restore original selection
	int32		selEnd = fRealSelEnd >= 0 ? fRealSelEnd : fSelEnd;

	fTextView.Select(fSelStart, selEnd);
}

// ---------------------------------------------------------------------------
//		RedoSelf
// ---------------------------------------------------------------------------

void
MClearUndoer::RedoSelf()
{
	// Delete text that was uncleared
	fTextView.Select(fSelStart, fSelEnd);
	fTextView.Delete();
}

// ---------------------------------------------------------------------------
//		AppendItemName
// ---------------------------------------------------------------------------

void
MClearUndoer::AppendItemName(
	String& inName)
{
	inName += "Clear";
}


// ===========================================================================
//	MTypingUndoer
// ===========================================================================

// ---------------------------------------------------------------------------
//		MTypingUndoer
// ---------------------------------------------------------------------------
//	Constructor

MTypingUndoer::MTypingUndoer(
	MIDETextView&	inTextView)
		: MTextUndoer(inTextView)
{
	fTypedText = nil;	
	fTypingStart = fTypingEnd = fSelStart;
}

// ---------------------------------------------------------------------------
//		~MTypingUndoer
// ---------------------------------------------------------------------------
//	Destructor

MTypingUndoer::~MTypingUndoer()
{
	delete[] fTypedText;
}

// ---------------------------------------------------------------------------
//		Reset
// ---------------------------------------------------------------------------
//	Re-initialize state of TypingAction

void
MTypingUndoer::Reset()
{
	delete[] fDeletedText;
	fDeletedText = nil;

	fTextView.GetSelection(&fSelStart, &fSelEnd);
	fDeletedTextLen = fSelEnd - fSelStart;
	fDeletedText = new char[fDeletedTextLen];
	memcpy(fDeletedText, fSelStart + fTextView.Text(), fDeletedTextLen);
	fIsDone = true;

	fTypingStart = fTypingEnd = fSelStart;
	
	delete[] fTypedText;
	fTypedText = nil;
	fUndoCount = 0;
}

// ---------------------------------------------------------------------------
//		SelectionChanged
// ---------------------------------------------------------------------------
//	When doing multiple undo we don't want this typing task to reset because
//	then we lose the previous text.  Instead we check if it would reset and
//	then just create a new typing task.

bool
MTypingUndoer::SelectionChanged()
{
	int32		selStart;
	int32		selEnd;

	fTextView.GetSelection(&selStart, &selEnd);

	return fTypingEnd != selStart || fTypingEnd != selEnd;
}

// ---------------------------------------------------------------------------
//		InputACharacter
// ---------------------------------------------------------------------------
//	Handle an input character typing action before it actually happens.

void
MTypingUndoer::InputACharacter(
	int32	inGlyphWidth)
{
	int32		selStart;
	int32		selEnd;

	fTextView.GetSelection(&selStart, &selEnd);

	if (fTypingEnd != selStart || fTypingEnd != selEnd)
	{
								// Selection has changed. Start a
		Reset();				// fresh typing sequence
	}

	fTypingEnd += inGlyphWidth;
}

// ---------------------------------------------------------------------------
//		InputCharacters
// ---------------------------------------------------------------------------
//	Should be called after more than one characters are added to the text
//	at a time.  Happens when autoindent adds several tabs for instance.

void
MTypingUndoer::InputCharacters(
	int32	inHowMany)
{
	int32		selStart;
	int32		selEnd;
	int32		typingEnd = fTypingEnd + inHowMany;

	fTextView.GetSelection(&selStart, &selEnd);

	if (typingEnd != selStart || typingEnd != selEnd)
	{
								// Selection has changed. Start a
		Reset();				// fresh typing sequence
	}

	fTypingEnd += inHowMany;
}

// ---------------------------------------------------------------------------
//		Replace
// ---------------------------------------------------------------------------
//	Handle a replace action.

void
MTypingUndoer::Replace(
	const char *	inString)
{
	int32		textLen = strlen(inString);

	// Save current typing run
	delete[] fTypedText;
	fTypedText = nil;

	fTypedText = new char[textLen];
	memcpy(fTypedText, inString, textLen);

	fTypingEnd = fTypingStart + textLen;
}

// ---------------------------------------------------------------------------
//		BackwardErase
// ---------------------------------------------------------------------------
//	Handle Backward Delete typing action
//
//	Backward delete erases the current selection if one or more characters
//	is selected. If the selection is a single insertion point, then
//	backward delete erases the one character before the insertion point.
//	The logic here was modified somewhat to fix some bugs - BDS.

void
MTypingUndoer::BackwardErase()
{
	int32		selStart;
	int32		selEnd;

	fTextView.GetSelection(&selStart, &selEnd);

	int32		glyphWidth = fTextView.GlyphWidth(selStart - 1);

	if (fTypingEnd != selStart || fTypingEnd != selEnd)
	{
								// Selection has changed. Start a
		Reset();				// fresh typing sequence
		if (selStart != selEnd) 
		{
			fTypingEnd += glyphWidth;
		}
		else
		{
								// Deleting before beginning of typing
			int32		newDeletedTextLen = fDeletedTextLen + glyphWidth;
			char*		deletedText = new char[newDeletedTextLen];
			memcpy(deletedText + glyphWidth, fDeletedText, fDeletedTextLen);
	
			fDeletedTextLen = newDeletedTextLen;
			fTypingStart = selStart - glyphWidth;
			selStart = fTypingStart;
	
			for (int i = 0; i < glyphWidth; ++i)
				deletedText[i] = fTextView.ByteAt(selStart++);

			delete[] fDeletedText;
			fDeletedText = deletedText;			

		}
	}
	else
	if (fTypingStart >= selStart)
	{
								// Deleting before beginning of typing
		int32		newDeletedTextLen = fDeletedTextLen + glyphWidth;
		char*		deletedText = new char[newDeletedTextLen];
		memcpy(deletedText + glyphWidth, fDeletedText, fDeletedTextLen);

		fDeletedTextLen = newDeletedTextLen;
		fTypingStart = selStart - glyphWidth;
		selStart = fTypingStart;

		for (int i = 0; i < glyphWidth; ++i)
			deletedText[i] = fTextView.ByteAt(selStart++);

		delete[] fDeletedText;
		fDeletedText = deletedText;
	}
	
	fTypingEnd -= glyphWidth;
}

// ---------------------------------------------------------------------------
//		ForwardErase
// ---------------------------------------------------------------------------
//	Handle Forward Delete typing action
//
//	Forward delete erases the current selection if one or more characters
//	is selected. If the selection is a single insertion point, then
//	forward delete erases the one character after the insertion point.

void
MTypingUndoer::ForwardErase()
{
	int32		selStart;
	int32		selEnd;

	fTextView.GetSelection(&selStart, &selEnd);

	int32		glyphWidth = fTextView.GlyphWidth(selStart);

	// the undo count lets us know if the user has done an undo since
	// starting to hit the forward delete key.  Otherwise there
	// is no way to know if the user has hit the forward delete several
	// times and then undo.  If they do this the typing sequence needs
	// to be reset.
	if (fTypingEnd != selStart || fTypingEnd != selEnd || 	fUndoCount > 0)
	{
								// Selection has changed. Start a
		Reset();				// fresh typing sequence

		if (fSelStart == fSelEnd) 
		{
								// Selection is a single insertion point
								// Select next character
			delete[] fDeletedText;
			fDeletedTextLen = glyphWidth;
			fDeletedText = new char[glyphWidth];
			
			for (int i = 0; i < glyphWidth; ++i)
				fDeletedText[i] = fTextView.ByteAt(selStart++);
		}
	}
	else
	{
								// Selection hasn't changed
								// Select next character
		int32		newDeletedTextLen = fDeletedTextLen + glyphWidth;
		char*		deletedText = new char[newDeletedTextLen];
		memcpy(deletedText, fDeletedText, fDeletedTextLen);

		for (int i = fDeletedTextLen; i < newDeletedTextLen; ++i)
			deletedText[i] = fTextView.ByteAt(selStart++);

		delete[] fDeletedText;
		fDeletedText = deletedText;
		fDeletedTextLen = newDeletedTextLen;
	}
}

// ---------------------------------------------------------------------------
//		RedoSelf
// ---------------------------------------------------------------------------
//	Redo a TypingAction by restoring the last typing sequence

void
MTypingUndoer::RedoSelf()
{
	// Delete original text
	fTextView.Select(fTypingStart, fTypingStart + fDeletedTextLen);
	fTextView.Delete();
	
	// Insert typing run
	fTextView.Insert(fTypedText, fTypingEnd - fTypingStart);	

	fUndoCount--;
}

// ---------------------------------------------------------------------------
//		UndoSelf
// ---------------------------------------------------------------------------
//	Undo a TypingAction by restoring the text and selection that
//	existed before the current typing sequence started

void
MTypingUndoer::UndoSelf()
{
	int32		textLen = fTypingEnd - fTypingStart;

	// Save current typing run
	delete[] fTypedText;
	fTypedText = nil;

	fTypedText = new char[textLen];
	memcpy(fTypedText, fTypingStart + fTextView.Text(), textLen);
	
	// Delete current typing run
	fTextView.Select(fTypingStart, fTypingEnd);
	fTextView.Delete();

	// Restore original text
	fTextView.Insert(fDeletedText, fDeletedTextLen);	

	// Restore original selection
	fTextView.Select(fSelStart, fSelEnd);
	
	fUndoCount++;
}

// ---------------------------------------------------------------------------
//		AppendItemName
// ---------------------------------------------------------------------------

void
MTypingUndoer::AppendItemName(
	String& inName)
{
	inName += "Typing";
}


// ===========================================================================
//	MIndentUndoer
// ===========================================================================

// ---------------------------------------------------------------------------
//		MIndentUndoer
// ---------------------------------------------------------------------------
//	Constructor

MIndentUndoer::MIndentUndoer(
	MIDETextView&	inTextView,
	bool			inIndentRight)
		: MTextUndoer(inTextView),
		fIndentToRight(inIndentRight)
{
	// Update the selection in the textview
	int32		start = inTextView.FindLineStart(fSelStart);
	int32		end = fSelEnd;

	if (inTextView.ByteAt(end - 1) != EOL_CHAR)
		end = inTextView.FindLineEnd(fSelEnd);

	// Save the existing selection and text
	if (start != fSelStart || end != fSelEnd)
	{
		inTextView.Select(start, end);
		fSelStart = start;
		fSelEnd = end;
		delete[] fDeletedText;
		
		fDeletedTextLen = end - start;
		fDeletedText = new char[fDeletedTextLen];
		memcpy(fDeletedText, fSelStart + fTextView.Text(), fDeletedTextLen);
	}

	// Do the shift
	if (fIndentToRight)
	{
		fTextView.ShiftRightSelf();
	}
	else
	{
		fTextView.ShiftLeftSelf();
	}

	// Save the new selection and text if something changed
	inTextView.GetSelection(&fNewSelStart, &fNewSelEnd);

	fShiftedText = nil;

	if (fNewSelStart != fSelStart || fNewSelEnd != fSelEnd)
	{
		fShiftedTextLen = fNewSelEnd - fNewSelStart;
		fShiftedText = new char[fShiftedTextLen];
		memcpy(fShiftedText, fNewSelStart + fTextView.Text(), fShiftedTextLen);
	}
}

// ---------------------------------------------------------------------------
//		~MTypingUndoer
// ---------------------------------------------------------------------------
//	Destructor

MIndentUndoer::~MIndentUndoer()
{
	delete[] fShiftedText;
}

// ---------------------------------------------------------------------------
//		UndoSelf
// ---------------------------------------------------------------------------
//	 Undo the shift by doing the opposite shift.

void
MIndentUndoer::UndoSelf()
{
	if (fNewSelStart != fSelStart || fNewSelEnd != fSelEnd)
	{
		// Restore selection
		fTextView.Select(fNewSelStart, fNewSelEnd);
		fTextView.Delete();
		fTextView.Insert(fDeletedText, fDeletedTextLen);
		fTextView.Select(fSelStart, fSelEnd);
	}
}

// ---------------------------------------------------------------------------
//		RedoSelf
// ---------------------------------------------------------------------------
//	 Redo the shift.

void
MIndentUndoer::RedoSelf()
{
	if (fNewSelStart != fSelStart || fNewSelEnd != fSelEnd)
	{
		// Restore selection
		fTextView.Select(fSelStart, fSelEnd);
		fTextView.Delete();
		fTextView.Insert(fShiftedText, fShiftedTextLen);
		fTextView.Select(fNewSelStart, fNewSelEnd);
	}
}

// ---------------------------------------------------------------------------
//		AppendItemName
// ---------------------------------------------------------------------------

void
MIndentUndoer::AppendItemName(
	String& inName)
{
	inName += "Indent";
}

// ===========================================================================
//	MAddOnUndoer
// ===========================================================================
//	An undo class that is a list of undo actions that can be performed by
//	Editor add-ons.  Add-ons can only perform Insert and Delete actions but
//	they can perform as many as they like.  We keep of list of MInsertUndoers
//	and MClearUndoers for each editor add-invocation.  MClearUndoer works
//	OK for the delete action.
// ---------------------------------------------------------------------------
//		MAddOnUndoer
// ---------------------------------------------------------------------------
//	Constructor

MAddOnUndoer::MAddOnUndoer(
	MIDETextView&	inTextView)
		: MTextUndoer(inTextView)
{
}

// ---------------------------------------------------------------------------
//		~MAddOnUndoer
// ---------------------------------------------------------------------------
//	Destructor

MAddOnUndoer::~MAddOnUndoer()
{
	for (int32 i = 0; i < fUndoList.CountItems(); ++i)
		delete fUndoList.ItemAt(i);
}

// ---------------------------------------------------------------------------
//		PushUndoer
// ---------------------------------------------------------------------------
//	 Add another undoer to the list.

void
MAddOnUndoer::PushUndoer(
	MTextUndoer*	inUndoer)
{
	fUndoList.AddItem(inUndoer);
}

// ---------------------------------------------------------------------------
//		UndoSelf
// ---------------------------------------------------------------------------
//	Play back all the undo actions from last to first.

void
MAddOnUndoer::UndoSelf()
{
	for (int32 i = fUndoList.CountItems() - 1; i >= 0; --i)
		fUndoList.ItemAt(i)->Undo();
}

// ---------------------------------------------------------------------------
//		RedoSelf
// ---------------------------------------------------------------------------
//	 Play back all the actions from first to last.

void
MAddOnUndoer::RedoSelf()
{
	for (int32 i = 0; i < fUndoList.CountItems(); ++i)
		fUndoList.ItemAt(i)->Redo();
}

// ---------------------------------------------------------------------------
//		AppendItemName
// ---------------------------------------------------------------------------

void
MAddOnUndoer::AppendItemName(
	String& inName)
{
	inName += "Addon Action";
}

// ===========================================================================
//	MInsertUndoer
// ===========================================================================
//	An undoer for inserts that come from editor add-ons.  Very similar to
//	the paste undoer except the text doesn't come from the clipboard.
// ---------------------------------------------------------------------------
//		MInsertUndoer
// ---------------------------------------------------------------------------
//	Constructor

MInsertUndoer::MInsertUndoer(
	MIDETextView&	inTextView,
	const char *	inInsertedText,
	ssize_t			inTextLength)
: MTextUndoer(inTextView)
{
	// Save the text
	fInsertedTextLen = inTextLength;
	fInsertedText = new char[inTextLength];
	memcpy(fInsertedText, inInsertedText, inTextLength);
}

// ---------------------------------------------------------------------------
//		~MInsertUndoer
// ---------------------------------------------------------------------------
//	Destructor

MInsertUndoer::~MInsertUndoer()
{
	delete[] fInsertedText;
}

// ---------------------------------------------------------------------------
//		RedoSelf
// ---------------------------------------------------------------------------

void
MInsertUndoer::RedoSelf()
{
	// Delete selected text
	fTextView.Select(fSelStart, fSelEnd);
	fTextView.Delete();
	
	// Restore the text that was inserted
	fTextView.Insert(fInsertedText, fInsertedTextLen);
	fTextView.Select(fSelStart, fSelStart + fInsertedTextLen);
}

// ---------------------------------------------------------------------------
//		UndoSelf
// ---------------------------------------------------------------------------

void
MInsertUndoer::UndoSelf()
{
	// Delete text that was inserted
	fTextView.Select(fSelStart, fSelStart + fInsertedTextLen);
	fTextView.Delete();
	
	// Restore text deleted by the insert
	fTextView.Insert(fInsertedText, fDeletedTextLen);

	// Restore selection
	fTextView.Select(fSelStart, fSelEnd);
}

// ---------------------------------------------------------------------------
//		AppendItemName
// ---------------------------------------------------------------------------

void
MInsertUndoer::AppendItemName(
	String& inName)
{
	inName += "Insert";
}
