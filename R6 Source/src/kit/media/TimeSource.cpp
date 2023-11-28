/*	BTimeSource.cpp	*/


/*	Real Time for TimeSource means the master system clock */
/*	Performance Time is the output of the BTimeSource; typically you will have */
/*	PerformanceTime = RealTime * Scale - Delta */
/*	Performance Time for any other node kind means what time the TimeSource thinks it is */
/*	and Media Time is its internal offset from start; typically it will be */
/*	MediaTime = PerformanceTime - StartDelta */

/*	For a TimeSource to be started means it's calling PublishTime() with a positive scale. */
/*	For a TimeSource to be stopped means it's called PublishTime() with a 0.0 scale and is */
/*	not calling PublishTime() until started again. */

#include "trinity_p.h"
#include "TimeSource.h"
#include "tr_debug.h"
#include "timesource_p.h"

#include <OS.h>
#include <stdio.h>
#include <Debug.h>
#include <string.h>
#include <limits.h>

#include <set>
#include <algorithm>

#define DO_NOTHING(x...)

#if DEBUG
#define FPRINTF fprintf
#else
#define FPRINTF DO_NOTHING
#endif

#define DIAGNOSTIC fprintf


//	How long to give other people time when we're waiting for the
//	timing buffer to become whole again, in microseconds.
#define BLIP_TIME 20


/* default is to just use system time */

class _BSlaveNodeStorageP : 
	public std::map<media_node_id, port_id>
{
public:
	_BSlaveNodeStorageP();
	_BSlaveNodeStorageP(
			const _BSlaveNodeStorageP & clone);
	_BSlaveNodeStorageP & operator=(
			const _BSlaveNodeStorageP & clone);
};

_BSlaveNodeStorageP::_BSlaveNodeStorageP()
{
	/* do nothing */
}

_BSlaveNodeStorageP::_BSlaveNodeStorageP(
	const _BSlaveNodeStorageP & clone) :
	std::map<media_node_id, port_id>(clone)
{
}

_BSlaveNodeStorageP &
_BSlaveNodeStorageP::operator=(
	const _BSlaveNodeStorageP & clone)
{
	std::map<media_node_id, port_id>::operator=(clone);
	return *this;
}

status_t
BTimeSource::SnoozeUntil(
	bigtime_t performance_time,
	bigtime_t with_latency,
	bool retry_signals)
{
again:
	status_t err = snooze_until(RealTimeFor(performance_time, with_latency), 
		B_SYSTEM_TIMEBASE);
	if (err == B_INTERRUPTED && retry_signals) {
		goto again;
	}
	if (!err && _mStopped)
	{
		err = B_MEDIA_TIME_SOURCE_STOPPED;
	}
	return err;
}

bigtime_t
BTimeSource::Now()
{
//	dlog("Now");
	return PerformanceTimeFor(RealTime());
}

bigtime_t
BTimeSource::PerformanceTimeFor(	/* both go through GetTime() */
	bigtime_t at_time)
{
	// don't do that!
	if (at_time == B_INFINITE_TIMEOUT)
		return B_INFINITE_TIMEOUT;
		
	bigtime_t performance_time = 0;
	bigtime_t real_time = 0;
	float drift = 1.0;
	status_t err;
	int blip = 0;
	int tries = 0;

	while ((B_OK != (err = GetTime(&performance_time, &real_time, &drift))) && (err != B_MEDIA_TIME_SOURCE_STOPPED)) {
		if (tries++ > 2) {
			if (tries > 15) {
				//	this is a pretty weird error condition -- typically something has gone awry
				return B_MEDIA_TIME_SOURCE_BUSY;
			}
			snooze(blip += BLIP_TIME);	// re-try, spinning, until we get it right
		}
	}
	if (_mStopped || (drift < 0.0001)) {
		return performance_time;
	}
	return performance_time + (bigtime_t)((at_time-real_time)*drift);
}

bigtime_t
BTimeSource::RealTimeFor(
	bigtime_t at_time,
	bigtime_t with_latency)
{
	// don't do that!
	if (at_time == B_INFINITE_TIMEOUT)
		return B_INFINITE_TIMEOUT;

	bigtime_t performance_time = 0;
	bigtime_t real_time = 0;
	float drift = 1.0;
	status_t err;
	int blip = 0;
	int tries = 0;

	while ((0 != (err = GetTime(&performance_time, &real_time, &drift))) && (err != B_MEDIA_TIME_SOURCE_STOPPED)) {
		if (tries++ > 2) {
			if (tries > 15) {
				return B_MEDIA_TIME_SOURCE_BUSY;
			}
			snooze(blip += BLIP_TIME);	// re-try, spinning, until we get it right
		}
	}
	if (_mStopped || (drift < 0.0001)) {
		return (9223372036854775807LL);	// INT64_MAX
	}
	return real_time + (bigtime_t)((at_time-performance_time)/drift) - with_latency;
}

