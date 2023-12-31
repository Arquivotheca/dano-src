 ddrover	*a;

	cpu_status ps = idisable( );
	acquire_spinlock( &ddsl);
	for (a=d->r; a; a=a->nextd)
		if (a->bg)
			release_sem_etc( a->sd, 1, B_DO_NOT_RESCHEDULE);
		else
			brelease( &a->b);
	d->r = 0;
	d->locked = FALSE;
	release_spinlock( &ddsl);
	irestore( ps);
}


static struct ddrover	*
enq( struct ddrover *rlist, struct ddrover *r)
{

	r->nextd = rlist;
	return (r);
}


static struct ddrover	*
deq( struct ddrover *rlist, struct ddrover *r)
{
	struct ddrover	*p;

	if (r == rlist)
		rlist = r->nextd;
	else
		for (p=rlist; p; p=p->nextd)
			if (p->nextd == r) {
				p->nextd = r->nextd;
				break;
			}
	return (rlist);
}


static struct ddrover	*
enqevent( struct ddrover *rlist, struct ddrover *r)
{

	r->nexte = rlist;
	return (r);
}


static struct ddrover	*
deqevent( struct ddrover *rlist, struct ddrover *r)
{
	struct ddrover	*p;

	if (r == rlist)
		rlist = r->nexte;
	else
		for (p=rlist; p; p=p->nexte)
			if (p->nexte == r) {
				p->nexte = r->nexte;
				break;
			}
	return (rlist);
}


static void
entryincr( struct ddrover *r, struct ddomain *d)
{
	uint	i;

	for (i=0; i<nel( r->e); ++i)
		if (r->e[i].d == d) {
			++r->e[i].n;
			return;
		}
	for (i=0; i<nel( r->e); ++i)
		unless (r->e[i].d) {
			if (d->bg)
				++r->nbg;
			else
				++r->nfg;
			r->e[i].d = d;
			++r->e[i].n;
			return;
		}
	dprintf( "ddacquire failure");
}


static void
rfree( )
{
	struct ddrover	*r;

	if ((r = rfreelist)) {
		rfreelist = r->nextd;
		delete_sem( r->sd);
		delete_sem( r->se);
		free( r);
	}
}


static void
set_sem_count( sem_id s, int vnew)
{
	int32	v;

	get_sem_count( s, &v);
	until (v == vnew)
		if (v < vnew)
			release_sem( s), ++v;
		else
			acquire_sem( s), --v;
}


bool
ddinit( )
{

	ddsem = create_sem( 1, "ddmain");
	return (ddsem >= 0);
}


void
dduninit( )
{

	while (rfreelist)
		rfree( );
	delete_sem( ddsem);
	idump( );
}
                                                                                                                                                                                                  

							Jan 19 2000
BeOS DATA DOMAINS
-----------------


INTRODUCTION

The files "dd.h" and "dd.c" provide a full-service synchronization
and arbitration service for the BeOS kernel.  Threads in the kernel
can block until unblocked by another thread or interrupt handler.
Access to data by multiple threads and interrupt handlers is serialized,
with interrupt-masking automatically managed.  No deadlocks of any kind
can arise.

Calls to BeOS functions involving spinlocks, semaphores and interrupts
are no longer needed, being replaced by the data domain functions
described below.  Both systems co-exist, and can call each other: the
side-effect will be some loss of data domain functionality.


TERMINOLOGY

Foreground: a state of processing where the CPU has taken an interrupt.
As relevant to data domains, this means execution is occurring in an
interrupt handler.

Background: a state of processing where the CPU is not in the foreground.
As relevant to data domains, this means a thread is executing code
somewhere in the kernel.  Masking interrupts does not change the state.

Rover: a flow of control, either background or foreground.  A background
rover is a BeOS thread.  A foreground rover is a flow of control through
an interrupt handler.

Data domain: one or more data elements grouped together for th