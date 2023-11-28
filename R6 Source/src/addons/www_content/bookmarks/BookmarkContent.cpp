//
//	Browser interface
//
#include <Application.h>
#include <Bitmap.h>
#include <View.h>
#include <Debug.h>
#include <stdio.h>
#include <Message.h>
#include <Screen.h>

#include <string.h>

#include "Content.h"
#include "ContentView.h"
#include "GHandler.h"

#include "BookmarkFile.h"
#include "BookmarkEditor.h"
#include "util.h"

using namespace Wagner;

// ---------------------- myBookmarkEditor ----------------------

static const BView* FindCaselessView(const BView* root, const char* name)
{
	PRINT(("Looking at: %p (%s)\n", root, root ? root->Name() : "--"));
	
	const char* rootName = root ? root->Name() : NULL;
	if (rootName == name) return root;
	if (!rootName || !name) return NULL;
	
	if( strcasecmp(rootName, name) == 0 ) {
		return const_cast<BView*>(root);
	}
	root = root->ChildAt(0);
	while( root ) {
		const BView* ret = FindCaselessView(root, name);
		if( ret ) return ret;
		root = root->NextSibling();
	}
	
	return 0;
}

class myBookmarkEditor : public TBookmarkEditor
{
public:
	myBookmarkEditor(BRect frame, const char* name,
					const BMessage& attrs,
					TBookmarkFile* bookmarks = 0,
					uint32 resizeFlags = B_FOLLOW_ALL,
					uint32 flags = B_WILL_DRAW | B_NAVIGABLE_JUMP | B_FRAME_EVENTS)
		: TBookmarkEditor(frame, name, attrs, bookmarks, resizeFlags, flags)
	{
	}
	
	virtual ~myBookmarkEditor()
	{
	}

	
	const BView* FindContentTarget() const
	{
		// Find content instance of frame containing the list of
		// bookmarks, to notify about changes.
		const BView* bmView = this;
		while( bmView->Parent() ) bmView = bmView->Parent();
		PRINT(("Finding content from top-level: %p (%s)\n",
				bmView, bmView ? bmView->Name() : "--"));
		return FindCaselessView(bmView, "_be:content");
	}
	
	virtual void OpenURL(TBookmarkItem* it)
	{
		BMessage openURL('ourl');
		openURL.AddString("url", it->URL());
		if (it->IsSecure()) {
			openURL.AddInt32("security_zone", 0);
		}
		be_app->PostMessage(&openURL);
	}
};

// ---------------------- BookmarkContentInstance ----------------------

class BookmarkContentInstance : public ContentInstance, public GHandler {
public:
	BookmarkContentInstance(Content *content, GHandler *handler,
							const BMessage& params);
	~BookmarkContentInstance();
	virtual status_t AttachedToView(BView *view, uint32 *contentFlags);
	virtual status_t DetachedFromView();
	virtual status_t FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);
	virtual status_t GetSize(int32 *width, int32 *height, uint32 *flags);

	virtual status_t Draw(BView *into, BRect exposed);
	
	virtual	status_t HandleMessage(BMessage *msg);
	
	virtual	void	 Cleanup()
	{
		ContentInstance::Cleanup();
		GHandler::Cleanup();
	};
	
private:
	myBookmarkEditor* fEditor;
	int32 fFrameWidth, fFrameHeight;
};

static void DecodeParams(BMessage* into, const BMessage* from)
{
	char* name;
	uint32 type;
	int32 count;

	const char *str;
	rgb_color color;
	
	for( int32 i = 0;
			from->GetInfo(B_STRING_TYPE, i, &name, &type, &count) == B_OK;
			i++ ) {
		if( from->FindString(name, &str) == B_OK ) {
			color = decode_color(str);
			PRINT(("Decoded color %s into (r=%d,g=%d,b=%d,a=%d)\n",
					str, color.red, color.green, color.blue, color.alpha));
			if( color.alpha != 0 ) into->AddInt32(name, *(int32*)&color);
			else into->AddString(name, str);
		}
	}
}

BookmarkContentInstance::BookmarkContentInstance(Content *content, GHandler *handler,
												 const BMessage& attrs)
	: ContentInstance(content, handler),
	  fEditor(0), fFrameWidth(-1), fFrameHeight(-1)
{
	BMessage params;
	attrs.FindMessage("paramTags", &params);
	
	BMessage decode;
	DecodeParams(&decode, &attrs);
	DecodeParams(&decode, &params);
	
	#if DEBUG
	PRINT(("Attributes: ")); attrs.PrintToStream();
	PRINT(("Parameters: ")); params.PrintToStream();
	#endif
	
	fEditor = new myBookmarkEditor(BRect(-100, -100, -50, -50), "*bookmarks",
									decode, 0, B_FOLLOW_NONE);
}

