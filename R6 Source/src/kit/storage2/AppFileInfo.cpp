 be called
at load-time if service is provided through a module.

dduninit() shuts down the entire data domain system.  It must be called
at unload-time if service is being provided through a module.

ddomain is an opaque struct representing a data domain.  It must be
zeroed before first use.  All public data elements in the domain system
must map to exactly one domain.  Data elements of a domain may be grouped
with their ddomain struct, or may be spread around.  The programmer must
establish and maintain this mapping.

A data domain has either foreground or background type.  ddbackground()
can be called before a domain's first use.  By default, domains are
type foreground.

ddrover is an opaque struct representing a rover.  A ddrover struct
must be assigned as soon a rover enters the domain system.  For device
drivers this includes all entrypoints, such as device open and the
interrupt handler.  The rover may call out of the domain system, but if
it reenters it must be identified by its original struct ddrover.

A rover has either foreground or background type.  For background rovers,
a struct ddrover is allocated and initialized with

	ddrstart( 0)

For foreground rovers (those that appear at an interrupt handler),
you must provide your own struct ddrover.  Then call

	ddrstart( &your_rover)

The struct ddrover must be pre-allocated from the kernel heap and zeroed
before first use.

Data private to one rover can be used without fear of contention;
private data in the domain system means stack data, or data referenced
through the stack.  All other data must be accessed by first acquiring
the domain for that data.  This is achieved by called ddacquire().
There are four possible cases:

	(1) Foreground rover acquires foreground domain.  This rover may
	have to spin while the domain is being held by another rover.
	On return, the domain will be acquired and held by this rover.

	(2) Foreground rover acquires background domain -- not allowed.

	(3) Background rover acquires background domain.  This rover may
	have to block while the domain is being held by another rover.
	On return, the domain will be acquired and held by this rover.

	(4) Background rover acquires foreground domain -- see case (3).

During the call to ddacquire(), the rover will lose hold of all its
acquired domains.  Other rovers may then gain control of these domains,
and change data.  This is the essense of the data domain approach to
deadlock prevention.  When ddacquire() returns, all acquired domains
will once again be held, but assertions about domain data will be invalid.

ddrelease() removes the data domain from the rover's list of acquired
domains; this call doesn't block.

ddacquire() and ddrealease() ref-count all rovers which have acquired a
domain.  If a rover acquires a domain it already holds, the rover will not
deadlock, block or lose its hold on acquired domains, even momentarily.
The domain remains acquired until ddrelease() is called the matching
number of times.

A rover must call ddrdone() when leaving the domain system.  For
foreground rovers, the struct ddrover will be reset for the next use.
For background rovers, the struct is recyled.  Any domains still held
are released.

ddrhousekeep() performs some internal housekeeping, such as resource
reclamation, and can be ignored by casual users.

In addition to the data arbitration services just described, the data
domain system provides inter-rover sychronization.  A background rover
may block itself by calling ddsleep(), typically after inspecting certain
data elements.  Another rover, foreground or background, can later wake
up that sleeping rover with ddwakeup().  Holding the appropriate data
domain prevents any race condition.

The (void *) argument is known historically as a "wait channel".
It serves as a "meeting point" and can have any value, although the
address of some data element is customarily chosen.  Where multiple rovers
are sleeping on the same wait channel, ddwakeup() has the effect of waking
all of them.  This "broadcast" effect, and the unregulated nature of the
wait channel value, means a rover can be woken spuriously, or wake up
only to find another rover has seized the disputed resource.  The rover
must reevaluate the situation, and either give up or sleep again.

This sychronization mechanism is identical to the sleep/wakeup found
on UNIX, except that data domains allow it to work in a multiprocessor
environment.

ddsleeptimeout() is ddsleep() with a timeout.  If the rover is still
sleeping in the call after the given time, it is woken up.  ddsnooze()
allows a background rover to block for a given amount of time.

As with ddacquire(), the rover loses hold of all its acquired domains
while blocked in ddsleep(), ddsleeptimeout() and ddsnooze().

A background rover, holding no foreground domains, is allowed to call
parts of the kernel outside the domain system, and become blocked.
Since the rover's domains are still held, multithreaded performance for
those data elements might be reduced.  Blocking indefinitely in this
way is probably an error.

