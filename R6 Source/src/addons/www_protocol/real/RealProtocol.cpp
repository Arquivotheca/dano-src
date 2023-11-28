#include <stdlib.h>
#include <ctype.h>
#include <Debug.h>
#include "Protocol.h"
#include "Resource.h"
#include "RealProtocol.h"

// ----------------------- RealProtocolFactory -----------------------

class RealProtocolFactory : public ProtocolFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_PROTOCOL_SCHEMES, "rtsp");
		into->AddString(S_PROTOCOL_SCHEMES, "pnm");
	}
	
	virtual Protocol* CreateProtocol(void* handle, const char* scheme)
	{
		(void)scheme;
		return new RealProtocol(handle);
	}
	
	virtual bool KeepLoaded() const
	{
		return true;
	}
};

extern "C" _EXPORT ProtocolFactory* make_nth_protocol(int32 n, image_id /* you */, uint32 /* flags */, ...)
{
	if( n == 0 ) return new RealProtocolFactory;
	return 0;
}

// ----------------------- RealProtocols -----------------------

RealProtocol::RealProtocol(void* handle)
	: Protocol(handle)
{
}

RealProtocol::~RealProtocol()
{
}

status_t RealProtocol::Open(const URL&, const URL&, BMessage*, uint32)
{
	return B_OK;
}

void RealProtocol::GetContentType(char *type, int size)
{
	strncpy(type, "application/vnd.rn-realmedia", size);
}
