/*	$Id: DNub.cpp,v 1.8 1999/02/11 15:51:54 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/10/98 14:56:15
*/

#include "bdb.h"
#include "DLocalNub.h"
#include "DTeam.h"
#include "DMessages.h"
#include "DCpuState.h"

extern "C" void _kget_thread_registers_(thread_id, cpu_state*);

#include <iostream>
#include <strstream>

DLocalNub::DLocalNub(port_id toNubPort, port_id fromNubPort, DTeam *team)
	: DNub(),
	fToNubPort(toNubPort), fFromNubPort(fromNubPort), fTeam(team)
{
	// in the case of the hand-off, we can race ahead of the Server thread that's
	// listening for nub messages, leading to mistakes/bugs setting up breakpoints
	// et cetera.  fNubReady is deleted once we actually know the debug nub port,
	// and in the meantime any operation that needs it will be blocked waiting for that
	// to happen.
	fNubReady = (toNubPort >= 0) ? -1 : create_sem(0, "nub-ready signal port");

	FailOSErr2(fReply = create_port(5, "reply port"));
	fServer = spawn_thread(Server, "Nub Listener", B_NORMAL_PRIORITY, this);
}

void DLocalNub::Start()
{
	// In normal circumstances, Start should just be part of the constructor
	// however, in the case of debug_server handoff, we get back a thread stopped
	// message so quickly, the result of the constructor hasn't been saved in
	// the DTeam.  Rather than create a whole new locking mechanism just for
	// this one case, I just separate the calls.
	FailOSErrMsg(resume_thread(fServer), "Failed to start debug thread");
} /* DLocalNub::DLocalNub */

DLocalNub::~DLocalNub()
{
} /* DLocalNub::DLocalNub */

void 
DLocalNub::StopThread(thread_id thread)
{
	debug_thread(thread);
}

void 
DLocalNub::KillThread(thread_id thread)
{
	kill_thread(thread);
}

void DLocalNub::ReadData(ptr_t address, void *buffer, size_t size)
{
	if (! IsLocked())
		THROW(("Nub should be locked first!"));

	// make sure we know fToNubPort before proceeding
	WaitForNub();

	if (fToNubPort < 0)
		THROW(("Nub was already closed"));

	nub_read_memory_msg msg;
	long what;

	msg.addr = (char *)address;
	msg.count = size;
	msg.reply_port = fReply;

	FailOSErr2 (write_port (fToNubPort, B_READ_MEMORY, &msg, sizeof (msg)));
	FailOSErr2 (read_port (fReply, &what, buffer, size));
	FailOSErr2 (what);
} /* DLocalNub::ReadData */

void DLocalNub::WriteData(ptr_t address, const void *buffer, size_t size)
{
	if (! IsLocked())
		THROW(("Nub should be locked first!"));

	// make sure we know fToNubPort before proceeding
	WaitForNub();

	if (fToNubPort < 0)
		THROW(("Nub was already closed"));

	nub_write_memory_msg msg;
	long what;

	msg.addr = (char *)address;
	msg.count = size;
	msg.reply_port = fReply;

	FailOSErr2 (write_port (fToNubPort, B_WRITE_MEMORY, &msg, sizeof (msg)));
	FailOSErr2 (write_port (fToNubPort, 0, buffer, size));
	FailOSErr2 (read_port (fReply, &what, NULL, 0));
	FailOSErr2 (what);
} /* DLocalNub::WriteData */

void DLocalNub::DTeamClosed()
{
	if (fToNubPort >= 0)
	{
		if (!IsLocked())
			THROW(("Nub should be locked first!"));

//		(void)write_port(fToNubPort, -1, NULL, 0);
		fToNubPort = -1;
		fTeam = NULL;
	}
} /* DLocalNub::DTeamClosed */

void DLocalNub::Kill()
{
	if (fToNubPort >= 0)
	{
		if (!IsLocked())
			THROW(("Nub should be locked first!"));

		(void)write_port(fToNubPort, -1, NULL, 0);
		fToNubPort = -1;
	}
} /* DLocalNub::Kill */

