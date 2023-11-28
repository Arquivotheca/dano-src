#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include "simplelist.h"
#include <OS.h>

enum EChangeNotices
{
	kInvalidNotice=-1L,
	kAllChanges=0
};
	

class BObservable;
class BObserver;


class BObserver
{
public:
			BObserver();
	virtual ~BObserver();
	
	// Our own little interface
	// InterestedIn() - allows the observer to be queried about what it is
	// interested in noticing.  This way a sender can query the object and it
	// can respond to ranges of messages without explicitly handing out a list.
	// Additionally, it can change it interests over time if appropriate.
	virtual	bool	IsInterestedIn(const int32 notice, BObservable *observable=0);
	
	// ReceiveNotice()  - this method is called once a observable has a message that it
	// wants to send out and has ascertained that an observer is interested in it.
	// The data argument is simply the data that was originally passed in to the
	// observer when we registered for notifications.
	virtual status_t	ReceiveNotice(const int32 notice=kAllChanges, BObservable *observable=0, void *data=0);
	
	// StartObserving starts the observation process
	virtual void	StartObserving(BObservable *);
	virtual void	StopObserving(BObservable *);
	virtual void	StopObservingAll();
			void	RemoveObservable(BObservable *);
	
			status_t	LockObserver();
			status_t	UnlockObserver();
			
protected:
	virtual void	DoReceiveNotice(const int32 notice=kAllChanges, BObservable *observable=0, void *data=0);


	sem_id					fActivitySem;		// Semaphore to protect deleting
	sem_id					fListSem;			// Semaphore to protect list
	simplelist<BObservable *>	fObservables;	// List of observables
	
private:
	BObserver(const BObserver &);				// These two are here because they
	BObserver & operator=(const BObserver &);	// Do not have implementations and we don't
												// want the compiler to generate them automatically
};

//======================================================================
//
//======================================================================

class BObservable
{
public:
			BObservable();
	virtual ~BObservable();
	
	// Our own little interface
	virtual	status_t	SendNotificationOf(const int32 notice=kAllChanges, void *data=0);
			status_t	AddObserver(BObserver *, void *data=0);
			status_t	RemoveObserver(BObserver *, void *data=0);
			status_t	RemoveAllObservers();
			
protected:

	virtual status_t NotifyOne(BObserver *observer, const int32=kAllChanges, void *data=0);
		
private:
		// Make these private until we can do them right
		BObservable(const BObservable &);
		BObservable & operator=(const BObservable &);

	sem_id					fObserverListSem;	// Semaphore to protect list
	simplelist<BObserver *>	fObservers;	// List of observers
};

#endif
