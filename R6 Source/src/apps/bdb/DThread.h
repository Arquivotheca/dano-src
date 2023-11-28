/*	$Id: DThread.h,v 1.4 1998/11/17 12:16:47 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/14/98 10:56:31
*/

#ifndef DTHREAD_H
#define DTHREAD_H

#include "DStackCrawl.h"
#include "DTypes.h"
#include <String.h>

enum {
	tsSuspended,
	tsRunning,
	tsDeleted
};

class DTeam;
class DStateMachine;
class DCpuState;

class DThread : public BLocker {
public:
			DThread(thread_id, DTeam&);
			
			thread_id GetID() const							{ return fThreadID; }
			int GetState() const								{ return fState; }
			DTeam& GetTeam() const						{ return fTeam; }
			BString Name() const;
			
			DStackCrawl& GetStackCrawl()					{ return fStackCrawl; }
			
			void Stopped(db_why_stopped, DCpuState*, void *);
			void Deleted();
			
			void DoRun(bool switchWorkspace = false);
			void DoStep(bool switchWorkspace = false);
			void DoStepOver(bool switchWorkspace = false);
			void DoStepOut(bool switchWorkspace = false);
			void DoKill();
			void DoStop();
			
			void DoDebugAction();
			bool HasDebugAction () const					{ return fAction != daNone; }
			EDebugAction LastAction() const				{ return fLastAction; }
			
			void SetCPU (const DCpuState& newCPU, bool tellListeners = true);
			DCpuState& GetCPU() const				{ return *fCPU; }
			
			void SetListener(BLooper *listener);
			db_why_stopped WhyStopped() const		{ return fWhy; }
			void *ThreadStoppedData() const		{ return fThreadStoppedData; }
			
protected:
			
			thread_id fThreadID;
			thread_info fInfo;				// backup for info, of course not all of it is valid always...
			int fState;
			DCpuState* fCPU;
			DTeam& fTeam;
			DStackCrawl fStackCrawl;
			db_why_stopped fWhy;
			void *fThreadStoppedData;
			BLooper *fListener;
			DStateMachine *fStateMachine;
			EDebugAction fAction, fLastAction;

private:
			~DThread();
};


#endif
