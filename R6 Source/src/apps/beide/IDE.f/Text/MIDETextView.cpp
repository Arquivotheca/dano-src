//==================================================================
//	MIDETextView.cpp
//	Copyright 1995 - 96  Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	BDS

#include <string.h>
#include <ctype.h>

#include "MIDETextView.h"
#include "MTextUndoer.h"
#include "MMessageWindow.h"
#include "MWEditUtils.h"
#include "ProjectCommands.h"
#include "IDEMessages.h"
#include "IDEApp.h"
#include "CString.h"
#include "Utils.h"

#include <InterfaceKit.h>
#include <SupportKit.h>
#include <Application.h>
#include <Clipboard.h>

const float kTextLeftMargin = 5.0;
const float kTextTopMargin = 3.0;
const int32 kInvalidAnchor = -1;

// Marker for when we find we can't get
// back to the saved state
const int32 kDocAlwaysDirtySaveMark = -2;

const STEStyle* kNullStyle;		// default font

// simple class to simplify handling of editor commands
class CursorCaretState {
public:
CursorCaretState(
	MIDETextView*	inView)
: fView(inView)
{
	// hide the cursor and caret
	be_app->ObscureCursor();
	if (inView->mCaretVisible)
		inView->InvertCaret();
}
~CursorCaretState()
{
	// draw the caret
	if (fView->mSelStart == fView->mSelEnd) {
		if (!fView->mCaretVisible)
			fView->InvertCaret();
	}
}
private:
	MIDETextView*	fView;
};

// ---------------------------------------------------------------------------
//		MIDETextView
// ---------------------------------------------------------------------------
//	Constructor

MIDETextView::MIDETextView(
	const BRect & 	area,
	SuffixType		inSuffix)
	: STEngine(
		area,
		"text",
		GetTextRect(area),
		kNullStyle,
		B_FOLLOW_ALL_SIDES,
		B_PULSE_NEEDED | B_WILL_DRAW | B_FRAME_EVENTS),
		fStyler(inSuffix, *this, mText, mLines, mStyles)
{
	fUndoer = nil;
	fTypingUndoer = nil;
	fAnchor = kInvalidAnchor;
	fUndoTop = -1;
	fUndoCurrent = -1;
	fUndoSaveMark = -1;
	fUsingMultipleUndo = true;
	fAnchorSelStart = kInvalidAnchor;
	fAnchorSelEnd = kInvalidAnchor;
	fFlashWhenTyping = false;
	fUsesSyntaxColoring = true;
	SetDirty(false);
//	UseOffscreen(B_RGB_32_BIT);
//	UseOffscreen(B_COLOR_8_BIT);
	SetColorSpace(BScreen().ColorSpace());
}

void
MIDETextView::Init()
{
	kNullStyle = new STEStyle(*be_fixed_font, black, 0);
	((STEStyle*)kNullStyle)->SetSpacing(B_BITMAP_SPACING);
}

// ---------------------------------------------------------------------------
//		GetTextRect
// ---------------------------------------------------------------------------

BRect
MIDETextView::GetTextRect(
	const BRect & area)
{
	BRect r = area;
	r.OffsetTo(B_ORIGIN);
	r.left += kTextLeftMargin;
	r.top += kTextTopMargin;
	r.right = r.left + 2000.0;
	return r;
}

// ---------------------------------------------------------------------------
//		~MIDETextView
// ---------------------------------------------------------------------------
//	Destructor

MIDETextView::~MIDETextView()
{
	ClearAllUndo();
}

// ------------------------------------------------------------
// 	MessageReceived
// ------------------------------------------------------------
//	Handle all of the editor commands.

void
MIDETextView::MessageReceived(
	BMessage	*message)
{
	switch (message->what) 
	{
		case cmd_MoveToPreviousCharacter:
		{
			CursorCaretState	state(this);
			HandleArrowKey(B_LEFT_ARROW);
		}
			break;

		case cmd_MoveToNextCharacter:
		{
			CursorCaretState	state(this);
			HandleArrowKey(B_RIGHT_ARROW);
		}
			break;

		case cmd_MoveToPreviousSubWord:
		case cmd_MoveToNextSubWord:
		case cmd_MoveToPreviousWord:
		case cmd_MoveToNextWord:
		case cmd_MoveToStartOfLine:
		case cmd_MoveToEndOfLine:
		case cmd_SelectPreviousCharacter:
		case cmd_SelectNextCharacter:
		case cmd_SelectPreviousSubWord:
		case cmd_SelectNextSubWord:
		case cmd_SelectPreviousWord:
		case cmd_SelectNextWord:
		case cmd_SelectToStartOfLine:
		case cmd_SelectToEndOfLine:
		{
			CursorCaretState	state(this);
			DoSpecialLeftRightKey(message->what);
		}
			break;

		case cmd_MoveToPreviousLine:
		{
			CursorCaretState	state(this);
			HandleArrowKey(B_UP_ARROW);
		}
			break;

		case cmd_MoveToNextLine:
		{
			CursorCaretState	state(this);
			HandleArrowKey(B_DOWN_ARROW);
		}
			break;

		case cmd_MoveToTopOfPage:
		case cmd_MoveToBottomOfPage:
		case cmd_MoveToTopOfFile:
		case cmd_MoveToBottomOfFile:
		case cmd_SelectPreviousLine:
		case cmd_SelectNextLine:
		case cmd_SelectToTopOfPage:
		case cmd_SelectToBottomOfPage:
		case cmd_SelectToStartOfFile:
		case cmd_SelectToEndOfFile:
		{
			CursorCaretState	state(this);
			DoSpecialUpDownKey(message->what);
		}
			break;

		case cmd_ScrollUpLine:
		case cmd_ScrollDownLine:
		{
			int32		index;
			int32		lineNumber;
			if (message->what == cmd_ScrollUpLine)
			{
				index = IndexAtPoint(0.0, Bounds().top + 1.0);
				lineNumber = OffsetToLine(index);
				lineNumber = max(lineNumber - 1, 0L);
			}
			else
			{
				index = IndexAtPoint(0.0, Bounds().bottom - 1.0);
				lineNumber = OffsetToLine(index);
				lineNumber = min(lineNumber + 1, CountLines() - 1);
			}
		
			const STELine* 		line = mLines[lineNumber];

			ScrollOffsetIntoView(line->offset);
		}
			break;

		case cmd_ScrollUpPage:
		{
			CursorCaretState	state(this);
			HandlePageKey(B_PAGE_UP);
		}
			break;
	
		case cmd_ScrollDownPage:
		{
			CursorCaretState	state(this);
			HandlePageKey(B_PAGE_DOWN);
		}
			break;

		case cmd_ScrollToTopOfFile:
		{
			CursorCaretState	state(this);
			HandlePageKey(B_HOME);
		}
			break;

		case cmd_ScrollToEndOfFile:
		{
			CursorCaretState	state(this);
			HandlePageKey(B_END);
		}
			break;
			
		case cmd_BackwardDelete:
		{
			if (mEditable)
			{
				CursorCaretState	state(this);

				BuildTypingTask();
				fTypingUndoer->BackwardErase();
				HandleBackspace();
			}
			else
				beep();
		}
			break;

		case cmd_ForwardDelete:
		{
			if (mEditable)
			{
				CursorCaretState	state(this);
	
				BuildTypingTask();
				fTypingUndoer->ForwardErase();
				HandleDelete();
			}
			else
				beep();
		}
			break;

		case cmd_DeleteToEndOfLine:
		{
			if (mEditable)
			{
				CursorCaretState	state(this);
				int32				end = FindLineEnd(mSelEnd);
				if (end < TextLength())
					end--;
				SpecialClear(end);
			}
			else
				beep();
		}
			break;

		case cmd_DeleteToEndOfFile:
		{
			if (mEditable)
			{
				CursorCaretState	state(this);
				SpecialClear(TextLength());
			}
			else
				beep();
		}
			break;

		// Repost get next/previous error to message window
		case cmd_PrevMessage:
		case cmd_NextMessage:
			MMessageWindow::MessageToMostRecent(*message);
			break;
			
		// override since we use custom messages for these
		case cmd_Cut:
			Cut();
			break;
			
		case cmd_Copy:
			Copy();
			break;
			
		case cmd_Paste:
			Paste();
			break;

		// allow the user to drag a file over me
		// this just represents a "open this file"
		// if the message contains text, then fall through so it can be pasted
		case B_SIMPLE_DATA:
			if (message->HasData("text/plain", B_MIME_TYPE) == false && message->HasRef("refs") == true) {
				message->what = msgOpenSourceFile;
				be_app_messenger.SendMessage(message);
				break;
			}
			// fall through if text/plain or no "refs"
			
		default:
			STEngine::MessageReceived(message);
			break;
	}
}

