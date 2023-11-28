/*	$Id: DNub.h,v 1.6 1999/02/11 15:51:54 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/10/98 14:53:42
*/

#ifndef DLOCALNUB_H
#define DLOCALNUB_H

class DTeam;
class DStackCrawl;

#include "DNub.h"

class DLocalNub : virtual public DNub
{
  public:
		// Nub's should only be created and closed by a DTeam object
	DLocalNub(port_id, port_id, DTeam *);
	void Start();
	
	void Kill();
	void Detach();
	void DTeamClosed();

	void StopThread(thread_id thread);
	void KillThread(thread_id thread);

	void ReadData(ptr_t address, void *buffer, size_t size);
	void WriteData(ptr_t address, const void *buffer, size_t size);
	
	void ReadString(ptr_t address, string& s);
	
	void Run(thread_id tid, const DCpuState& cpu);
	void Step(thread_id tid, const DCpuState& cpu, ptr_t lowPC, ptr_t highPC);
	void StepOver(thread_id tid, const DCpuState& cpu, ptr_t lowPC, ptr_t highPC);
	void StepOut(thread_id tid, const DCpuState& cpu);
	
	void SetBreakpoint(ptr_t address);
	void ClearBreakpoint(ptr_t address);
		
	void SetWatchpoint(ptr_t address);
	void ClearWatchpoint(ptr_t address);

	virtual void GetThreadRegisters(thread_id thread, DCpuState*& cpu);
	virtual void GetThreadList(team_id team, thread_map& outThreadList);
	virtual void GetThreadInfo(thread_id thread, thread_info& outThreadInfo);
	virtual void GetImageList(team_id team, image_list &outImageList);

  protected:
		// call Close to delete a nub
	virtual ~DLocalNub();

  private:			
		// This is the function that talks to the debugger nub
	static long Server(void *);
	long Serve();
	
	void WaitForNub();

	port_id fToNubPort, fFromNubPort;		// communciation with the kernel nub
	port_id fReply;
	sem_id fNubReady;
	thread_id fServer;
	DTeam *fTeam;
};

#endif
