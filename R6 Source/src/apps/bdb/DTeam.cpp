/*	$Id: DTeam.cpp,v 1.20 1999/05/11 21:31:08 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/13/98 09:59:10
*/

#include "bdb.h"
#include "DTeam.h"
#include "DNub.h"
#if __INTEL__
#	include "DLocalx86Nub.h"
#	include "DRemoteProxyNub.h"
#endif
#include "DThread.h"
#include "DMessages.h"
#include "DCpuState.h"
#include "DTeamWindow.h"
#include "DExprParser.h"
#include "DVariable.h"
#include "DCpuState.h"

#include <Messenger.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Alert.h>

#include <fcntl.h>
#include <strstream>

class P_bp_with_pc
{
  public:
	P_bp_with_pc(ptr_t pc) : fPC(pc) {}
	bool operator() (const DBreakpoint& bp) { return bp.PC() == fPC; }
	ptr_t fPC;
};

class P_ap_with_name
{
  public:
	P_ap_with_name(const char *name) : fName(name) {}
	bool operator() (const DTracepoint& ap) { return strcmp(ap.Name(), fName) == 0; }
	const char *fName;
};

class P_ap_with_addr
{
  public:
	P_ap_with_addr(ptr_t addr) : fAddr(addr) {}
	bool operator() (const DWatchpoint& wp) { return wp.Address() == fAddr; }
	ptr_t fAddr;
};

DTeam::DTeam(entry_ref& ref, BLooper *owner, int argc, char **argv)
	  : fSymWorld(this)
{
	fOwner = owner;
	fRef = ref;
	fNub = NULL;
	fTeamID = -1;
	fThrowPC = 0;
	fLoadAddonPC = 0;
	fSpawnThreadPC = 0;
	fOutputPipe = -1;
	fOutputThread = -1;
	
	if ((fArgc = argc) > 1)
	{
		fArgv = new (char*)[argc + 1];
		FailNil(fArgv);
		
		for (int c = 0; c < argc; c++)
			fArgv[c] = strdup(argv[c]);
		fArgv[argc] = NULL;
	}
	
	ReadBreakpoints();
} /* DTeam::DTeam */

DTeam::DTeam(team_id id, port_id port, BLooper *owner)
	  : fSymWorld(this)
{
	fOwner = owner;
	fNub = NULL;
	fTeamID = id;
	fThrowPC = 0;
	fLoadAddonPC = 0;
	fSpawnThreadPC = 0;
	fArgv = NULL;
	fArgc = 1;
	fOutputPipe = -1;
	fOutputThread = -1;
	
	image_info info;
	long cookie = 0;

	bool foundTeam = false;
	while (get_next_image_info(fTeamID, &cookie, &info) == B_OK)
	{
		if (info.type == B_APP_IMAGE)
		{
			FailOSErr(get_ref_for_path(info.name, &fRef));
			foundTeam = true;
			break;
		}
	}

	// were we pointed at a nonexistent team?
	if (!foundTeam)
	{
		THROW(("Invalid team_id %ld given", id));
	}

	Connect(port);

	ReadBreakpoints();
} /* DTeam::DTeam */

DTeam::DTeam(const char* host, uint16 port, BLooper *owner)
	  : fSymWorld(this)
{
	fOwner = owner;
	fNub = NULL;
	fTeamID = -2;		// -1 means launch, -2 means remote
	fThrowPC = 0;
	fLoadAddonPC = 0;
	fSpawnThreadPC = 0;
	fArgv = NULL;
	fArgc = 1;
	fOutputPipe = -1;
	fOutputThread = -1;
	
	Connect(host, port);

	ReadBreakpoints();
} /* DTeam::DTeam */

DTeam::~DTeam()
{
	if (fNub)
	{
		BAutolock lock(fNub);
		fNub->DTeamClosed();
	}
	
	if (fOutputPipe >= 0)
		close(fOutputPipe);

	WriteBreakpoints();
} /* DTeam::DTeam */

void
AddEnv(char **&env, bool &envUnchanged, const char *newEntry)
{
	int i = 0;

	while (env[i])
		i++;
	
	char **oldEnv = env;
	env = new (char *)[i + 2];

	for (i = 0; oldEnv[i]; i++)
		env[i] = oldEnv[i];

	env[i++] = const_cast<char *>(newEntry);
	env[i] = NULL;

	if (!envUnchanged)
		delete [] oldEnv;

	envUnchanged = false;
}

