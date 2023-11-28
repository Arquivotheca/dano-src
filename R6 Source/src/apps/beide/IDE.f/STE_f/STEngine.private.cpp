// ============================================================
//  STEngine.private.cpp	©1996 Hiroshi Lockheimer
// ============================================================
// 	STE Version 1.0a5


#include <ctype.h>
#include <string.h>

#include "STEngine.h"
#include "MHiliteColor.h"

#include <Application.h>
#include <ScrollBar.h>
#include <Region.h>
#include <Bitmap.h>
#include <Window.h>

// This char_map stuff is borrowed from string.c - see strpbrk
typedef unsigned char char_map[32];

inline void set_char_map(char_map map, uchar ch)
{	 map[ch>>3] |= (1 << (ch&7)); };
inline bool tst_char_map(char_map map, uchar ch)
{ 	return (map[ch>>3] &  (1 << (ch&7))); }

char_map		line_break_map;
char			line_breaks[] = {"\t\n &*+-/<=>\\^|"};

// ------------------------------------------------------------
// 	Setup
// ------------------------------------------------------------
// Needs to be called before any STEngines are created

void
STEngine::Setup()
{
	memset(line_break_map, '\0', sizeof(char_map));
	int		i = 0;
	uchar	c;
	while ((c = line_breaks[i++]) != '\0')
	{
		set_char_map(line_break_map, c);
	}
}

// ------------------------------------------------------------
// 	HandleBackspace
// ------------------------------------------------------------
// The Backspace key has been pressed

void
STEngine::HandleBackspace()
{
	if (mSelStart == mSelEnd) {
		if (mSelStart == 0)
			return;
		else
			mSelStart -= GlyphWidth(mSelStart - 1);
	}
	else
		DrawSelection(mSelStart, mSelEnd);

	RemoveRange(mSelStart, mSelEnd);
	mSelEnd = mSelStart;
	
	Refresh(mSelStart, mSelEnd, true, true);
	
	HandleModification();
}

// ------------------------------------------------------------
// 	HandleDelete
// ------------------------------------------------------------
// The Delete key has been pressed

void
STEngine::HandleDelete()
{
	if (mSelStart == mSelEnd) {
		if (mSelEnd == mText.Length())
			return;
		else 
			mSelEnd += GlyphWidth(mSelEnd);
	}
	else
		DrawSelection(mSelStart, mSelEnd);
		
	RemoveRange(mSelStart, mSelEnd);
	
	mSelEnd = mSelStart;
	
	Refresh(mSelStart, mSelEnd, true, true);
	
	HandleModification();
}

// ------------------------------------------------------------
// 	HandleArrowKey
// ------------------------------------------------------------
// One of the four arrow keys has been pressed

void
STEngine::HandleArrowKey(
	uint32	inArrowKey)
{
	// return if there's nowhere to go
	if (mText.Length() == 0)
		return;
		
	BMessage	*message = Window()->CurrentMessage();
	bool		shiftDown = message->FindInt32("modifiers") & B_SHIFT_KEY;
	int32		selStart = mSelStart;
	int32		selEnd = mSelEnd;
	int32		scrollToOffset = 0;

	switch (inArrowKey) {
		case B_UP_ARROW:
			if ((selStart == selEnd) || (shiftDown)) {
				BPoint point = OffsetToPoint(selStart);
				point.y--;
				selStart = PointToOffset(point);
				if (!shiftDown)
					selEnd = selStart;
				scrollToOffset = selStart;
				break;
			}
			// else fall through
			
		case B_LEFT_ARROW:
			if (shiftDown) {
				if (selStart > 0)
					selStart -= GlyphWidth(selStart - 1);
			}
			else {
				if (selStart == selEnd) {
					if (selStart > 0)
					{
						selStart -= GlyphWidth(selStart - 1);
						selEnd = selStart;
					}
				}
				else
					selEnd = selStart;
			}
			scrollToOffset = selStart;
			break;
			
		case B_DOWN_ARROW:
			if ((selStart == selEnd) || (shiftDown)) {
				float	height;
				BPoint	point = OffsetToPoint(selEnd, &height);
				point.y += height;
				selEnd = PointToOffset(point);
				if (!shiftDown)
					selStart = selEnd;
				scrollToOffset = selEnd;
				break;
			}
			// else fall through
			
		case B_RIGHT_ARROW:
			if (shiftDown) {
				if (selEnd < mText.Length())
					selEnd += GlyphWidth(selEnd);
			}
			else {
				if (selStart == selEnd) {
					if (selStart < mText.Length())
					{
						selEnd += GlyphWidth(selEnd);
						selStart = selEnd;
					}
				}
				else
					selStart = selEnd;
			}
			scrollToOffset = selEnd;
			break;
	}
	
	// invalidate the null style
	mStyles.InvalidateNullStyle();
	
	Select(selStart, selEnd);
	
	// scroll if needed
	ScrollOffsetIntoView(scrollToOffset);
}

// ------------------------------------------------------------
// 	HandlePageKey
// ------------------------------------------------------------
// Home, End, Page Up, or Page Down has been pressed

