/*	$Id: DMainWindow.cpp,v 1.3 1999/02/11 15:51:53 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 01/20/99 15:02:17
*/

#include "bdb.h"
#include "DMainWindow.h"
#include "DListBox.h"
#include "DPulseView.h"
#include "DMessages.h"
#include "DUtils.h"
#include "DTeamWindow.h"

#include <OutlineListView.h>

DMainWindow* DMainWindow::sWindow;

const ulong kMsgInvokeTeam = 'InvT';

DMainWindow::DMainWindow()
	: BWindow(gPrefs->GetPrefRect("main window pos", BRect(20, 80, 220, 280)),
		"Running Teams", B_DOCUMENT_WINDOW, 0)
{
	BRect frame(Bounds());
	frame.InsetBy(-2, -2);
	frame.top += 1;

	AddChild(fTeams = new DListBox(frame, "myList"));
	fTeams->AddColumn("Teams", 130);
	
	fTeams->List()->SetInvocationMessage(new BMessage(kMsgInvokeTeam));
	
	AddChild(new DPulseView());
	SetPulseRate(500000);
	
	Show();
} // DMainWindow::DMainWindow

DMainWindow::~DMainWindow()
{
} // DMainWindow::~DMainWindow

bool DMainWindow::QuitRequested()
{
	gPrefs->SetPrefRect("main window pos", Frame());
	Hide();
	return false;
} // DMainWindow::QuitRequested

DMainWindow* DMainWindow::Instance()
{
	if (sWindow == NULL)
		sWindow = new DMainWindow();

	return sWindow;
} // DMainWindow::Instance

void DMainWindow::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped() && msg->HasRef("refs"))
	{
		entry_ref ref;
		FailOSErr(msg->FindRef("refs", &ref));
		new DTeamWindow(ref);
	}
	else switch (msg->what)
	{
		case kMsgPulse:
			UpdateTeamList();
			break;
		
		case kMsgInvokeTeam:
		{
			DItem<team_id> *item = dynamic_cast<DItem<team_id>*>(fTeams->List()->ItemAt(fTeams->List()->CurrentSelection()));
			if (item)
			{
				DTeamWindow *w = DTeamWindow::GetTeamWindow(item->fData);
				if (w == NULL)
					new DTeamWindow(item->fData);
				else
					w->Activate();
			}
			break;
		}
		
		default:
			BWindow::MessageReceived(msg);
			break;
	}
} // DMainWindow::MessageReceived

static int compare(const void *a, const void *b)
{
	return (*static_cast<DItem<team_id>* const *>(a))->fData - (*static_cast<DItem<team_id>* const *>(b))->fData;
} // compare

void DMainWindow::UpdateTeamList()
{
	BListView *lstv = fTeams->List();
	BList lst;
	
	team_info ti;
	long cookie = 0;
	
	while (get_next_team_info(&cookie, &ti) == B_OK)
	{
		// don't even let people TRY to debug the kernel_team
		if (ti.team != B_SYSTEM_TEAM)
		{
			lst.AddItem(new DItem<team_id>(ti.args, ti.team));
		}
	}
	
	lst.SortItems(compare);
	
	int i, j;
	i = j = 0;
	
	while (i < lst.CountItems() || j < lstv->CountItems())
	{
		DItem<team_id> *ai, *bi;
		
		ai = static_cast<DItem<team_id>*>(lst.ItemAt(i));
		bi = static_cast<DItem<team_id>*>(lstv->ItemAt(j));
		
		if (ai == NULL && bi == NULL)
		{
			ASSERT(false);
			break;	// should not be possible, but I'm getting paranoid
		}
		
		if (ai == NULL)
		{
			lstv->RemoveItem(bi);
			delete bi;
		}
		else if (bi == NULL || *ai < *bi)
		{
			lstv->AddItem(ai, j);
			i++;
			j++;
		}
		else if (*bi < *ai)
		{
			lstv->RemoveItem(bi);
			delete bi;
			i++;
		}
		else
		{
			delete ai;
			i++;
			j++;
		}
	}
} // DMainWindow::UpdateTeamList
