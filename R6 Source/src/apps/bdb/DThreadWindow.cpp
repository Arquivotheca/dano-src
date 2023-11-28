/*	$Id: DThreadWindow.cpp,v 1.19 1999/05/03 13:09:58 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/09/98 19:33:05
*/

#include "bdb.h"
#include "DThreadWindow.h"
#include "DMessages.h"
#include "DRegsWindow.h"
#include "DTeamWindow.h"
#include "DDebugMessageWin.h"
#include "DStackCrawlView.h"
#include "DNub.h"
#include "DSourceView.h"
#include "DDebugApp.h"
#include "DVariable.h"
#include "DThread.h"
#include "DTeam.h"
#include "DStackFrame.h"
#include "DStackCrawl.h"
#include "DResizer.h"
#include "DListBox.h"
#include "DVariableItem.h"
#include "DFunction.h"
#include "DMemoryWindow.h"
#include "DUtils.h"
#include "DInternalProxy.h"

#include <MenuItem.h>
#include <StringView.h>
#include <ScrollView.h>
#include <OutlineListView.h>
#include <ListItem.h>
#include <String.h>
#include <File.h>
#include <Roster.h>
#include <Alert.h>

//#include <iostream>

class DoubleClickableStringView : public BStringView {
public:
	DoubleClickableStringView(BRect bounds, const char *name, 
		const char *text, uint32 resizeFlags, uint32 flags,
		BHandler *target, uint32 what)
		:	BStringView(bounds, name, text, resizeFlags, flags),
			target(target),
			what(what)
		{}

	virtual void MouseDown(BPoint)
		{
			BMessage *message = Window()->CurrentMessage();
			int32 clicks;
			if (message->FindInt32("clicks", &clicks) == B_OK
				&& clicks == 2)
				target->Looper()->PostMessage(what, target);
		}
private:
	BHandler *target;
	uint32 what;
};

struct VState
{
	bool expanded			: 1;
	EVarFormat format	: 7;
};

typedef map<ptr_t,vector<VState> > vstatesmap;

static vstatesmap sVStates;