void
STEngine::HandlePageKey(
	uint32	inPageKey)
{		
	switch (inPageKey) {
		case B_HOME:
		case B_END:
			ScrollToOffset((inPageKey == B_HOME) ? 0 : mText.Length());
			break;
			
		case B_PAGE_UP: 
		case B_PAGE_DOWN:
		{
			BScrollBar *vScroll = ScrollBar(B_VERTICAL);
			if (vScroll != NULL) {
				float delta = Bounds().Height();
				delta = (inPageKey == B_PAGE_UP) ? -delta : delta;
				
				vScroll->SetValue(vScroll->Value() + delta);
				Window()->UpdateIfNeeded();
			}
			break;
		}
	}
}

// ------------------------------------------------------------
// 	HandleAlphaKey
// ------------------------------------------------------------
// A printing key has been pressed

void
STEngine::HandleAlphaKey(
	const char *	inBytes, 
	int32 			inNumBytes)
{
	bool refresh = mSelStart != mText.Length();
	
	if (mSelStart != mSelEnd) {
		DrawSelection(mSelStart, mSelEnd);
		RemoveRange(mSelStart, mSelEnd);
		refresh = true;
	}

	InsertAt(inBytes, inNumBytes, mSelStart);
	
	mSelStart += inNumBytes;
	mSelEnd = mSelStart;

	Refresh(mSelStart, mSelEnd, refresh, true);
	
	HandleModification();
}

// ------------------------------------------------------------
// 	InsertAt
// ------------------------------------------------------------
// Copy inLength bytes of inText to the buffer, starting at offset
//
// Optionally apply inStyles to the newly inserted text

void
STEngine::InsertAt(
	const char				*inText,
	int32 					inLength,
	int32 					inOffset,
	ConstSTEStyleRangePtr	inStyles)
{
	// why add nothing?
	if (inLength < 1)
		return;
	
	// add the text to the buffer
	mText.InsertText(inText, inLength, inOffset);
	
	// update the start offsets of each line below inOffset
	mLines.BumpOffset(inLength, OffsetToLine(inOffset) + 1);
	
	// update the style runs
	mStyles.BumpOffset(inLength, mStyles.OffsetToRun(inOffset - 1) + 1);
	
	if (inStyles != NULL)
		SetStyleRange(inOffset, inOffset + inLength, inStyles, false);
	else {
		// apply nullStyle to inserted text
		mStyles.SyncNullStyle(inOffset);
		mStyles.SetStyleRange(inOffset, inOffset + inLength, 
							  mText.Length(), doAll, NULL);
	}
}

// ------------------------------------------------------------
// 	RemoveRange
// ------------------------------------------------------------
// Remove data that lies between fromOffset and toOffset

void
STEngine::RemoveRange(
	int32	fromOffset,
	int32 	toOffset)
{
	// sanity checking
	if ((fromOffset >= toOffset) || (fromOffset < 0) || (toOffset < 0))
		return;
		
	// set nullStyle to style at beginning of range
	mStyles.InvalidateNullStyle();
	mStyles.SyncNullStyle(fromOffset);	
	
	// remove from the text buffer
	mText.RemoveRange(fromOffset, toOffset);
	
	// remove any lines that have been obliterated
	mLines.RemoveLineRange(fromOffset, toOffset);
	
	// remove any style runs that have been obliterated
	mStyles.RemoveStyleRange(fromOffset, toOffset);
}

// ------------------------------------------------------------
// 	Refresh
// ------------------------------------------------------------
// Recalculate the line breaks from fromOffset to toOffset
// and redraw the text with the new line breaks
//
// If erase is true, the affected text area will be erased  
// before the text is drawn
//
// If scroll is true, the view will be scrolled so that
// the end of the selection is visible

void
STEngine::Refresh(
	int32	fromOffset,
	int32	toOffset,
	bool	erase,
	bool	scroll)
{
	float	saveHeight = mTextRect.Height();
	int32 	fromLine = OffsetToLine(fromOffset);
	int32 	toLine = OffsetToLine(toOffset);
	int32	saveFromLine = fromLine;
	int32	saveToLine = toLine;
	float	saveLineHeight = GetHeight(fromLine, fromLine);
	
	RecalLineBreaks(&fromLine, &toLine);

	float newHeight = mTextRect.Height();
	
	// if the line breaks have changed, force an erase
	if ( (fromLine != saveFromLine) || (toLine != saveToLine) || 
		 (newHeight != saveHeight) )
		erase = true;
	
	if (newHeight != saveHeight) {
		// the text area has changed
		if (newHeight < saveHeight)
			toLine = PixelToLine(saveHeight + mTextRect.top);
		else
			toLine = PixelToLine(newHeight + mTextRect.top);
	}
	
	int32 drawOffset = fromOffset;
	if ( (GetHeight(fromLine, fromLine) != saveLineHeight) || 
		 (newHeight < saveHeight) || (fromLine < saveFromLine) )
		drawOffset = mLines[fromLine]->offset;
			
	DrawLines(fromLine, toLine, drawOffset, erase);
	
	// erase the area below the text
	BRect bounds = Bounds();
	BRect eraseRect = bounds;
	eraseRect.top = mTextRect.top + mLines[mLines.NumLines()]->origin;
	eraseRect.bottom = mTextRect.top + saveHeight;
	if ((eraseRect.bottom > eraseRect.top) && (eraseRect.Intersects(bounds))) {
		SetLowColor(ViewColor());
		FillRect(eraseRect, B_SOLID_LOW);
	}
	
	// update the scroll bars if the text area has changed
	if (newHeight != saveHeight)
		UpdateScrollbars();

	if (scroll)
		ScrollToOffset(mSelEnd);
}

// ------------------------------------------------------------
// 	IsWordBreakChar
// ------------------------------------------------------------
// Return true if inChar will break a word

