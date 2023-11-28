// LocalControllable.cpp

// +++++ toDO
// - test ApplyParameterData/MakeParameterData

#include "LocalControllable.h"

#include <Locker.h>
#include <Messenger.h>
#include <MediaDefs.h>
#include <OS.h>

#include "Controllable.h"
#include "trinity_p.h"
#include "ParameterWeb.h"
#include "tr_debug.h"
#include "MediaAddOn.h"

#include <map>

#ifdef NDEBUG
#define DEBUGVAR(var, expression) expression
#else
#define DEBUGVAR(var, expression) var=expression
#endif

// ================================================================================
// the global instance set
// ================================================================================

namespace BPrivate {

class LocalControllableRoster
{
public:
	LocalControllableRoster() :
		_lockInit(0),
		_lock(0),
		_nextID(1)
	{
	}
	bool Lock()
	{
		int32 v = atomic_or(&_lockInit, 1);
		if(!v)
		{
			_lock = new BLocker("LocalControllableRoster");
			atomic_or(&_lockInit, 2);
		}
		else while(!(v & 2))
		{
			snooze(10000);
			v = atomic_or(&_lockInit, 0);
		}
		return _lock->Lock();
	}
	void Unlock()
	{
		DEBUGVAR(int32 v, atomic_or(&_lockInit, 0));
		ASSERT(v & 2);
		_lock->Unlock();
	}
	bool IsLocked() const
	{
		int32 v = atomic_or(&_lockInit, 0);
		if(!(v & 2))
			return false;
		return _lock->IsLocked();
	}

	status_t Add(BLocalControllable* instance)
	{
		ASSERT(IsLocked());
		if(!_map)
			_map = new map_t;
		if(Find(instance->ID()))
			return B_NOT_ALLOWED;
		if(!instance)
			return B_BAD_VALUE;
		_map->insert(map_t::value_type(instance->ID(), instance));
		return B_OK;
	}
	status_t Remove(BLocalControllable* instance)
	{
		ASSERT(IsLocked());
		if(!_map)
			return B_BAD_INDEX;
		return _map->erase(instance->ID()) ? B_OK : B_BAD_INDEX;
	}
	BLocalControllable* Find(int32 id) const
	{
		ASSERT(IsLocked());
		if(!_map)
			return 0;
		map_t::const_iterator it = _map->find(id);
		return (it == _map->end()) ? 0 : (*it).second;
	}
	
	int32 NextID()
	{
		ASSERT(IsLocked());
		return _nextID++;
	}

private:
	typedef std::map<int32, BLocalControllable*> map_t;
	map_t* _map;
	
	mutable int32    _lockInit;
	mutable BLocker* _lock;
	
	int32    _nextID;
};

}; // BPrivate
using namespace BPrivate;

// ================================================================================
// observer map for a given BLocalControllable*
// ================================================================================

class BLocalControllable::ObserverSet
{
public:
	status_t Add(
		const BMessenger& messenger,
		int32 notificationType)
	{
		observer_map_t::iterator itBegin = _map.lower_bound(notificationType);
		observer_map_t::const_iterator itEnd = _map.upper_bound(notificationType);
		while(itBegin != itEnd)
		{
			if((*itBegin).second == messenger)
			{
				// already mapped
				return B_OK;
			}
			++itBegin;
		}
		
		_map.insert(observer_map_t::value_type(
			notificationType,
			messenger));
		return B_OK;
	}
	
	status_t Remove(
		const BMessenger& messenger,
		int32 notificationType)
	{
		observer_map_t::iterator itBegin = _map.lower_bound(notificationType);
		observer_map_t::const_iterator itEnd = _map.upper_bound(notificationType);
		while(itBegin != itEnd)
		{
			if((*itBegin).second == messenger)
			{
				_map.erase(itBegin);
				return B_OK;
			}
			++itBegin;
		}
		return B_BAD_VALUE;
	}
	
	void Broadcast(
		BMessage* message,
		int32 notificationType)
	{
		ASSERT(message);
		while(true)
		{
			observer_map_t::const_iterator itBegin = _map.lower_bound(notificationType);
			observer_map_t::const_iterator itEnd = _map.upper_bound(notificationType);
			while(itBegin != itEnd)
			{
				(*itBegin).second.SendMessage(message);
				// +++++ handle errors
				++itBegin;
			}
			if(notificationType == B_MEDIA_WILDCARD)
				break;
			notificationType = B_MEDIA_WILDCARD;
		}
	}
private:
	typedef std::multimap<int32, BMessenger> observer_map_t;
	observer_map_t _map;
};


// ================================================================================

static LocalControllableRoster  _sRoster;