// ---------------------------------------------------------------------------
//		AdjustUndoMenuItems
// ---------------------------------------------------------------------------

void
MIDETextView::AdjustUndoMenuItems(
	BMenuItem&	inUndoItem,
	BMenuItem&	inRedoItem)
{
	if (fUsingMultipleUndo)
	{
		MTextUndoer*	undoer;
		if (fUndoCurrent >= 0)
		{
			undoer = fUndoList.ItemAt(fUndoCurrent);
			undoer->SetUndoText(inUndoItem);
		}
		else
		{
			inUndoItem.SetLabel("Can't Undo");
			inUndoItem.SetEnabled(false);
		}
		if (fUndoCurrent < fUndoTop)
		{
			undoer = fUndoList.ItemAt(fUndoCurrent + 1);
			undoer->SetRedoText(inRedoItem);
		}
		else
		{
			inRedoItem.SetLabel("Can't Redo");
			inRedoItem.SetEnabled(false);
		
		}
	}
	else
	{
		if (fUndoer)
			fUndoer->AdjustUndoMenuItem(inUndoItem);
		else
		{
			inUndoItem.SetLabel("Can't Undo");
			inUndoItem.SetEnabled(false);
			inRedoItem.SetLabel("Multiple Undo not enabled");
			inRedoItem.SetEnabled(false);
		}
	}
}

// ---------------------------------------------------------------------------
//		Print
// ---------------------------------------------------------------------------

void
MIDETextView::Print()
{
	BPrintJob		printJob(Window()->Title());
	if (B_NO_ERROR == IDEApp::BeAPP().PrintSetup(printJob))
	{
		BRect			r = printJob.PrintableRect();
		const int32		textHeight = (int32) GetHeight(0, LONG_MAX);
		const float		pageHeight = r.bottom - r.top;
		const int32		lastLine = CountLines();
		int32			firstLine = 0;
		
		r.OffsetTo(B_ORIGIN);

		printJob.BeginJob();

		// iterate through the lines until we get to the correct starting page
		// once we get there, iterate through the lines creating a page full
		// and then spool each page
		// stop when we finish all text, the user cancels, or we print all pages
		int32 firstPage = printJob.FirstPage(); 
		int32 lastPage = printJob.LastPage();
		int32 currentPage = 1;
		while (currentPage < firstPage && firstLine <= lastLine) {
			int32 nextLine = firstLine;
			while (nextLine <= lastLine && GetHeight(firstLine, nextLine) < pageHeight) {
				nextLine++;
			}
			// reached a page full - bump the page number and starting first line
			currentPage++;
			firstLine = nextLine;
		}
		
		// we are now at the point in our file where the user said to start printing...
		while(firstLine < lastLine && printJob.CanContinue() && currentPage <= lastPage) 
		{
			int32		nextLine = firstLine;
			while (nextLine <= lastLine && GetHeight(firstLine, nextLine) < pageHeight)
				nextLine++;

			r.top = mLines[firstLine]->origin;
			if (nextLine+1 < lastLine)
				r.bottom = mLines[nextLine+1]->origin - 1.0;
			else
				r.bottom = r.top + pageHeight;			// last page

			printJob.DrawView(this, r, BPoint(0, 0));
			printJob.SpoolPage();
			
			currentPage++;
			firstLine = nextLine;
		}

		if (printJob.CanContinue())
			printJob.CommitJob();
	}
}

// ---------------------------------------------------------------------------
//		KeyDown
// ---------------------------------------------------------------------------

void
MIDETextView::KeyDown(
	const char *	inBytes, 
	int32 			inNumBytes)
{
	if (!IsEditable())
	{
		beep();
		return;
	}

	switch (inBytes[0])
	{
		// These keys are invalid in text and should be ignored
		case B_ESCAPE:
		case B_INSERT:
		case B_FUNCTION_KEY:
		case B_PRINT_KEY:
		case B_SCROLL_KEY:
			break;

		case B_TAB:
			BuildTypingTask();
			fTypingUndoer->InputACharacter();
			STEngine::KeyDown(inBytes, inNumBytes);
			break;

		case B_RETURN:
		{
			BuildTypingTask();
//			fTypingUndoer->InputACharacter();
			int32		selStart;
			int32		selEnd;
			bool		doKeyDown = true;

			GetSelection(&selStart, &selEnd);
			
			// At beginning of line?
			if (fAutoIndent && selStart > 0 && mText[selStart-1] != EOL_CHAR)
			{
				int32		linestart = LineStart(selStart);	
				int32		whiteSpace = linestart;
				String		buff;
				char		c;

				while ((c = mText[whiteSpace]) != EOL_CHAR && isspace(c)
					&& whiteSpace < selStart)
				{
					whiteSpace++;
					buff += c;
				}

				whiteSpace -= linestart;
				if (whiteSpace > 0)
				{
					if (selStart == selEnd)
					{
						buff.Insert("\n", 0);		// Insert newline if not replacing
					}
					else
					{
						// replacing text
						fTypingUndoer->InputACharacter();
						STEngine::KeyDown(inBytes, inNumBytes);
					}

					Insert(buff, buff.GetLength());	// Insert whitespace
					fTypingUndoer->InputCharacters(buff.GetLength());
					doKeyDown = false;
				}
			}

			if (doKeyDown)
			{
				fTypingUndoer->InputACharacter();
				STEngine::KeyDown(inBytes, inNumBytes);
			}
			break;
		}

		default:			// Normal input
			{
				BuildTypingTask();
				fTypingUndoer->InputACharacter(inNumBytes);
				STEngine::KeyDown(inBytes, inNumBytes);
				if (fFlashWhenTyping)
				{
					if (inBytes[0] == '}') {
						FlashPreviousAccolade(ACCOLADES);
					}
					else
					if (inBytes[0] == ']') {
						FlashPreviousAccolade(SQUAREBRACKETS);
					}
					else
					if (inBytes[0] == ')') {
						FlashPreviousAccolade(ROUNDBRACKETS);
					}
				}
			}		
			break;
	}
}

// ------------------------------------------------------------
// 	HandleAlphaKey
// ------------------------------------------------------------
// A printing key has been pressed

void
MIDETextView::HandleAlphaKey(
	const char *	inBytes, 
	int32 			inNumBytes)
{
	if (! fUsesSyntaxColoring)
		STEngine::HandleAlphaKey(inBytes, inNumBytes);
	else
	{
		int32	selStart = mSelStart;
		int32	selEnd = mSelEnd;
		
		if (mSelStart != mSelEnd) 
		{
			DrawSelection(mSelStart, mSelEnd);
			RemoveRange(mSelStart, mSelEnd);
		}
	
		InsertAt(inBytes, inNumBytes, mSelStart);

		mSelStart += inNumBytes;
		mSelEnd = mSelStart;

		int32	 len = selStart == selEnd ? inNumBytes : 2;
		fStyler.TextChanged(selStart, len, true);		// Parse text
		ScrollToOffset(mSelEnd);
	
		HandleModification();
	}
}

// ------------------------------------------------------------
// 	HandleBackspace
// ------------------------------------------------------------
// The Backspace key has been pressed

void
MIDETextView::HandleBackspace()
{
	if (mSelStart == 0 && mSelEnd == 0)
		return;

	if (! fUsesSyntaxColoring)
		STEngine::HandleBackspace();
	else
	{
		int32	selStart = mSelStart;
		int32	selEnd = mSelEnd;
		int32	glyphwidth = GlyphWidth(selEnd - 1);

		if (mSelStart == mSelEnd) 
		{
				mSelStart -= glyphwidth;
		}
		else
			DrawSelection(mSelStart, mSelEnd);
	
		RemoveRange(mSelStart, mSelEnd);
		mSelEnd = mSelStart;
		
		if (selStart == selEnd)
			selStart = max(selStart - glyphwidth, 0L);
		int32	len = selStart == selEnd ? glyphwidth : 2;
		fStyler.TextChanged(selStart, len, false);	// Parse text
		ScrollToOffset(mSelEnd);

		HandleModification();
	}
}

// ------------------------------------------------------------
// 	HandleDelete
// ------------------------------------------------------------
// The Delete key has been pressed

void
MIDETextView::HandleDelete()
{
	if (mSelStart == TextLength() && mSelStart == mSelEnd)
		return;

	if (! fUsesSyntaxColoring)
		STEngine::HandleDelete();
	else
	{
		int32	selStart = mSelStart;
		int32	selEnd = mSelEnd;
		int32	glyphwidth = GlyphWidth(selStart);
	
		if (mSelStart == mSelEnd) 
		{
			mSelEnd += glyphwidth;
		}
		else
			DrawSelection(mSelStart, mSelEnd);
			
		RemoveRange(mSelStart, mSelEnd);
		mSelEnd = mSelStart;
		
		int32 	len = selStart == selEnd ? glyphwidth : selEnd - selStart;
		fStyler.TextChanged(selStart, len, false);	// Parse text
		ScrollToOffset(mSelEnd);

		HandleModification();
	}
}

