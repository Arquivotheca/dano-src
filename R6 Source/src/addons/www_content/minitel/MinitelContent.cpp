#include <stdio.h>

#include <Debug.h>
#include <View.h>
#include <ViewTransaction.h>

#include "MinitelContent.h"
#include "MinitelView.h"
#include "MinitelFont.h"


//#ifndef NDEBUG
//	#define ddprintf		printf
//#else
	#define ddprintf		if (1) {} else printf
//#endif


/*======================== ContentInstance ========================*/

// Note: fragile!  The hardcoded sizes of this view can't change much because
// The minitel view will resize itself and overlap they keyboard if they are too small.
MinitelContentInstance::MinitelContentInstance	(Content*			content,
												 GHandler*			handler,
												 const BMessage*	msg)
	: ContentInstance	(content, handler),
#ifdef USE_KEYBOARD_PANEL
	  fHeight			(25 * kFONT_CELL_HEIGHT + kKEYBOARD_BAR_HEIGHT),	// xxx Careful!
#else
	  fHeight			(25 * kFONT_CELL_HEIGHT),	// xxx Careful!
#endif
	  fWidth			(40 * kFONT_CELL_WIDTH),
	  fResult			(B_ERROR),
	  fMinitel			(NULL),
	  fView				(NULL)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::MinitelContentInstance()\n", (int)this);

	fResult = B_NO_ERROR;
}

/*-----------------------------------------------------------------*/

MinitelContentInstance::~MinitelContentInstance	()
{
	ddprintf("minitel.so: %x->MinitelContentInstance::~MinitelContentInstance()\n", (int)this);
}

/*-----------------------------------------------------------------*/

status_t
MinitelContentInstance::AttachedToView			(BView*		view,
												 uint32*	/* flags */)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::AttachedToView()\n", (int)this);
#ifndef NDEBUG
	FrameInParent().PrintToStream();
#endif

	fView = view;
	fView->AddChild(fMinitel = new Minitel(BRect(0, 0, fWidth - 1, fHeight - 1)));

	return B_OK;
}

/*-----------------------------------------------------------------*/

status_t
MinitelContentInstance::DetachedFromView		()
{
	ddprintf("minitel.so: %x->MinitelContentInstance::DetachedFromView()\n", (int)this);

	if ((fView) && (fMinitel))
	{
		fView->RemoveChild(fMinitel);
		delete fMinitel;
		fMinitel = NULL;
	}
	return B_OK;
}

/*-----------------------------------------------------------------*/

status_t
MinitelContentInstance::Draw					(BView*		/* view */,
												 BRect		/* exposed */)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::Draw()\n", (int)this);

	return B_OK;
}

/*-----------------------------------------------------------------*/

status_t
MinitelContentInstance::FrameChanged			(BRect new_frame,
												 int32 width,
												 int32 height)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::FrameChanged()\n", (int)this);
#ifndef NDEBUG
	new_frame.PrintToStream();
#endif

	if (fMinitel)
	{
		ViewTransaction trans;

		trans.Move(fMinitel, new_frame.left, new_frame.top);
		//trans.Resize(fMinitel, new_frame.Width(), new_frame.Height());
		trans.Commit();
	}

	//fWidth = width;
	//fHeight = height;

	return B_OK;
}

/*-----------------------------------------------------------------*/

status_t
MinitelContentInstance::GetSize					(int32*		width,
												 int32*		height,
												 uint32*	resize_flags)
{
	ddprintf("minitel.so %x->MinitelContentInstance::GetSize()\n", (int)this);

	*width = fWidth;
	*height = fHeight;

	return B_OK;
}

/*-----------------------------------------------------------------*/

status_t
MinitelContentInstance::ContentNotification		(BMessage*	msg)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::ContentNotification()\n", (int)this);

	return ContentInstance::ContentNotification(msg);
}

/*-----------------------------------------------------------------*/

void
MinitelContentInstance::MouseDown				(BPoint				where,
												 const BMessage*	msg)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::MouseDown()\n", (int)this);


	ContentInstance::MouseDown(where, msg);
}

/*-----------------------------------------------------------------*/

void
MinitelContentInstance::MouseUp					(BPoint				where,
												 const BMessage*	msg)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::MouseUp()\n", (int)this);

	ContentInstance::MouseUp(where, msg);
}

/*-----------------------------------------------------------------*/

