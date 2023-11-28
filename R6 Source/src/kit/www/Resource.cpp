#include <Autolock.h>
#include <Debug.h>
#include <Message.h>
#include <string.h>
#include <ctype.h>
#include "Resource.h"
#include "ResourceCache.h"
#include "URL.h"
#include "Content.h"
#include "Protocol.h"
#include "parameters.h"
#include "StringBuffer.h"

using namespace Wagner;

//#define DEBUG_RESOURCE_LEAKS 1

static int32 QueuedRequestRefID;
int32 Resource::fPendingRequests = 0;
NotifyHook Resource::fOnIdle = (NotifyHook) 0;
NotifyHook Resource::fOnBusy = (NotifyHook) 0;

#if DEBUG_RESOURCE_LEAKS
	#include <set>

	namespace Wagner {
		static BLocker resource_access;
		static set<Resource*> resources;
	};
#endif


Resource::QueuedRequest::QueuedRequest()
{
	listener = NULL;
};

Resource::QueuedRequest::~QueuedRequest()
{
	if (listener) listener->DecRefs(&QueuedRequestRefID);
	listener = (GHandler*)0xDEADBEEF;
};

void Resource::QueuedRequest::SetListener(GHandler *_listener)
{
	if (_listener) _listener->IncRefs(&QueuedRequestRefID);
	if (listener) listener->DecRefs(&QueuedRequestRefID);
	listener = _listener;
}

Resource::QueuedRequest & Resource::QueuedRequest::operator=(Resource::QueuedRequest &r)
{
	if (r.listener) r.listener->IncRefs(&QueuedRequestRefID);
	if (listener) listener->DecRefs(&QueuedRequestRefID);
	listener = r.listener;

	id = r.id;
	userData = r.userData;
	flags = r.flags;
	originalRequestURL = r.originalRequestURL;
	return *this;
}

Resource::QueuedRequest::QueuedRequest(Resource::QueuedRequest &r)
{
	listener = NULL;
	*this = r;
};

Resource::Resource(const URL &url)
	:
		fURL(url),
		fHashNext(0),
		fState(kStateNew),
		fContent(0),
		fContentInitialized(false),
		fCached(false),
		fHashPrev(0),
		fRefCount(0),
		fContentLength(0),
		fGroupID(-1),
		fCachePolicy(CC_CACHE),
		fQueuedCreateRequests(5),
		fStream(0),
		fInstantiateContent(0),
		fCreated(system_time())
{
	SetState(kStateLoading);	// Add to queue so we can cancel it.

#if DEBUG_RESOURCE_LEAKS
	BAutolock l(&resource_access);
	resources.insert(this);
#endif
}

Resource::~Resource()
{
	SetState(kStateDeleting);	// This is important to remove the resource
									// from its queue if it is on one.

#if DEBUG_RESOURCE_LEAKS
	BAutolock l(&resource_access);
	resources.erase(resources.find(this));
#endif
}

const URL& Resource::GetURL() const
{
	return fURL;
}

const char* Resource::GetContentType() const
{
	if (fContentType.Length() != 0)
		return fContentType.String();
	else
		return fMIMEType.String();
}

const char* Resource::GetMIMEType() const
{
	return fMIMEType.String();
}

size_t Resource::GetContentLength() const
{
	return fContentLength;
}

ssize_t Resource::FeedContent(const void *data, size_t count, bool done)
{
	// Notify the writer that it should stop.
	if (fState == kStateAborted)
		return B_ERROR;

	ssize_t written = fContent->Feed(data, count, done);
	if (done && written == (ssize_t) count) {
		// End of stream. Ready or not, time to instantiate.
		SetState(kStateLoaded);	// Set ourselves as loaded if we're finished.
		if (!fContentInitialized)
			HandleContentInitialized();
	} else if (!fContentInitialized && fContent->IsInitialized())
		HandleContentInitialized();

	return written;
}

void Resource::AcquireReference()
{
	BAutolock _lock(&resourceCache.fLock);
	if (fRefCount++ == 0 && fState == kStateInactive)
		SetState(kStateLoaded);
}

void Resource::ReleaseReference()
{
	BAutolock _lock(&resourceCache.fLock);
	if (--fRefCount == 0) {
		if (fState == kStateLoading) {
			Abort();
		}
		if (fState == kStateExpired || fState == kStateAborted ||
		    fState == kStateError || !fCached)
			delete this;	// This resource was already abandoned, delete it
		else if (fState == kStateLoaded)
			SetState(kStateInactive);
		else {
printf("************************argh\n");
		}
	}
}