bool
STEngine::IsWordBreakChar(
	uchar	inChar)
{
	return (ispunct(inChar) || isspace(inChar));
}

// ------------------------------------------------------------
// 	IsLineBreakChar
// ------------------------------------------------------------
// Return true if inChar will break a line

bool
STEngine::IsLineBreakChar(
	uchar	inChar)
{
	return tst_char_map(line_break_map, inChar) || (inChar == '\0');
}

// ------------------------------------------------------------
// 	RecalLineBreaks
// ------------------------------------------------------------
// Recalculate the line breaks starting at startLine
// Recalculate at least up to endLine
//
// Pass back the range of affected lines in startLine and endLine

void
STEngine::RecalLineBreaks(
	int32	*startLine,
	int32	*endLine)
{
	// are we insane?
	*startLine = (*startLine < 0) ? 0 : *startLine;
	*endLine = (*endLine > mLines.NumLines() - 1) ? mLines.NumLines() - 1 : *endLine;
	
	int32		textLength = mText.Length();
	int32		lineIndex = (*startLine > 0) ? *startLine - 1 : 0;
	int32		recalThreshold = mLines[*endLine + 1]->offset;
	const float	width = mTextRect.Width();
	
	// cast away the const-ness
	STELinePtr curLine = (STELinePtr)mLines[lineIndex];
	STELinePtr nextLine = (STELinePtr)curLine + 1;

	do {
		int32 	fromOffset = curLine->offset;
		float	ascent = 0.0;
		float	descent = 0.0;
		int32 	toOffset = FindLineBreak(fromOffset, &ascent, 
										 &descent, width);

		// we want to advance at least by one character
		if ((toOffset == fromOffset) && (fromOffset < textLength))
			toOffset++;
		
		// set the ascent of this line
		curLine->ascent = ascent;
		
		lineIndex++;
		STELine saveLine = *nextLine;		
		if ( (lineIndex > mLines.NumLines()) || 
			 (toOffset < nextLine->offset) ) {
			// the new line comes before the old line start, add a line
			STELine newLine;
			newLine.offset = toOffset;
			newLine.origin = curLine->origin + ascent + descent;
			newLine.ascent = 0.0;
			mLines.InsertLine(&newLine, lineIndex);
		}
		else {
			// update the exising line
			nextLine->offset = toOffset;
			nextLine->origin = curLine->origin + ascent + descent;
			
			// remove any lines that start before the current line
			while ( (lineIndex < mLines.NumLines()) &&
					(toOffset >= (mLines[lineIndex] + 1)->offset) )
				mLines.RemoveLines(lineIndex + 1);
			
			nextLine = (STELinePtr)mLines[lineIndex];
			if (nextLine->offset == saveLine.offset) {
				if (nextLine->offset >= recalThreshold) {
					if (nextLine->origin != saveLine.origin)
						mLines.BumpOrigin(nextLine->origin - saveLine.origin, 
										  lineIndex + 1);
					break;
				}
			}
			else {
				if ((lineIndex > 0) && (lineIndex == *startLine))
					*startLine = lineIndex - 1;
			}
		}

		curLine = (STELinePtr)mLines[lineIndex];
		nextLine = (STELinePtr)curLine + 1;
	} while (curLine->offset < textLength);

	// update the text rect
	float newHeight = GetHeight(0, mLines.NumLines() - 1);
	mTextRect.bottom = mTextRect.top + newHeight;

	*endLine = lineIndex - 1;
	*startLine = (*startLine > *endLine) ? *endLine : *startLine;
}

// ------------------------------------------------------------
// 	FindLineBreak
// ------------------------------------------------------------
// Determine where to break a line that is ioWidth wide, 
// starting at fromOffset
//
// Pass back the maximum ascent and descent for the line in
// outAscent and outDescent
// Set ioWidth to the total width of the line

