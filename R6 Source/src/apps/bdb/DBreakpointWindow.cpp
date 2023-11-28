/*	$Id: DBreakpointWindow.cpp,v 1.9 1999/05/03 13:09:48 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman

*/

#include "bdb.h"
#include "DBreakpointWindow.h"
#include "DMessages.h"
#include "DBreakpoint.h"
#include "DTeamWindow.h"
#include "DTeam.h"
#include "DListBox.h"

#include <OutlineListView.h>
#include <Beep.h>

static const uchar *kBPIcon = NULL;

const long
	kMsgInvokeBreakpoint = 'BkIn',
	kMsgDeleteBreakpoint = 'BkDl',
	kMsgToggleBreakpoint = 'BkTg';

const int
	kBreakpointColumn = 0,
	kSkipColumn = 1,
	kHitsColumn = 2,
	kConditionColumn = 3;

class DBreakpointItem : public DListItem
{
  public:
	DBreakpointItem(const DBreakpoint& bp);
	
	virtual void DrawItem(BView *owner, BRect frame, bool complete, int column);
	
	virtual const char* GetColumnText(int column);
	virtual void SetColumnText(int column, const char *newText);

	virtual bool ClickItem(BView *owner, BRect frame, BPoint where);

	virtual const char* Label() const;

	ptr_t PC() const		{ return fBreakpoint.PC(); }
	bpKind Kind() const	{ return fBreakpoint.Kind(); }

  private:
	const DBreakpoint& fBreakpoint;
	BOutlineListView* fListBox;
	BString fSkipText, fHitsText;
};

DBreakpointItem::DBreakpointItem(const DBreakpoint& bp)
	: DListItem(""), 
	  fBreakpoint(bp),
	  fListBox(NULL)
{
	char s[PATH_MAX];
	sprintf(s, "%s:%d:", fBreakpoint.Function(), fBreakpoint.Line());
	SetText(s);
} // DBreakpointItem::DBreakpointItem

void DBreakpointItem::DrawItem(BView *owner, BRect bounds, bool /*complete*/, int column)
{
	fListBox = static_cast<BOutlineListView*>(owner);

	font_height fh;
	be_plain_font->GetHeight(&fh);
	
	owner->SetLowColor(tint_color(ui_color(B_UI_DOCUMENT_BACKGROUND_COLOR),
		IsSelected() ? B_HIGHLIGHT_BACKGROUND_TINT : B_NO_TINT));
	owner->FillRect(bounds, B_SOLID_LOW);
	
	owner->SetFont(be_plain_font);
	owner->SetHighColor(tint_color(ui_color(B_UI_DOCUMENT_TEXT_COLOR),
		(fBreakpoint.Kind() < ebSaved) ? B_NO_TINT : B_DISABLED_LABEL_TINT));

	switch (column)
	{
	case kBreakpointColumn:
		{
			BPoint pos(bounds.left, bounds.bottom - fh.descent);
			DBreakpoint::Draw(owner, pos, fBreakpoint.Kind());
			owner->DrawString(Text(), BPoint(bounds.left + 10, bounds.bottom - fh.descent));
		}
		break;

	case kSkipColumn:
	case kHitsColumn:
		{
			char buf[16];
			sprintf(buf, "%lu", (column == kSkipColumn) ? fBreakpoint.SkipCount() : fBreakpoint.Hits());
			owner->DrawString(buf, BPoint(bounds.left + 2, bounds.bottom - fh.descent));
		}
		break;

	case kConditionColumn:
		owner->DrawString(fBreakpoint.GetCondition().String(), BPoint(bounds.left + 2, bounds.bottom - fh.descent));
		break;
	}
} // DBreakpointItem::DrawItem

