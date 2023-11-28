// DRemoteProxyNub - a DNub subclass for passing instructions back and
// forth between a local bdb and a remote debugging target

#include "DRemoteProxyNub.h"
#include "RemoteProtocol.h"
#include "DTeam.h"
#include "DCpuState.h"
#include "TCPLooper.h"
#include "TCPMessenger.h"
#include "UDPLooper.h"
#include "UDPMessenger.h"
#include <Message.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netdb.h>

#if DEBUG
#define dfprintf fprintf
#else
inline void dfprintf(FILE*, const char*, ...) { }
#endif

// TCPLooper subclass for talking to the remote proxy
class ProxyNubTCPLooper : public TCPLooper
{
public:
	ProxyNubTCPLooper(uint16 listenPort, port_id responsePort,
		TCPMessenger* proxyMessenger, DRemoteProxyNub& myNub);
	void MessageReceived(BMessage*);

private:
	port_id fResponsePort;
	TCPMessenger* fMessenger;
	DRemoteProxyNub& fNub;
};

// UDP version of the same thing
class ProxyNubUDPLooper : public UDPLooper
{
public:
	ProxyNubUDPLooper(uint16 listenPort, port_id responsePort,
		UDPMessenger* proxyMessenger, DRemoteProxyNub& myNub);
	void MessageReceived(BMessage*);

private:
	port_id fResponsePort;
	UDPMessenger* fMessenger;
	DRemoteProxyNub& fNub;
};

// DRemoteProxyNub implementation
DRemoteProxyNub::DRemoteProxyNub(const char *remoteHostname, uint16 remotePort, DTeam *team)
	: fTeam(team), fRemoteTeamID(-1), fTCPLooper(NULL), fTCPMessenger(NULL),
		fUDPLooper(NULL), fUDPMessenger(NULL)
{
	// First, figure out whether we're using TCP or UDP
	fUsingTCP = true;
	const char* udpenv = getenv("BDB_USE_UDP");
	if (udpenv && (!strcmp(udpenv, "1") || !strcmp(udpenv, "true") || !strcmp(udpenv, "TRUE")))
	{
fprintf(stderr, "BDB_USE_UDP=%s -- using UDP\n", udpenv);
		fUsingTCP = false;
	}

	// we require the the hostname lookup return at least one IPv4 address
	struct hostent* he = gethostbyname(remoteHostname);
	if (he && (he->h_addr_list != NULL) && (he->h_length == 4))
	{
		fNubMessagePort = create_port(4, "nub msg handoff port");
		resume_thread(spawn_thread(nub_message_thread, "nub message handler", B_NORMAL_PRIORITY, this));

		memcpy(&fRemoteAddress.sin_addr.s_addr, *he->h_addr_list, 4);		// grab the first returned address
		fRemoteAddress.sin_port = htons(remotePort);
		fRemoteAddress.sin_len = sizeof(fRemoteAddress);
		fRemoteAddress.sin_family = AF_INET;
		fResponsePort = create_port(4, "DRemoteProxyNub response port");

		if (fUsingTCP)
		{
fprintf(stderr, "--- instantiating tcp messenger & looper\n");
			fTCPMessenger = new TCPMessenger(remoteHostname, remotePort);

			// !!! make the TCP port etc. user-configurable!
			fTCPLooper = new ProxyNubTCPLooper(5039, fResponsePort, fTCPMessenger, *this);
			fTCPLooper->Run();
		}
		else			// using UDP
		{
fprintf(stderr, "--- instantiating udp messenger & looper\n");
			fUDPMessenger = new UDPMessenger(remoteHostname, remotePort);
			fUDPLooper = new ProxyNubUDPLooper(5039, fResponsePort, fUDPMessenger, *this);
			fUDPLooper->Run();
		}
	}
	else
	{
		THROW(("Unable to resolve remote hostname '%s'", remoteHostname));
	}

	fRemoteBinRoot = getenv("BDB_REMOTE_BIN_ROOT");
	if (!fRemoteBinRoot)
	{
		fprintf(stderr, "* BDB_REMOTE_BIN_ROOT not defined, no path-mapping will be done\n");
		fRemoteBinRoot = "";
	}
}

