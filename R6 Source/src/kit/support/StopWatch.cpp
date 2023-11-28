/*
	
	StopWatch.cpp
	
	Copyright 1994 Be, Inc. All Rights Reserved.

*/


#ifndef _STOP_WATCH_H
#include <StopWatch.h>
#endif

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef _OS_H
#include <OS.h>
#endif

#include <new>

/* --------------------------------------------------------- */

BStopWatch::BStopWatch(const char *name, uint32 flags)
{
	InitData(name, flags);
}

/* --------------------------------------------------------- */

void BStopWatch::InitData(const char *name, uint32 flags)
{
	fFlags = 0;
	fName = name;
	fSuspendTime = 0;
	fLap = 0;
	fLaps[0] = 0;
	Reset(flags);
}

/* --------------------------------------------------------- */

BStopWatch::~BStopWatch()
{
	if (!(fFlags&B_SILENT_STOP_WATCH)) {
		bigtime_t t = ElapsedTime();
		printf("StopWatch \"%s\": %Ld usecs.\n", fName, t);
		if (fLap != 0) {
			// lap feature was used

			// This stopping point is also considered a lap
			fLaps[++fLap] = t;

			printf("    ");
			for (long i = 1; i <= fLap; i++) {
				if ((i == 5) || (i == 9))
					printf("\n    ");
				printf("[%ld: %Ld#%Ld] ", i, fLaps[i], fLaps[i] - fLaps[i-1]);
			}
			printf("\n");
		}
	}
}

/* --------------------------------------------------------- */

void	BStopWatch::Suspend()
{
	if (fSuspendTime == 0)
		fSuspendTime = CurrentTime(&fStart);
}

/* --------------------------------------------------------- */

void	BStopWatch::Resume()
{
	if (fSuspendTime != 0) {
		fStart -= fSuspendTime;
		fSuspendTime = 0;
		fStart += CurrentTime(&fStart);
	}
}

/* --------------------------------------------------------- */

const char *BStopWatch::Name() const
{
	return fName;
}

/* --------------------------------------------------------- */

bigtime_t	BStopWatch::ElapsedTime() const
{
	
	if (fSuspendTime == 0) {
		const bigtime_t current = CurrentTime(const_cast<bigtime_t*>(&fStart));
		return current - fStart;
	} else {
		return fSuspendTime - fStart;
	}
}

/* --------------------------------------------------------- */

void	BStopWatch::Reset()
{
	Reset(fFlags);
}

/* --------------------------------------------------------- */

void	BStopWatch::Reset(uint32 flags)
{
	if ((fFlags&B_THREAD_STOP_WATCH) != (flags&B_THREAD_STOP_WATCH)) {
		fThread = find_thread(NULL);
	}
	fFlags = flags;
	fSuspendTime = 0;
	bigtime_t dummy = 0;
	fStart = CurrentTime(&dummy);
}

/* --------------------------------------------------------- */

bigtime_t BStopWatch::Lap()
{
	if (fLap == 8)
		return 0;

	bigtime_t t = ElapsedTime();
	fLaps[++fLap] = t;
	return t;
}

/* --------------------------------------------------------- */

bigtime_t BStopWatch::CurrentTime(bigtime_t* base) const
{
	if ((fFlags&B_THREAD_STOP_WATCH) != 0) {
		thread_info info;
		thread_info info2;
		status_t err = get_thread_info(fThread, &info);
		if (err != B_OK) return 0;
		err = get_thread_info(fThread, &info2);
		if (err != B_OK) return 0;
		
		// The overhead for get_thread_info() is hideous, so take
		// it out of the picture.  This -should- be multiplied by a
		// factor of 2 (for calling get_thread_info() twice), but
		// for some reason that becomes too large.  At least this is
		// enough to make its behaviour reasonable.
		*base += ( (info2.user_time+info2.kernel_time)
					- (info.user_time+info.kernel_time) ) * 1;
		
		return info2.user_time + info2.kernel_time;
	}
	
	return system_time();
}

/*-------------------------------------------------------------*/

BStopWatch::BStopWatch(const BStopWatch &) {}
BStopWatch &BStopWatch::operator=(const BStopWatch &) { return *this; }

/* ---------------------------------------------------------------- */

void BStopWatch::_ReservedStopWatch1() {}
void BStopWatch::_ReservedStopWatch2() {}

// --------- Deprecated BShape methods 11/1999 (Maui) ---------

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT BStopWatch*
	#if __GNUC__
	__10BStopWatchPCcb
	#elif __MWERKS__
	__10BStopWatchFPCcb
	#endif
	(void* This, const char* name, bool silent)
	{
		return new (This) BStopWatch(name, silent ? B_SILENT_STOP_WATCH : 0);
	}

}
#endif

/* --------------------------------------------------------- */
/* --------------------------------------------------------- */
/* --------------------------------------------------------- */