DThreadWindow::DThreadWindow(DThread& thread)
	: DWindow(gPrefs->GetPrefRect("threadwindowrect", BRect(100, 100, 400, 400)),
		thread.Name().String(), B_DOCUMENT_WINDOW, 0)
	, fThread(thread)
{
	fChangingStackList = false;
	fTeam = &thread.GetTeam();
	fPanel = NULL;
	
	BView *V1, *V2;
	BView *H1, *H2;

	BRect b(Bounds()), r;

	fTeamWindow = dynamic_cast<DTeamWindow*>(fThread.GetTeam().GetOwner());
	ASSERT_OR_THROW (fTeamWindow);

	r = b;

	AddChild(fMenuBar = HResources::GetMenuBar(r, 1));
	FailNil(fMenuBar);
	fMenuBar->FindItem(kMsgQuit)->SetTarget(be_app);
	
	r.top = fMenuBar->Frame().bottom;
	r.bottom = r.top + min(150.0, r.Height() / 3.0);

	AddChild(V1 = new BView(r, "V1", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, 0));
	r.top = r.bottom + 1;
	r.bottom = b.bottom;
	AddChild(V2 = new BView(r, "V2", B_FOLLOW_ALL_SIDES, 0));

	AddChild(new DResizer(BRect(), "Resizer1", V1, V2));

	r = V1->Bounds();
	r.right = r.left + r.Width() / 3;
	V1->AddChild(H1 = new BView(r, "H1", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 0));
	r.left = r.right + 1;
	r.right = V1->Bounds().right + 1;
	V1->AddChild(H2 = new BView(r, "H2", B_FOLLOW_ALL_SIDES, 0));

	V1->AddChild(new DResizer(BRect(), "Resizer2", H1, H2));

	r = H1->Bounds();
	H1->AddChild(fSC = new DStackCrawlView(r, "StackCrawl", fThread.GetStackCrawl(),
		B_FOLLOW_ALL_SIDES));
	
	r = H2->Bounds();
	H2->AddChild(fVariables = new DListBox(r, "Variables"));
	fVariables->AddColumn("Variables", 120);
	fVariables->AddColumn("", 40);
	
	r = V2->Bounds();
	r.bottom = r.top + 23;
	
	BView *v;
	V2->AddChild(v = new BView(r, "toolbar", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW));
	v->SetViewColor(kViewColor);
	
	r.OffsetTo(0, 0);
	r.InsetBy(1, 1);
	
	v->AddChild(fButtonBar = new HButtonBar(r, "ButtonBar", 0, this));
	
	r = v->Bounds();
	r.left += fButtonBar->Bounds().Width() + 10;
	r.bottom = r.top + 14; // yeah I know, this should be some font_height calculation...
	BStringView *label;
	v->AddChild(label = new DoubleClickableStringView(r, "label", "", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW,
		this, kMsgFileOpenInPreferredEditor));
	
	r = V2->Bounds();
	r.top = v->Frame().bottom + 1;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	
	fSource = new DSourceView(r, "source", fThread.GetTeam(), label);
	
	V2->AddChild(new BScrollView("source scroll", fSource, B_FOLLOW_ALL_SIDES, 0, true, true, B_NO_BORDER));
	
	BRect f(Frame());
//	f.top = f.bottom + 20;
//	f.bottom = f.top + 20;
	f.left = f.right;
	f.right = f.left + 180;
	f.bottom = f.top + 200;
	
	BString regsWindowTitle = "Registers [";
	regsWindowTitle += this->Title();
	regsWindowTitle += "]";
	fRegs = new DRegsWindow(f, regsWindowTitle.String(), fThread, 2);
	static_cast<DRegsWindow*>(fRegs)->SetTarget(this);
	
	if (gPrefs->GetPrefInt("showregs", true))
		fRegs->Show();
	else
		fRegs->Run();

	fShowAsm = gPrefs->GetPrefInt("interleave src and asm", false);

	// initialize the register window for this thread
	BMessage m1(kMsgNewRegisterData);
	m1.AddPointer("registers", &fThread.GetCPU());
	fRegs->PostMessage(&m1);

	BView *sv = FindView("code view");
	FailNil(sv);
	fMenuBar->FindItem(kMsgFind)->SetTarget(sv);
	fMenuBar->FindItem(kMsgFindAgain)->SetTarget(sv);
	fMenuBar->FindItem(kMsgShowDebuggerMsg)->SetEnabled(false);

	AddShortcut('r', B_CONTROL_KEY, new BMessage(kMsgSwitchRun));
	AddShortcut('s', B_CONTROL_KEY, new BMessage(kMsgSwitchStep));
	AddShortcut('t', B_CONTROL_KEY, new BMessage(kMsgSwitchStepOver));
	AddShortcut('u', B_CONTROL_KEY, new BMessage(kMsgSwitchStepOut));
	Show();
	
// register ourselves
	BMessage m(kMsgThreadWindowCreated);
	m.AddInt32("thread", fThread.GetID());
	m.AddPointer("window", this);
	fTeamWindow->PostMessage(&m);

	// initial state - not displaying anything
	fCurrentFunctionLowPC = 0xFFFFFFFF;
} /* DThreadWindow::DThreadWindow */

bool DThreadWindow::QuitRequested()
{
	this->ClearVariables();
	if (fPanel)
	{
		delete fPanel;
		fPanel = NULL;
	}
	
	if (fRegs->Lock())
	{
		gPrefs->SetPrefInt("showregs", fShowRegs);
		fRegs->Quit();
	}
	
	gPrefs->SetPrefRect("threadwindowrect", Frame());
	
	BMessage m(kMsgThreadWindowClosed);
	m.AddPointer("window", this);
	FailMessageTimedOutOSErr(BMessenger(fTeamWindow).SendMessage(&m, (BHandler *)0, 1000));
	fThread.SetListener(NULL);
	
	return true;
} /* DThreadWindow::QuitRequested */