DRemoteProxyNub::~DRemoteProxyNub()
{
	for (page_cache::iterator i = fCache.begin(); i != fCache.end(); i++)
	{
		delete [] (*i)->data;
	}
	delete_port(fNubMessagePort);
	delete fTCPMessenger;
	delete fUDPMessenger;
	QuitLooper();
}

void 
DRemoteProxyNub::Start()
{
	Attach();
}

// kill the target team and detach
void 
DRemoteProxyNub::Kill()
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't kill the remote target; not attached"));

	// no response, so no need to lock 'this'
	BMessage msg(BDB_MSG_KILL_TARGET);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);
	fTeam = NULL;
	QuitLooper();
}

// detach and allow the target team to proceed
void 
DRemoteProxyNub::Detach()
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't detach from the remote target; not attached"));

	// no response, so no need to lock 'this'
	BMessage msg(BDB_MSG_DETACH);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);
	fTeam = NULL;
	QuitLooper();
}

// Called when the owning DTeam object is deleted
void 
DRemoteProxyNub::DTeamClosed()
{
	fTeam = NULL;
	QuitLooper();
}

// Called after construction
void 
DRemoteProxyNub::Attach()
{
	// look up our own IP address
	struct hostent* he;
	struct utsname me;
	uname(&me);
	he = gethostbyname(me.nodename);
	if (!he)
		THROW(("Unable to resolve local host name '%s'; connection to remote target aborted.", me.nodename));

	BMessage msg(BDB_MSG_ATTACH);
	BAutolock lock(this);

	uint32 addr = **((uint32**) he->h_addr_list);
	msg.AddInt32("reply_ip", addr);
	msg.AddInt32("reply_port", htons(5039));		// !!! magic number; keep this somewhere else

	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// the attach response is a BMessage* with some number of image_info structs
	// included as B_RAW_TYPE data, all called "image"
	int32 code;
	BMessage* response;
	status_t err = read_port_etc(fResponsePort, &code, &response, sizeof(response), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to connect to remote host");
	FailOSErrMsg(!response, "Failure trying to connect to remote host; aborting connection");

	// the response has the remote team ID in it
	response->FindInt32("team", (int32*) &fRemoteTeamID);

	// Okay, we've got the response.  Now we loop over the image_info structs until
	// we find the main B_APP_IMAGE one, and key our image paths to that. 
	// Note that we keep the nub locked until we return from this function, so
	// we don't have to worry about other requests or nub actions accessing
	// this not-quite-attached object
	int32 index = 0;
	const image_info* info;
	ssize_t numBytes;
	while (response->FindData("image", B_RAW_TYPE, index, (const void**) &info, &numBytes) == B_OK)
	{
		image_info localImageInfo;

		// redirect the (remote) image path to the local root, but only if we've specified a
		// nontrivial root path for finding binaries on the local machine
		MapRemoteImageToLocal(*info, localImageInfo);

		if (localImageInfo.type == B_APP_IMAGE)
		{
			fprintf(stderr, "\n* found main remote app image: %s\n\n", localImageInfo.name);

			// sanity-check the local app image; if it doesn't exist, we need to bail
			BEntry appRef(localImageInfo.name, true);
			if (!appRef.Exists())
			{
				delete response;
				THROW(("Unable to find local application %s - aborting connection", localImageInfo.name));
			}

			entry_ref ref;
			appRef.GetRef(&ref);
			fTeam->SetRef(ref);
		}
		fTeam->ImageCreated(localImageInfo);
		index++;
	}

	delete response;
}

void 
DRemoteProxyNub::QuitLooper(void)
{
	if (fTCPLooper)
	{
		fTCPLooper->PostMessage(BDB_MSG_QUIT_LOOPER);
		fTCPLooper = NULL;
	}
	else if (fUDPLooper)
	{
		fUDPLooper->PostMessage(BDB_MSG_QUIT_LOOPER);
		fUDPLooper = NULL;
	}
}

void 
DRemoteProxyNub::StopThread(thread_id thread)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't stop thread on remote target; not attached"));

	BMessage msg(BDB_MSG_STOP_THREAD);
	msg.AddInt32("thread", thread);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// the stop-thread response is a single status_t
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to stop thread on remote host");
	FailOSErr2(result);
}

