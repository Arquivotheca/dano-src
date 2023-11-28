#include <Autolock.h>
#include <Debug.h>
#include <StopWatch.h>
#include <string.h>
#include <Messenger.h>
#include "Content.h"
#include "ContentManager.h"
#include "DNSCache.h"
#include "Gehnaphore.h"
#include "parameters.h"
#include "PasswordManager.h"
#include "Protocol.h"
#include "ResourceCache.h"
#include "Resource.h"
#include "SecurityManager.h"
#include "StringBuffer.h"
#include "ThreadPool.h"
#include "Timer.h"
#include "SecurityManager.h"

using namespace Wagner;

const bigtime_t kMinAdjustInterval = 250000;

struct JobData {
	Resource *resource;
	uint32 flags;
	ContentInstantiateHook instantiate;
	char *mimeType;
};

atom<Timer> ResourceCache::fAdjustCacheTimer;

ResourceCache Wagner::resourceCache;
static ThreadPool workerThreadPool(0, kMaxWorkerThreads);
static BinderNode::property cacheSettings;

ResourceCache::ResourceCache()
	:	fLock("Resource Cache Lock"),
		fShutdown(false),
		fMemoryUsage(0),
		fLastCacheAdjust(0),
		fRequestCount(0),
		fHitCount(0),
		fFreeHookToken(0)
{
	memset(&fResourceHash, 0, kResourceHashSize * sizeof(Resource*));
	BMessage data;
	fAdjustCacheTimer = new Wagner::Timer();
	fAdjustCacheTimer->Start(AdjustHook, &data, kCacheResizeInterval, 0x7fffffff);

	// register FreeHook with the memory adviser
	fFreeHookToken= register_free_func(FreeHook, this);
}

void ResourceCache::Shutdown()
{
	if (fShutdown)
		return;
	
	fAdjustCacheTimer = NULL;
	fShutdown = true;

	// These make sure the worker threads aren't blocked before we wait
	// for them to exit.
	AbortAll();
	dnsCache.KillPendingRequests(true);

	// This will wait for all of the worker threads to finish.  Note that we
	// have released the resource cache lock to prevent deadlocks.  The threads
	// should immediately fail once they get the lock, as the fShutdown flag is set
	// to true.
	workerThreadPool.Shutdown();

	InvalidateAll();

	// unregister from memory adviser
	unregister_free_func(fFreeHookToken);

#if DEBUG_RESOURCE_LEAKS
	printf("Leaked resources ---------------------\n");
	Resource::DumpResources();
	printf("--------------------------------------\n");
#endif
}

void ResourceCache::AbortAll()
{
	BAutolock _lock(&fLock);

	//unblock any DNS requests, so Abort() can proceed quickly
	dnsCache.KillPendingRequests(false);

	for (;;) {
		Resource *resource = (Resource*) fLoadingResources.Dequeue();
		if (resource == 0)
			break;
		
		resource->Abort();
	}

	//restart DNS
	dnsCache.ResumeProcessingRequests();
}

status_t ResourceCache::NewContentInstance(const URL &url, uint32 id, GHandler *listener, uint32 flags,
	const BMessage &userData, GroupID requestorsGroupID, ContentInstantiateHook instantiate,
	bool )
{
#if DEBUG_CACHE
	StringBuffer tmp;
	tmp << url;
	printf("ResourceCache::NewContentInstance(\"%s\", %d, %p)\n", tmp.String(), id, listener);
#endif
	return InternalNewContentInstance(url, id, listener, flags, userData, requestorsGroupID, instantiate,
		NULL, url);
}

status_t ResourceCache::NewContentInstance(const URL &url, uint32 id, GHandler *listener,
	uint32 flags, const BMessage &userData, GroupID requestorsGroupID,
	ContentInstantiateHook instantiate, const char *mimeType)
{
#if DEBUG_CACHE
	StringBuffer tmp;
	tmp << url;
	printf("ResourceCache::NewContentInstance(\"%s\", %d, %p)\n", tmp.String(), id, listener);
#endif

	return InternalNewContentInstance(url, id, listener, flags, userData, requestorsGroupID, instantiate,
		mimeType, url);
}

