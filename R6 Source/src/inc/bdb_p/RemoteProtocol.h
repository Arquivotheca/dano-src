// BMessage 'what' constants etc. for the bdb <-> remote proxy protocol

#ifndef RemoteProtocol_H
#define RemoteProtocol_H 1

#include <SupportDefs.h>

// instructions to our UDPLooper
const uint32 BDB_MSG_READ_DATA = 'Read';	// int32 "address", int32 "size" -> msg: int32 "address", raw data "data"
const uint32 BDB_MSG_WRITE_DATA = 'Writ';	// int32 "address", raw data "data" -> status_t result
const uint32 BDB_MSG_SET_BREAKPOINT = 'Sbkp';	// int32 "address" -> status_t result
const uint32 BDB_MSG_CLEAR_BREAKPOINT = 'Cbkp';	// int32 "address" -> status_t result
const uint32 BDB_MSG_SET_WATCHPOINT = 'Swch';	// int32 "address" -> status_t result
const uint32 BDB_MSG_CLEAR_WATCHPOINT = 'Cwch';	// int32 "address" -> status_t result
const uint32 BDB_MSG_STOP_THREAD = 'Stop';		// int32 "thread" [thread_id] -> status_t result
const uint32 BDB_MSG_KILL_THREAD = 'Kill';		// int32 "thread" [thread_id] -> status_t result
const uint32 BDB_MSG_GET_THREAD_REGS = 'Treg';	// int32 "thread" [thread_id] ->msg: raw cpu_state "cpu_state"
const uint32 BDB_MSG_GET_THREAD_INFO = 'Ginf';	// int32 "thread" [thread_id] -> msg: int32 "pc", int32 "sp"
const uint32 BDB_MSG_GET_THREAD_LIST = 'ThrL';		// int32 "team" [team_id] -> msg: int32[] "tid", string[] "tname"
const uint32 BDB_MSG_GET_IMAGE_LIST = 'ImgL';	// int32 "team" [team_id] -> msg: raw image_info's called "image"
const uint32 BDB_MSG_RUN = 'RunT';		// int32 "thread" [thread_id], raw data "cpu" [flat DCpuState] -> status_t result
const uint32 BDB_MSG_STEP = 'Step';		// int32 "thread" [thread_id], int32 "low" [low pc], int32 "high", raw data "cpu" [flat DCpuState] -> status_t result
const uint32 BDB_MSG_STEP_OVER = 'SOvr';		// int32 "thread" [thread_id], int32 "low" [low pc], int32 "high",  raw data "cpu" [flat DCpuState] -> status_t result
const uint32 BDB_MSG_STEP_OUT = 'SOut';		// int32 "thread" [thread_id], raw data "cpu" [flat DCpuState] -> status_t result
const uint32 BDB_MSG_KILL_TARGET = 'Nuke';		// no args
const uint32 BDB_MSG_ATTACH = 'Atth';			// int32 "team" [team_id] -> msg: raw image_info's called "image"
const uint32 BDB_MSG_DETACH = 'Dtch';				// no args
const uint32 BDB_MSG_QUIT_LOOPER = 'Qlup';		// no args

// responses from the remote proxy to the UDPLooper
const uint32 BDB_MSG_RESPONSE_ATTACH = 'Rach';
const uint32 BDB_MSG_RESPONSE_READ_DATA = 'Rrdd';
const uint32 BDB_MSG_RESPONSE_WRITE_DATA = 'Rwrt';
const uint32 BDB_MSG_RESPONSE_SET_BREAKPOINT = 'Rsbk';
const uint32 BDB_MSG_RESPONSE_CLEAR_BREAKPOINT = 'Rcbk';
const uint32 BDB_MSG_RESPONSE_SET_WATCHPOINT = 'Rswp';
const uint32 BDB_MSG_RESPONSE_CLEAR_WATCHPOINT = 'Rcwp';
const uint32 BDB_MSG_RESPONSE_STOP_THREAD = 'Rstp';
const uint32 BDB_MSG_RESPONSE_KILL_THREAD = 'Rkil';	
const uint32 BDB_MSG_RESPONSE_GET_THREAD_REGS = 'Rgtr';
const uint32 BDB_MSG_RESPONSE_GET_THREAD_INFO = 'Rgti';
const uint32 BDB_MSG_RESPONSE_GET_THREAD_LIST = 'Rgtl';
const uint32 BDB_MSG_RESPONSE_GET_IMAGE_LIST = 'Riml';
const uint32 BDB_MSG_RESPONSE_RUN = 'RRun';
const uint32 BDB_MSG_RESPONSE_STEP = 'RStp';
const uint32 BDB_MSG_RESPONSE_STEP_OVER = 'ROvr';
const uint32 BDB_MSG_RESPONSE_STEP_OUT = 'ROut';

// messages originating on the remote proxy

// 20-second timeout for trying to get a response from the remote nub
const bigtime_t NUB_TIMEOUT = 20000000LL;

// Constants identifying a flattened DCpuState
const uint16 FLAT_INTEL_CPUSTATE = 0xbe01;
const uint16 FLAT_ARM_CPUSTATE = 0xbe02;

#endif