void Resource::Abort()
{
	if (GetState() == kStateLoading) {
		SetState(kStateAborted);
		Protocol *protocol = dynamic_cast<Protocol*>(fStream);
		if (protocol)
			protocol->Abort();
	}
}

void Resource::SetContentType(const char *type)
{
	// If the content type is already set, don't replace.  This
	// is used for overriding content types.
	if (fMIMEType.Length() != 0)
		return;

	// Copy the mime type, switching to lowercase and stripping extra parameters off.
	char cleanMime[B_MIME_TYPE_LENGTH];
	int len = 0;
	const char *src = type;
	char *dest = cleanMime;
	while (*src && !isspace(*src) && *src != ';' && ++len < B_MIME_TYPE_LENGTH)
		if (*src >= 'A' && *src <= 'Z')
			*dest++ = *src++ - ('A' - 'a');
		else
			*dest++ = *src++;
	*dest = '\0';

	fMIMEType = cleanMime;
	if (strcmp(type, cleanMime) != 0)
		fContentType = type;
	else
		fContentType = "";

	if (fInstantiateContent)
		fContent = (*fInstantiateContent)(cleanMime);
	else
		fContent = Content::InstantiateContent(cleanMime, GetURL().GetExtension());

	if (fContent != 0)
		fContent->SetResource(this);
	else {
		// Couldn't instantiate content, so we still don't have a type.
		fContentType = "";
		fMIMEType = "";
	}
}

//
//	The content is ready to begin producing instances, handle queued up requests
//
void Resource::HandleContentInitialized()
{
	// This looks strange, but it ensures that the flag can't change between
	// the time that CreateContentInstance checks the flag and adds to
	// fQueuedCreateRequests (which would orphan requests).  After this, no other
	// threads will touch fQueuedCreateRequests, so no locking is needed for it.
	resourceCache.fLock.Lock();
	fContentInitialized = true;
	resourceCache.fLock.Unlock();

	LockQueue();
	SmartArray<QueuedRequest> createRequests(fQueuedCreateRequests);
	fQueuedCreateRequests.MakeEmpty();
	UnlockQueue();
	
	QueuedRequest request;
	int requestCount = createRequests.CountItems();
	for (int requestIndex = 0; requestIndex < requestCount; requestIndex++) {
		request = createRequests[requestIndex];
		bool failed = false;
		if (!(request.flags & LOAD_ON_ERROR) && fLoadError.Length() > 0) {
			// This user doesn't want an error page, and this is one.
			// send an error.
			failed = true;
		} else {
			// Create a new content object.
			atom<ContentInstance> instance;
			ContentInstance *instanceData;
			if (fContent->NewInstance(&instanceData, request.Listener(), request.userData) < 0)
				failed = true;
			else {
				instance = instanceData;
				instanceData->SetID(request.id);
		
#if DEBUG_CACHE
				printf("HandleContentInitialized: notify new content instance %d  handler %p\n",
					request.id, request.Listener());
				request.originalRequestURL.PrintToStream();
#endif
				BMessage msg(NEW_CONTENT_INSTANCE);
				request.originalRequestURL.AddToMessage("URL", &msg);
				msg.AddInt32("id", request.id);
				msg.AddMessage("user_data", &request.userData);
				msg.AddAtom("instance", (BAtom*)instance);
				request.Listener()->PostMessage(msg);
			}
		}

		if (failed) {
			BMessage msg(CONTENT_INSTANCE_ERROR);
			request.originalRequestURL.AddToMessage("URL",&msg);
			msg.AddInt32("id", request.id);
			msg.AddMessage("user_data", &request.userData);
			msg.AddString("error_msg", fLoadError.String());
			request.Listener()->PostMessage(msg);
			ReleaseReference();		// Release reference instance would have held
		}
	}
}

