// remote debugger proxy

#include "RemoteProtocol.h"
#include <TCPLooper.h>
#include <TCPMessenger.h>
#include <Message.h>
#include <Application.h>
#include <Entry.h>
#include <Path.h>
#include <Locker.h>
#include <Autolock.h>
#include <debugger.h>
#include <Debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

extern "C" void _kget_thread_registers_(thread_id, cpu_state*);

#if DEBUG
#define dfprintf fprintf
#else
inline void dfprintf(FILE*, ...) { }
#endif

// Utility function -- check an error, and bail if it's not OK
static void QuitOnError(status_t err, const char* message)
{
	if (err < 0)
	{
		fprintf(stderr, "ERROR: %s [0x%lx, %s]\n", message, (unsigned long) err, strerror(err));
		exit(err);
	}
}

// ProxyLooper listens for incoming TCP messages and bucks them asynchronously
// to the application looper.  It also spins off a thread that listens for nub-originated
// debugging events, and similarly forwards them to the application for processing.
class ProxyLooper : public TCPLooper
{
public:
	ProxyLooper(uint16 listenPort);
	void MessageReceived(BMessage* msg);

	void StartNubListener(port_id fromNubPort);

private:
	// static method that listens for nub-originated messages
	static long NubListener(void*);
	long Listen();

	BLocker fLock;
	port_id fFromNubPort;
	thread_id fNubListener;
	bool fListening;
};

ProxyLooper::ProxyLooper(uint16 listenPort)
	: TCPLooper(listenPort), fLock(true)
{
	fNubListener = spawn_thread(NubListener, "Nub Listener", B_NORMAL_PRIORITY, this);
}

void 
ProxyLooper::StartNubListener(port_id fromNubPort)
{
	fFromNubPort = fromNubPort;
	fListening = true;
	resume_thread(fNubListener);
}

void 
ProxyLooper::MessageReceived(BMessage *msg)
{
	dfprintf(stderr, "*** bdbproxy TCP looper got message (%ld = '%c%c%c%c')\n", msg->what,
		(msg->what & 0xFF000000) >> 24,
		(msg->what & 0x00FF0000) >> 16,
		(msg->what & 0x0000FF00) >> 8,
		msg->what & 0x000000FF);

	switch (msg->what)
	{
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
	case BDB_MSG_KILL_TARGET:
	case BDB_MSG_DETACH:
		be_app->PostMessage(msg);
		break;

	default:
#if DEBUG
fprintf(stderr, "* Unforwarded TCP message:\n");
msg->PrintToStream();
#endif
		break;
	}
}

long 
ProxyLooper::NubListener(void* obj)
{
	return static_cast<ProxyLooper*>(obj)->Listen();
}

long 
ProxyLooper::Listen()
{
	to_debugger_msg msg;
	long what;

	dfprintf(stderr, "<<< bdbproxy starting to listen for nub messages on port %d\n", fFromNubPort);
	while (fListening && (read_port(fFromNubPort, &what, &msg, sizeof(msg)) > 0))
	{
		BMessage bmsg(what);

		BAutolock lock(fLock);
		if (!fListening) break;

		if (what == B_THREAD_STOPPED)		// entities unique to thread-stopped messages
		{
			bmsg.AddInt32("port", msg.thread_stopped.nub_port);
			bmsg.AddInt32("why", msg.thread_stopped.why);
			bmsg.AddInt32("data", (int32) msg.thread_stopped.data);

			// The portable cpu-state representation used by bdb and bdbproxy
			// is a 2-byte architecture identifier followed by the binary architecture-
			// specific cpu_state structure.
			char flat[2 + sizeof(cpu_state)];
#if __INTEL__
			*((uint16*) flat) = FLAT_INTEL_CPUSTATE;
#elif __arm__
			*((uint16*) flat) = FLAT_ARM_CPUSTATE;
#else
#error Unsupported target architecture
#endif
			memcpy(flat + 2, &msg.thread_stopped.cpu, sizeof(cpu_state));
			bmsg.AddData("cpu", B_RAW_TYPE, flat, 2 + sizeof(cpu_state));
		}

		// entities common to B_THREAD_STOPPED, B_THREAD_CREATED, B_THREAD_DELETED
		if ((what == B_THREAD_STOPPED) ||
			(what == B_THREAD_CREATED) ||
			(what == B_THREAD_DELETED))
		{
#warning Relies on the thread member being in the same place in all three union variants
			bmsg.AddInt32("thread", msg.thread_stopped.thread);
		}

		// entities unique to the IMAGE_CREATED and IMAGE_DELETED messages
		if ((what == B_ELF_IMAGE_CREATED) || (what == B_ELF_IMAGE_DELETED))
		{
#warning Relies on the info member being in the same place in both union variants
			bmsg.AddData("image", B_RAW_TYPE, &msg.pef_image_created.info, sizeof(image_info));
			bmsg.AddInt32("token", msg.pef_image_created.reply_token);
		}

		// B_TEAM_DELETED doesn't have any data attached

		switch (what)
		{
		case B_TEAM_DELETED:
			// B_TEAM_DELETED is our cue to quit listening, so we set the termination flag
			fListening = false;
			// ...now deliberately fall through, because all of these messages just get bucked
			// over to the app thread for processing

		case B_THREAD_STOPPED:
		case B_THREAD_CREATED:
		case B_THREAD_DELETED:
		case B_ELF_IMAGE_CREATED:
		case B_ELF_IMAGE_DELETED:
			be_app->PostMessage(&bmsg);
			break;

		case -1:
		default:
			break;
		}
	}

	return 0;
}

