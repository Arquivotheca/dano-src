#ifndef _REALPROTOCOL_H
#define _REALPROTOCOL_H

#include <www2/Protocol.h>

using namespace B::WWW2;

class RealProtocol : public Protocol
{
public:
	RealProtocol(void* handle);
	virtual ~RealProtocol();
	virtual status_t Open(const BUrl &url, const BUrl&, BMessage *outErrorParams, uint32 flags);
	virtual void GetContentType(char *type, int size);
};

#endif
