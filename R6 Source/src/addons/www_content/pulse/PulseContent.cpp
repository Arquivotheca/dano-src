#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <www/util.h> // for find_color() in libwww
#include <Debug.h>

#include "PulseContent.h"
#include "PulseContentInstance.h"

// ---------------------- PulseContent ----------------------

PulseContent::PulseContent(void* handle)
	: Content(handle)
{
}

PulseContent::~PulseContent()
{
}

ssize_t
PulseContent::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
	(void)buffer;
	(void)done;
	return bufferLen;
}

size_t
PulseContent::GetMemoryUsage()
{
	return (sizeof(*this));
}

bool
PulseContent::IsInitialized()
{
	return true;
}

status_t
PulseContent::CreateInstance(ContentInstance **outInstance,
								  GHandler *handler, const BMessage &msg)
{
	*outInstance = new PulseContentInstance(this, handler, msg);
	return B_OK;
}

// ----------------------- JapaneseIMContentFactory -----------------------


void PulseContentFactory::GetIdentifiers(BMessage* into)
{
	 /*
	 ** BE AWARE: Any changes you make to these identifiers should
	 ** also be made in the 'addattr' command in the makefile.
	 */
	into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.Be.PulseContent");
}
	
Content* PulseContentFactory::CreateContent(void* handle,
												 const char* mime,
												 const char* extension)
{
	(void)mime;
	(void)extension;
	return new PulseContent(handle);
}

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	(void)you;
	(void)flags;
	if (n == 0) return new PulseContentFactory;
	return 0;
}
