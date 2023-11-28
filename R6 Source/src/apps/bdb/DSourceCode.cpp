/*	$Id: DSourceCode.cpp,v 1.12 1999/05/03 13:09:52 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/27/98 15:18:35
*/

#include "bdb.h"
#include "DSourceCode.h"
#include "HDefines.h"
#include "DMessages.h"
#include "DUtils.h"
#include "DFindDialog.h"

#include "HButtonBar.h"

#include <ScrollBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Beep.h>
#include <Clipboard.h>

#include <ctype.h>

uchar *DSourceCode::ec = NULL;
ushort *DSourceCode::accept, *DSourceCode::base, *DSourceCode::nxt, *DSourceCode::chk;
string DSourceCode::sfSearchText;
bool DSourceCode::sfIgnoreCase;

const rgb_color
	kCommentColor = { 0xa1, 0x64, 0x0e, 0xff},
	kTextColor = { 0x00, 0x00, 0x00, 0xff },
	kStringColor = { 0x3f, 0x48, 0x84, 0xff },
	kCharConstColor = { 0x85, 0x19, 0x19, 0xff },
	kKeyWordColor = { 0x39, 0x74, 0x79, 0xff },
	kSelectionColor = { 0xff, 0xec, 0x7c, 0xff };

DSourceCode::DSourceCode(BRect frame, const char *name)
	: BView(frame, name, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_NAVIGABLE)
{
	if (ec == NULL)
	{
		FailNilRes(ec = (uchar *)HResources::GetNamedResource('sctb', "ec"));
		FailNilRes(accept = (ushort *)HResources::GetNamedResource('sctb', "accept"));
		FailNilRes(base = (ushort *)HResources::GetNamedResource('sctb', "base"));
		FailNilRes(nxt = (ushort *)HResources::GetNamedResource('sctb', "nxt"));
		FailNilRes(chk = (ushort *)HResources::GetNamedResource('sctb', "chk"));
	}
	
	font_family ff;
	font_style fs;
	be_fixed_font->GetFamilyAndStyle(&ff, &fs);

	fFont.SetFamilyAndStyle(gPrefs->GetPrefString("font family", ff), gPrefs->GetPrefString("font style", fs));
	fFont.SetSize(gPrefs->GetPrefInt("font size", be_fixed_font->Size()));
	SetFont(&fFont);

	fAnchor = fCaret = 0;
	fText = NULL;
} /* DSourceCode::DSourceCode */

void DSourceCode::AttachedToWindow()
{
	SetFont(&fFont);
	
	fFont.GetHeight(&fFH);
	fLineHeight = ceil(fFH.ascent + fFH.descent + fFH.leading);
	
	fTabWidth = ceil(fFont.StringWidth(" ") * 4);

	HButtonBar *bar = dynamic_cast<HButtonBar*>(Window()->FindView("ButtonBar"));
	ASSERT_OR_THROW (bar);
	bar->SetTarget(this);
	
	ResizeTo(1e6, Bounds().Height());
} /* DSourceCode::AttachedToWindow */

void DSourceCode::Draw(BRect update)
{
	FillRect(update, B_SOLID_LOW);
	
	if (fLineBreaks.size() > 0)
	{
		int x1, x2, x;
		
		x1 = std::max((int)floor(update.top / fLineHeight) - 1, 0);
		x2 = (int)ceil(update.bottom / fLineHeight) + 1;
		
		for (x = x1; x <= x2; x++)
			DrawLine(x);
	}
} /* DSourceCode::Draw */

#define SETCOLOR(start,color,ix) 																\
do { 																									\
	if (starts &&																					\
		(ix == 0 || *(int *)&colors[ix - 1] != *(int *)&color) &&							\
		ix < 99)																						\
	{																									\
		if (ix && start == starts[ix - 1])														\
			ix--;																						\
		colors[ix] = color;																			\
		starts[ix] = start;																			\
		ix++;																							\
	}																									\
}																										\
while (false)

#define GETCHAR			(c = (i++ < size) ? text[i - 1] : 0)

enum {
	START, IDENT, OTHER, COMMENT, LCOMMENT, STRING,
	CHAR_CONST, LEAVE, PRAGMA1, PRAGMA2,
	INCL1, INCL2, INCL3
};

