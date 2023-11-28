#include <String.h>

#include "bdb.h"
#include "DWatchpointWindow.h"
#include "DMessages.h"
#include "DTeamWindow.h"
#include "DTeam.h"
#include "DListBox.h"
#include "DWatchpoint.h"

#include <OutlineListView.h>

const uint32 kMsgInvokeWatchpoint = 'BkIn';
const uint32 kMsgDeleteWatchpoint = 'BkDl';
const uint32 kMsgToggleWatchpoint = 'BkTg';

class DTracepointItem : public DListItem {
	// ToDo:
	// use this also as a base for DBreakpointItem
public:
	DTracepointItem()
		:	DListItem("")
		{}

	virtual DTracepoint &Tracepoint() = 0;

	virtual bool ClickItem(BView *owner, BRect frame, BPoint where);


	virtual const char* Label() const
		{ return ""; }
	
	virtual void DrawItem(BView *, BRect, bool, int);

protected:
	virtual const char* GetColumnText(int column);

private:
	typedef DListItem _inherited;
};

void
DTracepointItem::DrawItem(BView *owner, BRect bounds, bool complete, int column)
{
	if (column == 0) {
		font_height fh;
		be_plain_font->GetHeight(&fh);

		owner->SetLowColor(IsSelected() ? kViewColor : kWhite);
		owner->FillRect(bounds, B_SOLID_LOW);

		BPoint pos(bounds.left, bounds.bottom - fh.descent);
		Tracepoint().Draw(owner, pos, Tracepoint().Kind());
		
		owner->SetHighColor(kBlack);
		owner->DrawString(Text(), BPoint(bounds.left + 10, bounds.bottom - fh.descent));
	} else
		_inherited::DrawItem(owner, bounds, complete, column);
}

const char *
DTracepointItem::GetColumnText(int column)
{
	if (column == 0)
		return Text();
	
	return "wrong column";
}

bool
DTracepointItem::ClickItem(BView *owner, BRect frame, BPoint where)
{
	if (where.x > frame.left + 5)
		return false;
	
	font_height fh;
	owner->GetFontHeight(&fh);
	BPoint point(frame.left, frame.bottom - fh.descent);
	
	bool wasEnabled = !Tracepoint().Disabled();

	if (Tracepoint().Track(owner, point, wasEnabled)) {
		try {
			static_cast<DTracepointWindow*>(owner->Window())->
				ToggleTracepoint(&Tracepoint());
		} catch (HErr& e) {
			e.DoError();
		}
	}
	

	return true;
}

class DWatchpointItem : public DTracepointItem
{
public:
	DWatchpointItem(const DWatchpoint& bp);

	virtual DTracepoint &Tracepoint()
		{ return fWatchpoint; }

	DWatchpoint &Watchpoint()
		{ return fWatchpoint; }

protected:
	virtual const char* GetColumnText(int column);
	
private:
	DWatchpoint fWatchpoint;
	BString fAddress;
	typedef DTracepointItem _inherited;
};


DWatchpointItem::DWatchpointItem(const DWatchpoint &watchpoint)
	:	fWatchpoint(watchpoint)
{
	SetText(watchpoint.Name());
	sprintf(fAddress.LockBuffer(20), "0x%lx", fWatchpoint.Address());
	fAddress.UnlockBuffer();
}

const char *
DWatchpointItem::GetColumnText(int column)
{
	if (column == 1)
		return fAddress.String();
	
	return _inherited::GetColumnText(column);
}

// #pragma mark -

DWatchpointWindow::DWatchpointWindow(BWindow *owner)
	: DTracepointWindow(gPrefs->GetPrefRect("watchpoint window position", BRect(100, 100, 200, 200)),
		"Watchpoints", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0, owner)
{	
	BRect frame(Bounds());
	frame.InsetBy(-2, -2);
	frame.top += 1;

	AddChild(fList = new DListBox(frame, "myList"));
	fList->AddColumn("Watchpoint", 130);
	fList->AddColumn("Address", 40);

	AddShortcut(B_BACKSPACE, 0, new BMessage(kMsgDeleteWatchpoint));
	fList->SetDeleteMessage(kMsgDeleteWatchpoint);
	fList->SetToggleMessage(kMsgToggleWatchpoint);

	fList->List()->MakeFocus();
	
	if (gPrefs->GetPrefInt("watchpoint window visible"))
		Show();
	else
		Run();
}

bool
DWatchpointWindow::QuitRequested()
{
	Hide();
	return false;
}

void
DWatchpointWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		
		case kMsgWatchpointsChanged:
			ReloadWatchpoints();
			break;

		case kMsgDeleteWatchpoint:
			DeleteWatchpoint();
			break;
		
		case kMsgToggleWatchpoint:
			ToggleWatchpoint();
			break;
		
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

void 
DWatchpointWindow::ReloadWatchpoints()
{
	WatchpointList watchpoints;

	{
		DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
		
		BAutolock lock(team);
		
		if (lock.IsLocked())
			team.GetWatchpoints(watchpoints);
	}
	
	DisableUpdates();
	
	int selIx = fList->List()->CurrentSelection();
	
	for (int i = fList->List()->CountItems() - 1; i >= 0; i--)
		delete fList->List()->RemoveItem(i);
	
	WatchpointList::iterator i;
	
	for (i = watchpoints.begin(); i != watchpoints.end(); i++)
		fList->List()->AddItem(new DWatchpointItem(*i));
	
	EnableUpdates();
	UpdateIfNeeded();

	if (selIx >= 0)
		fList->List()->Select(selIx);
}

void 
DWatchpointWindow::ToggleTracepoint(DTracepoint *tracepoint)
{
	DWatchpoint *watchpoint = dynamic_cast<DWatchpoint *>(tracepoint);
	FailNil(watchpoint);
	
	if (watchpoint->Disabled())
		EnableWatchpoint(watchpoint->Address());
	else
		DisableWatchpoint(watchpoint->Address());
}

void
DWatchpointWindow::ClearWatchpoint(ptr_t addr)
{
	DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
	
	BAutolock lock(team);
	
	if (lock.IsLocked())
		team.ClearWatchpoint(addr);
}

void
DWatchpointWindow::EnableWatchpoint(ptr_t addr)
{
	DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
	BAutolock lock(team);
	
	if (lock.IsLocked())
		team.EnableWatchpoint(addr);
}

void
DWatchpointWindow::DisableWatchpoint(ptr_t addr)
{
	DTeam& team = static_cast<DTeamWindow*>(fOwner)->GetTeam();
	
	BAutolock lock(team);
	
	if (lock.IsLocked())
		team.DisableWatchpoint(addr);
}
void
DWatchpointWindow::DeleteWatchpoint()
{
	DWatchpointItem *item = static_cast<DWatchpointItem*>(fList->List()->
		ItemAt(fList->List()->CurrentSelection()));
	if (item)
		ClearWatchpoint(item->Watchpoint().Address());
}

void
DWatchpointWindow::ToggleWatchpoint()
{
	DWatchpointItem *item = static_cast<DWatchpointItem*>(fList->List()->
		ItemAt(fList->List()->CurrentSelection()));
	if (item) {
		if (item->Watchpoint().Disabled())
			EnableWatchpoint(item->Watchpoint().Address());
		else
			DisableWatchpoint(item->Watchpoint().Address());
	}
}
