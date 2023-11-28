

#include	<Application.h>
#include	<Locker.h>
#include	"RNode.h"
#include	"rico.h"


struct BTimer: BLocker {
	struct request {
			request( RNode, BMessage *, bigtime_t);
			~request( );
		request	*next;
		BMessage*message;
		bigtime_t	sendtime;
		RNode	node;
	};
	class looper: public BLooper {
		void	MessageReceived( BMessage *);
	};
	request	*rlist;
	sem_id	sem;
	void	initialize( );
		~BTimer( );
	private:
	bool	initialized;
	looper	*sentinel;
};


static BTimer	timer;


void
BTimerSend( RNode n, BMessage *m, bigtime_t d)
{

	new BTimer::request( n, m, system_time( )+d);
}


void
BTimer::initialize( )
{

	unless (initialized) {
		initialized = TRUE;
		sem = create_sem( 0, "BTimer");
		sentinel = new looper;
		sentinel->Run( );
		sentinel->PostMessage( 1);
		rename_thread( sentinel->Thread( ), "BTimer");
	}
}


BTimer::~BTimer( )
{

	if (initialized) {
		delete_sem( sem);
		thread_id t = sentinel->Thread( );
		sentinel->Lock( );
		sentinel->Quit( );
		long z;
		wait_for_thread( t, &z);
	}
}


void
BTimer::looper::MessageReceived( BMessage *m)
{

	loop {
		if (request *lbest = timer.rlist) {
			for (request *l=lbest; l; l=l->next)
				if (l->sendtime < lbest->sendtime)
					lbest = l;
			bigtime_t delay = lbest->sendtime - system_time( );
			/*
			 * To avoid bug in acquire_sem_etc, delay must be
			 * at least 1 ms.
			 */
			if ((delay > 0)
			and (acquire_sem_etc( timer.sem, 1, B_TIMEOUT, max( delay, 5000)) == B_BAD_SEM_ID))
				break;
		}
		else if (acquire_sem( timer.sem) == B_BAD_SEM_ID)
			break;
		for (request *l=timer.rlist; l; l=l->next)
			if (l->sendtime <= system_time( )) {
				l->node >> l->message;
				delete l;
				break;
			}
	}
	while (timer.rlist)
		delete timer.rlist;
}


BTimer::request::request( RNode n, BMessage *m, bigtime_t t):
node( n), message( m), sendtime( t)
{

	timer.Lock( );
	next = timer.rlist;
	timer.rlist = this;
	timer.initialize( );
	release_sem( timer.sem);
	timer.Unlock( );
}


BTimer::request::~request( )
{

	timer.Lock( );
	if (timer.rlist == this)
		timer.rlist = next;
	else if (timer.rlist)
		for (request *l=timer.rlist; l->next; l=l->next)
			if (l->next == this) {
				l->next = next;
				break;
			}
	timer.Unlock( );
}
