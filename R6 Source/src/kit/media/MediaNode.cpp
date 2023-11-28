
#include "trinity_p.h"
#include "timesource_p.h"
#include "BufferProducer.h"
#include "BufferConsumer.h"
#include "TimeSource.h"
#include "FileInterface.h"
#include "Controllable.h"
#include "tr_debug.h"

#include <string.h>
#include <OS.h>
#include <typeinfo>
#include <alloca.h>



#if DEBUG
#define FPRINTF(x) fprintf x
#define DEBUGARG(ident) ident
#else
#define FPRINTF(x) (void)0
#define DEBUGARG(ident)
#endif
#define DIAGNOSTIC(x) fprintf x


#define MEDIA_CONTROL_PORT_SIZE 64

int32 BMediaNode::_m_changeTag = 0;


BMediaNode::BMediaNode(
	const char * name)
{
	if (BMediaRoster::CurrentRoster() == NULL)
	{
		DIAGNOSTIC((stderr, "It is an error to create a BMediaNode when there is no BMediaRoster::Roster()\n"));
	}
	strncpy(_mName, name, sizeof(_mName));
	_mName[sizeof(_mName)-1] = 0;
	_mTimeSource = NULL;
	_mRefCount = 1;
	_mNodeID = 0;
	_mRunMode = B_DROP_DATA;
	_mChangeCount = 1;
	_mChangeCountReserved = 1;
	_mKinds = 0;
	_mTimeSourceID = 0;
	_mUnregisterWhenDone = true;
	_m_controlPort = -1;
	_m_producerThis = NULL;
	_m_consumerThis = NULL;
	_m_fileInterfaceThis = NULL;
	_m_controllableThis = NULL;
	_m_timeSourceThis = NULL;
}


status_t
BMediaNode::ReportError(
	node_error what,
	const BMessage * info)
{
	BMessage * msg;
	if (info) {
		msg = new BMessage(*info);
	}
	else {
		msg = new BMessage(what);
	}
	msg->AddInt32("be:node_id", ID());
	_BMediaRosterP * rosterP = (_BMediaRosterP *)BMediaRoster::CurrentRoster();
	
	status_t ret = B_OK;
	if (rosterP)
		ret = rosterP->Broadcast(this, *msg, what);
	else {
		ret = B_MEDIA_SYSTEM_FAILURE;
		dlog("ReportError() cannot find a current roster");
	}

	delete msg;
	dlog("Broadcast() returns %x in ReportError()\n", ret);
	return ret;
}


status_t
BMediaNode::NodeStopped(
	bigtime_t whenPerformance)
{
	//	if we're a producer, send data status
	if (_m_producerThis != NULL)
	{
		int32 cookie = 0;
		media_output output;
		while (B_OK <= _m_producerThis->GetNextOutput(&cookie, &output))
			if (output.destination.port > 0)
				_m_producerThis->SendDataStatus(B_PRODUCER_STOPPED, output.destination, whenPerformance);
		_m_producerThis->DisposeOutputCookie(cookie);
	}
	//	tell anybody listening about us stopping
	BMessage msg;
	media_node node = Node();
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	msg.AddInt64("when", whenPerformance);
	msg.AddInt32("be:timesource", _mTimeSourceID);
	
	_BMediaRosterP * rosterP = (_BMediaRosterP *)BMediaRoster::CurrentRoster();
	if (rosterP)
		return rosterP->Broadcast(this, msg, B_MEDIA_NODE_STOPPED);
	else {
		dlog("Cannot find a current roster in NodeStopped()");
		return B_MEDIA_SYSTEM_FAILURE;
	}
}


BMediaNode *
BMediaNode::Acquire()	/* return itself */
{
	atomic_add(&_mRefCount, 1);
FPRINTF((stderr, "BMediaNode::Acquire() (%ld/%s) count %ld\n", _mNodeID, _mName, _mRefCount));
	return this;
}

