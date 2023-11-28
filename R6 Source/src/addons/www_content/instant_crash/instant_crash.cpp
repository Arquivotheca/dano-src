
#include <string.h>

#include "Content.h"

#if 0
#define FUNC() puts(__PRETTY_FUNCTION__)
#else
#define FUNC()
#endif

#define CRASH() { float f = 1.0; (*(void(*)())&f)(); }

using namespace Wagner;

class InstantCrashContent;

class InstantCrashContentInstance : public ContentInstance {
public:
	InstantCrashContentInstance(InstantCrashContent * parent, GHandler * h, const BMessage & msg);
	virtual ~InstantCrashContentInstance();
	virtual	status_t GetSize(int32 *x, int32 *y, uint32 *outResizeFlags);
private:
	InstantCrashContent * m_parent;
};

class InstantCrashContent : public Content {
public:
	InstantCrashContent(void* handle, bool isLeaving);
	virtual ~InstantCrashContent();
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done);
	virtual size_t GetMemoryUsage();
private:
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage & msg);
	bool m_isLeaving;
};

InstantCrashContentInstance::InstantCrashContentInstance(InstantCrashContent * parent, GHandler * h, const BMessage &) :
	ContentInstance(parent, h)
{
	m_parent = parent;
}

InstantCrashContentInstance::~InstantCrashContentInstance()
{
	CRASH();
}

status_t 
InstantCrashContentInstance::GetSize(int32 *x, int32 *y, uint32 * outResizeFlags)
{
	*x = *y = 0;
	*outResizeFlags = 0;
	return B_OK;
}





InstantCrashContent::InstantCrashContent(void* handle, bool isLeaving)
	: Content(handle), m_isLeaving(isLeaving)
{
}


InstantCrashContent::~InstantCrashContent()
{
}

ssize_t 
InstantCrashContent::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
	return bufferLen;
}

size_t 
InstantCrashContent::GetMemoryUsage()
{
	return 10L;
}

status_t 
InstantCrashContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage & msg)
{
	if (!m_isLeaving) {
		CRASH();
	}
	*outInstance = new InstantCrashContentInstance(this, handler, msg);
	return 0;
}


// ----------------------- InstantCrashContentFactory -----------------------

class InstantCrashContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.hplus.instant_crash");
		into->AddString(S_CONTENT_EXTENSIONS, "icr");
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.hplus.leaving_crash");
		into->AddString(S_CONTENT_EXTENSIONS, "lcr");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char* mime,
								   const char* extension)
	{
		bool leaving = false;
		if (mime && !strcasecmp(mime, "application/x-vnd.hplus.leaving_crash"))
			leaving = true;
		if (extension && !strcasecmp(extension, "lcr"))
			leaving = true;
		return new InstantCrashContent(handle, leaving);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if( n == 0 ) return new InstantCrashContentFactory;
	return 0;
}
