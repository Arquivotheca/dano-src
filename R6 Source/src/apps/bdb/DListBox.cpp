/*	$Id: DListBox.cpp,v 1.12 1999/05/03 13:09:51 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

#include "bdb.h"
#include "DListBox.h"
#include "DUtils.h"
#include "DMessages.h"

#include <TextView.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <Window.h>

#include <ctype.h>
#include <string>
#include <algorithm>

using std::string;
using std::vector;

const unsigned long
	msg_AcceptEntry = 'Acpt',
	msg_CancelEntry = 'Cncl';

const float
	kDisclosureTriangleWidth = 14;

class DEditBox : public BTextView
{
  public:
	DEditBox(BRect frame, const char *name, DListView *owner);
	
	virtual void KeyDown(const char *bytes, int32 numBytes);

  private:
	DListView *fOwner;
};

class DListView : public BOutlineListView
{
  public:
	DListView(DListBox *box, BRect frame, const char *name,
							list_view_type type = B_SINGLE_SELECTION_LIST,
							uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS 
								| B_NAVIGABLE);
	
	virtual void MouseDown(BPoint where);
	virtual void MessageReceived(BMessage *msg);
	virtual void WindowActivated(bool active);
	
	virtual void KeyDown(const char *bytes, int32 numBytes);
	virtual void Pulse();
	
	void SetDeleteMessage(long msg)	{ fDeleteMsg = msg; }
	void SetToggleMessage(long msg)	{ fToggleMsg = msg; }
	
	void AcceptEntry();
	void CancelEntry();

  protected:
	virtual void ExpandOrCollapse(BListItem *item, bool expand);

  private:
  	
  	void HandleTab(bool backward);
  	
	int fLastItem, fEditColumn;
	long fDeleteMsg, fToggleMsg;
	BTextView *fEdit;
	BScrollView *fEditScroller;
	DListBox *fBox;
	
	bigtime_t fLastKeyDown;
	string fTyped;
};

//#pragma mark -

DEditBox::DEditBox(BRect frame, const char *name, DListView *owner)
	: BTextView(frame, name, frame, 0, B_WILL_DRAW), fOwner(owner)
{
	frame.OffsetTo(0, 0);
	frame.right = 1000;
	SetTextRect(frame);
	SetWordWrap(false);
	SetFontAndColor(be_fixed_font);
} // DEditBox::DEditBox

void DEditBox::KeyDown(const char *bytes, int32 numBytes)
{
	if (*bytes == B_RETURN || *bytes == B_TAB)
		BMessenger(fOwner).SendMessage(msg_AcceptEntry);
	else if (*bytes == B_ESCAPE)
		BMessenger(fOwner).SendMessage(msg_CancelEntry);
	else
		BTextView::KeyDown(bytes, numBytes);
} // DEditBox::AllowChar

//#pragma mark -

DListView::DListView(DListBox *box, BRect frame, const char *name, list_view_type type,
	uint32 resizeMask, uint32 flags)
	: BOutlineListView(frame, name, type, resizeMask, flags | B_PULSE_NEEDED)
{
	fBox = box;
	fLastItem = -1;
	fEdit = NULL;
	fDeleteMsg = 0;
} // DListView::DListView

void DListView::MouseDown(BPoint where)
{
	int32 clicks, buttons;
	
	if (fEdit)
		CancelEntry();

	BMessage* msg = Window()->CurrentMessage();
	msg->FindInt32("buttons", &buttons);
	do
	{
		int ix = IndexOf(where);
		DListItem *item = dynamic_cast<DListItem*>(ItemAt(ix));

		if (msg->FindInt32("clicks", &clicks) == B_OK &&
			((buttons & ~0x01) == 0) &&		// only button 1 counts for double-clicks!
			clicks == 2 &&
			ix == fLastItem &&
			item)
		{
			BRect f(ItemFrame(fLastItem)), r = f;
			const char *s;
			
			vector<DColumn>& cols = fBox->fColumns;
			vector<DColumn>::iterator i;
			
			r.left += kDisclosureTriangleWidth;
			
			for (i = cols.begin(); i != cols.end(); i++)
			{
				if (i + 1 == cols.end())
					r.right = f.right;
				else
					r.right = r.left + (*i).fWidth;

				if (r.Contains(where))
					break;
				r.left = r.right;
			}
			
			if (i == cols.end())
				break;
			
			fEditColumn = i - cols.begin();
			s = item->GetColumnText(fEditColumn);
			if (s == NULL)
			{
				if (item->IsExpanded())
					Collapse(item);
				else
					Expand(item);
				break;
			}
			
			fEdit = new DEditBox(r, "edit", this);
			AddChild(fEditScroller = new BScrollView("edit scroller", fEdit));
			fEditScroller->SetBorder(B_PLAIN_BORDER);

			fEdit->SetText(s);
			fEdit->Select(0, strlen(s));
			
			fEdit->MakeFocus();
			return;
		}

		if (item)
		{
			fLastItem = ix;
			Select(fLastItem);

			BRect f(ItemFrame(fLastItem));
			f.left += 11;
			if (item->ClickItem(this, f, where))
				return;
		}
	}
	while (false);

	BOutlineListView::MouseDown(where);

	fLastItem = CurrentSelection();

	// if this was a secondary mouse click, bring up the data-format menu
	// now that the selection has been properly updated
	if (buttons & 0x02)
	{
		BMessage menuMessage(kMsgDoFormatPopup);
		menuMessage.AddPoint("where", ConvertToScreen(where));
		Window()->PostMessage(&menuMessage);
	}
} // DListView::MouseDown

void DListView::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case msg_AcceptEntry:
			AcceptEntry();
			break;
		case msg_CancelEntry:
			CancelEntry();
			break;
		default:
			BOutlineListView::MessageReceived(msg);
		}
} // DListView::MessageReceived

void DListView::WindowActivated(bool active)
{
	if (!active && fEdit)
		CancelEntry();
} // DListView::WindowActivated

void DListView::KeyDown(const char *bytes, int32 numBytes)
{
	int selected = CurrentSelection();
	int32 modifiers;
	
	Looper()->CurrentMessage()->FindInt32("modifiers", &modifiers);
	
	try
	{
		switch (*bytes)
		{
			case B_DELETE:
				if (fDeleteMsg)
					Window()->PostMessage(fDeleteMsg);
				break;
			
			case B_RETURN:
				Invoke();
				break;
			
			case B_DOWN_ARROW:
				if (selected == -1)
					Select(CountItems() - 1);
				else if (selected < CountItems() - 1)
					Select(selected + 1);
				break;
			
			case B_UP_ARROW:
				if (selected == -1)
					Select(0);
				else if (selected > 0)
					Select(selected - 1);
				break;
			
			case B_RIGHT_ARROW:
//				if (modifiers & B_COMMAND_KEY)
				if (selected != -1)
					Expand(ItemAt(selected));
				break;
			
			case B_LEFT_ARROW:
//				if (modifiers & B_COMMAND_KEY)
				if (selected != -1)
					Collapse(ItemAt(selected));
				break;
			
			case B_SPACE:
				if (fToggleMsg)
					Window()->PostMessage(fToggleMsg);
				break;

//			case B_TAB:
//			{
//				int32 modifiers;
//				Looper()->CurrentMessage()->FindInt32("modifiers", &modifiers);
//				HandleTab(modifiers & B_SHIFT_KEY);
//				break;
//			}

			default:
				if (isalnum(*bytes) || (*bytes == B_BACKSPACE && fTyped.length()))
				{
					if (*bytes == B_BACKSPACE)
					{
						int l = mprevcharlen(fTyped.c_str() + fTyped.length());
						fTyped.erase(fTyped.length() - l, fTyped.length());
					}
					else if (fLastKeyDown == 0)
						fTyped.assign(bytes, numBytes);
					else
						fTyped.append(bytes, numBytes);
				
					fLastKeyDown = system_time();
				}
				else
					BView::KeyDown(bytes, numBytes);
				break;
		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}
	
	ScrollToSelection();
} // DListView::KeyDown

class IsLess
{
  public:
	IsLess() {}
	bool operator () (const DListItem* const& item, const string& t)
		{ return strcasecmp(item->Text(), t.c_str()) < 0; }
};

class IsLessItem
{
  public:
	IsLessItem() {}
	bool operator () (const DListItem* const & a, const DListItem* const& b) const
		{ return *a < *b; }
};

void DListView::Pulse()
{
	if (fLastKeyDown && fLastKeyDown < system_time() - 400000)
	{
		DListItem **start = (DListItem **)Items();
		vector<DListItem*> items(start, start + CountItems());

		std::stable_sort(items.begin(), items.end(), IsLessItem());
		
		vector<DListItem*>::iterator found;
		found = std::lower_bound(items.begin(), items.end(), fTyped, IsLess());
		
		if (found != items.end())
			Select(IndexOf(*found));
		else
			Select(-1);
		
		ScrollToSelection();
	
		fLastKeyDown = 0;
	}
} // DListView::Pulse

void DListView::HandleTab(bool backward)
{
	DListItem **start = (DListItem **)Items(), *item;

	item = static_cast<DListItem*>(ItemAt(CurrentSelection()));
	if (item == NULL)
		return;

	vector<DListItem*> items(start, start + CountItems());
	std::stable_sort(items.begin(), items.end(), IsLessItem());
	
	vector<DListItem*>::iterator found = std::find(items.begin(), items.end(), item);

	if (backward)
		--found;
	else
		++found;
	
	if (found != items.end())
		Select(IndexOf(*found));
	else
		Select(-1);
	
	ScrollToSelection();
} // DListView::HandleTab

void DListView::AcceptEntry()
{
	try
	{
		DListItem *item = dynamic_cast<DListItem*>(ItemAt(fLastItem));
		
		if (item)
			item->SetColumnText(fEditColumn, fEdit->Text());
	
		RemoveChild(fEditScroller);
		delete fEditScroller;
		fEdit = NULL;
		fEditScroller = NULL;
	}
	catch (HErr& e)
	{
		e.DoError();
	}
} // DListView::AcceptEntry

void DListView::CancelEntry()
{
	if (fEdit)
	{
		RemoveChild(fEditScroller);
		delete fEditScroller;
		fEdit = NULL;
		fEditScroller = NULL;
	}
} // DListView::CancelEntry

void DListView::ExpandOrCollapse(BListItem *item, bool expand)
{
	DListItem *dItem = dynamic_cast<DListItem*>(item);
	if (dItem)
	{
		if (expand)
			dItem->Expanded(this);
		else
			dItem->Collapsed(this);
	}
	
	if (item)
		BOutlineListView::ExpandOrCollapse(item, expand);
} // DListView::ExpandOrCollapse

//#pragma mark -

DListBox::DListBox(BRect frame, const char *name)
	: BView (frame, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
{
	fEdit = NULL;
	
	font_height fh;
	be_plain_font->GetHeight(&fh);
	
	BRect hRect = Bounds();
	hRect.bottom = hRect.top + fh.ascent + fh.descent + 6;
	
	BRect lRect = Bounds();
	
	lRect.top = hRect.bottom;
	lRect.right -= B_V_SCROLL_BAR_WIDTH;
	
	lRect.InsetBy(2, 2);
	
	fList = new DListView(this, lRect, "list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	AddChild(new BScrollView("scrl", fList, B_FOLLOW_ALL_SIDES, 0, false, true));
	
	SetViewColor(kViewColor);
} // DListBox::DListBox

void DListBox::Draw(BRect /*r*/)
{
	font_height fh;
	be_plain_font->GetHeight(&fh);
	
	BRect hRect = Bounds();
	hRect.bottom = hRect.top + fh.ascent + fh.descent + 4;
	
	SetHighColor(kVeryDark);
	StrokeRect(hRect);
	
	hRect.InsetBy(1, 1);
	
	SetHighColor(kWhite);
	StrokeLine(hRect.LeftBottom(), hRect.LeftTop());
	StrokeLine(hRect.LeftTop(), hRect.RightTop());
	
	SetHighColor(kDarkShadow);
	StrokeLine(hRect.LeftBottom(), hRect.RightBottom());
	StrokeLine(hRect.RightBottom(), hRect.RightTop());

	hRect.InsetBy(1, 1);
		
	SetLowColor(kViewColor);
	FillRect(hRect, B_SOLID_LOW);
	
	hRect.InsetBy(-1, 0);
	
	float y = hRect.bottom - fh.descent;
	float x = hRect.left + kDisclosureTriangleWidth;
	
	for (vector<DColumn>::iterator i = fColumns.begin(); i != fColumns.end(); i++)
	{
		if (x != hRect.left + kDisclosureTriangleWidth)
		{
			SetHighColor(kDarkShadow);
			StrokeLine(BPoint(x - 4, hRect.top), BPoint(x - 4, hRect.bottom));
			SetHighColor(kVeryDark);
			StrokeLine(BPoint(x - 3, hRect.top - 1), BPoint(x - 3, hRect.bottom + 1));
			SetHighColor(kWhite);
			StrokeLine(BPoint(x - 2, hRect.top), BPoint(x - 2, hRect.bottom));
		}
		
		SetHighColor(kBlack);
		DrawString((*i).fTitle, BPoint(x, y));
		x += (*i).fWidth;
	}
} // DListBox::Draw