int32
STEngine::FindLineBreak(
	int32	fromOffset,
	float	*outAscent,
	float	*outDescent,
	float	width)
{
	*outAscent = 0.0;
	*outDescent = 0.0;
	
	const int32 limit = mText.Length();

	// is fromOffset at the end?
	if (fromOffset >= limit) {
		// try to return valid height info anyway			
		if (mStyles.NumRuns() > 0)
			mStyles.Iterate(fromOffset, 1, NULL, outAscent, outDescent);
		else {
			if (mStyles.IsValidNullStyle()) {
				STEStyle style = mStyles.GetNullStyle();
				SetFont(&style);

				font_height info;
				GetFontHeight(&info);
				*outAscent = ceil(info.ascent);
				*outDescent = ceil(info.descent) + ceil(info.leading);
			}
		}

		return (limit);
	}
	
	bool	done = false;
	float	ascent = 0.0;
	float	descent = 0.0;
	int32	offset = fromOffset;
	int32	delta = 0;
	float	deltaWidth = 0.0;
	float	tabWidth = 0.0;
	float	strWidth = 0.0;
	float	maxAscent = 0.0;
	float	maxDescent = 0.0;
	
	// maybe we don't need to wrap the text?
	if (!mWrap) {
		int32 length = limit - fromOffset;
		mText.FindChar('\n', fromOffset, &length);
		offset = fromOffset + (++length);
		offset = (offset > limit) ? limit : offset;

		// iterate through the style runs for the heights
		while (int32 numChars = mStyles.Iterate(fromOffset, length, NULL, &ascent, &descent)) {
			maxAscent = (ascent > maxAscent) ? ascent : maxAscent;
			maxDescent = (descent > maxDescent) ? descent : maxDescent;
			fromOffset += numChars;
			length -= numChars;
		}

		*outAscent = maxAscent;
		*outDescent = maxDescent;

		return (offset);
	}
	
	// wrap the text
	do {
		bool foundTab = false;
		
		// find the next line break candidate
		for ( ; (offset + delta) < limit ; delta++) {
			if (IsLineBreakChar(mText[offset + delta]))
				break;
		}
		for ( ; (offset + delta) < limit; delta++) {
			uchar theChar = mText[offset + delta];
			if (!IsLineBreakChar(theChar))
				break;
			
			if (theChar == '\n') {
				// found a newline, we're done!
				done = true;
				delta++;
				break;
			}
			else {
				// include all trailing spaces and tabs,
				// but not spaces after tabs
				if ((theChar != ' ') && (theChar != '\t'))
					break;
				else {
					if ((theChar == ' ') && (foundTab))
						break;
					else {
						if (theChar == '\t')
							foundTab = true;
					}
				}
			}
		}
		delta = (delta < 1) ? 1 : delta;
	
		deltaWidth = StyledWidth(offset, delta, &ascent, &descent);
		strWidth += deltaWidth;

		if (!foundTab)
			tabWidth = 0.0;
		else {
			int32 tabCount = 0;
			for (int32 i = delta - 1; mText[offset + i] == '\t'; i--)
				tabCount++;

			tabWidth = ActualTabWidth(strWidth);
			if (tabCount > 1)
				tabWidth += ((tabCount - 1) * mTabWidth);

			strWidth += tabWidth;
		}
			
		if (strWidth >= width) {
			// we've found where the line will wrap
			bool foundNewline = done;
			done = true;
			int32 pos = delta - 1;
			if ((mText[offset + pos] != ' ') &&
				(mText[offset + pos] != '\t') &&
				(mText[offset + pos] != '\n'))
				break;
			
			strWidth -= (deltaWidth + tabWidth);
			
			for ( ; ((offset + pos) > offset); pos--) {
				uchar theChar = mText[offset + pos];
				if ((theChar != ' ') &&
					(theChar != '\t') &&
					(theChar != '\n'))
					break;
			}

			strWidth += StyledWidth(offset, pos + 1, &ascent, &descent);
			if (strWidth >= width)
				break;

			if (!foundNewline) {
				for ( ; (offset + delta) < limit; delta++) {
					if ((mText[offset + delta] != ' ') &&
						(mText[offset + delta] != '\t'))
						break;
				}
				if ( ((offset + delta) < limit) && 
					 (mText[offset + delta] == '\n') )
					delta++;
			}
			// get the ascent and descent of the spaces/tabs
			StyledWidth(offset, delta, &ascent, &descent);
		}

		maxAscent = (ascent > maxAscent) ? ascent : maxAscent;
		maxDescent = (descent > maxDescent) ? descent : maxDescent;

		offset += delta;
		delta = 0;
	} while ((offset < limit) && (!done));

	*outAscent = maxAscent;
	*outDescent = maxDescent;

	if ((offset - fromOffset) < 1) {
		// there weren't any words that fit entirely in this line
		// force a break in the middle of a word
		*outAscent = 0.0;
		*outDescent = 0.0;
		strWidth = 0.0;
		
		for (offset = fromOffset; offset < limit; offset++) {
			strWidth += StyledWidth(offset, 1, &ascent, &descent);
			
			if (strWidth >= width)
				break;
				
			maxAscent = (ascent > maxAscent) ? ascent : maxAscent;
			maxDescent = (descent > maxDescent) ? descent : maxDescent;
		}

		*outAscent = maxAscent;
		*outDescent = maxDescent;
	}
	
	offset = (offset < limit) ? offset : limit;

	return (offset);
}

// ------------------------------------------------------------
// 	StyledWidth
// ------------------------------------------------------------
// Return the width of length bytes of styled text beginning at
// fromOffset
//
// Pass back the maximum ascent and maximum descent of the text
// in outAscent and outDescent
//
// Pass NULL for outAscent and/or outDescent if you are not
// interested in that data
//
// Tab-widths are not calculated, use OffsetToPoint() if you need
// tab-inclusive widths

float
STEngine::StyledWidth(
	int32	fromOffset,
	int32 	length,
	float	*outAscent,
	float	*outDescent)
{
	float result = 0.0;
	float ascent = 0.0;
	float descent = 0.0;
	float maxAscent = 0.0;
	float maxDescent = 0.0;
	
	// iterate through the style runs
	ConstSTEStylePtr style = NULL;
	while (int32 numChars = mStyles.Iterate(fromOffset, length, &style, &ascent, &descent)) {		
		maxAscent = (ascent > maxAscent) ? ascent : maxAscent;
		maxDescent = (descent > maxDescent) ? descent : maxDescent;

		result += sWidths.StringWidth(mText, fromOffset, numChars, style);

		fromOffset += numChars;
		length -= numChars;
	}

	if (outAscent != NULL)
		*outAscent = maxAscent;
	if (outDescent != NULL)
		*outDescent = maxDescent;

	return (result);
}

// ------------------------------------------------------------
// 	ActualTabWidth
// ------------------------------------------------------------
// Return the actual tab width at location 
// 
// location should be in text rect coordinates