// ------------------------------------------------------------
// 	Insert
// ------------------------------------------------------------
// Copy inLength bytes from inText and insert it at the caret
// position, or at the beginning of the selection range
// 
// Pass NULL for inStyles if there is no style data
//
// The caret/selection will move with the insertion
//	Called from STEngine::Paste and STEngine::MessageDropped

void
MIDETextView::Insert(
	const char				*inText,
	int32					inLength,
	ConstSTEStyleRangePtr	/*inStyles*/)
{
	// do we really need to do anything?
	if (inLength < 1)
		return;
	
	bool	isActive = mActive;
	mActive = false;

	// hide the caret/unhilite the selection
	if (isActive) {
		if (mSelStart != mSelEnd)
			DrawSelection(mSelStart, mSelEnd);
		else {
			if (mCaretVisible)
				InvertCaret();
		}
	}
	
	int32	selStart;
	int32	selEnd;
	
	GetSelection(&selStart, &selEnd);

	// Always ignore any styles when inserting
	STEngine::Insert(inText, inLength, nil);

	if (fUsesSyntaxColoring)
		fStyler.TextChanged(selStart, inLength, true);	// Parse text

	// draw the caret/hilite the selection
	if (isActive) {
		if (mSelStart != mSelEnd)
			DrawSelection(mSelStart, mSelEnd);
		else {
			if (!mCaretVisible)
				InvertCaret();
		}
	}
	
	mActive = isActive;
}

// ---------------------------------------------------------------------------
//		Delete
// ---------------------------------------------------------------------------