//
//	Note: an important side effect of this method is that when it has exited, the
//  reference will either be released, or passed to a content object.
//	This gets called with the resource cache lock held.
//
void Resource::CreateContentInstance(GHandler *listener, uint32 id, const BMessage &userData,
	uint32 flags, const URL &originalRequestURL)
{
	if (fContentInitialized) {
		if (!(flags & LOAD_ON_ERROR) && fLoadError.Length() > 0) {
			// The user doesn't want an error page.  Send an error.
			BMessage msg(CONTENT_INSTANCE_ERROR);
			originalRequestURL.AddToMessage("URL",&msg);
			msg.AddInt32("id", id);
			msg.AddMessage("user_data", &userData);
			msg.AddString("error_msg", fLoadError.String());
			listener->PostMessage(msg);

			ReleaseReference();	// Release reference of this instance
		} else {
			// Create a new instance.  After we call GetContent(), we've passed
			// ownership of the reference to the content.  It will release it when
			// the content gets deleted.
			
			// unroll resource cache lock.  The prevents a deadlock, because
			// the resource cache lock always gets ordered after content locks.
			int32 recursion = 0;
			while (resourceCache.fLock.IsLocked()) {
				recursion++;
				resourceCache.fLock.Unlock();
			}
				
			atom<ContentInstance> instance;
			ContentInstance *instanceData;
			if (fContent->NewInstance(&instanceData, listener, userData) == B_OK) {
				instance = instanceData;
				if (instance) instance->SetID(id);
			};

			while (recursion-- > 0)
				resourceCache.fLock.Lock();

#if DEBUG_CACHE
			printf("CreateContentInstance: notify new content instance %d, listener %p\n",
				id, listener);
			originalRequestURL.PrintToStream();
#endif

			BMessage msg(NEW_CONTENT_INSTANCE);
			originalRequestURL.AddToMessage("URL",&msg);
			msg.AddInt32("id", id);
			msg.AddMessage("user_data", &userData);
			if (instance) msg.AddAtom("instance", (BAtom*)instance);
			else {
				msg.AddString("error_msg", "Error creating instance");
				msg.what = CONTENT_INSTANCE_ERROR;
			};
			listener->PostMessage(msg);
			if (msg.what == CONTENT_INSTANCE_ERROR) ReleaseReference();
		}
	} else {
		LockQueue();
		QueuedRequest & request = fQueuedCreateRequests[fQueuedCreateRequests.AddItem()];
		request.SetListener(listener);
		request.id = id;
		request.userData = userData;
		request.flags = flags;
		request.originalRequestURL = originalRequestURL;
		UnlockQueue();
	}
}

void Resource::SetError(const char *error)
{
	fLoadError = error;
}

void Resource::SetContentLength(size_t length)
{
	fContentLength = length;
}

ResourceState Resource::GetState() const
{
	return fState;
}

void Resource::SetState(ResourceState state)
{
	bool dispatcherror = false;
	{ // limited scope for this BAutolock:
	BAutolock _lock(&resourceCache.fLock);
	if (state == fState)
		return;

#if DEBUG_CACHE
	// Remove this resource from its old queue
	StringBuffer tmp;
	tmp << GetURL();
	const char *kStateStrings[] = {
		"kStateLoading",
		"kStateLoaded",
		"kStateInactive",
		"kStateAborted",
		"kStateError",
		"kStateExpired",
		"kStateDeleting",
		"kStateNew"
	};
	printf("resource \"%s\" SetState(%s)\n", tmp.String(), kStateStrings[state]);
#endif

	switch (fState) {
	case kStateLoading:
		if (--fPendingRequests == 0 && fOnIdle)
			(*fOnIdle)();
		resourceCache.fLoadingResources.RemoveEntry(this);
		break;
	case kStateInactive:
		if(fCached)	
			resourceCache.fInactiveResources.RemoveEntry(this);
		break;
	case kStateExpired:
	case kStateAborted:
		// Cannot change a resources state from EXPIRED to anything else.
		// Since it's no longer in the hash table in this situation, it
		// would be leaked when the last reference got released.
		return;
	default:
		;
	}

	fState = state;

	// Add it to its new queue
	switch (fState) {
	case kStateLoading:
		if (fPendingRequests++ == 0 && fOnBusy)
			(*fOnBusy)();
		resourceCache.fLoadingResources.Enqueue(this);
		break;
	case kStateInactive:
		if(fCached)	
			resourceCache.fInactiveResources.Enqueue(this);
		break;
	case kStateAborted:
	case kStateError: {
		if (fCached)
			resourceCache.RemoveResource(this);
		dispatcherror = true;

	}

	// Falls through...

	case kStateExpired:
		if (fRefCount == 0)
			delete this;

		break;
		
	default:
		;
	}
	
	} // limit scope of BAutolock

	if(dispatcherror)
	{
		// Dispatch requests with the resourcecache UNlocked.
		// This is important to prevent deadlocks, since otherwise
		// the PostMessage() call will try to lock a Gehnaphore which
		// may be locked by a thread that is currently trying to lock
		// the resourcecache (as a result of a CleanUp() call for example).
		BMessage msg(CONTENT_INSTANCE_ERROR);
		msg.AddString("error_msg", fLoadError.String());

		LockQueue();
		while (fQueuedCreateRequests.CountItems() > 0) {
			QueuedRequest &request = fQueuedCreateRequests[0];
			// Release reference that content instance would have held.
			// Don't call ReleaseReference because it would call back
			// into set state.  The kStateExpired case will make sure
			// it gets cleaned up.
			fRefCount--;

			BMessage sendMessage(msg);

			request.originalRequestURL.AddToMessage("URL",&sendMessage);
			sendMessage.AddMessage("user_data", &request.userData);
			sendMessage.AddInt32("id", request.id);
	
			fQueuedCreateRequests[0].Listener()->PostMessage(sendMessage);
			fQueuedCreateRequests.RemoveItem(0);
		}

		UnlockQueue();
	}
}