void DListBox::MouseDown(BPoint where)
{
	fList->CancelEntry();
	
	int col = ColumnDividerHitBy(where);
	
	if (col >= 0)
	{
		BRect r(Bounds());
		float x = r.left + kDisclosureTriangleWidth;
		
		if (col > 0) x += fColumns[col - 1].fWidth;
		
		BPoint p = where;
		unsigned long btns;
		
		do
		{
			if (p.x != where.x)
			{
				float dx = p.x - where.x;
				
				if (fColumns[col].fWidth + dx < 0)
					dx = -fColumns[col].fWidth;
				
				if (dx != 0)
				{
					fColumns[col].fWidth += dx;

					fList->Draw(fList->Bounds());
					Invalidate();

					where = p;
				}
			}
			
			GetMouse(&p, &btns);
		}
		while (btns);
	}
} // DListBox::MouseDown

int DListBox::ColumnDividerHitBy(BPoint where)
{
	font_height fh;
	int result = -1;
	
	be_plain_font->GetHeight(&fh);
	
	BRect hRect = Bounds();
	hRect.bottom = hRect.top + fh.ascent + fh.descent + 4;
	
	hRect.right = hRect.left + 6;
	
	hRect.OffsetBy(kDisclosureTriangleWidth - 5, 0);
	
	for (vector<DColumn>::iterator i = fColumns.begin(); i != fColumns.end() && result == -1; i++)
	{
		hRect.OffsetBy((*i).fWidth, 0);
		if (hRect.Contains(where))
			result = i - fColumns.begin();
	}
	
	return result;
} // DListBox::ColumnDividerHitBy