void DLocalNub::Detach ()
{
	if (!IsLocked())
		THROW(("Nub should be locked first!"));

	(void)remove_team_debugger(fTeam->GetID());
	fToNubPort = -1;
	fTeam = NULL;
} // DLocalNub::Detach

void 
DLocalNub::Run(thread_id tid, const DCpuState &cpu)
{
#if __INTEL__
	const Dx86CpuState* hostCPU = dynamic_cast<const Dx86CpuState*>(&cpu);
#else
#error Unsupported host architecture!
#endif
	if (!hostCPU)
		THROW(("Passed non-host CPU state"));

	if (!IsLocked())
		THROW(("Nub should be locked first!"));
	WaitForNub();

	to_nub_msg msg;
	msg.nub_run_thread.thread = tid;
	msg.nub_run_thread.align_to_double = 0;
	msg.nub_run_thread.cpu = hostCPU->GetNativeState();

	// !!! make this real
	FailOSErr2 (write_port_etc(fToNubPort, B_RUN_THREAD, &msg, sizeof(msg), B_TIMEOUT, 5000000));
}

void 
DLocalNub::Step(thread_id tid, const DCpuState &cpu, ptr_t lowPC, ptr_t highPC)
{
#if __INTEL__
	const Dx86CpuState* hostCPU = dynamic_cast<const Dx86CpuState*>(&cpu);
#else
#error Unsupported host architecture!
#endif
	if (!hostCPU)
		THROW(("Passed non-host CPU state"));

	if (!IsLocked())
		THROW(("Nub should be locked first!"));
	WaitForNub();

	to_nub_msg msg;
	msg.nub_step_thread.thread = tid;
	msg.nub_step_thread.align_to_double = 0;
	msg.nub_step_thread.cpu = hostCPU->GetNativeState();
	msg.nub_step_thread.low_pc = (char*) lowPC;
	msg.nub_step_thread.high_pc = (char*) highPC;

	// !!! make this real
	FailOSErr2 (write_port_etc(fToNubPort, B_STEP_THREAD, &msg, sizeof(msg), B_TIMEOUT, 5000000));
}

void 
DLocalNub::StepOver(thread_id tid, const DCpuState &cpu, ptr_t lowPC, ptr_t highPC)
{
#if __INTEL__
	const Dx86CpuState* hostCPU = dynamic_cast<const Dx86CpuState*>(&cpu);
#else
#error Unsupported host architecture!
#endif
	if (!hostCPU)
		THROW(("Passed non-host CPU state"));

	if (!IsLocked())
		THROW(("Nub should be locked first!"));
	WaitForNub();

	to_nub_msg msg;
	msg.nub_step_thread.thread = tid;
	msg.nub_step_thread.align_to_double = 0;
	msg.nub_step_thread.cpu = hostCPU->GetNativeState();
	msg.nub_step_thread.low_pc = (char*) lowPC;
	msg.nub_step_thread.high_pc = (char*) highPC;

	// !!! make this real
	FailOSErr2 (write_port_etc(fToNubPort, B_STEP_OVER_THREAD, &msg, sizeof(msg), B_TIMEOUT, 5000000));
}

void 
DLocalNub::StepOut(thread_id tid, const DCpuState &cpu)
{
#if __INTEL__
	const Dx86CpuState* hostCPU = dynamic_cast<const Dx86CpuState*>(&cpu);
#else
#error Unsupported host architecture!
#endif
	if (!hostCPU)
		THROW(("Passed non-host CPU state"));

	if (!IsLocked())
		THROW(("Nub should be locked first!"));
	WaitForNub();

	to_nub_msg msg;
	msg.nub_step_thread.thread = tid;
	msg.nub_step_thread.align_to_double = 0;
	msg.nub_step_thread.cpu = hostCPU->GetNativeState();

	// !!! make this real
	FailOSErr2 (write_port_etc(fToNubPort, B_STEP_OUT_THREAD, &msg, sizeof(msg), B_TIMEOUT, 5000000));
}