Content* Resource::GetContent() const
{
	return fContent;
}

void Resource::SetGroupID(GroupID groupID)
{
	fGroupID = groupID;
}

GroupID Resource::GetGroupID() const
{
	return fGroupID;
}

void Resource::SetContentFactory(ContentInstantiateHook hook)
{
	fInstantiateContent = hook;
}

bool Resource::GetRefresh(URL &url, bigtime_t *time)
{
	if (!fRefreshURL.IsValid())
		return false;
	url = fRefreshURL;
	*time = fRefreshInterval;
	return true;
}

void Resource::SetRefresh(URL &url, bigtime_t time)
{
	fRefreshURL = url;
	fRefreshInterval = time;
}

void Resource::SetReferrer(const URL &url)
{
	fReferrer = url;
}

const URL& Resource::GetReferrer() const
{
	return fReferrer;
}

bool Resource::SetStream(BDataIO *stream)
{
	BAutolock _lock(resourceCache.fLock);
	
	//delete any previous stream
	delete fStream;

	if (GetState() != kStateLoading) {
		//this Resource's load has been aborted - don't bother
		fStream = NULL;
		return false;
	}

	//remember out new stream
	fStream = stream;
	return true;
}

bool Resource::CancelRequest(uint32 id, GHandler *listener)
{
	LockQueue();

	bool removed = false;
	for (int i = fQueuedCreateRequests.CountItems() - 1; i >= 0; i--)
		if (fQueuedCreateRequests.ItemAt(i).id == id &&
			fQueuedCreateRequests.ItemAt(i).Listener() == listener) {
			fQueuedCreateRequests.RemoveItem(i);
			removed = true;
			ReleaseReference();		// Release reference that would have been for ContentInstance
			break;
		}

	UnlockQueue();

	return removed;
}

CachePolicy Resource::GetCachePolicy() const
{
	return fCachePolicy;
}

void Resource::SetCachePolicy(CachePolicy cachePolicy)
{
	// Note: you can't set a resource to any other cache policy
	// after it is CC_NO_CACHE...

	fCachePolicy = cachePolicy;
	if (cachePolicy == CC_NO_CACHE) {
		BAutolock _lock(&resourceCache.fLock);
		if (resourceCache.fShutdown)
			return;
			
		resourceCache.RemoveResource(this);
	}
}

