#include <View.h>
#include <stdio.h>
#include <Window.h>
#include <Application.h>
#include <Content.h>
#include <ContentView.h>
#include <Debug.h>
#include <GLooper.h>
#include "KeyTrapContent.h"

#define NOTIFY_KEY_DOWN 	'keyD'
#define NOTIFY_KEY_UP		'keyU'
#define TOUCH(x) (void)(x)

// ------------------------------------------------------------------------ //

class KeyTrapContentFactory : public ContentFactory {
public:
	virtual void GetIdentifiers(BMessage* into);
	virtual Content* CreateContent(void* handle, const char*, const char*);
};

// ------------------------------------------------------------------------ //

class KeyView : public BView
{
public:
	KeyView(int32 downCode=0, int32 upCode=0) :
		BView(BRect(0, 0, 0, 0), "", B_FOLLOW_NONE, 0),
		_downCode(downCode),
		_upCode(upCode),
		_target(0)
	{
		SetEventMask(B_KEYBOARD_EVENTS, 0);
	}

	void SetTarget(ContentInstance* instance)
	{
		_target = instance;
	}

	virtual void KeyDown(const char *bytes, int32 numBytes)
	{
		if(!_downCode || !_target || !bytes || numBytes < 1)
			return;
		char buffer[5];
		strncpy(buffer, bytes, min_c(numBytes, 4)+1);
		BMessage m(_downCode);
		m.AddString("key", buffer);
		_target->Notification(&m);
	}

	virtual void KeyUp(const char *bytes, int32 numBytes)
	{
		if(!_upCode || !_target || !bytes || numBytes < 1)
			return;
		char buffer[5];
		strncpy(buffer, bytes, min_c(numBytes, 4)+1);
		BMessage m(_upCode);
		m.AddString("key", buffer);
		_target->Notification(&m);
	}

private:
	int32				_downCode;
	int32				_upCode;
	ContentInstance*	_target;
};

// ------------------------------------------------------------------------ //

KeyTrapContentInstance::KeyTrapContentInstance(Content *content, GHandler *handler,
	const BMessage &msg) :
	ContentInstance(content, handler),
	_sink(0)
{
}

KeyTrapContentInstance::~KeyTrapContentInstance()
{
}

status_t 
KeyTrapContentInstance::AttachedToView(BView *view, uint32 *contentFlags)
{
	// find parent content view, initialize messenger
	while(view)
	{
		ContentView* v = dynamic_cast<ContentView*>(view);
		if(v)
		{
			ContentInstance* i = v->GetContentInstance();

			_sink = new KeyView(NOTIFY_KEY_DOWN, NOTIFY_KEY_UP);	
			_sink->SetTarget(i);
			view->AddChild(_sink);
			break;
		}
		view = view->Parent();
	}
	return B_OK;
}

status_t 
KeyTrapContentInstance::DetachedFromView()
{
	if(_sink)
	{
		_sink->RemoveSelf();
		delete _sink;
		_sink = 0;
	}
	return B_OK;
}



status_t KeyTrapContentInstance::GetSize(int32 *width, int32 *height, uint32 *outResizeFlags)
{
	*width = 0;
	*height = 0;
	*outResizeFlags = 0;
	return B_OK;
}

void 
KeyTrapContentInstance::Cleanup()
{
	ContentInstance::Cleanup();
	GHandler::Cleanup();
}

status_t 
KeyTrapContentInstance::HandleMessage(BMessage *msg)
{
	return GHandler::HandleMessage(msg);
}


// ------------------------------------------------------------------------ //


KeyTrapContent::KeyTrapContent(void *handle)
	: 	Content(handle)
{
}

size_t KeyTrapContent::GetMemoryUsage()
{
	return 0x10;
}

ssize_t KeyTrapContent::Feed(const void *buffer, ssize_t count, bool done)
{
	TOUCH(buffer);
	TOUCH(count);
	TOUCH(done);
	return B_OK;
}

status_t KeyTrapContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &msg)
{
	*outInstance = new KeyTrapContentInstance(this, handler, msg);
	return B_OK;
}


void KeyTrapContentFactory::GetIdentifiers(BMessage* into)
{
	 /*
	 ** BE AWARE: Any changes you make to these identifiers should
	 ** also be made in the 'addattr' command in the makefile.
	 */
	into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.Be.KeyTrap");
}

Content* KeyTrapContentFactory::CreateContent(void* handle, const char*, const char*)
{
	return new KeyTrapContent(handle);
}


extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you,
	uint32 flags, ...)
{
	if( n == 0 ) return new KeyTrapContentFactory;
	return 0;
}