// ================================================================================
// BLocalControllable
// ================================================================================

BLocalControllable::~BLocalControllable()
{
	if(mRegistered)
	{
		fprintf(stderr, "***** ~BLocalControllable(): STILL REGISTERED *****\n");
		DEBUGGER("You must call BLocalControllable::SetParameterWeb(NULL).");
	}	
	delete mWeb;
	delete mObservers;
}

status_t 
BLocalControllable::GetParameterWeb(BParameterWeb **out_web)
{
	if(!mRegistered) return B_NOT_ALLOWED;
	if(!mWeb)
		return B_BAD_VALUE;
	
	ssize_t size = mWeb->FlattenedSize();
	int8* buffer = (int8*)malloc(size);
	if(!buffer)
		return B_NO_MEMORY;
	status_t err = mWeb->Flatten(buffer, size);
	if(err < B_OK)
		return err;
	*out_web = new BParameterWeb;
	err = (*out_web)->Unflatten(mWeb->TypeCode(), buffer, size);
	free(buffer);
	return err;
}

int32 
BLocalControllable::ID() const
{
	return mID;
}

BLocalControllable::BLocalControllable() :
	mWeb(0),
	mChangeCount(0),
	mObservers(0),
	mRegistered(false)
{
	thread_id tid = find_thread(0);
	thread_info thr_info;
	status_t err = get_thread_info(tid, &thr_info);
	ASSERT(err == B_OK);
	team_info team_info;
	err = get_team_info(thr_info.team, &team_info);
	ASSERT(err == B_OK);
	const_cast<team_id&>(mTeam) = team_info.team;

	_sRoster.Lock();
	{
		const_cast<int32&>(mID) = _sRoster.NextID();
	}
	_sRoster.Unlock();
}

status_t 
BLocalControllable::SetParameterWeb(BParameterWeb *web)
{
	status_t err = B_OK;
	_sRoster.Lock();
	{
		if(mWeb)
		{
			delete mWeb;
			mWeb = 0;
		}
		
		if(web)
		{
			if(!mRegistered)
			{
				err = _sRoster.Add(this);
				if(err < B_OK)
				{
					fprintf(stderr,
						"BLocalControllable()::SetParameterWeb(): _sRoster.Add():\n\t%s\n",
						strerror(err));
				}
				mRegistered = true;
			}

			++mChangeCount;
			mWeb = web;
			mWeb->mLocalID = mID;
			mWeb->mLocalTeam = mTeam;
			mWeb->mLocalChangeCount = mChangeCount;
	
			BMessage m(B_MEDIA_WEB_CHANGED);
			m.AddInt32("be:localID", mID);
			Broadcast(&m);
			
			if(mObservers)
			{
				// remove all observers; they'll need to resubscribe using a web with the
				// updated change count.
				delete mObservers;
				mObservers = 0;
			}
		}
		else if(mRegistered)
		{
			status_t err = _sRoster.Remove(this);
			if(err < B_OK)
			{
				fprintf(stderr, "BLocalControllable::SetParameterWeb(): _sRoster.Remove():\n\t%s\n",
					strerror(err));
			}
			mRegistered = false;
		}
	}
	_sRoster.Unlock();
	return err;
}

const BParameterWeb *
BLocalControllable::Web() const
{
	const BParameterWeb* ret;
	_sRoster.Lock();
	{
		ret = mWeb;
	}
	_sRoster.Unlock();
	return ret;
}

status_t 
BLocalControllable::BroadcastChangedParameter(int32 id)
{
	if(!mRegistered) return B_NOT_ALLOWED;

	BMessage m(B_MEDIA_PARAMETER_CHANGED);
	m.AddInt32("be:localID", mID);
	m.AddInt32("parameter", id);
	Broadcast(&m);
	return B_OK;
}

status_t 
BLocalControllable::BroadcastNewParameterValue(
	bigtime_t when, int32 id, const void *newValue, size_t valueSize)
{
	if(!mRegistered) return B_NOT_ALLOWED;

	BMessage m(B_MEDIA_NEW_PARAMETER_VALUE);
	m.AddInt32("be:localID", mID);
	m.AddInt32("be:parameter", id);
	m.AddInt64("be:when", when);
	m.AddData("be:value", B_RAW_TYPE, newValue, valueSize);
	Broadcast(&m);
	return B_OK;
}

status_t 
BLocalControllable::StartControlPanel(BMessenger */*out_messenger*/)
{
	return B_ERROR;
}

