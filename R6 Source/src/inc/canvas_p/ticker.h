#ifndef TICKER_H
#define TICKER_H

#include "athread.h"

class BTickThread : public BRunnable
{
public:
			BTickThread(const char *name, const bigtime_t interval=1000000, long aPriority = B_NORMAL_PRIORITY);
			BTickThread(const int32 frequency, const char *name="ticker_thread", long aPriority = B_NORMAL_PRIORITY);
	virtual ~BTickThread();
	
	// Inherited from BRunnable
	virtual status_t	Run();
	virtual status_t	Quit();
	
	virtual bool	IsQuitting(){return fIsQuitting;};
	
protected:
	// Our own thing
	virtual void		Tick(const int32 tickCount, const bigtime_t tickTime);

	bool		fIsQuitting;	// Indicates that we're in the midst of quitting
	int32		fFrequency;		// How many times a second
								// Note this is an int.  It is not used 
								// to specify things like timecode 29.97.
								// that would be a higher mechanism.
	bigtime_t	fInterval;		// Number of microseconds between ticks
	int32		fTickCount;		// Current tick count.  Starts at 0
	sem_id		fQuittingSem;
	
private:
};


#endif