void DSourceCode::ColorLine(const char *text, int size, int&state, int *starts, rgb_color *colors)
{
	int i = 0, s = 0, kws = 0, cc_cnt = 0, esc = 0, ci = 0;
	char c;
	bool leave = false;
	
	if (state == COMMENT || state == LCOMMENT)
		SETCOLOR(0, kCommentColor, ci);
	else
		SETCOLOR(0, kTextColor, ci);
	
	if (size <= 0)
		return;
	
	while (!leave)
	{
		GETCHAR;
		
		switch (state) {
			case START:
				if (c == '#')
				{
					kws = Move(c, 1);
					state = PRAGMA1;
				}
				else if (isalpha(c) || c == '_')
				{
					kws = Move(c, 1);
					state = IDENT;
				}
				else if (c == '/' && text[i] == '*')
				{
					i++;
					state = COMMENT;
				}
				else if (c == '/' && text[i] == '/')
				{
					i++;
					state = LCOMMENT;
				}
				else if (c == '"')
					state = STRING;
				else if (c == '\'')
				{
					state = CHAR_CONST;
					cc_cnt = 0;
				}
				else if (c == '\n' || c == 0)
					leave = true;
					
				if (leave || (state != START && s < i))
				{
					SETCOLOR(s, kTextColor, ci);
					s = i - 1;
				}
				break;
			
			case COMMENT:
				if ((s == 0 || i > s + 1) && c == '*' && text[i] == '/')
				{
					SETCOLOR(std::max(s - 1, 0), kCommentColor, ci);
					s = i + 1;
					state = START;
				}
				else if (c == 0 || c == '\n')
				{
					SETCOLOR(std::max(s - 1, 0), kCommentColor, ci);
					leave = true;
				}
				break;

			case LCOMMENT:
				SETCOLOR(std::max(s - 1, 0), kCommentColor, ci);
				leave = true;
				if (text[size - 1] == '\n')
					state = START;
				break;
			
			case IDENT:
				if (!isalnum(c) && c != '_')
				{
					int kwc;

					if (i > s + 1 && (kwc = IsKeyWord(kws)) != 0)
					{
						SETCOLOR(s, kKeyWordColor, ci);
					}
					else
					{
						SETCOLOR(s, kTextColor, ci);
					}
					
					s = --i;
					state = START;
				}
				else if (kws)
					kws = Move((int)(unsigned char)c, kws);
				break;
			
			case PRAGMA1:
				if (c == ' ' || c == '\t')
					;
				else if (islower(c))
				{
					kws = Move((int)(unsigned char)c, kws);
					state = PRAGMA2;
				}
				else
				{
					SETCOLOR(s, kTextColor, ci);
					s = --i;
					state = START;
				}	
				break;
			
			case PRAGMA2:
				if (!islower(c))
				{
					int kwc;

					if (i > s + 2 && (kwc = IsKeyWord(kws)) != 0)
					{
						SETCOLOR(s, kKeyWordColor, ci);
					}
					else
					{
						SETCOLOR(s, kTextColor, ci);
					}
					
					state = strncmp(text+i-8, "include", 7) ? START : INCL1;
					s = --i;
				}
				else if (kws)
					kws = Move((int)(unsigned char)c, kws);
				break;
			
			case INCL1:
				if (c == '"')
					state = INCL2;
				else if (c == '<')
					state = INCL3;
				else if (c != ' ' && c != '\t')
				{
					state = START;
					i--;
				}
				break;
			
			case INCL2:
				if (c == '"')
				{
					SETCOLOR(s, kStringColor, ci);
					s = i;
					state = START;
				}
				else if (c == '\n' || c == 0)
				{
					SETCOLOR(s, kTextColor, ci);
					leave = true;
					state = START;
				}	
				break;
			
			case INCL3:
				if (c == '>')
				{
					SETCOLOR(s, kStringColor, ci);
					s = i;
					state = START;
				}
				else if (c == '\n' || c == 0)
				{
					SETCOLOR(s, kTextColor, ci);
					leave = true;
					state = START;
				}	
				break;
			
			case STRING:
				if (c == '"' && !esc)
				{
					SETCOLOR(s, kStringColor, ci);
					s = i;
					state = START;
				}
				else if (c == '\n' || c == 0)
				{
					if (text[i - 2] == '\\' && text[i - 3] != '\\')
					{
						SETCOLOR(s, kStringColor, ci);
					}
					else
					{
						SETCOLOR(s, kTextColor, ci);
						state = START;
					}
					
					s = size;
					leave = true;
				}
				else
					esc = !esc && (c == '\\');
				break;
			
			case CHAR_CONST:
				if (c == '\t' || c == '\n' || c == 0)	// don't like this
				{
					SETCOLOR(s, kTextColor, ci);
					s = i;
					state = START;
				}
				else if (c == '\'' && !esc)
				{
					if (cc_cnt != 1 && cc_cnt != 2 && cc_cnt != 4)
					{
						SETCOLOR(s, kTextColor, ci);
						s = --i;
						state = START;
					}
					else
					{
						SETCOLOR(s, kCharConstColor, ci);
						s = i;
						state = START;
					}
				}
				else
				{
					if (!esc) cc_cnt++;
					esc = !esc && (c == '\\');
				}
				break;
		}
	}
} /* DSourceCode::ColorLine */

