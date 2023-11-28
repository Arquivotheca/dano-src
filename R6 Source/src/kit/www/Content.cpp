#include <Messenger.h>
#include <Autolock.h>
#include <String.h>
#include <Debug.h>
#include "BArray.h"
#include "Resource.h"
#include "Content.h"
#include "ContentManager.h"

using namespace Wagner;

struct ContentHandlerType {
	char fMime[B_MIME_TYPE_LENGTH];
	Content* (*Instantiate)();
	ContentHandlerType *fNext;
};

namespace Wagner {
	class ContentInstanceList {
	public:
		ContentInstance *ItemAt(int index) { return fArray.ItemAt(index); }
		int32 CountItems() const { return fArray.CountItems(); }
		void RemoveItem(int32 index) { fArray.RemoveItem(index); }
		void AddItem(ContentInstance *instance) { fArray.AddItem(instance); }
	private:
		BArray<ContentInstance*> fArray;
	};
}

static ContentHandlerType* handlerList = 0;
static BMessage recentlyRegistered;

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

ContentInstance::ContentInstance(Content *content, GHandler *handler)
	:	m_handler(handler), m_content(content)
{
	if (m_handler) m_handler->IncRefs(this);
	m_flags = 0;
	m_parentContent = NULL;
}

void ContentInstance::Cleanup()
{
	GetContent()->TakeMeOffYourList(this);
}

ContentInstance::~ContentInstance()
{
	if (m_handler) m_handler->DecRefs(this);
	m_handler = NULL;
	GetContent()->InstanceDeleted(this);
}

int32 ContentInstance::ID()
{
	return m_id;
}

uint32 ContentInstance::Flags()
{
	return m_flags;
}

BRect ContentInstance::FrameInParent()
{
	return m_frame;
}

int32 ContentInstance::FullWidth()
{
	return m_width;
}

int32 ContentInstance::FullHeight()
{
	return m_height;
}

void ContentInstance::SetID(int32 id)
{
	m_id = id;
}

GHandler * ContentInstance::Handler()
{
	return m_handler;
}

ContentInstance * ContentInstance::ParentContent()
{
	return m_parentContent;
}

void ContentInstance::SetParentContent(ContentInstance *par)
{
	m_parentContent = par;
}

void ContentInstance::SetHandler(GHandler *handler)
{
	if (handler == m_handler) return;
	if (handler) handler->IncRefs(this);
	if (m_handler) m_handler->DecRefs(this);
	m_handler = handler;
}

void ContentInstance::SaveState(BMessage *)
{
}

void ContentInstance::SyncToState(BMessage *)
{
}

status_t ContentInstance::FrameChanged(BRect newFrame,
									   int32 fullWidth, int32 fullHeight)
{
	m_frame = newFrame;
	m_width = fullWidth;
	m_height = fullHeight;
	return B_OK;
}

status_t ContentInstance::GetSize(int32 *width, int32 *height, uint32 *resizeFlags)
{
	*width = *height = 0;
	*resizeFlags = 0;
	return B_OK;
}

status_t ContentInstance::AttachedToView(BView */*view*/, uint32 */*contentFlags*/)
{
	return B_OK;
}

status_t ContentInstance::DetachedFromView()
{
	return B_OK;
}

status_t ContentInstance::UsurpPredecessor(ContentInstance *oldContent)
{
	if (oldContent) {
		oldContent->DetachedFromView();
		oldContent->Release();
	};
	
	return B_OK;
}

void ContentInstance::Notification(BMessage *)
{
}

status_t ContentInstance::FetchDirty(BRegion &region)
{
	m_littleLock.Lock();
	region = m_dirty;
	m_dirty.MakeEmpty();
	m_littleLock.Unlock();
	return B_OK;
}

status_t 
ContentInstance::MarkDirty(const BRegion &dirtyReg)
{
	if (!m_handler) return B_ERROR;

	m_littleLock.Lock();
	bool sendMsg = !m_dirty.CountRects();
	m_dirty.Include(&dirtyReg);
	if (!m_dirty.CountRects()) sendMsg = false;
	m_littleLock.Unlock();
	
	if (sendMsg) {
		BMessage* msg = new BMessage(bmsgContentDirty);
		msg->AddInt32("id",m_id);
		msg->AddAtomRef("instance",atomref<BAtom>(this));
		m_handler->PostMessage(msg);
	};
	return B_OK;
}