void DTeam::Launch()
{
	BPath p;

	FailOSErr(BEntry(&fRef).GetPath(&p));
	
	char **argv, **env = environ;
	int argc;

	if (fArgc > 1)
	{
		argc = fArgc;
		argv = fArgv;
	}
	else
	{
		argv = new (char*)[2];
		argv[1] = NULL;
		argc = 1;
	}

	argv[0] = (char *)p.Path();

	
	bool envUnchanged = true;
	BString ms;

	int malloc_debug_level = gPrefs->GetPrefInt("malloc debug", 0);
	if (malloc_debug_level) {
		ms << "MALLOC_DEBUG=" << malloc_debug_level;
		AddEnv(env, envUnchanged, ms.String());
	}

	BString leakCheckString1;
	BString leakCheckString2;
	bool leakCheck = gPrefs->GetPrefInt("leak check", 0);

	if (leakCheck) {
		leakCheckString1 << "MALLOC_LEAK_CHECK=true";
		AddEnv(env, envUnchanged, leakCheckString1.String());
		leakCheckString2 << "NEW_LEAK_CHECK=true";
		AddEnv(env, envUnchanged, leakCheckString2.String());
	}

	thread_id thr = load_image(argc, (const char **)argv, (const char**)env);
	
	if (fArgc == 0) delete[] argv;
	if (!envUnchanged) delete[] env;
	
	FailOSErr2(thr);
	
	thread_info thi;
	FailOSErr(get_thread_info(thr, &thi));
	
	fTeamID = thi.team;
	
	Connect();

	ptr_t entryOffset = 0;
	
	if (gPrefs->GetPrefInt("stop at main", 1))
	{
		try
		{
			int entrySize;
			fSymWorld.GetFunctionOffsetAndSize("main", entryOffset, entrySize);
		}
		catch (HErr& e)
		{
			entryOffset = 0;
			(new BAlert("", "Could not set breakpoint at entrypoint", "OK", NULL, NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		}
	}


	DNub& nub = GetNub();
	BAutolock lock(nub);
	
	if (entryOffset)
		nub.SetBreakpoint(entryOffset);

	FailOSErr(resume_thread(thr));
	
	fOwner->PostMessage(kMsgElfImageCreated);
} // DTeam::Launch

void DTeam::Connect(port_id port)
{
	CreateRuntimeList(*this, fRT);

	port_id nubIn, nubOut;	
	if (port < 0)
	{
		FailOSErr2Msg(nubIn = create_port(5, "debug port"), "Failed to create debugger port");
		FailOSErr2(nubOut = install_team_debugger(fTeamID, nubIn));
	}
	else
	{
		// trigger the nub to set up
		// the nubOut at first read of nubIn
		nubIn = port;
		nubOut = -1;
	}
	
#if __INTEL__
	fNub = new DLocalx86Nub(nubOut, nubIn, this);
#else
#	error "Not supported on non-x86 architectures!"
#endif

	// load symbols before setting up the nub when local, to avoid racing
	// with the nub's processing of the initial THREAD_STOPPED message
	fSymWorld.LoadSymbols(fTeamID, fOwner);

	// Start the nub listening to the app...
	fNub->Start();

	// initialize the debug environment
	InitAfterConnect();
} // DTeam::Connect

void 
DTeam::Connect(const char *host, uint16 port)
{
	// attach to the remote proxy
#if __INTEL__
	fNub = new DRemoteProxyNub(host, port, this);
#else
#	error "Not supported on non-x86 architectures!"
#endif
	fNub->Start();		// attaches to the proxy machine

	InitAfterConnect();
}

void 
DTeam::InitAfterConnect()
{
	DNub& nub = GetNub();
	BAutolock lock(nub);

	if (gPrefs->GetPrefInt("break on c++ exception", 1))
		SetBreakOnException(true);
	
	if (gPrefs->GetPrefInt("stop on load_image", 0))
		SetBreakOnLoadImage(true);

	if (gPrefs->GetPrefInt("stop on spawn_thread", 0))
		SetBreakOnThreadCreation(true);
		
	BreakpointList::iterator bpi;
	for (bpi = fBreakpoints.begin(); bpi != fBreakpoints.end(); bpi++)
	{
		try
		{
			ptr_t pc = (*bpi).Revive(*this);
			if (pc != 0 && (*bpi).Kind() != ebDisabled)
				nub.SetBreakpoint(pc);
		}
		catch (...)
		{
			// bpi will be incremented in for loop, but also
			// needs to be incremented prior to erase, so
			// then move it back after
			BreakpointList::iterator t = bpi++;
			fBreakpoints.erase(t);
			bpi--;
		}
	}
	
	// Watchpoints are not reset on rerun...
	// Besides potentially being in add on's (that we can handle here and in ImageCreated)
	// They can also be on dynamic memory.  There is no way I can tell when I could re-enable
	// them in that case.  If we want to handle those on global data, they need to be handled
	// here, in ImageCreated and the deletion needs to be removed in TeamDeleted.
	
	fOwner->PostMessage(kMsgBreakpointsChanged);
} // DTeam::Connect

DTeam* DTeam::CreateTeam(entry_ref& ref, BLooper *owner, int argc = 0, char **argv = NULL)
{
	return new DTeam(ref, owner, argc, argv);
} /* DTeam::CreateTeam */

DTeam* DTeam::CreateTeam(team_id id, port_id port, BLooper *owner)
{
	return new DTeam(id, port, owner);
} // DTeam::CreateTeam

DTeam* DTeam::CreateTeam(const char* host, uint16 port, BLooper* owner)
{
	return new DTeam(host, port, owner);
}

DThread* DTeam::GetThread(thread_id thr)
{
	if (fThreads.count(thr) == 0)	// might happen if we start debugging a team
		ThreadCreated(thr);			// after it has been launched
	
	return fThreads[thr];
} /* DTeam::GetThread */

void DTeam::TeamDeleted()
{
	fNub = NULL;
	fTeamID = -1;

	BreakpointList::iterator bp;
	for (bp = fBreakpoints.begin(); bp != fBreakpoints.end(); bp++)
		(*bp).SetKind((*bp).Kind() == ebDisabled ? ebSavedDisabled : ebSaved);
	
	// We don't save watchpoints between runs (see comments in DTeam::Connect)
	// (Don't need to call nub.ClearWatchpoint because the team is gone anyway)
	fWatchpoints.erase(fWatchpoints.begin(), fWatchpoints.end());
	
	fSymWorld.TeamDeleted();
	
	fOwner->PostMessage(kMsgWatchpointsChanged);
	fOwner->PostMessage(kMsgBreakpointsChanged);
} /* DTeam::TeamDeleted */

void DTeam::ThreadCreated(thread_id thr)
{
	if (fThreads.count(thr))
		THROW(("How the hell can this happen, the newly created thread already existed!"));
	
	fThreads[thr] = new DThread(thr, *this);
} /* DTeam::ThreadCreated */

void DTeam::ThreadDeleted(thread_id thr)
{
	if (fThreads.count(thr))
	{
		DThread *thread = fThreads[thr];
		fThreads.erase(thr);
		
		BAutolock lock(thread);
		if (lock.IsLocked())
			thread->Deleted();
	}
} /* DTeam::ThreadDeleted */

void DTeam::ThreadStopped(thread_id thr, db_why_stopped why, DCpuState* cpu, void *threadStoppedData)
{
	std::map<thread_id, DThread*>::iterator i;

	i = fThreads.find(thr);
	if (i == fThreads.end())
	{
		ThreadCreated(thr);
		
		i = fThreads.find(thr);
		
		if (i == fThreads.end())
			THROW(("Could not handle stopped thread"));
	}

	if (fSwitchWorkspace)
		activate_workspace(fWorkspace);

	DThread *thread = (*i).second;
	
	BAutolock lock(thread);
	if (lock.IsLocked())
	{
		if (why == B_BREAKPOINT_HIT)
		{
			BreakpointList::iterator i = find_if(fBreakpoints.begin(), fBreakpoints.end(), P_bp_with_pc(cpu->GetPC()));
			if (i != fBreakpoints.end())
			{
				// before we evaluate conditional breakpoints etc. we have to ensure that the
				// thread is pointing to the correct stack frame etc.  We pass 'false' to prevent
				// the thread from updating the variable display every time we touch-and-go
				// over a disabled or otherwise inactive breakpoint.
				thread->SetCPU(*cpu, false);

				bool doStop = this->IsStopPoint(*i, *thread);
				if (doStop)
				{
					this->DoStopAction(*i, *thread, *cpu);
				}
				else
				{
					// we hit a breakpoint, but were told not to stop here
					// (like the condition is false)
					DNub& nub(GetNub());
					BAutolock lock(nub);
					nub.Run(thread->GetID(), *cpu);
					return;	// this is not the thread that set the breakpoint
				}
			}
		}

		fOwner->PostMessage(kMsgBreakpointsChanged);
		thread->Stopped(why, cpu, threadStoppedData);
	}
	else
	{
		// Unable to hand posession of the new DCpuState to the DThread,
		// so we have to take care of deleting it ourselves.
		delete cpu;
	}
} /* DTeam::ThreadStopped */

bool
DTeam::IsStopPoint(DBreakpoint& bp, DThread& thread)
{
	// Returns true if we really should stop

	// SingleShot:	check if correct thread
	// Conditional:	evaluate condition

	// Notice that if there is a conditional + counted, then
	// each time the condition is true, the count will be incremented
	// so we will stop the Nth time the breakpoint is hit with
	// that condition being true

	// Check one shot breakpoints - they don't get counts or conditions
	if (bp.Kind() == ebOneTime)
	{
		if (bp.ThreadID() == thread.GetID())
			return true;
		else
			return false;
	}
	
	// Allow conditional and counted to work together
	// So after checking condition, if we are to stop here
	// decrement and check the count
	bool stop = true;
	if (bp.GetCondition() != DBreakpoint::kAlwaysTrue) 
	{
		stop = this->EvaluateBreakpointCondition(bp, thread);
	}

	// If the condition is true, then increment the hit count on this breakpoint.
	// This returns a boolean saying whether we're actually supposed to stop
	// based on the breakpoint's skip count.  Note that the hit count tracks the
	// number of times we *would* stop, based on conditional evaluation/
	if (stop)
	{
		stop = bp.Hit();
	}

	return stop;	
}

void
DTeam::DoStopAction(DBreakpoint& bp, DThread& /*thread*/, DCpuState& cpu)
{
	// Do any actions associated with this breakpoint
	// SingleShot:	clear breakpoint
	// ActionPoint:	do action [not yet implemented]
	
	if (bp.Kind() == ebOneTime)
	{
		this->ClearBreakpoint(cpu.GetPC());
	}
}

bool
DTeam::EvaluateBreakpointCondition(DBreakpoint& bp, DThread& thread)
{
	DStackFrame& frame = thread.GetStackCrawl().GetCurrentFrame();
	bool result = true;
	DVariable* expression = NULL;
	try 
	{
		expression = DExprParser(bp.GetCondition().String()).Parse(frame);
		uint32 size = sizeof(int32);
		int32 value = 0;
		expression->GetValue(frame, (void*) &value, size);
		result = (value != 0);
	}
	catch (HErr& e)
	{
		BString moreText = (char*) e;
		moreText += " in the breakpoint condition.  Stopping at breakpoint.";
		HErr betterError(moreText.String());
		betterError.DoError();
		result = true;
	}

	// if the DExprParser.Parse() threw, then 'expression' will still be NULL, and
	// safe to delete.  Whether or not GetValue() threw, we'll still need to delete
	// the allocated DVariable before returning.
	delete expression;
	return result;
}

void 
DTeam::SwitchWorkspaceIfNeeded(bool switchWorkspace)
{
	fSwitchWorkspace = switchWorkspace;
	if (switchWorkspace) {
		fWorkspace = current_workspace();
		int32 workspace = fWorkspace + 1;
		if (workspace >= count_workspaces())
			workspace = 0;
		activate_workspace(workspace);
	}
}


void DTeam::ImageCreated(const image_info& info)
{
	fSymWorld.ImageCreated(info, fOwner);

	BreakpointList::iterator bp;
	for (bp = fBreakpoints.begin(); bp != fBreakpoints.end(); bp++)
	{
		if ((*bp).Kind() < ebSaved)
			continue;

		ptr_t pc;

		try
		{
			pc = (*bp).Revive(*this);
		}
		catch (...)
		{
			// bp will be incremented in for loop, but also
			// needs to be incremented prior to erase, so
			// then move it back after
			BreakpointList::iterator i = bp++;
			fBreakpoints.erase(i);
			bp--;
			continue;
		}

		if (pc == 0)
			continue;

		if (pc < (ptr_t)info.text || pc >= (ptr_t)info.text + info.text_size
			|| (*bp).Kind() == ebDisabled)
			continue;

		try
		{
			DNub& nub = GetNub();
			BAutolock lock(nub);
			
			nub.SetBreakpoint(pc);
		}
		catch (...)
		{
			// bogus breakpoint, delete it
			// bp will be incremented in for loop, but also
			// needs to be incremented prior to erase, so
			// then move it back after
			BreakpointList::iterator t = bp++;
			fBreakpoints.erase(t);
			bp--;
		}
	}
	
	CreateRuntimeList(*this, fRT);
	
	BMessage imageCreated(kMsgElfImageCreated);
	FailMessageTimedOutOSErr(BMessenger(fOwner).SendMessage(&imageCreated, (BHandler *)0, 1000));
} /* DTeam::ImageCreated */

void DTeam::ImageDeleted(const image_info& info)
{
	fSymWorld.ImageDeleted(info);
	
	BreakpointList::iterator bp;
	for (bp = fBreakpoints.begin(); bp != fBreakpoints.end(); bp++)
	{
		if ((*bp).Kind() != ebSaved &&
			(*bp).PC() >= (ptr_t)info.text && (*bp).PC() < (ptr_t)info.text + info.text_size)
		{
			(*bp).SetKind((*bp).Kind() == ebDisabled ? ebSavedDisabled : ebSaved);
		}
	}
	
	BMessage imageCreated(kMsgElfImageDeleted);
	FailMessageTimedOutOSErr(BMessenger(fOwner).SendMessage(&imageCreated, (BHandler *)0, 1000));
} /* DTeam::ImageDeleted */

void DTeam::GetStopsForFile(DFileNr file, std::set<int>& stops, std::set<int>& breakpoints)
{
	stops.clear();
	breakpoints.clear();

	fSymWorld.GetStopsForFile(file, stops);
	
	BreakpointList::iterator bp;
	
	for (bp = fBreakpoints.begin(); bp != fBreakpoints.end(); bp++)
	{
		if ((*bp).File() == file && (*bp).Kind() < ebSaved && (*bp).Kind() != ebDisabled)
			breakpoints.insert((*bp).Line());
	}
} /* DTeam::GetStopsForFile */

void DTeam::DoKill()
{
	if (fNub)
	{
		BAutolock lock(fNub);

		fNub->Kill();
		fNub = NULL;
	}
} /* DTeam::DoKill */

void DTeam::Detach ()
{
	if (fNub)
	{
		BAutolock lock(fNub);

		BreakpointList::iterator bpi;
		for (bpi = fBreakpoints.begin(); bpi != fBreakpoints.end(); bpi++)
		{
			if ((*bpi).Kind() < ebSaved)
				fNub->ClearBreakpoint((*bpi).PC());
		}

		fNub->Detach();
		fNub = NULL;
	}
} // DTeam::Detach

void DTeam::SetBreakpoint(ptr_t pc, thread_id oneTimeThread)
{
	DNub& nub = GetNub();	// keep using this construction in stead of *fNub, to force an ASSERT
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		nub.SetBreakpoint(pc);
		
		DStatement statement;
		
		fSymWorld.GetStatement(pc, statement);
		fBreakpoints.push_back(DBreakpoint(*this, statement.fFile, statement.fLine, pc,
			oneTimeThread >= 0 ? ebOneTime : ebAlways, statement.fSourceLevel, oneTimeThread));
		
		fOwner->PostMessage(kMsgBreakpointsChanged);
	}
} /* DTeam::SetBreakpoint */

void DTeam::ClearBreakpoint(ptr_t pc)
{
	DNub& nub = GetNub();	// keep using this construction in stead of *fNub, to force an ASSERT
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		BreakpointList::iterator i = find_if(fBreakpoints.begin(), fBreakpoints.end(), P_bp_with_pc(pc));

		if (i != fBreakpoints.end())
		{
			if ((*i).Kind() < ebSaved && (*i).Kind() != ebDisabled)
				nub.ClearBreakpoint(pc);
		
			fBreakpoints.erase(i);
			
			fOwner->PostMessage(kMsgBreakpointsChanged);
		}
	}
} /* DTeam::ClearBreakpoint */