BMediaNode *
BMediaNode::Release()	/* release will decrement refcount, and delete if 0 */
{
FPRINTF((stderr, "%s::Release() (%ld/%s) count %ld\n", Name(), _mNodeID, _mName, _mRefCount));
#if DEBUG
if ((_mRefCount < 0) || (_mRefCount > 100000)) debugger("Bad Release() happened\n");
#endif
	int32 id = _mNodeID;
	int32 r = atomic_add(&_mRefCount, -1);
	if (r == 1) {
		if (DeleteHook(this) != B_OK) {
			DIAGNOSTIC((stderr, "%ld::DeleteHook() returned error!\n", id));
		}
	}
	return (r > 1) ? this : 0;
}


const char *
BMediaNode::Name() const
{
	return _mName;
}


media_node_id
BMediaNode::ID() const
{
	return _mNodeID;
}


uint64
BMediaNode::Kinds() const
{
	return _mKinds;
}


void
BMediaNode::AddNodeKind(
	uint64 kind)
{
	_mKinds |= kind;
}


media_node
BMediaNode::Node() const
{
	media_node mn;
	mn.node = _mNodeID;
	mn.port = ControlPort();
	mn.kind = _mKinds;
	if (_mNodeID <= 0)
	{
		DIAGNOSTIC((stderr, "Warning: calling BMediaNode::Node() on a Node which is not registered is a bad idea!\n"));
	}
	return mn;
}

status_t 
BMediaNode::WaitForMessage(bigtime_t waitUntil, uint32 /*flags*/, void */*_reserved_*/)
{
	status_t err = B_TIMED_OUT;
	int32 code = 0;
	char msg[B_MEDIA_MESSAGE_SIZE];		

	if (waitUntil < 0) return err;
//	if ((waitUntil > 0) && (waitUntil < BTimeSource::RealTime() + 100LL))
//		return err;
	err = read_port_etc(ControlPort(), &code, msg, B_MEDIA_MESSAGE_SIZE, B_ABSOLUTE_TIMEOUT, waitUntil);

	if (err < B_OK)
		return err;
	size_t msgsize=(size_t)err;
	err = B_OK;
		
	//	call base interface
	if (BMediaNode::HandleMessage(code, msg, msgsize) == B_OK)
		goto ok;

	//	call possible subclass interfaces
	if((_m_producerThis != NULL) &&
			(_m_producerThis->BBufferProducer::HandleMessage(code, msg, msgsize) == B_OK))
		goto ok;
		
	if((_m_consumerThis != NULL) &&
			(_m_consumerThis->BBufferConsumer::HandleMessage(code, msg, msgsize) == B_OK))
		goto ok;
		
	if((_m_fileInterfaceThis != NULL) &&
			(_m_fileInterfaceThis->BFileInterface::HandleMessage(code, msg, msgsize) == B_OK))
		goto ok;
		
	if ((_m_controllableThis != NULL) &&
			(_m_controllableThis->BControllable::HandleMessage(code, msg, msgsize) == B_OK))
		goto ok;
		
	if ((_m_timeSourceThis != NULL) &&
			(_m_timeSourceThis->BTimeSource::HandleMessage(code, msg, msgsize) == B_OK))
		goto ok;

	//	call most-derived interface
	if (HandleMessage(code, msg, msgsize) == B_OK)
		goto ok;

	HandleBadMessage(code, msg, msgsize);

ok:
	return err;
}


void
BMediaNode::Start(
	bigtime_t /*when*/)
{
}


void
BMediaNode::_inspect_classes()
{
	_m_producerThis = dynamic_cast<BBufferProducer *>(this);
	_m_consumerThis = dynamic_cast<BBufferConsumer *>(this);
	_m_fileInterfaceThis = dynamic_cast<BFileInterface *>(this);
	_m_timeSourceThis = dynamic_cast<BTimeSource *>(this);
	_m_controllableThis = dynamic_cast<BControllable *>(this);
	assert((_m_producerThis == NULL) == ((_mKinds & B_BUFFER_PRODUCER) == 0));
	assert((_m_consumerThis == NULL) == ((_mKinds & B_BUFFER_CONSUMER) == 0));
	assert((_m_fileInterfaceThis == NULL) == ((_mKinds & B_FILE_INTERFACE) == 0));
	assert((_m_timeSourceThis == NULL) == ((_mKinds & B_TIME_SOURCE) == 0));
	assert((_m_controllableThis == NULL) == ((_mKinds & B_CONTROLLABLE) == 0));
}


