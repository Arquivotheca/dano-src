#ifndef _RESOURCE_CACHE_H
#define _RESOURCE_CACHE_H

#include <Message.h>
#include <Locker.h>
#include <Binder.h>
#include <support/memadviser.h>
#include "Timer.h"
#include "Queue.h"
#include "Resource.h"
#include "SecurityManager.h"

class GHandler;

namespace Wagner {

class URL;
class Content;

const uint32 kResourceHashSize = 3079;

// Resource load flags
const uint32 LOAD_ON_ERROR = 1;
const uint32 FORCE_RELOAD = 2;
const uint32 PRIVATE_COPY = 4;
const uint32 NO_LOAD = 8;	// return instance only if cached, return error if not loaded.
const uint32 TERSE_HEADER = 16;
const uint32 RANDOM_ACCESS = 32;

const uint32 NEW_CONTENT_INSTANCE = 'newc';
	// NEW_CONTENT_MESSAGE fields
	//		URL			"URL"			URL that this referred to (uses URL::AddToMessage).
	//		uint32 		"id" 			Identifier that user attached to this
	//									 content instance
	//		void* 		"instance" Data specific to the instance

const uint32 CONTENT_INSTANCE_ERROR = 'errc';
	//	CONTENT_INSTANCE_ERROR fields
	//		char*		"url"			URL of instance
	//		uint32		"id"			User defined identifier
	//		char*		"error_msg"		Error message


const uint32 REQUEST_SEPARATOR = 'sepr';	// Gets inserted in the message queue
											// in order after the requests.


class ResourceCache {
public:
	ResourceCache();

//////////////////////REMOVE ME////////////////////////////////////////
	status_t NewContentInstance(const URL&, uint32 id, GHandler *listener, uint32 flags,
		const BMessage &userData, GroupID requestorsGroup, ContentInstantiateHook
		instantiate = NULL, bool _REMOVE_THIS_ = false);
///////////////////////////////////////////////////////////////////////

	status_t NewContentInstance(const URL&, uint32 id, GHandler *listener, uint32 flags,
		const BMessage &userData, GroupID requestorsGroup, ContentInstantiateHook
		instantiate, const char *mimeType);
	void EnqueueSeparator(GHandler *handler, const BMessage &msg);

	bool CancelRequest(const URL&, uint32 id, GHandler *listener);
	void CancelPendingMessages(BHandler *handler);

	void AbortAll();

	// used for debugging.
	bool _CheckLock() const;

	// Synchronize worker threads, clear out resources, prohibit further
	// loadings.  This is one of the trickier functions around.
	void Shutdown();
	
	void DumpStats();
	void DumpResources();

	// Attempt to free a certain amount of memory.  This is used by things like
	// flash picture viewers, where a picture can be large. 
	// This are kept for backward compatibility and can be considered deprecated.
	bool TryToReserveMemory(int32 size);
	void SoftenMemoryReservation(int32 size);
	
	// This removes everything in the cache.  Use it when you're about
	// to allocate a lot of memory (like RealPlayer).
	void EmptyCache();
	
	// Even dumps active resources.  Used for language changes.
	void InvalidateAll();

	void MakeResourceStale(const URL &url);

	void AdjustCacheSize();
	
private:
	void RemoveResource(Resource*);
	size_t Trim(long sizeToFree);
	static void LoadResource(void *data, size_t size);
	static status_t LoadResourceInternal(void *data, size_t size, bool loadErrorPage,
							const URL& url, BMessage* outErrors);
	status_t InternalNewContentInstance(const URL &url, uint32 id, GHandler *listener,
		uint32 flags, const BMessage &userData, GroupID requestGroup,
		ContentInstantiateHook instantiate, const char *mimeType, const URL &originalRequestURL);
	static void DeleteInstanceFromMessage(BMessage *response);
	void RedirectResource(const URL &url, Resource *resource, bool overrideSecurity,
		ContentInstantiateHook instantiate, const char *mimeType);
	static void AdjustHook(const BMessage *data);
	static size_t FreeHook(void *cookie, size_t sizeToFree, free_level level);

	Resource *fResourceHash[kResourceHashSize];


	// This lock protects:
	//	1. hash table
	//  2. resource queues.
	//  3. pending request queue on each resource
	//	4. Invariant that once the content is initialized for a resource,
	//	   requests never get added to the request queue.
	// ordering: <private instance lock>->resourceLock.
	//		(expect calls into Content, ContentInstace
	//	   to lock, don't hold resourceLock when you do it).
	BLocker fLock;
	bool fShutdown;
	int32 fMemoryUsage;

	// These queues contain resources in corresponding states
	Queue fLoadingResources;
	Queue fInactiveResources;

	static atom<Timer> fAdjustCacheTimer;
	bigtime_t fLastCacheAdjust;

	// stats
	int32 fRequestCount;
	int32 fHitCount;

	// stores the token returned by the memory adviser
	int32 fFreeHookToken;

	friend class Resource;
};

extern ResourceCache resourceCache;

}

// specify domain in "be:settings", all other strings indicate name/value pairs */
void get_settings(BMessage *msg, BMessage *resp);
void set_settings(BMessage *msg);

// Ewwwww, gross!
void get_proxy_server(char *out_server, int *out_port);
void set_proxy_server(const char *server, int port);
bool get_proxy_password(char *out_user, char *out_password);
void get_navigator_setting(const char *name, char *out_value, size_t value_len, const char *def_value);

extern BinderNode::property proxySettings;
extern BinderNode::property navigatorSettings;
extern BinderNode::property configSettings;

extern void InitWebSettings();

#endif
