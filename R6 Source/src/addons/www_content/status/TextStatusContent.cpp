#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <www/util.h> // for find_color() in libwww
#include <Debug.h>

#include "TextStatusContent.h"
#include "TextStatusContentInstance.h"

// ---------------------- TextStatusContent ----------------------

TextStatusContent::TextStatusContent(void* handle)
	: Content(handle)
{
}

TextStatusContent::~TextStatusContent()
{
}

ssize_t
TextStatusContent::Feed(const void *UNUSED(buffer), ssize_t bufferLen, bool UNUSED(done))
{
	return bufferLen;
}

size_t
TextStatusContent::GetMemoryUsage()
{
	return (sizeof(*this));
}

bool
TextStatusContent::IsInitialized()
{
	return true;
}

status_t
TextStatusContent::CreateInstance(ContentInstance **outInstance,
								  GHandler *handler, const BMessage &msg)
{
	*outInstance = new TextStatusContentInstance(this, handler, msg);
	return B_OK;
}

// ----------------------- JapaneseIMContentFactory -----------------------


void TextStatusContentFactory::GetIdentifiers(BMessage* into)
{
	 /*
	 ** BE AWARE: Any changes you make to these identifiers should
	 ** also be made in the 'addattr' command in the makefile.
	 */
	into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.Be.TextStatus");
}
	
Content* TextStatusContentFactory::CreateContent(void* handle,
												 const char* mime,
												 const char* extension)
{
	(void)mime;
	(void)extension;
	return new TextStatusContent(handle);
}

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id UNUSED(you), uint32 UNUSED(flags), ...)
{
	if (n == 0) return new TextStatusContentFactory;
	return 0;
}