BeOS imposes severe restrictions on a processor with interrupts disabled
and spinlocks set, including a prohibition on BeOS kernel functions that
block.  Foreground rovers must obey these restrictions at all times.
Background rovers holding a foreground domain must also obey, except
that blocking data domain functions (e.g. ddacquire(), ddsleep()) are
always allowed.  This is key to interrupt-driven synchronization in the
domain system.


RETURN VALUES

ddinit() returns TRUE if the domain system is successfully initialized.
ddrstart() returns a pointer to an initialized struct ddrover, otherwise
0.  ddsleep(), ddsleeptimeout() and ddsnooze() return FALSE if signaled,
otherwise TRUE.
                                                                                                                                                                                                                                                                                                                                                                     

#include	<KernelExport.h>
#include	<tty/rico.h>
#include	<tty/intr.h>
#undef		idisable
#undef		irestore


#if LATENCY_INSTRUMENTATION


/*
 * interrupt latency resolution (usec)
 */
#define	RESOLUTION	10


static bigtime_t	tstart;
static uint		ttab[40];


void
istart( )
{

	tstart = system_time( );
}


void
idone( )
{

	uint t = system_time( ) - tstart;
	t /= RESOLUTION;
	unless (t < nel( ttab))
		t = nel( ttab) - 1;
	++ttab[t];
}


cpu_status
idisable( )
{

	cpu_status ps = disable_interrupts( );
	if (ps & 0x200)
		istart( );
	return (ps);
}


void
irestore( cpu_status ps)
{

	if (ps & 0x200)
		idone( );
	restore_interrupts( ps);
}


void
idump( )
{
	uint	i;

	dprintf( "interrupt latency:");
	for (i=0; i<nel( ttab); ++i)
		dprintf( " %d", ttab[i]);
	dprintf( "\n");
	memset( ttab, 0, sizeof ttab);
}
#endif
                                                                                                                                                                                                            

#include	<KernelExport.h>
#include	<Drivers.h>
#include	<termios.h>
#include	<tty/rico.h>
#include	<tty/str.h>
#include	<tty/barrier.h>
#include	<tty/dd.h>
#include	<tty/tty.h>


static struct tty	*ttysel;
static struct ddomain	ddi;
static thread_id	tid;
static bool		death;


int32
rtmain( void *a)
{
	struct ddrover	*r;
	struct tty	*tp;

	unless (r = ddrstart( 0))
		return (ENOMEM);
	ddacquire( r, &ddi);
	until (death)
		if (tp = ttysel) {
			ttysel = tp->ttysel;
			tp->ttysel = 0;
			ddrelease( r, &ddi);
			ttyprocsel( tp, r);
			ddacquire( r, &ddi);
		}
		else
			ddsleep( r, &ttysel);
	ddrdone( r);
	return (B_OK);
}


void
rtprocsel( struct tty *tp, struct ddrover *r)
{
	struct tty	*p;

	ddacquire( r, &ddi);
	unless (tp->ttysel)
		for (p=ttysel; p!=tp; p=p->ttysel)
			unless (p) {
				tp->ttysel = ttysel;
				ttysel = tp;
				ddwakeup( &ttysel);
				break;
			}
	ddrelease( r, &ddi);
}


void
rtinit( )
{

	tid = spawn_kernel_thread( rtmain, "tty rt", B_REAL_TIME_PRIORITY, 0);
	unless (tid < B_OK)
		resume_thread( tid);
}


