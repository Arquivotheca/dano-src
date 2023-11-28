/*	$Id: XView.cpp,v 1.4 1998/11/17 12:16:51 maarten Exp $

	Copyright 1997, Hekkelman Programmatuur

*/

#include "bdb.h"
#include "XView.h"
#include "DNub.h"
#include "DTeam.h"

#include <Region.h>
#include <ScrollBar.h>
#include <Application.h>
#include <Window.h>
#include <Clipboard.h>
#include <Beep.h>

#include <ctype.h>

using std::max;
using std::min;

const char gHex[] = "0123456789ABCDEF";
const bigtime_t kCaretTime = 500000;

enum eUndoKey {
	kCut = 256,
	kPaste
};

enum eUndoState {
	uEmpty,
	uTyping,
	uMoved
};

static void MakeOutline(BRegion& rgn)
{
	BRegion t1, t2, t3;
	
	t1 = rgn;
	t1.OffsetBy(1, 1);
	
	t2 = rgn;
	t2.OffsetBy(-1, -1);
	
	t3 = rgn;
	
	t3.Exclude(&t1);
	rgn.Exclude(&t2);
	rgn.Include(&t3);
} /* MakeOutline */

XView::XView(BRect frame, DTeam& team, ptr_t addr, size_t size)
	: BView(frame, "main", B_FOLLOW_TOP_BOTTOM,
		B_WILL_DRAW | B_NAVIGABLE | B_PULSE_NEEDED | B_FRAME_EVENTS),
	 fTeam(team)
{
	SetFont(be_fixed_font);
	be_fixed_font->GetHeight(&fFH);
	SetViewColor(B_TRANSPARENT_32_BIT);

	// We normally read one page of data at a time so that we don't get
	// errors reading beyond end of mapped data.  However, we need to read
	// enough pages so that we can show an entire object in one view.
	// While it would be tempting to just take the size and round it
	// up to a page size, this won't give us what we want.  
	// (Picture a 3k object sitting near the end of a page.)
	// We want to read enough bytes to start on a page and read to
	// the end of the object.
	// In other words (addr + size) - base_page_address ...and then
	// round that result up to a page size.
	// Doing a little factoring on the formula above gives:
	// addr - base_page_address + size
	
	fDataSize = addr - (addr & ~(B_PAGE_SIZE-1)) + size;
	// now round to multiple of page sizes
	fDataSize = (fDataSize & ~(B_PAGE_SIZE-1)) + B_PAGE_SIZE;
	
	// Get the data
	fData = new char[fDataSize];
	XView::InitializeMemoryBuffer(addr, size);
	
	fLines = max(fDataSize / 16 + ((fDataSize % 16) != 0), 1);

	fCharWidth = ceil(be_fixed_font->StringWidth("0"));
	fLineHeight = ceil(fFH.ascent + fFH.descent + fFH.leading);
	ResizeTo(fCharWidth * 68 + 6, Bounds().Height());
	
	fUpperNibble = false;
	fEditHex = false;
	fShowCaret = false;
	fCaretTime = 0;
	fActive = false;
//	fUndoState = uEmpty;
//	fUndoBuffer = NULL;
} /* XView::XView */

XView::~XView()
{
//••••••••••••••••••••
//	free(fData);
	delete[] fData;
} /* XView::~XView */

//void XView::SaveData()
//{
//	fItem->Seek(0, SEEK_SET);
//	fItem->SetSize(fDataSize);
//	CheckedWrite(*fItem, fData, fDataSize);
//} /* XView::SaveData */
	
void XView::Draw(BRect updateRect)
{
	int curLine = (int)floor(updateRect.top / fLineHeight);
	
	BRect r(Bounds());
	FillRect(r, B_SOLID_LOW);
	
	DrawAllLines(curLine);

	InvertSelection(fEditHex);
	fShowCaret = false;
} /* XView::Draw */