void
BMediaNode::PStart(
	bigtime_t DEBUGARG(when))
{
	dlog("BMediaNode::Start(%d, %Lx)", ID(), when);
}


void
BMediaNode::Preroll()
{
}


void
BMediaNode::PPreroll()
{
	dlog("BMediaNode::Preroll(%d)", ID());
}


void
BMediaNode::Stop(
	bigtime_t /*at_performance_time*/,
	bool /*immediate*/)
{
}


void
BMediaNode::PStop(
	bigtime_t DEBUGARG(at_performance_time),
	bool DEBUGARG(immediate))
{
	dlog("BMediaNode::Stop(%d, %Lx, %s)", ID(), at_performance_time, 
		immediate ? "immediate" : "queued");
}


void
BMediaNode::Seek(
	bigtime_t /*media_time*/,
	bigtime_t /*performance_time*/)	/* performance_time only used when running */
{
}


void
BMediaNode::PSeek(
	bigtime_t DEBUGARG(media_time),
	bigtime_t DEBUGARG(performance_time))	/* performance_time only used when running */
{
	dlog("BMediaNode::Seek(%d, %Lx, %Lx)", ID(), media_time, performance_time);
}


void
BMediaNode::SetTimeSource(
	BTimeSource * /*time_source*/)
{
}


void
BMediaNode::PSetTimeSource(
	BTimeSource * time_source)
{
	dlog("BMediaNode::SetTimeSourceFor(%d)", time_source ? time_source->ID() : 0);
	if (time_source == _mTimeSource) {
		FPRINTF((stderr, "Time source %p already set\n", time_source));
		return;
	}
	else if (time_source && _mTimeSource && (time_source->ID() == _mTimeSource->ID())) {
//		char * buf = (char*)alloca(500);
//		sprintf(buf, "Two instances of same time source (%ld/%s) [%x] [%x]\n", 
//				time_source->ID(), time_source->Name(), time_source, _mTimeSource);
//		DIAGNOSTIC((stderr, "%s", buf));
		BTimeSource * ts = dynamic_cast<BTimeSource *>(time_source->Acquire());
		_mTimeSource->Release();
		_mTimeSource = ts;
	}
	else {
		if (_mTimeSource) {
			_mTimeSource->RemoveMe(this);
			_mTimeSource->Release();
		}
		if (time_source) {
			_mTimeSource = dynamic_cast<BTimeSource *>(time_source->Acquire());
			if (_mTimeSource) {
				ASSERT(this != 0);
				_mTimeSource->AddMe(this);
			}
			else {
				DIAGNOSTIC((stderr, "BAD MOJO: _mTimeSource cast to NULL in BMediaNode::PSetTimeSource()\n"));
			}
		}
		else {
			_mTimeSource = NULL;
		}
	}
	dlog("mTimeSource = %x", _mTimeSource);
}


void
BMediaNode::SetRunMode(
	run_mode /*mode*/)
{
}


void
BMediaNode::PSetRunMode(
	run_mode mode,
	bigtime_t recordDelay)
{
	dlog("BMediaNode::SetRunMode(%d)", mode);
	_mRunMode = mode;
	if (_mKinds & B_BUFFER_PRODUCER)
	{
		BBufferProducer * bp = dynamic_cast<BBufferProducer *>(this);
		if (bp != NULL)
			bp->PSetRecordDelay(recordDelay);
	}
}


void
BMediaNode::TimeWarp(
	bigtime_t /*at_real_time*/,
	bigtime_t /*to_performance_time*/)
{
}


void
BMediaNode::PTimeWarp(
	bigtime_t DEBUGARG(at_real_time),
	bigtime_t DEBUGARG(to_performance_time))
{
	dlog("%s: BMediaNode::TimeWarp(%Ld, %Ld)", typeid(*this).name(), at_real_time, to_performance_time);
}


BMediaNode::run_mode
BMediaNode::RunMode() const
{
	return _mRunMode;
}


BMediaAddOn *
BMediaNode::AddOn(
	int32 * internal_id) const
{
	if (internal_id)
		*internal_id = 0;
	return 0;
}


