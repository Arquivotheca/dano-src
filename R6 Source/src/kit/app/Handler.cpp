/*****************************************************************************

     $Source: /net/bally/be/rcs/src/kit/app/Handler.cpp,v $

     $Revision: 1.38 $

     $Date: 1997/04/30 08:05:08 $

	 Written By: Peter Potrebic

     Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <Debug.h>

#include <stdlib.h>
#include <string.h>
#include <byteorder.h>

#include <Handler.h>
#include <Message.h>
#include <Messenger.h>
#include <Looper.h>
#include <MessageFilter.h>
#include <PropertyInfo.h>
#include <ObjectList.h>

#include <message_util.h>
#include <token.h>

const char *B_NOTIFICATION_SENDER = "be:sender";

//  ---- Observer list private classes ---

const uint32 kStartObserving = 'OBST';
const uint32 kStopObserving = 'OBSP';

#define kObserveTarget "be:observe_target"

class _ObserverWhatList {
public:
	_ObserverWhatList(uint32, BHandler *);
	_ObserverWhatList(uint32, const BMessenger *);
	
	~_ObserverWhatList();
	
	void Add(const BMessenger *);
	bool Remove(const BMessenger *);
	void Add(BHandler *);
	bool Remove(BHandler *);
	
	void Send(BMessage *);

	uint32 what;

private:
	BObjectList<BMessenger> *messengerObserverList;
		// for inter-looper and inter-team notices
	BObjectList<BHandler> *handlerObserverList;
		// holds handlers that haven't been attached yet
};


class _ObserverList : public BObjectList<_ObserverWhatList> {
public:
	_ObserverList()
		:	BObjectList<_ObserverWhatList>(10, true),
			allList(NULL)
		{}
	
	status_t StartObserving(const BMessenger *, uint32 what);
	status_t StopObserving(const BMessenger *, uint32 what);
	status_t StartObserving(BHandler *, uint32 what);
	status_t StopObserving(BHandler *, uint32 what);
	
	virtual void SendNotices(uint32 what, const BMessage * = NULL);

	_ObserverWhatList *allList;	
};

/*---------------------------------------------------------------*/

BHandler::BHandler(const char *name)
	: BArchivable()
{
	InitData(name);
}

void BHandler::InitData(const char *name)
{
	fToken = gDefaultTokens->NewToken(HANDLER_TOKEN_TYPE, this);

	fLooper = NULL;
	fName = NULL;
	SetName(name);
	fNextHandler = NULL;
	fFilters = NULL;
	fObserverList = NULL;
}

#if 0
#include <unistd.h>
#include <errno.h>
static hex_dump(void *p, ssize_t s)
{
	int fd = open("dump.out2", O_RDWR | O_CREAT);
	PRINT(("fd = %x (%s)\n", fd, strerror(errno)));
	long ww = write(fd, p, s);
	close(fd);
	return 0;
}
#endif

/*---------------------------------------------------------------*/

#define S_NAME	"_name"

BHandler::BHandler(BMessage *data)
	: BArchivable(data)
{
	const char *str;
	data->FindString(S_NAME, &str);
	InitData(str);
}

/*---------------------------------------------------------------*/

status_t BHandler::Archive(BMessage *data, bool deep) const
{
	status_t err;

	err = _inherited::Archive(data, deep);
	if (!err) {
		if (Name())
			err = data->AddString(S_NAME, Name());
	}
	return err;
}

/*---------------------------------------------------------------*/

BArchivable *BHandler::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BHandler"))
		return NULL;
	return new BHandler(data);
}

/*---------------------------------------------------------------*/

BHandler::~BHandler()
{
	if (Looper() && Looper()->Lock()) {
		BLooper *l = Looper();
		l->RemoveHandler(this);
		l->Unlock();
	}
	SetName(NULL);
	gDefaultTokens->RemoveToken(fToken);

	SetFilterList(NULL);

	delete fObserverList;
}

/*---------------------------------------------------------------*/

bool BHandler::LockLooper()
{
	BLooper	*loop = Looper();
	bool	val = loop && loop->Lock();

	if (val) {
		val = (Looper() == loop);
		if (!val)
			loop->Unlock();
	}

	return val;
}

/*---------------------------------------------------------------*/

status_t BHandler::LockLooperWithTimeout(bigtime_t timeout)
{
	BLooper		*loop = Looper();
	status_t	err = B_BAD_VALUE;
	
	if (loop) {
		err = loop->LockWithTimeout(timeout);

		if (err == B_OK) {
			if (Looper() != loop) {
				loop->Unlock();
				err = B_MISMATCHED_VALUES;
			}
		} 
	}

	return err;
}

/*---------------------------------------------------------------*/

void BHandler::UnlockLooper()
{
	Looper()->Unlock();
}

/*---------------------------------------------------------------*/

void BHandler::MessageReceived(BMessage *msg)
{
	status_t	err = B_OK;
	BMessage	reply(B_REPLY);

	switch (msg->what) {

		case B_GET_SUPPORTED_SUITES:
			err = GetSupportedSuites(&reply);
			reply.AddInt32("error", err);
			msg->SendReply(&reply);
			return;
	
		case B_GET_PROPERTY:
			{
				BMessage	specifier;
				int32		form;
				const char	*prop;
				int32		cur;
				bool 		handled = false;

				msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
				if (cur == -1 || strcmp(prop, "Messenger") == 0) {
					BMessenger	mess(this, Looper());
					err = reply.AddMessenger("result", mess);
					handled = true;
				} else if (strcmp(prop, "InternalName") == 0) {
					if (Name())
						err = reply.AddString("result", Name());
					else
						err = B_NAME_NOT_FOUND;
					handled = true;
				} else if (strcmp(prop, "Suites") == 0) {
					err = GetSupportedSuites(&reply);
					handled = true;
				}

				if (handled) {
					reply.AddInt32("error", err);
					msg->SendReply(&reply);
					return;
				}
				break;
			}

		case kStartObserving:
			{
				BMessenger target;
				if (msg->FindMessenger(kObserveTarget, &target) == B_OK) {
					int32 what = 0;
					msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &what);
					if (!fObserverList) {
						fObserverList = new _ObserverList;
					}
					fObserverList->StartObserving(&target, what);
				}
				return;
			}
	
		case kStopObserving:
			{
				BMessenger target;
				if (fObserverList
					&& msg->FindMessenger(kObserveTarget, &target) == B_OK) {
					int32 what = 0;
					msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &what);
					fObserverList->StopObserving(&target, what);
				}
				return;
			}

		default:
			break;
	}


	BHandler *parent = NextHandler();
	if (parent) {
		// Need to filter for this handler
//+		ASSERT(Looper() == parent->Looper());

		BHandler *target = parent->Looper()->handler_only_filter(msg, parent);
		if (target)
			target->MessageReceived(msg);
	} else if (msg->WasDropped() && (msg->what != B_MESSAGE_NOT_UNDERSTOOD)) {
		/*
		 Send a reply indicating that no one understood the message.
		 Any 'reply' to this new message should go back to the
		 original target of the dropped message.
	 	 */

		BHandler	*handler = NULL;
		int32 token = _get_message_target_(msg);
		gDefaultTokens->GetToken(token, HANDLER_TOKEN_TYPE, (void **) &handler);

//+		PRINT(("NOT_UNDERSTOOD, what=%.4s, handler=%x, name=%s\n",
//+			(char*) &(msg->what), handler, handler ? handler->Name() : "null"));
		BMessage reply(B_MESSAGE_NOT_UNDERSTOOD);

//+		// need to keep this information available
		BPoint pt;
		msg->FindPoint("_drop_point_", &pt);
		reply.AddPoint("_old_drop_point_", pt);
		msg->FindPoint("_drop_offset_", &pt);
		reply.AddPoint("_old_drop_offset_", pt);

		msg->SendReply(&reply, handler);
	} else if (msg->HasSpecifiers()) {
		reply.what = B_MESSAGE_NOT_UNDERSTOOD;
		reply.AddString("message", "Didn't understand the specifier(s)");
		reply.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
		msg->SendReply(&reply);
	}
}