void DTeam::ClearAllBreakpoints()
{
	DNub& nub = GetNub();
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		BreakpointList::iterator bp;
		
		for (bp = fBreakpoints.begin(); bp != fBreakpoints.end(); bp++)
		{
			if ((*bp).Kind() < ebSaved && (*bp).Kind() != ebDisabled)
				nub.ClearBreakpoint((*bp).PC());
		}
		
		fBreakpoints.clear();
		
		fOwner->PostMessage(kMsgBreakpointsChanged);
	}
} /* DTeam::ClearAllBreakpoints */

void DTeam::EnableBreakpoint(ptr_t pc)
{
	DNub& nub = GetNub();	// keep using this construction in stead of *fNub, to force an ASSERT
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		nub.SetBreakpoint(pc);
		
		BreakpointList::iterator i = find_if(fBreakpoints.begin(), fBreakpoints.end(), P_bp_with_pc(pc));
		if (i != fBreakpoints.end())
			(*i).SetKind(ebAlways);
		
		fOwner->PostMessage(kMsgBreakpointsChanged);
	}
} // DTeam::EnableBreakpoint

void DTeam::DisableBreakpoint(ptr_t pc)
{
	DNub& nub = GetNub();	// keep using this construction in stead of *fNub, to force an ASSERT
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		nub.ClearBreakpoint(pc);
		
		BreakpointList::iterator i = find_if(fBreakpoints.begin(), fBreakpoints.end(), P_bp_with_pc(pc));
		if (i != fBreakpoints.end())
			(*i).SetKind(ebDisabled);
		
		fOwner->PostMessage(kMsgBreakpointsChanged);
	}
} // DTeam::DisableBreakpoint