bool
BTimeSource::IsRunning()
{
	bigtime_t ignore_a, ignore_b;
	float ignore_c;
	status_t err;
	int blip = 0;
	int tries = 0;

	while ((0 != (err = GetTime(&ignore_a, &ignore_b, &ignore_c))) && (err != B_MEDIA_TIME_SOURCE_STOPPED)) {
		if (tries++ > 2) {
			if (tries > 15) {
				return true;
			}
			snooze(blip += BLIP_TIME);	// spin until we get a good reading, which updates _mStopped
		}
	}
				
	return !_mStopped;
}

status_t
BTimeSource::GetTime(	/* return B_OK if reading is trusted (it should be */
						/* at least one time out of four) */
	bigtime_t * media,
	bigtime_t * real,
	float * drift)
{
	*media = 0;
	*real = 0;
	*drift = 0.0;

	if (!dcheck(_mBuf != NULL)) {
		dlog("GetTime with no _mBuf (this=%x node=%d)", this, ID());
		return B_OK;	/* to avoid looping forever */
	}

	if (_mBuf->isStatic) {
		_mStopped = _mBuf->u.static_data.GetTime(media,real,drift);
	} else {
		uint32 back_count = _mBuf->u.dynamic_data.back_count;
		int32 ix = (back_count-1) & (N_TIME_TRANSMIT_STAMPS-1);
		bigtime_t was = _mBuf->u.dynamic_data.stamps[ix].performance_time;
		bigtime_t then = _mBuf->u.dynamic_data.stamps[ix].real_time;
		float xdrift = _mBuf->u.dynamic_data.stamps[ix].drift;
		*media = was;
		*real = then;
		*drift = xdrift;
		_mStopped = (xdrift < 0.000001);
		if (_mBuf->u.dynamic_data.back_count >= back_count + N_TIME_TRANSMIT_STAMPS)
			return B_MEDIA_TIME_SOURCE_BUSY;
	}
	return (_mStopped ? B_MEDIA_TIME_SOURCE_STOPPED : B_OK);
}


bigtime_t
BTimeSource::RealTime()	/* this produces real time, nothing else */
			/* is guaranteed to */
{
	return system_time();
}


status_t
BTimeSource::GetStartLatency(
	bigtime_t * out_latency)
{
	if (!dcheck(_mBuf != NULL)) {
		dlog("GetStartLatency with no _mBuf (this=%x node=%d)", this, ID());
		return B_NO_INIT;	/* to avoid looping forever */
	}
	*out_latency = 16384 + 4000 * _mBuf->u.dynamic_data.client_count;
	return B_OK;
}


BTimeSource::BTimeSource() :
	BMediaNode("%ERROR%")
{
	AddNodeKind(B_TIME_SOURCE);
	_mSlaveNodes = new _BSlaveNodeStorageP;
	/* can't create the area until we know the node ID... */
	_mStopped = true;
	_mArea = -1;
	_mBuf = NULL;
}