int DSourceCode::Move(int ch, int state)
{
	int c = ec[ch];
	
	if (c && chk[base[state] + c] == state)
		return nxt[base[state] + c];
	else
		return 0;
} /* DSourceCode::Move */

int DSourceCode::IsKeyWord(int state)
{
	if (state)
		return accept[state];
	else
		return 0;
} /* DSourceCode::IsKeyWord */

void DSourceCode::DrawLine(int lineNr)
{
	float x = 0, y = lineNr * fLineHeight;
	BRect E;
	
	float v = Parent()->ScrollBar(B_HORIZONTAL)->Value();
	E.Set(v, y + 1, v + Bounds().Width(), y + fLineHeight);
	y = E.bottom - fFH.descent;

	SetLowColor(kWhite);
	FillRect(E, B_SOLID_LOW);
	
	if (lineNr < 0 || lineNr >= (int)fLineBreaks.size())
		return;
	
	fDirty[lineNr] = false;
	
	int s, e, l;
	s = fLineBreaks[lineNr];
	e = std::min(fTextSize, fLineBreaks[lineNr + 1]);
	l = e - s - 1;
	
	const char *b = fText + s;
	int starts[100];
	rgb_color colors[100];
	
	if (l)
	{
		memset(starts, 0, sizeof(int) * 100);
		int start = fLineStates[lineNr];
		ColorLine(fText + s, l, start, starts, colors);
	}

	int a, c;
	a = std::min(fAnchor, fCaret);
	c = std::max(fAnchor, fCaret);

	if (a < c && a < e && c >= s && IsFocus())
	{
		BRect r(E);

		if (a > s)
			r.left = Offset2Position(a).x - v;
		else
			r.left = 3;

		if (c < e || (lineNr == LineCount() - 1 && c == fTextSize))
			r.right = Offset2Position(c).x - v;

		SetLowColor(kSelectionColor);
		FillRect(r, B_SOLID_LOW);
	}

	int i, j, ci = 0;
	
	bool draw = false;

	i = 0;
	j = 0;
	x = 0;

	SetHighColor(colors[0]);
	
	while (i < l)
	{
		if (b[i] == '\t' || i == a - s || i == c - s || (b[i] >= 0 && iscntrl(b[i])))
			draw = true;
		
		if (starts[ci] == i)
			draw = true;
		
		if (draw)
		{
			if (b[i] == '\t')
			{
				if (i - j > 0)
				{
					DrawString(b + j, i - j, BPoint(x + 3, y));
					x += StringWidth(b + j, i - j);
				}

				int t = (int)floor(x / fTabWidth) + 1;
				x = (rint(t * fTabWidth) > rint(x) ? t * fTabWidth : (t + 1) * fTabWidth);
				j = i + 1;
			}
			else if (b[i] >= 0 && iscntrl(b[i]))
			{
				if (i - j > 0)
				{
					DrawString(b + j, i - j, BPoint(x + 3, y));
					x += StringWidth(b + j, i - j);
				}
				
				DrawString("¿", BPoint(x + 3, y));
				
				x += StringWidth("¿", strlen("¿"));
				j = i + 1;
			}
			else if (i > j)
			{
				DrawString(b + j, i - j, BPoint(x + 3, y));
				x += StringWidth(b + j, i - j);
				j = i;
			}

			if (a != c && IsFocus())
			{
				if (i == a - s)
					SetLowColor(kSelectionColor);
				else if (i == c - s)
					SetLowColor(kWhite);
			}
			
			draw = false;

			if (starts[ci] == i)
			{
				SetHighColor(colors[ci]);
				ci++;
			}
		}

		if (b[i] == 0)
			break;

		i++;
	}

	if (i > j)
		DrawString(b + j, i - j, BPoint(x + 3, y));
} /* DSourceCode::DrawLine */