// #pragma mark -
//the ProxyApp actually processes messages, to decouple them from the
// TCP and kernel nub listeners

class ProxyApp : public BApplication
{
public:
	ProxyApp(uint16 listenPort);
	void ReadyToRun();
	void MessageReceived(BMessage* msg);
	void ArgvReceived(int32 argc, char **argv);

	void StopThread(thread_id thread) const;
	void KillThread(thread_id thread) const;
	void ReadData(uint32 address, uint32 size) const;
	void WriteData(uint32 address, const void* data, uint32 size) const;
	void SetBreakpoint(uint32 address) const;
	void ClearBreakpoint(uint32 address) const;
	void SetWatchpoint(uint32 address) const;
	void ClearWatchpoint(uint32 address) const;
	void GetThreadRegisters(thread_id thread) const;
	void GetThreadInfo(thread_id thread) const;
	void GetThreadList(team_id team) const;
	void GetImageList(team_id team) const;
//	void ToNubMessage(int what, const to_nub_msg& msg) const;
	void RunThread(thread_id tid, cpu_state& cpu) const;
	void Step(thread_id tid, cpu_state& cpu, uint32 low, uint32 high) const;
	void StepOver(thread_id tid, cpu_state& cpu, uint32 low, uint32 high) const;
	void StepOut(thread_id tid, cpu_state& cpu) const;

private:
	TCPMessenger* fMessenger;
	ProxyLooper* fProxyLooper;
	uint16 fListenPort;
	team_id fTeam;		// team that we're attached to
	port_id fFromNubPort;		// communciation from the kernel nub
	port_id fToNubPort;			// communication *to* the kernel nub
	port_id fReply;
	thread_id fMainThread;	// sublaunched app's main thread
	bool fNeedsLaunch;			// if we need to resume the thread when first hitting 'run'
};

ProxyApp::ProxyApp(uint16 listenPort)
	: BApplication("application/x-vnd.Be-debugger-proxy"),
	fMessenger(NULL), fListenPort(listenPort), fTeam(0), fFromNubPort(-1), fToNubPort(-1),
	fMainThread(0), fNeedsLaunch(false)
{
	fReply = create_port(5, "reply");
}

void 
ProxyApp::ReadyToRun()
{
	fProxyLooper = new ProxyLooper(fListenPort);
	fProxyLooper->Run();
}