void DTeam::SetBreakpointCondition(ptr_t pc, const char* condition)
{
	DNub& nub = GetNub();	// keep using this construction in stead of *fNub, to force an ASSERT
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		BreakpointList::iterator i = find_if(fBreakpoints.begin(), fBreakpoints.end(), P_bp_with_pc(pc));
		if (i != fBreakpoints.end())
			(*i).SetCondition(condition);
		
		fOwner->PostMessage(kMsgBreakpointsChanged);
	}
} // DTeam::SetBreakpointCondition

void 
DTeam::SetBreakpointSkipCount(ptr_t pc, unsigned long skipCount)
{
	DNub& nub = GetNub();	// keep using this construction in stead of *fNub, to force an ASSERT
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		BreakpointList::iterator i = find_if(fBreakpoints.begin(), fBreakpoints.end(), P_bp_with_pc(pc));
		if (i != fBreakpoints.end())
			(*i).SetSkipCount(skipCount);
		
		fOwner->PostMessage(kMsgBreakpointsChanged);
	}
}

void 
DTeam::SetBreakpointHitCount(ptr_t pc, unsigned long hitCount)
{
	DNub& nub = GetNub();	// keep using this construction in stead of *fNub, to force an ASSERT
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		BreakpointList::iterator i = find_if(fBreakpoints.begin(), fBreakpoints.end(), P_bp_with_pc(pc));
		if (i != fBreakpoints.end())
			(*i).SetHitCount(hitCount);
		
		fOwner->PostMessage(kMsgBreakpointsChanged);
	}
}