void
MinitelContentInstance::MouseMoved				(BPoint				where,
												 uint32				code,
												 const BMessage*	msg,
												 const BMessage*	event)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::MouseMoved()\n", (int)this);

	ContentInstance::MouseMoved(where, code, msg, event);
}

/*-----------------------------------------------------------------*/

void
MinitelContentInstance::KeyDown					(const char*		bytes,
												 int32				num_bytes,
												 const BMessage*	msg)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::KeyDown()\n", (int)this);

	ContentInstance::KeyDown(bytes, num_bytes, msg);
}

/*-----------------------------------------------------------------*/

void
MinitelContentInstance::KeyUp					(const char*		bytes,
												 int32				num_bytes,
												 const BMessage*	msg)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::KeyUp()\n", (int)this);

	ContentInstance::KeyUp(bytes, num_bytes, msg);
}

/*-----------------------------------------------------------------*/

void
MinitelContentInstance::SyncToState				(BMessage* msg)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::SyncToState()\n", (int)this);

	ContentInstance::SyncToState(msg);
}

/*-----------------------------------------------------------------*/

status_t
MinitelContentInstance::UsurpPredecessor		(ContentInstance* old_instance)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::UsurpPredecessor()\n", (int)this);

	return ContentInstance::UsurpPredecessor(old_instance);
}

/*-----------------------------------------------------------------*/

void
MinitelContentInstance::Notification			(BMessage* msg)
{
	ddprintf("minitel.so: %x->MinitelContentInstance::Notification()\n", (int)this);

	ContentInstance::Notification(msg);
}


/*============================ Content ============================*/

MinitelContent::MinitelContent					(void* handle)
 :	Content	(handle),
 	fResult	(B_ERROR)
{
	ddprintf("minitel.so: %x->MinitelContent::MinitelContent()\n", (int)this);

	fResult = B_NO_ERROR;
}

/*-----------------------------------------------------------------*/

MinitelContent::~MinitelContent					()
{
	ddprintf("minitel.so: %x->MinitelContent::~MinitelContent()\n", (int)this);
}

/*-----------------------------------------------------------------*/

status_t
MinitelContent::CreateInstance					(ContentInstance**	out_instance,
												 GHandler*			handler,
												 const BMessage&	msg)
{
	ddprintf("minitel.so: %x->MinitelContent::CreateInstance()\n", (int)this);
#ifndef NDEBUG
	msg.PrintToStream();
#endif

	if (fResult == B_NO_ERROR)
		*out_instance = new MinitelContentInstance(this, handler, &msg);
	return fResult;
}

/*-----------------------------------------------------------------*/

ssize_t
MinitelContent::Feed							(const void*	/* buffer */,
												 ssize_t		count,
												 bool			/* done */)
{
	ddprintf("minitel.so: %x->MinitelContent::Feed(%d)\n", (int)this, (int)count);

	return count;
}

/*-----------------------------------------------------------------*/

size_t
MinitelContent::GetMemoryUsage					()
{
	ddprintf("minitel.so: %x->MinitelContent::GetMemoryUsage()\n", (int)this);

	return 0;
}

/*-----------------------------------------------------------------*/

bool
MinitelContent::IsInitialized					()
{
	ddprintf("minitel.so: %x->MinitelContent::IsInitialized()\n", (int)this);

	return (fResult == B_NO_ERROR);
}


//========================================================================

Minitel::Minitel								(BRect rect)
 : BView		(rect,
				 "Minitel",
				 B_FOLLOW_NONE,
				 B_WILL_DRAW)
{
	BRect			r = rect;
	MinitelView*	view;

	r.OffsetTo(0, 0);
#ifdef USE_KEYBOARD_PANEL
	r.bottom -= kKEYBOARD_BAR_HEIGHT;
#endif
	AddChild(view = new MinitelView(r));
#ifdef USE_KEYBOARD_PANEL
	r.top = r.bottom + 1;
	r.bottom = r.top + kKEYBOARD_BAR_HEIGHT;
	r.left--;
	r.right++;
	AddChild(new KeyboardPanel(r, view));
#endif
}

// ====================== MinitelContentFactory ======================

class MinitelContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-minitel");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char* mime,
								   const char* extension)
	{
		(void)mime;
		(void)extension;
		return new MinitelContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if( n == 0 ) return new MinitelContentFactory;
	return 0;
}
