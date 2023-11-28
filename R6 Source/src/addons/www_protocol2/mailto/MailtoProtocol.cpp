/*
	MailtoProtocol.cpp
*/
#include <support2/String.h>
#include "MailtoProtocol.h"

MailtoProtocol::MailtoProtocol(void *inHandle)
	:	Protocol(inHandle)
{

}

MailtoProtocol::~MailtoProtocol()
{

}

status_t MailtoProtocol::Open(const BUrl &url, const BUrl &, BMessage *, uint32)
{
	// Ooo, using the BMessage API.
	// There's no easy way to handle mailto links within the current framework.
	// The alternative, as proposed by Jeff, was to have this protocol return
	// a redirect url to a local html page, which could then handle the Javascript
	// interaction. I started going down this route but it started to get ugly
	// fast. This seems to work and is relatively simple to implement. Ultimately I'm
	// going to create a binder node that can handle mail services, till that time
	// comes, we're going with the BMessage API... Kenny
#warning "Fix me"
// 	BMessenger wagner("application/x-vnd.Web");
// 	if(wagner.IsValid()) {
// 		BMessage message('mail');
// 		BString value;
// 		value << url.GetUserName() << "@" << url.GetHostName();
// 		message.AddString("email", value.String());
// 		url.GetQueryParameter("subject", &value);
// 		message.AddString("subject", value.String());
// 		url.GetQueryParameter("from", &value);
// 		message.AddString("from", value.String());
// 		url.GetQueryParameter("content-type", &value);
// 		message.AddString("content-type", value.String());
// 		url.GetQueryParameter("reply-to", &value);
// 		message.AddString("reply-to", value.String());
// 		url.GetQueryParameter("sender", &value);
// 		message.AddString("sender", value.String());
// 		url.GetQueryParameter("date", &value);
// 		message.AddString("date", value.String());
// 		wagner.SendMessage(&message);
// 	}
	return B_NO_CONTENT;
}

ssize_t MailtoProtocol::GetContentLength()
{
	return 0;
}

void MailtoProtocol::GetContentType(char *type, int)
{
	*type = '\0';
}

CachePolicy MailtoProtocol::GetCachePolicy()
{
	return CC_NO_CACHE;
}

void MailtoProtocol::Abort()
{
	// Clean-up
}

ssize_t MailtoProtocol::Read(void *, size_t)
{
	return 0;
}

ssize_t MailtoProtocol::ReadAt(off_t, void *, size_t)
{
	return B_NO_RANDOM_ACCESS;
}

off_t MailtoProtocol::Seek(off_t, uint32)
{
	return B_NO_RANDOM_ACCESS;
}

off_t MailtoProtocol::Position() const
{
	return 0;
}

class MailtoProtocolFactory : public ProtocolFactory
{
	public:
		virtual void GetIdentifiers(BMessage* into)
		{
			 /*
			 ** BE AWARE: Any changes you make to these identifiers should
			 ** also be made in the 'addattr' command in the makefile.
			 */
			into->Data().Overlay(S_PROTOCOL_SCHEMES, BValue::String("mailto"));
		}
	
		virtual Protocol* CreateProtocol(void* handle, const char*)
		{
			return new MailtoProtocol(handle);
		}
		
		virtual bool KeepLoaded() const
		{
			return true;
		}
};

extern "C" _EXPORT ProtocolFactory* make_nth_protocol(int32 n, image_id, uint32, ...)
{
	if (n == 0)
		return new MailtoProtocolFactory;

	return 0;
}