void DThreadWindow::MessageReceived(BMessage *msg)
{
	try
	{
		switch (msg->what)
		{
			case kMsgRun:
				DoRun(false);
				break;
			
			case kMsgSwitchRun:
				DoRun(true);
				break;
			
			case kMsgKill:
				DoKill();
				break;
			
			case kMsgStop:
				DoStop();
				break;
				
			case kMsgStep:
				DoStep(false);
				break;
			
			case kMsgSwitchStep:
				DoStep(true);
				break;
			
			case kMsgStepOver:
				DoStepOver(false);
				break;
			
			case kMsgSwitchStepOver:
				DoStepOver(true);
				break;
			
			case kMsgStepOut:
				DoStepOut(false);
				break;
			
			case kMsgSwitchStepOut:
				DoStepOut(true);
				break;
			
			case kMsgThreadStopped:
				ThreadStopped();
				break;

			case kMsgThreadDeleted:
				PostMessage(B_QUIT_REQUESTED);
				break;
			
			case kMsgSaveState:
				if (fPanel == NULL)
				{
					fPanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this));
					fPanel->Show();
				}
				else
					fPanel->Window()->Activate();
				break;


			case B_SAVE_REQUESTED:
			{
				entry_ref dir;
				const char *name;
				
				FailOSErr(msg->FindRef("directory", &dir));
				FailOSErr(msg->FindString("name", &name));
				SaveState(dir, name);
				break;
			}

			case kMsgRegisterModified:
			{
				// The DCpuState pointer we get here "belongs" to the DRegsWindow,
				// so we don't touch it.
				const DCpuState* newCpu;
				FailOSErr(msg->FindPointer("registers", (void**) &newCpu));
				fThread.SetCPU(*newCpu);
				break;
			}
			
			case kMsgShowRegisters:
			{
				BAutolock lock(fRegs);
				
				if (lock.IsLocked() && fRegs->IsHidden())
					fRegs->Show();
				fRegs->Activate();
				break;
			}
			
			case kMsgStackListSelection:
				if (!fChangingStackList)
				{
					StSetFlag flag(fChangingStackList);

					UpdateSource();
					UpdateVariables();
				}
				break;

			case kMsgBreakpointsChanged:
				fSource->ReloadBreakpoints();
				break;
			
			case kMsgFileOpenInPreferredEditor:
				PostMessage(msg, fSource);
				break;
	
			case kMsgShowBreakpoints:
			case kMsgShowWatchpoints:
				fTeamWindow->PostMessage(msg);
				break;
			
			case kMsgShowAssemblyCmd:
				this->ShowAssembly(!fShowAsm);
				break;
				
			case kMsgRefreshVariables:
				UpdateVariableValues();
				fVariables->List()->Invalidate();
				break;

			case kMsgShowDebuggerMsg:
				(new DDebugMessageWin(fDebuggerMsg))->Show();
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
	
	int state = fThread.GetState();
	
	fButtonBar->SetEnabled(kMsgRun,			state == tsSuspended);
	fButtonBar->SetEnabled(kMsgKill,		state != tsDeleted);
	fButtonBar->SetEnabled(kMsgStep,		state == tsSuspended);
	fButtonBar->SetEnabled(kMsgStepOver,	state == tsSuspended);
	fButtonBar->SetEnabled(kMsgStepOut,		state == tsSuspended);
	fButtonBar->SetEnabled(kMsgStop,		state == tsRunning);
} /* DThreadWindow::MessageReceived */

void 
DThreadWindow::MenusBeginning()
{
	DWindow::MenusBeginning();

	// now set the "checked" state of the "Show Assembly" menu item
	BMenuBar *mbar = static_cast<BMenuBar*>(FindView("mbar"));
	mbar->FindItem(kMsgShowAssemblyCmd)->SetMarked(fShowAsm);
}

void DThreadWindow::DoRun(bool switchWorkspace)
{
	BAutolock lock(fThread);

	if (lock.IsLocked())
	{
		fThread.DoRun(switchWorkspace);

		BAutolock lock(fRegs);
//		if (fShowRegs = ! fRegs->IsHidden())
//			fRegs->Hide();

		fSC->Clear();
		fSource->Clear();
	}
} /* DThreadWindow::DoRun */

void DThreadWindow::DoStep(bool switchWorkspace)
{
	BAutolock lock(fThread);
	
	if (lock.IsLocked())
	{
		fThread.DoStep(switchWorkspace);
		fSource->NoPC();

		BAutolock lock(fRegs);
//		if (fShowRegs = ! fRegs->IsHidden())
//			fRegs->Hide();
	}
} /* DThreadWindow::DoStep */

void DThreadWindow::DoStop()
{
	BAutolock lock(fThread);
	
	if (lock.IsLocked())
		fThread.DoStop();
} /* DThreadWindow::DoStop */

void DThreadWindow::DoStepOver(bool switchWorkspace)
{
	BAutolock lock(fThread);
	
	if (lock.IsLocked())
	{
		fThread.DoStepOver(switchWorkspace);
		fSource->NoPC();

		BAutolock lock(fRegs);
//		if (fShowRegs = ! fRegs->IsHidden())
//			fRegs->Hide();
	}
} /* DThreadWindow::DoStepOver */