void 
DRemoteProxyNub::KillThread(thread_id thread)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't kill thread on remote target; not attached"));

	BMessage msg(BDB_MSG_KILL_THREAD);
	msg.AddInt32("thread", thread);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// the kill-thread response is a single status_t
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to kill thread on remote host");
	FailOSErr2(result);
}

void 
DRemoteProxyNub::ReadData(ptr_t address, void *buffer, size_t size)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't read data from remote target; not attached"));

	CacheEntry* ce = DataInCache(address, size);
	// if it's cached, don't even talk to the remote target
	if (ce)
	{
		memcpy(buffer, ce->data + (address - ce->base), size);
	}
	else
	{
		// not in the cache; we need to talk to the nub after all
		BMessage msg(BDB_MSG_READ_DATA);
		msg.AddInt32("address", address);
		msg.AddInt32("size", size);

	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

		// the read-data response is a BMessage* with an int32 field and a generic B_RAW_TYPE  field:
		// int32 "address" == base address of returned block
		// raw "data" == the returned data from the remote machine
		int32 code;
		BMessage* response;
		status_t err = read_port_etc(fResponsePort, &code, &response, sizeof(response), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
		FailOSErr2Msg(err, "Timeout trying to read data from remote host");
		FailOSErrMsg(!response, "Failure trying to read data from remote host");

		ptr_t returnedBlock;
		unsigned char* data;
		ssize_t dataSize;
		FailOSErr2(response->FindInt32("address", (int32*) &returnedBlock));
		FailOSErr2(response->FindData("data", B_RAW_TYPE, ( const void**) &data, &dataSize));

		// cache the returned pages
		for (ptr_t blockAddr = returnedBlock; blockAddr < returnedBlock + dataSize; blockAddr += B_PAGE_SIZE)
		{
			if (!DataInCache(blockAddr, 1))
			{
				CacheEntry* ce = new CacheEntry;
				ce->base = blockAddr;
				ce->data = new unsigned char[B_PAGE_SIZE];
				memcpy(ce->data, data + (blockAddr - returnedBlock), B_PAGE_SIZE);
				fCache.push_back(ce);
			}
		}

		// return the actual requested data block, clean up, and return
		memcpy(buffer, data + (address - returnedBlock), size);
		delete response;
	}
}

void 
DRemoteProxyNub::WriteData(ptr_t address, const void *buffer, size_t size)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't write data to the remote target; not attached"));

	// if this is in a cached page, update our local copy too
	CacheEntry* ce = DataInCache(address, size);
	if (ce)
	{
		memcpy(ce->data + (address - ce->base), buffer, size);
	}

	// now tell the remote target to write this data
	BMessage msg(BDB_MSG_WRITE_DATA);
	msg.AddInt32("address", address);
	msg.AddData("data", B_RAW_TYPE, buffer, size);

	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// the write-data response is a single status_t
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err , "Timeout trying to write data to remote host");
	FailOSErr(result);
}

void 
DRemoteProxyNub::SetBreakpoint(ptr_t address)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't set breakpoint on the remote target; not attached"));

	// Outline of process:
	//		- lock this to serialize
	//		- post a message to the TCPLooper, instructing it to set the breakpoint (or whatever)
	//		- read the response from fResponsePort [async, supplied by the TCPLooper]
	//			- remember to handle timeouts!
	//		- unlock this
	BMessage msg(BDB_MSG_SET_BREAKPOINT);
	msg.AddInt32("address", address);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// the set-breakpoint response is a single status_t
	// 20-second timeout
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to set breakpoint on remote host");
	FailOSErr2(result);
}