void XView::DrawLine(int lineNr)
{
	BPoint p;
	p.y = fLineHeight * (lineNr + 1);
	char h[42], s[18];
	
	BRect r(Bounds());
	r.top = p.y - fLineHeight + fFH.descent + fFH.leading;
	r.bottom = p.y + fFH.descent;
	FillRect(r, B_SOLID_LOW);
	
	p.x = 3;
	
	char *sp = s + 8;	
	int addr = fAddr + lineNr * 16;
	*sp = 0;
	*--sp = gHex[addr & 0x0000000F];
	*--sp = gHex[(addr >>= 4) & 0x0000000F];
	*--sp = gHex[(addr >>= 4) & 0x0000000F];
	*--sp = gHex[(addr >>= 4) & 0x0000000F];
	*--sp = gHex[(addr >>= 4) & 0x0000000F];
	*--sp = gHex[(addr >>= 4) & 0x0000000F];
	*--sp = gHex[(addr >>= 4) & 0x0000000F];
	*--sp = gHex[(addr >> 4) & 0x0000000F];
	DrawString(s, p);
	
	unsigned char *dp = (unsigned char *)fData + lineNr * 16;
	char *hp = h - 1;
	int end = min(16, fDataSize - lineNr * 16);

	sp = s - 1;
	 
	int i;
	for (i = 0; i < end; i++)
	{
		unsigned char c = *dp++;

		*++hp = gHex[c >> 4];
		*++hp = gHex[c & 0x0F];
		
		if (i & 1)
		{
			if (i == 7)
				*++hp = ' ';
			*++hp = ' ';
		}
		
		*++sp = isprint(c) ? c : '.';
	}
	
	*++hp = 0;
	*++sp = 0;
	
	p.x = 3 + 10 * fCharWidth;
	DrawString(h, p);
	
	p.x = 3 + 52 * fCharWidth;
	DrawString(s, p);
} /* XView::DrawLine */

void XView::DrawAllLines(int lineNr)
{
	int topLine, lpp, maxLine;

	lpp = (int)ceil(Bounds().Height() / fLineHeight);
	topLine = (int)ceil(fScrollBar->Value() / fLineHeight);
	maxLine = min(fLines, topLine + lpp);

	do	DrawLine(lineNr++);
	while (lineNr < maxLine);
	
	if (maxLine < topLine + lpp)
	{
		BRect r(Bounds());
		r.top = maxLine * fLineHeight + fFH.descent + fFH.leading;
		FillRect(r, B_SOLID_LOW);
	}
} /* XView::DrawAllLines */

void XView::MouseDown(BPoint where)
{
	if (fCaret == fAnchor)
		HideCaret();

	bool extend = Looper()->CurrentMessage()->FindInt32("modifiers") & B_SHIFT_KEY;
	int pos;
	bool inHex;

	if (extend)
	{
		Pt2Pos(where, pos, inHex);
		if (fEditHex == inHex)
			ChangeSelection(pos);
		else
			beep();
	}
	else
	{
		InvertSelection(fEditHex);
		Pt2Pos(where, fCaret, fEditHex);
		fAnchor = fCaret;
	}
	
	ulong btns;
	BPoint last;
	GetMouse(&where, &btns);
	
	while (btns)
	{
		if (where != last)
		{
			Pt2Pos(where, pos, inHex);
			
			if (fEditHex == inHex)
				ChangeSelection(pos);

			ScrollToCaret();
			
			last = where;
		}
		
		GetMouse(&where, &btns);
	}
	
	ToggleCaret();
} /* XView::MouseDown */