const uint32 kLowTwoBits = 0x03;
const uint32 kMaxWatchpoints = 3;

void DTeam::SetWatchpoint(const char *name, uint32 kind, ptr_t pc)
{
	// Make sure we haven't hit our maximum already...
	
	WatchpointList::const_iterator i;
	uint32 enabledCount = 0;
	for (i = fWatchpoints.begin(); i != fWatchpoints.end(); i++)
	{
		if ((*i).Disabled() == false)
			enabledCount += 1;
	}
	
	if (enabledCount >= kMaxWatchpoints)
	{
		(new BAlert("", "A maximum of three enabled watchpoints are allowed.  Please remove or "
			"disable an existing watchpoint before adding another.", "OK"))->Go();
		DTeamWindow::GetTeamWindow(fTeamID)->PostMessage(kMsgShowWatchpoints);
		return;
	}
	
	// Make sure the watchpoint is long word aligned
	if ((pc & kLowTwoBits) != 0)
	{
		char addressBuffer[16];
		char normalizedAddressBuffer[16];
		sprintf(addressBuffer, "%lx", pc);
		sprintf(normalizedAddressBuffer, "%lx", pc & ~kLowTwoBits);

		BString message("The address: ");
		message += addressBuffer;
		message += " is not long word aligned.  Would you like to set the watchpoint at: ";
		message += normalizedAddressBuffer;
		message += "?";
		BAlert* alert = new BAlert("", message.String(), "OK", "Cancel");
		if (alert->Go() != 1) 
			return;
		pc &= ~kLowTwoBits;
	}
	
	DNub& nub = GetNub();
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		try
		{
			nub.SetWatchpoint(pc);
			fWatchpoints.push_back(DWatchpoint(name, kind, pc));
			fOwner->PostMessage(kMsgWatchpointsChanged);
		}
		catch (...)
		{
			char addressBuffer[16];
			sprintf(addressBuffer, "%lx", pc);
			BString message("Cannot set watchpoint at address: ");
			message += addressBuffer;
			message += ".";
			(new BAlert("", message.String(), "OK"))->Go();
		}
	}
} /* DTeam::SetWatchpoint */

