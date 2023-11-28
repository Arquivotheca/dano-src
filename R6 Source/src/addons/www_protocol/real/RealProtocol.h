#ifndef _REALPROTOCOL_H
#define _REALPROTOCOL_H

using namespace Wagner;

class RealProtocol : public Protocol {
public:
	RealProtocol(void* handle);
	virtual ~RealProtocol();
	virtual status_t Open(const URL &url, const URL&, BMessage* outErrorParams,
		uint32 flags);
	virtual void GetContentType(char *type, int size);
};

#endif