status_t Resource::ReadFromStream(BDataIO *stream, size_t contentLength)
{
	// Read the file data
	char buf[kReadBufferSize];
	size_t totalWritten = 0;
	unsigned offset = 0;
	bool readAllData = false;
	bool wantsMore = true;

	while (totalWritten < contentLength && GetState() != kStateAborted) {
		//	Read more data from the stream if needed
		ssize_t amountRead = 0;
		if (!readAllData && wantsMore && offset < kReadBufferSize) {
			amountRead = stream->Read(buf + offset, kReadBufferSize - offset);
			if (amountRead <= 0) {
				readAllData = true;
				amountRead = 0;
			} else if (amountRead + totalWritten >= contentLength)
				readAllData = true;
		}

		//	Terminate if nothing is left to do
		if (readAllData && offset == 0 && amountRead == 0) {
			FeedContent(buf, 0, true);	// notify reader that we're done
			SetContentLength(totalWritten);
			break;
		}

		//	Send data to the content handler
		ssize_t amountWritten = FeedContent(buf, amountRead + offset, readAllData);

		if (amountWritten == B_FINISH_STREAM || (amountWritten == B_ERRORS_END + 3 /* for compatibility */)) {
			Protocol *protocol = dynamic_cast<Protocol*>(stream);
			if (protocol)
				protocol->Abort();
				
			readAllData = true;
			break;
		}

		if (amountWritten < 0) {
			PRINT(("Aborted load\n"));
			break;	// Aborted load... the resource object has gone away.
		}

		if (amountWritten > amountRead + (ssize_t) offset)
			PRINT(("WARNING: content handler for resource returned %d bytes "
				"written, although I only sent %d\n", (int) amountWritten,
				(int) amountRead));

		//	Update counts
		totalWritten += amountWritten;
		if (amountWritten == 0) {
			offset += amountRead;
			wantsMore = true;
			if (readAllData || offset == kReadBufferSize) {
				PRINT(("WARNING: insatiable content handler.\n"));
				SetState(kStateAborted);
				return B_ERROR;
				// Out of luck, I haven't got any more data for you.
			}
		} else if (amountWritten < amountRead + (ssize_t) offset) {
			// Partial read.  The resource/content object will expect more
			// data on the next pass.
			memcpy(buf, buf + amountWritten, (offset + amountRead) - amountWritten);			
			offset += amountRead - amountWritten;
			wantsMore = false;
		} else {
			offset = 0;
			wantsMore = true;
		}

		//	Check for aborts
		if (fState == kStateAborted)
			return B_ERROR;
		
		if (!readAllData
			&& fContent->IsInitialized()
			&& fContent->CountInstances() == 0) {
			//	the user has navigated to another page before this
			//	resource has loaded.  It is no longer displayed anywhere,
			//	so mark it invalid.
			PRINT(("Aborting resource load\n"));
			BAutolock _lock(&resourceCache.fLock);
			if (fState != kStateAborted)
				SetState(kStateAborted);

			return B_ERROR;
		}
	}
	// Handle this special case
	if (GetState() == kStateAborted) {
		FeedContent(0, 0, true);
		return B_ERROR;
	} else if (contentLength == 0) {
		FeedContent(0, 0, true);
		SetState(kStateLoaded);
	} else if (readAllData) {
		atomic_add(&resourceCache.fMemoryUsage, fContent->GetMemoryUsage());
		SetState(kStateLoaded);
	} else {
		SetState(kStateAborted);
		return B_ERROR;
	}

	return B_OK;
}

bigtime_t Resource::GetAge() const
{
	return system_time() - fCreated;
}

bool Resource::MatchesType(const char *type)
{
	const char *src = GetMIMEType();
	while (*src && (*type || *type != ';')) {
		if (*src != tolower(*type))
			return false;

		src++;
		type++;
	}
	
	return true;
}

void Resource::SetNotify(NotifyHook onIdle, NotifyHook onBusy)
{
	fOnIdle = onIdle;
	fOnBusy = onBusy;
}

#if DEBUG_RESOURCE_LEAKS

const char *state_to_string(ResourceState state)
{
	const char *kStateStrings[] = {"Loading", "Loaded", "Inactive", "Aborted",
		"Error", "Expired", "Deleting"};
	if (state > sizeof(kStateStrings) / sizeof(char*))
		return "-- Bad State --";
	
	return kStateStrings[state];
}

void Resource::DumpResources()
{
	BAutolock l(&resource_access);
	printf("Addr         Refcnt        Type         State    #Inst  Age  Cached  URL\n");
	for(set<Resource*>::iterator i = resources.begin(); i != resources.end();
		i++) {
		Resource* r = *i;
		StringBuffer tmp;
		tmp << r->GetURL();
		printf("%p %8d %16s %8s %8d %4.2fs %c       %s\n", r, r->fRefCount, r->GetContentType(),
			state_to_string(r->fState), r->fContent
			? r->fContent->CountInstances() : -1, (double) r->GetAge() / 1000000,
			r->fCached ? 'y' : 'n', tmp.String());
	}
}

#else

void Resource::DumpResources()
{
}

#endif

void Resource::LockQueue()
{
	fQLock.Lock();
}

void Resource::UnlockQueue()
{
	fQLock.Unlock();
}