status_t 
BLocalControllable::ApplyParameterData(const void *data, size_t size)
{
	set_values_q * q = (set_values_q *)data;
	char * ptr = q->raw_data;
	char * end = &q->raw_data[size-(sizeof(*q)-sizeof(q->raw_data))];
	for (int ix=0; ix<q->num_values; ix++)
	{
		struct {
			bigtime_t when;
			int32 id;
			int32 size;
		} hdr;
		if ((size_t)(end-ptr) < sizeof(hdr)) {
			dlog("Corrupt data in CT_SET_VALUES (to many)");
			return B_BAD_VALUE;
		}
		memcpy(&hdr, ptr, sizeof(hdr));
		ptr += sizeof(hdr);
		if (end-ptr < hdr.size) {
			dlog("Corrupt data in CT_SET_VALUES (bad header)");
			return B_BAD_VALUE;
		}
		SetParameterValue(hdr.id, hdr.when, ptr, hdr.size);
		ptr += hdr.size;
	}
	return B_OK;
}

status_t 
BLocalControllable::MakeParameterData(void *buffer, size_t *ioSize)
{
	status_t ret;
	BParameterWeb *web;
	ret=GetParameterWeb(&web);
	if(ret!=B_OK)
		return ret;
	int32 numparams=web->CountParameters();
	int32 *params=new int32[numparams];
	for(int i=0;i<numparams;i++)
	{
		BParameter *param=web->ParameterAt(i);
		params[i]=param->ID();
	}
	ret=MakeParameterData(params,numparams,buffer,ioSize);
	delete params;
	return ret;
}

status_t 
BLocalControllable::MakeParameterData(const int32 *ids, int32 count, void *buffer, size_t *ioSize)
{
	status_t err = B_OK;
	char * ptr = (char *)buffer;
	size_t togo = *ioSize;
	struct {
		media_node node;
		int32 count;
	} hdr1;
	hdr1.node = media_node::null;
	hdr1.count = 0;
	if (sizeof(hdr1) > *ioSize) {
		*ioSize = 0;
		return B_NO_MEMORY;
	}
	ptr += sizeof(hdr1);
	togo -= sizeof(hdr1);
	for (int ix=0; ix<count; ix++)
	{
		struct {
			bigtime_t when;
			int32 control;
			size_t size;
		} hdr;
		hdr.size = togo - sizeof(hdr);
		if (hdr.size < 4) {	/* must be at least SOME space there */
			err = B_NO_MEMORY;
			break;
		}
		status_t e = GetParameterValue(ids[ix], &hdr.when, ptr+sizeof(hdr), &hdr.size);
		if (e == B_OK)
		{
			memcpy(ptr, &hdr, sizeof(hdr));
			ptr += sizeof(hdr)+hdr.size;
			togo -= sizeof(hdr)+hdr.size;
			hdr1.count++;
		}
		else
		{
			err = e;
		}
	}
	memcpy(buffer, &hdr1, sizeof(hdr1));
	*ioSize = ptr - (char*)buffer;
	return err;
}

// ================================================================================
// static API
// ================================================================================

// static
status_t 
BLocalControllable::SetValue(
	BParameterWeb *inWeb,
	int32 parameterID,
	bigtime_t when,
	const void *newValue,
	size_t valueSize)
{
	if(!inWeb->mLocalID)
		return B_BAD_TYPE;
	
	status_t err = B_BAD_INDEX;
	_sRoster.Lock();
	{
		BLocalControllable* instance = _sRoster.Find(inWeb->mLocalID);
		if(instance)
		{
			if(inWeb->mLocalTeam != instance->mTeam)
			{
				DEBUGGER(("BLocalControllable::SetValue(): team mismatch.\n"));
				err = B_BAD_VALUE;
			}
			else if(inWeb->mLocalChangeCount != instance->mChangeCount)
			{
				ASSERT(inWeb->mLocalChangeCount < instance->mChangeCount);
				fprintf(stderr,
					"BLocalControllable::SetValue(): web out of date.\n");
				err = B_BAD_VALUE;
			}
			else
			{
				instance->SetParameterValue(
					parameterID,
					when,
					newValue,
					valueSize);
				err = B_OK;
			}
		}
	}
	_sRoster.Unlock();
	return err;
}

// static
status_t 
BLocalControllable::GetValue(
	BParameterWeb *inWeb,
	int32 parameterID,
	bigtime_t *outLastChange,
	void *outValue,
	size_t *ioSize)
{
	if(!inWeb->mLocalID)
		return B_BAD_TYPE;
	
	status_t err = B_BAD_INDEX;
	_sRoster.Lock();
	{
		BLocalControllable* instance = _sRoster.Find(inWeb->mLocalID);
		if(instance)
		{
			if(inWeb->mLocalTeam != instance->mTeam)
			{
				DEBUGGER(("BLocalControllable::GetValue(): team mismatch.\n"));
				err = B_BAD_VALUE;
			}
			else if(inWeb->mLocalChangeCount != instance->mChangeCount)
			{
				ASSERT(inWeb->mLocalChangeCount < instance->mChangeCount);
				fprintf(stderr,
					"BLocalControllable::GetValue(): web out of date.\n");
				err = B_BAD_VALUE;
			}
			else
			{
				err = instance->GetParameterValue(
					parameterID,
					outLastChange,
					outValue,
					ioSize);
			}
		}
	}
	_sRoster.Unlock();
	return err;
}

