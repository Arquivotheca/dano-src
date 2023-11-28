// ============================================================
//  STEngine.cpp	©1996 Hiroshi Lockheimer
// ============================================================
// 	STE Version 1.0a5

#include "STEngine.h"

#include <Bitmap.h>
#include <Clipboard.h>
#include <Region.h>
#include <ScrollBar.h>
#include <Window.h>

// === Static Member Variables ===

STEWidthBuffer	STEngine::sWidths;


// ------------------------------------------------------------
// 	STEngine
// ------------------------------------------------------------
// Constructor
//
// textRect is in local coordinates
// nullStyle is the default style, all fields must be valid
//
// Initially the text is selectable, editable, and will wrap
// The initial tab width is that of four spaces in 'Kate'
// An offscreen bitmap is not used by default

STEngine::STEngine(
	BRect				frame,
	const char			*name,
	BRect				textRect,
	ConstSTEStylePtr	nullStyle,
	uint32 				resizeMask, 
	uint32				flags)
		: BView(frame, name, resizeMask, flags),
		  mStyles(nullStyle)
{
	mTextRect = textRect;
	mSelStart = 0;
	mSelEnd = 0;
	mCaretVisible = false;
	mCaretTime = 0;
	mClickOffset = -1;
	mClickCount = 0;
	mClickTime = 0;
	mDragOffset = -1;
	mDragOwner = false;
	mActive = false;
	mTabWidth = 28.0;
	mSelectable = true;
	mEditable = true;
	mWrap = true;
	mOffscreen = NULL;
}

// ------------------------------------------------------------
// 	~STEngine
// ------------------------------------------------------------
// Destructor

STEngine::~STEngine()
{
	if (mOffscreen != NULL) {
		mOffscreen->Lock();
		delete (mOffscreen);
	}
}

// ------------------------------------------------------------
// 	SetText
// ------------------------------------------------------------
// Replace the current text with inText
//
// Pass NULL for inStyles if there is no style data

