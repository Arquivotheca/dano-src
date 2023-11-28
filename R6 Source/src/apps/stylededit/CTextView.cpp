// ============================================================
//  CTextView.cpp	©1996 Hiroshi Lockheimer
// ============================================================

#include <Debug.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <Beep.h>
#include "CTextView.h"
#include "CStyledEditWindow.h"
#include "CStyledEditApp.h"


//#include <MessageFilter.h>

//class KeyFilter :
//	public BMessageFilter
//{
//public:
//		KeyFilter(
//				BView * view) :
//			BMessageFilter(B_KEY_DOWN)
//			{
//				m_view = view;
//			}
//virtual		filter_result Filter(
//				BMessage * message,
//				BHandler ** handler)
//			{
//				int32 modifiers;
//				const char * bytes;
//				if (!message->FindInt32("modifiers", &modifiers) && 
//					!message->FindString("bytes", &bytes) && 
//					(modifiers & B_COMMAND_KEY) && 
//					((*bytes) == B_LEFT_ARROW || (*bytes) == B_RIGHT_ARROW || (*bytes) == B_UP_ARROW || (*bytes) == B_DOWN_ARROW))
//				{
//					m_view->KeyDown(bytes, strlen(bytes));
//					return B_SKIP_MESSAGE;
//				}
//				return B_DISPATCH_MESSAGE;
//			}
//private:
//		BView * m_view;
//};


inline char
utf8_safe_tolower(char c)
	{ return ((((uint8)c) < 0x80) ? tolower(c) : c); };


CTextView::CTextView(
	BRect				frame,
	const char			*name, 
	BRect				textRect)
		: BTextView(frame, name, textRect, B_FOLLOW_ALL_SIDES,
		  B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE | B_FRAME_EVENTS)
{
	SetStylable(TRUE);
	SetWordWrap(TRUE);
	SetDoesUndo(TRUE);
}


void
CTextView::AttachedToWindow()
{
	BTextView::AttachedToWindow();
//	m_filter = new KeyFilter(this);
//	Window()->AddCommonFilter(m_filter);
}


void
CTextView::DetachedFromWindow()
{
	BTextView::DetachedFromWindow();
//	Window()->RemoveCommonFilter(m_filter);
//	delete m_filter;
}


void
CTextView::KeyDown(
	const char	*bytes,
	int32		numBytes)
{
	// try and do cmd-arrowkey shortcuts
//	uint32 modifiers = 0;
//	Window()->CurrentMessage()->FindInt32("modifiers", (int32 *)&modifiers);
//	if (modifiers & B_OPTION_KEY) {
//		int32 selstart, selend;
//		GetSelection(&selstart, &selend);
//		bool set = false;
//		if (*bytes == B_LEFT_ARROW) {
//			const char * t = Text();
//			if (selstart > 0) {
//				while (selstart > 0 && !(isalnum(t[selstart-1])||(t[selstart-1]=='_'))) {
//					selstart--;
//				}
//				while (selstart > 0 && (isalnum(t[selstart-1])||(t[selstart-1]=='_'))) {
//					selstart--;
//				}
//			}
//			if (!(modifiers & B_SHIFT_KEY)) {
//				selend = selstart;
//			}
//			set = true;
//		}
//		else if (*bytes == B_RIGHT_ARROW) {
//			const char * t = Text();
//			int32 end = TextLength();
//			while (selend < end && !(isalnum(t[selend])||(t[selend]=='_'))) {
//				selend++;
//			}
//			while (selend < end && (isalnum(t[selend])||(t[selend]=='_'))) {
//				selend++;
//			}
//			if (!(modifiers & B_SHIFT_KEY)) {
//				selstart = selend;
//			}
//			set = true;
//		}
//		if (set) {
//			Select(selstart, selend);
//			return;
//		}
//	}
//	else if (modifiers & B_COMMAND_KEY) {
//		int32 selstart, selend;
//		GetSelection(&selstart, &selend);
//		bool set = false;
//		if (*bytes == B_LEFT_ARROW) {
//			if (selstart > 1) {
//				int32 line = LineAt(selstart);
//				selstart = OffsetAt(line);
//				if (!(modifiers & B_SHIFT_KEY)) {
//					selend = selstart;
//				}
//			}
//			set = true;
//		}
//		else if (*bytes == B_RIGHT_ARROW) {
//			int32 end = TextLength();
//			if (selend < end) {
//				int32 line;
//				if (selstart < selend) {
//					line = LineAt(selend ? selend-1 : selend);
//					selend = OffsetAt(line+1);
//				}
//				else {
//					line = LineAt(selend);
//					selend = OffsetAt(line+1);
//					if (!(modifiers & B_SHIFT_KEY)) {
//						selend--;
//					}
//				}
//				if (!(modifiers & B_SHIFT_KEY)) {
//					selstart = selend;
//				}
//			}
//			set = true;
//		}
//		else if (*bytes == B_UP_ARROW) {
//			selstart = 0;
//			if (!(modifiers & B_SHIFT_KEY)) {
//				selend = 0;
//			}
//			set = true;
//		}
//		else if (*bytes == B_DOWN_ARROW) {
//			selend = TextLength();
//			if (!(modifiers & B_SHIFT_KEY)) {
//				selstart = selend;
//			}
//			set = true;
//		}
//		if (set) {
//			Select(selstart, selend);
//			ScrollToSelection();
//			return;
//		}
//	}

	// better feedback for non-editable views
	// NOTE: some of this should be pushed down into BTextView.
	uchar theChar = bytes[0];
	if (!IsEditable()) {
		switch( theChar ) {
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW:
			case B_UP_ARROW:
			case B_DOWN_ARROW:
			case B_HOME:
			case B_END:
			case B_PAGE_UP:
			case B_PAGE_DOWN:
				break;
			default:
				beep();
				return;
		}
	}

	BTextView::KeyDown(bytes, numBytes);
}


