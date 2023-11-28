#ifndef _CONTENT_H
#define _CONTENT_H

#include <OS.h>
#include <GHandler.h>
#include <Region.h>
#include <Locker.h>
#include <Atom.h>
#include <image.h>
#include "Gehnaphore.h"

class BMessage;
class BView;

namespace Wagner {

class Content;
class Resource;

#define B_DEFER_LAYOUT (B_ERRORS_END+1)

enum {
	bmsgContentDirty		= 'drty',
	bmsgLayoutCompletion	= 'layc',
	bmsgContentDisplayable	= 'cdsp',
	bmsgContentLoaded		= 'cldd'
};

enum {
	// Ask renderer to draw this content instance into an
	// off-screen bitmap.
	cifDoubleBuffer			= 0x00000001,
	
	// Tell renderer that content instance's area must be
	// erased before the instance is drawn.
	cifHasTransparency		= 0x00000002
};

// Content resize flags
const uint32 STRETCH_HORIZONTAL = 1;
const uint32 STRETCH_VERTICAL = 2;
const uint32 IS_DOCKABLE = 4;

#define S_CONTENT_MIME_TYPES 			"mime_types"
#define S_CONTENT_EXTENSIONS 			"extensions"
#define S_CONTENT_PLUGIN_IDS 			"plugin_ids"
#define S_CONTENT_PLUGIN_DESCRIPTION 	"plugin_desc"

class ContentInstance : virtual public BAtom
{
public:
									ContentInstance(Content *content, GHandler *handler);

		int32						ID();
		uint32						Flags();
		BRect						FrameInParent();
		int32						FullWidth();
		int32						FullHeight();
		
		void						SetID(int32 id);

		GHandler *					Handler();
		void						SetHandler(GHandler *handler);
		
		ContentInstance *			ParentContent();
		void						SetParentContent(ContentInstance *par);

virtual	status_t					AttachedToView(BView *view, uint32 *contentFlags);
virtual	status_t					DetachedFromView();
virtual	status_t					Draw(BView *into, BRect exposed);
virtual	status_t					FrameChanged(BRect newFrame,
												 int32 fullWidth, int32 fullHeight);
virtual	status_t					GetSize(int32 *x, int32 *y, uint32 *outResizeFlags);
virtual	status_t					ContentNotification(BMessage *msg);

virtual	void						MouseDown(BPoint where, const BMessage *event=NULL);
virtual	void						MouseUp(BPoint where, const BMessage *event=NULL);
virtual	void						MouseMoved(BPoint where, uint32 code, const BMessage *a_message, const BMessage *event=NULL);
virtual	void						KeyDown(const char *bytes, int32 numBytes, const BMessage *event=NULL);
virtual	void						KeyUp(const char *bytes, int32 numBytes, const BMessage *event=NULL);

virtual	void						CopyToClipboard();
virtual void						SelectAll();
virtual void						DeselectAll();

virtual void						SaveState(BMessage *msg);
virtual	void						SyncToState(BMessage *msg);
virtual	status_t					UsurpPredecessor(ContentInstance *oldInstance);

virtual	void						Notification(BMessage *msg);

		status_t					MarkDirty(const BRegion &dirtyReg);
		status_t					MarkDirty(BRect *dirtyRect=NULL);
		status_t					FetchDirty(BRegion &region);

		// TO DO: This should be "Content()"
		Content *					GetContent() const;
		
protected:

		void						SetFlags(uint32 flags);
virtual								~ContentInstance();
virtual	void						Cleanup();

private:
		/* FBC */
virtual	void						_ReservedContentInstance1();
virtual	void						_ReservedContentInstance2();
virtual	void						_ReservedContentInstance3();
virtual	void						_ReservedContentInstance4();
virtual	void						_ReservedContentInstance5();
virtual	void						_ReservedContentInstance6();
virtual	void						_ReservedContentInstance7();
virtual	void						_ReservedContentInstance8();
virtual	void						_ReservedContentInstance9();
virtual	void						_ReservedContentInstance10();
virtual	void						_ReservedContentInstance11();
virtual	void						_ReservedContentInstance12();
		uint32						_reserved[8];

