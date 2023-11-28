/*
	MailBinderSupport.cpp
*/
#include "MailDebug.h"
#include "MailBinderSupport.h"

using namespace Wagner;

// ----------------------------------------------------------------
//						MailboxNodeContainer
// #pragma mark -
// ----------------------------------------------------------------
MailboxNodeContainer::MailboxNodeContainer()
	:	BinderContainer()
{
	AddProperty("total", (int)(0), permsRead | permsWrite);
	AddProperty("unseen", (int)(0), permsRead | permsWrite);
	AddProperty("size", (int)(0), permsRead | permsWrite);
	AddProperty("quota", (int)(0), permsRead | permsWrite);
	AddProperty("lastSync", (int)(0), permsRead | permsWrite);
	AddProperty("messagesNewOnClient", (int)(0), permsRead | permsWrite);
}


MailboxNodeContainer::~MailboxNodeContainer()
{

}

get_status_t MailboxNodeContainer::ReadProperty(const char *name, property &outProperty, const property_list &inArgs)
{
	if (strcmp("SetProperties", name) == 0) {
		if (inArgs.Count() >= 5) {
			WriteProperty("total", inArgs[0]);
			WriteProperty("unseen", inArgs[1]);
			WriteProperty("size", inArgs[2]);
			WriteProperty("quota", inArgs[3]);
			WriteProperty("lastSync", inArgs[4]);
			if (inArgs.Count() == 6)
				WriteProperty("messagesNewOnClient", inArgs[5]);
		}
		return B_OK;
	}
	return BinderContainer::ReadProperty(name, outProperty, inArgs);
}

// ----------------------------------------------------------------
//						AccountStatusNode
// #pragma mark -
// ----------------------------------------------------------------
AccountStatusNode::AccountStatusNode()
	:	BinderContainer()
{
	AddProperty("inbox", new MailboxNodeContainer());
	AddProperty("Drafts", new MailboxNodeContainer());
	AddProperty("Sent Items", new MailboxNodeContainer());
	AddProperty("sending", new SendingStatusNode());
}

AccountStatusNode::~AccountStatusNode()
{
	// empty body
}

put_status_t AccountStatusNode::WriteProperty(const char *name, const property &inProperty)
{
	return BinderContainer::WriteProperty(name, inProperty);
}

get_status_t AccountStatusNode::ReadProperty(const char *name, property &outProperty, const property_list &inArgs)
{
	return BinderContainer::ReadProperty(name, outProperty, inArgs);
}

// ----------------------------------------------------------------
//						SendingStatusNode
// #pragma mark -
// ----------------------------------------------------------------
SendingStatusNode::SendingStatusNode()
	:	BinderContainer()
{
	AddProperty("status", (int)(0), permsRead | permsWrite);
	AddProperty("totalBytes", (int)(0), permsRead | permsWrite);
	AddProperty("bytesSent", (int)(0), permsRead | permsWrite);
}

SendingStatusNode::~SendingStatusNode()
{

}

get_status_t SendingStatusNode::ReadProperty(const char *name, property &outProperty, const property_list &inArgs)
{
	return BinderContainer::ReadProperty(name, outProperty, inArgs);
}


// ----------------------------------------------------------------
//						SendingStatusNode
// #pragma mark -
// ----------------------------------------------------------------
SendingNodeProxy::SendingNodeProxy(const char *username, uint32 status)
{
	ASSERT(username != NULL);
	fNode = BinderNode::Root()["user"][username]["email"]["account"]["status"]["Sending"];
	SetStatus(status);
}


SendingNodeProxy::~SendingNodeProxy()
{
	Reset();
}

void SendingNodeProxy::SetStatus(uint32 status)
{
	fNode["status"] = (int)status;
}

void SendingNodeProxy::SetTotalBytes(int32 totalBytes)
{
	fNode["totalBytes"] = totalBytes;
}

void SendingNodeProxy::UpdateBytesSent(int32 bytes)
{
	BinderNode::property prop = fNode["bytesSent"];
	double total = prop.Number() + (double)bytes;
	fNode["bytesSent"] = total;
}

void SendingNodeProxy::Reset()
{
	fNode["status"] = (int)0;
	fNode["totalBytes"] = (int)0;
	fNode["bytesSent"] = (int)0;
}
// End of MailBinderSupport.cpp