void DThreadWindow::DoStepOut(bool switchWorkspace)
{
	BAutolock lock(fThread);
	
	if (lock.IsLocked())
	{
		fThread.DoStepOut(switchWorkspace);
		fSource->NoPC();

		BAutolock lock(fRegs);
//		if (fShowRegs = ! fRegs->IsHidden())
//			fRegs->Hide();
	}
} /* DThreadWindow::DoStepOut */

void DThreadWindow::DoKill()
{
	BAutolock lock(fThread);
	
	if (lock.IsLocked())
	{
		fThread.DoKill();
		fSource->NoPC();

		BAutolock lock(fRegs);
//		if (fShowRegs = ! fRegs->IsHidden())
//			fRegs->Hide();
	}
} /* DThreadWindow::DoKill */

const char *kWhyMsg[] = {
		NULL,
		"Debugger Call",
		NULL,
		NULL,
		"NMI Hit",
		"Machine Check Exception",
		"Segment Violation",
		"Alignment Exception",
		"Divide Error",
		"Overflow Exception",
		"Bounds Check Exception",
		"Invalid Opcode Exception",
		"Segment Not Present",
		"Stack Fault",
		"General Protection Fault",
		"Floating Point Exception",
		"Get Profiling Info",
		"Watchpoint Hit"
	};

void DThreadWindow::ThreadStopped()
{
//	{
//		BAutolock rLock(fRegs);
//		if (fShowRegs && fRegs->IsHidden())
//			fRegs->Show();
//	}
	
	if (fThread.HasDebugAction())
	{
		BAutolock lock(fThread);
		if (lock.IsLocked())
			fThread.DoDebugAction();
	}
	else
	{
		// pass the new register information along to the reg. window
		BMessage m(kMsgNewRegisterData);
		m.AddPointer("registers", &fThread.GetCPU());
		fRegs->PostMessage(&m);
		
		if (fThread.WhyStopped() != B_DEBUGGER_CALL || gPrefs->GetPrefInt("output to window", 0) == 0)
			Activate();
		
		fSC->Update();
		
		
		UpdateSource();
		UpdateVariables();

		TellMeWhy();
	}
} /* DThreadWindow::ThreadStopped */

void DThreadWindow::TellMeWhy()
{
	db_why_stopped why = fThread.WhyStopped();
	const char *s = kWhyMsg[why];
	BString buf;
	
	if (s)
	{
		if (why == B_DEBUGGER_CALL)
		{
			try
			{
				ptr_t src = (ptr_t)NULL;

				DNub& nub = fThread.GetTeam().GetNub();
				BAutolock lock2(nub);

#if __INTEL__
	// pointer to debug string for intel is located at ebp + 8
				BAutolock lock(fThread);
			
				if (lock.IsLocked())
				{
					DStackFrame& sf = fThread.GetStackCrawl().GetCurrentFrame();

					nub.Read(sf.GetFP() + 8, src);
				}
#else
#	error not implemented yet...
#endif
				if (src)
				{
					nub.ReadString(src, buf.LockBuffer(4097), 4096);
					buf.UnlockBuffer();
					s = buf.String();
				}

				printf("debugger: %s\n", s);
				fDebuggerMsg = s;
				fMenuBar->FindItem(kMsgShowDebuggerMsg)->SetEnabled(true);
				if (gPrefs->GetPrefInt("output to window", 0))
				{
					BAutolock lock(fTeam);
		
					if (lock.IsLocked())
					{
						fTeam->PrintToOutputWindow(s);
						DoRun(fTeam->SwitchWorkspace());
					}
					
					return;
				}
			}
			catch (...) { }
		}
		else if (why == B_WATCHPOINT_HIT)
		{
			ptr_t where = (ptr_t)fThread.ThreadStoppedData();
			char hexAddr[10];
			sprintf(hexAddr, "%lx", where);
			
			BAutolock lock(fTeam);
			const DWatchpoint *watchpoint = fTeam->FindWatchpoint(where);
			buf << s << ": ";
			if (watchpoint)
				buf << watchpoint->Name() << " at";
			buf << " address 0x" << hexAddr << " written.";
			s = buf.String();
		}
		(new BAlert("", s, "OK"))->Go();
	}
} /* DThreadWindow::TellMeWhy */