status_t ResourceCache::InternalNewContentInstance(const URL &url, uint32 id, GHandler *listener,
	uint32 flags, const BMessage &userData, GroupID requestGroup,
	ContentInstantiateHook instantiate, const char *mimeType, const URL &originalRequestURL)
{
	if (!securityManager.CheckAccess(url, requestGroup)) {
		// Security violation
		StringBuffer urlBuf;
		urlBuf << url;
		BMessage msg(CONTENT_INSTANCE_ERROR);
		originalRequestURL.AddToMessage("URL",&msg);
		msg.AddString("url", urlBuf.String());
		msg.AddInt32("id", id);
		msg.AddMessage("user_data", &userData);
		msg.AddString("error_msg", "Security Violation");
		listener->PostMessage(msg);
		printf("Security Violation: Attempted access to url %s from group %s\n",
			urlBuf.String(), securityManager.GetGroupName(requestGroup));
		return B_OK;
	}

	if (!url.IsValid()) {
		StringBuffer urlBuf;
		urlBuf << originalRequestURL;
		BMessage msg(CONTENT_INSTANCE_ERROR);
		msg.AddInt32("id",id);
		originalRequestURL.AddToMessage("URL",&msg);
		msg.AddString("url", urlBuf.String());
		msg.AddString("error_msg", "Invalid request URL");
		msg.AddMessage("user_data", &userData);
		listener->PostMessage(msg);
			
		return B_OK;
	};

	Resource *resource = 0;

	fLock.Lock();

	if (fShutdown) {
		fLock.Unlock();
		return B_ERROR;		// This might need to send a notification
	}

	AdjustCacheSize();

	fRequestCount++;
	Resource **bucket = &fResourceHash[url.GenerateHash() % kResourceHashSize];
	for (resource = *bucket; resource; resource = resource->fHashNext)
		if (resource->GetURL() == url) {
			fHitCount++;
			break;
		}

	if (resource == 0 && (flags & NO_LOAD) != 0) {
		// This is no load, return an error if it couldn't be loaded.
		fLock.Unlock();
		return B_ERROR;
	}
	
	if (resource != 0 && (((flags & FORCE_RELOAD) != 0)
		|| resource->GetAge() > kResourceExpireAge
		|| (mimeType && mimeType[0] && !resource->MatchesType(mimeType)))) {
		// The caller has requested that this resource be reloaded,
		// or has performed a different post query, or this resource
		// is too old and needs to be dumped for a young trophy resource,
		// or the user has explicitly requested a content type and this
		// resource doesn't match that type.
		RemoveResource(resource);
		resource->AcquireReference();
		resource->SetState(kStateExpired);
		resource->ReleaseReference();
		resource = 0;
	}

	if (resource == 0 || (flags & PRIVATE_COPY) != 0) {
		// New resource
		resource = new Resource(url);
		resource->SetContentFactory(instantiate);
		if (mimeType) {
			resource->SetContentType(mimeType);
			if( resource->GetContent() == 0 ) {
				// Release references for each of these instances
				resource->SetError("unknown content type");
		
				// This will notify waiters and delete the resource
				resource->AcquireReference();
				resource->AcquireReference();
				resource->CreateContentInstance(listener, id, userData, flags, originalRequestURL);
				resource->SetState(kStateError);
				resource->ReleaseReference();
				fLock.Unlock();
				return B_ERROR;
			}
		}

		// Put resource into hash table.
		if ((flags & PRIVATE_COPY) == 0) {
			resource->fHashPrev = bucket;
			resource->fHashNext = *bucket;
			*bucket = resource;
			if (resource->fHashNext)
				resource->fHashNext->fHashPrev = &resource->fHashNext;
				
			resource->fCached = true;
		} else {
printf("private copy\n");
		}

		// Set the referrer if needed
		URL referrer;
		referrer.ExtractFromMessage("referrer", &userData);
		if (!referrer.IsValid())
			referrer.ExtractFromMessage("baseURL", &userData);

		if (strcmp(referrer.GetScheme(), "file") == 0)
			referrer.Reset();	// Don't send file's as referrers!

		resource->SetReferrer(referrer);

		// Acquire a reference on behalf of the loader thread
		resource->AcquireReference();

		JobData data;
		data.resource = resource;
		data.flags = flags;
		data.instantiate = instantiate;
		data.mimeType = mimeType ? strdup(mimeType) : 0;
		workerThreadPool.AddJob(LoadResource, (void*) &data, sizeof(data));
	}

	// Acquire a reference on behalf of the new content instance
	resource->AcquireReference();

	// Note that this may release the resource cache lock (but will re-lock it)
	// as a side effect to prevent a deadlock if it has to call into Content code.
	resource->CreateContentInstance(listener, id, userData, flags, originalRequestURL);

	fLock.Unlock();

	return B_OK;
}

