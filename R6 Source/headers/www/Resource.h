#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <String.h>
#include <Message.h>
#include <Atom.h>
#include "Gehnaphore.h"
#include "URL.h"
#include "SmartArray.h"
#include "Queue.h"

class BDataIO;
class GHandler;
class Atom;

namespace Wagner {

typedef int32 GroupID;

class Content;

typedef Content* (*ContentInstantiateHook)(const char *contentType);

enum CachePolicy {
	CC_CACHE,
	CC_NO_CACHE,
	CC_NO_STORE
};

enum ResourceState {
	kStateLoading,
	kStateLoaded,
	kStateInactive,
	kStateAborted,
	kStateError,
	kStateExpired,
	kStateDeleting,
	kStateNew
};

typedef void (*NotifyHook)();

class Resource : public QueueEntry {
public:
	Resource(const URL&);
	~Resource();
	const URL& GetURL() const;
	void SetContentType(const char *type);

	// Content-Type: <MIME type>[; <attribute>=<value>]*
	//
	// GetContentType() returns the entire Content-Type value
	//     as set by SetContentType(), including any trailing parameters.
	// GetMIMEType() returns just the MIME type, in lowercase and with
	//     all whitespace stripped.
	const char* GetContentType() const;
	const char* GetMIMEType() const;

	size_t GetContentLength() const;
	void SetContentLength(size_t length);
	ResourceState GetState() const;
	void SetState(ResourceState state);
	Content* GetContent() const;
	void SetGroupID(GroupID zone);
	GroupID GetGroupID() const;
	void SetContentFactory(ContentInstantiateHook);
	bool GetRefresh(URL &url, bigtime_t *time);
	void SetRefresh(URL &url, bigtime_t time);
	void SetReferrer(const URL &url);
	const URL& GetReferrer() const;
	bool SetStream(BDataIO *stream);

	void AcquireReference();
	void ReleaseReference();

	// Called when stop is pressed.  Stops load and sends an error.
	void Abort();

	// Create a new instance from the content and notify the listener
	// when it is created.  If the content type is not yet known, this
	// will queue up the request and send it when it is.
	void CreateContentInstance(GHandler *listener, uint32 id, const BMessage &userData,
		uint32 flags, const URL &originalRequestURL);
	bool CancelRequest(uint32 id, GHandler *handler);
	
	void SetError(const char *error);

	void SetCachePolicy(CachePolicy policy);
	CachePolicy GetCachePolicy() const;
	status_t ReadFromStream(BDataIO *stream, size_t contentLength);

	bigtime_t GetAge() const;
	bool MatchesType(const char *type);


	static void SetNotify(NotifyHook onIdle, NotifyHook onBusy);
	
	static void DumpResources();
	
private:
	void HandleContentInitialized();
	ssize_t FeedContent(const void *data, size_t count, bool done);

	struct QueuedRequest {
	public:
		QueuedRequest();
		QueuedRequest(QueuedRequest &);
		~QueuedRequest();
		QueuedRequest& operator =(QueuedRequest &);
		void SetListener(GHandler *_listener);
		GHandler *Listener() { return listener; };

		uint32 id;
		BMessage userData;
		uint32 flags;
		URL originalRequestURL;
	private:
		GHandler *listener;
	};

	// accessed during resource cache lookup
	URL fURL;
	Resource *fHashNext;

	// accessed during Feed
	ResourceState fState;
	atom<Content> fContent;	
	bool fContentInitialized;
	Gehnaphore fQLock;

	// added for debugging
	void LockQueue();
	void UnlockQueue(); 

	bool fCached;
	Resource **fHashPrev;
	int32 fRefCount;
	BString fContentType;
	BString fMIMEType;
	size_t fContentLength;
	GroupID fGroupID;
	CachePolicy fCachePolicy;
	SmartArray<QueuedRequest> fQueuedCreateRequests;
	BString fLoadError;
	BDataIO *fStream;
	ContentInstantiateHook fInstantiateContent;
	bigtime_t fCreated;
	URL fRefreshURL;
	bigtime_t fRefreshInterval;
	URL fReferrer;

	static int32 fPendingRequests;
	static NotifyHook fOnIdle;
	static NotifyHook fOnBusy;

	friend class ResourceCache;
	friend class Atom;
};

}

#endif
