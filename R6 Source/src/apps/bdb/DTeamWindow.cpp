/*	$Id: DTeamWindow.cpp,v 1.23 1999/05/11 21:31:09 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/10/98 09:02:51
*/

#include "bdb.h"
#include "DTeamWindow.h"
#include "DMessages.h"
#include "HAppResFile.h"
#include "DThreadWindow.h"
#include "DNub.h"
#include "DDebugApp.h"
#include "DPulseView.h"
#include "DTeam.h"
#include "DThread.h"
#include "DUtils.h"
#include "DSourceView.h"
#include "DResizer.h"
#include "DListBox.h"
#include "DBreakpointWindow.h"
#include "DWatchpointWindow.h"
#include "DShowAddress.h"
#include "DShowFunction.h"
#include "DVariableItem.h"
#include "DCpuState.h"
#include "DStackCrawlWindow.h"

#include <MenuItem.h>
#include <OutlineListView.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Screen.h>
#include <Roster.h>
#include <Beep.h>
#include <Alert.h>

using std::map;
using std::list;
using std::vector;
using std::pair;

list<DTeamWindow*> DTeamWindow::sfWindows;

DTeamWindow::DTeamWindow(entry_ref& ref, int argc, char **argv)
	: DWindow(GetNextTeamRect(), "team", B_DOCUMENT_WINDOW, 0)
{
	fTargetRef = ref;
	fTeam = DTeam::CreateTeam(ref, this, argc, argv);
	BuildWindow();
	
	sfWindows.push_back(this);
} /* DTeamWindow::DTeamWindow */

DTeamWindow::DTeamWindow(team_id id, port_id port)
	: DWindow(GetNextTeamRect(), "team", B_DOCUMENT_WINDOW, 0)
{
	fTeam = DTeam::CreateTeam(id, port, this);
	fTargetRef = fTeam->GetRef();
	BuildWindow();

	sfWindows.push_back(this);
} // DTeamWindow::DTeamWindow

DTeamWindow::DTeamWindow(const char *host, uint16 port)
	: DWindow(GetNextTeamRect(), "team", B_DOCUMENT_WINDOW, 0)
{
	fTeam = DTeam::CreateTeam(host, port, this);
	fTargetRef = fTeam->GetRef();
	BuildWindow();

	sfWindows.push_back(this);
}


DTeamWindow::~DTeamWindow()
{
	list<DTeamWindow*>::iterator wi = find(sfWindows.begin(), sfWindows.end(), this);
	if (wi != sfWindows.end())
		sfWindows.erase(wi);

	fTeam->Lock();
	delete fTeam;
} /* DTeamWindow::~DTeamWindow */
			