void DThreadWindow::UpdateSource()
{
	if (fSC->CountItems() > 0)
	{
		int32 ix = fSC->CurrentSelection();
		
		if (ix < 0)
		{
			StSetFlag flag(fChangingStackList);

			ix = fSC->CountItems() - 1;
			fSC->Select(ix);
			fSC->ScrollToSelection();
			fSource->Clear();
		}
		else
		{
			DStatement stmt;
			
			try
			{
				if (fThread.GetStackCrawl().GetNthStackFrame(ix).GetStatement(stmt))
					fSource->SetStatement(stmt, ix == fSC->CountItems() - 1);
				else
					fSource->Clear();
			}
			catch (...)
			{
				fSource->Clear();
			}
		}
	}
} /* DThreadWindow::UpdateSource */
	
void DThreadWindow::UpdateVariables()
{
	DisableUpdates();
	
	try
	{
		if (fSC->CountItems() > 0)
		{
			int32 ix = fSC->CurrentSelection();
			DStackFrame& frame = fThread.GetStackCrawl().GetNthStackFrame(ix);
			ptr_t pc = frame.GetPC();

			// look up the current function
			DFunction& f = fThread.GetTeam().GetSymWorld().GetFunction(pc);

			// is this a different function than we were previously displaying?
			if (f.LowPC() != fCurrentFunctionLowPC ||
				fCurrentStackIndx != ix)
			{
//				StListBuilder build;

				ClearVariables();
				
				varlist locals;
				f.BuildVariableList(locals);
				
				varlist::iterator vi;
				for (vi = locals.begin(); vi != locals.end(); vi++)
				{
					fVariables->List()->AddItem(*vi);
					if ((*vi)->Expandable())
					{
						fVariables->List()->AddItem(new BStringItem("", 1));
						fVariables->List()->Collapse(*vi);
					}
				}
				
				fCurrentFunctionLowPC = f.LowPC();
				fCurrentStackIndx = ix;
				
				vstatesmap::iterator vsi = sVStates.find(fCurrentFunctionLowPC);
				
				if (vsi != sVStates.end())
				{
					vector<VState>::iterator vi;
					int i;

					for (vi = (*vsi).second.begin(), i = 0; vi != (*vsi).second.end(); vi++, i++)
					{
						if ((*vi).expanded)
							fVariables->List()->Expand(fVariables->List()->ItemAt(i));
						
						DVariableItem *vItem = dynamic_cast<DVariableItem*>(fVariables->List()->ItemAt(i));
						if (vItem)
							vItem->SetFormat((*vi).format);
					}
				}
			}
			
			if (fChangingStackList)
			{
				for (int i = 0; i < fVariables->List()->FullListCountItems(); i++)
				{
					DVariableItem *vItem = dynamic_cast<DVariableItem*>(fVariables->List()->FullListItemAt(i));
					if (vItem)
					{
						bool changed = vItem->Changed();
						vItem->UpdateValue(frame);
						vItem->SetChanged(changed);
					}
				}
			}
			else
				UpdateVariableValues();
	
			fVariables->List()->Invalidate();
		}
		else if (fVariables->List()->CountItems())
			ClearVariables();
	}
	catch (...)
	{
		ClearVariables();
	}
	
	EnableUpdates();
} // DThreadWindow::UpdateVariables

DStackFrame* DThreadWindow::GetStackFrame()
{
	try
	{
		if (fSC->CurrentSelection() < 0)
			return NULL;
		else
			return &fThread.GetStackCrawl().GetNthStackFrame(fSC->CurrentSelection());
	}
	catch (...)
	{
		return NULL;
	}
} // DThreadWindow::GetCurrentFrame

void DThreadWindow::ClearVariables()
{
	if ((fCurrentFunctionLowPC != 0xFFFFFFFF) && fVariables->List()->CountItems())
	{
		try
		{
			vector<VState> vStates;
				
			for (int i = 0, m = fVariables->List()->CountItems(); i < m; i++)
			{
				VState s;
				DVariableItem *vItem = dynamic_cast<DVariableItem*>(fVariables->List()->ItemAt(i));
				
				if (vItem->IsCast())
				{
					uint32 level = vItem->OutlineLevel();
					
					BListItem *item;
					while ((item = fVariables->List()->ItemAt(i + 1)) != NULL &&
						item->OutlineLevel() > level)
					{
						i++;
					}
						
					continue;
				}
				else
				{
					s.expanded = fVariables->List()->ItemAt(i)->IsExpanded();
					s.format = vItem ? vItem->Format() : fmtDefault;
					
					vStates.push_back(s);
				}
			}
			
			sVStates[fCurrentFunctionLowPC] = vStates;
		}
		catch (...)
		{
		}
	}

	int32 cnt = fVariables->List()->FullListCountItems(), i;
	for (i = cnt - 1; i >= 0; i--)
		delete fVariables->List()->RemoveItem(i);
			
	fCurrentFunctionLowPC = 0xFFFFFFFF;
} // DThreadWindow::ClearVariables

