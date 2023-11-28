
#ifndef CDDRIVER_H
#define CDDRIVER_H

#include <OS.h>

class SCSIDevice;
class CDTrack;

class BLooper;
class BList;

class CDDriver
{
public:
	CDDriver(void);
	virtual ~CDDriver();

	virtual const char *Name(void) = 0;
	virtual const char *DeviceName(void) = 0;
	virtual const char *DevicePath(void) = 0;
	
	virtual bool IsSupportedDevice(SCSIDevice *dev) = 0;
	virtual bool IsBlankDisc(void) = 0;

	virtual CDDriver *GetDriverInstance(SCSIDevice *dev, BLooper *looper) = 0;
	
	/* these can only be used by the driver instance */
	virtual bool Burning(void) = 0;
	virtual float PercentFifo(void) = 0;
	virtual float PercentDone(void) = 0;
	
	virtual status_t Check(CDTrack *tracks) = 0;
	virtual status_t Start(CDTrack *tracks) = 0;
	virtual status_t Abort(void) = 0;
	virtual char *GetError(void) = 0;
	
	/* Fills a BList with Driver Instances, returns count */
	static void GetInstances(BList *list);
	
protected:
	void Lock(void) { acquire_sem(lock); }
	void Unlock(void) { release_sem(lock); }
	sem_id lock;
};

#endif