		GHandler *					m_handler;
		Content *					m_content;
		int32						m_id;
		uint32						m_flags;
		ContentInstance *			m_parentContent;
		BRect						m_frame;
		int32						m_width;
		int32						m_height;
		BRegion						m_dirty;
		Gehnaphore					m_littleLock;
};

class ContentHandle;

class Content : virtual public BAtom
{
public:
									Content();
									Content(void* handle);

		status_t					NewInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &userData);
		status_t					TakeMeOffYourList(ContentInstance *instance);
		void						InstanceDeleted(ContentInstance *instance);
		status_t					NotifyInstances(BMessage *msg);

		void						SetResource(Resource *resource);
		Resource *					GetResource() const;
		
virtual	ssize_t						Feed(const void *buffer, ssize_t bufferLen, bool done=false) = 0;

static	Content *					InstantiateContent(const char *mimetype, const char *extension);
static	void						RegisterContent(const char *mimetype, Content* (*Instantiate)());
static	void						GetRecentlyRegistered(BMessage* into);
virtual	size_t						GetMemoryUsage() = 0;

		status_t					MarkAllDirty(BRect *dirtyRect=NULL);

		int32						CountInstances();
virtual bool						IsInitialized();

protected:

virtual								~Content();
virtual	void						Cleanup();

virtual status_t					CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &msg) = 0;

private:
		/* FBC */
virtual	void						_ReservedContent1();
virtual	void						_ReservedContent2();
virtual	void						_ReservedContent3();
virtual	void						_ReservedContent4();
virtual	void						_ReservedContent5();
virtual	void						_ReservedContent6();
virtual	void						_ReservedContent7();
virtual	void						_ReservedContent8();
virtual	void						_ReservedContent9();
virtual	void						_ReservedContent10();
virtual	void						_ReservedContent11();
virtual	void						_ReservedContent12();
		uint32						_reserved[8];

		class ContentInstanceList	*m_instances;
		NestedGehnaphore			m_instanceListLock;
		Resource *					m_resource;
		ContentHandle *				m_handle;
		
};

class ContentFactory
{
public:
									ContentFactory();
virtual								~ContentFactory();

		//! Used to retrieve supported mime types/extensions and a description 
		//! of the Content.
		/*!
		 *   
		 */
virtual	void						GetIdentifiers(BMessage* into)			= 0;
virtual	Content*					CreateContent(	void* handle,
													const char* mime,
													const char* extension)	= 0;
	
		//! Returns true to try to keep your add-on loaded even when it
		//! is not being used.
virtual	bool						KeepLoaded() const;
	
		//! Returns the amount of memory used statically by this content add-on.
		/*!
		 *	That is, the amount of heap space it uses just to
		 *  be loaded but -not- any memory allocated privately by Content
		 *	or ContentInstances.
		 */
virtual	size_t						GetMemoryUsage() const;
	
private:
		/* FBC */
virtual	void						_ReservedContentFactory1();
virtual	void						_ReservedContentFactory2();
virtual	void						_ReservedContentFactory3();
virtual	void						_ReservedContentFactory4();
virtual	void						_ReservedContentFactory5();
virtual	void						_ReservedContentFactory6();
virtual	void						_ReservedContentFactory7();
virtual	void						_ReservedContentFactory8();
virtual	void						_ReservedContentFactory9();
virtual	void						_ReservedContentFactory10();
virtual	void						_ReservedContentFactory11();
virtual	void						_ReservedContentFactory12();
		uint32						_reserved[8];
};

}

// interface
typedef Wagner::ContentFactory* (*make_nth_content_type)(int32 n, image_id you, uint32 flags, ...);
extern "C" _EXPORT Wagner::ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...);

#endif