void DLocalNub::SetBreakpoint(ptr_t address)
{
	if (!IsLocked())
		THROW(("Nub should be locked first!"));
	WaitForNub();

	if (fToNubPort < 0)
		THROW(("Nub is closed or not yet known"));

	nub_set_breakpoint_msg msg;
	msg.reply_port = fReply;
	msg.addr = (char *)address;
	FailOSErr2(write_port(fToNubPort, B_SET_BREAKPOINT, &msg, sizeof(msg)));
	
	long code;
	FailOSErr2(read_port(fReply, &code, NULL, 0));
	FailOSErr2(code);
} /* DLocalNub::SetBreakpoint */

void DLocalNub::ClearBreakpoint(ptr_t address)
{
	if (!IsLocked())
		THROW(("Nub should be locked first!"));
	WaitForNub();

	if (fToNubPort < 0)
		THROW(("Nub was already closed"));

	nub_clear_breakpoint_msg msg;
	msg.reply_port = fReply;
	msg.addr = (char *)address;
	FailOSErr2(write_port(fToNubPort, B_CLEAR_BREAKPOINT, &msg, sizeof(msg)));
	
	long code;
	FailOSErr2(read_port(fReply, &code, NULL, 0));
	FailOSErr2(code);
} /* DLocalNub::ClearBreakpoint */

enum
{
	WATCHPOINT_READ		= 0x01,
	WATCHPOINT_WRITE	= 0x02
};

void DLocalNub::SetWatchpoint(ptr_t address)
{
	if (!IsLocked())
		THROW(("Nub should be locked first!"));
	WaitForNub();

	if (fToNubPort < 0)
		THROW(("Nub was already closed"));

	nub_set_watchpoint_msg msg;
	msg.reply_port = fReply;
	msg.addr = (char *)address;
	msg.type = WATCHPOINT_WRITE;
	FailOSErr2(write_port(fToNubPort, B_SET_WATCHPOINT, &msg, sizeof(msg)));
	
	long code;
	FailOSErr2(read_port(fReply, &code, NULL, 0));
	FailOSErr2(code);
} /* DLocalNub::SetWatchpoint */

void DLocalNub::ClearWatchpoint(ptr_t address)
{
	if (!IsLocked())
		THROW(("Nub should be locked first!"));
	WaitForNub();

	if (fToNubPort < 0)
		THROW(("Nub was already closed"));

	nub_clear_watchpoint_msg msg;
	msg.reply_port = fReply;
	msg.addr = (char *)address;
	FailOSErr2(write_port(fToNubPort, B_CLEAR_WATCHPOINT, &msg, sizeof(msg)));
	
	long code;
	FailOSErr2(read_port(fReply, &code, NULL, 0));
	FailOSErr2(code);
} /* DLocalNub::ClearWatchpoint */

void 
DLocalNub::GetThreadRegisters(thread_id thread, DCpuState*& outCpu)
{
	cpu_state localCPU;
	_kget_thread_registers_(thread, &localCPU);
	outCpu = DCpuState::LocalCpuState(localCPU);
}

void 
DLocalNub::GetThreadList(team_id team, thread_map &outThreadList)
{
	thread_info info;
	int32 cookie = 0;
	while (get_next_thread_info(team, &cookie, &info) == B_OK)
	{
		outThreadList.push_back(ThreadToken(info.thread, info.name));
	}
}

void 
DLocalNub::GetThreadInfo(thread_id thread, thread_info &outThreadInfo)
{
	get_thread_info(thread, &outThreadInfo);
}

void 
DLocalNub::GetImageList(team_id team, image_list &outImageList)
{
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(team, &cookie, &info) == B_OK)
	{
		outImageList.push_back(info);
	}
}

// Called whenever we need to access  the nub, this method just synchronizes
// with the thread-listening server thread to wait until we actually know the
// kernel port for sending messages to the nub.
void 
DLocalNub::WaitForNub()
{
	// always called with 'this' locked
	if (fNubReady >= 0)
	{
		// fully release 'this' lock to let the server thread proceed
		int32 nLocks = 0;
		while (IsLocked())
		{
			Unlock();
			nLocks++;
		}

		// wait for the nub to become known
		acquire_sem(fNubReady);

		// reacquire the right number of locks before returning
		for (int32 i = 0; i < nLocks; i++)
			Lock();	
	}
}