void XView::KeyDown(const char *bytes, int32 numBytes)
{
	HideCaret();
	
	long modifiers;
	Looper()->CurrentMessage()->FindInt32("modifiers", &modifiers);
	int curLine, caretLine, lpp;
	bool okToScroll = true, touch = false;

	lpp = (int)floor(Bounds().Height() / fLineHeight);
	curLine = (int)ceil(fScrollBar->Value() / fLineHeight);
	caretLine = fCaret / 16;
	
	switch (bytes[0])
	{
		case B_LEFT_ARROW:
			Step(B_LEFT_ARROW, modifiers & B_SHIFT_KEY);
			break;
		case B_RIGHT_ARROW:
			Step(B_RIGHT_ARROW, modifiers & B_SHIFT_KEY);
			break;
		case B_UP_ARROW:
			Step(B_UP_ARROW, modifiers & B_SHIFT_KEY);
			break;
		case B_DOWN_ARROW:
			Step(B_DOWN_ARROW, modifiers & B_SHIFT_KEY);
			break;
		case B_PAGE_DOWN:
			fScrollBar->SetValue((curLine + lpp) * fLineHeight);
			if (modifiers & B_SHIFT_KEY)
				ChangeSelection((fScrollBar->Value() / fLineHeight) * 16);
			else if (fAnchor == fCaret)
				fAnchor = fCaret = (int)(fScrollBar->Value() / fLineHeight) * 16;
			else
				okToScroll = false;
			fUpperNibble = false;
			break;
		case B_PAGE_UP:
			fScrollBar->SetValue(max((float)0, (curLine - lpp) * fLineHeight));
			if (modifiers & B_SHIFT_KEY)
				ChangeSelection((fScrollBar->Value() / fLineHeight) * 16);
			else if (fAnchor == fCaret)
				fAnchor = fCaret = (int)(fScrollBar->Value() / fLineHeight) * 16;
			else
				okToScroll = false;
			fUpperNibble = false;
			break;
		case B_END:
		{
			int end = modifiers & B_CONTROL_KEY ? fDataSize : fCaret + 15 - fCaret % 16;
			if (modifiers & B_SHIFT_KEY)
				ChangeSelection(end);
			else
				fAnchor = fCaret = end;
			fUpperNibble = false;
			break;
		}
		case B_HOME:
		{
			int home = modifiers & B_CONTROL_KEY ? 0 : fCaret - fCaret % 16;
			if (modifiers & B_SHIFT_KEY)
				ChangeSelection(home);
			else
				fAnchor = fCaret = home;
			fUpperNibble = false;
			break;
		}
		case B_TAB:
			InvertSelection(fEditHex);
			fEditHex = !fEditHex;
			InvertSelection(fEditHex);
			break;
		case B_BACKSPACE:
			if (fCaret <= 0)
			{
				beep();
				break;
			}
			if (fCaret == fAnchor)
				fAnchor = fCaret = max(fCaret - 1, 0);
		default:
			touch = true;

			if (fEditHex)
			{
				char c = toupper(bytes[0]);

				if (!isxdigit(bytes[0]))
					beep();
				else
				{
					int nibble = c > '9' ? c - 'A' + 10 : c - '0';

//					if (!fUpperNibble && fInsert)
//						Insert("", 1);
//					
					fData[fCaret] <<= 4;
					fData[fCaret] |= nibble;
					
					if (fUpperNibble)
					{
						fCaret++;
						fAnchor++;
						fUpperNibble = false;
					}
					else
						fUpperNibble = true;
					
					DrawLine(caretLine);
				}
			}
			else
			{
				if (fCaret < fDataSize - 1)
					memcpy(fData + fCaret, bytes, numBytes);

				DrawLine(caretLine);
	
				fAnchor += numBytes;
				fCaret += numBytes;
			}
			break;
	}
	
	if (okToScroll)
		ScrollToCaret();
	
	if (touch)
		SetDirty(true);
	
	ShowCaret();
} /* XView::KeyDown */

void XView::Step(int key, bool extend)
{
	if (!extend)
		InvertSelection(fEditHex);
	
	int newCaret = fCaret;
	
	switch (key)
	{
		case B_LEFT_ARROW:
			newCaret = max(0, fCaret - 1);
			break;
		case B_RIGHT_ARROW:
			newCaret = min(fDataSize, fCaret + 1);
			break;
		case B_UP_ARROW:
			newCaret = max(0, fCaret - 16);
			break;
		case B_DOWN_ARROW:
			newCaret = min(fDataSize, fCaret + 16);
			break;
		default:
			beep();
	}
	
	if (extend)
		ChangeSelection(newCaret);
	else
		fAnchor = fCaret = newCaret;

	fUpperNibble = false;
} /* XView::Step */

void XView::MessageReceived(BMessage *msg)
{
	int caretLine = fCaret / 16;
	
	switch (msg->what)
	{
		case B_COPY:
			DoCopy();
			break;
		
		case B_PASTE:
			DoPaste();
			DrawAllLines(caretLine);
			break;
		
		case B_SELECT_ALL:
			InvertSelection(fEditHex);
			fAnchor = 0;
			fCaret = fDataSize - 1;
			InvertSelection(fEditHex);
			break;
		
		default:
			BView::MessageReceived(msg);
	}
} /* XView::MessageReceived */

void XView::SetScrollBar(BScrollBar* scrollBar)
{
	fScrollBar = scrollBar;
	FrameResized(Bounds().Width(), Bounds().Height());	
	float beginningValue = (fAnchor&~0xf) / 16 * fLineHeight;
	fScrollBar->SetValue(beginningValue);
} /* XView::SetScrollBar */

