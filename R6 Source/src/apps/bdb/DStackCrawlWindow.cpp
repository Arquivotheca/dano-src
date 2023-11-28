/*	$Id: DStackCrawlWindow.cpp,v 1.2 1999/05/03 13:09:53 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/13/99 16:26:47
*/

#include "bdb.h"
#include "DStackCrawlWindow.h"
#include "DSourceCode.h"
#include "DSourceView.h"
#include "DListBox.h"
#include "DResizer.h"
#include "DStatement.h"
#include "DUtils.h"
#include "DTeam.h"
#include "DSymWorld.h"
#include "DStackCrawl.h"
#include "DMessages.h"

#include "HButtonBar.h"

#include <StringView.h>
#include <OutlineListView.h>
#include <ScrollView.h>

DStackCrawlWindow::DStackCrawlWindow(const char *name, DTeam& team, DStackCrawl& stackCrawl)
	: BWindow (BRect(100, 100, 500, 600), name, B_DOCUMENT_WINDOW, 0)
{
	BView *V1, *V2;

	BRect r(Bounds()), b = r;

	r.bottom = r.top + std::min(150.0, r.Height() / 3.0);

	AddChild(V1 = new BView(r, "V1", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, 0));
	r.top = r.bottom;
	r.bottom = b.bottom;
	AddChild(V2 = new BView(r, "V2", B_FOLLOW_ALL_SIDES, 0));

	AddChild(new DResizer(BRect(), "Resizer1", V1, V2));

	r = V1->Bounds();
	V1->AddChild(fFiles = new DListBox(r, "files"));

	r = V2->Bounds();
	r.bottom = r.top + 23;

	BView *v;
	V2->AddChild(v = new BView(r, "toolbar", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW));
	v->SetViewColor(kViewColor);
	
	r.OffsetTo(0, 0);
	r.InsetBy(1, 1);
	
	BView *buttonBar;
	v->AddChild(buttonBar = new HButtonBar(r, "ButtonBar", 101, this));
	
	r = v->Bounds();
	r.left += buttonBar->Bounds().Width() + 10;
	r.bottom = r.top + 14; // yeah I know, this should be some font_height calculation...
	BStringView *label;
	v->AddChild(label = new BStringView(r, "label", "", B_FOLLOW_LEFT_RIGHT));
	
	r = V2->Bounds();
	r.top = v->Frame().bottom + 1;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	
	fSource = new DSourceView(r, "source", team, label);
	
	V2->AddChild(new BScrollView("source scroll", fSource, B_FOLLOW_ALL_SIDES, 0, true, true, B_NO_BORDER));
	
	fFiles->AddColumn("Files", 10);

	uint32 deepest = stackCrawl.DeepestUserCode();

	for (uint32 i = 0; i < stackCrawl.CountFrames(); i++)
	{
		DStatement stmt;
		string name;
		
		stackCrawl[i].GetFunctionName(name);
		stackCrawl[i].GetStatement(stmt);
		
		fFiles->List()->AddItem(new DItem<DStatement>(name.c_str(), stmt));
		
		if (i == deepest)
			fSource->SetStatement(stmt, bpShowAddr);
	}
	
	fFiles->List()->Select(deepest);
	fFiles->List()->SetSelectionMessage(new BMessage(kMsgWhereIs));
	
	Show();
} // DStackCrawlWindow::DStackCrawlWindow
	
void DStackCrawlWindow::MessageReceived(BMessage *msg)
{
	if (msg->what == kMsgWhereIs)
	{
		int ix = fFiles->List()->CurrentSelection();
		DItem<DStatement> *item = dynamic_cast<DItem<DStatement>*>(fFiles->List()->ItemAt(ix));
		if (item)
			fSource->SetStatement(item->fData, bpShowAddr);
	}
	else
		BWindow::MessageReceived(msg);
} // DStackCrawlWindow::MessageReceived