BookmarkContentInstance::~BookmarkContentInstance()
{
	if( fEditor ) {
		if( fEditor->Parent() ) fEditor->RemoveSelf();
		delete fEditor;
	}
}

status_t BookmarkContentInstance::GetSize(int32 *width, int32 *height, uint32 *flags)
{
	*width = fFrameWidth > 0 ? fFrameWidth : 200;
	*height = fFrameHeight > 0 ? fFrameHeight : 200;
	PRINT(("BookmarkContentInstance: Returning preferred size (%.2f,%.2f)\n",
			*width, *height));
	*flags = STRETCH_HORIZONTAL | STRETCH_VERTICAL;
	return B_OK;
}

status_t BookmarkContentInstance::AttachedToView(BView *view, uint32 *contentFlags)
{
	PRINT(("BookmarkContentInstance: Attaching to %p\n", view));
	(void)contentFlags;
	if( fEditor ) {
		view->AddChild(fEditor);
		if( fFrameWidth > 0 && fFrameHeight > 0 ) {
			BMessage msg(T_BOOKMARK_SET_FRAME);
			msg.AddRect("frame", FrameInParent());
			BMessenger(fEditor).SendMessage(&msg);
		}
		const BView* bmView = fEditor->FindContentTarget();
		if( bmView ) {
			BMessenger target(bmView);
			fEditor->SetTarget(target);
			BMessage* msg = new BMessage(bmsgNotifyInstance);
			BMessage not('BOOK');
			msg->AddMessage("notification", &not);
			fEditor->SetMessage(msg);

			BMessage m(T_BOOKMARK_SET_FRAME);
			m.AddRect("frame", FrameInParent());
			BMessenger(fEditor).SendMessage(&m);
		} else {
			PRINT(("*** No _be:content content view found.\n"));
		}
	}
	return B_OK;
}

status_t BookmarkContentInstance::DetachedFromView()
{
	PRINT(("BookmarkContentInstance: Detaching from view.\n"));
	
	if( fEditor ) fEditor->RemoveSelf();
	return B_OK;
}

status_t BookmarkContentInstance::FrameChanged(BRect newFrame,
											   int32 fullWidth, int32 fullHeight)
{
	if( fEditor ) {
		fFrameHeight = (int32)newFrame.Height();
		fFrameWidth = (int32)newFrame.Width();
		BMessage msg(T_BOOKMARK_SET_FRAME);
		msg.AddRect("frame", newFrame);
		BMessenger(fEditor).SendMessage(&msg);
	};
	return ContentInstance::FrameChanged(newFrame, fullWidth, fullHeight);
}

status_t BookmarkContentInstance::Draw(BView *into, BRect exposed)
{
	(void)into;
	(void)exposed;
	return B_OK;
}
	
status_t BookmarkContentInstance::HandleMessage(BMessage *msg)
{
	if( msg->what == 'nfld' || msg->what == 'abmk' ) {
		// if this is a message to create a bookmark or folder,
		// forward it to the bookmark editor.
		BMessenger(fEditor).SendMessage(msg);
	} else {
		return GHandler::HandleMessage(msg);
	}
	
	return B_OK;
}

// ---------------------- BookmarkContent ----------------------

class BookmarkContent : public Content {
public:
	BookmarkContent(void* handle);
	virtual ~BookmarkContent();
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual size_t GetMemoryUsage();
	virtual	bool IsInitialized();

	TBookmarkFile* GetBookmarks();
	
private:
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&);

	TBookmarkFile* fBookmarks;

	friend class BookmarkContentInstance;
};


BookmarkContent::BookmarkContent(void* handle)
	: Content(handle),
	  fBookmarks(0)
{
}

BookmarkContent::~BookmarkContent()
{
	delete fBookmarks;
}

bool BookmarkContent::IsInitialized()
{
	return true;
}

size_t BookmarkContent::GetMemoryUsage()
{
	return 0;
}

ssize_t BookmarkContent::Feed(const void *d, ssize_t count, bool done)
{
	(void)d;
	(void)count;
	(void)done;

	return B_OK;
}

TBookmarkFile *BookmarkContent::GetBookmarks()
{
	return fBookmarks;
}

status_t BookmarkContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage& attributes)
{
	BMessage params;
	*outInstance = new BookmarkContentInstance(this, handler, attributes);
	return B_OK;
}

// ----------------------- BookmarkContentFactory -----------------------

class BookmarkContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "text/x-vnd.Be.Bookmarks");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char* mime,
								   const char* extension)
	{
		(void)mime;
		(void)extension;
		return new BookmarkContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if( n == 0 ) return new BookmarkContentFactory;
	return 0;
}