void
CTextView::WindowActivated(
	bool	state)
{
	BTextView::WindowActivated(state);
}


void
CTextView::MessageReceived(
	BMessage	*message)
{
	if (message->WasDropped()) {
		if (!AcceptsDrop(message)) {
			beep();
			return;
		}
	}

	BTextView::MessageReceived(message);
}


void
CTextView::FrameResized(
	float	width,
	float	height)
{
	BTextView::FrameResized(width, height);

	if (DoesWordWrap()) {
		BRect textRect = Bounds();
		textRect.OffsetTo(B_ORIGIN);
		textRect.InsetBy(3.0, 3.0);

		SetTextRect(textRect);
	}
}


void
CTextView::Paste(
	BClipboard	*clipboard)
{
	if (!IsEditable()) {
		beep();
		return;
	}

	BTextView::Paste(clipboard);
}


void
CTextView::InsertText(
	const char				*inText, 
	int32					inLength, 
	int32					inOffset,
	const text_run_array	*inRuns)
{
	BTextView::InsertText(inText, inLength, inOffset, inRuns);

	((CStyledEditWindow *)Window())->SetDirty(true);
}


void
CTextView::DeleteText(
	int32	fromOffset,
	int32	toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);
	
	((CStyledEditWindow *)Window())->SetDirty(true);
}


// stolen from the pre-DR9 Edit...

void
CTextView::Search(
	const char	*target, 
	bool		forward, 
	bool		wrap,
	bool		sensitive)
{
	bool result;

	if (forward)
		result = SearchForward(target, wrap, sensitive);
	else
		result = SearchBackward(target, wrap, sensitive);
	
	if (result)
		ScrollToSelection();
}

//----------------------------------------------------------------------
// SearchForward-- This routine actually implements the search. Ugly brute
//	       force search straight of of Sedgewick's "Algorithms in C"
//			Return TRUE if we found a match
//----------------------------------------------------------------------

