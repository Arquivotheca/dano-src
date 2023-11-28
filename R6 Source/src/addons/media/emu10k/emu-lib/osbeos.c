#include <unistd.h>
#include <OS.h>
#include "os8210.h"
#include "ipsd8210.h"
#include "emuerrs.h"
#include "emu10k_driver.h"
#include <RealtimeAlloc.h>

#define DRIVER_NAME "/dev/audio/emu10k/1"
#define CALLBACK_THREAD "emu10k1 callback"

#if !DEBUG
#define printf
#endif

typedef struct {
	int	fd;
	sem_id int_sem;
	IPSVCHANDLE serviceID;
	fIPService srvc;
	fIPCallback cb;
} callback_spec;

#if EVENT_LOGGING_STUFF

#define LOG_SIZE 120000
static char logbuf[LOG_SIZE];
static int logfill = 0;
static int logfile = -1;
static bool logging = false;

void
STARTLOG()
{
  start_event_log();
  logging = true;
}

void
STOPLOG()
{
  int n;

  if (!logging)
	return;
  logging = false;
  n = stop_event_log(logbuf + logfill, LOG_SIZE - logfill);
  if (LOG_SIZE > logfill) {
	if (logfile < 0) {
	  logfile = open("/boot/home/event-log", O_WRONLY | O_CREAT);
	  printf("open(/boot/home/event-log): %d\n", logfile);
	  if (logfile < 0)
		perror("open");
	}
	if (logfile >= 0) {
	  if (n)
		write(logfile, logbuf + logfill, n);
	  logfill += n;
	}
  }
  else
	close(logfile);
}

#endif

static int32
emu_callback(void* data)
{
	callback_spec* spec = (callback_spec*) data;
	IPCBHANDLE callbackID;
	status_t err;
	int t1, t2, t3;

	while (1) {
		err = acquire_sem(spec->int_sem);
		if (err == B_INTERRUPTED)
			continue;
		if (err != B_OK)
			break;

//		printf("<callback>\n", err);
		t1 = system_time();
		IDISABLE();
		spec->srvc(spec->serviceID, &callbackID);
		IENABLE();
		spec->cb(callbackID);
		t2 = (int) system_time();
		t3 = t2 - t1;
		//if (t3 > 400)
		//	printf("<%d>\n", t3);
	}

	osHeapFree(spec);
	return 0;
}

BOOL
ipsdSetupInterrupt(DWORD interruptID, IPSVCHANDLE serviceID,
					fIPService srvc, fIPCallback cb)
{
	status_t err;
	
	printf("emu10k1: ipsdSetupInterrupt()\n");
	
	if (srvc || cb) {
		callback_spec* spec = (callback_spec*) osHeapAlloc(sizeof(*spec), 0);
		if (spec == NULL)
			return FALSE;
		spec->fd = interruptID;
		spec->serviceID = serviceID;
		spec->srvc = srvc;
		spec->cb = cb;
		spec->int_sem = create_sem(0, "emu10k1 interrupt");
		if (spec->int_sem < 0) {
			osHeapFree(spec);
			return FALSE;
		}
		err = resume_thread(spawn_thread(emu_callback, CALLBACK_THREAD,
											B_REAL_TIME_PRIORITY, spec));
		if (err < 0) {
			osHeapFree(spec);
			delete_sem(spec->int_sem);
			return FALSE;
		}
		err = ioctl(spec->fd, EMU_SET_INTERRUPT_SEM, &spec->int_sem);
		if (err < 0) {
			osHeapFree(spec);
			delete_sem(spec->int_sem);
			return FALSE;
		}
	}
	else {
		sem_id old_sem;
		sem_id bad_sem = B_BAD_SEM_ID;

		err = ioctl(interruptID, EMU_GET_INTERRUPT_SEM, &old_sem);
		if (err < 0)
			return FALSE;
		err = ioctl(interruptID, EMU_SET_INTERRUPT_SEM, &bad_sem);
		if (err < 0)
			return FALSE;
		delete_sem(old_sem);
		wait_for_thread(find_thread(CALLBACK_THREAD), &err);
	}

	return TRUE;
}

static int32
open_driver()
{
	return open(DRIVER_NAME, O_RDONLY);
}

