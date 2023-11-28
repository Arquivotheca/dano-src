#ifndef TEXT_STATUS_CONTENT_H
#define TEXT_STATUS_CONTENT_H

#include <Content.h>
#include <String.h>
#include <View.h>

#include "TextClockContentInstance.h"

class TextClockContent : public Content {
public:
						TextClockContent(void* handle);
	virtual				~TextClockContent();
	virtual ssize_t		Feed(const void *UNUSED(buffer), ssize_t bufferLen,
							 bool UNUSED(done=false));
	virtual size_t		GetMemoryUsage();
	virtual	bool		IsInitialized();

private:
	virtual status_t	CreateInstance(ContentInstance **outInstance,
									   GHandler *handler, const BMessage&);
};

class TextClockContentFactory : public ContentFactory
{
public:
	virtual void		GetIdentifiers(BMessage* into);
	virtual Content*	CreateContent(void* handle,
									  const char* mime,
									  const char* extension);
};

#endif // TEXT_STATUS_CONTENT_H