/*
	MailCacheProxy.cpp
*/
#include "MailCacheProxy.h"
#include "MailDebug.h"

using namespace Wagner;

MailCacheProxy::MailCacheProxy()
	:	fUser(B_EMPTY_STRING),
		fMailbox(B_EMPTY_STRING)
{
	fCacheNode = BinderNode::Root()["service"]["mail"]["cache"];
	// ** Note: 'fCacheReply' is never actually used in this class.
	//			However, it's important that it not be left out. The
	//			MailCacheNode::ReadProperty requires that this
	//			parameter not be 'undefined'. It has to do with the
	//			Binder's ability to properly return the correct error
	//			code in get_status_t.
}

MailCacheProxy::MailCacheProxy(const char *user, const char *mailbox)
	:	fUser(user),
		fMailbox(mailbox)
{

}

MailCacheProxy::~MailCacheProxy()
{

}

bool MailCacheProxy::AddEntry(PartContainer &container)
{
	get_status_t result = DoAction("AddEntry", container);
	return (result.error == B_OK);
}

status_t MailCacheProxy::ResizeEntry(PartContainer &container)
{
	return DoAction("ResizeEntry", container);
}

status_t MailCacheProxy::RemoveEntry(PartContainer &container)
{
	return DoAction("RemoveEntry", container);
}

status_t MailCacheProxy::TouchEntry(PartContainer &container)
{
	return DoAction("TouchEntry", container);
}

bool MailCacheProxy::ContainsEntry(PartContainer &container)
{
	get_status_t result = DoAction("ContainsEntry", container);
	return (result.error == B_OK);
}

get_status_t MailCacheProxy::DoAction(const char *action, PartContainer &container)
{
	BinderNode::property_list args;
	BuildArgList(container, args);
	
	get_status_t result = fCacheNode->GetProperty(action, fCacheReply, args);
	return result;
}

void MailCacheProxy::BuildArgList(const PartContainer &container, BinderNode::property_list &args)
{
	args.AddItem(new BinderNode::property(container.User()));
	args.AddItem(new BinderNode::property(container.Mailbox()));
	args.AddItem(new BinderNode::property(container.Uid()));
	args.AddItem(new BinderNode::property(container.Part()));
	args.AddItem(new BinderNode::property(container.Size()));
}

// End of MailCacheProxy.cpp