void ResourceCache::EnqueueSeparator(GHandler *handler, const BMessage &msg)
{
	BMessage copy(msg);
	copy.what = REQUEST_SEPARATOR;
	handler->PostMessage(copy);
}

bool ResourceCache::CancelRequest(const URL &, uint32 id, GHandler *listener)
{
	// Since there might have been a redirection, we can't use the URL
	// to look up the resource.  Instead, step through resources that
	// are being loaded (the only case where a request would be pending) and
	// try to find one that can be cancelled.
	BAutolock _lock(&fLock);

#if DEBUG_CACHE		
	printf("ResourceCache::CancelRequest(%d, %p)...   ", id, listener);
#endif
	for (Resource *resource = (Resource*) fLoadingResources.Head();
		resource; resource = (Resource*) fLoadingResources.GetNext(resource)) {
		if (resource->CancelRequest(id, listener)) {
#if DEBUG_CACHE
			printf("Ok.\n");
#endif
			return true;
		}
	}

#if DEBUG_CACHE		
	printf("Failed.\n");
#endif
	return false;
}

void ResourceCache::CancelPendingMessages(BHandler *)
{
}

void ResourceCache::DeleteInstanceFromMessage(BMessage *response)
{
	ContentInstance *instance = 0;
	if (response->FindPointer("instance", (void**) &instance) >= B_OK)
		instance->DecRefs();
}

void ResourceCache::RedirectResource(const URL &redirectURL, Resource *resource, bool overrideSecurity, ContentInstantiateHook instantiate,
	const char *mimeType)
{
	resource->SetState(kStateExpired);
	BAutolock _lock(&resourceCache.fLock);
	if (resourceCache.fShutdown)
		return;
		
	resourceCache.RemoveResource(resource);

	GroupID customContentGroupID = securityManager.RegisterGroup("custom_content");
	// Now redirect all of the requests that were waiting on this
	// resource to the new one.
	while (resource->fQueuedCreateRequests.CountItems() != 0) {
		Resource::QueuedRequest request = resource->fQueuedCreateRequests.ItemAt(0);
		resource->fQueuedCreateRequests.RemoveItem(0L);
		resource->ReleaseReference();	// release instance reference
		resourceCache.InternalNewContentInstance(redirectURL, request.id,
			request.Listener(), request.flags, request.userData,
			overrideSecurity ? customContentGroupID : securityManager.GetGroupID(request
			.originalRequestURL), instantiate, mimeType, request.originalRequestURL);
	}
}

void ResourceCache::RemoveResource(Resource *resource)
{
	BAutolock _lock(&fLock);
	if (resource->fCached)
	{
		if(resource->fState == kStateInactive)
			fInactiveResources.RemoveEntry(resource);
				
		if (resource->fHashNext)
			resource->fHashNext->fHashPrev = resource->fHashPrev;
			
		*resource->fHashPrev = resource->fHashNext;
		resource->fCached = false;
	}
}

void ResourceCache::DumpStats()
{
#if PRINT_STATISTICS
	const int kMaxCount = 20;
	int cachedCount = 0;
	int counts[kMaxCount];
	for (int i = 0; i < kMaxCount; i++)
		counts[i] = 0;
	
	BAutolock _lock(&fLock);
	for (uint32 i = 0; i < kResourceHashSize; i++) {
		int bucketCount = 0;
		for (Resource *res = fResourceHash[i]; res; res = res->fHashNext) {
			cachedCount++;
			bucketCount++;
		}

		counts[bucketCount < kMaxCount ? bucketCount : kMaxCount]++;
	}

	printf("Resource Cache statistics:\n");
	printf("   Hits/Requests: %d/%d(%.2f%%)\n", fHitCount, fRequestCount,
		(float) fHitCount / fRequestCount * 100);
	printf("   Resource usage: %.1fk\n", (float) fMemoryUsage / 1024);
	printf("   Add-on memory: %.1fk\n", (float) ContentManager::Default().GetMemoryUsage() / 1024);
	printf("   Cached: %d\n", cachedCount);
	printf("   Inactive: %d\n", (int) fInactiveResources.CountItems());
	printf("   Loading: %d\n", (int) fLoadingResources.CountItems());
	printf("   ");
	for (int i = 1; i < kMaxCount; i++)
		if (counts[i] > 0)
			printf("%d:%d ", i, counts[i]);

	printf("\n");
#endif
}