/*-------------------------------------------------------------*/
	/*
	 A generic BHandler supports the following:
	 	GET "Messenger"			DIRECT form only
	 	GET "InternalName"		DIRECT form only
	*/

#if _SUPPORTS_FEATURE_SCRIPTING

static property_info prop_list[] = {
	{
	"Suites",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{ },
		{ 

			{
				{
					{ "suites", B_STRING_TYPE } 
				}
			},
			{
				{
					{ "messages", B_PROPERTY_INFO_TYPE } 
				}
			}
		},
		{}
	},
	{"Messenger",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{ B_MESSENGER_TYPE },
		{},
		{}
	},
	{"InternalName",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{ B_STRING_TYPE },
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};

#endif

/*-------------------------------------------------------------*/

BHandler *BHandler::ResolveSpecifier(BMessage *_SCRIPTING_ONLY(msg), int32 _SCRIPTING_ONLY(index),
	BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form), const char *_SCRIPTING_ONLY(prop))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	int	i;
	BPropertyInfo	pi(prop_list);
	i = pi.FindMatch(msg, index, spec, form, prop);
	if (i >= 0)
		return this;

	BMessage error_msg(B_MESSAGE_NOT_UNDERSTOOD);
	error_msg.AddString("message", "Didn't understand the specifier(s)");
	error_msg.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
	msg->SendReply(&error_msg);
#endif
	return NULL;
}

/*---------------------------------------------------------------*/

status_t	BHandler::GetSupportedSuites(BMessage *_SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", "suite/vnd.Be-handler");
	BPropertyInfo	pi(prop_list);
	data->AddFlat("messages", &pi);
	return B_OK;
#else
	return B_UNSUPPORTED;
#endif
}

/*---------------------------------------------------------------*/

BLooper	*BHandler::Looper() const
{
	return fLooper;
}

/*---------------------------------------------------------------*/

const char*	BHandler::Name() const
{
	return(fName);
}

/*---------------------------------------------------------------*/

void	BHandler::SetName(const char* name)
{
	if (fName) {
		free(fName);
		fName = NULL;
	}

	if (name)
		fName = strdup(name);
}

/*---------------------------------------------------------------*/

void	BHandler::SetFilterList(BList *filters)
{
	BLooper *loop = Looper();

	if (loop && !loop->IsLocked()) {
		debugger("Owning Looper must be locked before calling SetFilterList");
		return;
	}

	if (fFilters) {
		long c = fFilters->CountItems();
		for (long i = 0; i < c; i++) {
			BMessageFilter *filter = (BMessageFilter *) fFilters->ItemAtFast(i);
			ASSERT(filter);
			delete filter;
		}

		delete fFilters;
		fFilters = NULL;
	}
	fFilters = filters;
}

/*---------------------------------------------------------------*/

void	BHandler::SetLooper(BLooper *loop)
{
	fLooper = loop;

	if (fFilters) {
		int32	c = fFilters->CountItems();
		for (int32 i = 0; i < c; i++) {
			BMessageFilter *filter = (BMessageFilter *) fFilters->ItemAtFast(i);
			ASSERT(filter);
			filter->SetLooper(loop);
		}
	}
}

/*---------------------------------------------------------------*/

void	BHandler::AddFilter(BMessageFilter *filter)
{
	/*
	 This is just a handy shortcut so that in the common case the
	 developer doesn't have to create the initial BList by hand.
	*/ 
	BLooper *loop = Looper();

	if (loop && !loop->IsLocked()) {
		debugger("Owning Looper must be locked before calling AddFilter");
		return;
	}
	if (filter->Looper() != NULL) {
		debugger("A MessageFilter can only be used once.");
		return;
	}

	if (loop)
		filter->SetLooper(loop);

	if (fFilters == NULL) {
		fFilters = new BList();
	}

	fFilters->AddItem(filter);
}

/*---------------------------------------------------------------*/

bool	BHandler::RemoveFilter(BMessageFilter *filter)
{
	BLooper *loop = Looper();

	if (loop && !loop->IsLocked()) {
		debugger("Owning Looper must be locked before calling RemoveFilter");
		return false;
	}

	if (!fFilters)
		return false;

	if (fFilters->RemoveItem(filter)) {
		filter->SetLooper(NULL);
		return true;
	}
	return false;
}

/*---------------------------------------------------------------*/

BList	*BHandler::FilterList()
{
	return fFilters;
}

/*-------------------------------------------------------------*/
// The default parent handler is the looper. This is used 
// when passing messages up a handler chain.

BHandler *BHandler::NextHandler() const
{
	return fNextHandler;
}


void BHandler::SetNextHandler(BHandler *handler)
{
	if (!fLooper) {
		debugger("handler must belong to looper before setting NextHandler");
		return;
	}
	if (!fLooper->IsLocked()) {
		debugger("The handler's looper must be locked before setting NextHandler");
		return;
	}
	if (handler && (fLooper != handler->fLooper)) {
		debugger("The handler and it's NextHandler must have the same looper");
		return;
	}

	fNextHandler = handler;
}

