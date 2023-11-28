#include <String.h>
#include <Messenger.h>
#include <string.h>
#include "Resource.h"
#include "Content.h"
#include "Protocol.h"

using namespace Wagner;

class UpdaterContent;

class UpdaterContentInstance : public ContentInstance {
public:
	UpdaterContentInstance(UpdaterContent *parent, GHandler *h, const BMessage &msg);
	virtual ~UpdaterContentInstance();
	virtual	status_t GetSize(int32 *x, int32 *y, uint32 *outResizeFlags);
};

class UpdaterContent : public Content {
public:
	UpdaterContent(void *handle);
	virtual ~UpdaterContent();
	virtual size_t GetMemoryUsage();
	virtual	ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done);
private:
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage & msg);
	char *m_url;
	size_t m_totallen;
	size_t m_alloclen;
};

UpdaterContentInstance::UpdaterContentInstance(UpdaterContent *parent, GHandler *h, const BMessage &) :
	ContentInstance(parent, h)
{
}

UpdaterContentInstance::~UpdaterContentInstance()
{
}

status_t 
UpdaterContentInstance::GetSize(int32 *x, int32 *y, uint32 *outResizeFlags)
{
	*x = *y = 0;
	*outResizeFlags = 0;
	return B_OK;
}

UpdaterContent::UpdaterContent(void* handle)
	: Content(handle), m_totallen(0), m_alloclen(0)
{
}

UpdaterContent::~UpdaterContent()
{
}

size_t 
UpdaterContent::GetMemoryUsage()
{
	return 10L;
}

ssize_t
UpdaterContent::Feed(const void*, ssize_t, bool)
{
	return B_FINISH_STREAM;
}

status_t 
UpdaterContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage & msg)
{
	*outInstance = new UpdaterContentInstance(this, handler, msg);

	BString script_url;
	if (GetResource()->GetURL().GetQueryParameter("url", &script_url) == B_OK) {
		BMessage msg('ScRq');
		msg.AddString("command", "doscript");
		msg.AddString("SCRIPT_URL", script_url.String());
		printf("booyah!\n");
		msg.PrintToStream();
		BMessenger("application/x-vnd.Be-UPDT").SendMessage(&msg);
	}

	return 0;
}


// ----------------------- UpdaterContentFactory -----------------------

class UpdaterContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.be-iaupdater");
		into->AddString(S_CONTENT_EXTENSIONS, "upd");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char* mime,
								   const char* extension)
	{
		return new UpdaterContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if( n == 0 ) return new UpdaterContentFactory;
	return 0;
}
