/*
	MailCacheProxy.h
*/

#ifndef MAIL_CACHE_PROXY_H
#define MAIL_CACHE_PROXY_H

#include <Binder.h>
#include "PartContainer.h"

namespace Wagner {

class MailCacheProxy {
	public:
								MailCacheProxy();
								MailCacheProxy(const char *user, const char *mailbox);
								~MailCacheProxy();
	
		bool					AddEntry(PartContainer &container);
		status_t				ResizeEntry(PartContainer &container);
		status_t				RemoveEntry(PartContainer &container);
		status_t				TouchEntry(PartContainer &container);
		bool					ContainsEntry(PartContainer &container);

	private:
		get_status_t			DoAction(const char *action, PartContainer &container);
		void					BuildArgList(const PartContainer &container, BinderNode::property_list &args);
		
		BString fUser;
		BString fMailbox;
		BinderNode::property fCacheNode;
		BinderNode::property fCacheReply;
};
}
#endif
// End of MailCacheProxy.h
