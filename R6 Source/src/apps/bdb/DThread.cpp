/*	$Id: DThread.cpp,v 1.7 1999/05/03 13:09:58 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/14/98 14:57:06
*/

#include "bdb.h"
#include "DRuntimeSupport.h"
#include "DThread.h"
#include "DNub.h"
#include "DMessages.h"
#include "DTeam.h"
#include "DThreadWindow.h"
#include "DCpuState.h"

#include <Message.h>
#include <Messenger.h>
#include <Beep.h>

#include <iostream>

DThread::DThread(thread_id thread, DTeam& team)
	: fTeam(team)
	, fStackCrawl(*this), fThreadStoppedData(NULL)
{
	fThreadID = thread;
	fListener = NULL;
	fState = tsSuspended;
	fStateMachine = NULL;
	fLastAction = daNone;
	fCPU = NULL;
} /* DThread::DThread */

DThread::~DThread()
{
	delete fCPU;
	delete fStateMachine;

	// At present, fThreadStoppedData isn't actually used, nor is it
	// actually heap-based data, so we don't mess with it
} /* DThread::~DThread */

BString
DThread::Name() const
{
	BString name = fInfo.name;
	char buf[16];
	sprintf(buf, " [%d]", (int) fInfo.thread);
	name += buf;
	return name;
}

void DThread::Stopped(db_why_stopped why, DCpuState* cpu, void *threadStoppedData)
{
	if (fCPU) delete fCPU;
	fCPU = cpu;			// take posession; will delete

	fAction = daNone;
	fWhy = why;
	fState = tsSuspended;
	fThreadStoppedData = threadStoppedData;

	// if it's a deliberate stop...
	if (why <= B_SNGLSTP || why >= B_WATCHPOINT_HIT)
	{
		if (fTeam.GetSymWorld().IsDisabled(cpu->GetPC()))
			fAction = daStep;
		else
		{
			if (fStateMachine == NULL)
				fStateMachine = CheckRuntimeList(*this, fTeam.GetRuntimeList());
			
			if (fStateMachine)
			{
				fAction = fStateMachine->NextAction(*this);
				if (fStateMachine->IsDone())
				{
					delete fStateMachine;
					fStateMachine = NULL;
				}
			}
		}
	}
	else
	{
		delete fStateMachine;
		fStateMachine = NULL;
	}

	if (fAction == daNone)
		fStackCrawl.Update(*fCPU);

	if (! fListener)
	{
		DNub& nub = GetTeam().GetNub();
		BAutolock lock(nub);

		nub.GetThreadInfo(fThreadID, fInfo);
		fListener = new DThreadWindow(*this);
	}
	
	BMessage threadStopped(kMsgThreadStopped);
	FailMessageTimedOutOSErr(BMessenger(fListener).SendMessage(&threadStopped, (BHandler *)0, 1000));
} /* DThread::Stopped */

void DThread::Deleted()
{
	if (fListener)
		fListener->PostMessage(kMsgThreadDeleted);

	fState = tsDeleted;
	fThreadID = (thread_id) 0;
} /* DThread::Deleted */

void DThread::DoDebugAction()
{
	switch (fAction)
	{
		case daNone:			ASSERT(false);		break;
		case daStep:			DoStep(); 			break;
		case daStepOver:		DoStepOver();		break;
		case daStepOut:		DoStepOut();		break;
		case daRun:
			{
				BMessage run(kMsgRun);
				FailMessageTimedOutOSErr(BMessenger(fListener).SendMessage(&run, (BHandler *)0, 1000));
				break;
			}
	}
	fAction = daNone;
} // DThread::DoDebugAction

void DThread::DoRun(bool switchWorkspace)
{
	if (fState == tsRunning || fState == tsDeleted)
	{
		beep();
		return;
	}
	
	fTeam.SwitchWorkspaceIfNeeded(switchWorkspace);

	DNub& nub = fTeam.GetNub();
	BAutolock lock(nub);
	
	if (lock.IsLocked())
	{
		nub.Run(fThreadID, *fCPU);

		fState = tsRunning;
	}
	
	fLastAction = daRun;
} /* DThread::DoRun */