BTimeSource *
BMediaNode::TimeSource() const
{

	/*	Whoa!  Doing this, which simply sets the default audio time source if you
		call TimeSource() without having set one first, causes all kinds of weird behavior.
		The mixer gets deleted, no sound is ever played, a deadlock occurs.  Hmmm. */
/*
	if (!_mTimeSource) {
		media_node ts;
		if (((_BMediaRosterP *)BMediaRoster::Roster())->GetTimeSource(&ts) == B_OK) {
			BMediaNode *me = const_cast<BMediaNode*>(this);
			me->_mTimeSource = new _BTimeSourceP(ts.node);
			if (me->ID() == ts.node) {
				me->_mTimeSource->ReleaseMaster();
			}
			me->_mTimeSource->AddMe(me);
		} else {
			return ((_BMediaRosterP *)BMediaRoster::Roster())->ReturnNULLTimeSource();
		};
	}
	return _mTimeSource;
*/

	if (!_mTimeSource) {
//		if (_m_timeSourceThis) return _m_timeSourceThis;
		return ((_BMediaRosterP *)BMediaRoster::Roster())->ReturnNULLTimeSource();
	}
	return _mTimeSource;
}


BMediaNode::BMediaNode(		/* constructor sets refcount to 1 */
	const char * name,		/* this constructor is special for time source clones */
	media_node_id id,
	uint32 kinds)
{
	if (BMediaRoster::CurrentRoster() == NULL) {
		DIAGNOSTIC((stderr, "It is an error to create a BMediaNode when there is no BMediaRoster::Roster()\n"));
	}
	strncpy(_mName, name, sizeof(_mName));
	_mName[sizeof(_mName)-1] = 0;
	_mTimeSource = NULL;
	_mRefCount = 1;
	_mNodeID = id;
	_mRunMode = B_OFFLINE;
	_mChangeCount = 1;
	_mChangeCountReserved = 1;
	_mKinds = kinds;
	_mTimeSourceID = id;	/* this is special for time source clones */
	_mUnregisterWhenDone = false;
	_m_controlPort = -1;
	_m_producerThis = NULL;
	_m_consumerThis = NULL;
	_m_fileInterfaceThis = NULL;
	_m_controllableThis = NULL;
	_m_timeSourceThis = NULL;
}


BMediaNode::~BMediaNode()	/* should be called through Release() */
{
	if (_mRefCount != 0) {
		DIAGNOSTIC((stderr, "Node %ld ref count is %ld\n", _mNodeID, _mRefCount));
//#if !NDEBUG
		if (getenv("MALLOC_DEBUG") != 0) {
			debugger("BAD NODE REF COUNT IN DESTRUCTOR");
		}
//#endif
	}
	dassert(_mRefCount == 0);
	if (_mTimeSource != NULL) _mTimeSource->Release();
	if (_mUnregisterWhenDone) {
		((_BMediaRosterP *)BMediaRoster::Roster())->UnregisterNode(this);
	}
	delete_port(_m_controlPort);
}

status_t 
BMediaNode::AddTimer(bigtime_t /*at_performance_time*/, int32 /*cookie*/)
{
	return B_UNSUPPORTED;
}

void
BMediaNode::TimerExpired(bigtime_t atTime, int32 cookie, status_t error)
{
	sync_a ans;
	ans.error = error;
	ans.cookie = cookie;
	ans.time = atTime;
	ans.node = ID();
	write_port(cookie, M_SYNC_REPLY, &ans, sizeof(ans));
}

