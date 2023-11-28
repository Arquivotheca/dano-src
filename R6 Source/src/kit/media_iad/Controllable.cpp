/*	Controllable.cpp	*/

#include <MediaDefs.h>

#include <string.h>
#include <assert.h>
#include <OS.h>

#include <Messenger.h>
#include <Roster.h>
#include <image.h>

#include "Controllable.h"
#include "trinity_p.h"
#include "ParameterWeb.h"
#include "tr_debug.h"
#include "MediaAddOn.h"



namespace BPrivate {
class bc_lock {
public:
	bc_lock(BControllable & bc) : m_bc(bc), m_locked(bc.LockParameterWeb()) {}
	~bc_lock() { if (m_locked) m_bc.UnlockParameterWeb(); }
	bool operator!() { return !m_locked; }
	BControllable & m_bc;
	bool m_locked;
};
}
using namespace BPrivate;


BControllable::~BControllable()
{
	delete _mWeb;	/* bye, bye! */
	delete_sem(_m_webSem);
}


BParameterWeb *
BControllable::Web()
{
	if (_m_webBen > 0) {
		fprintf(stderr, "BControllable::Web() called without locking the web.\n");
	}
	return _mWeb;
}


status_t
BControllable::HandleMessage(
	int32 message,
	const void * data,
	size_t size)
{
	status_t err = B_OK;

	switch (message)
	{
	case CT_GET_WEB: {
		// WAA
		// By default we'll return 0 size and a out
		// of memory error.  We should really return a
		// better error than this though.  This will at
		// least stop us from crashing when there is no
		// control web.
		get_web_a * a = new get_web_a;
		a->size = 0;
		a->node = Node();
		a->error = B_NO_MEMORY;
		a->flags = 0;
		
		{
			bc_lock lock(*this);
	
			if (_mWeb) {
				a->size = _mWeb->FlattenedSize();
				if (a->size > sizeof(a->raw_data)) {
					err = ((_BMediaRosterP *)BMediaRoster::CurrentRoster())->
							FlattenHugeWeb(_mWeb, a->size, &a->big.area, &a->big.owner);
					a->big.big_size = a->size;
					a->size = 0;
					a->flags = _BIG_FLAT_WEB;
				}
				else {
					err = _mWeb->Flatten(a->raw_data, sizeof(a->raw_data));
				}
				
				a->node = Node();
				a->error = err;
				if (a->error < 0) {
					dlog("GET_WEB error %x", a->error);
				}
				else {
					dlog("GET_WEB size is %x", a->size);
				}
			}
		}
		if (write_port(((get_web_q *)data)->reply, CT_GET_WEB_REPLY, a, 
				sizeof(*a)-sizeof(a->raw_data)+a->size) < 0) {
			//	clean up if we couldn't pass on responsibility
			if (a->flags & _BIG_FLAT_WEB) {
				((_BMediaRosterP *)BMediaRoster::CurrentRoster())->
						RemoveAreaUser(a->big.area);
			}
		}
		delete a;
		} break;
	case CT_GET_VALUES: {
		get_values_a * a = new get_values_a;
		size_t size = sizeof(*a)-sizeof(a->error);
		a->error = MakeParameterData(((get_values_q *)data)->ids, 
			((get_values_q *)data)->count_ids, &a->node, 
			&size);
		write_port(((get_values_q *)data)->reply, CT_GET_VALUES_REPLY, 
			a, size+sizeof(a->error));
		delete a;
		} break;
	case CT_SET_VALUES: {
		(void)ApplyParameterData(data, size);
		} break;
	case CT_START_CONTROL_PANEL: {
		start_control_panel_a a;
		a.error = StartControlPanel(&a.messenger);
		a.cookie = ((start_control_panel_q *)data)->cookie;
		write_port(((start_control_panel_q *)data)->reply, CT_START_CONTROL_PANEL_REPLY,
			&a, sizeof(a));
		} break;
	default:
		return B_ERROR;
	}
	return err;
}