void 
DRemoteProxyNub::ClearBreakpoint(ptr_t address)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't clear breakpoints on the remote target; not attached"));

	BMessage msg(BDB_MSG_CLEAR_BREAKPOINT);
	msg.AddInt32("address", address);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// the clear-breakpoint response is a single status_t
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to clear breakpoint on remote host");
	FailOSErr2(result);
}

void 
DRemoteProxyNub::SetWatchpoint(ptr_t address)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't set watchpoints on the remote target; not attached"));

	BMessage msg(BDB_MSG_SET_WATCHPOINT);
	msg.AddInt32("address", address);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// the set-watchpoint response is a single status_t
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to set watchpoint on remote host");
	FailOSErr(result);
}

void 
DRemoteProxyNub::ClearWatchpoint(ptr_t address)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't clear watchpoints on the remote target; not attached"));

	BMessage msg(BDB_MSG_CLEAR_WATCHPOINT);
	msg.AddInt32("address", address);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// the clear-watchpoint response is a single status_t
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to clear watchpoint on remote host");
	FailOSErr2(result);
}

void 
DRemoteProxyNub::Run(thread_id tid, const DCpuState &cpu)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't send debugger messages to the remote target; not attached"));

	BMessage msg(BDB_MSG_RUN);
	msg.AddInt32("thread", tid);

	ssize_t size = cpu.FlattenedSize();
	char* buf = new char[size];
	cpu.Flatten(buf, size);
	msg.AddData("cpu", B_RAW_TYPE, buf, size);
	delete [] buf;

	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// run-thread message response is a simple "result" status_t indicating successful delivery
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to write debugger control message to remote host");
	FailOSErr2(result);

	// flush the page cache whenever we affect the thread's run state
	FlushPageCache();
}

void 
DRemoteProxyNub::Step(thread_id tid, const DCpuState &cpu, ptr_t lowPC, ptr_t highPC)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't send debugger messages to the remote target; not attached"));

	BMessage msg(BDB_MSG_STEP_OUT);
	msg.AddInt32("thread", tid);
	msg.AddInt32("low", lowPC);
	msg.AddInt32("high", highPC);

	ssize_t size = cpu.FlattenedSize();
	char* buf = new char[size];
	cpu.Flatten(buf, size);
	msg.AddData("cpu", B_RAW_TYPE, buf, size);
	delete [] buf;

	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// step message response is a simple "result" status_t indicating successful delivery
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to write debugger control message to remote host");
	FailOSErr2(result);

	FlushPageCache();
}

void 
DRemoteProxyNub::StepOver(thread_id tid, const DCpuState &cpu, ptr_t lowPC, ptr_t highPC)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't send debugger messages to the remote target; not attached"));

	BMessage msg(BDB_MSG_STEP_OVER);
	msg.AddInt32("thread", tid);
	msg.AddInt32("low", lowPC);
	msg.AddInt32("high", highPC);

	ssize_t size = cpu.FlattenedSize();
	char* buf = new char[size];
	cpu.Flatten(buf, size);
	msg.AddData("cpu", B_RAW_TYPE, buf, size);
	delete [] buf;

	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// step-over message response is a simple "result" status_t indicating successful delivery
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to write debugger control message to remote host");
	FailOSErr2(result);

	FlushPageCache();
}

void 
DRemoteProxyNub::StepOut(thread_id tid, const DCpuState &cpu)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't send debugger messages to the remote target; not attached"));

	BMessage msg(BDB_MSG_STEP_OUT);
	msg.AddInt32("thread", tid);

	ssize_t size = cpu.FlattenedSize();
	char* buf = new char[size];
	cpu.Flatten(buf, size);
	msg.AddData("cpu", B_RAW_TYPE, buf, size);
	delete [] buf;

	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// step-out message response is a simple "result" status_t indicating successful delivery
	int32 code;
	status_t result = B_OK;
	status_t err = read_port_etc(fResponsePort, &code, &result, sizeof(result), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to write debugger control message to remote host");
	FailOSErr2(result);

	FlushPageCache();
}