void DTeam::SetWatchpoint(uint32 kind, ptr_t pc)
{
	BString name;
	name << "Watchpoint " << fWatchpoints.size() + 1;
	this->SetWatchpoint(name.String(), kind, pc);
} /* DTeam::SetWatchpoint */

void DTeam::ClearWatchpoint(ptr_t addr)
{
	DNub& nub = GetNub();
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		WatchpointList::iterator i = find_if(fWatchpoints.begin(), fWatchpoints.end(), P_ap_with_addr(addr));

		if (i != fWatchpoints.end())
		{
			if (!(*i).Disabled())
				nub.ClearWatchpoint(addr);
			fWatchpoints.erase(i);
			fOwner->PostMessage(kMsgWatchpointsChanged);
		}
	}

} /* DTeam::ClearWatchpoint */

DWatchpoint *
DTeam::FindWatchpoint(ptr_t addr)
{
	WatchpointList::iterator i = find_if(fWatchpoints.begin(), fWatchpoints.end(), P_ap_with_addr(addr));

	if (i != fWatchpoints.end())
		return &(*i);
	
	return 0;
}


void 
DTeam::EnableWatchpoint(ptr_t addr)
{
	DNub& nub = GetNub();
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		WatchpointList::iterator i = find_if(fWatchpoints.begin(), fWatchpoints.end(), P_ap_with_addr(addr));

		if (i != fWatchpoints.end() && (*i).Disabled())
		{
			nub.SetWatchpoint(addr);
			(*i).SetDisabled(false);
			fOwner->PostMessage(kMsgWatchpointsChanged);
		}
	}
}

