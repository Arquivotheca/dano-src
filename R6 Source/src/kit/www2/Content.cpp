#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <support2/Handler.h>
#include <support2/String.h>

#include <www2/Resource.h>
#include <www2/Content.h>
#include <www2/ContentManager.h>

using namespace B::WWW2;

struct ContentHandlerType {
	char fMime[B_MIME_TYPE_LENGTH];
	Content* (*Instantiate)();
	ContentHandlerType *fNext;
};

static ContentHandlerType* handlerList = 0;
static BMessage recentlyRegistered;

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

ContentInstance::ContentInstance(Content *content, BHandler *handler)
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

BHandler * ContentInstance::Handler()
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

void ContentInstance::SetHandler(BHandler *handler)
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
#warning "Fix ContentInstance::MarkDirty"
//	if (!m_handler) return B_ERROR;
//
//	m_littleLock.Lock();
//	bool sendMsg = !m_dirty.CountRects();
//	m_dirty.Include(&dirtyReg);
//	if (!m_dirty.CountRects()) sendMsg = false;
//	m_littleLock.Unlock();
//	
//	if (sendMsg) {
//		BMessage* msg = new BMessage(bmsgContentDirty);
//		msg->AddInt32("id",m_id);
//		msg->AddAtomRef("instance",atomref<BAtom>(this));
//		m_handler->PostMessage(msg);
//	};
	return B_OK;
}

status_t ContentInstance::MarkDirty(BRect *dirtyRect)
{
#warning "Fix ContentInstance::MarkDirty"
//	if (!m_handler) return B_ERROR;
//
//	m_littleLock.Lock();
//	bool sendMsg = !m_dirty.CountRects();
//	if (dirtyRect) m_dirty.Include(*dirtyRect);
//	else m_dirty.Include(m_frame.OffsetToCopy(B_ORIGIN));
//	if (!m_dirty.CountRects()) sendMsg = false;
//	m_littleLock.Unlock();
//	
//	if (sendMsg) {
//		BMessage* msg = new BMessage(bmsgContentDirty);
//		msg->AddInt32("id",m_id);
//		msg->AddAtomRef("instance",atomref<BAtom>(this));
//		m_handler->PostMessage(msg);
//	};
	return B_OK;
}

Content * ContentInstance::GetContent() const
{
	return m_content;
}
		
status_t ContentInstance::ContentNotification(BMessage *msg)
{
#warning "ContentInstance::ContentNotification"
	if (!m_handler) return B_ERROR;

	BMessage* m = new BMessage(*msg);
//	m->AddInt32("id",m_id);
//	m->AddAtomRef("instance",atomref<BAtom>(this));
//	m_handler->PostMessage(m);

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
	: m_resource(NULL),
	  m_handle(NULL)
{
}

Content::Content(void* handle)
	: m_resource(NULL),
	  m_handle(reinterpret_cast<ContentHandle*>(handle))
{
}

void Content::Cleanup()
{
	m_instanceListLock.Lock();

	for (int32 i = m_instances.CountItems() - 1 ; i >= 0 ; i--) {
		m_instances.ItemAt(i)->DecRefs(this);
		m_instances.RemoveItemsAt(i);
	}

	m_instanceListLock.Unlock();
	
	m_resource = NULL;
};

Content::~Content()
{
	if (m_handle) m_handle->Close();
#warning "Fix me"
//	if (m_resource) m_resource->ReleaseReference();
}

status_t Content::NewInstance(ContentInstance **outInstance, BHandler *handler, const BMessage &userData)
{
	status_t err = CreateInstance(outInstance, handler, userData);
	if (err == B_OK) {
		m_instanceListLock.Lock();
		
		(*outInstance)->IncRefs(this);
		m_instances.AddItem(*outInstance);
	
		m_instanceListLock.Unlock();
	};
	return err;
}

status_t Content::TakeMeOffYourList(ContentInstance *instance)
{
	m_instanceListLock.Lock();
	
	for (int32 i = 0 ; i < m_instances.CountItems() ; i++) {
		if (instance == m_instances.ItemAt(i)) {
			m_instances.ItemAt(i)->DecRefs(this);
			m_instances.RemoveItemsAt(i);
			return B_OK;
		};
	};
	
	m_instanceListLock.Unlock();
	
	debugger("Content::TakeMeOffYourList.  A bad thing has happened.");	
	return B_ERROR;
}

void Content::InstanceDeleted(ContentInstance *)
{
#warning "Fix me"
//	if (m_resource) m_resource->ReleaseReference();
}

status_t Content::MarkAllDirty(BRect *dirtyRect=NULL)
{
	status_t err = B_OK;
	
	m_instanceListLock.Lock();
	
	for (int32 i = 0 ; i < m_instances.CountItems() ; i++) {
		status_t thisErr = m_instances.ItemAt(i)->MarkDirty(dirtyRect);
		if (thisErr) err = thisErr;
	};
	
	m_instanceListLock.Unlock();
	
	return err;
}

status_t Content::NotifyInstances(BMessage *msg)
{
	status_t err = B_OK;
	
	m_instanceListLock.Lock();
	
	for (int32 i = 0 ; i < m_instances.CountItems() ; i++) {
		status_t thisErr = m_instances.ItemAt(i)->ContentNotification(msg);
		if (thisErr) err = thisErr;
	};
	
	m_instanceListLock.Unlock();
	
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
	return m_instances.CountItems();
}

bool Content::IsInitialized()
{
	return true;
}

Content* Content::InstantiateContent(const char *mime, const char *extension)
{
	Content* content = ContentManager::Default().InstantiateContent(mime, extension);
	if( content ) return content;
	
	ContentManager::Default().Locker()->Lock();
	
	for (ContentHandlerType *handler = handlerList ; handler ; handler = handler->fNext) {
		if (strcasecmp(handler->fMime, mime) == 0) {
			ContentManager::Default().Locker()->Unlock();
			return handler->Instantiate();
		}
	}

	ContentManager::Default().Locker()->Unlock();
	return 0;
}

void Content::RegisterContent(const char *mime, Content* (*Instantiate)())
{
	ContentManager::Default().Locker()->Lock();
	ContentHandlerType *handler = new ContentHandlerType;
	strcpy(handler->fMime, mime);
	handler->Instantiate = Instantiate;
	handler->fNext = handlerList;
	handlerList = handler;
#warning "Fixme Content::RegisterContent"
//	recentlyRegistered.AddString(S_CONTENT_MIME_TYPES, mime);
	ContentManager::Default().Locker()->Unlock();
}

void Content::GetRecentlyRegistered(BMessage* into)
{
	ContentManager::Default().Locker()->Lock();
	*into = recentlyRegistered;
#warning "Fixme Content::GetRecentlyRegistered"
//	recentlyRegistered.MakeEmpty();
	ContentManager::Default().Locker()->Unlock();
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

