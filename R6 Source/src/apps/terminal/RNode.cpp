

#include	<Application.h>
#include	<Locker.h>
#include	<MessageQueue.h>
#include	<stdio.h>
#include	"RNode.h"
#include	"rico.h"


struct RNodeManager: BLocker {
		~RNodeManager( );
};

struct nodeinfo {
	BHandler	*handler;
	BLooper		*looper;
	thread_id	thread;
	BMessageQueue	*inq,
			*outq;
	uint		refcount,
			srccount,
			dstid,
			itraffic,
			otraffic,
			anomalies;
	RNode		*establisher;
	char		*label;
	void		deliver( BMessage *);
};


static nodeinfo		ntab[100];
static uint		nextid		= 1;
static RNodeManager	manager;
static BLooper		*defaultlooper( );


RNode::RNode( )
{

	manager.Lock( );
	id = 0;
	if (nextid < nel( ntab)) {
		id = nextid++;
		nodeinfo &n = ntab[id];
		++n.refcount;
	}
	manager.Unlock( );
}


RNode::~RNode( )
{

	manager.Lock( );
	nodeinfo &n = ntab[id];
	if (this == n.establisher) {
		n.establisher = 0;
		manager.Unlock( );
		Shutdown( );
	}
	else
		switch (n.refcount) {
		default:
			--n.refcount;
		case 0:
			manager.Unlock( );
			break;
		case 1:
			manager.Unlock( );
			Shutdown( );
		}
}


RNode
RNode::operator=( const RNode &s)
{

	unless (id == s.id) {
		this->~RNode( );
		manager.Lock( );
		id = s.id;
		nodeinfo &n = ntab[id];
		if (n.refcount)
			++n.refcount;
		manager.Unlock( );
	}
	return (*this);
}


RNode::RNode( RNode &s)
{

	manager.Lock( );
	id = s.id;
	nodeinfo &n = ntab[id];
	if (n.refcount)
		++n.refcount;
	manager.Unlock( );
}


void
RNode::Establish( BHandler *h, BLooper *l, char *s)
{
	BMessage	*m;

	manager.Lock( );
	nodeinfo &n = ntab[id];
	if (n.establisher)
		++n.anomalies;
	n.establisher = this;
	n.handler = h;
	if (n.looper = l)
		n.thread = n.looper->Thread( );
	n.label = s;
	if (n.handler) {
		if (n.looper) {
			manager.Unlock( );
			unless (n.handler->Looper( )) {
				if (n.looper->Lock( )) {
					n.looper->AddHandler( n.handler);
					n.looper->Unlock( );
				}
			}
		}
		else {
			manager.Unlock( );
			if (defaultlooper( )->Lock( )) {
				defaultlooper( )->AddHandler( n.handler);
				defaultlooper( )->Unlock( );
			}
		}
		manager.Lock( );
	}
	while ((n.handler || n.looper)
	and (n.inq)
	and (m = n.inq->NextMessage( )))
		n.deliver( m);
	delete n.inq;
	n.inq = 0;
	manager.Unlock( );
}


void
RNode::Establish( BHandler *h, char *s)
{

	Establish( h, 0, s);
}


bool
RNode::SetDestination( RNode dst)
{
	BMessage	*m;

	manager.Lock( );
	nodeinfo &n = ntab[id];
	unless ((n.refcount)
	and (not n.dstid)) {
		++n.anomalies;
		manager.Unlock( );
		return (FALSE);
	}
	n.dstid = dst.id;
	nodeinfo &d = ntab[dst.id];
	++d.srccount;
	while ((n.outq)
	and (m = n.outq->NextMessage( )))
		d.deliver( m);
	delete n.outq;
	n.outq = 0;
	manager.Unlock( );
	return (TRUE);
}


void
RNode::operator>>( BMessage *m)
{

	manager.Lock( );
	nodeinfo &n = ntab[id];
	if (n.refcount) {
		++n.otraffic;
		if (n.dstid)
			ntab[n.dstid].deliver( m);
		else {
			unless (n.outq)
				n.outq = new BMessageQueue;
			if (n.outq)
				n.outq->AddMessage( m);
		}
	}
	manager.Unlock( );
}