void DSourceCode::NewFile(BPositionIO& file)
{
	fTextSize = file.Seek(0, SEEK_END);
	file.Seek(0, SEEK_SET);

	if (fText) free(fText);
	fText = (char *)malloc(fTextSize + 1);
	FailNil(fText);
	fText[fTextSize] = 0;
	fAnchor = fCaret = 0;
	
	CheckedRead(file, fText, fTextSize);
	WrapText();
} /* DSourceCode::NewFile */

void DSourceCode::Clear()
{
	free(fText);
	fText = NULL;
	fLineBreaks.clear();
	fLineStates.clear();
	fDirty.clear();
	fAnchor = fCaret = 0;
} /* DSourceCode::Clear */

void DSourceCode::WrapText()
{
	fLineBreaks.clear();
	fLineStates.clear();
	fDirty.clear();
	
	char *p = fText, *s = fText;
	int state = 0;

	fLineBreaks.push_back(0);
	fLineStates.push_back(0);
	
	do
	{
		if (*p++ == '\n')
		{
			ColorLine(s, p - s, state, NULL, NULL);
			fLineBreaks.push_back(p - fText);
			fLineStates.push_back(state);
			s = p;
		}
	}
	while (*p);
	
	ColorLine(s, p - s, state, NULL, NULL);
	fLineBreaks.push_back(p - fText);
	fLineStates.push_back(state);
	
	fDirty.insert(fDirty.begin(), fLineBreaks.size(), false);
	
	ResizeTo(Frame().Width(), fLineBreaks.size() * fLineHeight);
} /* DSourceCode::WrapText */

void DSourceCode::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case B_COPY:
			if (fCaret != fAnchor)
				Copy();
			break;
		
		case kMsgFuncPopup:
		{
			if (fText)
			{
				BPoint p;
				FailOSErr(msg->FindPoint("where", &p));
				ShowFunctionMenu(p);
			}
			else
				beep();

			HButtonBar *bar = dynamic_cast<HButtonBar*>(Window()->FindView("ButtonBar"));
			ASSERT_OR_THROW (bar);
			bar->SetDown(kMsgFuncPopup, false);
			break;
		}
		
		case kMsgFindAgain:
			if (sfSearchText.length())
			{
				FindAgain(false);
				break;
			}
		
		case kMsgFind:
			try
			{
				DFindDialog *dlog = DialogCreator<DFindDialog>::CreateDialog(Window());
				BAutolock lock(dlog);
				dlog->SetTarget(sfSearchText.length() ? sfSearchText.c_str() : gPrefs->GetPrefString("find: text", ""), this);
			}
			catch (HErr& e)
			{
				e.DoError();
			}
			break;
		
		case kMsgDoFind:
		{
			const char *what;
			bool fromTop, ignoreCase;
			
			FailOSErr(msg->FindString("find", &what));
			
			sfSearchText.assign(what);
			
			FailOSErr(msg->FindBool("icase", &ignoreCase));
			FailOSErr(msg->FindBool("startAtTop", &fromTop));
			
			Find(ignoreCase, fromTop);
			break;
		}
		
		case kMsgFindSelection:
			if (fCaret != fAnchor)
			{
				sfSearchText.assign(fText + std::min(fAnchor, fCaret), abs(fAnchor - fCaret));
				Find(sfIgnoreCase, false);
			}
			break;
		
		case kMsgFindAgainBackward:
			FindAgain(true);
			break;
		
		case kMsgFindSelectionBackward:
			if (fCaret != fAnchor)
			{
				sfSearchText.assign(fText + std::min(fAnchor, fCaret), abs(fAnchor - fCaret));
				Find(sfIgnoreCase, false);
			}
			break;
		
		case kMsgEnterSearchString:
			sfSearchText.assign(fText + std::min(fAnchor, fCaret), abs(fAnchor - fCaret));
			break;
		
		case kMsgJumpToFunc:
			JumpToFunction(msg);
			break;
		
		default:
			BView::MessageReceived(msg);
			break;
	}
} /* DSourceCode::MessageReceived */