status_t ContentInstance::MarkDirty(BRect *dirtyRect)
{
	if (!m_handler) return B_ERROR;

	m_littleLock.Lock();
	bool sendMsg = !m_dirty.CountRects();
	if (dirtyRect) m_dirty.Include(*dirtyRect);
	else m_dirty.Include(m_frame.OffsetToCopy(B_ORIGIN));
	if (!m_dirty.CountRects()) sendMsg = false;
	m_littleLock.Unlock();
	
	if (sendMsg) {
		BMessage* msg = new BMessage(bmsgContentDirty);
		msg->AddInt32("id",m_id);
		msg->AddAtomRef("instance",atomref<BAtom>(this));
		m_handler->PostMessage(msg);
	};
	return B_OK;
}

Content * ContentInstance::GetContent() const
{
	return m_content;
}
		
status_t ContentInstance::ContentNotification(BMessage *msg)
{
	if (!m_handler) return B_ERROR;

	BMessage* m = new BMessage(*msg);
	m->AddInt32("id",m_id);
	m->AddAtomRef("instance",atomref<BAtom>(this));
	m_handler->PostMessage(m);

	return B_OK;
}

void ContentInstance::SetFlags(uint32 flags)
{
	m_flags = flags;
}

status_t ContentInstance::Draw(BView *, BRect )
{
	return B_OK;
}

void ContentInstance::MouseDown(BPoint /*where*/, const BMessage */*event*/)
{
}

void ContentInstance::MouseUp(BPoint /*where*/, const BMessage */*event*/)
{
}

void ContentInstance::MouseMoved(BPoint /*where*/, uint32 /*code*/, const BMessage */*a_message*/, const BMessage */*event*/)
{
}

void ContentInstance::KeyDown(const char */*bytes*/, int32 /*numBytes*/, const BMessage */*event*/)
{
}

void ContentInstance::KeyUp(const char */*bytes*/, int32 /*numBytes*/, const BMessage */*event*/)
{
}

void ContentInstance::CopyToClipboard()
{
}

void ContentInstance::SelectAll()
{
}

void ContentInstance::DeselectAll()
{
}

// -------------------------------------------------------------------------

void ContentInstance::_ReservedContentInstance1() {}
void ContentInstance::_ReservedContentInstance2() {}
void ContentInstance::_ReservedContentInstance3() {}
void ContentInstance::_ReservedContentInstance4() {}
void ContentInstance::_ReservedContentInstance5() {}
void ContentInstance::_ReservedContentInstance6() {}
void ContentInstance::_ReservedContentInstance7() {}
void ContentInstance::_ReservedContentInstance8() {}
void ContentInstance::_ReservedContentInstance9() {}
void ContentInstance::_ReservedContentInstance10() {}
void ContentInstance::_ReservedContentInstance11() {}
void ContentInstance::_ReservedContentInstance12() {}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

Content::Content()
	: m_instances(new ContentInstanceList), m_resource(NULL),
	  m_handle(NULL)
{
}

Content::Content(void* handle)
	: m_instances(new ContentInstanceList), m_resource(NULL),
	  m_handle(reinterpret_cast<ContentHandle*>(handle))
{
}

void Content::Cleanup()
{
	NestedGehnaphoreAutoLock _lock(m_instanceListLock);
	for (int32 i=m_instances->CountItems()-1;i>=0;i--) {
		m_instances->ItemAt(i)->DecRefs(this);
		m_instances->RemoveItem(i);
	}
	
	m_resource = NULL;
};

Content::~Content()
{
	if (m_handle) m_handle->Close();
	if (m_resource) m_resource->ReleaseReference();
	delete m_instances;
}

status_t Content::NewInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &userData)
{
	status_t err = CreateInstance(outInstance, handler, userData);
	if (err == B_OK) {
		NestedGehnaphoreAutoLock _lock(m_instanceListLock);
		(*outInstance)->IncRefs(this);
		m_instances->AddItem(*outInstance);
	};
	return err;
}