void 
ProxyApp::ArgvReceived(int32 argc, char **argv)
{
	team_info teamInfo;

	if (!strcmp(argv[1], "-p"))
	{
		// argv[1] == "-p"
		// argv[2] == nub port id, same as bdb's argv[2]
		// argv[3] == target team id, same as bdb's argv[3]
		if (argc != 4)
		{
			QuitOnError(B_BAD_VALUE, "Invalid arguments given");
		}

		fFromNubPort = atoi(argv[2]);
		fTeam = atoi(argv[3]);

		// don't let people try to debug the kernel team, which is always team_id 2
#warning This assumes the kernel team is always team_id 2
		if (fTeam == 2)
		{
			QuitOnError(B_BAD_VALUE, "You can't debug the kernel team");
		}

		// verify that these are valid team & port IDs
		status_t err = get_team_info(fTeam, &teamInfo);
		QuitOnError(err, "Invalid team ID");

		port_info portInfo;
		err = get_port_info(fFromNubPort, &portInfo);
		QuitOnError(err, "Invalid port ID");
	}
	else if (!strcmp(argv[1], "-t"))
	{
		// argv[1] = "-t"
		// argv[2] = target team id
		if (argc != 3)
		{
			QuitOnError(B_BAD_VALUE, "Invalid arguments given");
		}

		fTeam = atoi(argv[2]);
		if (fTeam == 2)
		{
			QuitOnError(B_BAD_VALUE, "You can't debug the kernel team");
		}

		status_t err = get_team_info(fTeam, &teamInfo);
		QuitOnError(err, "Invalid team ID");

		fFromNubPort = create_port(5, "debug port");
		QuitOnError(fFromNubPort, "Invalid port ID");

		// install ourselves as the team debugger for it
		fToNubPort = install_team_debugger(fTeam, fFromNubPort);
		QuitOnError(fToNubPort, "Unable to install team debugger");

		// we do NOT need to resume_thread() it when attaching
		fNeedsLaunch = false;
	}
	else		// sublaunching an app under the debugger; args are the app to launch
	{
		// try to find the app that we're trying to sublaunch
		entry_ref ref;
		status_t appFound = B_LAUNCH_FAILED_APP_NOT_FOUND;

		if (get_ref_for_path(argv[1], &ref) == B_OK)
		{
			BEntry tempEntry(&ref);
			if (tempEntry.Exists())
			{
				appFound = B_OK;
			}
		}

		QuitOnError(appFound, "Unable to find application");

		// set up the argv array for the new team
		const char** subArgv = new (const char*)[argc];

		// argv[0] is the path to the binary
		BPath p;
		BEntry(&ref).GetPath(&p);
		subArgv[0] = (char*) p.Path();

		// copy out the rest of the args.  argv[argc] == NULL,
		// the end-of-args marker, so this is safe.
		for (int i = 2; i <= argc; i++)
		{
			subArgv[i -1] = argv[i];
		}

		// launch the new app, inheriting our environment
		fMainThread = load_image(argc - 1, subArgv, (const char**) environ);
		QuitOnError(fMainThread, "Unable to load application image");

		// install as the team debugger, and make sure it drops into debug mode
		// as soon as it's activated [which happens when bdb attaches]
		thread_info info;
		get_thread_info(fMainThread, &info);
		fTeam = info.team;
		fFromNubPort = create_port(5, "debug port");
		QuitOnError(fFromNubPort, "Invalid port ID");
		fToNubPort = install_team_debugger(fTeam, fFromNubPort);
		QuitOnError(fToNubPort, "Unable to install team debugger");
		debug_thread(fMainThread);		// will be resumed & stop immediately when Attach() is called
		fNeedsLaunch = true;

		delete [] subArgv;
	}
}