void XView::GetSelection(BRegion& rgn, bool hex)
{
	if (fAnchor == fCaret)
		return;

	BRect r1, r2;

	int first, last;
	first = min(fAnchor, fCaret);
	last = max(fAnchor, fCaret);
	
	CharRect(first, hex, r1);
	CharRect(last, hex, r2);

	if (r1.top == r2.top)
		rgn.Include(r1 | r2);
	else if (hex)
	{
		r1.right = 3 + fCharWidth * 50;
		rgn.Include(r1);
		
		r1.left = 2 + fCharWidth * 10;
		
		r1.OffsetBy(0, fLineHeight);
		while (r1.top < r2.top)
		{
			rgn.Include(r1);
			r1.OffsetBy(0, fLineHeight);
		}
		
		r2.left = r1.left;
		rgn.Include(r2);
	}
	else
	{
		r1.right = 3 + fCharWidth * 68;
		rgn.Include(r1);
		
		r1.left = 2 + fCharWidth * 52;
		
		r1.OffsetBy(0, fLineHeight);
		while (r1.top < r2.top)
		{
			rgn.Include(r1);
			r1.OffsetBy(0, fLineHeight);
		}
		
		r2.left = r1.left;
		rgn.Include(r2);
	}
} /* XView::GetSelection */

void XView::Pt2Pos(BPoint where, int& pos, bool& inHex)
{
	int line = (int)floor(where.y / fLineHeight);
	
	if (where.x > fCharWidth * 51)
	{
		where.x -= fCharWidth * 52 + 3;
		pos = (int)floor(where.x / fCharWidth);
		pos = min(pos, 15);
		pos = max(pos, 0);
		pos += line * 16;
		pos = min(pos, fDataSize - 1);
		inHex = false;
	}
	else
	{
		where.x -= fCharWidth * 10 + 3;
		if (where.x >= fCharWidth * 35)
			where.x -= fCharWidth * 8;
		else if (where.x >= fCharWidth * 30)
			where.x -= fCharWidth * 7;
		else if (where.x >= fCharWidth * 25)
			where.x -= fCharWidth * 6;
		else if (where.x >= fCharWidth * 20)
			where.x -= fCharWidth * 5;
		else if (where.x >= fCharWidth * 14)
			where.x -= fCharWidth * 3;
		else if (where.x >= fCharWidth * 9)
			where.x -= fCharWidth * 2;
		else if (where.x >= fCharWidth * 4)
			where.x -= fCharWidth * 1;
		pos = (int)floor(where.x / (2 * fCharWidth));
		pos = min(pos, 15);
		pos = max(pos, 0);
		pos += line * 16;
		pos = min(pos, fDataSize - 1);
		inHex = true;
	}
} /* XView::Pt2Pos */

void XView::Pulse()
{
	if (fCaretTime < system_time() && fActive)
		ToggleCaret();
} /* XView::Pulse */

void XView::ToggleCaret()
{
	if (fCaret != fAnchor)
		return;

	fCaretTime = system_time() + kCaretTime;
	fShowCaret = !fShowCaret;

	SetDrawingMode(B_OP_INVERT);
	
	BRect c;
	CharRect(fCaret, fEditHex, c);

//	if (fInsert)
//		StrokeLine(c.LeftTop(), c.LeftBottom());
//	else
		FillRect(c);
	
	CharRect(fCaret, !fEditHex, c);
	
//	if (fInsert)
//		StrokeLine(c.LeftTop(), c.LeftBottom(), B_MIXED_COLORS);
//	else
		StrokeRect(c);
	
	SetDrawingMode(B_OP_COPY);
} /* XView::ToggleCaret */

void XView::MouseMoved(BPoint where, uint32 code, const BMessage* message)
{
	switch (code) 
	{
		case B_ENTERED_VIEW:
		case B_INSIDE_VIEW:
 			be_app->SetCursor(B_I_BEAM_CURSOR);
			break;

		case B_EXITED_VIEW:
			be_app->SetCursor(B_HAND_CURSOR);
			break;

		default:
			BView::MouseMoved(where, code, message);
			break;
	}
} /* XView::MouseMoved */ 

