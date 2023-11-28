#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <support2/Atom.h>
#include <support2/Message.h>
#include <support2/String.h>
#include <support2/URL.h>
#include <support2/Vector.h>
#include <www2/Content.h>
#include <www2/Queue.h>

namespace B {
namespace WWW2 {

using namespace B::Support2;

class B::Support2::BHandler;

typedef int32 GroupID;

class Content;
class Protocol;

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

class Resource : public QueueEntry
{
	public:
		Resource(const BUrl&);
		~Resource();
		const BUrl& GetURL() const;
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
		Content::ptr GetContent() const;
		void SetGroupID(GroupID zone);
		GroupID GetGroupID() const;
		void SetContentFactory(ContentInstantiateHook);
		bool GetRefresh(BUrl &url, bigtime_t *time);
		void SetRefresh(BUrl &url, bigtime_t time);
		void SetReferrer(const BUrl &url);
		const BUrl& GetReferrer() const;

		void AcquireReference();
		void ReleaseReference();

		// Called when stop is pressed.  Stops load and sends an error.
		void Abort();

		// Create a new instance from the content and notify the listener
		// when it is created.  If the content type is not yet known, this
		// will queue up the request and send it when it is.
		void CreateContentInstance(BHandler *listener, uint32 id, const BMessage &userData,
		        uint32 flags, const BUrl &originalRequestURL);
		bool CancelRequest(uint32 id, BHandler *handler);

		void SetError(const char *error);

		void SetCachePolicy(CachePolicy policy);
		CachePolicy GetCachePolicy() const;
		status_t ReadFromStream(Protocol *stream, size_t contentLength);

		bigtime_t GetAge() const;
		bool MatchesType(const char *type);


		static void SetNotify(NotifyHook onIdle, NotifyHook onBusy);

		static void DumpResources();

	private:
		void HandleContentInitialized();
		ssize_t FeedContent(const void *data, size_t count, bool done);

		struct QueuedRequest
		{
			public:
				QueuedRequest();
				QueuedRequest(const QueuedRequest &);
				~QueuedRequest();
				QueuedRequest& operator =(const QueuedRequest &);
				void SetListener(BHandler *_listener);
				
				BHandler *Listener()
				{
					return listener;
				};
	
				uint32 id;
				BMessage userData;
				uint32 flags;
				BUrl originalRequestURL;
			private:
				BHandler *listener;
		};

		// accessed during resource cache lookup
		BUrl fURL;
		Resource *fHashNext;

		// accessed during Feed
		ResourceState fState;
		atom_ptr<Content> fContent;
		bool fContentInitialized;
		BNestedLocker fQLock;

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
		BVector<QueuedRequest> fQueuedCreateRequests;
		BString fLoadError;
		Protocol *fStream;
		ContentInstantiateHook fInstantiateContent;
		bigtime_t fCreated;
		BUrl fRefreshURL;
		bigtime_t fRefreshInterval;
		BUrl fReferrer;

		static int32 fPendingRequests;
		static NotifyHook fOnIdle;
		static NotifyHook fOnBusy;

		friend class ResourceCache;
};

} } // namespace B::WWW2

#endif