void 
BHandler::SendNotices(uint32 what, const BMessage *message)
{
	if (fObserverList)
		fObserverList->SendNotices(what, message);
}

bool
BHandler::IsWatched() const
{
	return fObserverList ? true : false;
}

status_t 
BHandler::StartWatching(BHandler *target, uint32 what)
{
	// handle observing inside the same looper
	// also allow observing by a handler that is not yet attached
	// - assuming it will be in the same looper

	if (target->Looper() && target->Looper() != Looper()) {
		// handler in a different looper, use a messenger instead
		BMessenger messengerTarget(target);
		if (!messengerTarget.IsValid())
			return B_BAD_HANDLER;
		StartWatching(messengerTarget, what);
	}
	
	if (!fObserverList) 
		fObserverList = new _ObserverList;
	return fObserverList->StartObserving(target, what);
}

status_t 
BHandler::StartWatchingAll(BHandler *target)
{
	return StartWatching(target, B_OBSERVER_OBSERVE_ALL);
}

status_t 
BHandler::StopWatching(BHandler *target, uint32 what)
{
	if (target->Looper() && target->Looper() != Looper()) {
		// handler in a different looper, use a messenger instead
		BMessenger messengerTarget(target);
		if (!messengerTarget.IsValid())
			return B_BAD_HANDLER;
		StopWatching(messengerTarget, what);
	}

	if (!fObserverList)
		return B_BAD_HANDLER;

	return fObserverList->StopObserving(target, what);
}

status_t 
BHandler::StartWatching(BMessenger target, uint32 what)
{
	BMessage message(kStartObserving);
	BMessenger self(this, Looper());
	message.AddMessenger(kObserveTarget, self);
	message.AddInt32(B_OBSERVE_WHAT_CHANGE, what);
	return target.SendMessage(&message, (BHandler *)NULL, 1000000);
}

status_t 
BHandler::StartWatchingAll(BMessenger target)
{
	return StartWatching(target, B_OBSERVER_OBSERVE_ALL);
}

status_t 
BHandler::StopWatching(BMessenger target, uint32 what)
{
	BMessage message(kStopObserving);
	BMessenger self(this, Looper());
	message.AddMessenger(kObserveTarget, self);
	message.AddInt32(B_OBSERVE_WHAT_CHANGE, what);
	return target.SendMessage(&message, (BHandler *)NULL, 1000000);
}

status_t 
BHandler::StopWatchingAll(BHandler *target)
{
	return StopWatching(target, B_OBSERVER_OBSERVE_ALL);
}

status_t 
BHandler::StopWatchingAll(BMessenger target)
{
	return StopWatching(target, B_OBSERVER_OBSERVE_ALL);
}


/*----------------------------------------------------------------*/

