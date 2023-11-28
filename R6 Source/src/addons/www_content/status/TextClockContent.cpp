#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <www/util.h> // for find_color() in libwww
#include <Debug.h>

#include "TextClockContent.h"
#include "TextClockContentInstance.h"

// ---------------------- TextClockContent ----------------------

TextClockContent::TextClockContent(void* handle)
	: Content(handle)
{
}

TextClockContent::~TextClockContent()
{
}

ssize_t
TextClockContent::Feed(const void *UNUSED(buffer), ssize_t bufferLen, bool UNUSED(done))
{
	return bufferLen;
}

size_t
TextClockContent::GetMemoryUsage()
{
	return (sizeof(*this));
}

bool
TextClockContent::IsInitialized()
{
	return true;
}

status_t
TextClockContent::CreateInstance(ContentInstance **outInstance,
								  GHandler *handler, const BMessage &msg)
{
	*outInstance = new TextClockContentInstance(this, handler, msg);
	return B_OK;
}

// ----------------------- JapaneseIMContentFactory -----------------------


void TextClockContentFactory::GetIdentifiers(BMessage* into)
{
	 /*
	 ** BE AWARE: Any changes you make to these identifiers should
	 ** also be made in the 'addattr' command in the makefile.
	 */
	into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.Be.TextClock");
}
	
Content* TextClockContentFactory::CreateContent(void* handle,
												 const char* mime,
												 const char* extension)
{
	(void)mime;
	(void)extension;
	return new TextClockContent(handle);
}

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id UNUSED(you), uint32 UNUSED(flags), ...)
{
	if (n == 0) return new TextClockContentFactory;
	return 0;
}