float
STEngine::ActualTabWidth(
	float	location)
{
	if (mTabWidth <= 0.0)
		return (0.0);
		
	return ( mTabWidth - 
			 (location - ((int32)(location / mTabWidth)) * mTabWidth) );
}

// ------------------------------------------------------------
// 	DrawLines
// ------------------------------------------------------------
// Draw the lines from startLine to endLine
// Erase the affected area before drawing if erase is true
//
// startOffset gives the offset of the first character in startLine
// that needs to be erased (avoids flickering of the entire line)
//
// Pass a value of -1 in startOffset if you want the entire 
// line of startLine to be erased 

void
STEngine::DrawLines(
	int32	startLine,
	int32	endLine,
	int32	startOffset,
	bool	erase)
{
	BRect bounds = Bounds();
	
	// clip the text	
	BRect clipRect = bounds & mTextRect;
	clipRect.InsetBy(-1.0, -1.0);
	BRegion newClip;
	newClip.Set(clipRect);
	ConstrainClippingRegion(&newClip);

	// set the low color to the view color so that 
	// drawing to a non-white background will work	
	const rgb_color viewColor = ViewColor();
	SetLowColor(viewColor);

	// draw only those lines that are visible
	int32 startVisible = PixelToLine(bounds.top);
	int32 endVisible = PixelToLine(bounds.bottom);
	startLine = (startLine < startVisible) ? startVisible : startLine;
	endLine = (endLine > endVisible) ? endVisible : endLine;

	BRect 			eraseRect = clipRect;
	int32			startEraseLine = startLine;
	ConstSTELinePtr	line = mLines[startLine];

	if ((mOffscreen == NULL) && (erase) && (startOffset != -1)) {
		// erase only portion of first line
		startEraseLine++;
			
		int32 startErase = startOffset;
		if (startErase > line->offset) {
			for ( ; (mText[startErase] != ' ') && (mText[startErase] != '\t'); startErase--) {
				if (startErase <= line->offset)
					break;	
			}
			if (startErase > line->offset)
				startErase--;
		}

		eraseRect.left = OffsetToPoint(startErase).x;
		eraseRect.top = line->origin + mTextRect.top;
		eraseRect.bottom = (line + 1)->origin + mTextRect.top;
		
		FillRect(eraseRect, B_SOLID_LOW);

		eraseRect.left = clipRect.left;
	}

	BRect			lineRect;
	BPoint			penLoc(0.0, 0.0);
	const bool		drawHilite = mActive && (mSelStart != mSelEnd);
	const rgb_color	hiliteColor = HiliteColor();

	for (int32 i = startLine; i <= endLine; i++) {
		BView	*	drawView = this;
		int32 		length = (line + 1)->offset - line->offset;

		// DrawString() chokes if you draw a newline
		if (mText[(line + 1)->offset - 1] == '\n')
			length--;	
					
		if ((i > startLine) || (startOffset == -1) || (mOffscreen == NULL)) 
		{						
			lineRect.top = line->origin + mTextRect.top;;
			lineRect.bottom = (line + 1)->origin + mTextRect.top - 1.0;

			if ((erase) && (i >= startEraseLine)) {
				eraseRect.top = lineRect.top;
				eraseRect.bottom = lineRect.bottom;
				
				SetDrawingMode(B_OP_COPY);
				SetLowColor(viewColor);
				FillRect(eraseRect, B_SOLID_LOW);
			}

			penLoc.Set(mTextRect.left, line->origin + line->ascent + mTextRect.top);
			MovePenTo(penLoc);
		}
		else 
		{
			startEraseLine++;
			
			mOffscreen->Lock();
			drawView = mOffscreen->ChildAt(0);
			
			BRect	offBounds = mOffscreen->Bounds();
			float	lineHeight = (line + 1)->origin - line->origin;
			if (offBounds.Height() < lineHeight) {
				// bitmap isn't tall enough for the current line, resize
				color_space colors = mOffscreen->ColorSpace();
				delete (mOffscreen);
				mOffscreen = NULL;

				offBounds.bottom = lineHeight;
				mOffscreen = new BBitmap(offBounds, colors, true);
				drawView = new BView(offBounds, B_EMPTY_STRING, B_FOLLOW_NONE, 0);
				mOffscreen->Lock();				
				mOffscreen->AddChild(drawView);
			}
			
			drawView->SetLowColor(viewColor);
			drawView->SetDrawingMode(B_OP_COPY);
			drawView->FillRect(offBounds, B_SOLID_LOW);
			
			penLoc.Set(mTextRect.left, line->ascent);
			drawView->MovePenTo(penLoc);
			
			lineRect.top = 0.0;
			lineRect.bottom = lineHeight - 1.0;
		}

		// fill with the hilite color any selected text
		if (drawHilite)
		{
			const int32		lineOffset = line->offset;
			const int32		right = line->offset + length;
		
			if ((lineOffset >= mSelStart && lineOffset < mSelEnd) ||
				(mSelStart >= lineOffset && mSelStart <= right) ||
				(mSelEnd > line->offset && mSelEnd <= right))
			{
				lineRect.left = ceil(OffsetToPoint(mSelStart <= lineOffset ? lineOffset : mSelStart).x);
				lineRect.left = (lineRect.left < mTextRect.left) ? mTextRect.left : lineRect.left;

				if (mSelEnd >= (line + 1)->offset)
					lineRect.right = clipRect.right;
				else
					lineRect.right = ceil(OffsetToPoint(mSelEnd).x) - 1.0;

				drawView->SetLowColor(hiliteColor);
				drawView->SetDrawingMode(B_OP_COPY);
				drawView->FillRect(lineRect, B_SOLID_LOW);
				drawView->SetLowColor(viewColor);
				drawView->SetDrawingMode(B_OP_OVER);		// uses background color for antialiasing
			}
			else
				drawView->SetDrawingMode(B_OP_COPY);
		}
		else
			drawView->SetDrawingMode(B_OP_COPY);

		// do we have any text to draw?
		if (length > 0) {
			// iterate through each style on this line
			BPoint				startPenLoc = penLoc;
			bool 				foundTab = false;
			int32				tabChars = 0;
			int32 				offset = line->offset;
			ConstSTEStylePtr	style = NULL;

			while (int32 numChars = mStyles.Iterate(offset, length, &style)) {
				drawView->SetFont(style);
				drawView->SetHighColor(style->color);
				tabChars = numChars;

				do {
					startPenLoc = penLoc;

					foundTab = mText.FindChar('\t', offset, &tabChars);					
					
					const float		stringWidth = sWidths.StringWidth(mText, offset, tabChars, style);

					drawView->DrawString(mText.GetString(offset, tabChars), tabChars);
					
					penLoc.x += stringWidth;

					if (foundTab) {
						int32 numTabs = 0;
						for (numTabs = 0; (tabChars + numTabs) < numChars; numTabs++) {
							if (mText[offset + tabChars + numTabs] != '\t')
								break;
						}
												
						float tabWidth = ActualTabWidth(penLoc.x - mTextRect.left);
						if (numTabs > 1)
							tabWidth += ((numTabs - 1) * mTabWidth);

						tabChars += numTabs;
						penLoc.x += tabWidth;
						drawView->MovePenTo(penLoc);
					}

					offset += tabChars;
					length -= tabChars;
					numChars -= tabChars;
					tabChars = numChars;
				} while ((foundTab) && (tabChars > 0));
			}
		}

		if (drawView != this) {
			BRect 	offBounds = mOffscreen->Bounds();
			float	lineHeight = (line + 1)->origin - line->origin;
			
			BRect 	srcRect = offBounds;
			srcRect.bottom = srcRect.top + lineHeight - 1.0;

			BRect 	dstRect(0.0, line->origin + mTextRect.top, offBounds.Width(), 0.0);
			dstRect.bottom = dstRect.top + lineHeight - 1.0;

			drawView->Sync();
			DrawBitmap(mOffscreen, srcRect, dstRect);
			
			mOffscreen->Unlock();
		}
		
		line++;
	}

	ConstrainClippingRegion(NULL);
}