status_t
BMediaNode::HandleMessage(
	int32 message,
	const void * data,
	size_t size)
{
	status_t err = B_OK;
	switch (message) {
	case M_START: {
		PStart(((start_q *)data)->performance_time);
		Start(((start_q *)data)->performance_time);
		} break;
	case M_SYNC: {
		sync_q & req(*(sync_q *)data);
		status_t err = AddTimer(req.performance_time, req.reply);
		if (err < B_OK) {
			TimerExpired(req.performance_time, req.reply, err);
		}
		} break;
	case M_STOP: {


		bool b = (((stop_q *)data)->flags & stop_q::STOP_SYNC) ? true : false;
		PStop(((stop_q *)data)->performance_time, b);
		Stop(((stop_q *)data)->performance_time, b);
		if (b) {	/* send sync reply */
			stop_a ans;
			ans.error = B_OK;
			ans.cookie = ((stop_q *)data)->cookie;
			write_port(((stop_q *)data)->reply, M_STOP_REPLY, &ans, sizeof(ans));
		}
		} break;
	case M_SEEK: {
		PSeek(((seek_q *)data)->media_time, ((seek_q *)data)->performance_time);
		Seek(((seek_q *)data)->media_time, ((seek_q *)data)->performance_time);
		} break;
	case M_SET_TIMESOURCE: {
		_mTimeSourceID = ((set_time_source_q *)data)->time_source;
		_BTimeSourceP * source = new _BTimeSourceP(_mTimeSourceID);
		PSetTimeSource(source);
		SetTimeSource(source);
		source->Release(); /* Created with one + SetTimeSource acquires */
		if (_mTimeSourceID == ID()) {
			source->ReleaseMaster();
		}
		} break;
	case M_SET_RUN_MODE: {
		PSetRunMode(((set_run_mode_q *)data)->mode, ((set_run_mode_q *)data)->delay);
		SetRunMode(((set_run_mode_q *)data)->mode);
		} break;
	case M_TIMEWARP: {
		PTimeWarp(((timewarp_q *)data)->real_time, ((timewarp_q *)data)->performance_time);
		TimeWarp(((timewarp_q *)data)->real_time, ((timewarp_q *)data)->performance_time);
		} break;
	case M_PREROLL: {
		PPreroll();
		Preroll();
		preroll_a ans;
		ans.cookie = ((preroll_q *)data)->cookie;
		write_port(((preroll_q *)data)->reply, M_PREROLL_REPLY, &ans, sizeof(ans));
		} break;
	case M_REQUEST_COMPLETED: {
		const media_request_info *mri = reinterpret_cast<const media_request_info *>(data);
		mri->format.MetaData();
	//	printf("M_REQUEST_COMPLETED: %d/%d %08x\n",int(size),int(sizeof(media_request_info)+4),data);
		//assert((size == sizeof(media_request_info)+4));
		assert((mri->what != media_request_info::B_REQUEST_FORMAT_CHANGE));
		if (size == sizeof(media_request_info)+4)
			release_sem(*((sem_id*)(((uint8*)data)+sizeof(media_request_info))));
		err = RequestCompleted(*mri);
		} break;
	case M_GET_TIMESOURCE: {
		get_timesource_a ans;
		ans.cookie = ((get_timesource_q *)data)->cookie;
		if (!_mTimeSourceID) {
			_mTimeSourceID = TimeSource()->ID();
		}
		ans.time_source = _mTimeSourceID;
		write_port(((get_timesource_q *)data)->reply, M_GET_TIMESOURCE_REPLY, &ans, sizeof(ans));
		} break;
	/* M_BUFFER is really BC_BUFFER */
	case M_ROLL: {
		roll_q & r(*(roll_q *)data);
		if (r.media > -B_INFINITE_TIMEOUT)
		{
			PSeek(r.media, r.start);
			Seek(r.media, r.start);
		}
		PStart(r.start);
		Start(r.start);
		PStop(r.stop, false);
		Stop(r.stop, false);
		} break;
	case M_GET_ATTRIBUTES: {
		get_attributes_q & q(*(get_attributes_q *)data);
		get_attributes_a & ans(*(get_attributes_a *)alloca(B_MEDIA_MESSAGE_SIZE));
		ans.error = GetNodeAttributes(&((media_node_attribute *)&ans)[1], MAX_NODE_ATTRIBUTE_COUNT);
		ans.cookie = q.cookie;
		size_t s = sizeof(get_attributes_a);
		if (ans.error > 0) s += sizeof(media_node_attribute)*ans.error;
		write_port_etc(q.reply, M_GET_ATTRIBUTES_REPLY, &ans, s, B_TIMEOUT, DEFAULT_TIMEOUT);
		} break;
	case M_GET_MESSENGER: {
			get_messenger_q & q(*(get_messenger_q *)data);
			get_messenger_a ans;
			ans.cookie = q.cookie;
			if (BMediaRoster::CurrentRoster() == NULL) {
				ans.error = B_BAD_HANDLER;
			}
			else {
				ans.messenger = BMessenger(BMediaRoster::CurrentRoster());
				ans.error = ans.messenger.IsValid() ? B_OK : B_BAD_HANDLER;
//				FPRINTF(stderr, "Roster messenger is %s\n", strerror(ans.error));
//				for (int ix=0; ix<5; ix++) {
//					FPRINTF(stderr, "0x%04x\n", ((int32 *)&ans.messenger)[ix]);
//				}
			}
			(void)write_port_etc(q.reply, M_GET_MESSENGER_REPLY, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
		}
		break;
	case M_RECOVER_NODE: {
			recover_node_q & q(*(recover_node_q *)data);
			recover_node_a ans;
			ans.cookie = q.cookie;
			ans.id = ID();
			ans.error = (ans.id > 0) ? B_OK : B_MEDIA_BAD_NODE;
			(void)write_port_etc(q.reply, M_RECOVER_NODE_REPLY, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
		}
		break;
	default:
		return B_ERROR;
	}
	return err;
}


int32
BMediaNode::IncrementChangeTag()
{
	int32 count = MintChangeTag();
	ApplyChangeTag(count);
	return count;
}


int32
BMediaNode::ChangeTag()
{
	return _mChangeCount;
}


int32
BMediaNode::MintChangeTag()
{
	return atomic_add(&_mChangeCountReserved, 1)+1;
}

status_t
BMediaNode::ApplyChangeTag(
	int32 count)
{
	if (_mChangeCount <= count) {
		return B_MEDIA_STALE_CHANGE_COUNT;
	}
	_mChangeCount = count;
	return B_OK;
}


void
BMediaNode::HandleBadMessage(
	int32 code,
	const void * /*data*/,
	size_t DEBUGARG(size))
{
	/*	do nothing for now */
	/*	note that it is not an error for M_NODE_DIED to arrive here... BufferProducer
		and BufferConsumer both return B_ERROR for handling it, so it will get passed on
		to the other */
	if (code != M_NODE_DIED) {
		dlog("%s: BadMsg nod:%s:%d por:%d cod:%x siz:%d", 
			typeid(*this).name(), Name(), ID(), ControlPort(), code, size);
	};
}


port_id
BMediaNode::ControlPort() const
{
//	FPRINTF(stderr, "BMediaNode::ControlPort() should not be called!\n");
//	DEBUGGER("This function should never have been called!");
//	return -1;
	if (_m_controlPort == -1)
	{
		char name[32];
		sprintf(name, "%.20s CPort", Name());
		_m_controlPort = create_port(MEDIA_CONTROL_PORT_SIZE, name);
	}
	return _m_controlPort;
}


status_t
BMediaNode::DeleteHook(
	BMediaNode * /*node*/)
{
	// try to clean up if user didn't disconnect first
	if (_m_consumerThis != 0) {
		int32 cookie = 0;
		media_input in;
		cookie = 0;
		while (_m_consumerThis->GetNextInput(&cookie, &in) == B_OK) {
			if (in.source != media_source::null) {
				break_q cmd;
				cmd.from = in.source;
				cmd.where = in.destination;
				cmd.reply = -1;
				cmd.cookie = 0;
				(void)write_port_etc(in.source.port, BP_BREAK, &cmd, sizeof(cmd),
					B_TIMEOUT, 1000000LL);
			}
		}
	}
	if (_m_producerThis != 0) {
		int32 cookie = 0;
		media_output out;
		cookie = 0;
		while (_m_producerThis->GetNextOutput(&cookie, &out) == B_OK) {
			if (out.destination != media_destination::null) {
				broken_q cmd;
				cmd.producer = out.source;
				cmd.where = out.destination;
				(void)write_port_etc(out.destination.port, BC_BROKEN, &cmd, sizeof(cmd),
					B_TIMEOUT, 1000000LL);
			}
		}
	}
	delete this;
	return B_OK;
}


	static bool
	is_heap_address(
		void * ptr)
	{
		bool heap = ((((ulong)ptr)&0xf0000000UL)==0x80000000UL);
#if DEBUG
		if (!heap) {
			FPRINTF((stderr, "guarded_rtm_free(): 0x%lx is not a heap address\n", (ulong)ptr));
		}
#endif
		return heap;
	}

static void
guarded_rtm_free(
	void * ptr)
{
	rtm_pool * pool = rtm_get_pool(NULL, ptr);
	if (pool == NULL) {
		if (is_heap_address(ptr)) {
			::operator delete(ptr);
		}
	}
	else {
		rtm_free(ptr);
	}
}

void *
BMediaNode::operator new(
	size_t size)
{
	void * r = rtm_alloc(NULL, size);
	if (!r) throw std::bad_alloc();
	return r;
}


void *
BMediaNode::operator new(
	size_t size,
	const std::nothrow_t & /*t*/) throw()
{
	try {
		return rtm_alloc(NULL, size);
	}
	catch (...) {
		return NULL;
	}
}


void
BMediaNode::operator delete(
	void * ptr)
{
	return guarded_rtm_free(ptr);
}


#if !__MWERKS__
void
BMediaNode::operator delete(
	void * ptr, 
	const std::nothrow_t & /*t*/) throw()
{
	try {
		guarded_rtm_free(ptr);
	}
	catch (...) {
		//	nothing
	}
}
#endif


status_t
BMediaNode::RequestCompleted(
	const media_request_info & /*info*/)
{
	return B_BAD_VALUE;
}


int32
BMediaNode::NewChangeTag()	//	for use by BBufferConsumer, mostly
{
	return 1+atomic_add(&_m_changeTag, 1);
}


void
BMediaNode::NodeRegistered()
{
	//	do nothing
}


status_t
BMediaNode::GetNodeAttributes(
	media_node_attribute * /*outAttributes*/,
	size_t /*inMaxCount*/)
{
	return 0;
}



status_t
BMediaNode::_Reserved_MediaNode_0(void * arg)
{
	BMediaNode::RequestCompleted(*reinterpret_cast<media_request_info *>(arg));
	return B_OK;
}

status_t
BMediaNode::_Reserved_MediaNode_1(void * arg)
{
	return BMediaNode::DeleteHook(reinterpret_cast<BMediaNode *>(arg));
}

status_t
BMediaNode::_Reserved_MediaNode_2(void *)
{
	BMediaNode::NodeRegistered();
	return B_OK;
}

status_t
BMediaNode::_Reserved_MediaNode_3(void *arg)
{
//	return BMediaNode::GetNodeAttributes((media_node_attribute *)arg, MAX_NODE_ATTRIBUTE_COUNT);
	static media_node_attribute s_old_attr = { media_node_attribute::B_R40_COMPILED, 0, 0 };
	*(media_node_attribute *)arg = s_old_attr;
	return 1;
}

status_t
BMediaNode::_Reserved_MediaNode_4(void *)	//	AddTimer
{
	return B_UNSUPPORTED;
}

status_t
BMediaNode::_Reserved_MediaNode_5(void *)
{
	return B_ERROR;
}

status_t
BMediaNode::_Reserved_MediaNode_6(void *)
{
	return B_ERROR;
}

status_t
BMediaNode::_Reserved_MediaNode_7(void *)
{
	return B_ERROR;
}

status_t
BMediaNode::_Reserved_MediaNode_8(void *)
{
	return B_ERROR;
}

status_t
BMediaNode::_Reserved_MediaNode_9(void *)
{
	return B_ERROR;
}

status_t
BMediaNode::_Reserved_MediaNode_10(void *)
{
	return B_ERROR;
}

status_t
BMediaNode::_Reserved_MediaNode_11(void *)
{
	return B_ERROR;
}

status_t
BMediaNode::_Reserved_MediaNode_12(void *)
{
	return B_ERROR;
}

status_t
BMediaNode::_Reserved_MediaNode_13(void *)
{
	return B_ERROR;
}

status_t
BMediaNode::_Reserved_MediaNode_14(void *)
{
	return B_ERROR;
}

status_t
BMediaNode::_Reserved_MediaNode_15(void *)
{
	return B_ERROR;
}