void
STEngine::SetText(
	const char				*inText,
	int32					inLength,
	ConstSTEStyleRangePtr	inStyles)
{	
	// hide the caret/unhilite the selection
	HideSelection();

	// remove data from buffer
	if (mText.Length() > 0)
		RemoveRange(0, mText.Length());
		
	InsertAt(inText, inLength, 0, inStyles);
	
	mSelStart = mSelEnd = 0;	

	// recalc line breaks and draw the text
	Refresh(0, inLength, true, true);
	
	HandleModification();

	// draw the caret
	ShowSelection();
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

void
STEngine::Insert(
	const char				*inText,
	int32					inLength,
	ConstSTEStyleRangePtr	inStyles)
{
	// do we really need to do anything?
	if (inLength < 1)
		return;
	
	// hide the caret/unhilite the selection
	HideSelection();
	
	// copy data into buffer
	InsertAt(inText, inLength, mSelStart, inStyles);

	// offset the caret/selection
	int32 saveStart = mSelStart;
	mSelStart += inLength;
	mSelEnd += inLength;

	// recalc line breaks and draw the text
	Refresh(saveStart, mSelStart, true, true);
	
	HandleModification();

	// draw the caret/hilite the selection
	ShowSelection();
}

// ------------------------------------------------------------
// 	Delete
// ------------------------------------------------------------
// Delete the current selection

void
STEngine::Delete()
{
	// anything to delete?
	if (mSelStart == mSelEnd)
		return;
		
	// hide the caret/unhilite the selection
	HideSelection();
	
	// remove data from buffer
	RemoveRange(mSelStart, mSelEnd);
	
	// collapse the selection
	mSelEnd = mSelStart;
	
	// recalc line breaks and draw what's left
	Refresh(mSelStart, mSelEnd, true, true);
	
	HandleModification();
	
	// draw the caret
	ShowSelection();
}

// ------------------------------------------------------------
// 	Text
// ------------------------------------------------------------
// Return a pointer to the text, NULL if error
//
// Do not free the returned text or alter it in any way
//
// The pointer that is returned may become invalid after a
// subsequent call to any other STEngine function
// Copy it into your own buffer or use STEngine::GetText() if 
// you need the text for an extended period of time
//
// The text is null terminated

const char*
STEngine::Text()
{
	return (mText.Text());
}


// ------------------------------------------------------------
// 	TextLength
// ------------------------------------------------------------
// Return the length of the text buffer
//
// The length does not include the null terminator

int32
STEngine::TextLength() const
{
	return (mText.Length());
}


// ------------------------------------------------------------
// 	GetText
// ------------------------------------------------------------
// Copy into buffer up to length characters of the text 
// starting at offset
//
// The text is null terminated

void
STEngine::GetText(
	char	*buffer,
	int32 	offset,
	int32 	length) const
{
	int32 textLen = mText.Length();
	
	if ((offset < 0) || (offset > (textLen - 1))) {
		buffer[0] = '\0';
		return;
	}
		
	length = ((offset + length) > textLen) ? textLen - offset : length;
	for (int32 i = 0; i < length; i++)
		buffer[i] = mText[offset + i];
	buffer[length] = '\0';
}

// ------------------------------------------------------------
// 	ByteAt
// ------------------------------------------------------------
// Return the byte at offset

char
STEngine::ByteAt(
	int32	offset) const
{
	if ((offset < 0) || (offset > (mText.Length() - 1)))
		return ('\0');
		
	return (mText[offset]);
}


// ------------------------------------------------------------
// 	CountLines
// ------------------------------------------------------------
// Return the number of lines

int32
STEngine::CountLines() const
{
	int32 numLines = mLines.NumLines();
	
	int32 textLength = mText.Length();
	if (textLength > 0) {
		// add a line if the last character is a newline
		if (mText[textLength - 1] == '\n')
			numLines++;
	}
	
	return (numLines);
}


// ------------------------------------------------------------
// 	CurrentLine
// ------------------------------------------------------------
// Return the line number of the caret or the beginning of
// the selection
//
// Line numbers start at 0

int32
STEngine::CurrentLine() const
{
	int32 lineNum = OffsetToLine(mSelStart);
	
	int32 textLength = mText.Length();
	if ((textLength > 0) && (mSelStart == textLength)) {
		// add a line if the last character is a newline
		if (mText[textLength - 1] == '\n')
			lineNum++;
	}
	
	return (lineNum);
}


// ------------------------------------------------------------
// 	GoToLine
// ------------------------------------------------------------
// Move the caret to the beginning of lineNum
//
// This function does not automatically scroll the view, 
// use ScrollToSelection() if you want to ensure that lineNum 
// is visible
//
// Line numbers start at 0

void
STEngine::GoToLine(
	int32	lineNum)
{
	// hide the caret/unhilite the selection
	HideSelection();
	
	int32 saveLineNum = lineNum;
	int32 maxLine = mLines.NumLines() - 1;
	lineNum = (lineNum > maxLine) ? maxLine : lineNum;
	lineNum = (lineNum < 0) ? 0 : lineNum;
	
	mSelStart = mSelEnd = mLines[lineNum]->offset;
	
	int32 textLength = mText.Length();
	if ((textLength > 0) && (saveLineNum > maxLine)) {
		// add a line if the last character is a newline
		if (mText[textLength - 1] == '\n')
			mSelStart = mSelEnd = textLength;
	}
	
	// InvertCaret();
}

// ------------------------------------------------------------
// 	Cut
// ------------------------------------------------------------
// Copy the current selection into the clipboard and delete it
// from the buffer

void
STEngine::Cut()
{
	if (be_clipboard->Lock())
	{
		be_clipboard->Clear();
	
		// add text
		BMessage*	clipData = be_clipboard->Data();
		clipData->AddData("text/plain", B_MIME_TYPE, mText.Text() + mSelStart, mSelEnd - mSelStart);
	
	#if 0
		// no styles for now
		// add corresponding styles
		int32 				length = 0;
		STEStyleRangePtr	styles = GetStyleRange(mSelStart, mSelEnd, &length);			
		
		// reset the extra fields, override if you don't like this behavior
		for (int32 i = 0; i < styles->count; i++)
			styles->runs[i].style.extra = 0;
		
	//	clipData->AddData(STE_STYLE_TYPE, styles, length);
		
		free(styles);
	#endif
	
		be_clipboard->Commit();
		be_clipboard->Unlock();
		
		Delete();
	}
}

// ------------------------------------------------------------
// 	Copy
// ------------------------------------------------------------
// Copy the current selection into the clipboard
//
// The extra field in the clipboard will be reset to 0

void
STEngine::Copy()
{
	if (be_clipboard->Lock())
	{
		be_clipboard->Clear();
	
		// add text
		BMessage*	clipData = be_clipboard->Data();
		clipData->AddData("text/plain", B_MIME_TYPE, mText.Text() + mSelStart, mSelEnd - mSelStart);
	
	#if 0
		// no styles for now
		// add corresponding styles
		int32 				length = 0;
		STEStyleRangePtr	styles = GetStyleRange(mSelStart, mSelEnd, &length);			
		
		// reset the extra fields, override if you don't like this behavior
		for (int32 i = 0; i < styles->count; i++)
			styles->runs[i].style.extra = 0;
		
	//	clipData->AddData(STE_STYLE_TYPE, styles, length);
		
		free(styles);
	#endif
	
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
}

// ------------------------------------------------------------
// 	Paste
// ------------------------------------------------------------
// Copy the contents of the clipboard into the text buffer

void
STEngine::Paste()
{
	// do we really want what's in the clipboard?
	if (!CanPaste())
		return;
	
	// unhilite and delete the selection
	if (mSelStart != mSelEnd) {
		if (mActive)
			DrawSelection(mSelStart, mSelEnd);
	
		RemoveRange(mSelStart, mSelEnd);
		mSelEnd = mSelStart;
	}
		
	if (be_clipboard->Lock())
	{
		// get data from clipboard
		ssize_t 		textLen = 0;
		const char	*	text;
		
		if (B_NO_ERROR == be_clipboard->Data()->FindData("text/plain", B_MIME_TYPE, (const void**)&text, &textLen))
		{
			ssize_t 			styleLen = 0;
			STEStyleRangePtr	styles = NULL;
	//		styles = (STEStyleRangePtr)be_clipboard->FindData(STE_STYLE_TYPE, &styleLen);
			// for now we don't cut and paste styles
	
			// copy text and styles into the buffers
			Insert(text, textLen, styles);
		}
	
		be_clipboard->Unlock();
	}
}

// ------------------------------------------------------------
// 	Clear
// ------------------------------------------------------------
// Delete the current selection

void
STEngine::Clear()
{
	Delete();
}

// ------------------------------------------------------------
// 	SelectAll
// ------------------------------------------------------------
// Select everything

void
STEngine::SelectAll()
{
	if (mSelectable)
		Select(0, mText.Length());
}

// ------------------------------------------------------------
// 	CanPaste
// ------------------------------------------------------------
// Return true if the clipboard contains data that can be inserted 

bool
STEngine::CanPaste()
{
	if (!mEditable)
		return (false);
		
	bool result = false;
	
	if (be_clipboard->Lock())
	{
		const char *	text;
		ssize_t			textLen;
		
		if (B_NO_ERROR == be_clipboard->Data()->FindData("text/plain", B_MIME_TYPE, (const void**)&text, &textLen))
			result = true;
		
		be_clipboard->Unlock();
	}
	
	return (result);
}

// ------------------------------------------------------------
// 	CanDrop
// ------------------------------------------------------------
// Return true if the message contains data that can be dropped

bool
STEngine::CanDrop(
	const BMessage	*inMessage)
{
	if (!mEditable)
		return (false);
		
	if (inMessage->HasData("text/plain", B_MIME_TYPE))
		return (true);
		
	return (false);
}

// ------------------------------------------------------------
// 	Select
// ------------------------------------------------------------
// Select (hilite) a range of text

void
STEngine::Select(
	int32	startOffset,
	int32 	endOffset)
{
	// pin offsets at reasonable values
	if (startOffset < 0)
		startOffset = 0;
	else
	if (startOffset > mText.Length())
		startOffset = mText.Length();
	if (endOffset < 0)
		endOffset = 0;
	else
	if (endOffset > mText.Length())
		endOffset = mText.Length();

	// a negative selection?
	// or is the new selection any different from the current selection?
	if ((startOffset > endOffset) || ((startOffset == mSelStart) && (endOffset == mSelEnd)))
		return;
	
	mStyles.InvalidateNullStyle();
	
	// hide the caret
	if (mCaretVisible)
		InvertCaret();

	ConstSTELinePtr		line;
	int32 selStart = mSelStart;
	int32 selEnd = mSelEnd;
	int32 startLine;
	int32 endLine;

	mSelStart = startOffset;
	mSelEnd = endOffset;

	if (startOffset == endOffset) {
		if (selStart != selEnd) {
			// unhilite the selection
			if (mSelectable)
			{
				startLine = OffsetToLine(selStart);
				endLine = OffsetToLine(selEnd > 1 && mText[selEnd - 1] == '\n' ? selEnd - 1 : selEnd);
				line = mLines[startLine];

				DrawLines(startLine, endLine, selStart - line->offset, true);
			}
		}
		if ((mActive) /*&& (mEditable)*/)
			InvertCaret();
	}
	else {
		if ((mSelectable)) {
			// does the new selection overlap with the current one?
			if ( ((startOffset < selStart) && (endOffset < selStart)) ||
				 ((endOffset > selEnd) && (startOffset > selEnd)) ) {
				// they don't overlap, don't bother with stretching
				// thanks to Brian Stern for the code snippet
				startLine = OffsetToLine(selStart);
				endLine = OffsetToLine(selEnd > 1 && mText[selEnd - 1] == '\n' ? selEnd - 1 : selEnd);
				line = mLines[startLine];

				DrawLines(startLine, endLine, selStart - line->offset, true);

				startLine = OffsetToLine(startOffset);
				endLine = OffsetToLine(endOffset > 1 && mText[endOffset - 1] == '\n' ? endOffset - 1 : endOffset);
				line = mLines[startLine];

				DrawLines(startLine, endLine, startOffset - line->offset, true);
			}
			else {
				// stretch the selection, draw only what's different
				int32 start, end;
				if (startOffset != selStart) {
					// start of selection has changed
					if (startOffset > selStart) {
						start = selStart;
						end = startOffset;
					}
					else {
						start = startOffset;
						end = selStart;
					}

					startLine = OffsetToLine(start);
					endLine = OffsetToLine(end > 1 && mText[end - 1] == '\n' ? end - 1 : end);

					line = mLines[startLine];
	
					DrawLines(startLine, endLine, start - line->offset, true);
				}
				
				if (endOffset != selEnd) {
					// end of selection has changed
					if (endOffset > selEnd) {
						start = selEnd;
						end = endOffset;
					}
					else {
						start = endOffset;
						end = selEnd;
					}

					startLine = OffsetToLine(start);
					endLine = OffsetToLine(end > 1 && mText[end - 1] == '\n' ? end - 1 : end);
					line = mLines[startLine];
	
					DrawLines(startLine, endLine, start - line->offset, true);
				}
			}
		}
	}
}

// ------------------------------------------------------------
// 	HideSelection
// ------------------------------------------------------------
// Hide the visible selection.

void
STEngine::HideSelection()
{
	if (mActive) {
		if (mSelStart != mSelEnd)
		{
			const int32		startLine = OffsetToLine(mSelStart);
			const int32		endLine = OffsetToLine(mSelEnd > 1 && mText[mSelEnd - 1] == '\n' ? mSelEnd - 1 : mSelEnd);
			mActive = false;
			DrawLines(startLine, endLine, -1, true);
			mActive = true;
		}
		else {
			if (mCaretVisible)
				InvertCaret();
		}
	}
}

// ------------------------------------------------------------
// 	ShowSelection
// ------------------------------------------------------------
// Restore the visible selection.

void
STEngine::ShowSelection()
{
	if (mActive) {
		if (mSelStart != mSelEnd)
		{
			const int32		startLine = OffsetToLine(mSelStart);
			const int32		endLine = OffsetToLine(mSelEnd > 1 && mText[mSelEnd - 1] == '\n' ? mSelEnd - 1 : mSelEnd);
			DrawLines(startLine, endLine);
		}
		else {
			if (mCaretVisible)
				InvertCaret();
		}
	}
}

// ------------------------------------------------------------
// 	GetSelection
// ------------------------------------------------------------
// Pass back the current selection offsets

void
STEngine::GetSelection(
	int32	*outStart,
	int32 	*outEnd) const
{
	*outStart = mSelStart;
	*outEnd = mSelEnd;
}

// ------------------------------------------------------------
// 	SetStyle
// ------------------------------------------------------------
// Set the style for the current selection
//
// inMode specifies the pertinent fields of inStyle
// It can be one (or a combination) of the following:
//
//	doFont			-	set font
//	doSize			-	set size
//	doShear			-	set shear
//	doUnderline		-	set underline
//	doColor			-	set color
//	doExtra			-	set the extra field
//	doAll			-	set everything
//	addSize			-	add size value
	
void
STEngine::SetStyle(
	uint32				inMode,
	ConstSTEStylePtr	inStyle)
{
	// hide the caret/unhilite the selection
	HideSelection();
	
	// add the style to the style buffer
	mStyles.SetStyleRange(mSelStart, mSelEnd, mText.Length(),
						  inMode, inStyle);
						
	if ((inMode & doFont) || (inMode & doSize))
		// recalc the line breaks and redraw with new style
		Refresh(mSelStart, mSelEnd, mSelStart != mSelEnd, false);
	else
		// the line breaks wont change, simply redraw
		DrawLines(OffsetToLine(mSelStart), OffsetToLine(mSelEnd), 
				  mSelStart, true);
	
	HandleModification();
	
	// draw the caret/hilite the selection
	ShowSelection();
}

// ------------------------------------------------------------
// 	GetStyle
// ------------------------------------------------------------
// Get the style at inOffset

void
STEngine::GetStyle(
	int32		inOffset,
	STEStylePtr	outStyle) const
{
	mStyles.GetStyle(inOffset, outStyle);
}

// ------------------------------------------------------------
// 	SetStyleRange
// ------------------------------------------------------------
// Set the styles of a range of text

void
STEngine::SetStyleRange(
	int32					startOffset,
	int32 					endOffset,
	ConstSTEStyleRangePtr	inStyles,
	bool					inRefresh)
{
	int32 numStyles = inStyles->count;
	if (numStyles < 1)
		return;
			
	if (inRefresh) {
		// hide the caret/unhilite the selection
		if (mActive) {
			if (mSelStart != mSelEnd)
				DrawSelection(mSelStart, mSelEnd);
			else {
				if (mCaretVisible)
					InvertCaret();
			}
		}
	}
	
	// pin offsets at reasonable values
	int32 textLength = mText.Length();
	startOffset = (startOffset < 0) ? 0 : startOffset;
	endOffset = (endOffset < 0) ? 0 : endOffset;
	endOffset = (endOffset > textLength) ? textLength : endOffset;
	
	// loop through the style runs
	ConstSTEStyleRunPtr	theRun = &inStyles->runs[0];
	for (int32 index = 0; index < numStyles; index++) {
		int32 fromOffset = theRun->offset + startOffset;
		int32 toOffset = endOffset;
		if ((index + 1) < numStyles) {
			toOffset = (theRun + 1)->offset + startOffset;
			toOffset = (toOffset > endOffset) ? endOffset : toOffset;
		}

		mStyles.SetStyleRange(fromOffset, toOffset, textLength,
						  	  doAll, &theRun->style);
						
		theRun++;
	}
	
	mStyles.InvalidateNullStyle();
	
	if (inRefresh) {
		Refresh(startOffset, endOffset, true, false);
		
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

// ------------------------------------------------------------
// 	GetStyleRange
// ------------------------------------------------------------
// Return the styles of a range of text
//
// You are responsible for freeing the buffer that is returned
//
// Pass NULL for outLength if you are not interested in the
// length of the buffer

STEStyleRangePtr
STEngine::GetStyleRange(
	int32	startOffset,
	int32	endOffset,
	int32	*outLength) const
{
	STEStyleRangePtr result = mStyles.GetStyleRange(startOffset, endOffset - 1);
	
	if (outLength != NULL)
		*outLength = sizeof(int32) + (sizeof(STEStyleRun) * result->count);
	
	return (result);
}

// ------------------------------------------------------------
// 	IsContinuousStyle
// ------------------------------------------------------------
// Return whether the attributes that are set in ioMode are 
// continuous over the selection range 
//
// Any attributes in ioMode that aren't contiuous will be cleared
//
// outStyle will be set with the continuous attributes only

bool
STEngine::IsContinuousStyle(
	uint32		*ioMode,
	STEStylePtr	outStyle) const
{
	bool result = true;
	
	if ((mSelStart == mSelEnd) && (mStyles.IsValidNullStyle())) {
		STEStyle nullStyle = mStyles.GetNullStyle();
		mStyles.SetStyle(*ioMode, &nullStyle, outStyle);
	}
	else
		result = mStyles.IsContinuousStyle(ioMode, outStyle, 
									   	   mSelStart, mSelEnd);
			
	return (result);
}

// ------------------------------------------------------------
// 	OffsetToLine
// ------------------------------------------------------------
// Return the number of the line in the line array that 
// contains offset
// 
// Line numbers start at 0

int32
STEngine::OffsetToLine(
	int32	offset) const
{
	return (mLines.OffsetToLine(offset));
}

// ------------------------------------------------------------
// 	PixelToLine
// ------------------------------------------------------------
// Return the number of the line at the vertical location
// pixel should be in local coordinates
// 
// Line numbers start at 0

int32
STEngine::PixelToLine(
	float	pixel) const
{
	return (mLines.PixelToLine(pixel - mTextRect.top));
}

// ------------------------------------------------------------
// 	OffsetAt
// ------------------------------------------------------------
// Return the offset of the character that starts the line

int32
STEngine::OffsetAt(
	int32	line) const
{
    int32 lineCount = mLines.NumLines();
	
	line = (line < 0) ? 0 : line;
	line = (line > lineCount) ? lineCount : line;

    return mLines[line]->offset;
}

// ------------------------------------------------------------
// 	OffsetToPoint
// ------------------------------------------------------------
// Return the local coordinates of the character at inOffset
// Pass back the height of the line that contains inOffset in outHeight
//
// The vertical coordinate will be at the origin (top) of the line
// The horizontal coordinate will be to the left of the character
// at offset
//
// Pass NULL for outHeight if you do not need to know the height

BPoint
STEngine::OffsetToPoint(
	int32	inOffset,
	float	*outHeight)
{
	BPoint 			result;
	int32			textLength = mText.Length();
	int32 			lineNum = OffsetToLine(inOffset);
	ConstSTELinePtr	line = mLines[lineNum];
	float 			height = (line + 1)->origin - line->origin;
	
	result.x = 0.0;
	result.y = line->origin + mTextRect.top;
	
	if (textLength > 0) {
		// special case: go down one line if inOffset is a newline
		if ((inOffset == textLength) && (mText[textLength - 1] == '\n')) {
			float	ascent = 0.0;
			float	descent = 0.0;
	
			StyledWidth(inOffset, 1, &ascent, &descent);
			
			result.y += height;
			height = (ascent + descent);
		}
		else {
			int32	offset = line->offset;
			int32	length = inOffset - line->offset;
			int32	numChars = length;
			bool	foundTab = false;		
			do {
				foundTab = mText.FindChar('\t', offset, &numChars);
			
				result.x += StyledWidth(offset, numChars);
		
				if (foundTab) {
					int32 numTabs = 0;
					for (numTabs = 0; (numChars + numTabs) < length; numTabs++) {
						if (mText[offset + numChars + numTabs] != '\t')
							break;
					}
											
					float tabWidth = ActualTabWidth(result.x);
					if (numTabs > 1)
						tabWidth += ((numTabs - 1) * mTabWidth);
		
					result.x += tabWidth;
					numChars += numTabs;
				}
				
				offset += numChars;
				length -= numChars;
				numChars = length;
			} while ((foundTab) && (length > 0));
		} 
	}

	// convert from text rect coordinates
	result.x += (mTextRect.left - 1.0);

	if (outHeight != NULL)
		*outHeight = height;
		
	return (result);
}

// ------------------------------------------------------------
// 	PointToOffset
// ------------------------------------------------------------
// Return the offset of the character that lies at point
// 
// point should be in local coordinates

int32
STEngine::PointToOffset(
	BPoint	point)
{
	// should we even bother?
	if (point.y >= mTextRect.bottom)
		return (mText.Length());
	else {
		if (point.y < mTextRect.top)
			return (0);
	}

	int32			lineNum = PixelToLine(point.y);
	ConstSTELinePtr	line = mLines[lineNum];
	
	// special case: if point is within the text rect and PixelToLine()
	// tells us that it's on the last line, but if point is actually  
	// lower than the bottom of the last line, return the last offset 
	// (can happen for newlines)
	if (lineNum == (mLines.NumLines() - 1)) {
		if (point.y >= ((line + 1)->origin + mTextRect.top))
			return (mText.Length());
	}
	
	// convert to text rect coordinates
	point.x -= mTextRect.left;
	point.x = (point.x < 0.0) ? 0.0 : point.x;
	
	// do a pseudo-binary search of the character widths on the line
	// that PixelToLine() gave us
	// note: the right half of a character returns its offset + 1
	int32	offset = line->offset;
	int32	saveOffset = offset;
	int32	limit = (line + 1)->offset;
	int32	length = limit - line->offset;
	float	sigmaWidth = 0.0;
	int32	numChars = length;
	bool	foundTab = false;
	bool	done = false;
	uchar	c;

	do {		
		// any tabs?
		foundTab = mText.FindChar('\t', offset, &numChars);

		if (numChars > 0) {
			int32 delta = numChars / 2;
			
			while (((c = mText[offset + delta]) & 0x80) != 0 && ! ((c & 0xC0) == 0xC0))
				delta--;
			if (delta < GlyphWidth(offset))
				delta = GlyphWidth(offset);

			do {
				int32 glyphwidth = GlyphWidth(offset + delta - 1);
				float deltaWidth = StyledWidth(offset, delta);
				float leftWidth = StyledWidth(offset + delta - glyphwidth, glyphwidth);
				sigmaWidth += deltaWidth;
	
				if (point.x >= (sigmaWidth - (leftWidth / 2.0))) {
					// we're to the left of the point
					float rightWidth = StyledWidth(offset + delta, GlyphWidth(offset + delta));
					if (point.x < (sigmaWidth + (rightWidth / 2.0f))) {
						// the next character is to the right, we're done!
						offset += delta;
						done = true;
						break;
					}
					else {
						// still too far to the left, measure some more
						offset += delta;
						delta /= 2;
						while (((c = mText[offset + delta]) & 0x80) != 0 && ! ((c & 0xC0) == 0xC0))
							delta--;
						if (delta < GlyphWidth(offset))
							delta = GlyphWidth(offset);
					}
				}
				else {
					// oops, we overshot the point, go back some 
					sigmaWidth -= deltaWidth;
					
					if (delta == GlyphWidth(offset)) {
						done = true;
						break;
					}
						
					delta /= 2;
					while (((c = mText[offset + delta]) & 0x80) != 0 && ! ((c & 0xC0) == 0xC0))
						delta--;
					if (delta < GlyphWidth(offset))
						delta = GlyphWidth(offset);
				}
			} while (offset < (numChars + saveOffset));
		}
		
		if (done || (offset >= limit))
			break;
			
		if (foundTab) {
			float tabWidth = ActualTabWidth(sigmaWidth);
			
			// is the point in the left-half of the tab?
			if (point.x < (sigmaWidth + (tabWidth / 2.0f)))
				break;
			else {
				// is the point in the right-half of the tab?
				if (point.x < (sigmaWidth + tabWidth)) {
					offset++;
					break;
				}
			}
				
			sigmaWidth += tabWidth;
			numChars++;
			
			// maybe we have more tabs?
			bool	foundPoint = false;
			int32	numTabs = 0;
			for (numTabs = 0; (numChars + numTabs) < length; numTabs++) {
				if (mText[saveOffset + numChars + numTabs] != '\t')
					break;
				
				if (point.x < (sigmaWidth + (mTabWidth / 2.0f))) {
					foundPoint = true;
					break;
				}
				else {
					if (point.x < (sigmaWidth + mTabWidth)) {
						foundPoint = true;
						numTabs++;
						break;
					}
				}
				
				sigmaWidth += mTabWidth;
			}

			if (foundPoint) {
				offset = saveOffset + numChars + numTabs;
				break;
			}

			numChars += numTabs;
		}

		offset = saveOffset + numChars;
		saveOffset = offset;
		length -= numChars;
		numChars = length;
	} while (length > 0);
	
	if (offset == (line + 1)->offset) {
		// special case: newlines aren't visible
		// return the offset of the character preceding the newline
		if (mText[offset - 1] == '\n')
			return (offset - 1);

		// special case: return the offset preceding any spaces that 
		// aren't at the end of the buffer
		if ((offset != mText.Length()) && (mText[offset - 1] == ' '))
			return (offset - 1);
	}
	
	return (offset);
}

// ------------------------------------------------------------
// 	GlyphWidth
// ------------------------------------------------------------
// Return the number of bytes in the multibyte UTF8 character at
// the specified offset.  It doesn't matter whether the byte
// at this offset is the first, second, or third byte of
// the glyph.
// This code assumes only valid utf8 chars are in the buffer.

int32
STEngine::GlyphWidth(
	int32	inOffset) const
{
//	ASSERT(inOffset >= 0 && inOffset <= mText.Length());
	int32	result;
	uchar	c = mText[inOffset];

	if (c < 0x80)			// one byte char
		result = 1;
	else
	if (c >= 0xe0)			// first byte of three byte glyph
		result = 3;
	else
	if (c >= 0xc0)			// first byte of two byte glyph
		result = 2;
	else
	{
		uchar	d = mText[inOffset - 1];
		
		if (d >= 0xe0)		// was second byte of three byte glyph
			result = 3;
		else
		if (d >= 0xc0)		// was second byte of two byte glyph
			result = 2;
		else
		if (d >= 0x80)
			result = 3;		// was third byte of three byte glyph
		else
			result = 1;		// invalid utf8 glyph
	}

	return result;
}

// ------------------------------------------------------------
// 	FindWord
// ------------------------------------------------------------
// Return a pair of offsets that describe a word at inOffset
//
// Override STEngine::IsWordBreakChar() to define a 'word'

void
STEngine::FindWord(
	int32	inOffset, 
	int32	*outFromOffset,
	int32 	*outToOffset)
{
	int32 offset;
	
	// check to the left
	for (offset = inOffset; offset > 0; offset--) {
		if (IsWordBreakChar(mText[offset - 1]))
			break;
	}
	*outFromOffset = offset;

	// check to the right
	int32 textLen = mText.Length();
	for (offset = inOffset; offset < textLen; offset++) {
		if (IsWordBreakChar(mText[offset]))
			break;
	}
	*outToOffset = offset;
}

// ------------------------------------------------------------
// 	GetHeight
// ------------------------------------------------------------
// Return the height from startLine to endLine
						
float
STEngine::GetHeight(
	int32	startLine,
	int32	endLine)
{
	int32 lastChar = mText.Length() - 1;
	int32 lastLine = mLines.NumLines() - 1;
	startLine = (startLine < 0) ? 0 : startLine;
	endLine = (endLine > lastLine) ? lastLine : endLine;
	
	float height = mLines[endLine + 1]->origin - 
				   mLines[startLine]->origin;
				
	if ((endLine == lastLine) && (mText[lastChar] == '\n')) {
		float ascent = 0.0;
		float descent = 0.0;

		StyledWidth(lastChar, 1, &ascent, &descent);
		
		height += (ascent + descent);
	}
	
	return (height);
}

// ------------------------------------------------------------
// 	GetHiliteRegion
// ------------------------------------------------------------
// Return the region that encompasses the characters in the
// range of startOffset to endOffset

void
STEngine::GetHiliteRegion(
	int32	startOffset,
	int32	endOffset,
	BRegion	*outRegion)
{
	outRegion->MakeEmpty();

	// return an empty region if the range is invalid
	if (startOffset >= endOffset)
		return;

	float	startLineHeight = 0.0;
	float	endLineHeight = 0.0;
	BPoint	startPt = OffsetToPoint(startOffset, &startLineHeight);
	startPt.x = ceil(startPt.x);
	startPt.y = ceil(startPt.y);
	BPoint	endPt = OffsetToPoint(endOffset, &endLineHeight);
	endPt.x = ceil(endPt.x);
	endPt.y = ceil(endPt.y);
	BRect	selRect;

	if (startPt.y == endPt.y) {
		// this is a one-line region
		selRect.left = (startPt.x < mTextRect.left) ? mTextRect.left : startPt.x;
		selRect.top = startPt.y;
		selRect.right = endPt.x - 1.0;
		selRect.bottom = endPt.y + endLineHeight - 1.0;
		outRegion->Include(selRect);
	}
	else {
		// more than one line in the specified offset range
		selRect.left = (startPt.x < mTextRect.left) ? mTextRect.left : startPt.x;
		selRect.top = startPt.y;
		selRect.right = mTextRect.right;
		selRect.bottom = startPt.y + startLineHeight - 1.0;
		outRegion->Include(selRect);
		
		if ((startPt.y + startLineHeight) < endPt.y) {
			// more than two lines in the range
			selRect.left = mTextRect.left;
			selRect.top = startPt.y + startLineHeight;
			selRect.right = mTextRect.right;
			selRect.bottom = endPt.y - 1.0;
			outRegion->Include(selRect);
		}
		
		if (mTextRect.left < endPt.x)
		{
			selRect.left = mTextRect.left;
			selRect.top = endPt.y;
			selRect.right = endPt.x - 1.0;
			selRect.bottom = endPt.y + endLineHeight - 1.0;
			outRegion->Include(selRect);
		}
	}
}

// ------------------------------------------------------------
// 	ScrollToOffset
// ------------------------------------------------------------
// Scroll the view so that inOffset is fully visible
//
// This function does nothing if there are no scroll bars 
// that target this view

void
STEngine::ScrollToOffset(
	int32	inOffset)
{
	BRect	bounds = Bounds();
	float	lineHeight = 0.0;
	BPoint	point = OffsetToPoint(inOffset, &lineHeight);
	bool	update = false;
	
	if ((point.x < bounds.left) || (point.x >= bounds.right)) {
		BScrollBar *hScroll = ScrollBar(B_HORIZONTAL);
		if (hScroll != NULL) {
			update = true;
			hScroll->SetValue(point.x - (bounds.IntegerWidth() / 2));
		}
	}
	
	if ((point.y < bounds.top) || ((point.y + lineHeight) >= bounds.bottom)) {
		BScrollBar *vScroll = ScrollBar(B_VERTICAL);
		if (vScroll != NULL) {
			update = true;
			vScroll->SetValue(point.y - (bounds.IntegerHeight() / 2));
		}
	}
	
	if (update)
		Window()->UpdateIfNeeded();
}

// ------------------------------------------------------------
// 	ScrollToSelection
// ------------------------------------------------------------
// Scroll the view so that the beginning of the selection is
// within visible range
//
// This function does nothing if there are no scroll bars 
// that target this view

void
STEngine::ScrollToSelection()
{
	ScrollToOffset(mSelStart);
}

// ------------------------------------------------------------
// 	ScrollOffsetIntoView
// ------------------------------------------------------------
//	Scroll the view the minimum amount so that inOffset is fully visible
//	BDS

void
STEngine::ScrollOffsetIntoView(
	int32		inOffset)
{
	if (inOffset < 0)
		inOffset = 0;
	if (inOffset > TextLength())
		inOffset = TextLength();

	BRect			bounds = Bounds();
	float			lineHeight = 0.0;
	BPoint 			point = OffsetToPoint(inOffset, &lineHeight);
	BScrollBar*		hScroller = ScrollBar(B_HORIZONTAL);
	BScrollBar*		vScroller = ScrollBar(B_VERTICAL);
	
	if (hScroller != NULL) {
		if (point.x < bounds.left)
			hScroller->SetValue(point.x);
		else
		if (point.x >= bounds.right)
			hScroller->SetValue(point.x - bounds.Width());
	}
	
	if (vScroller != NULL) 
	{
		if (point.y < bounds.top)
			vScroller->SetValue(point.y);
		else
		if (point.y + lineHeight > bounds.bottom)
			vScroller->SetValue(point.y - bounds.Height() + lineHeight);
	}
}

// ------------------------------------------------------------
// 	SetTextRect
// ------------------------------------------------------------
// Set the text rect
//
// This function will also resize the offscreen bitmap 
// (if there is one)

void
STEngine::SetTextRect(
	BRect	rect)
{
	if (rect == mTextRect)
		return;
		
	mTextRect = rect;
	mTextRect.bottom = mTextRect.top;

	if (mOffscreen != NULL) {
		mOffscreen->Lock();
		color_space colors = mOffscreen->ColorSpace();
		mOffscreen->Unlock();
		UseOffscreen(colors);
	}
	
	if (Window() != NULL) {
		if (mActive) {
			// hide the caret, unhilite the selection
			if (mSelStart != mSelEnd)
				DrawSelection(mSelStart, mSelEnd);
			else {
				if (mCaretVisible)
					InvertCaret();
			}
		}
		
		Refresh(0, mText.Length(), true, false);
		
		// invalidate and immediately redraw in case the 
		// text rect has gotten smaller 
		Invalidate();
		Window()->UpdateIfNeeded();
	}
}

// ------------------------------------------------------------
// 	TextRect
// ------------------------------------------------------------
// Get the text rect

BRect
STEngine::TextRect() const
{
	return (mTextRect);
}

// ------------------------------------------------------------
// 	SetTabWidth
// ------------------------------------------------------------
// Set the width of the tab character

void
STEngine::SetTabWidth(
	float	width)
{
	if (width == mTabWidth)
		return;
		
	mTabWidth = width;
	
	if (Window() != NULL) {
		if (mActive) {
			// hide the caret, unhilite the selection
			if (mSelStart != mSelEnd)
				DrawSelection(mSelStart, mSelEnd);
			else {
				if (mCaretVisible)
					InvertCaret();
			}
		}
	
		Refresh(0, mText.Length(), true, false);
		
		if (mActive) {
			// show the caret, hilite the selection
			if (mSelStart != mSelEnd)
				DrawSelection(mSelStart, mSelEnd);
			else {
				if (!mCaretVisible)
					InvertCaret();
			}
		}
	}
}

// ------------------------------------------------------------
// 	TabWidth
// ------------------------------------------------------------
// Return the width of the tab character

float
STEngine::TabWidth() const
{
	return (mTabWidth);
}

// ------------------------------------------------------------
// 	MakeSelectable
// ------------------------------------------------------------
// Set whether the caret/selection range will be displayed

void
STEngine::MakeSelectable(
	bool	selectable)
{
	if (selectable == mSelectable)
		return;
		
	mSelectable = selectable;
	
	if (Window() != NULL) {
		if (mActive) {
			// show/hide the caret, hilite/unhilite the selection
			if (mSelStart != mSelEnd)
				DrawSelection(mSelStart, mSelEnd);
			else
				InvertCaret();
		}
	}
}

// ------------------------------------------------------------
// 	IsSelectable
// ------------------------------------------------------------
// Return whether the caret/selection range will be displayed

bool
STEngine::IsSelectable() const
{
	return (mSelectable);
}

// ------------------------------------------------------------
// 	MakeEditable
// ------------------------------------------------------------
// Set whether the text can be edited

void
STEngine::MakeEditable(
	bool	editable)
{
	if (editable == mEditable)
		return;
		
	mEditable = editable;
	
	if (Window() != NULL) {
		if (mActive) {
			if ((!mEditable) && (mCaretVisible))
				InvertCaret();
		}
	}
}

// ------------------------------------------------------------
// 	IsEditable
// ------------------------------------------------------------
// Return whether the text can be edited

#if 0
bool
STEngine::IsEditable() const
{
	return (mEditable);
}
#endif

// ------------------------------------------------------------
// 	SetWordWrap
// ------------------------------------------------------------
// Set whether the text should be wrapped 

void
STEngine::SetWordWrap(
	bool	wrap)
{
	if (wrap == mWrap)
		return;
		
	mWrap = wrap;
	
	if (Window() != NULL) {
		if (mActive) {
			// hide the caret, unhilite the selection
			if (mSelStart != mSelEnd)
				DrawSelection(mSelStart, mSelEnd);
			else {
				if (mCaretVisible)
					InvertCaret();
			}
		}
		
		Refresh(0, mText.Length(), true, false);
		
		if (mActive) {
			// show the caret, hilite the selection
			if (mSelStart != mSelEnd)
				DrawSelection(mSelStart, mSelEnd);
			else {
				if (!mCaretVisible)
					InvertCaret();
			}
		}
	}
}

// ------------------------------------------------------------
// 	DoesWordWrap
// ------------------------------------------------------------
// Return whether the text is wrapped 

bool
STEngine::DoesWordWrap() const
{
	return (mWrap);
}

// ------------------------------------------------------------
// 	UseOffscreen
// ------------------------------------------------------------
// Use an offscreen bitmap that is colors deep
//
// The bitmap is used only for drawing the current line

void
STEngine::UseOffscreen(
	color_space	colors)
{
	// delete in case we're resizing or changing color spaces
	if (mOffscreen != NULL) {
		mOffscreen->Lock();
		delete (mOffscreen);
		mOffscreen = NULL;
	}
	
	BRect offBounds;
	offBounds.left = 0.0;
	offBounds.top = 0.0;
	offBounds.right = mTextRect.Width();
	offBounds.bottom = mLines[1]->origin - mLines[0]->origin;
	mOffscreen = new BBitmap(offBounds, colors, true);
	mOffscreen->Lock();
	mOffscreen->AddChild(new BView(offBounds, B_EMPTY_STRING, B_FOLLOW_NONE, 0));
	mOffscreen->Unlock();
}

// ------------------------------------------------------------
// 	DontUseOffscreen
// ------------------------------------------------------------
// Delete the offscreen bitmap and settle with the ugly 
// flickers

void
STEngine::DontUseOffscreen()
{
	if (mOffscreen == NULL)
		return;
		
	mOffscreen->Lock();
	delete (mOffscreen);
	mOffscreen = NULL;
}

// ------------------------------------------------------------
// 	DoesUseOffscreen
// ------------------------------------------------------------
// Return whether an offscreen bitmap is being used

bool
STEngine::DoesUseOffscreen() const
{
	return (mOffscreen != NULL);
}