void 
DRemoteProxyNub::GetThreadRegisters(thread_id thread, DCpuState*& outCpu)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't get remote thread registers; not attached"));

	BMessage msg(BDB_MSG_GET_THREAD_REGS);
	msg.AddInt32("thread", thread);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// response is a BMessage containing a raw cpu_state structure called "cpu_state"
	int32 code;
	BMessage* response;
	status_t err = read_port_etc(fResponsePort, &code, &response, sizeof(response), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to get thread registers from remote host");
	FailOSErrMsg(!response, "Failure trying to get thread registers from remote host");

	const void* cpuData = NULL;
	ssize_t size;
	FailOSErr2(response->FindData("cpu", B_RAW_TYPE, &cpuData, &size));
	outCpu = DCpuState::Unflatten(cpuData);
	delete response;
}

void 
DRemoteProxyNub::GetThreadList(team_id /*team*/, thread_map &outThreadList)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't get remote thread list; not attached"));

	// ignore the team argument and pass the (known) remote target team ID
	BMessage msg(BDB_MSG_GET_THREAD_LIST);
	msg.AddInt32("team", fRemoteTeamID);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// response is a BMessage containing thread_id/name pairs called "tid" and "tname"
	int32 code;
	BMessage* response;
	status_t err = read_port_etc(fResponsePort, &code, &response, sizeof(response), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to get thread list from remote host");
	FailOSErrMsg(!response, "Failure trying to get thread list from remote host");

	thread_id tid;
	const char* tname;
	int32 index = 0;
	while (response->FindInt32("tid", index, &tid) == B_OK)
	{
		response->FindString("tname", index, &tname);
		outThreadList.push_back(ThreadToken(tid, tname));
		index++;
	}
	delete response;
}

void 
DRemoteProxyNub::GetThreadInfo(thread_id thread, thread_info &outThreadInfo)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't get remote thread info; not attached"));

	BMessage msg(BDB_MSG_GET_THREAD_INFO);
	msg.AddInt32("thread", thread);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// response is a BMessage containing a thread_info struct called "tinfo"
	int32 code;
	BMessage* response;
	status_t err = read_port_etc(fResponsePort, &code, &response, sizeof(response), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to get thread details from remote host");
	FailOSErrMsg(!response, "Failure trying to get thread details from remote host");

	const thread_info* tinfo;
	ssize_t size;
	FailOSErr2(response->FindData("tinfo", B_RAW_TYPE, (const void**) &tinfo, &size));
	memcpy(&outThreadInfo, tinfo, sizeof(thread_info));
	delete response;
}

void 
DRemoteProxyNub::GetImageList(team_id /*team*/, image_list &outImageList)
{
	BAutolock lock(this);
	if (fRemoteTeamID < 0) THROW(("Can't get remote image list; not attached"));

	// ignore the team argument and pass the (known) remote target team ID
	BMessage msg(BDB_MSG_GET_IMAGE_LIST);
	msg.AddInt32("team", fRemoteTeamID);
	if (fUsingTCP)
		fTCPLooper->PostMessage(&msg);
	else
		fUDPLooper->PostMessage(&msg);

	// response is a BMessage containing image_info objects called "image"
	int32 code;
	BMessage* response;
	status_t err = read_port_etc(fResponsePort, &code, &response, sizeof(response), B_RELATIVE_TIMEOUT, NUB_TIMEOUT);
	FailOSErr2Msg(err, "Timeout trying to get image info from remote host");
	FailOSErrMsg(!response, "Failure trying to get image info from remote host");

	ssize_t size;
	const image_info* image;
	int32 index = 0;
	while (response->FindData("image", B_RAW_TYPE, index, (const void**) &image, &size) == B_OK)
	{
		image_info localImage;
		MapRemoteImageToLocal(*image, localImage);
		outImageList.push_back(localImage);
		index++;
	}
	delete response;
}

// returns a pointer to the cached page for the given address range, NULL if not cached
DRemoteProxyNub::CacheEntry* 
DRemoteProxyNub::DataInCache(ptr_t addr, size_t size) const
{
	for (page_cache::const_iterator i = fCache.begin(); i != fCache.end(); i++)
	{
		if ( (addr >= (*i)->base) && (addr + size - 1 < (*i)->base + B_PAGE_SIZE) )
			return *i;
	}
	return NULL;
}