void DThread::DoStep(bool switchWorkspace)
{
	if (fState == tsRunning || fState == tsDeleted)
	{
		beep();
		return;
	}
	
	fTeam.SwitchWorkspaceIfNeeded(switchWorkspace);

	DNub& nub = fTeam.GetNub();
	BAutolock lock(nub);
	
	if (lock.IsLocked())
	{
		DStatement statement;
		fStackCrawl.GetCurrentFrame().GetStatement(statement);
		nub.Step(fThreadID, *fCPU, statement.fPC, statement.fPC + statement.fSize);

		fState = tsRunning;
	}
	
	fLastAction = daStep;
} /* DThread::DoStep */

void DThread::DoStepOver(bool switchWorkspace)
{
	if (fState == tsRunning || fState == tsDeleted)
	{
		beep();
		return;
	}
	
	fTeam.SwitchWorkspaceIfNeeded(switchWorkspace);

	DNub& nub = fTeam.GetNub();
	BAutolock lock(nub);
	
	if (lock.IsLocked())
	{
		DStatement statement;
		fStackCrawl.GetCurrentFrame().GetStatement(statement);

		nub.StepOver(fThreadID, *fCPU, statement.fPC,
			(statement.fSize > 0) ? statement.fPC + statement.fSize : statement.fPC);
		fState = tsRunning;
	}
	
	fLastAction = daStepOver;
} /* DThread::DoStepOver */

void DThread::DoStepOut(bool switchWorkspace)
{
	if (fState == tsRunning || fState == tsDeleted)
	{
		beep();
		return;
	}
	
	fTeam.SwitchWorkspaceIfNeeded(switchWorkspace);

	DNub& nub = fTeam.GetNub();
	BAutolock lock(nub);
	
	if (lock.IsLocked())
	{
		nub.StepOut(fThreadID, *fCPU);
		fState = tsRunning;
	}
	
	fLastAction = daStepOut;
} /* DThread::DoStepOut */

void DThread::DoKill()
{
	if (fState == tsDeleted)
	{
		beep();
		return;
	}
	
	// instruct the nub proxy (possibly remote) to kill the thread
	{
		DNub& nub = GetTeam().GetNub();
		BAutolock nubLock(nub);
		nub.KillThread(fThreadID);
	}

	if (fState == tsSuspended)	// to make the thread go away, it is blocked now
		DoStep();

	fLastAction = daNone;
} /* DThread::DoKill */

void DThread::DoStop()
{
	if (fState != tsRunning)
	{
		beep();
		return;
	}
	
	// instruct the nub proxy (possibly remote) to stop the thread
	{
		DNub& nub = GetTeam().GetNub();
		BAutolock nubLock(nub);
		nub.StopThread(fThreadID);
	}

	fLastAction = daNone;
} /* DThread::DoStop */

void DThread::SetListener(BLooper *looper)
{
	fListener = looper;
	
	BWindow *w = dynamic_cast<BWindow*>(looper);
	if (w)
	{
		DNub& nub = GetTeam().GetNub();
		BAutolock nubLock(nub);
		BAutolock winLock(w);

		thread_info ti;
		nub.GetThreadInfo(fThreadID, ti);
		
		w->SetTitle(ti.name);
	}
	
	if (fListener == NULL && fThreadID == 0)
		delete this;
} /* DThread::SetListener */

void DThread::SetCPU(const DCpuState& newCPU, bool tellListeners)
{
	fCPU->Assign(newCPU);
	fStackCrawl.Update(*fCPU);
	
	// If the first call to DThread::Stopped causes us to handle some runtime support
	// functions, we can get to here without a listener setup yet.
	if (tellListeners && fListener) {
		BMessage refresh(kMsgRefreshVariables);
		FailMessageTimedOutOSErr(BMessenger(fListener).SendMessage(&refresh, (BHandler *)0, 1000));
	}
} // DThread::SetCPU
