#include <www2/ContentManager.h>
#include "HTTP.h"

class HttpProtocolFactory : public ProtocolFactory
{
public:
	virtual void GetIdentifiers(BMessage*);
	virtual Protocol* CreateProtocol(void*, const char*);
	virtual bool KeepLoaded() const;
};

void HttpProtocolFactory::GetIdentifiers(BMessage *into)
{
	 // BE AWARE: Any changes you make to these identifiers should
	 // also be made in the 'addattr' command in the makefile.
	into->Data().Overlay(S_PROTOCOL_SCHEMES, BValue::String("http"));
	into->Data().Overlay(S_PROTOCOL_SCHEMES, BValue::String("https"));
}

Protocol* HttpProtocolFactory::CreateProtocol(void *handle, const char*)
{
	return new Http(handle);
}

bool HttpProtocolFactory::KeepLoaded() const
{
	return true;
}

extern "C" _EXPORT ProtocolFactory* make_nth_protocol(int32 n, image_id, uint32, ...)
{
	if (n == 0)
		return new HttpProtocolFactory;

	return 0;
}