void 
ProxyApp::MessageReceived(BMessage *msg)
{
	// many actions need the "address" data, so we just try to extract it here
	uint32 address = 0;
	msg->FindInt32("address", (int32*) &address);

	switch (msg->what)
	{
	// messages from bdb are reposted here so that they can be handled asynchronously
	// with the TCPLooper's operation.
	case BDB_MSG_ATTACH:
	{
		// "reply_ip" == ip address in *network* byte order
		// "reply_port" = tcp port in *network* byte order

		uint32 reply_ip;
		uint32 reply_port;
		msg->FindInt32("reply_ip", (int32*) &reply_ip);
		msg->FindInt32("reply_port", (int32*) &reply_port);

		dfprintf(stderr, "*** bdbproxy got ATTACH message:\n\treply_ip = %d.%d.%d.%d\n\treply_port = %d\n",
			(reply_ip & 0x000000FF),
			(reply_ip & 0x0000FF00) >> 8,
			(reply_ip & 0x00FF0000) >> 16,
			(reply_ip & 0xFF000000) >> 24,
			ntohs(reply_port));

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = reply_ip;
		addr.sin_port = (in_port_t) reply_port;
		fMessenger = new TCPMessenger(addr);

		// the ATTACH response is a message with all images' info
		BMessage resp(BDB_MSG_RESPONSE_ATTACH);		
		image_info info;
		long cookie = 0;
		while (get_next_image_info(fTeam, &cookie, &info) == B_OK)
		{
			resp.AddData("image", B_RAW_TYPE, &info, sizeof(info));
		}
		resp.AddInt32("team", fTeam);
		fMessenger->SendMessage(&resp);

		if (fNeedsLaunch)
		{
			fNeedsLaunch = false;
			resume_thread(fMainThread);
		}

		// last of all, start listening for (and forwarding) debug messages
		fProxyLooper->StartNubListener(fFromNubPort);
		break;
	}

	case BDB_MSG_STOP_THREAD:
	{
		thread_id thread;
		msg->FindInt32("thread", (int32*) &thread);
		StopThread(thread);
		break;
	}

	case BDB_MSG_KILL_THREAD:
	{
		thread_id thread;
		msg->FindInt32("thread", (int32*) &thread);
		KillThread(thread);
		break;
	}

	case BDB_MSG_READ_DATA:
	{
		uint32 size;
		msg->FindInt32("size", (int32*) &size);
		ReadData(address, size);
		break;
	}

	case BDB_MSG_WRITE_DATA:
	{
		ssize_t size;
		const void* data;
		msg->FindData("data", B_RAW_TYPE, &data, &size);
		WriteData(address, data, size);
		break;
	}

	case BDB_MSG_SET_BREAKPOINT:
		SetBreakpoint(address);
		break;

	case BDB_MSG_CLEAR_BREAKPOINT:
		ClearBreakpoint(address);
		break;

	case BDB_MSG_SET_WATCHPOINT:
		SetWatchpoint(address);
		break;

	case BDB_MSG_CLEAR_WATCHPOINT:
		ClearWatchpoint(address);
		break;

	case BDB_MSG_GET_THREAD_REGS:
	{
		thread_id thread;
		msg->FindInt32("thread", (int32*) &thread);
		GetThreadRegisters(thread);
		break;
	}

	case BDB_MSG_GET_THREAD_INFO:
	{
		thread_id thread;
		msg->FindInt32("thread", (int32*) &thread);
		GetThreadInfo(thread);
		break;
	}

	case BDB_MSG_GET_THREAD_LIST:
	{
		team_id team;
		msg->FindInt32("team", (int32*) &team);
		GetThreadList(team);
		break;
	}

	case BDB_MSG_GET_IMAGE_LIST:
	{
		team_id team;
		msg->FindInt32("team", (int32*) &team);
		GetImageList(team);
		break;
	}

	// These four are all quite similar
	case BDB_MSG_RUN:
	case BDB_MSG_STEP:
	case BDB_MSG_STEP_OVER:
	case BDB_MSG_STEP_OUT:
	{
		cpu_state cpu;
		const char* ptr;
		ssize_t size;
		msg->FindData("cpu", B_RAW_TYPE, (const void**) &ptr, &size);
		memcpy(&cpu, ptr + 2, sizeof(cpu));		// we can assume that it's the right kind of cpu_state
		thread_id tid = (thread_id) msg->FindInt32("thread");
		uint32 low = (uint32) msg->FindInt32("low");		// these two may fail, but that's okay
		uint32 high = (uint32) msg->FindInt32("high");
		switch (msg->what)
		{
			case BDB_MSG_RUN:					RunThread(tid, cpu); break;
			case BDB_MSG_STEP:					Step(tid, cpu, low, high); break;
			case BDB_MSG_STEP_OVER:		StepOver(tid, cpu, low, high); break;
			case BDB_MSG_STEP_OUT:		StepOut(tid, cpu); break;
		}
		break;
	}

	// In both of these cases, we quit the proxy
	case BDB_MSG_KILL_TARGET:
		write_port(fToNubPort, -1, NULL, 0);
		Quit();
		break;

	case BDB_MSG_DETACH:
		remove_team_debugger(fTeam);
		Quit();
		break;

	// messages that originate in the nub also get handled in this thread;
	// mostly this means forwarding the message to bdb
	case B_THREAD_STOPPED:
	// fall through
	case B_THREAD_CREATED:
	case B_THREAD_DELETED:
	case B_ELF_IMAGE_CREATED:
	case B_ELF_IMAGE_DELETED:
	case B_TEAM_DELETED:
		if (fMessenger)
		{
			fMessenger->SendMessage(msg);
			if (msg->what == B_THREAD_STOPPED)
			{
				msg->FindInt32("port", &fToNubPort);
			}
			else if (msg->what == B_ELF_IMAGE_CREATED)
			{
				nub_acknowlege_image_created_msg reply;
				msg->FindInt32("token", &reply.token);
				write_port(fToNubPort, B_ACKNOWLEGE_IMAGE_CREATED, &reply, sizeof(reply));
			}
			else if (msg->what == B_TEAM_DELETED)
			{
				Quit();
			}
		}
		break;

	default:
		break;
	}
}