const char *kDBMsg[] = {
	"B_THREAD_STOPPED",
	"B_TEAM_CREATED",
	"B_TEAM_DELETED",
	"B_ELF_IMAGE_CREATED",
	"B_ELF_IMAGE_DELETED",
	"B_THREAD_CREATED",
	"B_THREAD_DELETED"
};

long DLocalNub::Server(void *data)
{
	return static_cast<DLocalNub*>(data)->Serve();
} /* DLocalNub::Server */

long DLocalNub::Serve()
{
	to_debugger_msg msg;
	long what;

	while (fTeam && read_port(fFromNubPort, &what, &msg, sizeof(msg)) > 0)
	{
		SERIAL_PRINT(("bdb: Got a message from the nub: %s\n",
			what >= 0 && what < 7 ? kDBMsg[what] : "Unknown message!!!"));
		
		BAutolock lock(this);
		
		if (fTeam == NULL)
			break;
		
		try
		{
			switch (what)
			{
				case B_THREAD_STOPPED:
				{
					BAutolock lock(fTeam);
					
					if (lock.IsLocked())
					{
						ASSERT_OR_THROW (msg.thread_stopped.team == fTeam->GetID());
						// In the case of a debug-server hand-off, I don't know the
						// debugger nub until I get the first thread stopped
						if (fToNubPort < 0)
						{
							fToNubPort = msg.thread_stopped.nub_port;
							// release anybody who's waiting for us to learn fToNubPort
							sem_id oldReady = fNubReady;
							fNubReady = -1;
							delete_sem(oldReady);
						}
						ASSERT_OR_THROW (msg.thread_stopped.nub_port == fToNubPort);

						// The DThread object will eventually take ownership of the DCpuState
						DCpuState* localState = DCpuState::LocalCpuState(msg.thread_stopped.cpu);
						fTeam->ThreadStopped(msg.thread_stopped.thread, msg.thread_stopped.why,
							localState, msg.thread_stopped.data);
					}
					break;
				}
				
				case B_THREAD_CREATED:
				{
					BAutolock lock(fTeam);
					
					if (lock.IsLocked())
					{
						ASSERT_OR_THROW (msg.thread_created.team == fTeam->GetID());
						fTeam->ThreadCreated(msg.thread_created.thread);
					}
					break;
				}
				
				case B_THREAD_DELETED:
				{
					BAutolock lock(fTeam);
					
					if (lock.IsLocked())
					{
						ASSERT_OR_THROW (msg.thread_deleted.team == fTeam->GetID());
						fTeam->ThreadDeleted(msg.thread_deleted.thread);
					}
					break;
				}
				
				case B_TEAM_DELETED:
				{
					BAutolock lock(fTeam);
					
					if (lock.IsLocked())
					{
						ASSERT_OR_THROW (msg.team_deleted.team == fTeam->GetID());
						fTeam->TeamDeleted();
						fTeam = NULL;
					}
					break;
				}
				
				case B_ELF_IMAGE_CREATED:
				{
					BAutolock lock(fTeam);
					
					if (lock.IsLocked())
					{
						ASSERT_OR_THROW (msg.pef_image_created.team == fTeam->GetID());
						fTeam->ImageCreated(msg.pef_image_created.info);
						
						nub_acknowlege_image_created_msg reply;
						reply.token = msg.pef_image_created.reply_token;
						(void) write_port(fToNubPort, B_ACKNOWLEGE_IMAGE_CREATED, &reply, sizeof(reply));
					}
					break;
				}

				case B_ELF_IMAGE_DELETED:
				{
					BAutolock lock(fTeam);
					
					if (lock.IsLocked())
					{
						ASSERT_OR_THROW (msg.pef_image_deleted.team == fTeam->GetID());
						fTeam->ImageDeleted(msg.pef_image_deleted.info);
					}
					break;
				}
				
				case -1:
					break;

				default:
					THROW(("Unsupported debug message: %d", what));
			}
		}
		catch (HErr& e)
		{
			e.DoError();
		}
		catch (...) {}
	}

	delete this;
	
	return 0;
} /* DLocalNub::Serve */