void XView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);

	int page = (int)(height / fLineHeight - 1);
	int lines = max(0, fLines - page);
	
	fScrollBar->SetRange(0, lines * fLineHeight);
	fScrollBar->SetSteps(fLineHeight, page * fLineHeight);
	
	Draw(Bounds());
} /* XView::FrameResized */

void XView::CharRect(int pos, bool hex, BRect& r)
{

	r.top = fLineHeight * (pos / 16) + fFH.leading + 1;

	if (hex)
	{
		pos %= 16;
		r.left = 2 + (10 + 2 * pos + pos / 2 + (pos > 7)) * fCharWidth;
		r.right = r.left + 2 * fCharWidth;
	}
	else
	{
		r.left = 2 + (52 + pos % 16) * fCharWidth;
		r.right = r.left + fCharWidth;
	}
	
	r.bottom = r.top + fLineHeight;
} /* XView::CharRect */

void XView::InvertSelection(bool hex)
{
	BRegion rgn;
	
	SetDrawingMode(B_OP_INVERT);

	GetSelection(rgn, hex);
	FillRegion(&rgn);
	
	rgn.MakeEmpty();
	GetSelection(rgn, !hex);
	MakeOutline(rgn);
	FillRegion(&rgn);

	SetDrawingMode(B_OP_COPY);
} /* XView::InvertSelection */

void XView::ChangeSelection(int newCaret)
{
	newCaret = min(fDataSize - 1, newCaret);
	newCaret = max(0, newCaret);
	if (newCaret == fCaret) return;

	BRegion oldSel, newSel, rgn;
	
	SetDrawingMode(B_OP_INVERT);

	GetSelection(oldSel, fEditHex);

	GetSelection(rgn, !fEditHex);
	MakeOutline(rgn);
	FillRegion(&rgn);
	
	fCaret = newCaret;
	fAnchor = min(fAnchor, fDataSize - 1);
	
	rgn.MakeEmpty();
	GetSelection(rgn, !fEditHex);
	MakeOutline(rgn);
	FillRegion(&rgn);

	GetSelection(newSel, fEditHex);

	rgn = newSel;
	rgn.Exclude(&oldSel);
	FillRegion(&rgn);
	
	rgn = oldSel;
	rgn.Exclude(&newSel);
	FillRegion(&rgn);
	
	SetDrawingMode(B_OP_COPY);

	fUpperNibble = false;
} /* XView::ChangeSelection */

void XView::ScrollToCaret()
{
	int curLine, caretLine, lpp;
	
	lpp = (int)floor(Bounds().Height() / fLineHeight);
	curLine = (int)ceil(fScrollBar->Value() / fLineHeight);
	caretLine = fCaret / 16;
	
	HideCaret();

	if (caretLine < curLine)
		fScrollBar->SetValue(caretLine * fLineHeight);
	else if (curLine + lpp <= caretLine)
		fScrollBar->SetValue((caretLine - (lpp>>1)) * fLineHeight);
	
	Window()->UpdateIfNeeded();
} /* XView::ScrollToCaret */

void XView::Select(int from, int to)
{
	InvertSelection(fEditHex);
	fAnchor = from;
	fCaret = to;
	InvertSelection(fEditHex);
	fUpperNibble = false;
} /* XView::Select */

void XView::SelectAll()
{
	Select(0, fDataSize - 1);
} /* XView::Select */

void XView::DoCopy()
{
	be_clipboard->Lock();
	be_clipboard->Clear();
	
	try
	{
		char *s;
		int begin, end, size;
		begin = min(fAnchor, fCaret);
		end = max(fAnchor, fCaret);
		size = end - begin + 1;
		
		if (fEditHex)
		{
			size *= 2;
			s = (char *)malloc(size);
			FailNil(s);
			unsigned char *dp = (unsigned char *)(fData + begin);
			
			for (int i = 0; i < size; i += 2)
			{
				s[i] = gHex[dp[i / 2] >> 4];
				s[i + 1] = gHex[dp[i / 2] & 0x0F];
			}
		}
		else
		{
			s = (char *)malloc(size);
			FailNil(s);
			memcpy(s, fData + begin, size);
		}
		
		be_clipboard->Data()->AddData("text/plain", B_MIME_TYPE, s, size);
		
		free(s);
		
		be_clipboard->Commit();
	}
	catch (HErr& e)
	{
		e.DoError();
	}
	be_clipboard->Unlock();
} /* XView::DoCopy */