void 
DTeam::DisableWatchpoint(ptr_t addr)
{
	DNub& nub = GetNub();
	
	BAutolock lock(nub);
	if (lock.IsLocked())
	{
		WatchpointList::iterator i = find_if(fWatchpoints.begin(), fWatchpoints.end(), P_ap_with_addr(addr));

		if (i != fWatchpoints.end() && !(*i).Disabled())
		{
			nub.ClearWatchpoint(addr);
			(*i).SetDisabled(true);
			fOwner->PostMessage(kMsgWatchpointsChanged);
		}
	}
}

void 
DTeam::GetWatchpoints(WatchpointList &watchpoints)
{
	watchpoints = fWatchpoints;
}

DNub& DTeam::GetNub() const
{
	if (fNub == NULL)
		THROW(("Not running"));
	
	return *fNub;
} // DTeam::GetNub

void DTeam::GetBreakpoints(BreakpointList& breakpoints) const
{
	breakpoints = fBreakpoints;
} // DTeam::GetBreakpoints(set<DBreakpoint>& breakpoints)

BreakpointList& DTeam::GetBreakpoints()
{
	return fBreakpoints;
} // DTeam::GetBreakpoints(set<DBreakpoint>& breakpoints)

void DTeam::WriteBreakpoints()
{
	entry_ref ref = fRef;
	
	char name[NAME_MAX];
	strcpy(name, fRef.name);
	strcat(name, ".dbg");
	
	ref.set_name(name);
	
	BPath p;
	if (BEntry(&ref).GetPath(&p) == B_OK)
	{
		FILE *f = fopen(p.Path(), "w");
		if (f)
		{
			fputs("# window positions\n", f);

			fprintf(f, "pos:team:%g,%g,%g,%g\n", fTeamWindowRect.left,
				fTeamWindowRect.top, fTeamWindowRect.right, fTeamWindowRect.bottom);
			
			fprintf(f, "pos:brkp:%g,%g,%g,%g\n", fBreakpointsWindowRect.left,
				fBreakpointsWindowRect.top, fBreakpointsWindowRect.right, fBreakpointsWindowRect.bottom);
			
			fputs("# breakpoints\n", f);
			
			BreakpointList::iterator bp;
	
			for (bp = fBreakpoints.begin(); bp != fBreakpoints.end(); bp++)
				(*bp).Write(f);

			fclose(f);
		}
	}
} // DTeam::WriteBreakpoints

void DTeam::ReadBreakpoints()
{
	entry_ref ref = fRef;
	
	char name[NAME_MAX];
	strcpy(name, fRef.name);
	strcat(name, ".dbg");
	
	ref.set_name(name);
	
	BPath p;
	if (BEntry(&ref).GetPath(&p) == B_OK)
	{
		FILE *f = fopen(p.Path(), "r");
		if (f)
		{
			char s[PATH_MAX + 20];
			
			while (fgets(s, PATH_MAX + 20, f))
			{
				if (s[0] == '#')
					continue;
			
				char *t = strrchr(s, '\n'); if (t) *t = 0;
				
				if (strncmp(s, "pos:", 4) == 0)
				{
					t = strrchr(s, ':');
					if (t == NULL) continue;
					
					BRect r;
					r.left = strtod(t + 1, &t);		if (t == NULL || *t != ';') continue;
					r.top = strtod(t + 1, &t);		if (t == NULL || *t != ';') continue;
					r.right = strtod(t + 1, &t);		if (t == NULL || *t != ';') continue;
					r.bottom = strtod(t + 1, &t);
					
					if (strncmp(s + 4, "team", 4) == 0)
						fTeamWindowRect = r;
					else if (strncmp(s + 4, "brkp", 4) == 0)
						fBreakpointsWindowRect = r;
				}
				else
				{
					try
					{
						DBreakpoint bp(s);
						fBreakpoints.push_back(bp);
					}
					catch (...) {}
				}
			}

			fclose(f);
		}
	}
} // DTeam::ReadBreakpoints

void DTeam::SetBreakOnException(bool on)
{
	DNub& nub = GetNub();
	BAutolock lock(nub);
	
	if (on == true)
	{
		try
		{
			// only get the address again if we never went to get it before
			if (fThrowPC == 0) {
				int throwSize;
#if __INTEL__	/* FIXME: Could this be __GNUC__ instead? */
				fSymWorld.GetFunctionOffsetAndSize("__throw", fThrowPC, throwSize);
#else
#	error
#endif
			}
			nub.SetBreakpoint(fThrowPC);
		}
		catch (HErr& e)
		{
//			(new BAlert("", "Could not set breakpoint at __throw", "OK"))->Go();
		}
	}
	else if (on == false && fThrowPC)
	{
		nub.ClearBreakpoint(fThrowPC);
		fThrowPC = 0;
	}
	
	gPrefs->SetPrefInt("break on c++ exception", on);
} // DTeam::SetBreakOnException

