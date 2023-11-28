#ifndef __KEYTRAP_CONTENT_H__
#define __KEYTRAP_CONTENT_H__

#include <Content.h>
#include <Handler.h>
#include "GHandler.h"

using namespace Wagner;

class KeyTrapContent;
class BView;
class KeyView;

class KeyTrapContentInstance : public ContentInstance, public GHandler {
public:
	KeyTrapContentInstance(Content *content, GHandler *handler, const BMessage &msg);
	~KeyTrapContentInstance();

	virtual	status_t AttachedToView(BView *view, uint32 *contentFlags);
	virtual	status_t DetachedFromView();
	virtual	status_t GetSize(int32 *x, int32 *y, uint32 *outResizeFlags);

	virtual	void Cleanup();
	virtual	status_t HandleMessage(BMessage *msg);
	

private:
	KeyView*		_sink;
};

class KeyTrapContent : public Content {
public:
	KeyTrapContent(void *handle);
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual size_t GetMemoryUsage();

private:
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &msg);

};

#endif // __KEYTRAP_CONTENT_H__