// Flush the contents of the page cache
void 
DRemoteProxyNub::FlushPageCache()
{
	for (page_cache::iterator i = fCache.begin(); i != fCache.end(); i++)
	{
		delete [] (*i)->data;
	}
	fCache.clear();
}

void 
DRemoteProxyNub::MapRemoteImageToLocal(const image_info& remoteImage, image_info &localImageInfo) const
{
	memcpy(&localImageInfo, &remoteImage, sizeof(remoteImage));
	if (strlen(fRemoteBinRoot) && !strncmp(localImageInfo.name, "/boot", 5))
	{
		const char* p = remoteImage.name + 5;		// the part after the "/boot"
		strcpy(localImageInfo.name, fRemoteBinRoot);
		strcat(localImageInfo.name, p);
	}
}

// #pragma mark -
//ProxyNubTCPLooper implementation
ProxyNubTCPLooper::ProxyNubTCPLooper(uint16 listenPort, port_id responsePort,
	TCPMessenger* proxyMessenger, DRemoteProxyNub& myNub)
	: TCPLooper(listenPort), fResponsePort(responsePort), fMessenger(proxyMessenger), fNub(myNub)
{
}

void 
ProxyNubTCPLooper::MessageReceived(BMessage* msg)
{
	status_t err, result;

	dfprintf(stderr, "*** ProxyNubTCPLooper got message 0x%08lx = '%c%c%c%c':\n", msg->what,
		char((msg->what & 0xFF000000) >> 24),
		char((msg->what & 0x00FF0000) >> 16),
		char((msg->what & 0x0000FF00) >> 8),
		char((msg->what & 0x000000FF)));

	switch (msg->what)
	{
	// messages to be forwarded to the remote proxy
	case BDB_MSG_ATTACH:
	case BDB_MSG_STOP_THREAD:
	case BDB_MSG_KILL_THREAD:
	case BDB_MSG_READ_DATA:
	case BDB_MSG_WRITE_DATA:
	case BDB_MSG_SET_BREAKPOINT:
	case BDB_MSG_CLEAR_BREAKPOINT:
	case BDB_MSG_SET_WATCHPOINT:
	case BDB_MSG_CLEAR_WATCHPOINT:
	case BDB_MSG_GET_THREAD_REGS:
	case BDB_MSG_GET_THREAD_INFO:
	case BDB_MSG_GET_THREAD_LIST:
	case BDB_MSG_GET_IMAGE_LIST:
	case BDB_MSG_RUN:
	case BDB_MSG_STEP:
	case BDB_MSG_STEP_OVER:
	case BDB_MSG_STEP_OUT:
		// if there's a transmission error, we have to cleanly report that error, which we
		// do by deleting the response port out from under the caller [who is holding
		// our lock, and blocked on the port waiting for a response]
		err = fMessenger->SendMessage(msg);
		if (err)
		{
			port_id oldPort = fResponsePort;
			fResponsePort = create_port(4, "DRemoteProxyNub response port");
			delete_port(oldPort);		// reader will get a BAD_PORT error now
		}
		break;

	// these two messages don't return responses at all
	case BDB_MSG_KILL_TARGET:
	case BDB_MSG_DETACH:
		fMessenger->SendMessage(msg);
		break;

	// responses from the remote proxy

	// all of these responses are just a "result" status_t member
	case BDB_MSG_RESPONSE_STOP_THREAD:
	case BDB_MSG_RESPONSE_KILL_THREAD:
	case BDB_MSG_RESPONSE_WRITE_DATA:
	case BDB_MSG_RESPONSE_SET_BREAKPOINT:
	case BDB_MSG_RESPONSE_CLEAR_BREAKPOINT:
	case BDB_MSG_RESPONSE_SET_WATCHPOINT:
	case BDB_MSG_RESPONSE_CLEAR_WATCHPOINT:
	case BDB_MSG_RESPONSE_RUN:
	case BDB_MSG_RESPONSE_STEP:
	case BDB_MSG_RESPONSE_STEP_OVER:
	case BDB_MSG_RESPONSE_STEP_OUT:
		err = msg->FindInt32("result", &result);
		if (err) result = err;
		write_port(fResponsePort, 0, &result, sizeof(result));
		break;

	// these three responses pass through the BMessage* itself
	case BDB_MSG_RESPONSE_READ_DATA:
	case BDB_MSG_RESPONSE_ATTACH:
	case BDB_MSG_RESPONSE_GET_THREAD_REGS:
	case BDB_MSG_RESPONSE_GET_THREAD_INFO:
	case BDB_MSG_RESPONSE_GET_THREAD_LIST:
	case BDB_MSG_RESPONSE_GET_IMAGE_LIST:
		DetachCurrentMessage();
		write_port(fResponsePort, 0, &msg, sizeof(msg));
		break;

	// messages originating on the remote proxy get handled in a separate thread
	// because their handling might require talking to the nub, i.e. cause a deadlock.
	case B_THREAD_STOPPED:
	case B_THREAD_CREATED:
	case B_THREAD_DELETED:
	case B_ELF_IMAGE_CREATED:
	case B_ELF_IMAGE_DELETED:
	case B_TEAM_DELETED:
		DetachCurrentMessage();
		write_port(fNub.fNubMessagePort, 0, &msg, sizeof(msg));
		break;

	case BDB_MSG_QUIT_LOOPER:
		Quit();
		break;

	default:
		;
	}
}

