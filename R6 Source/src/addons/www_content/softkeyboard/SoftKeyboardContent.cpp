//
//	Browser interface
//
#include <Bitmap.h>
#include <View.h>
#include <Debug.h>
#include <stdio.h>
#include <Message.h>
#include <Screen.h>

#include "Content.h"
#include "SoftKeyboard.h"

#include <Debug.h>

#include <URL.h>

using namespace Wagner;

// ---------------------- SoftKeyboardContentInstance ----------------------

class SoftKeyboardContentInstance : public ContentInstance
{
public:
						SoftKeyboardContentInstance(Content *content,
													GHandler *handler,
													const BMessage& params);
	virtual				~SoftKeyboardContentInstance();
	
	virtual status_t	AttachedToView(BView *view, uint32 *contentFlags);
	virtual status_t	DetachedFromView();
	virtual status_t	GetSize(int32 *width, int32 *height, uint32 *flags);
	virtual	status_t	FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);

private:
	SoftKeyboard	* fKeyboardView;
	float			fFrameWidth, fFrameHeight;
};


SoftKeyboardContentInstance::SoftKeyboardContentInstance(Content *content, GHandler *handler,
												 const BMessage& params)
	: ContentInstance(content, handler),
	  fKeyboardView(0), fFrameWidth(200), fFrameHeight(200)
{
	status_t err;
	const char * configPath;
	
	err = params.FindString("kbdconfig", &configPath);
	if (err != B_OK)
		configPath = "$RESOURCES/SoftKeyboard/qwerty.kbd";
	
	URL url;
	url.ExtractFromMessage("baseURL", &params);
	
	
//# if 0
	// Find this by stripping off the rightmost chars up until the '/'
	char * relativeDir = strdup(url.GetPath());
	char * p = relativeDir;
	while (*p)
		++p;
	while (*p != '/' && p > relativeDir)
		--p;
	if (p != relativeDir)
		*p = '\0';
	BString basePath = url.GetHostName();
	basePath.Append(relativeDir);
	
	fKeyboardView = new SoftKeyboard(BRect(0, 0, 10, 10), "*soft_keyboard",
											B_FOLLOW_LEFT | B_FOLLOW_TOP,
											configPath,
											&params,
											basePath.String());
	
	BRect bounds = fKeyboardView->Bounds();
	fFrameWidth = bounds.Width();
	fFrameHeight = bounds.Height();

}


SoftKeyboardContentInstance::~SoftKeyboardContentInstance()
{
	if(fKeyboardView)
	{
		if (fKeyboardView->Parent())
			fKeyboardView->RemoveSelf();
		delete fKeyboardView;
	}
}


status_t SoftKeyboardContentInstance::AttachedToView(BView * view, uint32 * contentFlags)
{
	// Unused
	(void)contentFlags;
	
	if( fKeyboardView )
	{
		view->AddChild(fKeyboardView);
	}
	return B_OK;
}


status_t SoftKeyboardContentInstance::DetachedFromView()
{
	if(fKeyboardView)
		fKeyboardView->RemoveSelf();
	return B_OK;
}


status_t SoftKeyboardContentInstance::GetSize(int32 * width, int32 * height, uint32 * flags)
{
	// dimensions are real sizes, not BRect "stupid sizes".
	*width = int32(fFrameWidth+1);
	*height = int32(fFrameHeight+1);
	*flags = 0;
	return B_OK;
}


status_t
SoftKeyboardContentInstance::FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight)
{
	fKeyboardView->MoveTo(newFrame.LeftTop());
	return ContentInstance::FrameChanged(newFrame, fullWidth, fullHeight);
}




// ---------------------- SoftKeyboardContent ----------------------

class SoftKeyboardContent : public Content {
public:
	SoftKeyboardContent(void* handle);
	virtual ~SoftKeyboardContent();
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual size_t GetMemoryUsage();
	virtual	bool IsInitialized();

private:
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&);

	friend class SoftKeyboardContentInstance;
};


SoftKeyboardContent::SoftKeyboardContent(void* handle)
	: Content(handle)
{
}

SoftKeyboardContent::~SoftKeyboardContent()
{
}

bool SoftKeyboardContent::IsInitialized()
{
	return true;
}

size_t SoftKeyboardContent::GetMemoryUsage()
{
	return 0;
}

ssize_t SoftKeyboardContent::Feed(const void *d, ssize_t count, bool done)
{
	(void)d;
	(void)count;
	(void)done;

	return B_OK;
}

status_t SoftKeyboardContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage& attributes)
{
	BMessage params;
	attributes.FindMessage("paramTags", &params);
	*outInstance = new SoftKeyboardContentInstance(this, handler, attributes);
	
	return B_OK;
}

// ----------------------- SoftKeyboardContentFactory -----------------------

class SoftKeyboardContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.Be.SoftKeyboard");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char* mime,
								   const char* extension)
	{
		(void)mime;
		(void)extension;
		return new SoftKeyboardContent(handle);
	}
};


extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if( n == 0 ) return new SoftKeyboardContentFactory;
	return 0;
}
