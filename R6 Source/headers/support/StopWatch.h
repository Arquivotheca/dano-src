/******************************************************************************
/
/	File:			StopWatch.h
/
/	Description:	BStopWatch class defines a handy code timing debug tool.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef _STOP_WATCH_H
#define _STOP_WATCH_H

#include <BeBuild.h>
#include <OS.h>
#include <SupportDefs.h>

enum {
	B_SILENT_STOP_WATCH		= (1<<0),
	B_THREAD_STOP_WATCH		= (1<<1)
};

/*-------------------------------------------------------------*/
/*----- BStopWatch class --------------------------------------*/

class BStopWatch {
public:
					BStopWatch(	const char *name, uint32 flags = 0);
virtual				~BStopWatch();

		void		Suspend();
		void		Resume();
		bigtime_t	Lap();
		bigtime_t	ElapsedTime() const;
		void		Reset();
		void		Reset(uint32 flags);
		const char	*Name() const;

/*----- Private or reserved ---------------*/
private:

virtual	void		_ReservedStopWatch1();
virtual	void		_ReservedStopWatch2();

		void		InitData(const char* name, uint32 flags);
		bigtime_t	CurrentTime(bigtime_t* base) const;

					BStopWatch(const BStopWatch &);
		BStopWatch	&operator=(const BStopWatch &);
		
		bigtime_t	fStart;
		bigtime_t	fSuspendTime;
		bigtime_t	fLaps[10];
		int32		fLap;
		const char	*fName;
		thread_id	fThread;
		uint32		fFlags;
		bool		_reserved;
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _STOP_WATCH_H */