static int Compare(const void *a, const void *b)
{
	return strcmp(
		(*static_cast<BMenuItem* const *>(a))->Label(),
		(*static_cast<BMenuItem* const *>(b))->Label());
}

void DSourceCode::ShowFunctionMenu(BPoint where)
{
	BPopUpMenu popup("Funcs");
	popup.SetFont(be_plain_font);

	BList items;
	ScanForFunctions(items);
	
	if (modifiers() & B_OPTION_KEY)
		items.SortItems(Compare);
	
	popup.AddList(&items, 0);
	
	if (popup.CountItems() == 0)
		popup.AddItem(new BMenuItem("Nothing Found", NULL));

	BRect r;
	
	r.Set(where.x - 4, where.y - 20, where.x + 24, where.y + 4);

	popup.SetTargetForItems(this);
	popup.Go(where, true, true, r);
} /* DSourceCode::ShowFunctionMenu */

void DSourceCode::JumpToFunction(BMessage *msg)
{
	int32 offset;
	FailOSErr(msg->FindInt32("offset", &offset));
	
	int line = Offset2Line(offset);
	Parent()->ScrollBar(B_VERTICAL)->SetValue(line * fLineHeight);
} /* DSourceCode::JumpToFunction */

int DSourceCode::Offset2Line(int offset)
{
	int L = 0, R = fLineBreaks.size() - 1, line = 0;
	
	while (L <= R)
	{
		line = (L + R) / 2;

		if (fLineBreaks[line] < offset)
			L = line + 1;
		else if (fLineBreaks[line] > offset)
			R = line - 1;
		else
			break;
	}
	
	return fLineBreaks[line] <= offset ? line : line - 1;
} /* DSourceCode::Offset2Line */