void ResourceCache::DumpResources()
{
#if PRINT_STATISTICS
	for (uint32 i = 0; i < kResourceHashSize; i++) {
		if (fResourceHash[i]) {
			printf("Bucket %u: \n", (unsigned) i);
			for (Resource *res = fResourceHash[i]; res; res = res->fHashNext) {
				StringBuffer tmp;
				tmp << res->GetURL();
				printf("    %s(%d)\n", tmp.String(), res->fRefCount);
			}
		}
	}
	
	printf("\n");
#endif
}

bool ResourceCache::TryToReserveMemory(int32 size)
{
	/*
	 * just for backward compatibility -- remove
	 */
	return madv_reserve_memory(size, "Wagner::ResourCache");
/*
	BAutolock _lock(&fLock);
	system_info sinfo;
	get_system_info(&sinfo);
	if (sinfo.max_pages - sinfo.used_pages < (size + fProtectedMemory/2) / B_PAGE_SIZE) {
		rc= Trim(size + fProtectedMemory/2 - ((sinfo.max_pages - sinfo.used_pages) * B_PAGE_SIZE));
	}

	if (rc) {
		fProtectedMemory += size;
	}

	return rc;
*/
}

void
ResourceCache::SoftenMemoryReservation(int32 size)
{
	/*
	 * just for backward compatibility -- remove
	 */
	madv_finished_allocating(size);
/*
	BAutolock _lock(&fLock);
	fProtectedMemory -= size;

	if  (fProtectedMemory< 0) {
		fProtectedMemory= 0;
	}
*/
}