// Utility function, to avoid code duplication
static void SendResult(TCPMessenger* messenger, uint32 what, long result)
{
	BMessage resp(what);
	resp.AddInt32("result", result);
	messenger->SendMessage(&resp);
}

// These are all called only by the app's MessageReceived(), so access to the nub is
// serialized through the app's looper lock.
void 
ProxyApp::StopThread(thread_id thread) const
{
	debug_thread(thread);
	SendResult(fMessenger, BDB_MSG_RESPONSE_STOP_THREAD, B_OK);
}

void 
ProxyApp::KillThread(thread_id thread) const
{
	kill_thread(thread);
	SendResult(fMessenger, BDB_MSG_RESPONSE_STOP_THREAD, B_OK);
}

void 
ProxyApp::ReadData(uint32 address, uint32 size) const
{
	long what;
	nub_read_memory_msg msg;

	// find the pages that contain the specified address range
	uint32 base = address & 0xFFFFF000;
	uint32 last = (address + size) & 0xFFFFF000;
	msg.addr = (char*) base;
	msg.count = last - base + B_PAGE_SIZE;
	msg.reply_port = fReply;

	unsigned char* buffer = new unsigned char[msg.count];

	write_port(fToNubPort, B_READ_MEMORY, &msg, sizeof(msg));
	read_port(fReply, &what, buffer, msg.count);

	BMessage resp(BDB_MSG_RESPONSE_READ_DATA);
	resp.AddInt32("address", (int32) msg.addr);
	resp.AddData("data", B_RAW_TYPE, buffer, msg.count);
	fMessenger->SendMessage(&resp);

	delete [] buffer;
}

void 
ProxyApp::WriteData(uint32 address, const void *data, uint32 size) const
{
	nub_write_memory_msg msg;
	long what;

	msg.addr = (char*) address;
	msg.count = size;
	msg.reply_port = fReply;

	write_port(fToNubPort, B_WRITE_MEMORY, &msg, sizeof(msg));
	write_port(fToNubPort, 0, data, size);
	read_port(fReply, &what, NULL, 0);

	SendResult(fMessenger, BDB_MSG_RESPONSE_WRITE_DATA, what);
}

void 
ProxyApp::SetBreakpoint(uint32 address) const
{
	nub_set_breakpoint_msg msg;
	msg.addr = (char*) address;
	msg.reply_port = fReply;

	long what;
	write_port(fToNubPort, B_SET_BREAKPOINT, &msg, sizeof(msg));
	read_port(fReply, &what, NULL, 0);

	SendResult(fMessenger, BDB_MSG_RESPONSE_SET_BREAKPOINT, what);
}

void 
ProxyApp::ClearBreakpoint(uint32 address) const
{
	nub_clear_breakpoint_msg msg;
	msg.addr = (char*) address;
	msg.reply_port = fReply;

	long what;
	write_port(fToNubPort, B_CLEAR_BREAKPOINT, &msg, sizeof(msg));
	read_port(fReply, &what, NULL, 0);

	SendResult(fMessenger, BDB_MSG_RESPONSE_CLEAR_BREAKPOINT, what);
}

#define WATCHPOINT_READ 0x01;
#define WATCHPOINT_WRITE 0x02;

void 
ProxyApp::SetWatchpoint(uint32 address) const
{
	nub_set_watchpoint_msg msg;
	msg.addr = (char*) address;
	msg.type = WATCHPOINT_WRITE;
	msg.reply_port = fReply;
	write_port(fToNubPort, B_SET_WATCHPOINT, &msg, sizeof(msg));

	long what;
	read_port(fReply, &what, NULL, 0);
	SendResult(fMessenger, BDB_MSG_RESPONSE_SET_WATCHPOINT, what);
}

void 
ProxyApp::ClearWatchpoint(uint32 address) const
{
	nub_clear_watchpoint_msg msg;
	msg.addr = (char*) address;
	msg.reply_port = fReply;
	write_port(fToNubPort, B_CLEAR_WATCHPOINT, &msg, sizeof(msg));

	long what;
	read_port(fReply, &what, NULL, 0);
	SendResult(fMessenger, BDB_MSG_RESPONSE_CLEAR_WATCHPOINT, what);
}