// ------------------------------------------------------------
// 	InsetRegion
// ------------------------------------------------------------

void
STEngine::InsetRegion(
	BRegion&	inoutRegion) const
{
	BRect			r[3];
	const int32		count = inoutRegion.CountRects();
	
	for (int i = 0; i < count; i++)
	{
		r[i] = inoutRegion.RectAt(i);
		r[i].InsetBy(1.0, 1.0);
	}
	
	inoutRegion.MakeEmpty();

	switch (count)
	{
		case 1:
			inoutRegion.Include(r[0]);
			break;
		case 2:
			if (r[0].left <= r[1].right)
			{
				if (r[0].left == r[1].left)
					r[1].top -= 2.0;
				else
				{
					r[2].Set(r[0].left, r[0].top, r[1].right, r[1].bottom);
					inoutRegion.Include(r[2]);
				}
			}
			inoutRegion.Include(r[0]);
			inoutRegion.Include(r[1]);
			break;
		case 3:
			r[0].bottom += 2.0;
			r[2].top -= 2.0;
			inoutRegion.Include(r[0]);
			inoutRegion.Include(r[1]);
			inoutRegion.Include(r[2]);
			break;
	}
}

// ------------------------------------------------------------
// 	DrawSelection
// ------------------------------------------------------------
// Hilite the characters between startOffset and endOffset

void
STEngine::DrawSelection(
	int32	startOffset,
	int32	endOffset)
{		
	// get real
	if (startOffset >= endOffset)
		return;
	
	BRegion 	selRegion;
	GetHiliteRegion(startOffset, endOffset, &selRegion);

	if (! mActive)
	{
		BRegion		insetRegion = selRegion;
		InsetRegion(insetRegion);

		selRegion.Exclude(&insetRegion);
	}

	SetDrawingMode(B_OP_SELECT);
	SetHighColor(HiliteColor());
	SetLowColor(ViewColor());
	FillRegion(&selRegion);
	SetDrawingMode(B_OP_COPY);
}

// ------------------------------------------------------------
// 	DrawCaret
// ------------------------------------------------------------
// Draw the caret at offset

void
STEngine::DrawCaret(
	int32	offset)
{
	float	lineHeight = 0.0;
	BPoint	caretPoint = OffsetToPoint(offset, &lineHeight);
	caretPoint.x = (caretPoint.x > mTextRect.right) ? mTextRect.right : caretPoint.x;
	
	BRect caretRect;
	caretRect.left = caretRect.right = caretPoint.x;
	caretRect.top = caretPoint.y;
	caretRect.bottom = caretPoint.y + lineHeight;
	
	InvertRect(caretRect);
}

// ------------------------------------------------------------
// 	InvertCaret
// ------------------------------------------------------------
// Invert the caret at mSelStart

