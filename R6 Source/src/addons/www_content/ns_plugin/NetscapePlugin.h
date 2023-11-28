#ifndef _NETSCAPE_PLUGIN_H
#define _NETSCAPE_PLUGIN_H

#include <Content.h>

using namespace Wagner;

class NetscapePlugin : public ContentInstance {
public:
	NetscapePlugin(Content*, GHandler*);
	virtual ~NetscapePlugin();
	virtual	status_t AttachedToView(BView *view, uint32 *contentFlags);
	virtual	status_t DetachedFromView(BView *view);
	virtual	status_t Draw(BView *into, BPoint location, BRect exposed);
	virtual	status_t FrameResized(int32 newWidth, int32 newHeight);
	virtual	status_t GetSize(int32 *x, int32 *y);
	virtual	status_t ContentNotification(BMessage *msg);
	ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
};

#endif