void 
DTeam::SetBreakOnLoadImage(bool on)
{
	DNub& nub = GetNub();
	BAutolock lock(nub);

	if (!fLoadAddonPC) {
		try
		{
			int entrySize;
			fSymWorld.GetFunctionOffsetAndSize("load_add_on", fLoadAddonPC,
				entrySize);
		}
		catch (HErr& e)
		{
			// just silently continue - app has no load_add_on calls
			fLoadAddonPC = 0;
		}
	}

	if (on) {
		if (fLoadAddonPC)
			nub.SetBreakpoint(fLoadAddonPC);
	} else {
		if (fLoadAddonPC)
			nub.ClearBreakpoint(fLoadAddonPC);
		fLoadAddonPC = 0;
	}

	gPrefs->SetPrefInt("stop on load_image", on);
}

void 
DTeam::SetBreakOnThreadCreation(bool on)
{
	DNub& nub = GetNub();
	BAutolock lock(nub);

	if (!fSpawnThreadPC) {
		try
		{
			int entrySize;
			fSymWorld.GetFunctionOffsetAndSize("spawn_thread", fSpawnThreadPC,
				entrySize);
		}
		catch (HErr& e)
		{
			// just silently continue - app has no spawn_thread calls
			fSpawnThreadPC = 0;
		}
	}

	if (on) {
		if (fSpawnThreadPC)
			nub.SetBreakpoint(fSpawnThreadPC);
	} else {
		if (fSpawnThreadPC)
			nub.ClearBreakpoint(fSpawnThreadPC);
		fSpawnThreadPC = 0;
	}

	gPrefs->SetPrefInt("stop on spawn_thread", on);
}


void DTeam::PrintToOutputWindow(const char *s)
{
	if (!IsLocked())
		THROW(("DTeam is not locked"));
	
// I wanted to use named pipes, but they keep locking up the whole app

//	if (fOutputPipe < 0)
//	{
//		strstream s;
//		s << "/pipe/debug_output_" << fTeamID << ends;
//		fPipeName = s.str();
//		
//		if ((fOutputPipe = open(s.str(), O_WRONLY | O_CREAT | O_NONBLOCK)) < 0)
//			THROW(("Could not open output pipe"));
//	}
//	
//	thread_info ti;
//	if (get_thread_info(fOutputThread, &ti) != B_OK)
//	{
//		string t("Debug output of Team ");
//		t += fRef.name;
//		
//		char path [PATH_MAX];
//		char *argv[10];
//	
//		find_directory (B_BEOS_APPS_DIRECTORY, -1, false, path, PATH_MAX);
//		strcat (path, "/Terminal");
//		argv[0] = path;
//		argv[1] = "-t";
//		argv[2] = t.c_str();
//		argv[3] = "/bin/cat";
//		argv[4] = fPipeName.c_str();
//		argv[5] = NULL;
//		
//		FailOSErr2 (fOutputThread = load_image(5, argv, environ));
//		resume_thread (fOutputThread);
//		
//			// give cat a chance to open the pipe for reading...
//			// will block otherwise, dunno why...
//		snooze(1000000);
//	}
//
//	(void)write(fOutputPipe, s, strlen(s));
	
	if (fOutputPipe < 0)
	{
		std::strstream ss;
		ss << "/tmp/debug_output_" << fTeamID << std::ends;
		fPipeName = ss.str();
		delete[] ss.str();
		
		if ((fOutputPipe = open(fPipeName.c_str(), O_WRONLY | O_CREAT | O_NONBLOCK)) < 0)
			THROW(("Could not open output file"));
	}
		
		// check if the Terminal is still running
	thread_info ti;
	if (get_thread_info(fOutputThread, &ti) != B_OK)
	{
		string t("Debug output of Team ");
		t += fRef.name;
		
		char path [PATH_MAX];
		const char *argv[10];
	
		find_directory (B_BEOS_APPS_DIRECTORY, -1, false, path, PATH_MAX);
		strcat (path, "/Terminal");
		argv[0] = path;
		argv[1] = "-t";
		argv[2] = t.c_str();
		argv[3] = "/bin/tail";
		argv[4] = "-f";
		argv[5] = fPipeName.c_str();
		argv[6] = NULL;
		
		FailOSErr2 (fOutputThread = load_image(6, argv, (const char **)environ));
		resume_thread (fOutputThread);
	}

	(void)write(fOutputPipe, s, strlen(s));
} // DTeam::PrintToOutputWindow

