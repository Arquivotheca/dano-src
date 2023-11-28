/*	$Id: DTeam.h,v 1.14 1999/05/11 21:31:09 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/12/98 15:15:12
*/

#ifndef DTEAM_H
#define DTEAM_H

#include "DSymWorld.h"
#include "DBreakpoint.h"
#include "DWatchpoint.h"
#include "DRuntimeSupport.h"

#include <list>
#include <map>
using std::string;

typedef std::list<DBreakpoint> BreakpointList;
typedef std::list<DWatchpoint> WatchpointList;

class DNub;
class DThread;
class DCpuState;

class DTeam : public BLocker
{
		// Constructors are private, you should use CreateTeam to construct
		// DTeam's
	DTeam(entry_ref&, BLooper *, int argc, char **argv);
	DTeam(team_id id, port_id port, BLooper *owner);
	DTeam(const char* host, uint16 port, BLooper* owner);

  public:
	~DTeam();

		// To start debugging an app call this static function and pass in
		// an entry_ref to the app
	static	DTeam* CreateTeam(entry_ref& ref, BLooper *owner, int argc = 0, char **argv = NULL);
	
		// Or call this one to start debugging an already running team
	static DTeam* CreateTeam(team_id id, port_id port, BLooper *owner);

		// Or call this one to connect to a remote proxy
	static DTeam* CreateTeam(const char* host, uint16 port, BLooper* owner);

		// To launch the target
	void Launch();

		// To access DThreads
	DThread* GetThread(thread_id);

		// Every team should have only one nub to the debugger...
	DNub& GetNub() const;

		// The symbolic world can be accessed through this one
	DSymWorld& GetSymWorld()				{ return fSymWorld; };
	
		// Call this method to get a set of breakable linenrs and the breakpoints
		// already set in this file
	void GetStopsForFile(DFileNr file, std::set<int>& lines, std::set<int>& breakpoints);

		// The next set of functions is called by the Nub to notify this
		// team of any interesting event
	void TeamDeleted();
	void ThreadCreated(thread_id);
	void ThreadDeleted(thread_id);
	void ThreadStopped(thread_id, db_why_stopped, DCpuState*, void *);
	void ImageCreated(const image_info&);
	void ImageDeleted(const image_info&);
	
	void SetBreakpoint(ptr_t pc, thread_id oneTimeThread);
	void ClearBreakpoint(ptr_t pc);
	void EnableBreakpoint(ptr_t pc);
	void DisableBreakpoint(ptr_t pc);
	void ClearAllBreakpoints();
	void GetBreakpoints(BreakpointList& breakpoints) const;
	BreakpointList& GetBreakpoints();

	void SetBreakpointCondition(ptr_t pc, const char* condition);
	void SetBreakpointSkipCount(ptr_t pc, unsigned long skipCount);
	void SetBreakpointHitCount(ptr_t pc, unsigned long hitCount);
	bool EvaluateBreakpointCondition(DBreakpoint& bp, DThread& thread);

	void SetBreakOnException(bool on);
	void SetBreakOnLoadImage(bool on);
	void SetBreakOnThreadCreation(bool on);

	void ReadBreakpoints();
	void WriteBreakpoints();
	
	void SetWatchpoint(const char *name, uint32 kind, ptr_t addr);
	void SetWatchpoint(uint32 kind, ptr_t addr);
	void ClearWatchpoint(ptr_t addr);
//	void ClearWatchpoint(const char *name);
	void ClearAllWatchpoints();
	void EnableWatchpoint(ptr_t addr);
	void DisableWatchpoint(ptr_t addr);
	void GetWatchpoints(WatchpointList& watchpoints);
	DWatchpoint *FindWatchpoint(ptr_t addr);
	
	void DoKill();
	void Detach();
	
	team_id GetID() const						{ return fTeamID; };
	BLooper *GetOwner() const					{ return fOwner; };
	ptr_t ThrowPC() const						{ return fThrowPC; }
	ptr_t LoadAddOnPC() const					{ return fLoadAddonPC; }
	ptr_t SpawnThreadPC() const					{ return fSpawnThreadPC; }
	
	DRuntimeSupportList& GetRuntimeList()	{ return fRT; }
	const entry_ref& GetRef() const			{ return fRef; }
	void SetRef(const entry_ref& ref)		{ fRef = ref; }
	
	BRect& TeamWindowRect()					{ return fTeamWindowRect; }
	BRect& BreakpointsWindowRect()			{ return fBreakpointsWindowRect; }
	
	void PrintToOutputWindow(const char *s);
	
	bool SwitchWorkspace() const
		{ return fSwitchWorkspace; }
	void SwitchWorkspaceIfNeeded(bool switchWorkspace);

  private:
  	
		// to connect the debugger to an already launched team  	
  	void Connect(port_id port = -1);

	// connect the debugger to a remote proxy
	void Connect(const char* host, uint16 port = 5038);

	// post-connect initialization
	void InitAfterConnect();

	bool IsStopPoint(DBreakpoint& bp, DThread& thread);
	void DoStopAction(DBreakpoint& bp, DThread& thread, DCpuState& cpu);

	int fArgc;
	char **fArgv;
	ptr_t fThrowPC;
	ptr_t fLoadAddonPC;
	ptr_t fSpawnThreadPC;
	entry_ref fRef;
	BLooper *fOwner;
	team_id fTeamID;
	DNub *fNub;
	DSymWorld fSymWorld;
	std::map<thread_id, DThread*> fThreads;
	BreakpointList fBreakpoints;
	WatchpointList fWatchpoints;
	DRuntimeSupportList fRT;
	BRect fTeamWindowRect, fBreakpointsWindowRect;
	int fOutputPipe;
	string fPipeName;
	thread_id fOutputThread;

	bool fSwitchWorkspace;
	int32 fWorkspace;
};

#endif