OSVIRTADDR
osAllocPages(DWORD dwNumPages,  OSMEMHANDLE *memhdl /* IO */)
{
	emu_area_spec spec;
	status_t err;
	int fd;

	printf("osAllocPages(%d)\n", dwNumPages);
	fd = open_driver();
	if (fd < 0)
		return NULL;

	spec.size = 4096 * dwNumPages;
	err = ioctl(fd, EMU_ALLOC_AREA, &spec);
	close(fd);
	printf("Area %d:  0x%08x\n", spec.area, spec.addr);
	if (err != B_OK)
		return NULL;

	*memhdl = spec.area;
	return spec.addr;
}

OSVIRTADDR
osAllocContiguousPages(DWORD dwNumPages, OSPHYSADDR *retPhysicalAddr /* IO */,
                       OSMEMHANDLE *retMemHdl /* IO */)
{
	emu_area_spec spec;
	status_t err;
	int fd;

	printf("osAllocContiguousPages(%d)\n", dwNumPages);
	fd = open_driver();
	if (fd < 0)
		return NULL;

	spec.size = 4096 * dwNumPages;
	err = ioctl(fd, EMU_ALLOC_CONTIGUOUS_AREA, &spec);
	close(fd);
	printf("Area %d:  0x%08x\n", spec.area, spec.addr);
	if (err != B_OK)
		return NULL;

	*retMemHdl = spec.area;
	*retPhysicalAddr = (OSPHYSADDR) spec.phys_addr;
	return spec.addr;
}

EMUSTAT
osFreePages(OSMEMHANDLE memhdl)
{
	status_t err;

	printf("osFreePages(%d)\n", memhdl);
	err = delete_area(memhdl);
	if (err < 0) {
		printf("osFreePages: delete_area() failed: 0x%08x\n", err);
		return OSERR_BAD_MAPPING;
	}
	return SUCCESS;
}

#define HEAP_SIZE 8000
static rtm_pool* heap;

OSVIRTADDR
osHeapAlloc(DWORD dwNumBytes, DWORD dwFlags)
{
	OSVIRTADDR addr;

	if (!heap) {
	  rtm_create_pool(&heap, HEAP_SIZE, "E-Mu 10k");
	  if (!heap) {
		printf("osHeapAlloc(): Can't allocate heap\n");
		return 0;
	  }
	}

	addr = (OSVIRTADDR) rtm_alloc(heap, dwNumBytes);
	if (!addr)
	  printf("osHeapAlloc(%d) failed!\n", dwNumBytes);
	//printf("osHeapAlloc(%d) = 0x%x\n", dwNumBytes, addr);
	return addr;
}

void
osHeapFree(OSVIRTADDR virtAddr)
{
	status_t err;
	//printf("osHeapFree(0x%x)\n", virtAddr);
	err = rtm_free(virtAddr);
	if (err < 0)
	  printf("rtm_free(): %x\n", err);
}

OSVIRTADDR
osLockedHeapAlloc(DWORD dwNumBytes)
{
	//printf("osLockedHeapAlloc(%d) = 0x%08x\n", dwNumBytes, addr);
	return osHeapAlloc(dwNumBytes, 0);
}

void
osLockedHeapFree(OSVIRTADDR virtAddr, DWORD dwNumBytes)
{
	//printf("osLockedHeapFree(0x%08x, %d)\n", virtAddr, dwNumBytes);
	osHeapFree(virtAddr);
}

EMUSTAT
osLockVirtualRange(OSVIRTADDR virtAddr, DWORD dwNumPages,  
					DWORD dwNumRetPages /* VSIZE */,
					OSPHYSADDR *retPhysAddrs /* IO */)
{
	emu_area_spec spec;
	status_t err;
	int fd;

	printf("osLockVirtualRange(0x%08x, %d, %d)\n",
		   virtAddr, dwNumPages, dwNumRetPages);

	fd = open_driver();
	if (fd < 0)
		return -1;

	spec.addr = virtAddr;
	spec.size = 4096 * dwNumPages;
	spec.map_entries = dwNumRetPages;
	spec.phys_map = (void**) retPhysAddrs;
	err = ioctl(fd, EMU_LOCK_RANGE, &spec);
	close(fd);

	if (err < 0)
		printf("osLockVirtualRange(0x%08x, %d, %d) failed\n",
					virtAddr, dwNumPages, dwNumRetPages);

	return (err == B_OK ? SUCCESS : -1);
}

EMUSTAT
osUnlockVirtualRange(OSVIRTADDR virtAddr, DWORD dwNumPages)
{
	emu_area_spec spec;
	status_t err;
	int fd;

	//printf("osUnlockVirtualRange(0x%08x, %d)\n", virtAddr, dwNumPages);

	fd = open_driver();
	if (fd < 0)
		return -1;

	spec.addr = virtAddr;
	spec.size = 4096 * dwNumPages;
	err = ioctl(fd, EMU_UNLOCK_RANGE, &spec);
	close(fd);

	return (err == B_OK ? SUCCESS : -1);
}