status_t Content::TakeMeOffYourList(ContentInstance *instance)
{
	NestedGehnaphoreAutoLock _lock(m_instanceListLock);
	for (int32 i=0;i<m_instances->CountItems();i++) {
		if (instance == m_instances->ItemAt(i)) {
			m_instances->ItemAt(i)->DecRefs(this);
			m_instances->RemoveItem(i);
			return B_OK;
		};
	};

	debugger("Content::TakeMeOffYourList.  A bad thing has happened.");	
	return B_ERROR;
}

void Content::InstanceDeleted(ContentInstance *)
{
	if (m_resource) m_resource->ReleaseReference();
}

status_t Content::MarkAllDirty(BRect *dirtyRect=NULL)
{
	status_t err = B_OK;
	NestedGehnaphoreAutoLock _lock(m_instanceListLock);
	for (int32 i=0;i<m_instances->CountItems();i++) {
		status_t thisErr = m_instances->ItemAt(i)->MarkDirty(dirtyRect);
		if (thisErr) err = thisErr;
	};
	
	return err;
}

status_t Content::NotifyInstances(BMessage *msg)
{
	status_t err = B_OK;
	NestedGehnaphoreAutoLock _lock(m_instanceListLock);
	for (int32 i=0;i<m_instances->CountItems();i++) {
		status_t thisErr = m_instances->ItemAt(i)->ContentNotification(msg);
		if (thisErr) err = thisErr;
	};
	
	return err;
}

void Content::SetResource(Resource *resource)
{
	m_resource = resource;
}

Resource * Content::GetResource() const
{
	return m_resource;
}


int32 Content::CountInstances()
{
	return m_instances->CountItems();
}

bool Content::IsInitialized()
{
	return true;
}

Content* Content::InstantiateContent(const char *mime, const char *extension)
{
	Content* content = ContentManager::Default().InstantiateContent(mime, extension);
	if( content ) return content;
	
	BAutolock _lock(ContentManager::Default().Locker());
	for (ContentHandlerType *handler = handlerList; handler;
		handler = handler->fNext)
		if (strcasecmp(handler->fMime, mime) == 0) 
			return handler->Instantiate();

	return 0;
}

void Content::RegisterContent(const char *mime, Content* (*Instantiate)())
{
	BAutolock _lock(ContentManager::Default().Locker());
	ContentHandlerType *handler = new ContentHandlerType;
	strcpy(handler->fMime, mime);
	handler->Instantiate = Instantiate;
	handler->fNext = handlerList;
	handlerList = handler;
	recentlyRegistered.AddString(S_CONTENT_MIME_TYPES, mime);
}

void Content::GetRecentlyRegistered(BMessage* into)
{
	BAutolock _lock(ContentManager::Default().Locker());
	*into = recentlyRegistered;
	recentlyRegistered.MakeEmpty();
}

// -------------------------------------------------------------------------

void Content::_ReservedContent1() {}
void Content::_ReservedContent2() {}
void Content::_ReservedContent3() {}
void Content::_ReservedContent4() {}
void Content::_ReservedContent5() {}
void Content::_ReservedContent6() {}
void Content::_ReservedContent7() {}
void Content::_ReservedContent8() {}
void Content::_ReservedContent9() {}
void Content::_ReservedContent10() {}
void Content::_ReservedContent11() {}
void Content::_ReservedContent12() {}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

ContentFactory::ContentFactory()
{
}

ContentFactory::~ContentFactory()
{
}

bool ContentFactory::KeepLoaded() const
{
	return false;
}

size_t ContentFactory::GetMemoryUsage() const
{
	return 0;
}

// -------------------------------------------------------------------------

void ContentFactory::_ReservedContentFactory1() {}
void ContentFactory::_ReservedContentFactory2() {}
void ContentFactory::_ReservedContentFactory3() {}
void ContentFactory::_ReservedContentFactory4() {}
void ContentFactory::_ReservedContentFactory5() {}
void ContentFactory::_ReservedContentFactory6() {}
void ContentFactory::_ReservedContentFactory7() {}
void ContentFactory::_ReservedContentFactory8() {}
void ContentFactory::_ReservedContentFactory9() {}
void ContentFactory::_ReservedContentFactory10() {}
void ContentFactory::_ReservedContentFactory11() {}
void ContentFactory::_ReservedContentFactory12() {}