bool DTeamWindow::QuitRequested()
{
	try
	{
		map<thread_id,DThreadWindow*>::iterator twi;
		
		for (twi = fThreadWindows.begin(); twi != fThreadWindows.end(); twi++)
		{
			if ((*twi).second->Lock())
			{
				if ((*twi).second->QuitRequested())
					(*twi).second->Quit();
				else
				{
					(*twi).second->Unlock();
					return false;
				}
			}
		}
		
		if (fTeam->GetID() != -1)
		{
			BAlert* a = new BAlert("", "Do you want to kill the app or resume it?", "Kill", "Resume", "Cancel");
			a->SetShortcut(0, 'k');	// "kill"
			a->SetShortcut(1, 'r');		// "resume"
			a->SetShortcut(2, 27);	// esc == "cancel"
			switch (a->Go())
			{
				case 0:	DoKill(); break;
				case 1:	Detach(); break;
				default:	return false;
			}
		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}
	
	fBreakpointWindow->Lock();
	gPrefs->SetPrefInt("breakpoint window visible", ! fBreakpointWindow->IsHidden());
	gPrefs->SetPrefRect("breakpoint window position", fBreakpointWindow->Frame());
	fTeam->BreakpointsWindowRect() = fBreakpointWindow->Frame();
	fBreakpointWindow->Quit();
	
	fWatchpointWindow->Lock();
	gPrefs->SetPrefInt("watchpoint window visible", ! fWatchpointWindow->IsHidden());
	gPrefs->SetPrefRect("watchpoint window position", fWatchpointWindow->Frame());
	fTeam->BreakpointsWindowRect() = fWatchpointWindow->Frame();
	fWatchpointWindow->Quit();

	gPrefs->SetPrefRect("teamwindowrect", Frame());
	fTeam->TeamWindowRect() = Frame();

	be_app->PostMessage(kMsgTeamWindowClosed);

	list<DTeamWindow*>::iterator wi = find(sfWindows.begin(), sfWindows.end(), this);
	if (wi != sfWindows.end())
		sfWindows.erase(wi);

	return true;
} /* DTeamWindow::QuitRequested */

void DTeamWindow::BuildWindow()
{
	if (fTeam->TeamWindowRect().IsValid())
	{
		BRect f = fTeam->TeamWindowRect();
		MoveTo(f.left, f.top);
		ResizeTo(f.Width(), f.Height());
	}

	BView *V1, *V2;
	BView *H1, *H2, *H3, *H4;

	BRect r(Bounds()), b = r;
	BMenuBar *mbar;
	
	AddChild(mbar = HResources::GetMenuBar(r, 100));
	FailNil(mbar);
	mbar->FindItem(kMsgQuit)->SetTarget(be_app);

	// set up the resizer between the source pane and the upper panes
	r.top = mbar->Frame().bottom;
	r.bottom = r.top + std::min(150.0, r.Height() / 3.0);

	AddChild(V1 = new BView(r, "V1", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, 0));
	r.top = r.bottom + 1;
	r.bottom = b.bottom;
	AddChild(V2 = new BView(r, "V2", B_FOLLOW_ALL_SIDES, 0));

	AddChild(new DResizer(BRect(), "Resizer1", V1, V2));

	// now the resizers between the thread, file, and globals panes
	r = V1->Bounds();
	r.right = r.left + r.Width() / 4;
	V1->AddChild(H1 = new BView(r, "H1", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 0));

	r.left = r.right + 1;
	r.right = V1->Bounds().right + 1;
	V1->AddChild(H4 = new BView(r, "H4", B_FOLLOW_ALL_SIDES, 0));

	r = H4->Bounds();
	r.right = r.left + r.Width() / 3;
	H4->AddChild(H2 = new BView(r, "H2", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 0));
	r.left = r.right + 1;
	r.right = H4->Bounds().right + 1;
	H4->AddChild(H3 = new BView(r, "H3", B_FOLLOW_ALL_SIDES, 0));

	V1->AddChild(new DResizer(BRect(), "Resizer2", H1, H4));
	H4->AddChild(new DResizer(BRect(), "Resizer3", H2, H3));

	r = H1->Bounds();
	H1->AddChild(fThreads = new DListBox(r, "thread list"));
	fThreads->List()->SetInvocationMessage(new BMessage(kMsgWhereIs));

	r = H2->Bounds();
	H2->AddChild(fFiles = new DListBox(r, "files"));
	fFiles->List()->SetSelectionMessage(new BMessage(kMsgFileSelection));
	fFiles->List()->SetInvocationMessage(new BMessage(kMsgFileOpenInPreferredEditor));
	
	
	r = H3->Bounds();
	H3->AddChild(fVariables = new DListBox(r, "Globals"));
	fVariables->AddColumn("Globals", 120);
	fVariables->AddColumn("", 40);
	
	r = V2->Bounds();
	r.bottom = r.top + 23;
	
	BView *v;
	V2->AddChild(v = new BView(r, "toolbar", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW));
	v->SetViewColor(kViewColor);
	
	r.OffsetTo(0, 0);
	r.InsetBy(1, 1);
	
	v->AddChild(fButtonBar = new HButtonBar(r, "ButtonBar", 100, this));
	
	r = v->Bounds();
	r.left += fButtonBar->Bounds().Width() + 10;
	r.bottom = r.top + 14; // yeah I know, this should be some font_height calculation...
	BStringView *label;
	v->AddChild(label = new BStringView(r, "label", "", B_FOLLOW_LEFT_RIGHT));
	
	r = V2->Bounds();
	r.top = v->Frame().bottom + 1;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	
	fSource = new DSourceView(r, "source", *fTeam, label);
	
	V2->AddChild(new BScrollView("source scroll", fSource, B_FOLLOW_ALL_SIDES, 0, true, true, B_NO_BORDER));
	
	fFiles->AddColumn("Files", 10);
	fThreads->AddColumn("Threads", 10);
	
	ListThreads();
	ListFiles();
	ListGlobals();
	
	AddChild(new DPulseView);
	SetPulseRate(2000000);
	fLastGlobalUpdate = 0;

	fBreakpointWindow = new DBreakpointWindow(this);
	fWatchpointWindow = new DWatchpointWindow(this);

	char title[300];
	sprintf(title, "Team: %s", fTargetRef.name);
	SetTitle(title);

	Show();
	PostMessage(kMsgRun);
} // DTeamWindow::BuildWindow

void DTeamWindow::DoKill()
{
	BAutolock lock(fTeam);
	
	if (fTeam->IsLocked())
		fTeam->DoKill();
} /* DTeamWindow::DoKill */

void DTeamWindow::Detach()
{
	BAutolock lock(fTeam);
	
	if (fTeam->IsLocked())
		fTeam->Detach();
} // DTeamWindow::Detach

static long launch(void *p)
{
	DTeam *team = (DTeam *)p;
	team->Launch();
	return 0;
} // launch

void DTeamWindow::MessageReceived(BMessage *msg)
{
	try
	{
		switch (msg->what)
		{
			case kMsgKill:
				DoKill();
				break;
			
			case kMsgDebugThread:
				DoDebugThread();
				break;
			
			case kMsgPulse:
				if (fLastGlobalUpdate < system_time() - 250000)
				{
					ListThreads();
					fLastGlobalUpdate = system_time();
				}
				break;
			
			case kMsgStatus:
			{
				BStringView *label = dynamic_cast<BStringView*>(FindView("label"));
				if (label)
				{
					const char *s;
					msg->FindString("status", &s);
					if (s)
						label->SetText(s);
				}
				UpdateIfNeeded();
				break;
			}
			
			case kMsgFileSelection:
			{
				DItem<DFileNr> *item = static_cast<DItem<DFileNr>*>(fFiles->List()->ItemAt(fFiles->List()->CurrentSelection()));
				if (item)
					fSource->SetSourceFile(item->fData);
				break;
			}
			
			case kMsgFileOpenInPreferredEditor:
			{
				DItem<DFileNr> *item = static_cast<DItem<DFileNr>*>(fFiles->List()->ItemAt(fFiles->List()->CurrentSelection()));
				if (item) {
					entry_ref ref(DSourceFileTable::Instance()[item->fData]);
					BRoster().Launch(&ref);
				}
				break;
			}
			
			case kMsgThreadWindowCreated:
			{
				thread_id thr;
				DThreadWindow *w;
				FailOSErr(msg->FindInt32("thread", &thr));
				FailOSErr(msg->FindPointer("window", (void**)&w));
				fThreadWindows[thr] = w;
				break;
			}
			
			case kMsgThreadWindowClosed:
			{
				DThreadWindow *w;
				FailOSErr(msg->FindPointer("window", (void **)&w));
				for (map<thread_id,DThreadWindow*>::iterator i = fThreadWindows.begin(); i != fThreadWindows.end(); i++)
				{
					if ((*i).second == w)
					{
						fThreadWindows.erase(i);
						break;
					}
				}
				break;
			}
			
			case kMsgRun:
				if (fTeam->GetID() == -1)
				{
					FailOSErr(resume_thread(spawn_thread(launch, "launch", B_NORMAL_PRIORITY, fTeam)));
//					fTeam->Launch();
				}

//				ListFiles();
//				ListGlobals();
				break;
			
			case kMsgFilesChanged:
				ListFiles();
				break;
				
			case kMsgGlobalsChanged:
				ListGlobals();
				break;

			case kMsgElfImageCreated:
			case kMsgElfImageDeleted:
				ListFiles();
				ListGlobals();
//				break;	fall thru
			
			case kMsgBreakpointsChanged:
			{
				fSource->ReloadBreakpoints();
				map<thread_id,DThreadWindow*>::iterator i;
				for (i = fThreadWindows.begin(); i != fThreadWindows.end(); i++)
					(*i).second->PostMessage(kMsgBreakpointsChanged);
				fBreakpointWindow->PostMessage(kMsgBreakpointsChanged);
				break;
			}
			
			case kMsgShowBreakpoints:
			{
				BAutolock lock(fBreakpointWindow);

				if (fBreakpointWindow->IsHidden())
					fBreakpointWindow->Show();

				fBreakpointWindow->Activate();
				break;
			}
			
			case kMsgWatchpointsChanged:
				fWatchpointWindow->PostMessage(kMsgWatchpointsChanged);
				break;
			
			case kMsgShowWatchpoints:
			{
				BAutolock lock(fWatchpointWindow);

				if (fWatchpointWindow->IsHidden())
					fWatchpointWindow->Show();

				fWatchpointWindow->Activate();
				break;
			}
			
			case kMsgShowAddress:
			{
				ptr_t addr;
				FailOSErr(msg->FindInt32("address", (int32*)&addr));
				ShowAddress(addr);
				break;
			}
			
			case kMsgShowAddressCmd:
			{
				DShowAddress *dlog = DialogCreator<DShowAddress>::CreateDialog(this);
				if (dlog) dlog->SetTeam(fTeam);
				break;
			}
			
			case kMsgShowFunction:
			{
				const char *name;
				FailOSErr(msg->FindString("name", &name));
				ShowFunction(name);
				break;
			}
			
			case kMsgShowFunctionCmd:
			{
				DialogCreator<DShowFunction>::CreateDialog(this);
				break;
			}

			case kMsgBreakOnException:
			case kMsgBreakOnLoadAddon:
			case kMsgBreakOnSpawnThread:
			{
				BAutolock lock(fTeam);
				if (lock.IsLocked()) {
					switch (msg->what) {
						case kMsgBreakOnException:
							fTeam->SetBreakOnException(fTeam->ThrowPC() == 0);
							break;
							
						case kMsgBreakOnLoadAddon:
							fTeam->SetBreakOnLoadImage(fTeam->LoadAddOnPC() == 0);
							break;
							
						case kMsgBreakOnSpawnThread:
							fTeam->SetBreakOnThreadCreation(fTeam->SpawnThreadPC() == 0);
							break;
					}
				}
				break;
			}
						
			case kMsgWhereIs:
				DoWhereIs();
				break;
			
			default:
				DWindow::MessageReceived(msg);
				break;
		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}
	
	fButtonBar->SetEnabled(kMsgRun,				fTeam->GetID() == -1);
	fButtonBar->SetEnabled(kMsgDebugThread, fThreads->List()->CurrentSelection() > -1);
	fButtonBar->SetEnabled(kMsgKill,				fTeam->GetID() != -1);
} /* DTeamWindow::MessageReceived */

void DTeamWindow::ListThreads()
{
	DisableUpdates();

	// We can't look at the local machine for thread info; we have to go through the DNub

	// this may throw if the nub has not yet been attached, e.g. if we're launching another team
	// and it hasn't yet finished loading.  if so, we ignore the exception and proceed to fill out a
	// (blank) thread list.
	try
	{
		BAutolock lock(GetTeam().GetNub());

		// get the thread list from the nub
		thread_map threads;
		GetTeam().GetNub().GetThreadList(GetTeam().GetID(), threads);

		// remove any current items that aren't in the new thread list
		for (int32 i = 0; i < fThreads->List()->CountItems(); )
		{
			DItem<thread_id>* item = static_cast<DItem<thread_id>*>(fThreads->List()->ItemAt(i));
			thread_map::iterator ti = find(threads.begin(), threads.end(), item->fData);
			if (ti != threads.end())	// already in the list?
			{
				i++;						// advance to the next item in the listview
				threads.erase(ti);	// remove it from the list because we don't need to add it
			}
			else
			{
				// delete this 'stale' DItem from the list view, retaining iterator over the list view
				fThreads->List()->RemoveItem(i);
				delete item;
			}
		}

		// now any items still in the list correspond to threads not yet in the list view,
		// so we walk the 'threads' list and add them
		for (thread_map::iterator ti = threads.begin(); ti != threads.end(); ti++)
		{
			DItem<thread_id>* item = new DItem<thread_id>((*ti).name.c_str(), (*ti).id);
			fThreads->List()->AddItem(item);
		}
	}
	catch (...)
	{
		/* do nothing about the thread list; there's no nub yet */
	}

	// and update the globals
	if (GetTeam().GetID() >= 0)
		UpdateVariableValues();
	else
		ListGlobals(); // gets rid of them
		
	EnableUpdates();
} /* DTeamWindow::ListThreads */

BRect DTeamWindow::GetNextTeamRect()
{
	BRect r = gPrefs->GetPrefRect("teamwindowrect", BRect(120, 120, 400, 400)), rNext;
	
	rNext = r;
	rNext.OffsetBy(10, 10);

	if (rNext.Height() < 300) rNext.bottom = rNext.top + 300;
	if (rNext.Width() < 300) rNext.right = rNext.left + 300;
	
	BRect sr = BScreen().Frame();
	
	if (rNext.right > sr.right || rNext.bottom > sr.bottom)
		rNext.OffsetTo(100, 100);

	gPrefs->SetPrefRect("teamwindowrect", rNext);
	
	return r;
} /* DTeamWindow::GetNextTeamRect */

DTeamWindow* DTeamWindow::GetNextTeamWindow(uint32& cookie)
{
	DTeamWindow *next = NULL;
	
	if (sfWindows.size())
	{
		list<DTeamWindow*>::iterator wi;
		
		if (cookie == 0)
			wi = sfWindows.begin();
		else
			find(sfWindows.begin(), sfWindows.end(), (DTeamWindow*)cookie);
		
		if (wi != sfWindows.end())
		{
			next = *wi;
			++wi;
			if (wi != sfWindows.end())
				cookie = (uint32)*wi;
			else
				cookie = ~0L;
		}
	}
	
	return next;
} /* DTeamWindow::GetNextTeamWindow */

static int CompareItems(const void *a, const void *b)
{
	const DItem<DFileNr> *ia, *ib;
	ia = *static_cast<const DItem<DFileNr>* const *>(a);
	ib = *static_cast<const DItem<DFileNr>* const *>(b);
	
	return strcmp(DSourceFileTable::Instance()[ia->fData].name, DSourceFileTable::Instance()[ib->fData].name);
} // CompareItems

void DTeamWindow::ListFiles()
{
	vector<DFileNr> files;
	GetTeam().GetSymWorld().GetSourceFiles(files);

	while (fFiles->List()->CountItems())
		delete fFiles->List()->RemoveItem((int32)0);
	
	vector<DFileNr>::iterator fi;
	for (fi = files.begin(); fi != files.end(); fi++)
		fFiles->List()->AddItem(new DItem<DFileNr>(DSourceFileTable::Instance()[(*fi)].name, *fi));
	
	fFiles->List()->SortItems(CompareItems);

	UpdateIfNeeded();
} /* DTeamWindow::ListFiles */

typedef map<DVariable*, DVariableItem*> variablemap;

void DTeamWindow::ListGlobals()
{
	DisableUpdates();
	
	try
	{
		BAutolock lock(fTeam);
		if (!lock.IsLocked()) THROW(("Failed to lock team"));
		
		BOutlineListView& list = *fVariables->List();
	
		// create a map of the current items in our list (take only the level 0 items)
		
		variablemap currentMap;
			
		for (int ix = 0; ix < list.CountItems(); ix++)
		{
			DVariableItem *item = dynamic_cast<DVariableItem*>(list.ItemAt(ix));
			if (item && item->OutlineLevel() == 0)
				currentMap.insert(pair<DVariable*, DVariableItem*>(item->Variable(), item));
		}
	
		// now ask the symworld for a new set of global variables
		
		vector<pair<DVariable*, ptr_t> > globals;	
		GetTeam().GetSymWorld().GetGlobals(globals);
	
		// What we want to do is merge globals with what we currently have in our list
		// We will go through the globals and for each global
		// Look it up in our currentMap
		// if found: delete it from the map
		// if not found: add it to the item list
		// When we are done, the items left in the map are now obsolete and need 
		// to be deleted.
		
		for (vector<pair<DVariable*, ptr_t> >::const_iterator gi = globals.begin(); gi != globals.end(); gi++)
		{
			
			variablemap::iterator pair = currentMap.find((*gi).first);
			if (pair == currentMap.end())
			{
				// we have a new variable that needs to be added to our list
				DVariableItem* vItem = DVariableItem::CreateGlobalItem((*gi).first, (*gi).second, GetTeam().GetNub());
				if (vItem)
				{
					list.AddItem(vItem);
					if (vItem->Expandable())
					{
						vItem->SetExpanded(false);
						list.AddUnder(new BStringItem(""), vItem);
					}
				}
			}
			else 
			{
				// we already have this variable in our list
				// remove it from the map
				currentMap.erase(pair);
			}
		}
	
		// At the end of the loop, currentMap is left with items that
		// no longer are global variables, delete them from the list
		// and delete the item itself
		for (variablemap::iterator oi = currentMap.begin(); oi != currentMap.end(); oi++)
		{
			int32 ix = list.FullListIndexOf((*oi).second);
			list.RemoveItem(ix);
			// the DVariableItem is now orphaned... delete it
			// (since our container is going away at the end of method, don't
			// worry about erasing it from the container)
			delete (*oi).second;
		}
	}
	catch (HErr& err)
	{
		fVariables->List()->MakeEmpty();		// might contain garbage
		err.DoError();
	}

	EnableUpdates();
	UpdateIfNeeded();
} // DTeamWindow::ListGlobals

void DTeamWindow::ShowAddress(ptr_t addr)
{
	DStatement stmt;
	
	if (GetTeam().GetSymWorld().GetStatement(addr, stmt))
	{
		Activate();
		fSource->SetStatement(stmt, bpShowAddr);
	}
	else
		beep();
} // DTeamWindow::ShowAddress

void DTeamWindow::ShowFunction(const char* name)
{
	// First look up the function by name.
	// If we find it, use the pc to get the statement.
	// (It would appear easier to get the statement
	// based on the function name, but if the user types
	// a mangled name, this gives us the asm information
	// when we really want to source level.)
	// The function -> pc -> statement assures us that
	// I get the highest level information possible.
	
	try
	{
		DStatement stmt;
		ptr_t pc;
		int unusedSize;
		GetTeam().GetSymWorld().GetFunctionOffsetAndSize(name, pc, unusedSize);
		if (GetTeam().GetSymWorld().GetStatement(pc, stmt))
		{
			Activate();
			fSource->SetStatement(stmt, bpShowAddr);
		}
	}
	catch (...)
	{
		beep();
	}
} // DTeamWindow::ShowFunction

void DTeamWindow::MenusBeginning()
{
	DWindow::MenusBeginning();

	BMenuBar *mbar = static_cast<BMenuBar*>(FindView("mbar"));
	FailNil(mbar);

	BMenuItem *item = mbar->FindItem(kMsgBreakOnException);
	if (item)
		item->SetMarked(fTeam->ThrowPC() != 0);

	item = mbar->FindItem(kMsgBreakOnLoadAddon);
	if (item)
		item->SetMarked(fTeam->LoadAddOnPC() != 0);

	item = mbar->FindItem(kMsgBreakOnSpawnThread);
	if (item)
		item->SetMarked(fTeam->SpawnThreadPC() != 0);
	
	item = mbar->FindItem(kMsgRun);
	if (item)
		item->SetEnabled(fTeam->GetID() == -1);
} // DTeamWindow::MenusBeginning

void DTeamWindow::DoDebugThread()
{
	DItem<thread_id> *item = static_cast<DItem<thread_id>*>(fThreads->List()->ItemAt(fThreads->List()->CurrentSelection()));
	if (item)
	{
		DNub& nub = GetTeam().GetNub();
		BAutolock lock(nub);
		nub.StopThread(item->fData);
	}
} // DTeamWindow::DoDebugThread

void DTeamWindow::DoWhereIs()
{
	DItem<thread_id> *item = static_cast<DItem<thread_id>*>(fThreads->List()->ItemAt(fThreads->List()->CurrentSelection()));
	if (item)
	{
		DNub& nub = GetTeam().GetNub();
		BAutolock lock(nub);

		thread_info ti;
		nub.GetThreadInfo(item->fData, ti);
		
		if (ti.state <= B_THREAD_ASLEEP)
			HErr("You can't view the stack crawl of a running thread").DoError();
		else
		{
			DCpuState* cpu;
			nub.GetThreadRegisters(item->fData, cpu);
			
			DThread *t = GetTeam().GetThread(item->fData);
			DStackCrawl sc(*t);
			
			nub.GetStackCrawl(*cpu, sc);

			char buf[512];
			sprintf(buf, "Where Is: %s", t->Name().String());
			new DStackCrawlWindow(buf, *fTeam, sc);
		}
	}
} // DTeamWindow::DoWhereIs

DTeamWindow* DTeamWindow::GetTeamWindow(team_id id)
{
	DTeamWindow *w = NULL;
	
	if (sfWindows.size())
	{
		list<DTeamWindow*>::iterator wi;
		
		for (wi = sfWindows.begin(); w == NULL && wi != sfWindows.end(); wi++)
			if ((*wi)->GetTeam().GetID() == id)
				w = *wi;
	}
	
	return w;
} // DTeamWindow::GetTeamWindow

void DTeamWindow::FillWindowMenu(BMenu *menu)
{
	bool addedSeparator = false;
	if (sfWindows.size())
	{
		list<DTeamWindow*>::iterator wi;
		
		for (wi = sfWindows.begin(); wi != sfWindows.end(); wi++)
		{
			if (!addedSeparator) {
				menu->AddSeparatorItem();
				addedSeparator = true;
			}

			BMessage *msg;
			
			FailNil(msg = new BMessage(kMsgSelectWindow));
			msg->AddPointer("window", *wi);
			
			bool teamWindow = dynamic_cast<DTeamWindow *>(*wi) != 0;
			menu->AddItem(new BMenuItem((*wi)->Title(), msg, teamWindow ? 'T' : 0,
				teamWindow ? (B_COMMAND_KEY | B_OPTION_KEY) : 0));
			
			map<thread_id,DThreadWindow*>::iterator twi;
			
			for (twi = (*wi)->fThreadWindows.begin(); twi != (*wi)->fThreadWindows.end(); twi++)
			{
				FailNil(msg = new BMessage(kMsgSelectWindow));
				msg->AddPointer("window", (*twi).second);

				char s[256];
				strcpy(s, "  ");
				strcat(s, (*twi).second->Title());

				menu->AddItem(new BMenuItem(s, msg));
			}
		}
	}
} // DTeamWindow::FillWindowMenu
