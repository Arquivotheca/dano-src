/*
	PartContainer.h
*/
#ifndef PART_CONTAINER_H
#define PART_CONTAINER_H

#include <Resource.h>
#include <String.h>
#include <URL.h>

namespace Wagner {

class PartContainer {
	public:
								PartContainer();
								PartContainer(const char *mailbox, const char *uid, const char *part);
								~PartContainer();

		
		void					Init(const char *mailbox = B_EMPTY_STRING, const char *uid = B_EMPTY_STRING, const char *part = B_EMPTY_STRING);
		bool					IsValid();
		
		void					SetUser(const char *user);
		void					SetMailbox(const char *mailbox);
		void					SetUid(const char *uid);
		void					SetPart(const char *part);
		void					SetSize(int size);
		void					SetCache(bool cache);
		void					SetFlagRead(bool flag);
		void					SetIsMainPart(bool main);
		void					SetBypassRedirectServer(bool bypass);
		
		void					SetRedirectUrl(const URL &url);
		void					SetContentType(const char *type);
		void					SetCachePolicy(CachePolicy cachePolicy);

		void					SetRedirectUrlPtr(URL *ptr);
		void					SetContentTypePtr(BString *ptr);
		void					SetCachePolicyPtr(CachePolicy *ptr);
		
		
		const char *			User() const;
		const char *			Mailbox() const;
		const char *			Uid() const;
		const char *			Part() const;
		int						Size() const;
		bool					Cache() const;
		bool					FlagRead() const;
		bool					IsMainPart() const;
		bool					BypassRedirectServer() const;
		
		URL *					RedirectUrlPtr();
		BString *				ContentTypePtr();
		CachePolicy *			CachePolicyPtr();	

	private:
								// Don't implement, you can't copy a PartContainer.
								PartContainer(const PartContainer &inContainer);
		PartContainer &		operator=(const PartContainer &inContainer);

		BString fUser;
		BString fMailbox;
		BString fUid;
		BString fPart;
		int fSize;
		bool fCache;
		bool fFlagRead;
		bool fIsMainPart;
		bool fBypassRedirectServer;
		URL *fRedirectUrl;
		BString *fContentType;
		CachePolicy *fCachePolicy;
};

}
#endif
// End of PartContainer.h