void
MIDETextView::Delete()
{
	int32		selStart;
	int32		selEnd;
	
	GetSelection(&selStart, &selEnd);

	STEngine::Delete();

	if (fUsesSyntaxColoring)
	{
		// hide the caret/unhilite the selection
		if (mActive) {
			if (mSelStart != mSelEnd)
				DrawSelection(mSelStart, mSelEnd);
			else {
				if (mCaretVisible)
					InvertCaret();
			}
		}

		fStyler.TextChanged(selStart, selEnd - selStart, false);// Parse text

		// draw the caret/hilite the selection
		if (mActive) {
			if (mSelStart != mSelEnd)
				DrawSelection(mSelStart, mSelEnd);
			else {
				if (!mCaretVisible)
					InvertCaret();
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		LineStart
// ---------------------------------------------------------------------------

int32
MIDETextView::LineStart(
	int32	inOffset)
{
	ASSERT(inOffset <= TextLength());

	int32				line = OffsetToLine(inOffset);
	const STELine*		lineRec = mLines[line];

	return lineRec->offset;
}

// ---------------------------------------------------------------------------
//		Undo
// ---------------------------------------------------------------------------

void
MIDETextView::Undo()
{
	if (fUsingMultipleUndo)
	{
		if (fUndoCurrent >= 0) {
			MTextUndoer* undoer = fUndoList.ItemAt(fUndoCurrent--);
			undoer->Undo();
			// fUndoSaveMark is set at the point in the undo stack
			// where the document was saved (in an unsaved document this would
			// be -1)  When we reach that point, the document is no 
			// longer dirty.
			this->SetDirty(fUndoCurrent != fUndoSaveMark);	 	
		}
	}
	else if (fUndoer) {
		fUndoer->Undo();
		SetDirty(true);	
	}
}

// ---------------------------------------------------------------------------
//		Redo
// ---------------------------------------------------------------------------

void
MIDETextView::Redo()
{
	if (fUsingMultipleUndo)
	{
		if (fUndoCurrent < fUndoTop)
		{
			MTextUndoer*	undoer = fUndoList.ItemAt(++fUndoCurrent);
			undoer->Redo();
			// There is one case where redo actually cleans the document
			// rather than making it dirty.  This is when we do 
			// some actions, save, undo, and then redo back up to the 
			// exact point where we did the save
			SetDirty(fUndoCurrent != fUndoSaveMark);
		}
	}
	else
	if (fUndoer)
	{
		fUndoer->Redo();
		SetDirty(true);	
	}
}

// ---------------------------------------------------------------------------
//		PushUndoer
// ---------------------------------------------------------------------------

void
MIDETextView::PushUndoer()
{
	if (fUsingMultipleUndo)
	{
		ASSERT(fUndoCurrent <= fUndoTop);

		if (fUndoCurrent != fUndoTop)
		{
			for (int32 i = fUndoTop; i > fUndoCurrent; --i)
				delete fUndoList.RemoveItemAt(i);
			fUndoTop = fUndoCurrent;
		}
		
		fUndoList.AddItem(fUndoer);
		fUndoTop++;
		fUndoCurrent++;

		// if we have undone back before our save mark
		// and are now pushing something new, we can never
		// get back to the saved state...
		if (fUndoCurrent <= fUndoSaveMark) {
			fUndoSaveMark = kDocAlwaysDirtySaveMark;
		}

		ASSERT(fUndoTop == fUndoList.CountItems() - 1);
	}
}

// ---------------------------------------------------------------------------
//		BuildTypingTask
// ---------------------------------------------------------------------------

void
MIDETextView::BuildTypingTask()
{
	if (fUsingMultipleUndo)
	{
		if (fTypingUndoer == nil || fUndoCurrent != fUndoTop ||
			(fTypingUndoer != nil && fTypingUndoer->SelectionChanged()))
		{
			fTypingUndoer = new MTypingUndoer(*this);
			fUndoer = fTypingUndoer;
			PushUndoer();
		}
	}
	else
	if (fTypingUndoer == nil)
	{
		delete fUndoer;
		fTypingUndoer = new MTypingUndoer(*this);
		fUndoer = fTypingUndoer;
	}
}

// ---------------------------------------------------------------------------
//		ClearTypingTask
// ---------------------------------------------------------------------------

void
MIDETextView::ClearTypingTask()
{
	if (! fUsingMultipleUndo)
		delete fUndoer;

	fTypingUndoer = nil;
	fUndoer = nil;
}

// ---------------------------------------------------------------------------
//		ClearAllUndo
// ---------------------------------------------------------------------------

void
MIDETextView::ClearAllUndo()
{
	ClearTypingTask();
	if (fUsingMultipleUndo)
	{
		for (int32 i = fUndoTop; i >= 0; --i)
			delete fUndoList.ItemAt(i);
		
		fUndoList.MakeEmpty();
		fUndoTop = fUndoCurrent = -1;
		fUndoSaveMark = fUndoCurrent;
	}
}

// ---------------------------------------------------------------------------

void
MIDETextView::DocumentSaved()
{
	// When we save the document, it is no longer dirty
	// Also, we need to remember what the current undo state is.  
	// If in the course of a bunch of undo's we cross through this
	// point in the undo stack, an undo makes the document clean rather
	// than dirty.  (But one more undo makes it dirty again. -- and
	// the same thing for redo.)

	this->SetDirty(false);
	fUndoSaveMark = fUndoCurrent;
}

// ---------------------------------------------------------------------------
//		StartAddon
// ---------------------------------------------------------------------------
//	An editor add-on is about to be called in response to a menu choice.
//	We create an MAddOnUndoer and all insert and delete actions performed
//	by the add-on are added to this undoer.

void
MIDETextView::StartAddon()
{
	fAddOnUndoer = new MAddOnUndoer(*this);
}

// ---------------------------------------------------------------------------
//		StopAddon
// ---------------------------------------------------------------------------
//	The add-on is finished.  If the add-on did something we add the MAddOnUndoer
//	to our list of undo actions.  If it didn't do anything we just delete it.

void
MIDETextView::StopAddon()
{
	if (fAddOnUndoer->ActionCount() > 0)
	{
		ClearTypingTask();
		fUndoer = fAddOnUndoer;
		PushUndoer();
	}
	else
	{
		delete fAddOnUndoer;	// the add-on didn't make any actions
	}
}

// ---------------------------------------------------------------------------
//		AddonInsert
// ---------------------------------------------------------------------------
//	Handle an insert action from an editor add-on.

void
MIDETextView::AddonInsert(
	const char* 	inText, 
	int32 			inLength)
{
	fAddOnUndoer->PushUndoer(new MInsertUndoer(*this, inText, inLength));

	Insert(inText, inLength);
}

// ---------------------------------------------------------------------------
//		AddonDelete
// ---------------------------------------------------------------------------
//	Handle a delete action from an editor add-on.

void
MIDETextView::AddonDelete()
{
	fAddOnUndoer->PushUndoer(new MClearUndoer(*this));

	Delete();
}

// ---------------------------------------------------------------------------
//		Cut
// ---------------------------------------------------------------------------

void
MIDETextView::Cut()
{
	ClearTypingTask();
	
	fUndoer = new MCutUndoer(*this);
	PushUndoer();
	STEngine::Cut();
}

// ---------------------------------------------------------------------------
//		Copy
// ---------------------------------------------------------------------------
//	no copy undoer yet.

void
MIDETextView::Copy()
{
	STEngine::Copy();
}

// ---------------------------------------------------------------------------
//		Paste
// ---------------------------------------------------------------------------

void
MIDETextView::Paste()
{
	ClearTypingTask();
	
	fUndoer = new MPasteUndoer(*this);
	PushUndoer();
	
	if (be_clipboard->Lock())
	{
		// Get data from clipboard
		ssize_t 		textLen = 0;
		const char	*	text;
	
		if (B_NO_ERROR == be_clipboard->Data()->FindData(kTextPlain, B_MIME_TYPE, (const void**) &text, &textLen))
		{
			// Replace the text in the clipboard if there are any Mac_Returns in it
			for (int32 i = 0; i < textLen; i++)
			{
				if (text[i] == MAC_RETURN)
				{
					String		newText(text, textLen);
				
					newText.Replace(MAC_RETURN, EOL_CHAR);
					be_clipboard->Clear();
					be_clipboard->Data()->AddData(kTextPlain, B_MIME_TYPE, newText, textLen);
					be_clipboard->Commit();
					break;
				}
			}
		}
	
		be_clipboard->Unlock();
	}

	STEngine::Paste();

	ScrollToSelection();
}

// ---------------------------------------------------------------------------
//		Clear
// ---------------------------------------------------------------------------

void
MIDETextView::Clear()
{
	ClearTypingTask();
	
	fUndoer = new MClearUndoer(*this);
	PushUndoer();

	Delete();
}

// ---------------------------------------------------------------------------
//		SpecialClear
// ---------------------------------------------------------------------------
//	Special clear for 'delete to end of line' and 'delete to end of file'.
//	inEndOffset is the end of the text to be deleted.  This allows
//	using the delete function to do the deleting of text and to still
//	restore the selection correctly.

void
MIDETextView::SpecialClear(
	int32	inEndOffset)
{
	int32			selEnd = mSelEnd;
	mSelEnd = inEndOffset;

	ClearTypingTask();			
	fUndoer = new MClearUndoer(*this, selEnd);
	PushUndoer();
	Delete();
}

// ------------------------------------------------------------
// 		MessageDropped
// ------------------------------------------------------------
// Respond to dropped messages

bool
MIDETextView::MessageDropped(
	BMessage	*inMessage,
	BPoint 		where,
	BPoint		offset)
{
	bool		deleting = false;
	int32		selStart;
	int32		length;

	// Are we sure we like this message?
	if (CanDrop(inMessage))
	{		
		int32	dropOffset = PointToOffset(where);
		bool	sameWindowDrop = (mActive) && (mSelStart != mSelEnd);
		
		if (sameWindowDrop && (dropOffset < mSelStart || dropOffset > mSelEnd))
		{
			deleting = true;
			selStart = mSelStart;
			length = mSelEnd - mSelStart;
			if (selStart > dropOffset)
				selStart += length;
		}

		// Dropping onto itself?
		if (! sameWindowDrop || dropOffset < mSelStart || dropOffset > mSelEnd)
		{
			ClearTypingTask();
			
			ssize_t 	textLen = 0;
			char *	text = nil;

			if (B_NO_ERROR == inMessage->FindData("text/plain", B_MIME_TYPE, (const void**) &text, &textLen))
			{
				if (text != nil)
				{
					// Replace the text in the message if there are any Mac_Returns in it
					for (int32 i = 0; i < textLen; i++)
					{
						if (text[i] == MAC_RETURN)
						{
							String		newText(text, textLen);
						
							newText.Replace(MAC_RETURN, EOL_CHAR);
							inMessage->RemoveName("text/plain");
							inMessage->AddData("text/plain", B_MIME_TYPE, (void*)(const char*) newText, textLen);
							break;
						}
					}
	
				}
			}
		
			ASSERT(text != nil);
			if (text != nil)
			{
				fUndoer = new MDragUndoer(*this, text, textLen, dropOffset, sameWindowDrop);
				PushUndoer();
			}
		}
	}

	bool	result = STEngine::MessageDropped(inMessage, where, offset);
	
	// Deal with reparsing the text next to the text that is deleted in a same window move-drag
	if (deleting && fUsesSyntaxColoring)
	{
		// hide the caret/unhilite the selection
		HideSelection();

		fStyler.TextChanged(selStart, length, false);	// Parse text
	
		// draw the caret/hilite the selection
		ShowSelection();
	}

	return result;
}

// ---------------------------------------------------------------------------
//		Replace
// ---------------------------------------------------------------------------
//	Replace the selected text with the input string in an undoable way.

void
MIDETextView::Replace(
	const char * inString)
{
	ClearTypingTask();
	BuildTypingTask();

	fTypingUndoer->Replace(inString);
	
	int32		selStart;
	int32		selEnd;

	GetSelection(&selStart, &selEnd);

	int32		len = strlen(inString);
	STEngine::Delete();
	Insert(inString, len);

	Select(selStart, selStart + len);
}

// ---------------------------------------------------------------------------
//		BreaksAtChar
// ---------------------------------------------------------------------------
//	Indicate whether the character causes word break.  Called for double
//	selection.

bool
MIDETextView::IsWordBreakChar(
	uchar inChar)
{
	return ! (islower(inChar) || isupper(inChar) || isdigit(inChar) || inChar == '_');
}

// ---------------------------------------------------------------------------
//		ShiftRight
// ---------------------------------------------------------------------------
//	Shift all the lines in the current selection right by inserting a tab
//	at the beginning of each line.

void
MIDETextView::ShiftRight()
{
	ClearTypingTask();
	
	// The indentundoer does the shift
	fUndoer = new MIndentUndoer(*this, true);
	PushUndoer();

	SetDirty();
}

// ---------------------------------------------------------------------------
//		ShiftRightSelf
// ---------------------------------------------------------------------------
//	Shift all the lines in the current selection right by inserting a tab
//	at the beginning of each line.

void
MIDETextView::ShiftRightSelf()
{
	int32			start;
	int32			end;
	MList<int32>	lineStarts;
	STEStyleRange	range;
	
	// Get the style for text so inserted tabs are always text style
	// Otherwise the inserted tabs can be another style and the lineheight can change
	range.count = 1;
	range.runs[0].offset = 0;
	range.runs[0].style = fStyler.GetTextStyle();

	BuildSelectionsLineStarts(lineStarts);

	// hide the caret/unhilite the selection
	HideSelection();

	// Add the tabs to the start of all the lines in the selection
	int32		index = lineStarts.CountItems() - 1;

	while (lineStarts.GetNthItem(start, index--))
	{
		InsertAt("\t", 1, start, &range);
	}

	// Select the entire line for each line in the selection
	if (lineStarts.GetFirstItem(start) && lineStarts.GetLastItem(end))
	{
		end = FindLineEnd(end + lineStarts.CountItems());
		
		mSelStart = start;
		mSelEnd = end;
		Refresh(start, end, true, false);
	}

	ShowSelection();
}

// ---------------------------------------------------------------------------
//		ShiftLeft
// ---------------------------------------------------------------------------
//	Shift all the lines in the current selection left by removing spaces or tabs
//	at the beginning of these lines.

void
MIDETextView::ShiftLeft()
{
	ClearTypingTask();
	
	// The indentundoer does the shift
	fUndoer = new MIndentUndoer(*this, false);
	PushUndoer();

	SetDirty();
}

// ---------------------------------------------------------------------------
//		ShiftLeftSelf
// ---------------------------------------------------------------------------
//	Shift all the lines in the current selection left by removing spaces or tabs
//	at the beginning of these lines.

void
MIDETextView::ShiftLeftSelf()
{
	int32			start;
	int32			end;
	int32			charsDeleted = 0;
	MList<int32>	lineStarts;

	BuildSelectionsLineStarts(lineStarts);

	// hide the caret/unhilite the selection
	HideSelection();

	// Remove the first chars from the start of all the lines in the selection
	int32		index = lineStarts.CountItems() - 1;

	while (lineStarts.GetNthItem(start, index--))
	{
		uchar			c = mText[start];

		if (c == ' ' || c == '\t')
		{
			RemoveRange(start, start+1);
			charsDeleted++;
		}
	}

	// Select the entire line for each line in the selection
	if (charsDeleted > 0 && lineStarts.GetFirstItem(start) && lineStarts.GetLastItem(end))
	{
		end = FindLineEnd(end - charsDeleted + 1);

		mSelStart = start;
		mSelEnd = end;
		Refresh(start, end, true, false);
	}

	// draw the caret/hilite the selection
	ShowSelection();
}

// ---------------------------------------------------------------------------
//		BuildSelectionsLineStarts
// ---------------------------------------------------------------------------
//	Build a linestarts array for all the lines in the current selection.

void
MIDETextView::BuildSelectionsLineStarts(MList<int32>& outLineStarts)
{
	int32			start;
	int32			end;

	GetSelection(&start, &end);

	if (mText[end-1] == EOL_CHAR && start != end)
		end--;

	int32				lineNumber = OffsetToLine(start);
	int32				lastLineNumber = OffsetToLine(end);
	const STELine*		line;

	for (; lineNumber <= lastLineNumber; lineNumber++)
	{
		line = mLines[lineNumber];
		outLineStarts.AddItem(line->offset);
	}
}

// ---------------------------------------------------------------------------
//		FindLineStart
// ---------------------------------------------------------------------------
//	Scan backwards to find the beginning of the line that includes the
//	char at an offset of inIndex.  returns the offset of the eol char at the
//	end of the previous line or zero for the first line.

int32
MIDETextView::FindLineStart(int32 inIndex)
{
	int32			lineNumber = OffsetToLine(inIndex);
	const STELine*	line = mLines[lineNumber];

	return line->offset;
}

// ---------------------------------------------------------------------------
//		FindLineEnd
// ---------------------------------------------------------------------------
//	Returns the offset of the next '\n' after inIndex.

int32
MIDETextView::FindLineEnd(int32 inIndex)
{
	int32				lineNumber = OffsetToLine(inIndex);
	const STELine*		line = mLines[lineNumber++];
	const STELine*		nextLine;
	int32				nextLineOffset;

	if (lineNumber < mLines.NumLines())
	{
		nextLine = mLines[lineNumber];
		nextLineOffset = nextLine->offset;
	}
	else
		nextLineOffset = mText.Length();

	return nextLineOffset;
}

// ---------------------------------------------------------------------------
//		GetNextSelectedLineStart
// ---------------------------------------------------------------------------
//	returns the offset of the next eol character after inStart.  Also 
//	returns whether this eol char comes before inEnd.

bool
MIDETextView::GetNextSelectedLineStart(int32 &inStart, int32 inEnd)
{
	inStart = FindLineEnd(inStart);
	
	return inStart < inEnd;
}

// ---------------------------------------------------------------------------
//		Isletter
// ---------------------------------------------------------------------------

inline bool
MIDETextView::Isletter(short c)
{
	return (isalnum(c) || (c == '_'));	// isalphanumeric
}

// ---------------------------------------------------------------------------
//		NE_PrevLine
// ---------------------------------------------------------------------------
/*	Purpose..:	Return the position of the first character in prev line */
/*	Input....:	pointer to a char position in current line				*/
/*	Input....:	pointer to first char in buffer 						*/
/*	Return...:	pointer to first character in previous line 			*/

const char *
MIDETextView::NE_PrevLine(
	const char *text_pos, 
	const char *buffer_start)
{
	while (text_pos > buffer_start && * (text_pos -1) != EOL_CHAR)
		text_pos--;
	if (text_pos > buffer_start) {
		text_pos--;
		while (text_pos > buffer_start && * (text_pos - 1) != EOL_CHAR)
			text_pos--;
	}
	return(text_pos);
}

// ---------------------------------------------------------------------------
//		NE_NextLineMono
// ---------------------------------------------------------------------------
/*	Purpose..:	Return the position of the first character in next line */
/*	Input....:	pointer to a char position in current line				*/
/*	Input....:	pointer to first char in buffer 						*/
/*	Return...:	pointer to first character in next line 				*/

const char *
MIDETextView::NE_NextLineMono(
	const char *text_pos, 
	const char *limit)
{
	while (*text_pos != 0 && *text_pos != EOL_CHAR && text_pos < limit)
		text_pos++;
	if (*text_pos != 0 && text_pos < limit)
		text_pos++;
	return(text_pos);
}

// ---------------------------------------------------------------------------
//		NE_PixelToText
// ---------------------------------------------------------------------------
/*	Purpose..:	Find the text position of an x position 				*/
/*	Input....:	pointer to text line position							*/
/*	Input....:	x position												*/
/*	Return...:	pointer to text position								*/

const char *
MIDETextView::NE_PixelToText(
	const char *	text, 
	float 			xpos)
{
	float		x = 0.0;
	float		width;
	float		NE_charwidth = TabWidth();

	while (1) 
	{
		switch (*text) 
		{
			case '\t':
				width = NE_charwidth - (float) ((int32) x % (int32) NE_charwidth);
				break;

			case '\n':
			case 0:
				return(text);

			default:
				width = StringWidth(text, 1);
				break;
		}

		if (x + (width / 2.0f) >= xpos)
			return(text);

		x += width;
		text++;
	}
}

enum UnitsKind{
	nounits,
	smallunits,			// one glyph
	mediumunits,		// subword
	bigunits			// file
};

// ---------------------------------------------------------------------------
//		DoSpecialLeftRightKey
// ---------------------------------------------------------------------------
//	Handle left or right arrow keys in combination with command-shift-option.
//	This code is taken from the CW7 GM MWTextEdit.c.

void
MIDETextView::DoSpecialLeftRightKey(
	CommandT	inCommand)
{
	int32		s;
	int32		e;
	int32		n;
	int32		dir;
	const char *subword;
	const char *text = Text();
	int32		len = TextLength();
	bool		select = false;			// formerly shift key
//	bool		bigunits = false;		// formerly command key
//	bool		smallunits = false;		// formerly option key
	// small units represents subwords and is usually control-
	// medium units represents words and is usually option-
	// big units represents whole lines and is usually command
	UnitsKind	units = nounits;

	GetSelection(&s, &e);

	if (fAnchorSelStart != s || fAnchorSelEnd != e)
		fAnchor = s;

	// There are four orthogonal properties to be set here:
	// towards the start or end of file, selection, bigunits or small units

	// Towards start or end of file?
	switch (inCommand)
	{
		case cmd_MoveToPreviousSubWord:
		case cmd_MoveToPreviousWord:
		case cmd_MoveToStartOfLine:
		case cmd_SelectPreviousCharacter:
		case cmd_SelectPreviousSubWord:
		case cmd_SelectPreviousWord:
		case cmd_SelectToStartOfLine:
			n = s;
			dir = -1;
			break;

		case cmd_MoveToNextSubWord:
		case cmd_MoveToNextWord:
		case cmd_MoveToEndOfLine:
		case cmd_SelectNextCharacter:
		case cmd_SelectNextSubWord:
		case cmd_SelectNextWord:
		case cmd_SelectToEndOfLine:
			n = e;
			dir = 1;
			break;
	}

	// Selection?
	switch (inCommand)
	{
		case cmd_SelectPreviousCharacter:
		case cmd_SelectPreviousSubWord:
		case cmd_SelectPreviousWord:
		case cmd_SelectToStartOfLine:
		case cmd_SelectNextCharacter:
		case cmd_SelectNextSubWord:
		case cmd_SelectNextWord:
		case cmd_SelectToEndOfLine:
			select = true;		
			break;
	}

	// BigUnits or small units?
	switch (inCommand)
	{
		case cmd_MoveToStartOfLine:
		case cmd_MoveToEndOfLine:
		case cmd_SelectToStartOfLine:
		case cmd_SelectToEndOfLine:
			units = bigunits;
			break;

		case cmd_MoveToPreviousWord:
		case cmd_MoveToNextWord:
		case cmd_SelectPreviousWord:
		case cmd_SelectNextWord:
			units = mediumunits;
			break;

		case cmd_MoveToPreviousSubWord:
		case cmd_MoveToNextSubWord:
		case cmd_SelectPreviousSubWord:
		case cmd_SelectNextSubWord:
			units = smallunits;
			break;
	}
	
	if (select && s != e) 
	{
		if (s == fAnchor)
			n = e;
		else
			n = s;
	}

	switch (units) 
	{
		case bigunits:
			if (dir < 0 || * (text + n) != '\n') {
				while (n + dir >= 0L && n + dir <= len) {
					if (*(text + n + dir) == '\n') {
						if (dir > 0)
							n++;
						break;
					}
					n += dir;
				}
			}
		break;
		
		case mediumunits:
			if ((dir < 0) || (!Isletter (*(text + n)))) {
				while (n + dir >= 0L && n + dir <= len &&
					   !Isletter (*(text + n + dir)))
					n += dir;
			}
			while (n + dir >= 0L && n + dir <= len) {
				if (!Isletter (*(text + n + dir))) {
					if (dir > 0)
						n++;
					break;
				}
				n += dir;
			}
		break;

		case smallunits:
			if (dir < 0)
				subword = MWEdit_PrevSubWord(&text[n], text);
			else
				subword = MWEdit_NextSubWord(&text[n], text + len);
			n = subword - text;
			break;
		
		default:
			if (select || s == e) {
				// if we are going backwards (and n isn't 0) then we need n - 1
				dir *= GlyphWidth((dir > 0 || n == 0) ? n : n-1);
				if (n + dir >= 0L && n + dir <= len)
					n += dir;
			}
	}
	
	if (select) 
	{
		if (n >= fAnchor) 
		{
			s = fAnchor;
			e = n;
		} else {
			s = n;
			e = fAnchor;
		}
	}
	else
	{
		fAnchor = s = e = n;
		fAnchorWidth = (int32) OffsetToPoint(s).x;
	}
	
	Select(s, e);

	if (e == fAnchor)
		ScrollToOffset(s);
	else
		ScrollToOffset(e);


	fAnchorSelStart = s;
	fAnchorSelEnd = e;
	if (s == e)
		fAnchor = s;
}

// ---------------------------------------------------------------------------
//		DoSpecialUpDownKey
// ---------------------------------------------------------------------------
//	Handle up or down arrow keys in combination with command-shift-option.

void
MIDETextView::DoSpecialUpDownKey(
	CommandT	inCommand)
{
	int32		s;
	int32		e;
	int32		i;
	const char *cp;
	const char *text = Text();
	int32		len = TextLength();
	bool		select = false;			// formerly shift key
	bool		bigunits = false;		// formerly command key
	bool		smallunits = false;		// formerly option key
	
	// There are four orthogonal properties to be set here:
	// towards the start or end of file, selection, bigunits or small units

	// Selection?
	switch (inCommand)
	{
		case cmd_SelectPreviousLine:
		case cmd_SelectToTopOfPage:
		case cmd_SelectToStartOfFile:
		case cmd_SelectNextLine:
		case cmd_SelectToBottomOfPage:
		case cmd_SelectToEndOfFile:
			select = true;		
			break;
	}

	// BigUnits or small units?
	switch (inCommand)
	{
		case cmd_MoveToTopOfFile:
		case cmd_SelectToStartOfFile:
		case cmd_MoveToBottomOfFile:
		case cmd_SelectToEndOfFile:
			bigunits = true;	
			break;

		case cmd_MoveToTopOfPage:
		case cmd_SelectToTopOfPage:
		case cmd_MoveToBottomOfPage:
		case cmd_SelectToBottomOfPage:
			smallunits = true;		
			break;
	}

	GetSelection(&s, &e);

	if (fAnchorSelStart != s || fAnchorSelEnd != e)
	{
		fAnchor = s;
		fAnchorWidth = (int32) OffsetToPoint(s).x;
	}

	i = fAnchorWidth;

	// Towards start or end of file?
	switch (inCommand)
	{
		// towards start of file
		case cmd_MoveToPreviousLine:
		case cmd_MoveToTopOfPage:
		case cmd_MoveToTopOfFile:
		case cmd_SelectPreviousLine:
		case cmd_SelectToTopOfPage:
		case cmd_SelectToStartOfFile:
			if (select) 
			{
				if (bigunits)			//	Select from anchor to start of file ...
				{
					s = 0L;
					e = fAnchor;
					break;				//	exit
				} else 
				if (smallunits)			//	Move up one page with shift ...
				{
					s = FindPageTop(s, e);
				} else {				//	Move up one line with shift ...
					if (e == fAnchor)
						cp = NE_PrevLine(text + s, text);
					else
						cp = NE_PrevLine (text + e, text);
				//	s = NE_PixelToText(cp, i) - text;
					BPoint	p = OffsetToPoint(cp - text, nil);
					p.x = i;
					s = PointToOffset(p);
				}

				if (s < fAnchor)
					e = fAnchor;
				else 
				{
					e = s;
					s = fAnchor;
				}
			} else {					// No Shift ...
				if (bigunits) 
				{
					s = 0L;
				} 
				else 
				if (smallunits) 
				{
					s = FindPageTop(s, e);
				}
	
				e = s;
			}
			break;

		// towards end of file
		case cmd_MoveToNextLine:
		case cmd_MoveToBottomOfPage:
		case cmd_MoveToBottomOfFile:
		case cmd_SelectNextLine:
		case cmd_SelectToBottomOfPage:
		case cmd_SelectToEndOfFile:
			if (select) 
			{
				if (bigunits)			//	Select from anchor to end of file ...
				{
					s = fAnchor;
					e = len;
					break;				//	exit
				} 
				else 
				if (smallunits)				//	Move down one page with shift ...
				{
					s = FindPageBottom(s, e);
				} 
				else 
				{						//	Move down one line with shift ...
					if (s == fAnchor)
						cp = NE_NextLineMono (text + e, text + len);
					else
						cp = NE_NextLineMono (text + s, text + len);
	
					BPoint	p = OffsetToPoint(cp - text, nil);
					p.x = i;
					s = PointToOffset(p);
				}

				if (s < fAnchor)
					e = fAnchor;
				else 
				{
					e = s;
					s = fAnchor;
				}
			} 
			else 
			{		// No shift key ...
				if (bigunits) 
				{
					e = len;
				}
				else
				if (smallunits) 
				{
					// Move down (viewlines - 1) lines ...
					e = FindPageBottom(s, e);
				}

				s = e;
			}
			break;
	}
	
	Select(s, e);

	if (e == fAnchor)
		ScrollToOffset(s, ScrollTop);
	else
		ScrollToOffset(e, ScrollTop);

	fAnchorSelStart = s;
	fAnchorSelEnd = e;
	if (s == e)
		fAnchor = s;
}

// ---------------------------------------------------------------------------
//		FindPageTop
// ---------------------------------------------------------------------------
//	Find the offset of the first character on the first whole line on the currently 
//	visible page.

int32
MIDETextView::FindPageTop(
	int32	inSelStart,
	int32	inSelEnd)
{
	BRect		bounds = Bounds();
	int32		index = IndexAtPoint(0.0, bounds.top + 1.0);
	float		lineHeight = 0.0;
	BPoint 		point = OffsetToPoint(index, &lineHeight);

	if (point.y < bounds.top)
	{
		point.y = bounds.top + lineHeight;
		index = IndexAtPoint(0.0, point.y);
	}

	// Are we already at the top of the page?
	if (inSelStart == inSelEnd && index == inSelStart)
	{
		float	v = max((float) 0.0, (float) (point.y - bounds.Height() + 1.0));
		index = IndexAtPoint(0.0, v);
		point = OffsetToPoint(index, &lineHeight);

		if (point.y < v)
			index = IndexAtPoint(0.0, v + lineHeight);
	}

	return index;
}

// ---------------------------------------------------------------------------
//		FindPageBottom
// ---------------------------------------------------------------------------
//	Find the offset of the first character on the last whole line on the currently 
//	visible page.

int32
MIDETextView::FindPageBottom(
	int32	inSelStart,
	int32	inSelEnd)
{
	BRect		bounds = Bounds();
	int32		index = IndexAtPoint(0.0, bounds.bottom - 1.0);
	float		lineHeight = 0.0;
	BPoint 		point = OffsetToPoint(index, &lineHeight);

	if (point.y + lineHeight > bounds.bottom)
	{
		point.y = bounds.bottom - lineHeight;
		index = IndexAtPoint(0.0, point.y);
	}

	// Are we already at the bottom of the page?
	if (inSelStart == inSelEnd && index == inSelStart)
	{
		float v = point.y + bounds.Height() - 1.0;
		index = IndexAtPoint(0.0, v);
		point = OffsetToPoint(index, &lineHeight);

		if (point.y + lineHeight > v) {
			index = IndexAtPoint(0.0, v - lineHeight);
		}
	}

	return index;
}

// ---------------------------------------------------------------------------
// Helper functions for DocBalance
// ---------------------------------------------------------------------------

const int32 kMaxStackSize = 512;

int32
MIDETextView::FindOpeningMatch(const char* text, int32 pos)
{
	// Search backwards for an unmatched {, [, or (
	// If we started outside a comment, then ignore
	// braces inside comments.  However, if we started
	// inside a comment, adopt the style of the first
	// hit.  (This allows us to click somewhere
	// inside this comment and still balance for the
	// function as a whole.  Or inside this parenthetical
	// statement and then balance again for the function
	// as a whole -- except the first line of this comment
	// messes it up because of the brace, bracket, and parenthesis!)
	
	char stack[kMaxStackSize];
	int32 sp = 0;
	bool found = false;
	bool firstHit = true;
	bool startInComment = this->InCommentOrString(text, pos);
	
	while (pos >= 0 && found == false) {
		char c = text[--pos];
		switch (c) {
			case '{':
			case '[':
			case '(':
			{
				// set up how we are going to continue based
				// on our first hit (see comments above)...
				if (firstHit == true && startInComment == true) {
					startInComment = this->InCommentOrString(text, pos);
				}
				firstHit = false;
				
				if (startInComment == this->InCommentOrString(text, pos)) {
					if (sp == 0) {
						found = true;
					}
					else {
						char topOfStack = stack[--sp];
						if (c == '{' && topOfStack != '}' ||
								c == '[' && topOfStack != ']' ||
								c == '(' && topOfStack != ')') {
							throw "mismatched pairing";
						}
					}
				}
				break;
			}
			
			case ')':
			case ']':
			case '}':
				// set up how we are going to continue based
				// on our first hit (see comments above)...
				if (firstHit == true && startInComment == true) {
					startInComment = this->InCommentOrString(text, pos);
				}
				firstHit = false;

				if (startInComment == this->InCommentOrString(text, pos)) {
					if (sp >= kMaxStackSize) {
						throw "stack overflow";
					}
					stack[sp++] = c;
				}
				break;
		}
	}

	return pos;
}

// ---------------------------------------------------------------------------

int32
MIDETextView::FindClosingMatch(const char* text, int32 pos)
{
	// Search forward for the match of the character
	// currently at text[pos]
	// ...don't match inside comments/strings unless our 
	// opening character started out in a comment/string
	
	bool startInComment = this->InCommentOrString(text, pos);
	char stack[kMaxStackSize];
	int32 sp = 0;
	bool found = false;
	
	while (found == false) {
		char c = text[pos++];
		switch (c) {
			case 0:
				throw "end of file";
				break;

			case '}':
			case ']':
			case ')':
			{
				if (startInComment == this->InCommentOrString(text, pos-1)) {
					// since our starting pos should
					// be at the beginning "brace"
					// we should have one matching
					// element on the stack
					if (sp == 0) {
						throw "no match";
					}
					char topOfStack = stack[--sp];
					if (c == '}' && topOfStack != '{' ||
							c == ']' && topOfStack != '[' ||
							c == ')' && topOfStack != '(') {
						throw "mismatched pairing";
					}				
					// check if we have popped off our match
					if (sp == 0) {
						// we have already incremented pos
						// get it back to closing character position
						pos -= 1;;
						found = true;
					}
				}
				break;
			}
			
			case '{':
			case '(':
			case '[':
			{
				if (startInComment == this->InCommentOrString(text, pos-1)) {
					if (sp >= kMaxStackSize) {
						throw "stack overflow";
					}
					stack[sp++] = c;
				}
				break;
			}
		}
	}
	
	return pos;
}

// ---------------------------------------------------------------------------

void 
MIDETextView::DocBalance()
{
	// Balance the first encountered ( ) or [ ] or { }
	// This is done by searching backwards for the first found opening "brace"
	// and then searching forward from that point for the closing "brace".
	
	const char *text = this->Text();
	int32 start = 0;
	int32 end = 0;
	this->GetSelection(&start, &end);

	// handle the case where we are currently selecting a valid range by
	// bumping the start and end markers
	// (else) handle the case where we have the opening brace selected
	// already by bumping start so that we match that one
	// (if we have a closing brace selected, the matching code as is
	// will match that one)
	if (start != 0 && ((text[start - 1] == '{' && text[end] == '}') ||
				   (text[start - 1] == '(' && text[end] == ')') ||
				   (text[start - 1] == '[' && text[end] == ']'))) {
		// the entire selection is currently surrounded by braces
		// bump to include them so we find the next enclosing pair
		start -= 1; 
		end += 1;
	} 
	else if (end == start + 1) {
		if (text[start] == '{' || text[start] == '(' || text[start] == '[') {
			start += 1;
		}
	}

	try {
		int32 openPos = this->FindOpeningMatch(text, start);
		int32 closePos = this->FindClosingMatch(text, openPos);
		// select the contents of "braces" not the "braces" themselves
		this->Select(openPos+1, closePos);
	}
	catch (...) {
		// troubles during the matching of open or close...
		// give up
		beep();
	}
}	

// ---------------------------------------------------------------------------
//		BalanceWhileTyping
// ---------------------------------------------------------------------------
//	We're being told to set our flashwhentyping settings.
//	The value in the settings window is in hundreths of a second.  We convert
//	here to microseconds for the call to snooze.

void
MIDETextView::BalanceWhileTyping(
	BMessage&	inMessage)
{
	int32		val;

	if (B_NO_ERROR == inMessage.FindInt32(kFlashDelay, &val))
	{
		fFlashWhenTyping = true;
		
		fFlashingDelay = val * 10000;
	}
	else
	{
		fFlashWhenTyping = false;
	}
}

/****************************************************************/
/* Purpose..: Flashes the previous open accolade '(','[' or '{' */
/*			  I know that an accolade is only a '{', HUMOR ME!! */
/* Input....: Pointer to the document ...						*/
/* Input....: What type of accolade to flash ...				*/
/* Returns..: ---												*/
/****************************************************************/

void 
MIDETextView::FlashPreviousAccolade(
	AccoladeType	mode)
{
	short 			openAccolades = 0;
	int32 			curPos;
	int32 			originalPos;
	int32			selStart;
	int32			selEnd;
	const char *	theText = Text();

	GetSelection(&selStart, &selEnd);

	if (selStart != selEnd)
		return;

	curPos = originalPos = selStart;

	//	Skip the previous character ...
	curPos--;

	if (this->InCommentOrString(theText, curPos) == true) {
		// don't bother if we are currently in a string or comment...
		return;
	}

	while (--curPos >= 0) {
		switch (mode) {
			case ACCOLADES:
				switch (*(theText + curPos)) {
					case 0x7d:				// '}'
						if (this->InCommentOrString(theText, curPos) == false) {
							openAccolades++;
						}
						break;
					case 0x7b:				// '{'
						if (this->InCommentOrString(theText, curPos) == false) {
							openAccolades--;
						}
						break;
					default:
						break;
				}
				break;
			case SQUAREBRACKETS:
				switch (*(theText + curPos)) {
					case 0x5d:				// ']'
						if (this->InCommentOrString(theText, curPos) == false) {
							openAccolades++;
						}
						break;
					case 0x5b:				// '['
						if (this->InCommentOrString(theText, curPos) == false) {
							openAccolades--;
						}
						break;
					default:
						break;
				}
				break;
			case ROUNDBRACKETS:
				switch (*(theText + curPos)) {
					case 0x29:				// ')'
						if (this->InCommentOrString(theText, curPos) == false) {
							openAccolades++;
						}
						break;
					case 0x28:				// '('
						if (this->InCommentOrString(theText, curPos) == false) {
							openAccolades--;
						}
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
		if (openAccolades == -1) {
			//	Got it ... Now highlight it ...
			Select(curPos, curPos + 1L);
			ScrollToSelection();

			snooze(fFlashingDelay);

			//	Then return to where you were ...
			Select(originalPos, originalPos);
			ScrollToSelection();

			return;
		}
	}

	//	reached beginning of document and didn't find it ...
	beep();
}

// ---------------------------------------------------------------------------

bool
MIDETextView::InCommentOrString(const char* text, int32 position)
{
	// small utility function that is used by FlashPreviousAccolade 
	// to determine if a matching character is in a comment or 
	// literal string and should be ignored

	// We are in a comment or string if our style reflects that state
	// ...or... if we are immediately surrounded by single quotes
	
	int32 prev = position > 0 ? position - 1 : position;
	int32 next = position + 1;
	
	TextStateT style = fStyler.GetStyleAtPosition(position);
	return (style == CCommentStyle || style == CppCommentStyle || style == StringStyle
			|| (text[prev] == '\'' && text[next] == '\''));
}

// ---------------------------------------------------------------------------
//		ScrollToFunction
// ---------------------------------------------------------------------------
//	Scroll to a function name so that any comments preceding it are at the
//	top of the window.

void
MIDETextView::ScrollToFunction(
	int32 	inSelStart, 
	int32 	inSelEnd)
{
	const int32		commentsOffset = AdjustOffsetForFunctionComments(inSelStart);

	Select(inSelStart, inSelEnd);

	ScrollToOffset(TextLength(), ScrollTop);
	ScrollToOffset(commentsOffset, ScrollTop);
	ScrollToOffset(inSelStart, ScrollTop);
}

// ---------------------------------------------------------------------------
//		# AdjustOffsetForFunctionComments
// ---------------------------------------------------------------------------
//	Adjust the function's start offset for any preceeding comments ...

int32 
MIDETextView::AdjustOffsetForFunctionComments(
	int32	inOffset)
{
	const char*		text;
	const char*		textstart;
	bool			hascomment;
	bool			done = false;
	bool			commentfound = false;

	textstart = Text();
	text = textstart + inOffset;
	text = MWEdit_LineStart(text, textstart);

	const int32		savedoffset = text - textstart;

	while (text > textstart && !done)
	{	//	Look backwards for a comment ...
		text--;
		hascomment = false;

		while (text > textstart && *text != EOL_CHAR && !done)
		{	//	check if the line contains a comment ...
			if (*text == '/')
			{
				if (*(text - 1) == '*')
				{	//	Skip back to the start of the comment ...
					text--;
					while (text > textstart)
					{
						if (*text == '*' && *(text - 1) == '/')
						{
							text--;
							break;
						}
						text--;
					}
					text--;
					hascomment = true;
				}
				else if (*(text - 1) == '/')
				{	//	Skip the comment ...
					text -= 2;
					hascomment = true;
				}
				else
				{
					text--;
				}
			}
			else if (*text == ';' || *text == '}')
			{
				if (hascomment)
					hascomment = false;
				text = MWEdit_NextLineMono(text, textstart + TextLength());
				done = true;
			}
			else
			{
				text--;
			}
		}
		if (!hascomment && commentfound)
		{
			text++;
			text = MWEdit_NextLineMono(text, textstart + TextLength());
			done = true;
		}
		else if (hascomment && !commentfound)
		{	//	we can start looking for the last line without a comment ...
			commentfound = true;
		}
	}

	if (text <= textstart || !commentfound)
	{	//	we've gone too far ...
		text = textstart + savedoffset;
	}

	return (text - textstart);
}

// ---------------------------------------------------------------------------
//		HandleModification
// ---------------------------------------------------------------------------

void
MIDETextView::HandleModification()
{
	SetDirty();
}

// ---------------------------------------------------------------------------
//		HasSelection
// ---------------------------------------------------------------------------

bool
MIDETextView::HasSelection()
{
	int32	selStart;
	int32	selEnd;
	
	GetSelection(&selStart, &selEnd);
	
	return selEnd != selStart;
}

// ---------------------------------------------------------------------------
//		SetAutoindent
// ---------------------------------------------------------------------------

void
MIDETextView::SetAutoindent(
	bool	inAutoIndent)
{
	fAutoIndent = inAutoIndent;
}

// ---------------------------------------------------------------------------
//		UpdateFontInfo
// ---------------------------------------------------------------------------

void
MIDETextView::UpdateFontInfo(
	const font_family	inFontFamily,
	const font_style	inFontStyle,
	float				inFontSize,
	float				inTabSize,
	bool				inDoAutoIndent)
{
	if (! fUsesSyntaxColoring)
	{
		STEStyleRange		range;
		STEStyle&			style = range.runs[0].style;

		range.count = 1;
		range.runs[0].offset = 0;
		style.SetFamilyAndStyle(inFontFamily, inFontStyle);
		style.SetSize(inFontSize);
		style.color = black;
		style.extra = 0;

		mStyles.SetNullStyle(doAll, &style);
		if (TextLength() > 0)
			SetStyleRange(0, TextLength(), &range, true);
	
		SetTabWidth(inTabSize * style.StringWidth("M"));
	}

	SetAutoindent(inDoAutoIndent);
}

// ---------------------------------------------------------------------------
//		UpdateSyntaxStyleInfo
// ---------------------------------------------------------------------------
//	Pass the style info to the syntax style object.

void
MIDETextView::UpdateSyntaxStyleInfo(
	const SyntaxStylePrefs&	inPrefs,
	float	inTabSize)
{
ASSERT(sizeof(SyntaxStylePrefs) == 688);
ASSERT(sizeof(FontInfo) == 136);

	STEStyle		style;

	style.SetFamilyAndStyle(inPrefs.text.family, inPrefs.text.style);
	style.SetSize(inPrefs.text.size);
	style.color = inPrefs.text.color;
	style.SetSpacing(B_BITMAP_SPACING);
	fStyler.SetStyle(sTextStyle, style);
	
	SetTabWidth(inTabSize * style.StringWidth("M"));

	style.SetFamilyAndStyle(inPrefs.comments.family, inPrefs.comments.style);
	style.SetSize(inPrefs.comments.size);
	style.color = inPrefs.comments.color;
	style.SetSpacing(B_BITMAP_SPACING);
	fStyler.SetStyle(sCommentStyle, style);

	style.SetFamilyAndStyle(inPrefs.strings.family, inPrefs.strings.style);
	style.SetSize(inPrefs.strings.size);
	style.color = inPrefs.strings.color;
	style.SetSpacing(B_BITMAP_SPACING);
	fStyler.SetStyle(sStringStyle, style);

	style.SetFamilyAndStyle(inPrefs.keywords.family, inPrefs.keywords.style);
	style.SetSize(inPrefs.keywords.size);
	style.color = inPrefs.keywords.color;
	style.SetSpacing(B_BITMAP_SPACING);
	fStyler.SetStyle(sKeywordStyle, style);

	fUsesSyntaxColoring = inPrefs.useSyntaxStyling;

	if (fUsesSyntaxColoring)
	{
		ParseText();
		ScrollToSelection();	// This isn't really the right place for this
	}
}

// ------------------------------------------------------------
// 	ScrollToOffset
// ------------------------------------------------------------
// Scroll the view so that inOffset is fully visible

void
MIDETextView::ScrollToOffset(
	int32		inOffset,
	ScrollType 	inType)
{
	if (inOffset < 0) {
		inOffset = 0;
	}
	if (inOffset > TextLength()) {
		inOffset = TextLength();
	}
	
	if (inType == ScrollMiddle) {
		STEngine::ScrollToOffset(inOffset);
	}
	else {
		BRect			bounds = Bounds();
		float			lineHeight = 0.0;
		BPoint 			point = this->OffsetToPoint(inOffset, &lineHeight);
		BScrollBar*		hScroller = ScrollBar(B_HORIZONTAL);
		BScrollBar*		vScroller = ScrollBar(B_VERTICAL);
		
		if (hScroller != NULL) {
			if (point.x < bounds.left) {
				hScroller->SetValue(point.x);
			}
			else if (point.x >= bounds.right) {
				hScroller->SetValue(point.x - bounds.Width());
			}
		}
		
		// ScrollTop actually handles both Scroll-index-to-Top and 
		// Scroll-index-to-Bottom
		// If the point we want visible is above the current view,
		// we scroll so that point is at the top of the view
		// ...if it is below the current view, we scroll so that
		// point is at the bottom of the view
		
		if (vScroller != NULL) {
			if (point.y < bounds.top) {
				vScroller->SetValue(point.y);
			}
			else if (point.y + lineHeight > bounds.bottom) {
				vScroller->SetValue(point.y - bounds.Height() + lineHeight);
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		UseSyntaxColoring
// ---------------------------------------------------------------------------

void
MIDETextView::SetColorSpace(
	color_space 	inColorSpace)
{
	if (mOffscreen == nil || mOffscreen->ColorSpace() != inColorSpace)
	{
		UseOffscreen(inColorSpace);
	}
}

// ---------------------------------------------------------------------------
//		UseSyntaxColoring
// ---------------------------------------------------------------------------

void
MIDETextView::UseSyntaxColoring(
	bool	inUseIt)
{
	if (inUseIt != fUsesSyntaxColoring)
	{
		fUsesSyntaxColoring = inUseIt;
		if (inUseIt)
			ParseText();
	}
}

// ------------------------------------------------------------
// 	ParseText
// ------------------------------------------------------------

void
MIDETextView::ParseText()
{
	if (fUsesSyntaxColoring)
	{
		fStyler.ParseAllText();
		mStyles.InvalidateNullStyle();
	}
}

// ---------------------------------------------------------------------------
//		SetSuffixType
// ---------------------------------------------------------------------------
//	Adjusts the kind of syntax styling to be done by this object.
//	Doesn't update the view.  Call ParseAllText for that.

void
MIDETextView::SetSuffixType(
	SuffixType		inSuffix)
{
	if (fStyler.SetSuffixType(inSuffix))
		ParseText();
}