status_t ResourceCache::LoadResourceInternal(void *_data, size_t, bool loadErrorPage,
								const URL& url, BMessage* inOutParams)
{
	JobData *data = (JobData*) _data;
	ssize_t err = B_OK;
	URL redirectURL;
	Protocol *protocol = 0;
	char contentType[B_MIME_TYPE_LENGTH];
	bigtime_t delay;
	bool isRedirect;

	StringBuffer threadName;
	threadName << "Load ";
	url.AppendTo(threadName);
	rename_thread(find_thread(NULL), threadName.String());

	while (true) {
		protocol = Protocol::InstantiateProtocol(url.GetScheme());
		if (protocol == 0) {
			PRINT(("No loader for scheme \"%s\"\n", url.GetScheme()));
			err = B_BAD_VALUE;
			inOutParams->AddString(S_ERROR_TEMPLATE, "Errors/scheme.html");
			inOutParams->AddString(S_ERROR_SCHEME, url.GetScheme());
			goto finished;
		}
	
		if (!data->resource->SetStream(protocol)) {
			//this resource has already been aborted - bail
			goto finished;
		}

		err = protocol->Open(url, data->resource->GetReferrer(), inOutParams,
			data->flags & (loadErrorPage ? ~0 : ~LOAD_ON_ERROR));

		if (err == B_AUTHENTICATION_ERROR) {
			// Handle authentication.  This is kind of an inefficient way
			// to do it, as we always fail and retry.
			const char *challenge = "";
			inOutParams->FindString(S_CHALLENGE_STRING, &challenge);
			
			// Look up realm...
			BString user, password;
			if (passwordManager.GetPassword(url.GetHostName(), challenge, &user, &password)) {
				URL augmentedURL(url.GetScheme(), url.GetHostName(), url.GetPath(),
					url.GetPort(), url.GetFragment(), user.String(), password.String(),
					url.GetQuery(), url.GetQueryMethod());
	
				inOutParams->MakeEmpty();
				protocol = Protocol::InstantiateProtocol(augmentedURL.GetScheme());
				if (protocol == 0) {
					err = B_BAD_VALUE;
					inOutParams->AddString(S_ERROR_TEMPLATE, "Errors/scheme.html");
					inOutParams->AddString(S_ERROR_SCHEME, url.GetScheme());
					goto finished;
				}

				if (!data->resource->SetStream(protocol))
					goto finished;

				err = protocol->Open(augmentedURL, data->resource->GetReferrer(), inOutParams, data->flags
					& (loadErrorPage ? ~0 : ~LOAD_ON_ERROR));
			}
		}
		
		break;
	}

	if (err == B_AUTHENTICATION_ERROR) {
		inOutParams->RemoveName(S_ERROR_TEMPLATE);
		inOutParams->AddString(S_ERROR_TEMPLATE, "Errors/password.html");
		inOutParams->AddString(S_ERROR_MIME_TYPE, "text/html");
		goto finished;
	}

	if (err < B_OK)
		goto finished;
	
	data->resource->SetGroupID(securityManager.GetGroupID(url));
	data->resource->SetCachePolicy(protocol->GetCachePolicy());

	isRedirect = protocol->GetRedirectURL(redirectURL, &delay);
	if (isRedirect) {
		if (delay == 0) {
			// If this is a redirect with a zero timeout, handle it immediately
			resourceCache.RedirectResource(redirectURL, data->resource, false,
				data->instantiate, data->mimeType);
			goto finished;
		} else
			data->resource->SetRefresh(redirectURL, delay);
	}

	protocol->GetContentType(contentType, B_MIME_TYPE_LENGTH);
	data->resource->SetContentType(contentType);
	if (data->resource->GetContent() == 0) {
		PRINT(("No content for type \"%s\"\n", contentType));
		err = B_BAD_VALUE;
		inOutParams->AddString(S_ERROR_TEMPLATE, "Errors/content.html");
		inOutParams->AddString(S_ERROR_MIME_TYPE, contentType);
		goto finished;
	}
	
	data->resource->SetContentLength(protocol->GetContentLength());
	err = data->resource->ReadFromStream(protocol, data->resource->GetContentLength());
	if (err < B_OK) {
		inOutParams->AddString(S_ERROR_TEMPLATE, "Errors/io.html");
		inOutParams->AddString(S_ERROR_MESSAGE, strerror(err));
		inOutParams->AddString(S_ERROR_MIME_TYPE, contentType);
		goto finished;
	}

finished:
	if (protocol && err <= B_ERROR) {
		// Important.  Make sure protocol knows we gave up.  HTTP, for example,
		// will assume that you've read all the data for doing keepalive.
		//
		// Addendum: This is much less important now.  Http does a much better
		//   job of detecting when its Connection is in a bad state and doesn't
		//   reuse it in those cases.  Usually, you should be able to just
		//   delete your Http, and things should be fine.  Basically, Http just
		//   keeps track of whether somebody has Read() all the data that it
		//   expects to be waiting for us in the Connection.  No other Protocols
		//   that I can find use Abort() to do anything at all.  Still, I'm
		//   loath to remove this call, in case there are cases that it covers
		//   that I missed in Http.
		//       -- Laz, 2001-04-27
		protocol->Abort();
	}
							
	//clean up 'protocol'
	data->resource->SetStream(NULL);
	
	if( err < B_OK ) {
		StringBuffer buf;
		buf << url;
		inOutParams->AddString("requested_url", buf.String());

#if DEBUG
		PRINT(("error opening URL: "));
		inOutParams->PrintToStream();
#endif
	}
	
	return err < B_OK ? err : B_OK;
}

void ResourceCache::LoadResource(void *_data, size_t)
{
	JobData *data = (JobData*) _data;
	BMessage errorParams;
	
	status_t err = B_OK;
	
	if (data->resource->GetState() == kStateAborted)
		goto finished;

	err = LoadResourceInternal(data, 0, true, data->resource->GetURL(), &errorParams);
	if (err != B_OK) {
		if ((data->flags & LOAD_ON_ERROR) && data->resource->GetContent() == 0
			&& err != B_NO_CONTENT) {
			//
			// Load an internal error page
			//
			URL errorURL("file://$SCRIPTS/errorgen");
			
			const char  *name;
			uint32 type; 
			int32 count; 
			for (int32 i = 0; errorParams.GetInfo(B_STRING_TYPE, i, &name, &type, &count)
				== B_OK; i++) {
				const char* value;
				if (errorParams.FindString(name, &value) == B_OK)
					errorURL.AddQueryParameter(name, value);
			}
			
			errorParams.MakeEmpty();
			resourceCache.RedirectResource(errorURL, data->resource, true, 0, 0);
		} else {
			//
			// Just give up on this resource
			//
			resourceCache.RemoveResource(data->resource);
			data->resource->SetState(kStateError);
		}
	}

finished:
	free(data->mimeType);
	data->resource->ReleaseReference();	// release loader thread's reference
}