void
STEngine::InvertCaret()
{
	DrawCaret(mSelStart);
	mCaretVisible = !mCaretVisible;
	mCaretTime = system_time();
}

// ------------------------------------------------------------
// 	DragCaret
// ------------------------------------------------------------
// Draw a temporary caret at offset 

void
STEngine::DragCaret(
	int32	offset)
{
	// does the caret need to move?
	if (offset == mDragOffset)
		return;
	
	// hide the previous drag caret
	if (mDragOffset != -1)
		DrawCaret(mDragOffset);
		
	// do we have a new location?
	if (offset != -1) {
		if (mActive) {
			// ignore if offset is within active selection
			if ((offset >= mSelStart) && (offset <= mSelEnd)) {
				mDragOffset = -1;
				return;
			}
		}
		
		DrawCaret(offset);
	}
	
	mDragOffset = offset;
}

// ------------------------------------------------------------
// 	TrackDrag
// ------------------------------------------------------------
// Track and give feedback for a drag
//
// This function gets called repeatedly while there is a drag
// that needs to be tracked
//
// When mDragOwner is true, this function will get called
// even when the drag is not above this view
//
// When this view is the drag owner (mDragOwner), it is assumed
// that it can handle its own drag (CanDrop() is not consulted)

void
STEngine::TrackDrag(
	BPoint	where)
{
	BRect bounds = Bounds();

	// are we the drag owner?
	if (!mDragOwner) {
		// drag isn't ours, don't worry about auto-scrolling
		if (bounds.Contains(where))
			DragCaret(PointToOffset(where));
		return;
	}
	
	// expand the bounds
	bounds.InsetBy(-B_V_SCROLL_BAR_WIDTH, -B_H_SCROLL_BAR_HEIGHT);
	
	// is the mouse within the expanded bounds?
	if (bounds.Contains(where)) {
		int32 hDelta = 0;
		int32 vDelta = 0;
		
		BScrollBar *hScroll = ScrollBar(B_HORIZONTAL);
		if (hScroll != NULL) {
			// left edge
			if (where.x < (bounds.left + B_V_SCROLL_BAR_WIDTH)) {
				hDelta = (int32) (where.x - (bounds.left + B_V_SCROLL_BAR_WIDTH));
			}
			else {
				// right edge
				if (where.x > (bounds.right - B_V_SCROLL_BAR_WIDTH))
					hDelta = (int32) (where.x - (bounds.right - B_V_SCROLL_BAR_WIDTH));
			}
			
			if (hDelta != 0) {
				DragCaret(-1);
				hScroll->SetValue(hScroll->Value() + (hDelta * 5));
			}
		}
		
		BScrollBar *vScroll = ScrollBar(B_VERTICAL);
		if (vScroll != NULL) {	
			// top edge
			if (where.y < (bounds.top + B_H_SCROLL_BAR_HEIGHT)) {
				vDelta = (int32) (where.y - (bounds.top + B_H_SCROLL_BAR_HEIGHT));
			}
			else {
				// bottom edge
				if (where.y > (bounds.bottom - B_H_SCROLL_BAR_HEIGHT))
					vDelta = (int32) (where.y - (bounds.bottom - B_H_SCROLL_BAR_HEIGHT));
			}
			
			if (vDelta != 0) {
				DragCaret(-1);
				vScroll->SetValue(vScroll->Value() + (vDelta * 5));
			}
		}
		
		if ((hDelta != 0) || (vDelta != 0))
			Window()->UpdateIfNeeded();
		else
			DragCaret(PointToOffset(where));
	}
}

// ------------------------------------------------------------
// 	InitiateDrag
// ------------------------------------------------------------
// Create a new drag message and pass it on to the app_server

void
STEngine::InitiateDrag()
{	
	// create a new drag message
	BMessage *drag = new BMessage(B_SIMPLE_DATA);
	
	// add the text
	drag->AddData("text/plain", B_MIME_TYPE, mText.Text() + mSelStart, 
			  	  mSelEnd - mSelStart);
	
	// get the corresponding styles
	int32 				styleLen = 0;
	STEStyleRangePtr	styles = GetStyleRange(mSelStart, mSelEnd, &styleLen);
	
	// reset the extra fields, override if you don't like this behavior
	for (int32 i = 0; i < styles->count; i++)
		styles->runs[i].style.extra = 0;
	
	drag->AddData("style", STE_STYLE_TYPE, styles, styleLen);

	BRegion hiliteRgn;
	GetHiliteRegion(mSelStart, mSelEnd, &hiliteRgn);
	BRect dragRect = hiliteRgn.Frame();
	BRect bounds = Bounds();	
	if (!bounds.Contains(dragRect))
		dragRect = bounds & dragRect;
		
	be_app->SetCursor(B_HAND_CURSOR);
	DragMessage(drag, dragRect);
	
	// we're a proud owner of a drag
	mDragOwner = true;
	Window()->SetPulseRate(100000.0);
}

// ------------------------------------------------------------
// 	MessageDropped
// ------------------------------------------------------------
// Respond to dropped messages