void XView::DoPaste()
{
	be_clipboard->Lock();

	try
	{
		const char *s;
		ssize_t size;
		
		if (be_clipboard->Data()->FindData("text/plain", B_MIME_TYPE, (const void **)&s, &size) == B_NO_ERROR)
		{
			if (fEditHex)
			{
				bool valid = true;
	
				if (size & 1)
					size++;
				
				char *data = (char *)malloc(size / 2);
				FailNil(data);
				int i;
				
				for (i = 0; i < size && valid; i += 2)
				{
					if (isxdigit(s[i]) && isxdigit(s[i + 1]))
					{
						char c = toupper(s[i]);
						data[i / 2] = (c > '9' ? c - 'A' + 10 : c - '0') << 4;
						c = toupper(s[i + 1]);
						data[i / 2] |= c > '9' ? c - 'A' + 10 : c - '0';
					}
					else
						valid = false;
				}
				
				if (valid)
					size /= 2;
				else
				{
					beep();
					size = 0;
				}
			}

			if (size)
			{
				fAnchor = min(fAnchor, fCaret);
				fCaret = fAnchor + size;
				memcpy(fData + fAnchor, s, size);
				Draw(Bounds());
			}
		}

		SetDirty(true);
	}
	catch (HErr& e)
	{
		e.DoError();
	}
	
	be_clipboard->Unlock();
} /* XView::DoPaste */

void XView::SetDirty(bool dirty)
{
	fDirty = dirty;
//	if (fDirty)
//		fItem->SetDirty(true);
} /* XView::SetDirty */

bool XView::IsDirty()
{
	return fDirty;
} /* XView::IsDirty */

void XView::WindowActivated(bool active)
{
	fActive = active;

	if (active)
		ShowCaret();
	else
		HideCaret();
} /* XView::WindowActivated */

ptr_t XView::GetSelectionAddress()
{
	// We want the lowest address of the selection, so get
	// the minimum of fCaret and fAnchor
	
	return fAddr + min(fCaret, fAnchor);
} /* XView::GetSelectionAddress */

ptr_t XView::GetSelectionValue()
{
	// We want the lowest address of the selection, so get
	// the minimum of fCaret and fAnchor
	
	ptr_t value = 0;
	int readoffset = min(fCaret, fAnchor);
	
	// Now get the value at that address
	// (make sure we don't read beyond our buffer)
	int32 loopCount = min(4, fDataSize - readoffset);
	int32 shift = 0;
	for (int32 i = 0; i < loopCount; i++)
	{
		uint32 oneByte = fData[readoffset+i];
		value |= ((oneByte & 0xff) << shift);
		shift += 8;
	}
	
	return value;
} /* XView::GetSelectionValue */


void XView::RefreshView()
{
	// Read in the memory again, and force a redraw
	this->DoMemoryRead(fAddr);
	this->Invalidate();
} /* XView::RefreshView */

void XView::RefreshViewAt(ptr_t addr)
{
	// If the given address is already within our range, just move the caret
	// otherwise, read in the memory again (at given address) and force a 
	// redraw

	if (fAddr <= addr && fAddr+fDataSize > addr)
	{
		int startOffset = addr - fAddr;
		int endOffset = min(startOffset+3, fDataSize-1);
		this->Select(startOffset, endOffset);
		this->ScrollToCaret();
	}
	else 
	{
		this->InitializeMemoryBuffer(addr, 4);
		this->Invalidate();
		this->ScrollToCaret();
	}
} /* XView::RefreshViewAt */

void XView::DoMemoryRead(ptr_t addr)
{
	BAutolock l1(fTeam);
	DNub& nub = fTeam.GetNub();
	BAutolock l2(nub);

	nub.ReadData(addr, fData, fDataSize);
}

void XView::InitializeMemoryBuffer(ptr_t addr, size_t size)
{
	// Read the page that contains the address
	// If it fails, the page must not be mapped
	
	ptr_t baseAddr = addr & ~(B_PAGE_SIZE-1);
	int anchor = addr - baseAddr;

	this->DoMemoryRead(baseAddr);

	fAddr = baseAddr;
	fAnchor = anchor;
	int selLength = size == 0 ? 0 : size - 1;
	fCaret = fAnchor + selLength;
} /* XView::InitializeMemoryBuffer */
