#ifndef PULSE_CONTENT_H
#define PULSE_CONTENT_H

#include <Content.h>
#include <String.h>
#include <View.h>

#include "PulseContentInstance.h"


class PulseContent : public Content {
public:
						PulseContent(void* handle);
	virtual				~PulseContent();
	virtual ssize_t		Feed(const void *buffer, ssize_t bufferLen,
							 bool done=false);
	virtual size_t		GetMemoryUsage();
	virtual	bool		IsInitialized();

private:
	virtual status_t	CreateInstance(ContentInstance **outInstance,
									   GHandler *handler, const BMessage&);
};

class PulseContentFactory : public ContentFactory
{
public:
	virtual void		GetIdentifiers(BMessage* into);
	virtual Content*	CreateContent(void* handle,
									  const char* mime,
									  const char* extension);
};

#endif // PULSE_CONTENT_H
