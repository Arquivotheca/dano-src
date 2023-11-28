#include "NetscapePlugin.h"
#include "npapi.h"
#include "npupp.h"

class NetscapeContent : public Content {
public:
	NetscapeContent();
	~NetscapeContent();
	virtual	ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual	size_t GetMemoryUsage();
	virtual bool IsInitialized();
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler,
		const BMessage&);
private:
	NetscapePlugin *fInstance;
	NPNetscapeFuncs fHooks;
};

NetscapePlugin::NetscapePlugin(Content *content, GHandler *handler)
	:	ContentInstance(content, handler)
{
}

NetscapePlugin::~NetscapePlugin()
{
}

status_t NetscapePlugin::AttachedToView(BView *view, uint32 *contentFlags)
{
}

status_t NetscapePlugin::DetachedFromView(BView *view)
{
}

status_t NetscapePlugin::FrameResized(int32 newWidth, int32 newHeight)
{
}

status_t NetscapePlugin::Draw(BView *into, BPoint location, BRect exposed)
{
}

status_t NetscapePlugin::GetSize(int32 *x, int32 *y)
{
}

status_t NetscapePlugin::ContentNotification(BMessage *msg)
{
}

ssize_t NetscapePlugin::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
}

NetscapeContent::NetscapeContent()
{
}

NetscapeContent::~NetscapeContent()
{
}

bool NetscapeContent::IsInitialized()
{
	return true;
}

ssize_t NetscapeContent::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
	if (fInstance)
		fInstance->Feed(buffer, bufferLen, done);
}

size_t NetscapeContent::GetMemoryUsage()
{
	return 0;
}

status_t NetscapeContent::CreateInstance(ContentInstance **outInstance, GHandler *handler,
	const BMessage&)
{
}