bool
CTextView::SearchForward(
	const char	*text, 
	bool		wrap, 
	bool		sensitive)
{
	const char	*theText;
	char		*data;
	long		matchPos;
	long		startPos;
	long		selStart, selEnd;
	long		i, j;
	long		slen = strlen(text);
	long		textLen;

	if (slen == 0)
		return FALSE;
	
	textLen = TextLength();

	// Determine if we have an existing selection (selStart != selEnd)
	// If we do, go one space past the selection; otherwise use current pos 
	GetSelection(&selStart, &selEnd);
	if (selStart != selEnd)
		startPos = selEnd;
	else
		startPos = selStart;
	
//+	PRINT(("search for=%s, startPos = %d, tlen=%d\n",
//+		text, startPos, textLen));

	// if we're wrapping and the starting postion is at the end --> reposition
	if (wrap && (startPos >= textLen))
		startPos = 0;

	theText = Text();	// Get (null terminated) ptr to text in view

	// make copy of text, converting to lower case if necessary
	data = (char *)malloc(slen + 1);
	for (i = 0; i < slen; i++)
		data[i] = sensitive ? text[i] : utf8_safe_tolower(text[i]);

	/*
	 Duplicate the following 'for' loop for the Case-Sensitive vs. Insensitive
	 search. Don't put the check inside the 'for' loop to save a little
	 performance. The 2 copies of the 'for' loop should be kept
	 idnetical in every other respect.
	*/
	if (sensitive) {
		for (i = startPos, j = 0; j < slen && i < textLen; i++, j++) {
			while ((i < textLen) && (theText[i] != data[j])) {
				i -= (j-1);
				j = 0;
			}
			if (wrap && (j+1 < slen) && (i+1 >= textLen)) {
				wrap = FALSE;
				i = -1;
				j = 0;
				textLen = startPos;		// stop searching at start position
			}
		}
	} else {
		for (i = startPos, j = 0; j < slen && i < textLen; i++, j++) {
			while ((i < textLen) && (utf8_safe_tolower(theText[i]) != data[j])) {
				i -= (j-1);
				j = 0;
			}
			if (wrap && (j+1 < slen) && (i+1 >= textLen)) {
				wrap = FALSE;
				i = -1;
				j = 0;
				textLen = startPos;		// stop searching at start position
			}
		}
	}

	if ((slen == 1) && (j == slen)) {
	// pjp There is some bug in the above code when dealing with one character
	// long strings. I'm trying to work around it here.
		matchPos = i-slen;
//+		PRINT(("BUG - pos=%d, (%x == %x)\n", matchPos, data[0], theText[matchPos]));
		if (data[0] != ((sensitive) ? theText[matchPos] : utf8_safe_tolower(theText[matchPos])))
			j = 0;
	}

	free(data);

	if (j == slen) {
		matchPos = i - slen;
//+		PRINT(("MATCH: pos=%d, i=%d, j=%d, slen=%d, textLen=%d\n",
//+			matchPos, i, j, slen, textLen));
		this->Select(matchPos, matchPos + slen);
		return TRUE;
	} 

//+	PRINT(("NOT  : i=%d, j=%d, slen=%d, textLen=%d\n",
//+		i, j, slen, textLen));

	return FALSE;
}

	
//----------------------------------------------------------------------
// SearchBackward -- reverse of above
//----------------------------------------------------------------------

bool
CTextView::SearchBackward(
	const char	*text,
	bool		wrap,
	bool		sensitive)
{
	const char	*theText;
	char		*data;
	long		matchPos;
	long		selStart, selEnd;
	long		i, j;
	long		slen = strlen(text);
	long		textLen;
	long		zero = 0;
	
	if (slen == 0)
		return FALSE;

	GetSelection(&selStart, &selEnd);
		
	selStart--;

	textLen = TextLength();

	theText = Text();	// Get (null terminated) ptr to text in view

	// if we're wrapping and current position is at front - reposition
	if (wrap && (selStart < 0))
		selStart = textLen - 1;

	// make copy of text, converting to lower case if necessary
	data = (char *)malloc(slen + 1);
	for (i = 0; i < slen; i++)
		data[i] = sensitive ? text[i] : utf8_safe_tolower(text[i]);
		
	/*
	 Duplicate the following 'for' loop for the Case-Sensitive vs. Insensitive
	 search. Don't put the check inside the 'for' loop to save a little
	 performance. The 2 copies of the 'for' loop should be kept
	 idnetical in every other respect.
	*/
	if (sensitive) {
		for (i = selStart, j = slen-1; (j >= 0) && (i >= zero); i--, j--) {
			while ((i >= zero) && (utf8_safe_tolower(theText[i]) != data[j])) {
				i += slen-j-2;
				j = slen-1;
			}
			if (wrap && (j-1 >= 0) && (i-1 < 0)) {
				wrap = FALSE;
				j = slen-1;
				i = textLen - 1;
				zero = selEnd;
			}
		}
	} else {
		for (i = selStart, j = slen-1; (j >= 0) && (i >= zero); i--, j--) {
			while ((i >= zero) && (utf8_safe_tolower(theText[i]) != data[j])) {
				i += slen-j-2;
				j = slen-1;
			}
			if (wrap && (j-1 >= 0) && (i-1 < 0)) {
				wrap = FALSE;
				j = slen-1;
				i = textLen - 1;
				zero = selEnd;
			}
		}
	}

	if ((slen == 1) && (j < 0)) {
	// pjp There is some bug in the above code when dealing with one character
	// long strings. I'm trying to work around it here.
		matchPos = i+1;
//+		PRINT(("BUG - pos=%d, (%x == %x)\n", matchPos, data[0], theText[matchPos]));
		if (data[0] != utf8_safe_tolower(theText[matchPos]))
			j = 0;
	}

	free(data);

	if (j < 0) {
		matchPos = i+1;
//		PRINT(("MATCH: pos=%d, i=%d, j=%d, slen=%d\n",
//			matchPos, i, j, slen));
		Select(matchPos, matchPos + slen);
		return TRUE;
	}

	return FALSE;
}