// static
status_t 
BLocalControllable::AddObserver(
	BParameterWeb *ofWeb,
	const BMessenger &messenger,
	int32 notificationType)
{
	if(!ofWeb->mLocalID)
		return B_BAD_TYPE;
	
	status_t err = B_BAD_INDEX;
	_sRoster.Lock();
	{
		BLocalControllable* instance = _sRoster.Find(ofWeb->mLocalID);
		if(instance)
		{
			if(ofWeb->mLocalTeam != instance->mTeam)
			{
				DEBUGGER(("BLocalControllable::AddObserver(): team mismatch.\n"));
				err = B_BAD_VALUE;
			}
			else if(ofWeb->mLocalChangeCount != instance->mChangeCount)
			{
				ASSERT(ofWeb->mLocalChangeCount < instance->mChangeCount);
				fprintf(stderr,
					"BLocalControllable::AddObserver(): web out of date.\n");
				err = B_BAD_VALUE;
			}
			else
			{
				if(!instance->mObservers)
					instance->mObservers = new ObserverSet;
				err = instance->mObservers->Add(
					messenger,
					notificationType);

				if(err == B_OK &&
					(notificationType == B_MEDIA_WILDCARD ||
					 notificationType == B_MEDIA_NEW_PARAMETER_VALUE))
				{
					// send notification for each parameter
					int32 bufferSize = 32;
					const int32 bufferMax = 1024;
					int8* buffer = (int8*)malloc(bufferSize);
					if(!buffer)
						goto _bail;
					int32 c = instance->mWeb->CountParameters();
					for(int32 n = 0; n < c; n++)
					{
						BParameter* param = instance->mWeb->ParameterAt(n);
						ASSERT(param);
						bigtime_t when;
						while(true)
						{
							size_t size = bufferSize;
							err = param->GetValue(buffer, &size, &when);
							if(err == B_NO_MEMORY)
							{
								int32 newBufferSize = bufferSize << 2;
								if(newBufferSize > bufferMax)
									// give up on this parameter
									break;
								int8* newBuffer = (int8*)realloc(buffer, newBufferSize);
								if(!newBuffer)
									break;
								buffer = newBuffer;
								bufferSize = newBufferSize;
							}
							else if(err != B_OK)
							{
								// generic mishap
								break;
							}
							
							// +++++ this results in repeated nested lock/unlocks, ick.
							instance->BroadcastNewParameterValue(
								when, param->ID(), buffer, size);
							break;
						}
					}
					free(buffer);
					err = B_OK;
				}
			}
		}
	}
_bail:
	_sRoster.Unlock();
	return err;
}

// static
status_t 
BLocalControllable::RemoveObserver(
	BParameterWeb *ofWeb,
	const BMessenger &messenger,
	int32 notificationType)
{
	if(!ofWeb->mLocalID)
		return B_BAD_TYPE;
	
	status_t err = B_BAD_INDEX;
	_sRoster.Lock();
	{
		BLocalControllable* instance = _sRoster.Find(ofWeb->mLocalID);
		if(instance && instance->mObservers)
		{
			if(ofWeb->mLocalTeam != instance->mTeam)
			{
				DEBUGGER(("BLocalControllable::RemoveObserver(): team mismatch.\n"));
				err = B_BAD_VALUE;
			}
			else
			{
				err = instance->mObservers->Remove(messenger, notificationType);
			}
		}
	}
	_sRoster.Unlock();
	return err;
}

// ================================================================================

void 
BLocalControllable::Broadcast(BMessage *message)
{
	_sRoster.Lock();
	{
		if(mObservers)
		{
			mObservers->Broadcast(message, message->what);
		}
	}
	_sRoster.Unlock();
}


// ================================================================================

status_t
BLocalControllable::_Reserved_LocalControllable_0(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_1(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_2(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_3(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_4(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_5(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_6(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_7(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_8(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_9(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_10(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_11(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_12(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_13(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_14(void *)
{
	return B_ERROR;
}

status_t
BLocalControllable::_Reserved_LocalControllable_15(void *)
{
	return B_ERROR;
}