void DThreadWindow::SaveState(const entry_ref& dir, const char *name)
{
	try
	{
		BAutolock lock(fThread);
		
		if (fThread.GetState() != tsSuspended)
			THROW(("Cannot save the state of a thread that is running or deleted."));
		
		BFile file;
		FailOSErr(BDirectory(&dir).CreateFile(name, &file));
		
		int fd;
		FailOSErr2(fd = file.Dup());
		FILE *f = fdopen(fd, "w+");
		
		if (f)
		{
			fprintf(f, "Log for thread %ld (%s) of team %ld (%s)\n", fThread.GetID(), fThread.Name().String(),
				 fTeam->GetID(), fTeam->GetRef().name);
			
			db_why_stopped why = fThread.WhyStopped();
			const char *w = kWhyMsg[why];
			if (w == NULL)
			{
				switch (why)
				{
#if __INTEL__
					case B_THREAD_NOT_RUNNING:	w = "Thread not running"; break;
					case B_BREAKPOINT_HIT:			w = "Breakpoint hit"; break;
					case B_SNGLSTP:						w = "Single step"; break;
					default:									w = "Unknown"; break;
#endif
				}
			}
			
			fprintf(f, "Reason why stopped: \"%s\"", w);
			
			fprintf(f, "\nRegisters:\n");
			
			struct SRegInfo
			{
				char regName[12];
				long regOffset;
				long regSize;
			} *regs;
			
			const void *p = HResources::GetResource('RegI', 0);
			long cnt = *(long *)p;
			regs = (SRegInfo *)((char *)p + sizeof(long));
			uchar *cpu = (uchar *)(&fThread.GetCPU());
			int i;
			
			for (i = 0; i < cnt; i++)
			{
				ulong x = 0;
				
				switch (regs[i].regSize)
				{
					case 1:	x = *((uint8 *)(cpu + regs[i].regOffset));
					case 2:	x = *((uint16 *)(cpu + regs[i].regOffset));
					case 4:	x = *((uint32 *)(cpu + regs[i].regOffset));
				}
				
				char name[32];
				strcpy(name, "               ");
				strncpy(name, regs[i].regName, min((int)strlen(regs[i].regName), 15));
				
				fprintf(f, "    %s: 0x%08lx\n", name, x);
			}
			
			fprintf(f, "\nStack Crawl:\n    pc          fp          procedure\n");
			
			DStackCrawl& sc = fThread.GetStackCrawl();
			
			for (i = sc.CountFrames() - 1; i >= 0; i--)
			{
				DStackFrame& frame = sc[i];
				string proc;
				
				frame.GetFunctionName(proc);
				fprintf(f, "    0x%08lx  0x%08lx  %s\n", frame.GetPC(), frame.GetFP(), proc.c_str());
			}
		}
		
		fclose(f);
		close(fd);
	}
	catch (HErr& e)
	{
		e.DoError();
	}
	
	delete fPanel;
	fPanel = NULL;
} // DThreadWindow::SaveState

void DThreadWindow::ShowAssembly(bool newShowState)
{
	fShowAsm = newShowState;
	gPrefs->SetPrefInt("interleave src and asm", fShowAsm);

	// !!! choice of file to display is currently handled indirectly by the DStatement calculation
	// of where the PC currently is.  In order to force the use of a different file, we have to blow
	// away the cached file token for the current statement.  Yes, this is a hack, but it'll take a
	// substantial redesign of the statement/source/asm file handling to make it cleaner, and
	// in the meantime this works.
	try
	{
		int32 ix = fSC->CurrentSelection();
		if (ix >= 0)
		{
			fThread.GetStackCrawl().GetNthStackFrame(ix).ResetStatement();
		}
	}
	catch (HErr& e) { /* ignore errors here */ }

	fSource->Clear();
	UpdateSource();
}