EMUSTAT
osCreateMutex(OSMUTEXHDL *pmutexhdl)
{
	sem_id sem = create_sem(1, "Emu10k1 sem");
	if (sem < 0)
		return OSERR_NO_MEMORY;
	*pmutexhdl = sem;
	return SUCCESS;
}

EMUSTAT
osRequestMutex(OSMUTEXHDL mutexhdl)
{
	return (acquire_sem_etc(mutexhdl, 1, B_TIMEOUT, 0) == B_OK ? SUCCESS : -1);
}

void
osReleaseMutex(OSMUTEXHDL mutexhdl)
{
	release_sem(mutexhdl);
}

void
osWaitMutex(OSMUTEXHDL mutexhdl)
{
	acquire_sem(mutexhdl);
}

void
osDeleteMutex(OSMUTEXHDL mutexhdl)
{
	delete_sem(mutexhdl);
}

BOOL
osPollMutex(DWORD dwHandle)
{
	int32 count;
	status_t err = get_sem_count(dwHandle, &count);
	return (err == B_OK && count > 0);
}


void
OSFxCallBack(ULONG ulHandle, ULONG callID, ULONG ulEvent,
				ULONG ulParam, ULONG ulSystemParam)
{
	/* what should this do? */
}


typedef struct {
	DWORD dwMillisecs;
	OSCALLBACK fCallback;
	DWORD dwUser;
} oscallback_holder;

static int32
oscallback_thread(void* data)
{
	oscallback_holder* cbh = (oscallback_holder*) data;
	while(1) {
		snooze(cbh->dwMillisecs * 1000);
		if (cbh->dwMillisecs <= 0) {
			osHeapFree(cbh);
			return 0;
		}
		cbh->dwMillisecs = 0;
		cbh->fCallback(cbh->dwUser);
	}
}
	
EMUSTAT
osScheduleTimeout(DWORD dwMillisecs, OSCALLBACK fCallback,
                   DWORD dwUser, OSTIMEOUTHANDLE *pRetHandle)
{
	/* cheezy implementation of cheezy function */
	status_t err;
	oscallback_holder* cbh = (oscallback_holder*) osHeapAlloc(sizeof (*cbh), 0);

	printf("osScheduleTimeout(%d)\n", dwMillisecs);

	if (cbh == 0)
		return OSERR_NO_MEMORY;
	cbh->dwMillisecs = dwMillisecs;
	cbh->fCallback = fCallback;
	cbh->dwUser = dwUser;
	err = resume_thread(spawn_thread(oscallback_thread, "Timeout Callback",
										B_NORMAL_PRIORITY, (void*) cbh));
	if (err != B_OK)
		return -1;
	*pRetHandle = (OSTIMEOUTHANDLE) cbh;
	return SUCCESS;
}


EMUSTAT
osUnscheduleTimeout(OSTIMEOUTHANDLE handle)
{
	oscallback_holder* cbh = (oscallback_holder*) handle;
	cbh->dwMillisecs = 0;
	return SUCCESS;
}


EMUSTAT
osRescheduleTimeout(OSTIMEOUTHANDLE handle, DWORD dwMillisecs)
{
	oscallback_holder* cbh = (oscallback_holder*) handle;
	cbh->dwMillisecs = dwMillisecs;
	return SUCCESS;
}


static int32 GlobalLockOwner = -1;
static sem_id GlobalLockSem = B_BAD_SEM_ID;
static int32 GlobalLockCount = 0;
static int32 GlobalLockOwnerCount = 0;

void
osDisableInterrupts()
{
  thread_id moi = find_thread(NULL);
  status_t err;

  if (GlobalLockOwner == moi) {
	++GlobalLockOwnerCount;
	return;
  }

  if (GlobalLockSem < 0)
	GlobalLockSem = create_sem(0, "emu10k1 lock");

  if (atomic_add(&GlobalLockCount, 1) > 0)
	do err = acquire_sem(GlobalLockSem);
	while (err == B_INTERRUPTED);

  GlobalLockOwner = moi;
  GlobalLockOwnerCount = 1;
}

void
osEnableInterrupts()
{
  if (--GlobalLockOwnerCount == 0) {
	GlobalLockOwner = -1;
	if (atomic_add(&GlobalLockCount, -1) > 1)
	  release_sem(GlobalLockSem);
  }
}