void
CTextView::Replace(bool once, const char *text, const char *newText,
	bool forward, bool wrap, bool sensitive)
{
	long	found;

	if (forward)
		found = ReplaceForward(once, text, newText, wrap, sensitive);
	else
		found = ReplaceBackward(once, text, newText, wrap, sensitive);
	
	if (found)
		ScrollToSelection();
}

//----------------------------------------------------------------------
bool
CTextView::ReplaceForward(bool once, const char *text,
	const char *newText, bool wrap, bool sensitive)
{
	long	found = 0;
	long	curStart;
	long	curEnd;
	long	initialStart;
	long	initialEnd;
	long	prevStart;
	long	prevEnd;
	long	oldLength;
	bool	wrapped = FALSE;

	GetSelection(&initialStart, &initialEnd);

	prevStart = prevEnd = 0;

	while(SearchForward(text, wrap, sensitive)) {
		GetSelection(&curStart, &curEnd);
		if (wrap) {
			if (!wrapped && (curStart < prevStart))
				wrapped = TRUE;
			if (wrapped && (curStart > initialStart)) {
				// we're finished, but let's restore previous selection pt
				Select(prevStart, prevEnd);
				break;
			}
		}
		found++;

		// the first time we find something Commit previous editing. Don't
		// do it sooner in case there is nothing to replace.
//		if (found == 1)
//			Commit();

		oldLength = TextLength();
		Delete();
		Insert(newText);
//		UpdateMarkers(curStart, curEnd, TextLength() - oldLength);
		GetSelection(&curStart, &curEnd);
		prevStart = curStart;
		prevEnd = curEnd;
		
		if (once)
			break;
	}

//	if (found)
//		SetDirty(TRUE);
	
	return found;
}

//----------------------------------------------------------------------
bool
CTextView::ReplaceBackward(bool once, const char *text,
	const char *newText, bool wrap, bool sensitive)
{
	long	found = 0;
	long	curStart;
	long	curEnd;
	long	initialStart;
	long	initialEnd;
	long	prevStart;
	long	prevEnd;
	long	oldLength;
	bool	wrapped = FALSE;

	GetSelection(&initialStart, &initialEnd);

	prevStart = prevEnd = TextLength();

	while(SearchBackward(text, wrap, sensitive)) {
		GetSelection(&curStart, &curEnd);
		if (wrap) {
			if (!wrapped && (curStart > prevStart))
				wrapped = TRUE;
			if (wrapped && (curStart < initialStart)) {
				// we're finished, but let's restore previous selection pt
				Select(prevStart, prevEnd);
				break;
			}
		}
		found++;

		// the first time we find something Commit previous editing. Don't
		// do it sooner in case there is nothing to replace.
//		if (found == 1)
//			Commit();

		oldLength = TextLength();
		Delete();
		Insert(newText);
//		UpdateMarkers(curStart, curEnd, TextLength() - oldLength);
		GetSelection(&curStart, &curEnd);
		prevStart = curStart;
		prevEnd = curEnd;
		
		if (once)
			break;
	}

//	if (found)
//		SetDirty(TRUE);
	
	return found;
}


void
CTextView::GetDragParameters(
	BMessage	*drag,
	BBitmap		**bitmap,
	BPoint		*point,
	BHandler	**handler)
{
	BTextView::GetDragParameters(drag, bitmap, point, handler);

	const char	*kClippingName = "Clipping from %s";
	const char	*docName = Window()->Title();
	char		*clipName = (char *)malloc(strlen(kClippingName) + strlen(docName) + 1);
	sprintf(clipName, kClippingName, docName);
	
	drag->AddString("be:clip_name", clipName);

	free(clipName);
}