void 
ProxyApp::GetThreadRegisters(thread_id thread) const
{
	cpu_state cpu;
	_kget_thread_registers_(thread, &cpu);

	BMessage resp(BDB_MSG_RESPONSE_GET_THREAD_REGS);

	// The portable cpu-state representation used by bdb and bdbproxy
	// is a 2-byte architecture identifier followed by the binary architecture-
	// specific cpu_state structure.
	unsigned char stateBuffer[2 + sizeof(cpu)];
#if __INTEL__
	*((uint16*)stateBuffer) = FLAT_INTEL_CPUSTATE;
#elif __arm__
	*((uint16*)stateBuffer) = FLAT_ARM_CPUSTATE;
#else
#error Unsupported platform!
#endif
	memcpy(stateBuffer + 2, &cpu, sizeof(cpu));

	resp.AddData("cpu", B_RAW_TYPE, stateBuffer, sizeof(cpu));
	fMessenger->SendMessage(&resp);
}

void 
ProxyApp::GetThreadInfo(thread_id thread) const
{
	BMessage resp(BDB_MSG_RESPONSE_GET_THREAD_INFO);
	thread_info info;
	if (get_thread_info(thread, &info) == B_OK)
	{
		resp.AddData("tinfo", B_RAW_TYPE, &info, sizeof(info));
	}
	fMessenger->SendMessage(&resp);
}

void 
ProxyApp::GetThreadList(team_id team) const
{
	BMessage resp(BDB_MSG_RESPONSE_GET_THREAD_LIST);
	thread_info info;
	int32 cookie = 0;
	while (get_next_thread_info(team, &cookie, &info) == B_OK)
	{
		resp.AddInt32("tid", info.thread);
		resp.AddString("tname", info.name);
	}
	fMessenger->SendMessage(&resp);
}

void 
ProxyApp::GetImageList(team_id team) const
{
	BMessage resp(BDB_MSG_RESPONSE_GET_IMAGE_LIST);
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(team, &cookie, &info) == B_OK)
	{
		resp.AddData("image", B_RAW_TYPE, &info, sizeof(info));
	}
	fMessenger->SendMessage(&resp);
}

void 
ProxyApp::RunThread(thread_id tid, cpu_state &cpu) const
{
	to_nub_msg msg;
	msg.nub_run_thread.thread = tid;
	msg.nub_run_thread.align_to_double = 0;
	msg.nub_run_thread.cpu = cpu;

	status_t err = write_port_etc(fToNubPort, B_RUN_THREAD, &msg, sizeof(msg), B_TIMEOUT, 5000000);
	SendResult(fMessenger, BDB_MSG_RESPONSE_RUN, err);
}

void 
ProxyApp::Step(thread_id tid, cpu_state &cpu, uint32 low, uint32 high) const
{
	to_nub_msg msg;
	msg.nub_step_thread.thread = tid;
	msg.nub_step_thread.align_to_double = 0;
	msg.nub_step_thread.cpu = cpu;
	msg.nub_step_thread.low_pc = (char*) low;
	msg.nub_step_thread.high_pc = (char*) high;

	status_t err = write_port_etc(fToNubPort, B_STEP_THREAD, &msg, sizeof(msg), B_TIMEOUT, 5000000);
	SendResult(fMessenger, BDB_MSG_RESPONSE_STEP, err);
}

void 
ProxyApp::StepOver(thread_id tid, cpu_state &cpu, uint32 low, uint32 high) const
{
	to_nub_msg msg;
	msg.nub_step_thread.thread = tid;
	msg.nub_step_thread.align_to_double = 0;
	msg.nub_step_thread.cpu = cpu;
	msg.nub_step_thread.low_pc = (char*) low;
	msg.nub_step_thread.high_pc = (char*) high;

	status_t err = write_port_etc(fToNubPort, B_STEP_OVER_THREAD, &msg, sizeof(msg), B_TIMEOUT, 5000000);
	SendResult(fMessenger, BDB_MSG_RESPONSE_STEP_OVER, err);
}

void 
ProxyApp::StepOut(thread_id tid, cpu_state &cpu) const
{
	to_nub_msg msg;
	msg.nub_step_thread.thread = tid;
	msg.nub_step_thread.align_to_double = 0;
	msg.nub_step_thread.cpu = cpu;

	status_t err = write_port_etc(fToNubPort, B_STEP_OUT_THREAD, &msg, sizeof(msg), B_TIMEOUT, 5000000);
	SendResult(fMessenger, BDB_MSG_RESPONSE_STEP_OUT, err);
}

// #pragma mark -

int main(int argc, char**)
{
	if (argc <2)
	{
		QuitOnError(B_ERROR, "No arguments given");
	}

	ProxyApp app(5038);		// !!! derive this from arguments
	app.Run();
	return 0;
}