bool
STEngine::MessageDropped(
	BMessage	*inMessage,
	BPoint 		where,
	BPoint		offset)
{
#pragma unused(offset)
		
	if (mDragOwner) {
		// our drag has come to an end
		mDragOwner = false;
		Window()->SetPulseRate(500000.0);
	}
	
	// make sure the drag caret is erased
	DragCaret(-1);

	if (mActive)
		be_app->SetCursor(B_I_BEAM_CURSOR);
		
	// are we sure we like this message?
	if (!CanDrop(inMessage))
		return (false);
		
	int32 dropOffset = PointToOffset(where);
	
	// if this was our drag, move instead of copy
	if ((mActive) && (mSelStart != mSelEnd)) {
		// dropping onto itself?
		if ((dropOffset >= mSelStart) && (dropOffset <= mSelEnd))
			return (true);
			
		// adjust the offset if the drop is after the selection
		if (dropOffset > mSelEnd)
			dropOffset -= (mSelEnd - mSelStart);
			
		// delete the selection
		DrawSelection(mSelStart, mSelEnd);
		RemoveRange(mSelStart, mSelEnd);
		mSelEnd = mSelStart;
		Refresh(mSelStart, mSelEnd, true, false);
	}
		
	Select(dropOffset, dropOffset);
	
	ssize_t		dataLen = 0;
	char *		text = NULL;
	status_t	err = inMessage->FindData("text/plain", B_MIME_TYPE, (const void**)&text, &dataLen);

	if (err == B_NO_ERROR && text != NULL) 
	{
		ssize_t				styleLen = 0;
		STEStyleRangePtr 	styles = NULL;
		err = inMessage->FindData("style", STE_STYLE_TYPE, (const void**)&styles, &styleLen);
	
		Insert(text, dataLen, styles);	
		Select(dropOffset, dropOffset + dataLen);		// for outline hiliting
	}
	
	if (!mActive)
		DrawSelection(mSelStart, mSelEnd);

	return (true);
}

// ------------------------------------------------------------
// 	UpdateScrollbars
// ------------------------------------------------------------
// Adjust the scroll bars so that they reflect the current 
// visible size and position of the text area

void
STEngine::UpdateScrollbars()
{
	BRect bounds = Bounds();
	
	// do we have a horizontal scroll bar?
	BScrollBar *hScroll = ScrollBar(B_HORIZONTAL);

	if (hScroll != NULL) {
		float viewWidth = bounds.Width();
		float dataWidth = mTextRect.Width();
		dataWidth += (ceil(mTextRect.left) + 1.0);
		
		float maxRange = dataWidth - viewWidth;
		maxRange = (maxRange < 0) ? 0 : maxRange;
		
		hScroll->SetRange(0.0, maxRange);
		hScroll->SetSteps(10.0, dataWidth / 10.0f);

		float proportion = viewWidth / dataWidth;
		if (hScroll->Proportion() != proportion) {
			hScroll->SetProportion(proportion);
		}
	}

	// how about a vertical scroll bar?
	BScrollBar *vScroll = ScrollBar(B_VERTICAL);
	if (vScroll != NULL) {
		float viewHeight = bounds.Height();
		float dataHeight = mTextRect.Height();
		dataHeight += (ceil(mTextRect.top) + 1);
		
		float maxRange = dataHeight - viewHeight;
		maxRange = (maxRange < 0) ? 0 : maxRange;
		
		vScroll->SetRange(0.0, maxRange);
		vScroll->SetSteps(12.0, viewHeight);

		float proportion = viewHeight / dataHeight;
		if (vScroll->Proportion() != proportion) {
			vScroll->SetProportion(proportion);
		}
	}
}

// ------------------------------------------------------------
// 	Activate
// ------------------------------------------------------------
// Activate the text area
//
// Draw the caret/hilite the selection, set the cursor

void
STEngine::Activate()
{
	mActive = true;

	if (mSelStart != mSelEnd)	{
		if (mSelectable)
		{
			// draw selected text
			int32 startLine = OffsetToLine(mSelStart);
			int32 endLine = OffsetToLine(mSelEnd > 1 && mText[mSelEnd - 1] == '\n' ? mSelEnd - 1 : mSelEnd);

			DrawLines(startLine, endLine, -1, true);
		}
	}
	else {
//		if (mEditable)
			InvertCaret();
	}
	
	BPoint 	where;
	uint32	buttons;
	GetMouse(&where, &buttons);
	if (Bounds().Contains(where))
		be_app->SetCursor(B_I_BEAM_CURSOR);
}

// ------------------------------------------------------------
// 	Deactivate
// ------------------------------------------------------------
// Deactivate the text area
//
// Hide the caret/unhilite the selection, set the cursor

void
STEngine::Deactivate()
{
	mActive = false;
	
	if (mSelStart != mSelEnd) {
		if (mSelectable)
		{
			// remove active hiliting
			int32 startLine = OffsetToLine(mSelStart);
			int32 endLine = OffsetToLine(mSelEnd > 1 && mText[mSelEnd - 1] == '\n' ? mSelEnd - 1 : mSelEnd);

			DrawLines(startLine, endLine, -1, true);
			
			// draw outline hiliting
			DrawSelection(mSelStart, mSelEnd);
		}
	}
	else {
		if (mCaretVisible)
			InvertCaret();
	}
	
	BPoint 	where;
	uint32	buttons;
	GetMouse(&where, &buttons);
	if (Bounds().Contains(where))
		be_app->SetCursor(B_HAND_CURSOR);
}

// ------------------------------------------------------------
// 	HandleModification
// ------------------------------------------------------------
// This function gets called when the text or a style run
// has been modified
//
// The default implementation of this function is empty, override
// if you wish to do something useful

void
STEngine::HandleModification()
{
}