const char* DBreakpointItem::GetColumnText(int column)
{
	char buf[16];
	switch (column)
	{
	case kSkipColumn:
		sprintf(buf, "%lu", fBreakpoint.SkipCount());
		fSkipText = buf;
		return fSkipText.String();

	case kHitsColumn:
		sprintf(buf, "%lu", fBreakpoint.Hits());
		fHitsText = buf;
		return fHitsText.String();

	case kConditionColumn:
		return fBreakpoint.GetCondition().String();
	}

	// none of the above; return NULL
	return NULL;
} // DBreakpointItem::GetColumnText

void DBreakpointItem::SetColumnText(int column, const char *newText)
{
	if (fListBox)
	{
		DBreakpointWindow *w = static_cast<DBreakpointWindow*>(fListBox->Window());
		switch (column)
		{
		case kSkipColumn:
			w->SetBreakpointSkipCount(fBreakpoint.PC(), strtoul(newText, NULL, 10));
			break;

		case kHitsColumn:
			w->SetBreakpointHitCount(fBreakpoint.PC(), strtoul(newText, NULL, 10));
			break;

		case kConditionColumn:
			w->SetBreakpointCondition(fBreakpoint.PC(), newText);
			break;
		}
	}
}

bool DBreakpointItem::ClickItem(BView *owner, BRect frame, BPoint where)
{
	if (where.x > frame.left + 5 || fBreakpoint.Kind() >= ebSaved)
		return false;
	
	font_height fh;
	owner->GetFontHeight(&fh);
	BPoint p(frame.left, frame.bottom - fh.descent);
	
	bool wasEnabled = (fBreakpoint.Kind() != ebDisabled);

	if (DBreakpoint::Track(owner, p, wasEnabled))
	{
		DBreakpointWindow *w = static_cast<DBreakpointWindow*>(owner->Window());
		
		try
		{
			if (wasEnabled)
				w->DisableBreakpoint(fBreakpoint.PC());
			else
				w->EnableBreakpoint(fBreakpoint.PC());
		}
		catch (HErr& e)
		{
			e.DoError();
		}
	}
	
	return true;
} // DBreakpointItem::ClickItem

const char* DBreakpointItem::Label() const
{
	return "";		// no order necessary
} // DBreakpointItem::Label

//#pragma mark -

DBreakpointWindow::DBreakpointWindow(BWindow *owner)
	: BWindow(gPrefs->GetPrefRect("breakpoint window position", BRect(100, 100, 200, 200)),
		"Breakpoints", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0)
{
	if (kBPIcon == NULL)
		kBPIcon = (uchar *)HResources::GetResource('MICN', 1001);
	
	fOwner = owner;
	
	BRect frame(Bounds());
	frame.InsetBy(-2, -2);
	frame.top += 1;

	AddChild(fList = new DListBox(frame, "myList"));
	fList->AddColumn("Breakpoint", 130);
	fList->AddColumn("Skip", 40);
	fList->AddColumn("Hits", 40);
	fList->AddColumn("Condition", 40);

	fList->List()->SetInvocationMessage(new BMessage(kMsgInvokeBreakpoint));

	ReloadBreakpoints();
	
	AddShortcut(B_BACKSPACE, 0, new BMessage(kMsgDeleteBreakpoint));
	fList->SetDeleteMessage(kMsgDeleteBreakpoint);
	fList->SetToggleMessage(kMsgToggleBreakpoint);

	fList->List()->MakeFocus();
	
	if (gPrefs->GetPrefInt("breakpoint window visible"))
		Show();
	else
		Run();
} // DBreakpointWindow::DBreakpointWindow

bool DBreakpointWindow::QuitRequested()
{
	Hide();
	return false;
} // DBreakpointWindow::QuitRequested

void DBreakpointWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgBreakpointsChanged:
			ReloadBreakpoints();
			break;
		
		case kMsgInvokeBreakpoint:
			DisplayBreakpoint();
			break;
		
		case kMsgDeleteBreakpoint:
			DeleteBreakpoint();
			break;
		
		case kMsgToggleBreakpoint:
			ToggleBreakpoint();
			break;
		
		default:
			BWindow::MessageReceived(msg);
			break;
	}
} // DBreakpointWindow::MessageReceived