void DListBox::AddColumn(const char *title, float width)
{
	DColumn col;
	
	col.fTitle = strdup(title);
	col.fWidth = width;
	
	fColumns.push_back (col);
} // DListBox::AddColumn

void DListBox::FrameResized(float /*w*/, float /*h*/)
{
	Invalidate();
	fList->Invalidate();
} // DListBox::FrameResized

void DListBox::Changed()
{
	fList->Invalidate();
	Window()->UpdateIfNeeded();
} // DListBox::Changed

BOutlineListView* DListBox::List() const
{
	return fList;
} // DListBox::List

void DListBox::SetDeleteMessage(long msg)
{
	fList->SetDeleteMessage(msg);
} // DListBox::SetDeleteMessage

void DListBox::SetToggleMessage(long msg)
{
	fList->SetToggleMessage(msg);
} // DListBox::SetDeleteMessage

//#pragma mark -

DListItem::DListItem(const char *txt)
	: BStringItem(txt)
{
	fBox = NULL;
} // DListItem::DListItem

void DListItem::DrawItem(BView *owner, BRect bounds, bool complete)
{
	vector<DColumn>::iterator i;
	uint32 j;
	
	BRect r(bounds);
		
		// this sucks....
	if (fBox == NULL)
	{
		BView *v = owner;
		
		while (fBox == NULL && v != NULL)
		{
			fBox = dynamic_cast<DListBox*>(v);
			v = v->Parent();
		}
		
		ASSERT_OR_THROW (fBox);
	}
	
	for (i = fBox->fColumns.begin(), j = 0; i != fBox->fColumns.end(); i++, j++)
	{
		if (j == 0)
			r.right = (*i).fWidth + 14;
		else if (j == fBox->fColumns.size() - 1)
			r.right = bounds.right;
		else
			r.right = r.left + (*i).fWidth;

		DrawItem(owner, r, complete, j);
		r.left = r.right;
	}
} // DListItem::DrawItem

void DListItem::DrawItem(BView *owner, BRect bounds, bool /*complete*/, int column)
{
	font_height fh;
	be_plain_font->GetHeight(&fh);
	
	owner->SetLowColor(tint_color(ui_color(B_UI_DOCUMENT_BACKGROUND_COLOR),
		IsSelected() ? B_HIGHLIGHT_BACKGROUND_TINT : B_NO_TINT));
	owner->FillRect(bounds, B_SOLID_LOW);
	
	owner->DrawString(GetColumnText(column),
		BPoint(bounds.left + 2, bounds.bottom - fh.descent));
} // DListItem::DrawItem

void DListItem::Expanded(BOutlineListView* /*list*/)
{
} // DListItem::Expanded

void DListItem::Collapsed(BOutlineListView* /*list*/)
{
} // DListItem::Collapsed

const char* DListItem::GetColumnText(int /*column*/)
{
	return NULL;
} // DListItem::GetColumnText

void DListItem::SetColumnText(int /*column*/, const char* /*newText*/)
{
	ASSERT(false);
} // DListItem::SetColumnText

bool DListItem::ClickItem(BView* /*owner*/, BRect /*frame*/, BPoint /*where*/)
{
	return false;
} // DListItem::ClickItem
