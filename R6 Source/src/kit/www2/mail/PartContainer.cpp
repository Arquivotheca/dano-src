/*
	PartContainer.cpp
*/
#include <Binder.h>
#include "PartContainer.h"

using namespace Wagner;

PartContainer::PartContainer()
{
	Init();
}

PartContainer::PartContainer(const char *mailbox, const char *uid, const char *part)
{
	Init(mailbox, uid, part);
}

PartContainer::~PartContainer()
{

}

void PartContainer::Init(const char *mailbox, const char *uid, const char *part)
{
	ASSERT((mailbox != NULL) && (uid != NULL) && (part != NULL));
	
	// Default to the logged in user and selected mailbox.
	BinderNode::property prop = BinderNode::Root()["user"]["~"]["name"];
	fUser = prop.String();
	fMailbox = mailbox;
	fUid = uid;
	fPart = part;
	fSize = 0;
	fCache = true;
	fFlagRead = true;
	fIsMainPart = true;
	fBypassRedirectServer = false;
	fRedirectUrl = NULL;
	fContentType = NULL;
	fCachePolicy = NULL;
}

bool PartContainer::IsValid()
{
	if ((fUser == B_EMPTY_STRING) || (fMailbox == B_EMPTY_STRING) || (fUid == B_EMPTY_STRING) || (fPart == B_EMPTY_STRING))
		return false;

	return true;
}

void PartContainer::SetUser(const char *user)
{
	ASSERT(user != NULL);
	fUser = user;
}

void PartContainer::SetMailbox(const char *mailbox)
{
	ASSERT(mailbox != NULL);
	fMailbox = mailbox;
}

void PartContainer::SetUid(const char *uid)
{
	ASSERT(uid != NULL);
	fUid = uid;
}

void PartContainer::SetPart(const char *part)
{
	ASSERT(part != NULL);
	fPart = part;
}

void PartContainer::SetSize(int size)
{
	fSize = size;
}

void PartContainer::SetCache(bool cache)
{
	fCache = cache;
}

void PartContainer::SetFlagRead(bool flag)
{
	fFlagRead = flag;
}

void PartContainer::SetIsMainPart(bool main)
{
	fIsMainPart = main;
}

void PartContainer::SetBypassRedirectServer(bool bypass)
{
	fBypassRedirectServer = bypass;
}

void PartContainer::SetRedirectUrl(const URL &url)
{
	if (fRedirectUrl != NULL)
		fRedirectUrl->SetTo(url);
}

void PartContainer::SetContentType(const char *type)
{
	if (fContentType != NULL)
		fContentType->SetTo(type);
}

void PartContainer::SetCachePolicy(CachePolicy cachePolicy)
{
	if (fCachePolicy != NULL)
		*fCachePolicy = cachePolicy;
}


void PartContainer::SetRedirectUrlPtr(URL *ptr)
{
	fRedirectUrl = ptr;
}

void PartContainer::SetContentTypePtr(BString *ptr)
{
	fContentType = ptr;
}

void PartContainer::SetCachePolicyPtr(CachePolicy *ptr)
{
	fCachePolicy = ptr;
}

const char *PartContainer::User() const
{
	return fUser.String();
}

const char *PartContainer::Mailbox() const
{
	return fMailbox.String();
}

const char *PartContainer::Uid() const
{
	return fUid.String();
}

const char *PartContainer::Part() const
{
	return fPart.String();
}

int PartContainer::Size() const
{
	return fSize;
}

bool PartContainer::Cache() const
{
	return fCache;
}

bool PartContainer::FlagRead() const
{
	return fFlagRead;
}

bool PartContainer::IsMainPart() const
{
	return fIsMainPart;
}

bool PartContainer::BypassRedirectServer() const
{
	return fBypassRedirectServer;
}

URL *PartContainer::RedirectUrlPtr()
{
	return fRedirectUrl;
}

BString *PartContainer::ContentTypePtr()
{
	return fContentType;
}

CachePolicy *PartContainer::CachePolicyPtr()
{
	return fCachePolicy;
}

// End of PartContainer.cpp