void
RNode::operator>>( int what)
{

	*this >> new BMessage( what);
}


/*
 * This node will no longer accept or originate messages.
 */
void
RNode::Shutdown( )
{

	manager.Lock( );
	nodeinfo &n = ntab[id];
	if ((n.refcount)
	and (n.dstid)
	and (not --ntab[n.dstid].srccount))
		*this >> EOF;
	n.refcount = 0;
	manager.Unlock( );
}


void
RNode::Dump( )
{

	nodeinfo &n = ntab[id];
	if (n.label)
		fprintf( stderr, "RNode::DumpInfo: id = %d, label = \"%s\"\n", id, n.label);
	else
		fprintf( stderr, "RNode::DumpInfo: id = %d, label = 0\n", id);
}


int
RNode::DumpTable( )
{

	fprintf( stderr, "id   looper thread  handler Q's     traffic ref src dst label\n");
	fprintf( stderr, "                            I O a   in  out  ct  ct  id\n");
	for (uint i=0; i<nextid; ++i) {
		nodeinfo &n = ntab[i];
		fprintf( stderr, "%2d ", i);
		fprintf( stderr, "%08X ", n.looper);
		if (n.looper)
			fprintf( stderr, "%6d ", n.thread);
		else
			fprintf( stderr, "     - ");
		fprintf( stderr, "%08X ", n.handler);
		fprintf( stderr, "%d ", n.inq? n.inq->CountMessages( ): 0);
		fprintf( stderr, "%d ", n.outq? n.outq->CountMessages( ): 0);
		fprintf( stderr, "%d ", n.anomalies);
		fprintf( stderr, "%4d ", n.itraffic);
		fprintf( stderr, "%4d ", n.otraffic);
		fprintf( stderr, "%3d ", n.refcount);
		fprintf( stderr, "%3d ", n.srccount);
		fprintf( stderr, "%3d", n.dstid);
		if (n.label)
			fprintf( stderr, " %s", n.label);
		fprintf( stderr, "\n");
	}
	return (0);
}


static BLooper	*
defaultlooper( )
{
	static BLooper	*looper;

	unless (looper) {
		looper = new BLooper;
		looper->Run( );
		rename_thread( looper->Thread( ), "RNodeManager");
	}
	return (looper);
}


RNodeManager::~RNodeManager( )
{
	long		z;
	thread_id	app;
	int32		cookie;
	thread_info	info;

	Lock( );
	for (uint i=0; i<nextid; ++i)
		ntab[i].deliver( new BMessage( EOF));
	app = find_thread( 0);
	for (uint i=0; i<nextid; ++i) {
		nodeinfo &n = ntab[i];
		n.refcount = 0;
		if ((n.looper)
		and (n.thread != app)) {
			/*
			 * A BLooper has 10 seconds to clean up.
			 */
			uint r = 0;
			loop {
				int s = resume_thread( n.thread);
				if ((s == B_BAD_THREAD_ID)
				or (s == B_BAD_VALUE))
					break;
				if (++r == 1000) {
					kill_thread( n.thread);
					break;
				}
				Unlock( );
				snooze( 10000);
				Lock( );
			}
		}
	}
	Unlock( );
	thread_id t = defaultlooper( )->Thread( );
	defaultlooper( )->Lock( );
	defaultlooper( )->Quit( );
	wait_for_thread( t, &z);
}


void
nodeinfo::deliver( BMessage *m)
{

	unless (refcount)
		delete m;
	else if (looper) {
		++itraffic;
		manager.Unlock( );
		if (handler)
			looper->PostMessage( m, handler);
		else
			looper->PostMessage( m);
		delete m;
		manager.Lock( );
	}
	else if (handler) {
		++itraffic;
		manager.Unlock( );
		defaultlooper( )->PostMessage( m, handler);
		delete m;
		manager.Lock( );
	}
	else {
		unless (inq)
			inq = new BMessageQueue;
		if (inq)
			inq->AddMessage( m);
	}
}
