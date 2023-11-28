/*
	MailBinderSupport.cpp
*/
#include <Debug.h>
#include "MailBinderSupport.h"

using namespace Wagner;

// ----------------------------------------------------------------
//						MailboxNodeContainer
// ----------------------------------------------------------------
MailboxNodeContainer::MailboxNodeContainer()
	:	BinderContainer()
{
	AddProperty("total", (int)(0), permsRead | permsWrite);
	AddProperty("unseen", (int)(0), permsRead | permsWrite);
	AddProperty("size", (int)(0), permsRead | permsWrite);
	AddProperty("quota", (int)(0), permsRead | permsWrite);
	AddProperty("lastSync", (int)(0), permsRead | permsWrite);
}


MailboxNodeContainer::~MailboxNodeContainer()
{
	// empty body
}

get_status_t MailboxNodeContainer::ReadProperty(const char *name, property &outProperty, const property_list &inArgs)
{
	if (strcmp("SetProperties", name) == 0) {
		if (inArgs.Count() == 5) {
			BinderNode::property prop;
			
			ReadProperty("total", prop);
			if (prop.Number() != inArgs[0].Number())
				WriteProperty("total", inArgs[0]);
			
			ReadProperty("unseen", prop);
			if (prop.Number() != inArgs[1].Number())
				WriteProperty("unseen", inArgs[1]);
			
			ReadProperty("size", prop);
			if (prop.Number() != inArgs[2].Number())
				WriteProperty("size", inArgs[2]);
			
			ReadProperty("quota", prop);
			if (prop.Number() != inArgs[3].Number())
				WriteProperty("quota", inArgs[3]);

			ReadProperty("lastSync", prop);
			if (prop.Number() != inArgs[4].Number())
				WriteProperty("lastSync", inArgs[4]);
			
		}
		return B_OK;
	}
	return BinderContainer::ReadProperty(name, outProperty, inArgs);
}

// ----------------------------------------------------------------
//						MailBinderNode
// ----------------------------------------------------------------
MailBinderNode::MailBinderNode()
	:	BinderContainer()
{
	AddProperty("inbox", new MailboxNodeContainer());
	AddProperty("Drafts", new MailboxNodeContainer());
	AddProperty("Sent Items", new MailboxNodeContainer());
}

MailBinderNode::~MailBinderNode()
{
	// empty body
}

put_status_t MailBinderNode::WriteProperty(const char *name, const property &inProperty)
{
	return BinderContainer::WriteProperty(name, inProperty);
}

get_status_t MailBinderNode::ReadProperty(const char *name, property &outProperty, const property_list &inArgs)
{
	return BinderContainer::ReadProperty(name, outProperty, inArgs);
}

// End of MailCacheSupport.cpp