status_t BHandler::Perform(perform_code d, void *arg)
{
	return _inherited::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

BHandler::BHandler(const BHandler &)
	:	BArchivable()
	{}
BHandler	&BHandler::operator=(const BHandler &) { return *this; }

/* ---------------------------------------------------------------- */

void BHandler::_ReservedHandler2() {}
void BHandler::_ReservedHandler3() {}
void BHandler::_ReservedHandler4() {}

#if _SUPPORTS_FEATURE_SCRIPTING

struct _oproperty_info_ {
	char	*name;
	uint32	commands[10];
	uint32	specifiers[10];
	char	*usage;
	uint32	extra_data;
};

const uint32 _V2_PROP_INFO_FLAG_	= 0x00000001;
const uint32 _VALUE_INFO_FLAG_		= 0x00000002;


BPropertyInfo::BPropertyInfo(property_info *d, value_info *vi, bool freeOnDel)
{
	fPropInfo = d;
	fValueInfo = vi;
	fOldPropInfo = NULL;
	fPropCount = fValueCount = 0;
	while (d && d->name) {
		fPropCount++;
		d++;
	}
	while (vi && vi->name) {
		fValueCount++;
		vi++;
	}
	fInHeap = freeOnDel;
	fOldInHeap = false;
}

/*-------------------------------------------------------------*/

//
// DEPRECATED
//
// really takes a pointer to _oproperty_info_
//
BPropertyInfo::BPropertyInfo(property_info *nd, bool freeOnDel)
{
	_oproperty_info_ *d = (_oproperty_info_ *) nd;

	fPropInfo = ConvertToNew(d);
	fOldPropInfo = d;
	fValueInfo = NULL;
	fPropCount = 0;
	while (d && d->name) {
		fPropCount++;
		d++;
	}
	fInHeap = true;
	fOldInHeap = freeOnDel;
}

/*-------------------------------------------------------------*/

BPropertyInfo::~BPropertyInfo()
{
//+	PRINT(("Destructor: x(%x,%d), i(%x,%d)\n",
//+		fPropInfo, fInHeap, fOldPropInfo, fOldInHeap));
	FreeMem();
}

/*-------------------------------------------------------------*/

bool BPropertyInfo::IsFixedSize() const
{
	return false;
}

/*-------------------------------------------------------------*/

type_code BPropertyInfo::TypeCode() const
{
	return B_PROPERTY_INFO_TYPE;
}

/*-------------------------------------------------------------*/

bool BPropertyInfo::AllowsTypeCode(type_code code) const
{
//+	PRINT(("AllowsType (%.4s)\n", (char*) &code));
	if ((code == B_PROPERTY_INFO_TYPE))
		return true;
	return false;
}

/*-------------------------------------------------------------*/

ssize_t BPropertyInfo::FlattenedSize() const
{
	ssize_t			size = 0;
	property_info	*d = fPropInfo;
	int32			c = 0;

	size += sizeof(char);		// leave space for Endian flag
	size += sizeof(fPropCount);	// first thing is a count field
	size += sizeof(uint32);		// leave space for a format field

	/*
	 Compatibility - just calculate size for the old property_info struct
	 as of BeOS version R3.
	*/
	while (c++ < fPropCount) {
		ASSERT(d->name);
		size += strlen(d->name) + 1;

		if (d->usage) {
			size += strlen(d->usage);
		} 
		size += 1;							// leave room for a NULL.

		for (int i = 0; (i < 10) && d->commands[i] != 0; i++)
			size += sizeof(d->commands[0]);
		size += sizeof(d->commands[0]);			// space for 0 to end the list

		for (int i = 0; (i < 10) && d->specifiers[i] != 0; i++)
			size += sizeof(d->specifiers[0]);
		size += sizeof(d->specifiers[0]);		// space for 0 to end the list

		size += sizeof(d->extra_data);

		d++;
	}


	/*
	 Compatibility - now calculate size including the new fields.
	*/
	c = 0;
	d = fPropInfo;
	while (c++ < fPropCount) {
		// deal with the "types" array
		for (int i = 0; (i < 10) && d->types[i] != 0; i++)
			size += sizeof(d->types[0]);
		size += sizeof(d->types[0]);		// space for 0 to end the list

#if 1
		// deal with the "ctypes" array
		compound_type	*ct = d->ctypes;
		for (int i = 0; (i < 3) && ct[i].pairs[0].name; i++) {
			for (int j = 0; (j < 5) && ct[i].pairs[j].name; j++) {
				size += strlen(ct[i].pairs[j].name) + 1;
				size += sizeof(ct[i].pairs[j].type);
//+				if (ct[i].pairs[j].label) {
//+					size += strlen(ct[i].pairs[j].label);
//+				}
//+				size += 1;					// leave room for a NULL.
			}
			size += sizeof(int32);			// space to end entry_pair list
		}
		size += sizeof(int32);				// space to end compound_type list
#endif

		d++;
	}

	/*
	 Now we're dealing with the value_info data
	*/

	if (fValueInfo)  {
		ASSERT(fValueCount > 0);
		size += sizeof(fValueCount);		// first thing is a count field
	}

	c = 0;
	value_info *v = fValueInfo;
	while (c++ < fValueCount) {
		ASSERT(fValueInfo);
		size += sizeof(v->kind);
		size += sizeof(v->value);

		if (v->name) {
			size += strlen(v->name);
		}
		size += 1;							// leave room for a NULL.

		if (v->usage) {
			size += strlen(v->usage);
		}
		size += 1;							// leave room for a NULL.

		size += sizeof(v->extra_data);

		v++;
	}

//+	PRINT(("flatsize=%d\n", size));
	return size;
}

/*-------------------------------------------------------------*/

void BPropertyInfo::PrintToStream() const
{
	property_info	*d = fPropInfo;
	int32			c = 0;

	if (!fPropInfo)
		goto next;

	printf("%14s   %s%22s %s%15s %s\n", "property", "commands", " ", "types", " ", "specifiers");
	printf("--------------------------------------------------------------"
			"------------------\n");

	while (c++ < fPropCount) {
		ASSERT(d->name);
		printf("%14s   ", d->name);

		int i;

		for (i = 0; d->commands[i] != 0; i++) {
			/* Might as well get rid of another endian-dependancy... AJH */
			int32 swappedvalue = B_BENDIAN_TO_HOST_INT32(d->commands[i]);
			printf("%.4s ", (char *) &swappedvalue);
		}
		while (i++ <= 5)
			printf("%5s", " ");

		for (i = 0; d->types[i] != 0; i++)
			printf("%.4s ", (char *) &(d->types[i]));
		while (i++ <= 3)
			printf("%5s", " ");

		for (i = 0; d->specifiers[i] != 0; i++)
			printf("%2ld ", d->specifiers[i]);

		if (d->usage || d->extra_data) {
			printf("\n%17s", " ");
			if (d->usage)
				printf("%s\t", d->usage);
			if (d->extra_data)
				printf("(extra_data: 0x%lx)", d->extra_data);
		}
		if (d->ctypes[0].pairs[0].name) {
			printf("\n");
			for (int i = 0; (i < 3) && d->ctypes[i].pairs[0].name; i++) {
				printf("%17s", " ");
				for (int j = 0; (j < 5) && d->ctypes[i].pairs[j].name; j++) {
					if (j > 0)
						printf(", ");
					printf("(%.4s,%s)",
						(char*) &(d->ctypes[i].pairs[j].type),
						d->ctypes[i].pairs[j].name);
				}
				printf("\n");
			}
		}

		printf("\n");
		d++;
	}

next:

	if (!fValueInfo)
		return;
		
	printf("\n%14s   %s\t%s\t%s\n", "property", "value", "kind", "usage");
	printf("-------------------------------------------------------"
		"-------------------------\n");

	value_info	*v = fValueInfo;
	c = 0;

	while (c++ < fValueCount) {
		ASSERT(v->name);
		printf("%14s   ", v->name);

		printf("%.4s\t%.4s", (char*) &(v->value),
			v->kind == B_COMMAND_KIND ? "Cmd " : "Type");
		
		if (v->usage) {
			printf("\t%s", v->usage);
		}

		printf("\n");
		v++;
	}
}

/*-------------------------------------------------------------*/

property_info *BPropertyInfo::ConvertToNew(const _oproperty_info_ *pi)
{
	int32					c = 0;
	const _oproperty_info_	*d = pi;
	property_info			*new_info;
	property_info			*ni;

	while (d && d->name) {
		c++;
		d++;
	}

	new_info = (property_info *) malloc(sizeof(property_info) * c);

	d = pi;
	ni = new_info;
	while (d && d->name) {
		*((_oproperty_info_ *) ni) = *d;
		if (d->name)
			ni->name = strdup(d->name);
		if (d->usage)
			ni->usage = strdup(d->usage);
		ni->types[0] = 0;
		d++;
		ni++;
	}
	return new_info;
}

/*-------------------------------------------------------------*/

_oproperty_info_ *BPropertyInfo::ConvertFromNew(const property_info *pi)
{
	int32				c = 0;
	const property_info	*d = pi;
	_oproperty_info_	*new_info;
	_oproperty_info_	*ni;

	while (d && d->name) {
		c++;
		d++;
	}

	new_info = (_oproperty_info_ *) malloc(sizeof(_oproperty_info_) * c);

	d = pi;
	ni = new_info;
	while (d && d->name) {
		*ni = *((_oproperty_info_ *) d);
		if (d->name)
			ni->name = strdup(d->name);
		if (d->usage)
			ni->usage = strdup(d->usage);
		d++;
		ni++;
	}
	return new_info;
}

/*-------------------------------------------------------------*/

void BPropertyInfo::FreeMem()
{
	if (fPropInfo && fInHeap) {
//+		PRINT(("Free PropInfo\n"));
		FreeInfoArray(fPropInfo, fPropCount);

		free(fPropInfo);
		fPropInfo = NULL;
	}
	
	if (fValueInfo && fInHeap) {
//+		PRINT(("Free ValueInfo\n"));
		FreeInfoArray(fValueInfo, fValueCount);

		free(fValueInfo);
		fValueInfo = NULL;
		fValueCount = 0;
	}
	
	if (fOldPropInfo && fOldInHeap) {
//+		PRINT(("Free OldPropInfo\n"));
		FreeInfoArray(fOldPropInfo, fPropCount);

		free(fOldPropInfo);
		fOldPropInfo = NULL;
	}
	fPropCount = 0;
}

/*-------------------------------------------------------------*/

void BPropertyInfo::FreeInfoArray(property_info *d, int32 count)
{
	int32			c = 0;

	while (c++ < count) {
		if (d->name)
			free(d->name);
		if (d->usage)
			free(d->usage);
		d++;
	}
}

/*-------------------------------------------------------------*/

void BPropertyInfo::FreeInfoArray(value_info *d, int32 count)
{
	int32			c = 0;

	while (c++ < count) {
		if (d->name)
			free(d->name);
		if (d->usage)
			free(d->usage);
		d++;
	}
}

/*-------------------------------------------------------------*/

void BPropertyInfo::FreeInfoArray(_oproperty_info_ *d, int32 count)
{
	int32			c = 0;

	while (c++ < count) {
		if (d->name)
			free(d->name);
		if (d->usage)
			free(d->usage);
		d++;
	}
}

/*-------------------------------------------------------------*/

status_t BPropertyInfo::Flatten(void *buffer, ssize_t) const
{
	property_info	*d = fPropInfo;
	char				*p = (char *) buffer;
	int32				c = 0;
	uint32				format = _V2_PROP_INFO_FLAG_;
	uint8				endian = B_HOST_IS_BENDIAN;

	memcpy(p, &endian, sizeof(endian));
	p += sizeof(endian);

	memcpy(p, &fPropCount, sizeof(fPropCount));
	p += sizeof(fPropCount);

	if (fValueInfo)
		format |= _VALUE_INFO_FLAG_;

	memcpy(p, &format, sizeof(format));
	p += sizeof(format);

	/*
	 First in the buffer is the data for old style property_info struct.
	 BeOS version R3 and earlier.
	*/
	while (c++ < fPropCount) {
		ssize_t	len;
		ASSERT(d->name);
		len = strlen(d->name) + 1;
		memcpy(p, d->name, len);
		p += len;

		if (d->usage) {
			len = strlen(d->usage) + 1;
			strcpy(p, d->usage);
			p += len;
		} else {
			*p = 0;			// if no syntax string just write a NULL char
			p += 1;
		}

		// extra_data field
		memcpy(p, &(d->extra_data), sizeof(d->extra_data));
		p += sizeof(d->extra_data);

		// command array
		for (int i = 0; (i < 10) && d->commands[i] != 0; i++) {
			memcpy(p, &(d->commands[i]), sizeof(d->commands[i]));
			p += sizeof(d->commands[i]);
		}
		memset(p, 0, sizeof(d->commands[0]));	// terminate list of commands
		p += sizeof(d->commands[0]);

		// specifier array
		for (int i = 0; (i < 10) && d->specifiers[i] != 0; i++) {
			memcpy(p, &(d->specifiers[i]), sizeof(d->specifiers[i]));
			p += sizeof(d->specifiers[i]);
		}
		memset(p, 0, sizeof(d->specifiers[0]));	// terminate list of specifiers
		p += sizeof(d->specifiers[0]);

		d++;
	}

	/*
	 Need to deal with the new "types" field here, at the end, so that
	 this data format is backward compatible. A BPropertyInfo flattened
	 buffer should be readable by an old application
	*/
	c = 0;
	d = fPropInfo;
	while (c++ < fPropCount) {
		// types array
		for (int i = 0; (i < 10) && d->types[i] != 0; i++) {
			memcpy(p, &(d->types[i]), sizeof(d->types[i]));
			p += sizeof(d->types[i]);
		}
		memset(p, 0, sizeof(d->types[0]));	// add terminating 0
		p += sizeof(d->types[0]);

#if 1
		// deal with the "ctypes" array
		compound_type	*ct = d->ctypes;
		for (int i = 0; (i < 3) && ct[i].pairs[0].name; i++) {
			for (int j = 0; (j < 5) && ct[i].pairs[j].name; j++) {
				ssize_t	len;
				len = strlen(ct[i].pairs[j].name) + 1;
				memcpy(p, ct[i].pairs[j].name, len);
				p += len;

				memcpy(p, &(ct[i].pairs[j].type), sizeof(ct[i].pairs[j].type));
				p += sizeof(ct[i].pairs[j].type);

//+				if (ct[i].pairs[j].label) {
//+					len = strlen(ct[i].pairs[j].label) + 1;
//+					memcpy(p, ct[i].pairs[j].label, len);
//+					p += len;
//+				} else {
//+					*p = 0;					// no <label> just write a NULL 
//+					p += 1;
//+				}
			}
			memset(p, 0, sizeof(int32));	// terminate list of entry_pairs
			p += sizeof(int32);
		}
		memset(p, 0, sizeof(int32));		// terminate list of compound_types
		p += sizeof(int32);
#endif

		d++;
	}

	if (!fValueInfo)
		return B_OK;

	memcpy(p, &fValueCount, sizeof(fValueCount));
	p += sizeof(fValueCount);

	c = 0;
	value_info *v = fValueInfo;
	while (c++ < fValueCount) {
		ssize_t	len;
		ASSERT(v->name);

		// kind field
		memcpy(p, &(v->kind), sizeof(v->kind));
		p += sizeof(v->kind);

		// value field
		memcpy(p, &(v->value), sizeof(v->value));
		p += sizeof(v->value);

		len = strlen(v->name) + 1;
		memcpy(p, v->name, len);
		p += len;

		if (v->usage) {
			len = strlen(v->usage) + 1;
			strcpy(p, v->usage);
			p += len;
		} else {
			*p = 0;			// if no syntax string just write a NULL char
			p += 1;
		}

		// extra_data field
		memcpy(p, &(v->extra_data), sizeof(v->extra_data));
		p += sizeof(v->extra_data);

		v++;
	}

	return B_OK;
}

/*-------------------------------------------------------------*/

status_t BPropertyInfo::Unflatten(type_code, const void *buffer, ssize_t /*size*/)
{
	property_info	*d;
	char			*p = (char *) buffer;
	int32			c = 0;
	int32			format;
	uint8			endian;
	bool			swap;
	bool			has_value_info = false;
	bool			has_new_prop_info = false;
	
//+	PRINT(("Unflatten (%.4s, %d)\n", (char*) &type, size));
	FreeMem();
	
	memcpy(&endian, p, sizeof(endian));
	p += sizeof(endian);

	swap = (endian != B_HOST_IS_BENDIAN);

	memcpy(&fPropCount, p, sizeof(fPropCount));
	if (swap)
		fPropCount = __swap_int32(fPropCount);
	p += sizeof(fPropCount);

	memcpy(&format, p, sizeof(format));
	if (swap)
		format = __swap_int32(format);
	p += sizeof(format);
	
	fPropInfo = (property_info *) malloc(fPropCount*sizeof(property_info));
	if (!fPropInfo) {
		fPropCount = 0;
		return B_NO_MEMORY;
	}
	memset(fPropInfo, 0, fPropCount * sizeof(property_info));

	// Compatibility. In pre R4 releases set the format field to -1
	if ((format == -1) || (format == -2)) {
		format = 0;
	} else {
		has_new_prop_info = (format & _V2_PROP_INFO_FLAG_) != 0;
		has_value_info = (format & _VALUE_INFO_FLAG_) != 0;
	}

	d = fPropInfo;

//+	PRINT(("fPropCount=%d, c=%d, d=%x\n", fPropCount, c, d));
//+	hex_dump(p, size);

	while (c++ < fPropCount) {
//+		PRINT(("prop=%s\n", p));
		d->name = strdup(p);
		if (!d->name) {
			FreeMem();
			return B_NO_MEMORY;
		}

		p += strlen(d->name) + 1;

		if (*p) {
			d->usage = strdup(p);
			if (!d->usage) {
				FreeMem();
				return B_NO_MEMORY;
			}

			p += strlen(d->usage);
		} 
		p += 1;								// skip the NULL char

		// extra_data field
		memcpy(&(d->extra_data), p, sizeof(d->extra_data));
		if (swap)
			d->extra_data = __swap_int32(d->extra_data);
		p += sizeof(d->extra_data);

		int i;

		// command array
		i = 0;
		while ((i < 10) && *((long *) p) != 0) {
			memcpy(&(d->commands[i]), p, sizeof(d->commands[i]));
			if (swap)
				d->commands[i] = __swap_int32(d->commands[i]);
			p += sizeof(d->commands[i]);
			i++;
		}
		p += sizeof(d->commands[0]);		// skip the terminating 0

		// specifier array
		i = 0;
		while ((i < 10) && *((long *) p) != 0) {
			memcpy(&(d->specifiers[i]), p, sizeof(d->specifiers[i]));
			if (swap)
				d->specifiers[i] = __swap_int32(d->specifiers[i]);
			p += sizeof(d->specifiers[i]);
			i++;
		}
		p += sizeof(d->specifiers[0]);		// skip the terminating 0

		d++;
	}

//+	ASSERT((has_new_prop_info && (p-buffer < size)) ||
//+			(!has_prop_info && (p-buffer == size)));

//+	PRINT(("Unflatten almost done. size=%d, psize=%d\n",size, p - buffer));
	if (has_new_prop_info) {
		int	i;
		d = fPropInfo;
		c = 0;
		while (c++ < fPropCount) {
			// types array
			i = 0;
			while ((i < 10) && *((long *) p) != 0) {
				memcpy(&(d->types[i]), p, sizeof(d->types[i]));
				if (swap)
					d->types[i] = __swap_int32(d->types[i]);
				p += sizeof(d->types[i]);
				i++;
			}
			p += sizeof(d->types[0]);		// skip the terminating 0
#if 1
			// deal with the "ctypes" array
			compound_type	*ct = d->ctypes;
			for (int i = 0; (i < 3) && *((long *) p); i++) {
				for (int j = 0; (j < 5) && *((long *) p); j++) {
					ct[i].pairs[j].name = strdup(p);
					if (!ct[i].pairs[j].name) {
						FreeMem();
						return B_NO_MEMORY;
					}

					p += strlen(ct[i].pairs[j].name) + 1;

					memcpy(&(ct[i].pairs[j].type), p,
						sizeof(ct[i].pairs[j].type));
					if (swap)
						ct[i].pairs[j].type = __swap_int32(ct[i].pairs[j].type);
					p += sizeof(ct[i].pairs[j].type);

//+					if (*p) {
//+						ct[i].pairs[j].label = strdup(p);
//+						if (!ct[i].pairs[j].label) {
//+							FreeMem();
//+							return B_NO_MEMORY;
//+						}
//+
//+						p += strlen(ct[i].pairs[j].label);
//+					} 
//+					p += 1;								// skip the NULL char

				}
				p += sizeof(int32);			// skip termination of pairs list
			}
			p += sizeof(int32);				// skip termination ctypes list
#endif

			d++;
		}
	}
	
	if (has_value_info) {
		value_info	*v;
		c = 0;

		memcpy(&fValueCount, p, sizeof(fValueCount));
		if (swap)
			fValueCount = __swap_int16(fValueCount);
		p += sizeof(fValueCount);

		fValueInfo = (value_info *) malloc(fValueCount*sizeof(value_info));
		if (!fValueInfo) {
			fValueCount = 0;
			return B_NO_MEMORY;
		}
		memset(fValueInfo, 0, fValueCount * sizeof(value_info));

		v = fValueInfo;

		while (c++ < fValueCount) {

			// kind field
			memcpy(&(v->kind), p, sizeof(v->kind));
			if (swap)
				v->kind = (value_kind) __swap_int32(v->kind);
			p += sizeof(v->kind);

			// value field
			memcpy(&(v->value), p, sizeof(v->value));
			if (swap)
				v->value = __swap_int32(v->value);
			p += sizeof(v->value);

//+			PRINT(("value=%s\n", p));
			v->name = strdup(p);
			if (!v->name) {
				FreeMem();
				return B_NO_MEMORY;
			}

			p += strlen(v->name) + 1;

			if (*p) {
				v->usage = strdup(p);
				if (!v->usage) {
					FreeMem();
					return B_NO_MEMORY;
				}

				p += strlen(v->usage);
			}
			p += 1;							// skip the NULL char

			// extra_data field
			memcpy(&(v->extra_data), p, sizeof(v->extra_data));
			if (swap)
				v->extra_data = __swap_int32(v->extra_data);
			p += sizeof(v->extra_data);

			v++;
		}
	}

//+	PRINT(("Unflatten done. size=%d, psize=%d\n",size, p - buffer));

	return B_OK;
}

/*-------------------------------------------------------------*/

int32 BPropertyInfo::CountProperties() const
{
	return fPropCount;
}

/*-------------------------------------------------------------*/

int32 BPropertyInfo::CountValues() const
{
	return (int32) fValueCount;
}


/*-------------------------------------------------------------*/

const property_info *BPropertyInfo::Properties() const
{
	return fPropInfo;
}

/*-------------------------------------------------------------*/

const value_info *BPropertyInfo::Values() const
{
	return fValueInfo;
}

/*-------------------------------------------------------------*/

//
// DEPRECATED
//
// really returns a pointer to _oproperty_info_
//
const property_info *BPropertyInfo::PropertyInfo() const
{
	if (!fOldPropInfo && fPropInfo) {
		BPropertyInfo *THIS = const_cast<BPropertyInfo *>(this);
		// old code asking for data in old format
		THIS->fOldPropInfo = ConvertFromNew(fPropInfo);
		if (fOldPropInfo)
			THIS->fOldInHeap = true;
	}
	return (property_info *) fOldPropInfo;
}

/* ---------------------------------------------------------------- */

//
// DEPRECATED
//
// This third arg is reall a pointer to _oproperty_info_
//
bool BPropertyInfo::MatchCommand(uint32 what, int32 index, property_info *npi)
{
	_oproperty_info_	*pi = (_oproperty_info_ *) npi;

	if (!pi)
		return false;

	/*
	 If we're not on the last specifier then they only way to make a match
	 is with a property_info item that doesn't have any commands.
	*/
	if (index >= 1) {
		// A 0 in the first command is the "wild_card".
		if (pi->commands[0] == 0) {
			return true;
		}
		return false;
	}

	for (int i = 0; pi->commands[i] != 0; i++) {
		if (what == pi->commands[i])
			return true;
	}
	return false;
}

/* ---------------------------------------------------------------- */

//
// DEPRECATED
//
// This third arg is reall a pointer to _oproperty_info_
//
bool BPropertyInfo::MatchSpecifier(uint32 form, property_info *npi)
{
	_oproperty_info_	*pi = (_oproperty_info_ *) npi;

	if (!pi)
		return false;

	// a 0 for the first form implies that we should match everything
	if (pi->specifiers[0] == 0)
		return true;

	for (int i = 0; pi->specifiers[i] != 0; i++) {
		if (form == pi->specifiers[i])
			return true;
	}
	return false;
}

/* ---------------------------------------------------------------- */

bool BPropertyInfo::FindCommand(uint32 what, int32 index,
	property_info *pi)
{
	/*
	 If we're not on the last specifier then they only way to make a match
	 is with a property_info item that doesn't have any commands.
	*/
	if (index >= 1) {
		// A 0 in the first command is the "wild_card".
		if (pi->commands[0] == 0) {
//+			PRINT(("MatchCmd - special match (index=%d)\n", index));
			return true;
		}
		return false;
	}

	for (int i = 0; pi->commands[i] != 0; i++) {
//+		PRINT(("what=%.4s, cmd[%d]=%.4s\n",
//+			(char *) &what, i, (char*) &(pi->commands[i])));
		if (what == pi->commands[i])
			return true;
	}
	return false;
}

/* ---------------------------------------------------------------- */

bool BPropertyInfo::FindSpecifier(uint32 form, property_info *pi)
{
	// a 0 for the first form implies that we should match everything
	if (pi->specifiers[0] == 0)
		return true;

	for (int i = 0; pi->specifiers[i] != 0; i++) {
//+		PRINT(("form=%d, spec[%d]=%d\n", form, i, pi->specifiers[i]));
		if (form == pi->specifiers[i])
			return true;
	}
	return false;
}

/* ---------------------------------------------------------------- */

int32 BPropertyInfo::FindMatch(BMessage *msg, int32 index, BMessage *,
	int32 form, const char *prop, void *data) const
{
	int32			c = 0;
	property_info	*pi = fPropInfo;

	while (c++ < fPropCount) {
//+		PRINT(("prop[%d]=%s\n", c-1, pi->name));
		if (strcmp(pi->name, prop) == 0) {
			if (FindSpecifier(form, pi) &&
				FindCommand(msg->what, index, pi)) {
					if (data) {
						*((void**)data) = (void*) pi->extra_data;
					}
					return c-1;
			}
		}
		pi++;
	}

	if (index == 0) {
		c = 0;
		pi = fPropInfo;
		while (c++ < fPropCount) {
			if (strcmp(pi->name, prop) == 0) {
				if (FindSpecifier(form, pi) &&
					(pi->commands[0] == 0)) {
//+					PRINT(("!!! FindMatch, prop=%s, i=%d\n", prop, c-1));
					if (data) {
//+						*data = (void*) pi->extra_data;
						*((void**)data) = (void*) pi->extra_data;
					}
					return c-1;
				}
			}
			pi++;
		}
	}

	if (data) {
		data = (void*) 0;
	}
	return -1;
}

/* ---------------------------------------------------------------- */

BPropertyInfo::BPropertyInfo(const BPropertyInfo &)
	:	BFlattenable()
	{}

BPropertyInfo &BPropertyInfo::operator=(const BPropertyInfo &)
{
	return *this;
}

/* ---------------------------------------------------------------- */

void BPropertyInfo::_ReservedPropertyInfo1() {}
void BPropertyInfo::_ReservedPropertyInfo2() {}
void BPropertyInfo::_ReservedPropertyInfo3() {}
void BPropertyInfo::_ReservedPropertyInfo4() {}

#endif

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

struct MatchWhat : public UnaryPredicate<_ObserverWhatList> {
	// for searching the observer list
	MatchWhat(uint32 what)
		:	what(what)
		{}

	virtual int operator()(const _ObserverWhatList *item) const
		{
			if (what < item->what)
				return -1;
			else if (what > item->what)
				return 1;
			return 0;
		}
	
	static int Compare(const _ObserverWhatList *item1, const _ObserverWhatList *item2)
		{
			if (item1->what < item2->what)
				return -1;
			else if (item2->what > item1->what)
				return 1;
			return 0;
		}

	uint32 what;
};

_ObserverWhatList::_ObserverWhatList(uint32 what, BHandler *target)
	:	what(what),
		messengerObserverList(NULL),
		handlerObserverList(NULL)
{
	Add(target);
}

_ObserverWhatList::_ObserverWhatList(uint32 what, const BMessenger *target)
	:	what(what),
		messengerObserverList(new BObjectList<BMessenger>(5, true)),
		handlerObserverList(NULL)
{
	// the messenger list owns it's own copies and deletes them when
	// removed/deleted
	messengerObserverList->AddItem(new BMessenger(*target));
}

_ObserverWhatList::~_ObserverWhatList()
{
	delete messengerObserverList;
	delete handlerObserverList;
}

void 
_ObserverWhatList::Add(const BMessenger *target)
{
	if (!messengerObserverList)
		// lazily allocate the lists
		messengerObserverList = new BObjectList<BMessenger>(5, true);
	messengerObserverList->AddItem(new BMessenger(*target)); 
}

bool 
_ObserverWhatList::Remove(const BMessenger *target)
{
	if (!messengerObserverList)
		// we don't even have a list, can't remove
		return false;
		
	int32 count = messengerObserverList->CountItems();
	for (int32 index = 0; index < count; index++) {
		if (*messengerObserverList->ItemAt(index) == *target) {
			delete messengerObserverList->RemoveItemAt(index);
			return true;
		}
	}
	return false;
}

void 
_ObserverWhatList::Add(BHandler *target)
{
	BMessenger messengerTarget(target);
	if (messengerTarget.IsValid()) {
		if (!messengerObserverList)
			messengerObserverList = new BObjectList<BMessenger>(5, true);
		messengerObserverList->AddItem(new BMessenger(target));
	} else {
		if (!handlerObserverList)
			handlerObserverList = new BObjectList<BHandler>(5, false);
		handlerObserverList->AddItem(target);
	}
}

bool 
_ObserverWhatList::Remove(BHandler *target)
{	
	BMessenger tmp(target);
	if (tmp.IsValid()) {
		int32 count = messengerObserverList->CountItems();
		for (int32 index = 0; index < count; index++) {
			if (*messengerObserverList->ItemAt(index) == tmp) {
				delete messengerObserverList->RemoveItemAt(index);
				return true;
			}
		}
	}
	// not found in messenger list, try the handler list
	int32 count = handlerObserverList->CountItems();
	for (int32 index = 0; index < count; index++) {
		if (handlerObserverList->ItemAt(index) == target) {
			handlerObserverList->RemoveItemAt(index);
			return true;
		}
	}
	return false;
}

void 
_ObserverWhatList::Send(BMessage *message)
{
	if (handlerObserverList) {
		// traverse the handler list and pick out handlers that are attached
		// and can be targetted by a messenger
		int32 count = handlerObserverList->CountItems();
		for (int32 index = 0; index < count; ) {
			BHandler *target = handlerObserverList->ItemAt(index);
			// we have a problem here for targets that have never been attached
			// to a looper, didn't get a StopWatching call called on them and
			// got deleteded - their old memory will get unmapped and we will crash
			// here
			BMessenger messengerTarget(target);
			if (messengerTarget.IsValid()) {
				// handler can create a valid messenger,
				// migrate it to the messenger list, will get sent from there
				
				if (!messengerObserverList)
					messengerObserverList = new BObjectList<BMessenger>(5, true);
				messengerObserverList->AddItem(new BMessenger(target));
				handlerObserverList->RemoveItemAt(index);
				count--;
			} else
				// handler not yet attached, ignore the send for now
				index++;
		}
	}
	
	if (messengerObserverList) {
		// send notices to all the messenger-based targets
		int32 count = messengerObserverList->CountItems();
		for (int32 index = 0; index < count; ) {
			BMessenger *target = messengerObserverList->ItemAt(index);

			// before sending, check if target is still valid, remove it
			// if not
			bool validTarget = target->IsValid();
			if (validTarget && target->IsTargetLocal())
				validTarget &= target->Target(NULL) != NULL;
				
			if (validTarget) {
				target->SendMessage(message, (BHandler *)NULL, 1000000);
				index++;
			} else {
				// messenger is dead, get it out of the list
				delete messengerObserverList->RemoveItemAt(index);
				count--;
			}
		}
	}

}


void 
_ObserverList::SendNotices(uint32 what, const BMessage *message)
{
	ASSERT(what != B_OBSERVER_OBSERVE_ALL);
	_ObserverWhatList *list = FindIf(MatchWhat(what));

	if (!list && !allList)
		// no observers for this <what>
		return;

	// build the message to send
	BMessage *clone;
	if (message) {
		clone = new BMessage(*message);
		clone->AddInt32(B_OBSERVE_ORIGINAL_WHAT, message->what);
		clone->what = B_OBSERVER_NOTICE_CHANGE;
	} else
		clone = new BMessage(B_OBSERVER_NOTICE_CHANGE);
	clone->AddInt32(B_OBSERVE_WHAT_CHANGE, what);
	
	// send it to all interrested parties
	if (list)
		list->Send(clone);
	if (allList)
		allList->Send(clone);

	delete clone;
}

status_t 
_ObserverList::StartObserving(const BMessenger *target, uint32 what)
{
	_ObserverWhatList *list = FindIf(MatchWhat(what));
	if (!list) {
		list = new _ObserverWhatList(what, target);
		BinaryInsert(list, &MatchWhat::Compare);
		if (what == B_OBSERVER_OBSERVE_ALL && !allList)
			allList = list;
	} else 
		list->Add(target);

	return B_OK;
}

status_t 
_ObserverList::StopObserving(const BMessenger *target, uint32 what)
{
	_ObserverWhatList *list = FindIf(MatchWhat(what));
	if (list)
		list->Remove(target);
	
	return B_OK;
}

status_t 
_ObserverList::StartObserving(BHandler *target, uint32 what)
{
	_ObserverWhatList *list = FindIf(MatchWhat(what));
	if (!list) {
		list = new _ObserverWhatList(what, target);
		BinaryInsert(list, &MatchWhat::Compare);
		if (what == B_OBSERVER_OBSERVE_ALL && !allList)
			allList = list;
	} else 
		list->Add(target);
	
	return B_OK;
}

status_t 
_ObserverList::StopObserving(BHandler *target, uint32 what)
{
	_ObserverWhatList *list = FindIf(MatchWhat(what));
	if (!list)
		return B_BAD_HANDLER;
	
	if (!list->Remove(target))
		return B_BAD_HANDLER;
		
	return B_OK;
}

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT void
	#if __GNUC__
	_ReservedHandler1__8BHandler
	#elif __MWERKS__
	_ReservedHandler1__8BHandlerFv
	#endif
	(BHandler* This, uint32 what, const BMessage *msg)
	{
		This->BHandler::SendNotices(what, msg);
	}

}
#endif