status_t
BControllable::ApplyParameterData(
	const void * data,
	size_t size)
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
		if (end-ptr < sizeof(hdr)) {
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
BControllable::MakeParameterData(
	const int32 * ids,
	int32 count,
	void * buffer,
	size_t * ioSize)
{
	status_t err = B_OK;
	char * ptr = (char *)buffer;
	size_t togo = *ioSize;
	struct {
		media_node node;
		int32 count;
	} hdr1;
	hdr1.node = Node();
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
	return err;
}


BControllable::BControllable() :
	BMediaNode("%ERROR%")	/* call SetParameterWeb() from your constructor */
{
	AddNodeKind(B_CONTROLLABLE);
	_mWeb = NULL;
	_m_webSem = create_sem(0, Name());
	_m_webBen = 1;
}


status_t
BControllable::SetParameterWeb(
	BParameterWeb * web)
{
	bc_lock lock(*this);

	bool notify = false;
	if (_mWeb != web) {
		notify = true;
		if (_mWeb) {
			delete _mWeb;
		}
	}
	_mWeb = web;
	if (web) {
		web->mNode = Node();
		if (notify) {
			BMessage msg;
			media_node node = Node();
			msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
			((_BMediaRosterP *)BMediaRoster::Roster())->Broadcast(this, msg, B_MEDIA_WEB_CHANGED);
		}
	}
	return B_OK;
}


/* Call when the actual control changes, NOT when the value changes. */
/* A typical case would be a CD with a Selector for Track when a new CD is inserted */
status_t
BControllable::BroadcastChangedParameter(
	int32 id)
{
	BMessage msg;
	media_node node = Node();
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	msg.AddInt32("parameter", id);
	return ((_BMediaRosterP *)BMediaRoster::CurrentRoster())->Broadcast(this, msg, B_MEDIA_PARAMETER_CHANGED);
}


status_t
BControllable::BroadcastNewParameterValue(
	bigtime_t when,
	int32 id,
	void * data,
	size_t size)
{
	BMessage msg;
	media_node node = Node();
	msg.AddData("be:node", B_RAW_TYPE, &node, sizeof(node));
	msg.AddInt32("be:parameter", id);
	msg.AddInt64("be:when", when);
	msg.AddData("be:value", B_RAW_TYPE, data, size);
	return ((_BMediaRosterP *)BMediaRoster::CurrentRoster())->Broadcast(this, msg, B_MEDIA_NEW_PARAMETER_VALUE);
}


status_t
BControllable::StartControlPanel(
	BMessenger * out_messenger)
{
	int32 id;
	BMediaAddOn * addon = AddOn(&id);
	if (addon == NULL) {
		return B_ERROR;
	}
	image_id im = addon->ImageID();
	if (im < 0) {
		return im;
	}
	image_info imin;
	status_t err = get_image_info(im, &imin);
	if (err < B_OK) {
		return err;
	}
	entry_ref ref;
	err = get_ref_for_path(imin.name, &ref);
	if (err < B_OK) {
		return err;
	}
	team_id team = -1;
	char value[32];
	sprintf(value, "node=%d", ID());
	char * argv[2] = { value, NULL };
	err = be_roster->Launch(&ref, 1, argv, &team);
	if (err >= B_OK) {
		*out_messenger = BMessenger(NULL, team);
		err = B_OK;
	}
	return err;
}


bool
BControllable::LockParameterWeb()
{
	status_t err = B_OK;
	if (atomic_add(&_m_webBen, -1) < 1) {
		err = acquire_sem(_m_webSem);
	}
	return (err >= B_OK);
}

void
BControllable::UnlockParameterWeb()
{
	assert(_m_webBen < 1);
	if (atomic_add(&_m_webBen, 1) < 0) {
		release_sem(_m_webSem);
	}
}



		/* Mmmh, stuffing! */
status_t
BControllable::_Reserved_Controllable_0(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_1(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_2(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_3(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_4(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_5(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_6(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_7(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_8(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_9(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_10(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_11(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_12(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_13(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_14(void *)
{
	return B_ERROR;
}

status_t
BControllable::_Reserved_Controllable_15(void *)
{
	return B_ERROR;
}