void
rtuninit( )
{
	struct ddrover	*r;
	status_t	s,
			t;

	if (r = ddrstart( 0)) {
		ddacquire( r, &ddi);
		death = TRUE;
		ddrelease( r, &ddi);
		ddwakeup( &ttysel);
		s = wait_for_thread( tid, &t);
		ddrdone( r);
	}
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
#include	<stdlib.h>
#include	<tty/rico.h>
#include	<tty/str.h>


#define	min( a, b)	((a)<(b)? (a): (b))


void	*malloc( );


bool
salloc( struct str *s, uint n)
{

	sinit( s, malloc( n), n);
	s->allocated = TRUE;
	return (s->buffer != 0);
}


void
sinit( struct str *s, uchar buffer[], uint n)
{

	s->bufsize = n;
	s->buffer = buffer;
	s->count = 0;
	s->tail = 0;
	s->allocated = FALSE;
}


void
sfree( struct str *s)
{

	if ((s->allocated)
	and (s->buffer)) {
		free( s->buffer);
		s->buffer = 0;
	}
}


bool
sputb( struct str *s, uint c)
{

	unless ((s->buffer)
	and (s->count < s->bufsize))
		return (FALSE);
	s->buffer[(s->tail+s->count)%s->bufsize] = c;
	++s->count;
	return (TRUE);
}


bool
sputs( struct str *s, uchar *a, uint n)
{
	uint	i;

	unless (s->buffer)
		return (FALSE);
	while (n) {
		uint h = s->tail + s->count;
		if (h < s->bufsize)
			i = s->bufsize - h;
		else {
			h %= s->bufsize;
			i = s->tail - h;
		}
		unless (i = min( i, n))
			return (FALSE);
		memcpy( s->buffer+h, a, i);
		s->count += i;
		a += i;
		n -= i;
	}
	return (TRUE);
}


uint
sseglen( struct str *s)
{

	return (min( s->count, s->bufsize-s->tail));
}


uint
sgetb( struct str *s)
{
	uint	c;

	unless ((s->buffer)
	and (s->count))
		return (EOF);
	c = s->buffer[s->tail];
	s->tail = (s->tail+1) % s->bufsize;
	--s->count;
	return (c);
}


uchar	*
sgets( struct str *s, uint n)
{
	uchar	*p;

	if ((p = s->buffer)) {
		p += s->tail;
		s->tail = (s->tail+n) % s->bufsize;
		s->count -= min( s->count, n);
	}
	return (p);
}


void
sungets( struct str *s, uint n)
{

	s->tail = (s->tail+s->bufsize-n) % s->bufsize;
	s->count = min( s->count+n, s->bufsize);
}


uint
sunputb( struct str *s)
{

	unless ((s->buffer)
	and (s->count))
		return (EOF);
	--s->count;
	return (s->buffer[(s->tail+s->count)%s->bufsize]);
}
                                                                                                                                                                                                                                                      

#include	<KernelExport.h>
#include	<fsproto.h>
#include	<module.h>
#include	<Drivers.h>
#include	<signal.h>
#include	<termios.h>
#include	<tty/rico.h>
#include	<tty/str.h>
#include	<tty/barrier.h>
#include	<tty/dd.h>
#include	<tty/tty.h>


#define	BSIZE		2048
#define	ILOWATER	(BSIZE * 2/8)
#define	IHIWATER	(BSIZE * 7/8)
#define	OHIWATER	(BSIZE * 7/8)
#define	OLOWATER	(BSIZE * 1/8)


struct sel {
	struct sel	*next;
	uint		ref;
	selectsync	*sync;
	uint8		event;
};


const static struct termios TDEFAULT = {
	0,			/* c_iflag */
	OPOST,			/* c_oflag */
	B19200|CS8|CREAD|HUPCL,	/* c_cflag */
	0,			/* c_lflag */
	0,			/* c_line */
	0,			/* c_ixxxxx */
	0,			/* c_oxxxxx */
	{
		ctrl( 'C'),		/* c_cc[VINTR] */
		ctrl( '\\'),		/* c_cc[VQUIT] */
		ctrl( 'H'),		/* c_cc[VERASE] */
		ctrl( 'U'),		/* c_cc[VKILL] */
		ctrl( 'D'),		/* c_cc[VMIN] */
		10,			/* c_cc[VTIME] */
		0,			/* c_cc[VEOL2] */
		0,			/* c_cc[VSWTCH] */
		ctrl( 'S'),		/* c_cc[VSTART] */
		ctrl( 'Q'),		/* c_cc[VSTOP] */
		0			/* c_cc[VSUSP] */
	}
};
const static struct winsize WDEFAULT = {
	24, 80, 240, 800
};


static status_t	releasedd( struct tty *, struct ddrover *, status_t);
static int	waitevent( struct tty *, struct ddrover *, uint *);
static int	waiticount( struct tty *, struct ddrover *, int *);
static int	waitocount( struct tty *, struct ddrover *, int *);
static void	seldeq( struct tty *, struct sel *);
static void	drainraw( struct tty *, struct ddrover *, bool);
static void	iproc( struct tty *, struct ddrover *, int);
static void	evalcarrier( struct tty *);
static void	evalwritable( struct tty *, struct ddrover *);
static void	iresume( struct tty *, struct ddrover *);
static void	echo( struct tty *, struct ddrover *, uint);
static void	oproc( struct tty *, struct str *, int);
static void	ostart( struct tty *, struct ddrover *);
static void	cleartty( struct tty *);
//static void	setflag( struct tty *, struct ddrover *, uint); // not really used
//static void	clrflag( struct tty *, struct ddrover *, uint); // not really used
static bool	osync( struct tty *, struct ddrover *);
static bool	terminated( struct tty *);
       void	ttyprocsel( struct tty *, struct ddrover *);
extern void	*malloc( );

static bigtime_t	getvtime( struct ttyfile *);


void
ttyinit( struct tty *tp, bool bg)
{

	ddbackground( &tp->dd);
	if (bg)
		ddbackground( &tp->ddi);
}


/*
 * To do:
 *	software flow control
 *	SETA* needs needs flushing & waiting behavior
 *	SETA* needs correct RTS response (evaliresume)
 */
status_t
ttyopen( struct ttyfile *tf, struct ddrover *r, bool (*s)( struct tty *, struct ddrover *, uint))
{
	struct tty	*tp	= tf->tty;

	tf->vtime = 0;
	ddacquire( r, &tp->dd);
	if (tp->flags & TTYEXCLUSIVE)
		return (releasedd( tp, r, B_BUSY));
	if (tf->flags & O_EXCL) {
		if (tp->nopen)
			return (releasedd( tp, r, B_BUSY));
		tp->flags |= TTYEXCLUSIVE;
	}
	unless (tp->nopen++) {
		unless ((salloc( &tp->istr, BSIZE))
		and (salloc( &tp->rstr, BSIZE))
		and (salloc( &tp->ostr, BSIZE))) {
			cleartty( tp);
			return (releasedd( tp, r, B_ERROR));
		}
		tp->t = TDEFAULT;
		tp->wsize = WDEFAULT;
		tp->flags |= TTYREADING;
		tp->service = s;
		unless ((*tp->service)( tp, r, TTYENABLE)) {
			cleartty( tp);
			return (releasedd( tp, r, ENODEV));
		}
	}
	unless (tf->flags & O_NONBLOCK) {
		ddacquire( r, &tp->ddi);
		until (tp->flags & TTYCARRIER) {
			unless (ddsleep( r, tp)) {
				ddrelease( r, &tp->ddi);
				unless (--tp->nopen) {
					(*tp->service)( tp, r, TTYDISABLE);
					cleartty( tp);
				}
				return (releasedd( tp, r, B_INTERRUPTED));
			}
		}
		ddrelease( r, &tp->ddi);
	}
	return (releasedd( tp, r, B_OK));
}


status_t
ttyclose( struct ttyfile *tf, struct ddrover *r)
{
	struct tty	*tp	= tf->tty;
	bigtime_t	end	= system_time( ) + 3000000;

	ddacquire( r, &tp->dd);
	ddacquire( r, &tp->ddi);
	while (tp->nopen == 1)
		if (scount( &tp->ostr)) {
			unless ((system_time( ) < end)
			and (ddsleeptimeout( r, tp, 1000000)))
				break;
		}
		else {
			if ((*tp->service)( tp, r, TTYOSYNC)) {
				ddsnooze( r, 50000);
				break;
			}
			unless (ddsnooze( r, 10000))
				break;
		}
	ddrelease( r, &tp->ddi);
	return (releasedd( tp, r, B_OK));
}


status_t
ttyfree( struct ttyfile *tf, struct ddrover *r)
{
	struct tty	*tp	= tf->tty;

	ddacquire( r, &tp->dd);
	unless (--tp->nopen) {
		(*tp->service)( tp, r, TTYDISABLE);
		cleartty( tp);
		ddrhousekeep( );
		snooze( 200000);
	}
	return (releasedd( tp, r, B_OK));
}


status_t
ttyread( struct ttyfile *tf, struct ddrover *r, char buf[], size_t *count)
{
	struct tty	*tp	= tf->tty;
	bigtime_t	lastt;
	uint		i	= 0;
	uint		lastn	= ~0;
	uint		n;

	status_t s = B_OK;
	ddacquire( r, &tp->dd);
	loop {
		uint ia = tp->iactivity;
		bigtime_t d = 0;
		drainraw( tp, r, TRUE);
		n = scount( &tp->istr);
		unless (n == lastn) {
			lastn = n;
			lastt = system_time( );
		}
		if (tp->t.c_lflag & ICANON) {
			if (terminated( tp)) {
				n = min( n, *count);
				while (i < n) {
					uint c = sgetb( &tp->istr);
					if (c == tp->t.c_cc[VEOF])
						break;
					buf[i++] = c;
					if (c == '\n')
						break;
				}
				iresume( tp, r);
				*count = i;
				break;
			}
			unless (tp->flags & TTYREADING) {
				unless (n < ILOWATER) {
					n = min( n-ILOWATER, *count);
					while (i < n)
						buf[i++] = sgetb( &tp->istr);
				}
				iresume( tp, r);
				*count = i;
				break;
			}
		}
		else {
			unless (n < *count) {
				while (i < *count)
					buf[i++] = sgetb( &tp->istr);
				iresume( tp, r);
				*count = i;
				break;
			}
			if (tp->t.c_cc[VMIN]) {
				unless (n < tp->t.c_cc[VMIN]) {
					while (i < n)
						buf[i++] = sgetb( &tp->istr);
					iresume( tp, r);
					*count = i;
					break;
				}
				if (n) {
					bigtime_t vt = getvtime( tf);
					if (vt) {
						d = lastt+vt - system_time( );
						if (d <= 0) {
							while (i < n) {
								buf[i++] = sgetb( &tp->istr);
							}
							iresume( tp, r);
							*count = i;
							break;
						}
					}
				}
			}
			else {
				bigtime_t vt = getvtime( tf);
				d = lastt+vt - system_time( );
				if ((n) or (d <= 0)) {
					while (i < n)
						buf[i++] = sgetb( &tp->istr);
					iresume( tp, r);
					*count = i;
					break;
				}
			}
		}
		unless (tp->flags & TTYCARRIER) {
			*count = 0;
			s = EIO;
			break;
		}
		if (tf->flags & O_NONBLOCK) {
			*count = 0;
			s = EAGAIN;
			break;
		}
		ddacquire( r, &tp->ddi);
		if (tp->iactivity == ia) {
			if (d) {
				d = max( d, 1000);		// kernel bug?
			}
			unless (ddsleeptimeout( r, tp, d)) {
				ddrelease( r, &tp->ddi);
				*count = 0;
				s = B_INTERRUPTED;
				break;
			}
		}
		ddrelease( r, &tp->ddi);
	}
	drainraw( tp, r, FALSE);
	return (releasedd( tp, r, s));
}


status_t
ttywrite( struct ttyfile *tf, struct ddrover *r, const char buf[], size_t *count)
{
	struct tty	*tp	= tf->tty;
	const uint	OCSIZE	= 8,
			WSIZE	= 200 + OCSIZE;
	uchar		wbuf[WSIZE];
	struct str	s;
	uint		n,
			i;

	ddacquire( r, &tp->dd);
	sinit( &s, wbuf, WSIZE);
	n = 0;
	i = 0;
	while (tp->flags & TTYCARRIER) {
		if (n+i < *count) {
			if (scount( &s) <= WSIZE-OCSIZE) {
				oproc( tp, &s, buf[n+i]);
				++i;
				continue;
			}
		}
		else unless (scount( &s))
			break;
		ddacquire( r, &tp->ddi);
		if (scount( &tp->ostr)+scount( &s) <= OHIWATER) {
			uint j;
			while ((j = sseglen( &s))) {
				sputs( &tp->ostr, sgets( &s, j), j);
			}
			ostart( tp, r);
			ddrelease( r, &tp->ddi);
			n += i;
			i = 0;
			continue;
		}
		if (tf->flags & O_NONBLOCK) {
			ddrelease( r, &tp->ddi);
			if ((*count = n)) {
				return (releasedd( tp, r, B_OK));
			}
			return (releasedd( tp, r, EAGAIN));
		}
		unless (ddsleep( r, tp)) {
			ddrelease( r, &tp->ddi);
			*count = 0;
			return (releasedd( tp, r, B_INTERRUPTED));
		}
		ddrelease( r, &tp->ddi);
	}
	if ((*count = n)) {
		return (releasedd( tp, r, B_OK));
	}
	return (releasedd( tp, r, EIO));
}


status_t
ttyselect( struct ttyfile *tf, struct ddrover *r, uint8 event, uint32 ref, selectsync *sync)
{
	struct tty	*tp	= tf->tty;
	struct sel	*p;

	unless (p = malloc( sizeof *p))
		return (ENOMEM);
	p->event = event;
	p->ref = ref;
	p->sync = sync;
	ddacquire( r, &tp->dd);
	ddacquire( r, &tp->ddi);
	p->next = tp->sel;
	tp->sel = p;
	ddrelease( r, &tp->ddi);
	ttyprocsel( tp, r);
	ddrelease( r, &tp->dd);
	return (B_OK);
}


status_t
ttydeselect( struct ttyfile *tf, struct ddrover *r, uint8 event, selectsync *sync)
{
	struct tty	*tp	= tf->tty;
	struct sel	*p;

	ddacquire( r, &tp->dd);
	ddacquire( r, &tp->ddi);
	p = tp->sel;
	loop {
		unless (p) {
			ddrelease( r, &tp->ddi);
			break;
		}
		if ((p->event == event)
		and (p->sync == sync)) {
			seldeq( tp, p);
			ddrelease( r, &tp->ddi);
			free( p);
			break;
		}
		p = p->next;
	}
	ddrelease( r, &tp->dd);
	return (B_OK);
}


status_t
ttycontrol( struct ttyfile *tf, struct ddrover *r, ulong com, void *buf, size_t len)
{
	struct tty	*tp	= tf->tty;
	struct termio	t;
	bigtime_t	vt;
	int		i;

	ddacquire( r, &tp->dd);
	switch (com) {
	case TCGETA:
		*(struct termio *)buf = tp->t;
		break;
	case TCSETA:
	case TCSETAW:
	case TCSETAF:
		t = *(struct termio *)buf;
		ddacquire( r, &tp->ddi);
		if (t.c_cflag == tp->t.c_cflag)
			tp->t = t;
		else {
			unless ((com == TCSETA)
			or (osync( tp, r))) {
				ddrelease( r, &tp->ddi);
				return (releasedd( tp, r, B_INTERRUPTED));
			}
			tp->t = t;
			(*tp->service)( tp, r, TTYSETMODES);
		}
		evalwritable( tp, r);
		evalcarrier( tp);
		if (com == TCSETAF) {
			sclear( &tp->istr);
			iresume( tp, r);
		}
		ddrelease( r, &tp->ddi);
		break;
	case TCWAITEVENT:
		i = waitevent( tp, r, buf);
		return (releasedd( tp, r, i));
	case TCFLSH:
		ddacquire( r, &tp->ddi);
		switch ((int)buf) {
		case 2:
			sclear( &tp->ostr);
		case 0:
			sclear( &tp->istr);
			iresume( tp, r);
			break;
		case 1:
			sclear( &tp->ostr);
		}
		ddrelease( r, &tp->ddi);
		ddwakeup( tp);
		break;
	case TCSBRK:
		unless (osync( tp, r)) {
			return (releasedd( tp, r, B_INTERRUPTED));
		}
		if (buf == 0) {
			ddacquire( r, &tp->ddi);
			(*tp->service)( tp, r, TTYSETBREAK);
			ddsnooze( r, 250000);
			(*tp->service)( tp, r, TTYCLRBREAK);
			ddrelease( r, &tp->ddi);
		}
		break;
	case TCXONC:
	case TCQUERYCONNECTED:
		break;
	case TCSETDTR:
		i = *(bool *)buf? TTYSETDTR: TTYCLRDTR;
		ddacquire( r, &tp->ddi);
		(*tp->service)( tp, r, i);
		ddrelease( r, &tp->ddi);
		break;
	case TCSETRTS:
		unless (tp->flags & TTYFLOWFORCED) {
			i = *(bool *)buf? TTYIRESUME: TTYISTOP;
			ddacquire( r, &tp->ddi);
			(*tp->service)( tp, r, i);
			ddrelease( r, &tp->ddi);
		}
		break;
	case TCGETBITS:
		if (buf) {
			uint i = 0;
			ddacquire( r, &tp->ddi);
			(*tp->service)( tp, r, TTYGETSIGNALS);
			if (tp->flags & TTYHWCTS)
				i |= TCGB_CTS;
			if (tp->flags & TTYHWDSR)
				i |= TCGB_DSR;
			if (tp->flags & TTYHWDCD)
				i |= TCGB_DCD;
			if (tp->flags & TTYHWRI)
				i |= TCGB_RI;
			ddrelease( r, &tp->ddi);
			*(uint *)buf = i;
		}
		break;
	case TIOCGWINSZ:
		*(struct winsize *)buf = tp->wsize;
		break;
	case TIOCSWINSZ:
	case 'wsiz':
		unless ((tp->wsize.ws_row == ((struct winsize *)buf)->ws_row)
		and (tp->wsize.ws_col == ((struct winsize *)buf)->ws_col)) {
			tp->wsize = *(struct winsize *)buf;
			if (tp->pgid)
				send_signal( -tp->pgid, SIGWINCH);
		}
		break;
	case B_SET_BLOCKING_IO:
		tf->flags &= ~ O_NONBLOCK;
		ddwakeup( tp);
		break;
	case B_SET_NONBLOCKING_IO:
		tf->flags |= O_NONBLOCK;
		ddwakeup( tp);
		break;
	case TCVTIME:
		vt = tf->vtime;
		tf->vtime = *(bigtime_t *)buf;
		*(bigtime_t *)buf = vt;
		break;
	case 'pgid':
		tp->pgid = (pid_t) buf;
		break;
	case 'ichr':
		return (releasedd( tp, r, waiticount( tp, r, buf)));
	case 'ochr':
		return (releasedd( tp, r, waitocount( tp, r, buf)));
	default:
		return (releasedd( tp, r, EINVAL));
	}
	return (releasedd( tp, r, B_OK));
}


void
ttyin( struct tty *tp, struct ddrover *r, int c)
{

	++tp->iactivity;
	if ((tp->ibusy)
	and (not (tp->flags&TTYFLOWFORCED)))
		sputb( &tp->rstr, c);
	else
		iproc( tp, r, c);
}


int
ttyout( struct tty *tp, struct ddrover *r)
{
	int	c;

	if ((scount( &tp->ostr))
	and (tp->flags & TTYWRITABLE))
		c = sgetb( &tp->ostr);
	else {
		tp->flags &= ~TTYWRITING;
		ddwakeup( tp);
		c = -1;
	}
	return (c);
}


void
ttyhwsignal( struct tty *tp, struct ddrover *r, int sig, bool asserted)
{

	ddacquire( r, &tp->ddi);
	switch (sig) {
	case TTYHWDCD:
		if (asserted)
			tp->flags |= TTYHWDCD;
		else
			tp->flags &= ~ TTYHWDCD;
		evalcarrier( tp);
		break;
	case TTYHWCTS:
		if (asserted)
			tp->flags |= TTYHWCTS;
		else
			tp->flags &= ~ TTYHWCTS;
		evalwritable( tp, r);
		break;
	case TTYHWDSR:
		if (asserted)
			tp->flags |= TTYHWDSR;
		else
			tp->flags &= ~ TTYHWDSR;
		break;
	case TTYHWRI:
		if (asserted)
			tp->flags |= TTYHWRI;
		else
			tp->flags &= ~ TTYHWRI;
	}
	++tp->iactivity;
	ddrelease( r, &tp->ddi);
}


void
ttyilock( struct tty *tp, struct ddrover *r, bool b)
{

	if (b)
		ddacquire( r, &tp->ddi);
	else {
		if (tp->sel) {
			rtprocsel( tp, r);
		}
		ddwakeup( tp);
		ddrelease( r, &tp->ddi);
	}
}


void
ttyprocsel( struct tty *tp, struct ddrover *r)
{
	struct sel	*p;

	ddacquire( r, &tp->dd);
	ddacquire( r, &tp->ddi);
	p = tp->sel;
	while (p) {
		switch (p->event) {
		case SELECT_READ:
			if (scount( &tp->rstr) + scount( &tp->istr))
				break;
			p = p->ne