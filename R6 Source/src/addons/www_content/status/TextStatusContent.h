#ifndef TEXT_STATUS_CONTENT_H
#define TEXT_STATUS_CONTENT_H

#include <Content.h>
#include <String.h>
#include <View.h>

#include "TextStatusContentInstance.h"

class TextStatusContent : public Content {
public:
						TextStatusContent(void* handle);
	virtual				~TextStatusContent();
	virtual ssize_t		Feed(const void *UNUSED(buffer), ssize_t bufferLen,
							 bool UNUSED(done=false));
	virtual size_t		GetMemoryUsage();
	virtual	bool		IsInitialized();

private:
	virtual status_t	CreateInstance(ContentInstance **outInstance,
									   GHandler *handler, const BMessage&);
};

class TextStatusContentFactory : public ContentFactory
{
public:
	virtual void		GetIdentifiers(BMessage* into);
	virtual Content*	CreateContent(void* handle,
									  const char* mime,
									  const char* extension);
};

#endif // TEXT_STATUS_CONTENT_H
