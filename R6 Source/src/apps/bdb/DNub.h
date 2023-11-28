// DNub - API for bdb's C++ interface to the kernel debugging nub

#ifndef DNUB_H
#define DNUB_H 1

#include "bdb.h"
#include <string>
#include <vector>
#include <kernel/OS.h>
#include <support/Locker.h>

using std::string;
class DStackCrawl;
class DCpuState;

// ThreadToken is enough information about a thread to refer to it uniquely
// and to display it meaningfully to the user, with enough supporting methods
// to put it in STL containers.
struct ThreadToken
{
	thread_id id;
	string name;

	ThreadToken() : id(-1), name() { }
	ThreadToken(thread_id theID, const char* theName) : id(theID), name(theName) { }
	ThreadToken(thread_id theID, const string& theName) : id(theID), name(theName) { }

	// default copy ctor and assignment operator work perfectly well for
	// this struct, so we don't provide them ourselves

	// thread_ids are globally unique, so we needn't ever compare names
	int operator==(const ThreadToken& rhs) { return (id == rhs.id); }
	int operator==(thread_id otherID) { return (id == otherID); }
	int operator<(const ThreadToken& rhs) { return (id < rhs.id); }
	int operator<(thread_id otherID) { return (id == otherID); }
};

typedef std::vector<ThreadToken> thread_map;

// GetImageList() fills out one of these:
typedef std::vector<image_info> image_list;

class DNub : public BLocker
{
  public:
		// Nub's should only be created and closed by a DTeam object
	DNub();

	virtual void Start() = 0;
	virtual void Kill() = 0;
	virtual void Detach() = 0;
	virtual void DTeamClosed() = 0;

	virtual void StopThread(thread_id thread) = 0;
	virtual void KillThread(thread_id thread) = 0;

	virtual void ReadData(ptr_t address, void *buffer, size_t size) = 0;
	virtual void WriteData(ptr_t address, const void *buffer, size_t size) = 0;
	
	virtual void SetBreakpoint(ptr_t address) = 0;
	virtual void ClearBreakpoint(ptr_t address) = 0;
		
	virtual void SetWatchpoint(ptr_t address) = 0;
	virtual void ClearWatchpoint(ptr_t address) = 0;

	// Do a stack crawl for a certain pc. This information is processor
	// dependant so therefore a subclass should implement it.
	virtual void GetStackCrawl(DCpuState& frame, DStackCrawl& outCrawl) = 0;

	// Okay, this is silly.  We're breaking ToNubMessage into several new
	// methods that provide specific functionality.
	virtual void Run(thread_id tid, const DCpuState& cpu) = 0;
	virtual void Step(thread_id tid, const DCpuState& cpu, ptr_t lowPC, ptr_t highPC) = 0;
	virtual void StepOver(thread_id tid, const DCpuState& cpu, ptr_t lowPC, ptr_t highPC) = 0;
	virtual void StepOut(thread_id tid, const DCpuState& cpu) = 0;

	// Does *NOT* delete the cpu pointer before returning a new object
	virtual void GetThreadRegisters(thread_id thread, DCpuState*& outCpu) = 0;

	virtual void GetThreadList(team_id team, thread_map& outThreadList) = 0;
	virtual void GetThreadInfo(thread_id thread, thread_info& outThreadInfo) = 0;
	virtual void GetImageList(team_id team, image_list& outImageList) = 0;

	// nonvirtual methods, implemented in terms of the above virtuals
	template <class T>
	void Read(ptr_t address, T& value)
	{
		ReadData(address, &value, sizeof(T));
	}
	
	template <class T>
	void Write(ptr_t address, const T& value)
	{
		WriteData(address, &value, sizeof(T));
	}
	
	void ReadString(ptr_t address, char *s, size_t max);
	void ReadString(ptr_t address, string& s);
	
protected:
	virtual ~DNub() = 0;
};

#endif