void ResourceCache::MakeResourceStale(const URL &url)
{
	BAutolock _lock(&fLock);
	if (fShutdown)
		return;
		
	Resource **bucket = &fResourceHash[url.GenerateHash() % kResourceHashSize];
	Resource *resource = 0;
	for (resource = *bucket; resource; resource = resource->fHashNext)
		if (resource->GetURL() == url) {
			resource->AcquireReference();
			break;
		}

	if (resource != 0) {
		RemoveResource(resource);
		resource->SetState(kStateExpired);
		resource->ReleaseReference();
	}
}

void ResourceCache::AdjustCacheSize()
{
	BAutolock _lock(&fLock);
	bigtime_t now = system_time();
	if (now - fLastCacheAdjust > kMinAdjustInterval) {
		fLastCacheAdjust = now;
		system_info sinfo;
		get_system_info(&sinfo);
		if (sinfo.max_pages - sinfo.used_pages < kFreeMemoryLowWater / B_PAGE_SIZE) {
			Trim(kFreeMemoryHighWater - ((sinfo.max_pages - sinfo.used_pages) * B_PAGE_SIZE));
		}
	}
}

void ResourceCache::AdjustHook(const BMessage *)
{
	resourceCache.AdjustCacheSize();
}

size_t ResourceCache::FreeHook(void *cookie, size_t sizeToFree, free_level level)
{
	BAutolock _lock(&((ResourceCache*)(cookie))->fLock);

	if(level== B_FREE_CACHED) {
		return ((ResourceCache*)(cookie))->Trim(sizeToFree);
	} else {
		return ((ResourceCache*)(cookie))->Trim(0x7fffffff);
	}
}

void ResourceCache::EmptyCache()
{
	BAutolock _lock(&fLock);
	Trim(0x7fffffff);
}

void ResourceCache::InvalidateAll()
{
	BAutolock _lock(&fLock);
	for (size_t bucket = 0; bucket < kResourceHashSize; bucket++) {
		Resource *resource = fResourceHash[bucket];
		while (resource) {
			Resource *next = resource->fHashNext;
			RemoveResource(resource);
			resource->SetState(kStateExpired);
			resource = next;
		}
	}
}

size_t ResourceCache::Trim(long sizeToFree)
{
	// Now start pruning unneeded resources.
	long trimmed= 0;
	Resource *resource = (Resource*) fInactiveResources.Dequeue();
	
	while ((trimmed < sizeToFree) && resource)
	{
#if DEBUG_CACHE
		StringBuffer tmp;
		tmp << resource->GetURL();
		printf("Purging url from cache \"%s\"\n", tmp.String());
#endif
		if(resource->fRefCount == 0)
		{
			RemoveResource(resource);
			if (resource->GetContent()) {
				trimmed+= resource->GetContent()->GetMemoryUsage();
				atomic_add(&fMemoryUsage, -resource->GetContent()->GetMemoryUsage());
			}
		
			delete resource;
		}
		//This resource should not be here because there is still a reference
		//being held on it.  Lets set it back to a loaded state and leave it
		//out of the Inactive Queue
		else
		{
#if DEBUG_CACHE
		StringBuffer tmp;
		tmp << resource->GetURL();
		printf("Referenced resource found in the inactive queue \"%s\"\n", tmp.String());
#endif
			resource->SetState(kStateLoaded);
		}
			
		resource = (Resource*) fInactiveResources.Dequeue();
	}

	// Now prune out any add-ons that are no longer needed.
	ContentManager::Default().PruneAddOnTime(15);

	return trimmed;
}

bool ResourceCache::_CheckLock() const
{
	return fLock.IsLocked();
}