void DBreakpointWindow::ReloadBreakpoints()
{
	int selIx = -1;

	DisableUpdates();

	DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
	BAutolock lock(team);
	if (lock.IsLocked())
	{
		std::list<DBreakpoint>& breakpoints = team.GetBreakpoints();

		selIx = fList->List()->CurrentSelection();

		for (int i = fList->List()->CountItems() - 1; i >= 0; i--)
			delete fList->List()->RemoveItem(i);
	
		std::list<DBreakpoint>::iterator bpi;
		for (bpi = breakpoints.begin(); bpi != breakpoints.end(); bpi++)
			fList->List()->AddItem(new DBreakpointItem(*bpi));
	}
	
	EnableUpdates();
	UpdateIfNeeded();

	if (selIx >= 0)
		fList->List()->Select(selIx);
} // DBreakpointWindow::ReloadBreakpoints

void DBreakpointWindow::ClearBreakpoint(ptr_t pc)
{
	DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
	
	BAutolock lock(team);
	
	if (lock.IsLocked())
		team.ClearBreakpoint(pc);
} // DBreakpointWindow::ClearBreakpoint

void DBreakpointWindow::EnableBreakpoint(ptr_t pc)
{
	DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
	
	BAutolock lock(team);
	
	if (lock.IsLocked())
		team.EnableBreakpoint(pc);
} // DBreakpointWindow::EnableBreakpoint

void DBreakpointWindow::DisableBreakpoint(ptr_t pc)
{
	DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
	
	BAutolock lock(team);
	
	if (lock.IsLocked())
		team.DisableBreakpoint(pc);
} // DBreakpointWindow::DisableBreakpoint

void DBreakpointWindow::SetBreakpointCondition(ptr_t pc, const char* condition)
{
	DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
	
	BAutolock lock(team);
	if (lock.IsLocked())
		team.SetBreakpointCondition(pc, condition);
} // DBreakpointWindow::DisableBreakpoint

void 
DBreakpointWindow::SetBreakpointSkipCount(ptr_t pc, unsigned long skipCount)
{
	DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
	
	BAutolock lock(team);
	if (lock.IsLocked())
		team.SetBreakpointSkipCount(pc, skipCount);
}

void 
DBreakpointWindow::SetBreakpointHitCount(ptr_t pc, unsigned long hitCount)
{
	DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
	
	BAutolock lock(team);
	if (lock.IsLocked())
		team.SetBreakpointHitCount(pc, hitCount);
}

void DBreakpointWindow::DisplayBreakpoint()
{
	DBreakpointItem *bp = static_cast<DBreakpointItem*>(fList->List()->ItemAt(fList->List()->CurrentSelection()));
	if (bp)
	{
		DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
		
		BAutolock lock(team);
		
		if (lock.IsLocked())
		{
			BMessage msg(kMsgShowAddress);
			msg.AddInt32("address", bp->PC());
			FailMessageTimedOutOSErr(BMessenger(team.GetOwner()).SendMessage(&msg,
				(BHandler *)0, 1000));
		}
	}
} // DBreakpointWindow::DisplayBreakpoint

void DBreakpointWindow::DeleteBreakpoint()
{
	DBreakpointItem *bp = static_cast<DBreakpointItem*>(fList->List()->ItemAt(fList->List()->CurrentSelection()));
	if (bp)
		ClearBreakpoint(bp->PC());
} // DBreakpointWindow::DeleteBreakpoint

void DBreakpointWindow::ToggleBreakpoint()
{
	DBreakpointItem *bp = static_cast<DBreakpointItem*>(fList->List()->ItemAt(fList->List()->CurrentSelection()));
	if (bp)
	{
		if (bp->Kind() == ebDisabled)
			EnableBreakpoint(bp->PC());
		else if (bp->Kind() == ebAlways || bp->Kind() == ebOneTime)
			DisableBreakpoint(bp->PC());
		else
			beep();
	}
} // DBreakpointWindow::DeleteBreakpoint