//#pragma mark -

//ProxyNubUDPLooper implementation
ProxyNubUDPLooper::ProxyNubUDPLooper(uint16 listenPort, port_id responsePort,
	UDPMessenger* proxyMessenger, DRemoteProxyNub& myNub)
	: UDPLooper(listenPort), fResponsePort(responsePort), fMessenger(proxyMessenger), fNub(myNub)
{
}

void 
ProxyNubUDPLooper::MessageReceived(BMessage* msg)
{
	status_t err, result;

	dfprintf(stderr, "*** ProxyNubUDPLooper got message 0x%08lx = '%c%c%c%c':\n", msg->what,
		char((msg->what & 0xFF000000) >> 24),
		char((msg->what & 0x00FF0000) >> 16),
		char((msg->what & 0x0000FF00) >> 8),
		char((msg->what & 0x000000FF)));

	switch (msg->what)
	{
	// messages to be forwarded to the remote proxy
	case BDB_MSG_ATTACH:
	case BDB_MSG_STOP_THREAD:
	case BDB_MSG_KILL_THREAD:
	case BDB_MSG_READ_DATA:
	case BDB_MSG_WRITE_DATA:
	case BDB_MSG_SET_BREAKPOINT:
	case BDB_MSG_CLEAR_BREAKPOINT:
	case BDB_MSG_SET_WATCHPOINT:
	case BDB_MSG_CLEAR_WATCHPOINT:
	case BDB_MSG_GET_THREAD_REGS:
	case BDB_MSG_GET_THREAD_INFO:
	case BDB_MSG_GET_THREAD_LIST:
	case BDB_MSG_GET_IMAGE_LIST:
	case BDB_MSG_RUN:
	case BDB_MSG_STEP:
	case BDB_MSG_STEP_OVER:
	case BDB_MSG_STEP_OUT:
		// if there's a transmission error, we have to cleanly report that error, which we
		// do by deleting the response port out from under the caller [who is holding
		// our lock, and blocked on the port waiting for a response]
		err = fMessenger->SendMessage(msg);
		if (err)
		{
			port_id oldPort = fResponsePort;
			fResponsePort = create_port(4, "DRemoteProxyNub response port");
			delete_port(oldPort);		// reader will get a BAD_PORT error now
		}
		break;

	// these two messages don't return responses at all
	case BDB_MSG_KILL_TARGET:
	case BDB_MSG_DETACH:
		fMessenger->SendMessage(msg);
		break;

	// responses from the remote proxy

	// all of these responses are just a "result" status_t member
	case BDB_MSG_RESPONSE_STOP_THREAD:
	case BDB_MSG_RESPONSE_KILL_THREAD:
	case BDB_MSG_RESPONSE_WRITE_DATA:
	case BDB_MSG_RESPONSE_SET_BREAKPOINT:
	case BDB_MSG_RESPONSE_CLEAR_BREAKPOINT:
	case BDB_MSG_RESPONSE_SET_WATCHPOINT:
	case BDB_MSG_RESPONSE_CLEAR_WATCHPOINT:
	case BDB_MSG_RESPONSE_RUN:
	case BDB_MSG_RESPONSE_STEP:
	case BDB_MSG_RESPONSE_STEP_OVER:
	case BDB_MSG_RESPONSE_STEP_OUT:
		err = msg->FindInt32("result", &result);
		if (err) result = err;
		write_port(fResponsePort, 0, &result, sizeof(result));
		break;

	// these three responses pass through the BMessage* itself
	case BDB_MSG_RESPONSE_READ_DATA:
	case BDB_MSG_RESPONSE_ATTACH:
	case BDB_MSG_RESPONSE_GET_THREAD_REGS:
	case BDB_MSG_RESPONSE_GET_THREAD_INFO:
	case BDB_MSG_RESPONSE_GET_THREAD_LIST:
	case BDB_MSG_RESPONSE_GET_IMAGE_LIST:
		DetachCurrentMessage();
		write_port(fResponsePort, 0, &msg, sizeof(msg));
		break;

	// messages originating on the remote proxy get handled in a separate thread
	// because their handling might require talking to the nub, i.e. cause a deadlock.
	case B_THREAD_STOPPED:
	case B_THREAD_CREATED:
	case B_THREAD_DELETED:
	case B_ELF_IMAGE_CREATED:
	case B_ELF_IMAGE_DELETED:
	case B_TEAM_DELETED:
		DetachCurrentMessage();
		write_port(fNub.fNubMessagePort, 0, &msg, sizeof(msg));
		break;

	case BDB_MSG_QUIT_LOOPER:
		Quit();
		break;

	default:
		;
	}
}