void DSourceCode::MouseDown(BPoint where)
{
	if (!fText)
		return;

#if B_BEOS_VERSION_DANO
	// in R5.1 (exp), MakeFocus will attempt to move the view in order to
	// make as much as possible of it visible at a time.  We don't want this,
	// because we're saving space for the left-edge breakpoint edit region,
	// so we call the explicit exp "set focus but don't relocate" method.
	MakeFocusNoScroll(true);
#else
	MakeFocus(true);
#endif

	try
	{
		uint32 modifiers, btns;
		
		FailOSErr(Looper()->CurrentMessage()->FindInt32("modifiers", (int32*)&modifiers));
		FailOSErr(Looper()->CurrentMessage()->FindInt32("buttons", (int32*)&btns));
		
		int curOffset = Position2Offset(where);
		int anchor1, anchor2;
		anchor1 = std::min(fAnchor, fCaret);
		anchor2 = std::max(fAnchor, fCaret);
		
		if (curOffset > anchor1 && curOffset < anchor2 &&
			(btns & (B_SECONDARY_MOUSE_BUTTON | B_TERTIARY_MOUSE_BUTTON) ||
			 WaitMouseMoved(where)))
		{
			int len = anchor2 - anchor1;
			char *s = (char *)malloc(len + 1);
			FailNil(s);
			memcpy(s, fText + anchor1, len);
			s[len] = 0;
			
			BMessage drag(B_SIMPLE_DATA);
			FailOSErr(drag.AddData("text/plain", B_MIME_DATA, s, len));
			FailOSErr(drag.AddString("be:clip_name", "Text Snippet from bdb"));
			
			BRegion rgn;
			Selection2Region(rgn);
			DragMessage(&drag, rgn.Frame());

			free(s);
			return;
		}

		if (modifiers & B_SHIFT_KEY)
			ChangeSelection(fAnchor, curOffset);
		else
		{
			anchor1 = anchor2 = curOffset;
			ChangeSelection(anchor1, anchor2);
		}

//		float v = -1;
		BPoint cur;
		
		GetMouse(&cur, &btns);
		while (btns)
		{
			if (where != cur
//			 || v != sBar->Value()
			 )
			{
				curOffset = Position2Offset(cur);
				
				if (curOffset < anchor1)
					ChangeSelection(std::max(anchor2, curOffset), std::min(anchor1, curOffset));
				else if (curOffset > anchor2)
					ChangeSelection(std::min(anchor1, curOffset), std::max(anchor2, curOffset));
				
//				v = sBar->Value();
//				ScrollToCaret();
				where = cur;
			}
			
			snooze(20000);
			GetMouse(&cur, &btns);
		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}
} // DSourceCode::MouseDown

BPoint DSourceCode::Offset2Position(int offset)
{
	int line = Offset2Line(offset);
	
	float x = 0;
	int o = offset - fLineBreaks[line];
	int s = fLineBreaks[line];
	
	while (o > 0)
	{
		int cl;
		if (fText[s] == '\t')
		{
			cl = 1;
			int t = (int)floor(x / fTabWidth) + 1;
			x = (rint(t * fTabWidth) > rint(x) ? t * fTabWidth : (t + 1) * fTabWidth);
		}
		else
		{
			cl = mcharlen(fText + s);
			x += StringWidth(fText + s, cl);
		}
		
		s += cl;
		o -= cl;
	}
	
	return BPoint(x + 2, line * fLineHeight);
} /* DSourceCode::Offset2Position */

int DSourceCode::LinePosition2Offset(int line, float position)
{
	position -= 3;

	if (position < 0) return 0;
	
	int l = fLineBreaks[line];
	int m = (line < LineCount() - 1) ? fLineBreaks[line + 1] - 1 : fTextSize;
	
	float x = 0, lx = 0;
	int o = 0;
	int s = l, cl = 0;

	while (o + l < m && x < position)
	{
		if (fText[s] == '\t')
		{
			cl = 1;
			int t = (int)floor(x / fTabWidth) + 1;
			lx = t * fTabWidth - x;
			x = t * fTabWidth;
		}
		else
		{
			cl = mcharlen(fText + s);
			lx = StringWidth(fText + s, cl);

			x += lx;
		}
		
		s += cl;
		o += cl;
	}
	
	if (x > position)
	{
		float dx = x - position;
		if (dx > lx / 2)
			o -= cl;
	}

	return o;
} /* DSourceCode::LinePosition2Offset */

int DSourceCode::Position2Offset(BPoint where)
{
	int line;
	
	line = std::max(0, (int)floor(where.y / fLineHeight));
	
	if (line > LineCount() - 1)
		line = LineCount() - 1;

	return fLineBreaks[line] + LinePosition2Offset(line, where.x);
} /* DSourceCode::Position2Offset */

void DSourceCode::ChangeSelection(int newAnchor, int newCaret)
{
	int na, nc;
	int oa, oc;
	
	na = std::min(newAnchor, newCaret);
	nc = std::max(newAnchor, newCaret);
	
	oa = std::min(fAnchor, fCaret);
	oc = std::max(fAnchor, fCaret);
	
	int ls, le;
	
	if (nc < oa || na > oc)
	{
		ls = Offset2Line(na);
		le = Offset2Line(nc);
		
		TouchLines(ls, le);
	
		ls = Offset2Line(oa);
		le = Offset2Line(oc);
		
		TouchLines(ls, le);
	}
	else
	{
		ls = Offset2Line(std::min(oa, na));
		le = Offset2Line(std::max(oa, na));
		
		TouchLines(ls, le);
	
		ls = Offset2Line(std::min(oc, nc));
		le = Offset2Line(std::max(oc, nc));
		
		TouchLines(ls, le);
	}

	fCaret = newCaret;
	fAnchor = newAnchor;
	
	RedrawDirtyLines();
} // DSourceCode::ChangeSelection

void DSourceCode::TouchLines(int from, int to)
{
	for (int i = from; i <= to; i++)
		fDirty[i] = true;
} // DSourceCode::TouchLines

void DSourceCode::Selection2Region(BRegion& rgn)
{
	rgn.MakeEmpty();
	BRect clip(Bounds());
	
	if (fAnchor == fCaret)
		return;

	int first, last, firstLine, lastLine;
	first = std::min(fAnchor, fCaret);
	last = std::max(fAnchor, fCaret);
	
	firstLine = Offset2Line(first);
	lastLine = Offset2Line(last);
	
	BPoint p1, p2;
	
	p1 = Offset2Position(first);
	p1.x = std::max(3, (int)rint(p1.x));
	p1.y = rint(p1.y);
	p2 = Offset2Position(last);
	p2.x = rint(p2.x);
	p2.y = rint(p2.y);

	if (p1.y == p2.y)
	{
		BRect r(p1.x, p1.y, p2.x, p1.y + fLineHeight);
		if (clip.Intersects(r))
			rgn.Include(r & clip);
	}
	else
	{
		BRect r, b(Bounds());
		
		int lines = Offset2Line(last) - Offset2Line(first) - 1;
		
		r.Set(p1.x, p1.y, b.right, p1.y + fLineHeight);
		if (clip.Intersects(r))
			rgn.Include(r & clip);
		
		r.left = 3;
		r.OffsetBy(0, fLineHeight);
		while (lines--)
		{
			if (clip.Intersects(r))
				rgn.Include(r & clip);
			r.OffsetBy(0, fLineHeight);
		}
		
		r.right = p2.x;
		if (clip.Intersects(r))
			rgn.Include(r & clip);
	}
} /* DSourceCode::Selection2Region */

void DSourceCode::RedrawDirtyLines()
{
	if (fLineBreaks.size() > 0)
	{
		int x1, x2, x;
		BRect b(Bounds());
		
		x1 = std::max((int)floor(b.top / fLineHeight) - 1, 0);
		x2 = (int)ceil(b.bottom / fLineHeight) + 1;
		
		for (x = x1; x <= x2; x++)
		{
			if (fDirty[x])
			{
				DrawLine(x);
			}
		}
	}
} // DSourceCode::RedrawDirtyLines

void DSourceCode::MakeFocus(bool focus)
{
	BView::MakeFocus(focus);
	if (fCaret != fAnchor)
	{
		TouchLines(Offset2Line(std::min(fAnchor, fCaret)), Offset2Line(std::max(fAnchor, fCaret)));
		RedrawDirtyLines();
	}
} // DSourceCode::MakeFocus

void DSourceCode::Copy()
{
	char *s;
	int size = abs(fCaret - fAnchor);

	try
	{
		s = (char *)malloc(size);
		FailNil(s);
		
		memcpy(s, fText + std::min(fCaret, fAnchor), size);
		
		be_clipboard->Lock();
		
		be_clipboard->Clear();
		be_clipboard->Data()->AddData("text/plain", B_MIME_DATA, s, size);
		be_clipboard->Commit();
		be_clipboard->Unlock();
		
		free(s);
	}
	catch(HErr& e)
	{
		e.DoError();
	}
} // DSourceCode::Copy

bool DSourceCode::WaitMouseMoved(BPoint where)
{
	bigtime_t longEnough = system_time() + 250000;
	
	do
	{
		BPoint p;
		unsigned long btns;
		
		GetMouse(&p, &btns);
		
		if (!btns)
			return false;
		
		if (fabs(where.x - p.x) > 2 || fabs(where.y - p.y) > 2)
			return true;
	}
	while (system_time() < longEnough);
	
	return true;
} // DSourceCode::WaitMouseMoved

void initskip(const unsigned char *p, int skip[], bool ignoreCase)
{
	int M = strlen((char *)p), i;
	
	for (i = 0; i < 255; i++)
		skip[i] = M;
	
	if (ignoreCase)
	{
		for (i = 0; i < M; i++)
			skip[toupper(p[i])] = M - i - 1;
	}
	else
	{
		for (i = 0; i < M; i++)
			skip[p[i]] = M - i - 1;
	}
} /* initskip */

int mismatchsearch(const unsigned char *p, const unsigned char *a, int N, int skip[], bool ignoreCase)
{
	ASSERT_OR_THROW (p);
	ASSERT_OR_THROW (a);
	ASSERT_OR_THROW (skip);
	int i, j, t, M = strlen((char *)p);

	if (ignoreCase)
	{
		for (i = M - 1, j = M - 1; j >= 0; i--, j--)
		{
			while (toupper(a[i]) != toupper(p[j]))
			{
				t = skip[toupper(a[i])];
				i += (M - j > t) ? M - j : t;
				if (i >= N)
					return N;
				j = M - 1;
			}
		}
	}
	else
	{
		for (i = M - 1, j = M - 1; j >= 0; i--, j--)
		{
			while (a[i] != p[j])
			{
				t = skip[a[i]];
				i += (M - j > t) ? M - j : t;
				if (i >= N)
					return N;
				j = M - 1;
			}
		}
	}
	return i;
} /* mismatchsearch */

void initskip_b(const unsigned char *p, int skip[], bool ignoreCase)
{
	int M = strlen((char *)p), i;
	
	for (i = 0; i < 255; i++)
		skip[i] = M;
	
	if (ignoreCase)
	{
		for (i = M - 1; i >= 0; i--)
			skip[toupper(p[i])] = i;
	}
	else
	{
		for (i = M - 1; i >= 0; i--)
			skip[p[i]] = i;
	}
} /* initskip_b */

int mismatchsearch_b(const unsigned char *p, const unsigned char *a, int N, int skip[], bool ignoreCase)
{
	ASSERT_OR_THROW (p);
	ASSERT_OR_THROW (a);
	ASSERT_OR_THROW (skip);
	int i, j, t, M = strlen((char *)p);

	if (ignoreCase)
	{
		for (i = N - M, j = 0; j < M; i++, j++)
		{
			while (toupper(a[i]) != toupper(p[j]))
			{
				t = skip[toupper(a[i])];
				i -= (j + 1 > t) ? j + 1 : t;
				if (i < 0)
					return -1;
				j = 0;
			}
		}
	}
	else
	{
		for (i = N - M, j = 0; j < M; i++, j++)
		{
			while (a[i] != p[j])
			{
				t = skip[a[i]];
				i -= (j + 1 > t) ? j + 1 : t;
				if (i < 0)
					return -1;
				j = 0;
			}
		}
	}
	return i - M;
} /* mismatchsearch_b */

int find(const char *what, int offset, const char *buf, int bufSize, bool ignoreCase, bool backwards)
{
	int skip[256];

	if (backwards)
	{
		initskip_b((unsigned char *)what, skip, ignoreCase);
		offset = mismatchsearch_b((unsigned char *)what, (unsigned char *)buf,
					offset + strlen(what) - 1, skip, ignoreCase);
	}
	else
	{
		initskip((unsigned char *)what, skip, ignoreCase);
		offset += mismatchsearch((unsigned char *)what, (unsigned char *)buf + offset,
			bufSize - offset, skip, ignoreCase) + 1;
	}

	return offset;
} /* Find */

void DSourceCode::Find(bool ignoreCase, bool fromTop, bool backwards)
{
	bool startAtTop;
	sfIgnoreCase = ignoreCase;
	startAtTop = fromTop;
	
	int offset = startAtTop ? 0 : std::max(fCaret, fAnchor);
	offset = find(sfSearchText.c_str(), offset, fText, fTextSize, sfIgnoreCase, backwards);

	MakeFocus(true);

	if (offset >= 0 && offset < fTextSize)
		Select(offset, offset + sfSearchText.length());
	else
		beep();
} // DSourceCode::Find

void DSourceCode::FindAgain(bool backwards)
{
	int offset = backwards ? std::min(fCaret, fAnchor) : std::max(fCaret, fAnchor);
	offset = find(sfSearchText.c_str(), offset, fText, fTextSize, sfIgnoreCase, backwards);

	MakeFocus(true);

	if (offset >= 0 && offset < fTextSize)
		Select(offset, offset + sfSearchText.length());
	else
		beep();
} // DSourceCode::FindAgain

void DSourceCode::Select(int from, int to)
{
	ChangeSelection(from, to);

	int line = Offset2Line(fCaret);
	float y = (line - 1) * fLineHeight;
	BRect b(Parent()->Bounds());

	if (y < b.top || y + fLineHeight > b.bottom)
	{
		int tl = (int)(line - (b.Height() / fLineHeight) / 2);
		Parent()->ScrollBar(B_VERTICAL)->SetValue(tl * fLineHeight);
	}
} // DSourceCode::Select