void
BTimeSource::FinishCreate()
{
	if (_mArea > 0) {
		dlog("Multiple call to FinishCreate() detected for TimeSource %d (area %d)", ID(), _mArea);
		return;
	}
	void * addr = NULL;
	char aname[B_OS_NAME_LENGTH];
	sprintf(aname, "_BTimeSource %ld", ID());
	_mOrigArea = create_area(aname, &addr, B_ANY_ADDRESS, 
		(sizeof(_time_transmit_buf)+B_PAGE_SIZE-1)&~(B_PAGE_SIZE-1),
		B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	((_BMediaRosterP*)BMediaRoster::Roster())->RegisterDedicatedArea(_mOrigArea);
	_mArea = ((_BMediaRosterP*)BMediaRoster::Roster())->NewAreaUser(_mOrigArea);
	_mBuf = (_time_transmit_buf *)addr;
	memset((void *)_mBuf, 0, sizeof(*_mBuf));
	_mBuf->isStatic = 0;
	PublishTime(0, 0, 0.0);
}


BTimeSource::~BTimeSource()
{
	/*** notify slave nodes we're going away? ***/
	delete _mSlaveNodes;
	_BMediaRosterP * r = (_BMediaRosterP*)BMediaRoster::CurrentRoster();
	if (r) r->RemoveAreaUser(_mOrigArea);
	_mArea = B_BAD_VALUE;
}


void
BTimeSource::PublishTime(	/* call this at convenient times to */
				/* update approximation */
	bigtime_t media,
	bigtime_t real,
	float drift)
{
	if (_mBuf->isStatic) {
		DIAGNOSTIC(stderr, "PublishTime called on a static time source!\n");
		return;
	};
	
	if (!dcheck(_mBuf != NULL)) {
		dlog("_mBuf == NULL in PublishTime");
		return;
	}
//	dlog("PublishTime(%Ld, %Ld, %g)", media, real, drift);

/*** There is a race condition here! ***/
/*** If two people call PublishTime at the same time, you can get into ***/
/*** a situation where guy 1 adds to front_count, then guy 2 adds to ***/
/*** front_count and back_count, which means that the stamp updated ***/
/*** by guy 1 may be invalid until back_count is updated again! As ***/
/*** protection, the GetTime() function checks for this problem and ***/
/*** returns false if bad. However, the real solution is to only call ***/
/*** PublishTime() from one place so there will be no overlap problem. ***/

	int32 ix = atomic_add((int32 *)&_mBuf->u.dynamic_data.front_count, 1);
	ix = ix & (N_TIME_TRANSMIT_STAMPS-1);
	_mBuf->u.dynamic_data.stamps[ix].performance_time = media;
	_mBuf->u.dynamic_data.stamps[ix].real_time = real;
	_mBuf->u.dynamic_data.stamps[ix].drift = drift;
	atomic_add((int32 *)&_mBuf->u.dynamic_data.back_count, 1); /* it's safe to read us */
}


BTimeSource::BTimeSource(
	media_node_id id) :
	BMediaNode("%ERROR%", id, B_TIME_SOURCE)
{
	AddNodeKind(B_TIME_SOURCE);
	_mSlaveNodes = new _BSlaveNodeStorageP;
	char aname[B_OS_NAME_LENGTH];
	sprintf(aname, "_BTimeSource %ld", ID());
	void * addr = NULL;
	_mStopped = true;
	_mOrigArea = find_area(aname);
	if (_mOrigArea < 0) {
		_mArea = _mOrigArea;
		dlog("Cannot find area %s", aname);
	}
	else {
		area_info ai;
		_mArea = ((_BMediaRosterP*)BMediaRoster::Roster())->NewAreaUser(_mOrigArea);
		get_area_info(_mArea,&ai);
		addr = ai.address;
//		clone_area(aname, &addr, B_ANY_ADDRESS, 
//			B_READ_AREA|B_WRITE_AREA, orig);
		dlog("Cloned area is %x", _mArea);
	}
	_mBuf = (_time_transmit_buf *)addr;
}


status_t
BTimeSource::AddMe(
	BMediaNode * node)
{
	if (_mBuf->isStatic) {
		return _mBuf->u.static_data.AddClient(node->ControlPort());
	} else {
		add_node_q cmd;
		cmd.node = node->ID();
		status_t err = write_port(ControlPort(), TS_ADD_NODE, &cmd, sizeof(cmd));
		if (err > 0) err = B_OK;
		return err;
	};
}


status_t
BTimeSource::RemoveMe(
	BMediaNode * node)
{
	if (_mBuf->isStatic) {
		return _mBuf->u.static_data.RemoveClient(node->ControlPort());
	} else {
		remove_node_q cmd;
		cmd.node = node->ID();
		status_t err = write_port(ControlPort(), TS_REMOVE_NODE, &cmd, sizeof(cmd));
		if (err > 0) err = B_OK;
		return err;
	};
}


	class _SendFunc
	{
	public:
		int32 _code;
		const void * _data;
		ssize_t _size;
		inline _SendFunc(
				int32 code,
				const void * data,
				ssize_t size)
			{
				_code = code;
				_data = data;
				_size = size;
			}
		inline void operator() (
				pair<const media_node_id, port_id> p)
			{
				write_port(p.second, _code, _data, _size);
			}
	};


void
BTimeSource::BroadcastTimeWarp(
	bigtime_t real_time,
	bigtime_t performance_time)
{
	if (!_mSlaveNodes) return;

	timewarp_q cmd;
	cmd.real_time = real_time;
	cmd.performance_time = performance_time;
	for_each(_mSlaveNodes->begin(), _mSlaveNodes->end(), _SendFunc(M_TIMEWARP, &cmd, sizeof(cmd)));
}


void
BTimeSource::SendRunMode(
	run_mode mode)
{
	if (!_mSlaveNodes) return;

	set_run_mode_q cmd;
	cmd.mode = mode;
	for_each(_mSlaveNodes->begin(), _mSlaveNodes->end(), _SendFunc(M_SET_RUN_MODE, &cmd, sizeof(cmd)));
}


void
BTimeSource::SetRunMode(
	run_mode mode)
{
	SendRunMode(mode);
}


status_t
BTimeSource::HandleMessage(
	int32 message,
	const void * data,
	size_t /*size*/)
{
	switch (message) {
	case TS_ADD_NODE: {
			media_node mnode;
			if (B_OK == ((_BMediaRosterP *)BMediaRoster::Roster())->GetNodeFor(
				((add_node_q *)data)->node, &mnode)) {
				if (!atomic_add((int32 *)&_mBuf->u.dynamic_data.client_count, 1) && !_mSlaveNodes)
					_mSlaveNodes = new _BSlaveNodeStorageP;
				_mSlaveNodes->insert(pair<const media_node_id, port_id>(mnode.node, mnode.port));

				// Release ref to node.
				BMediaRoster::Roster()->ReleaseNode(mnode);
			}
		}
		break;
	case TS_REMOVE_NODE: {
			std::map<media_node_id, port_id>::iterator i = _mSlaveNodes->find(((remove_node_q *)data)->node);
			if (i != _mSlaveNodes->end()) {
				_mSlaveNodes->erase(i);
				atomic_add((int32 *)&_mBuf->u.dynamic_data.client_count, -1);
			}
		}
		break;
	case TS_GET_START_LATENCY: {
			get_start_latency_a ans = { 0, 0, 0 };
			ans.error = GetStartLatency(&ans.latency);
			ans.cookie = ((get_start_latency_q *)data)->cookie;
			if (0 > write_port_etc(((get_start_latency_q *)data)->reply, 
				TS_GET_START_LATENCY_REPLY, &ans, sizeof(ans), B_TIMEOUT, 500000)) {
				dlog("TS_GET_START_LATENCY reply gets error (timeout?)");
			}
		}
		break;
	
	case TS_OP: {
			ts_op_a ans = {B_OK};
			time_source_op_info *info = (time_source_op_info *)data;
			ans.error = TimeSourceOp(*info, NULL);
			if (info->op == B_TIMESOURCE_STOP_IMMEDIATELY) {
				if (0 > write_port_etc(((ts_op_q *)data)->reply, 
					TS_OP_REPLY, &ans, sizeof(ans), B_TIMEOUT, 500000)) {
					dlog("TS_OP_REPLY reply gets error (timeout?)");
				}
			}
		}
		break;
	
	default:
		return B_ERROR;
	}
	return B_OK;
}

void 
BTimeSource::DirectStart(bigtime_t at)
{
	_mBuf->u.static_data.Start(at);
}

void 
BTimeSource::DirectStop(bigtime_t at, bool immediate)
{
	_mBuf->u.static_data.Stop(at,immediate);
}

void 
BTimeSource::DirectSeek(bigtime_t to, bigtime_t at)
{
	_mBuf->u.static_data.Seek(to,at);
}

void 
BTimeSource::DirectSetRunMode(run_mode mode)
{
	_mBuf->u.static_data.SetRunMode(mode);
}

_BTimeSourceP::_BTimeSourceP(
	media_node_id source) :
	BMediaNode("Cloned BTimeSource", source, B_TIME_SOURCE),
	BTimeSource(source)
{
	status_t error = ((_BMediaRosterP *)BMediaRoster::Roster())->GetNodeFor(source, &m_node);
	if ((error >= B_OK) && (m_node != media_node::null)) {
		_mControlPort = m_node.port;
	} else {
		dlog("_BTimeSourceP error %x (%s) node %d", error, strerror(error), m_node.node);
		_mControlPort = error;
	}
	FPRINTF(stderr, "BTimeSourceP::_mControlPort is %ld\n", _mControlPort);
}

BMediaAddOn *
_BTimeSourceP::AddOn(
	int32 * internal_id) const
{
	if (internal_id) *internal_id = 0;
	return NULL;
}

port_id
_BTimeSourceP::ControlPort() const
{
	return _mControlPort;
}

void
_BTimeSourceP::ReleaseMaster()
{
	if (m_node.node < 0) return;
	BMediaRoster * r = BMediaRoster::CurrentRoster();
	if (r != NULL) r->ReleaseNode(m_node);
	m_node.node = -1;
}

_BTimeSourceP::~_BTimeSourceP()
{
	if (m_node.node != -1) { // can be removed by ReleaseMaster
		BMediaRoster * r = BMediaRoster::CurrentRoster();
		if (r != NULL) {
			r->ReleaseNode(m_node);
		}
	};
}

status_t 
_BTimeSourceP::TimeSourceOp(const time_source_op_info &/*op*/, void */*_reserved*/)
{
	ASSERT("_BTimeSourceP::TimeSourceOp should never be called!\n");
	return B_ERROR;
}


//--------------------------------------------------------------

_SysTimeSource::_SysTimeSource(
	const char * name) :
	BMediaNode(name),
	BTimeSource()
{
	BMediaRoster::Roster()->RegisterNode(this);
	_mBuf->isStatic = 1;
	_mBuf->u.static_data.waterLine = -1;
	_mBuf->u.static_data.stopTime = 0x7FFFFFFFFFFFFFFFLL;
	_mBuf->u.static_data.startTime = 0x7FFFFFFFFFFFFFFFLL;
	_mBuf->u.static_data.stoppedAt = 0;
	_mBuf->u.static_data.seekTime = 0x7FFFFFFFFFFFFFFFLL;
	_mBuf->u.static_data.needToSeek = 0;
	_mBuf->u.static_data.pendingSeek = 0;
	_mBuf->u.static_data.delta = -system_time();
	for (int32 i=0;i<N_MAX_CLIENT_COUNT;i++) {
		_mBuf->u.static_data.clients[i].port = B_BAD_VALUE;
		_mBuf->u.static_data.clients[i].state = 0;
	};
	FPRINTF(stderr, "_SysTimeSource::_SysTimeSource(%ld)\n", ID());
}

port_id
_SysTimeSource::ControlPort() const
{
	return B_BAD_VALUE;
}

BMediaAddOn *
_SysTimeSource::AddOn(int32 * internal_id) const
{
	if (internal_id) *internal_id = 0;
	return NULL;
}

_SysTimeSource::~_SysTimeSource()
{
	FPRINTF(stderr, "_SysTimeSource::~_SysTimeSource(%ld)\n", ID());
}

status_t 
_SysTimeSource::TimeSourceOp(const time_source_op_info &/*op*/, void */*_reserved*/)
{
	ASSERT("_SysTimeSource::TimeSourceOp should never be called!\n");
	return B_ERROR;
}


#if 0
BMediaAddOn *
_SysTimeSource::AddOn(
	int32 * internal_id) const
{
	if (internal_id)
		*internal_id = 0;
	return NULL;
}

_SysTimeSource::~_SysTimeSource()
{
	write_port(mPort, 0x60000000, NULL, 0);
	status_t status;
	wait_for_thread(mThread, &status);
	delete_port(mPort);
}

status_t
_SysTimeSource::ThreadEntry(
	void * data)
{
	dlog("Entering thread %d (node %d)", find_thread(NULL), ((_SysTimeSource *)data)->ID());
	((_SysTimeSource *)data)->Run();
	dlog("_SysTimeSource died");
	return 0;
}

#define LATENCY 3000

void
_SysTimeSource::Run()
{
	while (true)
	{
		int32 code;
		status_t err;
		bigtime_t real = RealTime();
		bigtime_t timeout = (mRunning || mStarting) ? 200000LL : 1000000000000000LL;	/* very long timeout */
		/* START */
		if (!mRunning && mStarting)
		{
			if (real >= mStartTime-timeout)
			{
				timeout = mStartTime - real;
				if (timeout < LATENCY)
				{
					dlog("START detected");
					mDelta = mMediaTime - mStartTime;	/* delta is typically negative */
					mStarting = false;
					mRunning = true;
					PublishTime(mMediaTime, mStartTime, 1.0);
					/* notify people we've started */
					BTimeSource::BroadcastTimeWarp(mStartTime, mMediaTime);
				}
				else
					dlog("START in %Ld (real=%Ld mStartTime=%Ld)", timeout, real, mStartTime);
			}
		}
		/* STOP */
		else if (mRunning && mStopping)
		{
			if (real >= mStopTime-timeout)
			{
				timeout = mStopTime-real;
				if (timeout <= 0)
				{
					dlog("STOP detected");
					mRunning = false;
					mStopping = false;
					PublishTime(0, 0, 0.0);	/* lets people know we're stopped */
					timeout = 0;
					/* there may be a queued seek that we shouldn't lose */
					if (!mSeeking)
					{
						mMediaTime = mStopTime+mDelta;
					}
					mSeeking = false;
				}
				else
					dlog("STOP in %Ld (real=%Ld mStopTime=%Ld)", timeout, real, mStopTime);
			}
		}
		/* SEEK */
		else if (mRunning && mSeeking)
		{
			if (real >= mSeekTime-timeout)
			{
				timeout = mSeekTime-real;
				if (timeout <= 0)
				{
					dlog("SEEK detected");
					timeout = 0;
					mDelta = mMediaTime-mSeekTime;
					mSeeking = false;
					BroadcastTimeWarp(mSeekTime, mMediaTime);
				}
				else
					dlog("SEEK in %Ld (real=%Ld mSeekTime=%Ld", timeout, real, mSeekTime);
			}
		}
		media_message msg;
		if (timeout < 0) timeout = 1;
//		dlog("mRunning=%s  timeout=%Lx", mRunning?"true":"false", timeout);
		err = read_port_etc(mPort, &code, &msg, sizeof(msg), B_TIMEOUT, timeout);
		bigtime_t r = RealTime();
		PublishTime(r + mDelta, r, mRunning ? 1.0 : 0.0);	/* delta is typically negative */
		if (err == B_TIMED_OUT || err == EWOULDBLOCK)
		{
			continue;
		}
		else if (err == B_INTERRUPTED)
		{
			continue;
		}
		else if (err < B_OK)
		{
			dlog("Unknown error %x in _SysTimeSource (%s)", err, strerror(err));
			break;
		}
		if (code == 0x60000000)
		{
			break; /* done! */
		}
		else if (BMediaNode::HandleMessage(code, &msg, sizeof(msg)) &&
				BTimeSource::HandleMessage(code, &msg, sizeof(msg)))
		{
			BMediaNode::HandleBadMessage(code, &msg, sizeof(msg));
		}
		dlog("reached end of loop (0x%x)", code);
	}
}

/* Since these are called from HandleMessage(), we're always synced and there is no race. */
void
_SysTimeSource::Start(
	bigtime_t real_time)
{
	mStarting = true;
	mStartTime = real_time;
}

void
_SysTimeSource::Stop(
	bigtime_t real_time,
	bool immediate)
{
	mStopping = true;
	mStopTime = immediate ? real_time : system_time();
}

void
_SysTimeSource::Seek(
	bigtime_t media_time,
	bigtime_t real_time)
{
	if (mRunning) {
		mSeeking = true;
		mSeekTime = real_time;
	}
	mMediaTime = media_time;
}

#endif

		/* Mmmh, stuffing! */
status_t
BTimeSource::_Reserved_TimeSource_0(void *arg)
{
	if (!arg)
		return B_BAD_VALUE;
		
	time_source_op_info info = (const time_source_op_info &) arg;
	
	switch(info.op)
	{
		case B_TIMESOURCE_START:
			Start(info.real_time);
			return B_OK;
			
		case B_TIMESOURCE_STOP:
			Stop(info.real_time, false);
			return B_OK;
		
		case B_TIMESOURCE_STOP_IMMEDIATELY:
			Stop(info.real_time, true);
			return B_OK;
			
		case B_TIMESOURCE_SEEK:
			Seek(info.performance_time, info.real_time);
			return B_OK;
	}

	return B_OK;
}

status_t
BTimeSource::_Reserved_TimeSource_1(void *)
{
	return B_ERROR;
}

status_t
BTimeSource::_Reserved_TimeSource_2(void *)
{
	return B_ERROR;
}

status_t
BTimeSource::_Reserved_TimeSource_3(void *)
{
	return B_ERROR;
}

status_t
BTimeSource::_Reserved_TimeSource_4(void *)
{
	return B_ERROR;
}

status_t
BTimeSource::_Reserved_TimeSource_5(void *)
{
	return B_ERROR;
}