//#pragma mark -

int32 
DRemoteProxyNub::nub_message_thread(void *arg)
{
	static_cast<DRemoteProxyNub*>(arg)->HandleNubMessages();
	return 0;
}

void 
DRemoteProxyNub::HandleNubMessages()
{
	BMessage* msg;
	int32 code;
	while (read_port(fNubMessagePort, &code, &msg, sizeof(msg)) >= 0)
	{
		fTeam->Lock();
		thread_id tid = msg->FindInt32("thread");		// may fail, but we don't care
		image_info info;
		const void* infoPtr;
		ssize_t size;
		if (msg->FindData("image", B_RAW_TYPE, &infoPtr, &size) == B_OK)
		{
			memcpy(&info, infoPtr, sizeof(image_info));
		}

		switch (msg->what)
		{
		case B_THREAD_STOPPED:
		{
			db_why_stopped why = (db_why_stopped) msg->FindInt32("why");

			const void* flatCpu;
			msg->FindData("cpu", B_RAW_TYPE, &flatCpu, &size);
			DCpuState* cpuState = DCpuState::Unflatten(flatCpu);

			void* otherData = (void*) msg->FindInt32("data");

			// The DThread object will take ownership of 'cpuState', 'otherData' isn't really
			// a pointer to data, although it looks that way in <debugger.h>
			fTeam->ThreadStopped(tid, why, cpuState, otherData);
			break;
		}

		case B_THREAD_CREATED:
			fTeam->ThreadCreated(tid);
			break;

		case B_THREAD_DELETED:
			fTeam->ThreadDeleted(tid);
			break;

		case B_ELF_IMAGE_CREATED:
			fTeam->ImageCreated(info);
			break;

		case B_ELF_IMAGE_DELETED:
			fTeam->ImageDeleted(info);
			break;

		case B_TEAM_DELETED:
			fTeam->TeamDeleted();
			break;
		}
		fTeam->Unlock();

		delete msg;
	}
}

