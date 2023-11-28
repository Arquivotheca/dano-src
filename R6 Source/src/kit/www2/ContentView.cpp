#include <www2/Content.h>
#include <www2/ContentView.h>
#include <www2/History.h>
#include <www2/Resource.h>
#include <www2/ResourceCache.h>
#include <www2/StringBuffer.h>
#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Binder.h>
#include <Bitmap.h>
#include <Debug.h>
#include <MenuField.h>
#include <Message.h>
#include <MessageQueue.h>
#include <PrintJob.h>
#include <RadioButton.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <stdio.h>
#include <StopWatch.h>
#include <string.h>
#include <support2/Vector.h>
#include <TextView.h>
#include <typeinfo>
#include <Window.h>
#include <www/TellBrowser.h>


using namespace B::WWW2;

static BView *FindViewCaselessN(BView *root, const char *name, int length);

const char *kSpecialFramePrefix = "_be:content";
// This is a hack for demos and stuff.  When an attempt is made to load a specific
// URL, it silently redirects to another. The redirect list get's populated
// by iterating through the binder node /service/web/redirect.
struct RiggedRedirect {
	BUrl src;
	BUrl dest;
};
//	{"http://www.sony.com", "http://www.sony.com/index3.htm"},

extern void ViewSource(const BUrl &url);

class GHandlerBridgeTarget
{
	private:
				Gehnaphore		m_lock;
				GMessageQueue	m_q;
	public:
		virtual	void			QueueMessage(BMessage *msg) { GehnaphoreAutoLock _(m_lock); m_q.QueueMessage(new BMessage(msg)); };
				BMessage *		DequeueMessage() { GehnaphoreAutoLock _(m_lock); return m_q.DequeueMessage(); };
};

class GHandlerBridge : public GHandler
{

	private:

				Gehnaphore				m_lock;
				GHandlerBridgeTarget *	m_target;

	public:
										GHandlerBridge(GHandlerBridgeTarget *target)
											: GHandler("BView bridge")
										{
											m_target = target;
										};
	
										~GHandlerBridge() {
										};

				void					Cleanup() {
											m_lock.Lock();
											m_target = NULL;
											GHandler::Cleanup();
											m_lock.Unlock();
										};
										
		virtual	status_t				HandleMessage(BMessage *message) {
											m_lock.Lock();
											if (m_target) m_target->QueueMessage(message);
											m_lock.Unlock();
											return B_OK;
										};
};

namespace B {
namespace WWW2 {

const uint32 FORCE_REGET = 16;

enum {
	cvfHScrollbarVisible	= 0x00010000,
	cvfVScrollbarVisible	= 0x00020000,
	cvoRequestPending		= 0x00040000,
	cvfBridgeQueued			= 0x00080000,
	cvfContentDisplayable	= 0x00100000,
	cvfContentLayedout		= 0x00200000
};

class RealContentView : public BView, public GHandlerBridgeTarget
{
				atom<ContentInstance>	m_contentInstance;
				ContentView *			m_parent;
				History *				m_history;
				int32					m_width,m_height;
				uint32					m_flags;
				int32					m_id;
				int32					m_usingId;
				uint32					m_contentResizeFlags;
				BRect					m_contentRect;
				int32					m_contentError;
				int32					m_index;
				BUrl					m_requestedURL;
				GHandlerBridge *		m_bridge;
				int32					m_lastContentWidth;
				int32					m_lastContentHeight;
				bool					m_needHScroll;
				bool					m_needVScroll;
				bool					m_moreHoriz;
				bool					m_moreVert;
				float					m_restoreHPos;
				float					m_restoreVPos;
				static BBitmap *		m_backBuffer;
				static BView *			m_backView;
				static BLocker			m_backBufferCreateLock;
				BRegion					m_dirtyRegion;
				static BVector<RiggedRedirect> m_redirects;
				static bool				m_populatedRedirects;
				
				void					TweakScrollbars(bool fromLayout=false);
				BView*					PickFocusControl();
				void					MoveFocusHere(bool include_content=false);
				bool					UpdateContentRect();
				void					BuildRedirectList();
				bool					IsURLInAncestry(const BUrl &url);
				
		virtual	void			QueueMessage(BMessage *msg) {
			BMessage m('brig');
			GHandlerBridgeTarget::QueueMessage(msg);
			if (!(atomic_or((int32*)&m_flags,cvfBridgeQueued) & cvfBridgeQueued)) BMessenger(this).SendMessage(&m,(BHandler*)NULL,0);
		};

	public:
										RealContentView(BRect r, const char *name, uint32 follow, uint32 flags, uint32 options, ContentView *parent, History *history);
		virtual							~RealContentView();

				status_t				SetContent(const BUrl &url, uint32 flags, GroupID groupID, BMessage *userData=NULL);
				status_t				SetContent(ContentInstance *content);
				ContentInstance *		GetContentInstance() const { return m_contentInstance; };

				void					MoreSpace(bool h, bool v);
				
				RealContentView *		ParentContentView();
				RealContentView *		TopContentView();
				RealContentView *		RealTopContentView();
				bool					LinkTo(const char *target, BUrl &url, GroupID groupID, uint32 flags, bool goingDown=false, BMessage *msg=NULL);

				void					SetIndex(int32 index) { m_index = index; };
				int32					Index() { return m_index; };
				History *				GetHistory();
				status_t				GetFramePath(History::FramePath *framePath);
				void					RestoreContent(BMessage *restore);

		virtual	void					Draw(BRect updateRect);
		virtual	void					AttachedToWindow();
		virtual void					DetachedFromWindow();
		virtual	void					FrameResized(float width, float height);
		virtual	void					MessageReceived(BMessage *msg);

		virtual	void					MouseDown(BPoint where);
		virtual	void					MouseUp(BPoint where);
		virtual	void					MouseMoved(	BPoint where, uint32 code, const BMessage *a_message);
		virtual	void					KeyDown(const char *bytes, int32 numBytes);
		virtual	void					KeyUp(const char *bytes, int32 numBytes);
		
				void					Broadcast(const BMessage *msg);

				void					Close();
				void					Reload(bool flush_cache);
				void					GoForward();
				void					GoBackward();
				void					GoOffset(int32 offset);
				void					GoNearest(const char *nearest);
				void					Rewind();
				void					Print();
				void 					GetURL(BUrl *url);

				void					GetPreviousURL(BUrl *url);
				void					GetNextURL(BUrl *url);
				void					GetHistoryLength(int32 *length);
				void					ClearHistory();

				void					StopLoading();
				
				bool					IsPrivate() const;
};

} } // nanepsace B::WWW2

using namespace B::WWW2;

BBitmap* RealContentView::m_backBuffer = 0;
BView*	RealContentView::m_backView = 0;
BLocker	RealContentView::m_backBufferCreateLock("back buffer create");
BVector<RiggedRedirect> RealContentView::m_redirects;
bool RealContentView::m_populatedRedirects = false;

RealContentView::RealContentView(BRect r, const char *name, uint32 follow, uint32 flags, uint32 options, ContentView *parent, History *history)
	: BView(r,name,follow,flags|B_WILL_DRAW|B_FRAME_EVENTS|B_SUBPIXEL_PRECISE)
{
	if (m_backBuffer == 0) {
		BAutolock _lock(&m_backBufferCreateLock);
		if (m_backBuffer == 0) {
			// The backbuffer is shared by all content views, and protected by the bitmap's lock
			BScreen screen(Window());
			m_backBuffer = new BBitmap(screen.Frame(), B_BITMAP_ACCEPTS_VIEWS|0x40000000, screen.ColorSpace());
			m_backView = new BView(screen.Frame(), NULL, B_FOLLOW_NONE, B_SUBPIXEL_PRECISE);
			m_backBuffer->Lock();
			m_backBuffer->AddChild(m_backView);
			m_backBuffer->Unlock();
		}
	}

	m_contentInstance = NULL;
	m_flags = options;
	m_usingId = m_id = 0;
	m_parent = parent;
	m_contentResizeFlags = 0;
	m_contentError = 0;
	m_history = NULL;
	m_index = -99999;
	m_history = history;

	m_bridge = NULL;
	m_lastContentWidth = m_lastContentHeight = 0;
	m_needHScroll = m_needVScroll = false;
	m_moreHoriz = m_moreVert = false;
	m_restoreHPos = m_restoreVPos = 0.;
	
	if (!RealContentView::m_populatedRedirects) {
		BuildRedirectList();
		RealContentView::m_populatedRedirects = true;
	}
}

RealContentView::~RealContentView()
{
}

void RealContentView::Broadcast(const BMessage *msg)
{
	if (m_contentInstance) m_contentInstance->Notification(const_cast<BMessage*>(msg));

	ContentView *cv;
	RealContentView *rcv;
	BView *bv = ChildAt(0);
	while (bv) {
		cv = dynamic_cast<ContentView*>(bv);
		if (cv) {
			rcv = cv->GetRealContentView();
			if (rcv) rcv->Broadcast(msg);
		};
		bv = bv->NextSibling();
	};
}

void RealContentView::AttachedToWindow()
{
	BRect bounds = Bounds();
	SetViewColor(B_TRANSPARENT_COLOR);
	FrameResized(bounds.Width(), bounds.Height());
	m_bridge = new GHandlerBridge(this);
	m_bridge->Acquire(this);

	BMessage restore;
	if (m_history && m_history->WasRestored())
		Looper()->PostMessage(bmsgGoBack, this);	// Reload page user was on
}

RealContentView * RealContentView::ParentContentView()
{
	if (m_flags & cvoIsTop) return NULL;
	RealContentView *v = dynamic_cast<RealContentView*>(m_parent->Parent());
	return v;
};

RealContentView * RealContentView::TopContentView()
{
	if (m_flags & cvoIsTop) return this;
	RealContentView *v = ParentContentView();
	if (!v) return this;
	return v->TopContentView();
};

RealContentView * RealContentView::RealTopContentView()
{
	RealContentView *parent,*v = this;
	while ((parent = dynamic_cast<RealContentView*>(v->m_parent->Parent())) != NULL)
		v = parent;
	return v;
};

void RealContentView::DetachedFromWindow()
{
	if (m_contentInstance)	m_contentInstance->DetachedFromView();
	if (atomic_and((int32*)&m_flags,~cvoRequestPending) & cvoRequestPending)
		resourceCache.CancelRequest(m_requestedURL, m_id, m_bridge);
	m_bridge->Release(this);
	resourceCache.CancelPendingMessages(this);
	Window()->DequeueAll();
	m_contentInstance = NULL;
}

void RealContentView::Close()
{
	if (m_flags & cvoIsClosable)
		Window()->Close();
	else
		windowManager.CloseWindow(Name());
};

status_t RealContentView::GetFramePath(History::FramePath *framePath)
{
	status_t err = B_OK;
	if (Index() < 0) return B_ERROR;
	if (ParentContentView()) {
		err = ParentContentView()->GetFramePath(framePath);
		if (framePath->depth >= framePath->maxDepth) return B_ERROR;
		if (err == B_OK) framePath->indices[framePath->depth++] = Index();
	} else {
		framePath->depth = 0;
	};
	return B_OK;
}

History * RealContentView::GetHistory()
{
	RealContentView *v=this;
	while (v->ParentContentView() && !v->m_history) v = v->ParentContentView();
	return v->m_history;
};

bool RealContentView::LinkTo(const char *target, BUrl &url, GroupID groupID,
	uint32 flags, bool goingDown, BMessage *msg)
{
	RealContentView *v = NULL;
	
	if (!goingDown) {
		bool setReferrer = false;
		
		msg->FindBool("setReferrer", &setReferrer);
		
		if (!target || !strcasecmp(target,Name()) || !strcasecmp(target,"_self")) {
			if (m_contentInstance && setReferrer)
				m_contentInstance->GetContent()->GetResource()->GetURL().AddToMessage("referrer", msg);

			SetContent(url, flags, groupID, msg);
			return true;
		} else if (!strcasecmp(target,"_parent")) {
			if (!strncasecmp(kSpecialFramePrefix, Name(), strlen(kSpecialFramePrefix))) {
				// If my name starts with "_be:content", I am treated as the top level
				// frame as far as all subcontent in concerned.  In this case,
				// don't iterate any higher.
				return false;
			}
			
			if ((v = ParentContentView()))
				return v->LinkTo(NULL,url,groupID,flags,false,msg);

			return false;
		} else {
			v = ParentContentView();
			if (v && strncasecmp(kSpecialFramePrefix, Name(), strlen(kSpecialFramePrefix)) != 0)
				return v->LinkTo(target,url,groupID,flags,false,msg);
			else if (!strcasecmp(target,"_top") || !strcasecmp(target,"_blank")) {
				if (m_contentInstance && setReferrer)
					m_contentInstance->GetContent()->GetResource()->GetURL().AddToMessage("referrer", msg);
				SetContent(url, flags, groupID, msg);
				return true;
			};
		};
	};	

	/* Has an absolutely named target.  We need to search. */
	ContentView *cv;
	RealContentView *rcv;
	BView *bv = ChildAt(0);
	while (bv) {
		cv = dynamic_cast<ContentView*>(bv);
		if (cv) {
			rcv = cv->GetRealContentView();
			if (rcv) {
				if (!strcasecmp(target,rcv->Name())) {
					rcv->SetContent(url,flags,groupID,msg);
					return true;
				} else {
					if (rcv->LinkTo(target,url,groupID,flags,true,msg)) return true;
				};
			};
		};
		bv = bv->NextSibling();
	};
	return false;
};

void RealContentView::MessageReceived(BMessage *_msg)
{
	BMessage *msg = _msg;
	if (m_contentInstance && msg->WasDropped()) {
		// forward drag-n-drop messages to the ContentInstance if it is a GHandler
		ContentInstance *i = m_contentInstance;
		GHandler *handler = dynamic_cast<GHandler*>(i);
		if (handler) {
			BMessage transformed(*msg);
			BPoint p = transformed.DropPoint();
			transformed.RemoveName("_drop_point_");
			ConvertFromScreen(&p);
			transformed.AddPoint("_drop_point_", p);
			handler->PostMessage(transformed);
		}
		BView::MessageReceived(msg);
		return;
	}
	if (msg->what == 'brig') {
		atomic_and((int32*)&m_flags,~cvfBridgeQueued);
		msg = DequeueMessage();
	}

	if (!msg) return;
	
//	msg->PrintToStream();

	dispatch:
	
	int32 id;
	switch (msg->what) {
		case NEW_CONTENT_INSTANCE: {
			atom<ContentInstance> content;
			if (msg->FindAtom("instance", content) == B_OK) {
				int32 contentWidth = 0, contentHeight = 0;
				content->GetSize(&contentWidth, &contentHeight, &m_contentResizeFlags);
				if (msg->FindInt32("id", &id) == B_OK && id == m_id) {
					atomic_and((int32*)&m_flags, ~cvoRequestPending);
					if ((m_contentResizeFlags & IS_DOCKABLE) != 0) {
						// This is a dockable content.  Need to dock it.
						// Remove this entry from the history (If this frame
						// has one.  It won't if it's a javascript pane).
						if (GetHistory())
							GetHistory()->RemoveCurrentEntry();

						// Send a message to the top view about this, and
						// delete this instance.
						BMessage notify('dock');
						StringBuffer urlString;
						urlString << content->GetContent()->GetResource()->GetURL();
						notify.AddString("url", urlString.String());
						BView *top = this;
						while (top->Parent())
							top = top->Parent();

						dynamic_cast<ContentView*>(top)->GetRealContentView()
							->GetContentInstance()->Notification(&notify);
					} else {
						SetContent(content);
					}
				}
			}
			break;
		}

		case CONTENT_INSTANCE_ERROR: {
			atomic_and((int32*)&m_flags, ~cvoRequestPending);
				if (GetHistory())
					GetHistory()->RemoveCurrentEntry();

			m_contentError = -1;
			Invalidate();
		} break;
		
		case bmsgLayoutCompletion: {
			if (msg->FindInt32("id", &id) == B_OK && id == m_usingId) {
				// if we're the top content in an alert window,
				// resize to show as much as possible
				//
				if (Window() && Window()->Feel() == B_MODAL_APP_WINDOW_FEEL &&
					ParentContentView() == NULL && m_contentInstance)
				{
					BRect screenRect = BScreen().Frame();
					BRect winRect = Window()->Frame();
					int32 contentWidth = 0, contentHeight = 0;
					int32 availHeight;

					m_contentInstance->GetSize(&contentWidth, &contentHeight, &m_contentResizeFlags);
					if (winRect.top + contentHeight > screenRect.bottom)
						availHeight = (int32)(screenRect.bottom - winRect.top);
					else
						availHeight = contentHeight;

					Window()->ResizeTo(winRect.Width(), availHeight);
					m_needHScroll = false;
					m_needVScroll = false;
					m_moreHoriz = m_moreVert = false;
					m_parent->SetScrollbarState(false, contentHeight > availHeight);
				}
				else {
					if( m_moreHoriz ) {
						m_needHScroll = false;
					}
					if( m_moreVert ) {
						m_needVScroll = false;
					}
					m_moreHoriz = m_moreVert = false;
					
					TweakScrollbars(true);
				}
			}
		} break;

		case bmsgContentDisplayable: {
			m_flags |= cvfContentDisplayable;
			MoveFocusHere();
			if (m_dirtyRegion.CountRects()) {
				for (int32 i=0;i<m_dirtyRegion.CountRects();i++)
					Invalidate(m_dirtyRegion.RectAt(i).OffsetByCopy(m_contentRect.LeftTop()));
				m_dirtyRegion.MakeEmpty();
				TweakScrollbars();
			}
		} break;

		case bmsgContentDirty: {
			if (msg->FindInt32("id", &id) == B_OK && id == m_usingId) {
				BRegion region;
				m_contentInstance->FetchDirty(region);
				m_dirtyRegion.Include(&region);
				if (m_flags & cvfContentDisplayable) {
					for (int32 i=0;i<m_dirtyRegion.CountRects();i++)
						Invalidate(m_dirtyRegion.RectAt(i).OffsetByCopy(m_contentRect.LeftTop()));
					m_dirtyRegion.MakeEmpty();
				}
			}
			TweakScrollbars();
		} break;
		
		case bmsgContentLoaded: {
			MoveFocusHere();
		} break;
		
		case bmsgLinkTo: {
			GroupID groupID;
			BUrl url;
			url.ExtractFromMessage("url",msg);
			if (msg->FindInt32("requestor_groupid", (int32*) &groupID) != B_OK) {
				printf("WARNING: Add requestor_groupid to bmsgLinkTo message\n");
				groupID = securityManager.GetGroupID(url);
			}
			int32 flags = msg->FindInt32("flags") | LOAD_ON_ERROR | RECORD_IN_HISTORY;
			if (!LinkTo(msg->FindString("target"),url,groupID,flags,false,msg) &&
				!LinkTo("_top",url,groupID,flags,false,msg))
				SetContent(url, flags, groupID, msg);
		} break;

		case bmsgClearHistory: {
			TopContentView()->ClearHistory();
		} break;
		
		case bmsgGoBack: {
			TopContentView()->GoBackward();
		} break;
		case bmsgGoForward: {
			TopContentView()->GoForward();
		} break;
		case bmsgGoOffset: {
			int32 offset = 0;
			if(msg->FindInt32("offset",&offset)==B_OK) {
				TopContentView()->GoOffset(offset);
			}
		} break;
		case bmsgGoNearest: {
			const char *nearest = 0;
			if(msg->FindString("nearest",&nearest)==B_OK) {
				TopContentView()->GoNearest(nearest);
			}
		} break;
		case bmsgReload: {
			Reload(msg->FindBool("flush_cache"));
		} break;
		case bmsgStop: {
			TopContentView()->StopLoading();
		} break;
		case bmsgJSCloseWindow: {
			Close();
		} break;
		case bmsgBroadcast: {
			BMessage not;
			if (msg->FindMessage("notification", &not) == B_OK ) {
				#if DEBUG
				PRINT(("Sending broadcast!"));
				not.PrintToStream();
				#endif
				Broadcast(&not);
			}
		} break;
		case bmsgNotifyInstance: {
			BMessage not;
			if( m_contentInstance &&
					msg->FindMessage("notification", &not) == B_OK ) {
				#if DEBUG
				PRINT(("Sending notification to %s: ", Name()));
				not.PrintToStream();
				#endif
				m_contentInstance->Notification(&not);
			}
		} break;
		case bmsgStateChange: {
			int32 vflags = msg->FindInt32("vflags");
			if ((Flags() & B_DRAW_ON_CHILDREN) != (vflags & B_DRAW_ON_CHILDREN)) {
				SetFlags(vflags);
				Invalidate();
			};
		} break;
		case B_COPY: {
			m_contentInstance->CopyToClipboard();
		} break;
		case B_SELECT_ALL: {
			m_contentInstance->SelectAll();
		} break;
		case bmsgDeselectAll: {
			m_contentInstance->DeselectAll();
		} break;
		case B_MOUSE_WHEEL_CHANGED: {
			BView::MessageReceived(msg);
			BPoint location;
			uint32 buttons;
			GetMouse(&location,&buttons);
			MouseMoved(location,Bounds().Contains(location) ? B_INSIDE_VIEW : B_OUTSIDE_VIEW,0);
		} break;
		default:
			BView::MessageReceived(msg);
	};
	
	if (msg != _msg) delete msg;
	msg = DequeueMessage();
	if (msg) goto dispatch;
}

void RealContentView::Draw(BRect updateRect)
{
	BRegion reg;
	BRect bounds = Bounds();
	reg.Include(bounds);
	BView *child = ChildAt(0);
	while (child) {
		if ((dynamic_cast<ContentView*>(child) || (child->Name() && (child->Name()[0] == '*'))) &&
			!child->IsHidden())
			reg.Exclude(child->Frame().OffsetByCopy(bounds.LeftTop()));
		child = child->NextSibling();
	};
	ConstrainClippingRegion(&reg);

	if (m_contentInstance) {
		// Draw borders depending on flags
		SetHighColor(0, 0, 0);
		const BRect b(Bounds());
		if ((m_contentResizeFlags & STRETCH_HORIZONTAL) == 0) {
			if(b.left <= m_contentRect.left-1 ) {
				FillRect(BRect(b.left, b.top, m_contentRect.left-1, b.bottom));
			}
			if( m_contentRect.right+1 <= b.right ) {
				FillRect(BRect(m_contentRect.right+1, b.top, b.right, b.bottom));
			}
		}
		
		if ((m_contentResizeFlags & STRETCH_VERTICAL) == 0) {
			if( b.top <= m_contentRect.top-1 ) {
				FillRect(BRect(b.left, b.top, b.right, m_contentRect.top-1));
			}
			if( m_contentRect.bottom+1 <= b.bottom ) {
				FillRect(BRect(b.left, m_contentRect.bottom+1, b.right, b.bottom));
			}
		}

		// Now draw the real shit
		if ((IsPrinting() == false) && (m_flags & cvoDoubleBuffer) && (m_contentInstance->Flags() & cifDoubleBuffer)) {
			BRect r = updateRect.OffsetToCopy(B_ORIGIN);
			if (r.Width() >= 0 && r.Height() >= 0) {
				BPoint p(-updateRect.left,-updateRect.top);
				m_backBuffer->Lock();
				m_backView->SetOrigin(p);
				m_backView->PushState();
				if ((m_contentInstance->Flags() & cifHasTransparency)) {
					// Need to erase background of content instance.
					m_backView->SetHighColor(0, 0, 0);
					m_backView->FillRect(updateRect);
				}

				m_contentInstance->Draw(m_backView, updateRect);
				m_backView->PopState();
				m_backView->Sync();
				DrawBitmap(m_backBuffer, updateRect.LeftTop());
				m_backBuffer->Unlock();
			}
		} else {
			PushState();
			if ((m_contentInstance->Flags() & cifHasTransparency)) {
				// Need to erase background of content instance.
				SetHighColor(0, 0, 0);
				FillRect(updateRect);
			}
			m_contentInstance->Draw(this,updateRect);
			PopState();
		};
	} else {
		SetHighColor(0, 0, 0);
		FillRect(updateRect);
	};

	ConstrainClippingRegion(NULL);
}

void RealContentView::FrameResized(float , float )
{
	float newWidth = (int32) (Bounds().Width());
	float newHeight = (int32) (Bounds().Height());

	m_width = (int32)newWidth;	
	m_height = (int32)newHeight;	

	if( UpdateContentRect() ) Invalidate();
	if (m_contentInstance) {
		int32 fullWidth = m_width;
		int32 fullHeight = m_height;
		if( m_parent ) m_parent->GetAvailSize(&fullWidth, &fullHeight);
		if (m_contentInstance->FrameChanged(m_contentRect,
											fullWidth, fullHeight) == B_OK) {
			if( m_moreHoriz ) {
				m_needHScroll = false;
			}
			if( m_moreVert ) {
				m_needVScroll = false;
			}
			m_moreHoriz = m_moreVert = false;
			
			TweakScrollbars(true);
		} else {
			m_lastContentWidth = m_lastContentHeight = 0;
		}
	}
}

void RealContentView::TweakScrollbars(bool fromLayout)
{
	bool h=false,v=false;

	if( !fromLayout ) m_parent->GetScrollbarState(&h, &v);
	
	BScrollBar *hsb = ScrollBar(B_HORIZONTAL);
	BScrollBar *vsb = ScrollBar(B_VERTICAL);
	
	// anything to do?
	if (!hsb && !vsb) return;
	
	int32 availWidth, availHeight;
	m_parent->GetAvailSize(&availWidth, &availHeight);
	
	int32 scrollWidth, scrollHeight;
	m_parent->GetScrollbarDimens(&scrollWidth, &scrollHeight);
	
	int32 contentWidth = 0, contentHeight = 0;
	if (m_contentInstance) {
		m_contentInstance->GetSize(&contentWidth, &contentHeight, &m_contentResizeFlags);
		
		// If size of content decreased, we have to completely
		// re-evaluate whether scroll bars are needed.
		if( contentWidth < m_lastContentWidth ) m_needHScroll = false;
		if( contentHeight < m_lastContentHeight ) m_needVScroll = false;
	}

	if( hsb && m_needHScroll ) h = true;
	if( vsb && m_needVScroll ) v = true;
	
	if( hsb && !m_needHScroll ) {
		// If we don't yet know that we absolutely need the horizontal
		// scroll bar, see what to do with it.
		if( contentWidth > availWidth ) {
			m_needHScroll = true;
			h = true;
			MoveFocusHere(true);
		} else if( v && contentWidth > (availWidth-scrollWidth) ) {
			h = true;
		} else {
			h = false;
		}
	}
	
	if( vsb && !m_needVScroll ) {
		// If we don't yet know that we absolutely need the vertical
		// scroll bar, see what to do with it.
		if( contentHeight > availHeight ) {
			m_needVScroll = true;
			v = true;
			MoveFocusHere(true);
		} else if( h && contentHeight > (availHeight-scrollHeight) ) {
			v = true;
		} else {
			v = false;
		}
	}
	
	if( h ) availHeight -= scrollHeight;
	if( v ) availWidth -= scrollWidth;
	
	if (hsb) {
		if (!h) {
			hsb->SetProportion(0);
			hsb->SetRange(0,0);
		} else {
			hsb->SetProportion(((float)availWidth)/contentWidth);
			hsb->SetRange(0,contentWidth-availWidth);
			
			if(m_restoreHPos != 0.) {
				hsb->SetValue(m_restoreHPos);
				m_restoreHPos = 0.;
			}
			
			// Set steps.
			float fw = be_plain_font->StringWidth("00");
			float large_step = Bounds().Width() - fw;
			if( large_step < 0 ) large_step = Bounds().Width();
			hsb->SetSteps(ceil(fw), ceil(large_step));
		};
	};

	if (vsb) {
		if (!v) {
			vsb->SetProportion(0);
			vsb->SetRange(0,0);
		} else {
			vsb->SetProportion(((float)availHeight)/contentHeight);
			vsb->SetRange(0,contentHeight-availHeight);
			
			if(m_restoreVPos != 0.) {
				vsb->SetValue(m_restoreVPos);
				m_restoreVPos = 0.;
			}

			// Set steps.
			font_height fh;
			be_plain_font->GetHeight(&fh);
			float large_step = Bounds().Height() - (fh.ascent+fh.descent+fh.leading);
			if( large_step < 0 ) large_step = Bounds().Height();
			vsb->SetSteps(ceil(fh.ascent+fh.descent+fh.leading), ceil(large_step));
		};
	};
	
	if (fromLayout) {
		m_lastContentWidth = contentWidth;
		m_lastContentHeight = contentHeight;
	}
	
	m_parent->SetScrollbarState(h,v);
}

static BView* hunt_for_focus_control(BRect bounds, BView* parent)
{
	if (!parent) return 0;
	BView* child = parent->ChildAt(0);
	while (child) {
		BRect frame(child->Frame());
		frame.OffsetBy(-bounds.left, -bounds.top);
		if (bounds.Intersects(frame)) {
			// Figure out of this is really a control we want to give
			// focus to.  We do not give it to menu fields, radio
			// buttons, and multi-line text areas because they will
			// eat all of the cursor keys.
			ContentView* contentview = dynamic_cast<ContentView*>(child);
			if (child->Flags()&B_NAVIGABLE) {
				BMenuField* menufield = dynamic_cast<BMenuField*>(child);
				BRadioButton* radio = dynamic_cast<BRadioButton*>(child);
				BTextView* textview = dynamic_cast<BTextView*>(child);
				bool oneline = textview ? textview->IsResizable() : true;
				if (!menufield && !radio && !contentview && oneline) {
					// Special magic: if it is a text view, start out
					// with everything selected.
					if (textview) textview->SelectAll();
					return child;
				}
			}
			
			// If this isn't another content view, step into it looking
			// for a focus child.
			if (!contentview) {
				BRect childbounds(child->Bounds());
				BRect childframe(child->Frame());
				childframe = childframe & (bounds.OffsetToCopy(0, 0));
				childframe.OffsetTo(childbounds.LeftTop());
				BView* found = hunt_for_focus_control(childframe, child);
				if (found) return found;
			}
		}
		
		child = child->NextSibling();
	}
	
	return 0;
}

BView* RealContentView::PickFocusControl()
{
	return hunt_for_focus_control(Bounds(), this);
}

void RealContentView::MoveFocusHere(bool include_content)
{
	// Select a view to focus, only if no view currently has focus
	// or the current focus is not explicitly selected by the user.
	BView* focus = Window() ? Window()->CurrentFocus() : 0;
	if (focus && focus->IsExplicitFocus())
		return;
	
	if (!IsFocus()) {
		// This applies focus to this view, only if focus is not already
		// in one of its children.
		while (focus && focus != this) {
			focus = focus->Parent();
			// Don't look across frames.
			if (dynamic_cast<ContentView*>(focus)) focus = 0;
		}
		if (!focus) {
			focus = PickFocusControl();
			if (focus) focus->MakeFocus();
			else if (include_content) MakeFocus();
		}
		
	} else {
		focus = PickFocusControl();
		if (focus) focus->MakeFocus();
	}
}

bool RealContentView::UpdateContentRect()
{
	float oldLeft = m_contentRect.left;
	float oldTop = m_contentRect.top;
	
	int32 contentWidth=0,contentHeight=0;
	if (m_contentInstance)
		m_contentInstance->GetSize(&contentWidth, &contentHeight, &m_contentResizeFlags);

	if (m_contentResizeFlags & STRETCH_HORIZONTAL) {
		m_contentRect.left = 0;
		m_contentRect.right = m_width - 1; 
	} else {
		float inset = (m_width - contentWidth) / 2;
		if (inset < 0) {
			m_contentRect.left = 0;
			m_contentRect.right = contentWidth-1;
		} else {
			m_contentRect.left = inset;
			m_contentRect.right = m_width - inset - 1;
		}
	}
	
	if (m_contentResizeFlags & STRETCH_VERTICAL) {
		m_contentRect.top = 0;
		m_contentRect.bottom = m_height - 1;
	} else {
		float inset = (m_height - contentHeight) / 2;
		if (inset < 0) {
			m_contentRect.top = 0;
			m_contentRect.bottom = contentHeight-1;
		} else {
			m_contentRect.top = inset;
			m_contentRect.bottom = m_height - inset - 1;
		}
	}
	
	if( oldLeft != m_contentRect.left || oldTop != m_contentRect.top ) return true;
	return false;
}

void RealContentView::BuildRedirectList()
{
	BinderNode::property prop = BinderNode::Root()["service"]["web"]["redirect"];
	BinderNode::iterator iterate = prop->Properties();
	BString source;
	BString destination;
	
	while ((source = iterate.Next()) != "") {
		destination = prop[source.String()].String();
		RiggedRedirect item;
		item.src = source.String();
		item.dest = destination.String();
		RealContentView::m_redirects.AddItem(item);
	}
}

ContentView *ContentViewForPath(ContentView *cview, const History::FramePath &framePath)
{
	// cview is top ContentView
	//
	for (int i = 0; i < framePath.depth; i++) {
		RealContentView *rcview = cview->GetRealContentView();
		cview = dynamic_cast<ContentView*>(rcview->ChildAt(0));
		while(cview && cview->Index() != framePath.indices[i])
			cview = dynamic_cast<ContentView*>(cview->NextSibling());
	}

	return cview;
}

void SaveFrameState(ContentView *cview, History *history, History::FramePath &framePath)
{
	// cview is at the specified path

	ContentInstance *instance;
	if ((instance = cview->GetContentInstance()) != NULL) {
		BMessage msg;
		instance->SaveState(&msg);

		RealContentView *rcv = cview->GetRealContentView();
		if(rcv)
		{
			BScrollBar *sb = rcv->ScrollBar(B_HORIZONTAL);
			if(sb && !sb->IsHidden())
				msg.AddFloat("hpos", sb->Value());

			sb = rcv->ScrollBar(B_VERTICAL);
			if(sb && !sb->IsHidden())
				msg.AddFloat("vpos", sb->Value());
		}

		history->SetFrameData(framePath, msg);
	}

	if (framePath.depth < framePath.maxDepth) {
		RealContentView *rcview;
		if ((rcview = cview->GetRealContentView()) != NULL) {
			framePath.depth++;
			for (int32 i = 0; i < rcview->CountChildren(); i++) {
				ContentView *ccview = dynamic_cast<ContentView*>(rcview->ChildAt(i));
				if (ccview) {
					int32 index = ccview->Index();
					framePath.indices[framePath.depth-1] = index;
					// could be a non-frame ContentView
					if (index >= 0 && history->FrameExists(framePath)) {
						SaveFrameState(ccview, history, framePath);
					}
				}
			}
			framePath.depth--;
		}
	}
}

status_t RealContentView::SetContent(const BUrl &requestURL, uint32 flags, GroupID groupID, BMessage *userData)
{
	BUrl url(requestURL);
	int32 count = m_redirects.CountItems();
	for (int32 i = 0; i < count; i++) {
		if (url == m_redirects.ItemAt(i).src) {
			url = m_redirects.ItemAt(i).dest;
			break;
		}
	}
	
	if (IsURLInAncestry(url)) {
		// Prevent recursive loading of same URL.
		return B_ERROR;
	}
	
	// Invalid or default group id given, so determine group id from URL
	if(groupID < 0)
		groupID = securityManager.GetGroupID(url);
	
	//
	//	Note that each page we load will get a new ID, allowing the
	//	message handling code to reject new instances if the user
	//	navigates before a page gets loaded.
	//
	if (atomic_or((int32*)&m_flags,cvoRequestPending) & cvoRequestPending) {
		if (m_requestedURL.Equals(url)) return B_OK;
		resourceCache.CancelRequest(m_requestedURL, m_id, m_bridge);
	};

	if (m_contentInstance &&
		m_contentInstance->GetContent()->GetResource()->GetURL().Equals(url) &&
		!(flags & (FORCE_REGET|FORCE_RELOAD)))
	{
		atomic_and((int32*)&m_flags,~cvoRequestPending);

		const char *frag = url.GetFragment();
		if (!frag || !*frag) {
			frag = "top";
		}

		BMessage *msg = new BMessage(bmsgLinkToFragment);
		msg->AddString("fragment", frag);
		m_contentInstance->ContentNotification(msg);

		return B_OK;
	}

	History *history = NULL;
	flags &= ~FORCE_REGET;
	if ((flags & RECORD_IN_HISTORY) && ((TopContentView()==this) || (Index() >= 0))) history = GetHistory();
	if ((flags & USER_ACTION) && history) {
		if (Window() && Window()->Lock()) {
			History::FramePath framePath(64);
			if (GetFramePath(&framePath) == B_OK)
				SaveFrameState(m_parent, history, framePath);
			Window()->Unlock();
		}
		history->Checkpoint();
	}

	BMessage msg;
	if (!userData) userData = &msg;
	resourceCache.NewContentInstance(url, atomic_add(&m_id, 1) + 1, m_bridge, flags & 0xFFFF,
		*userData, groupID, (ContentInstantiateHook)NULL, (const char *)NULL);
	m_requestedURL = url;

	if (history) {
		History::FramePath framePath(64);
		if (GetFramePath(&framePath) == B_OK) {
			history->Goto(framePath,url,groupID);
			// Play sound for opening a page, but only if this is not
			// the first page in the view.  This is a gross hack to avoid
			// multiple sounds when opening frame sets.
			if( m_contentInstance ) {
				if( Window() ) Window()->SetCurrentBeep("Page Open");
				BMessage msg(bmsgPageOpening);
				msg.AddInt64("time", system_time());
				be_app->PostMessage(&msg);
			}
		}
	}

	return B_OK;
}

status_t RealContentView::SetContent(ContentInstance *instance)
{
	if (m_contentInstance == instance)
		return B_OK;

	Window()->Lock();

	RealContentView *v;
	ContentInstance *parent = NULL;
	atom<ContentInstance> oldContent = m_contentInstance;
	if (instance) instance->SetHandler(m_bridge);

	if ((v = ParentContentView()))
		parent = v->GetContentInstance();

	m_contentInstance = instance;
	m_contentInstance->SetParentContent(parent);
	m_contentError = 0;
	m_flags |= cvfContentDisplayable;

	int32 count = CountChildren();
	for (int32 i=0;i<count;i++) {
		ContentView *v = dynamic_cast<ContentView*>(ChildAt(i));
		if (v && v->GetContentInstance()) {
			v->GetContentInstance()->SetParentContent(m_contentInstance);
		}
	}

	if (oldContent) oldContent->Acquire();
	m_contentInstance->UsurpPredecessor(oldContent);

	// Initialize view state for showing new content.
	ScrollTo(0, 0);
	if( m_parent ) m_parent->SetScrollbarState(false, false);
	m_contentResizeFlags = 0;
	m_lastContentWidth = m_lastContentHeight = 0;
	m_needHScroll = m_needVScroll = false;
	m_moreHoriz = m_moreVert = false;
	
	if (m_contentInstance) {
		uint32 flags;
		m_usingId = m_contentInstance->ID();
		m_contentInstance->AttachedToView(this, &flags);
		UpdateContentRect();
		int32 fullWidth = m_width;
		int32 fullHeight = m_height;
		if( m_parent ) m_parent->GetAvailSize(&fullWidth, &fullHeight);
		m_contentInstance->FrameChanged(m_contentRect, fullWidth, fullHeight);
	};

	Invalidate();

	// Send a message to the top view so it can update the toolbar
	BMessage notify('nurl');
	BView *top = this;
	while (top->Parent())
		top = top->Parent();
	dynamic_cast<ContentView*>(top)->GetRealContentView()
		->GetContentInstance()->Notification(&notify);

	Window()->Unlock();

	return B_OK;
}

void RealContentView::MoreSpace(bool h, bool v)
{
	if( h ) m_moreHoriz = true;
	if( v ) m_moreVert = true;
}

void RealContentView::MouseDown(BPoint where)
{
	int32 modifiers = Window()->CurrentMessage()->FindInt32("modifiers");
	if ((modifiers & (B_CONTROL_KEY | B_COMMAND_KEY | B_SHIFT_KEY)) == 
		(B_CONTROL_KEY | B_SHIFT_KEY)) {
		ViewSource(GetContentInstance()->GetContent()->GetResource()->GetURL());
		return;
	} else if ((modifiers & (B_CONTROL_KEY | B_COMMAND_KEY | B_SHIFT_KEY)) == 
		(B_CONTROL_KEY | B_COMMAND_KEY | B_SHIFT_KEY)) {
		ViewSource(TopContentView()->GetContentInstance()->GetContent()->GetResource()->GetURL());
		return;
	}

	SetMouseEventMask(B_POINTER_EVENTS,B_NO_POINTER_HISTORY);
	
	if( m_parent ) {
		// If this content view has scroll bars, let it take focus.
		bool h=false,v=false;
		m_parent->GetScrollbarState(&h, &v);
		if( h || v ) {
			MakeFocus();
			SetExplicitFocus();
		}
	}
	
	if (m_contentInstance) m_contentInstance->MouseDown(where,Window()->CurrentMessage());
}

void RealContentView::MouseUp(BPoint where)
{
	if (m_contentInstance) m_contentInstance->MouseUp(where,Window()->CurrentMessage());
}

void RealContentView::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
{
	if (m_contentInstance) m_contentInstance->MouseMoved(where,code,a_message,Window()->CurrentMessage());
}

void RealContentView::KeyDown(const char* bytes, int32 numBytes)
{
	bool handled = false;
	
	if( numBytes == 1 ) {
	
		BPoint  scrollTo(0, 0);
		float smallX=0, largeX=0;
		float smallY=0, largeY=0;
		
		BScrollBar* hbar = ScrollBar(B_HORIZONTAL);
		BScrollBar* vbar = ScrollBar(B_VERTICAL);
		if( hbar ) {
			scrollTo.x = hbar->Value();
			hbar->GetSteps(&smallX, &largeX);
		}
		if( vbar ) {
			scrollTo.y = vbar->Value();
			vbar->GetSteps(&smallY, &largeY);
		}
		
		handled = true;
		
		switch( *bytes ) {
			case B_UP_ARROW:		scrollTo.y -= smallY;		break;
			case B_DOWN_ARROW:		scrollTo.y += smallY;		break;
			case B_LEFT_ARROW:		scrollTo.x -= smallX;		break;
			case B_RIGHT_ARROW:		scrollTo.x += smallX;		break;
			
			case B_PAGE_UP:			scrollTo.y -= largeY;		break;
			case ' ':
			case B_PAGE_DOWN:		scrollTo.y += largeY;		break;
			
			case B_HOME:			scrollTo = BPoint(0, 0);	break;
			case B_END:				scrollTo.y = 1e20;			break;
			default:
				handled = false;
				break;
		}
		
		if( hbar ) hbar->SetValue(scrollTo.x);
		if( vbar ) vbar->SetValue(scrollTo.y);
		be_app->ObscureCursor();
		
	}
	
	if( !handled ) {
		if (m_contentInstance) m_contentInstance->KeyDown(bytes,numBytes,Window()->CurrentMessage());
		BView::KeyDown(bytes, numBytes);
	}
}

void RealContentView::KeyUp(const char *bytes, int32 numBytes)
{
	if (m_contentInstance) m_contentInstance->KeyUp(bytes,numBytes,Window()->CurrentMessage());
	
	BView::KeyUp(bytes, numBytes);
}

void RealContentView::RestoreContent(BMessage *restore)
{
	BUrl url; url.ExtractFromMessage("url",restore);

	BMessage frameData;
	if (restore->HasMessage("frame_data")) {
		restore->FindMessage("frame_data", &frameData);
		float pos;
		if (frameData.FindFloat("hpos", &pos) == B_OK)
			m_restoreHPos = pos;
		else
			m_restoreHPos = 0.;

		if (frameData.FindFloat("vpos", &pos) == B_OK)
			m_restoreVPos = pos;
		else
			m_restoreVPos = 0.;
	}

	if (m_contentInstance && m_contentInstance->GetContent()->GetResource()->GetURL().Equals(url)) {
		if (restore->HasMessage("frame_data")) {
			m_contentInstance->SyncToState(&frameData);
		}
		
		BView *child = ChildAt(0);
		BMessage childRestore;
		while (child) {
			ContentView *cv = dynamic_cast<ContentView*>(child);
			if (cv && (cv->GetRealContentView()->Index() >= 0)) {
				RealContentView *rcv = cv->GetRealContentView();
				childRestore.MakeEmpty();
				if (restore->FindMessage("frame_set",rcv->Index(),&childRestore) == B_OK)
					rcv->RestoreContent(&childRestore);
			};
			child = child->NextSibling();
		};
	} else {
		GroupID groupID = securityManager.GetGroupID(m_requestedURL);
		if (restore->HasInt32("group_id"))
			groupID = (GroupID)restore->FindInt32("group_id");

		if (atomic_or((int32*)&m_flags,cvoRequestPending) & cvoRequestPending)
			resourceCache.CancelRequest(m_requestedURL, m_id, m_bridge);

		BMessage userData;
		userData.AddMessage("history",restore);
		resourceCache.NewContentInstance(url, atomic_add(&m_id, 1) + 1, m_bridge,
			LOAD_ON_ERROR, userData, groupID, (ContentInstantiateHook)NULL, (const char *)NULL);
		m_requestedURL = url;
	};
};

void RealContentView::Reload(bool flushCache)
{
	if (m_contentInstance) {
		Content *content = m_contentInstance->GetContent();
		if (content) {
			Resource *res = content->GetResource();
			if (res) {
				uint32 flags = LOAD_ON_ERROR | FORCE_REGET;
				if (flushCache) flags |= FORCE_RELOAD;
				m_parent->SetContent(res->GetURL(), flags, res->GetGroupID());
			}
		}
	}
}

void RealContentView::ClearHistory()
{
	if (m_history) {
		m_history->Clear();
	}
}

void RealContentView::GoForward()
{
	if (m_history && m_history->CanGoForward()) {
		History::FramePath deltaPath(64);
		m_history->ForwardFrameDelta(&deltaPath);
		ContentView *cview;
		if((cview = ContentViewForPath(TopContentView()->m_parent, deltaPath)) != NULL)
			SaveFrameState(cview, m_history, deltaPath);

		BMessage restore;
		m_history->GoForward(&restore);
		RestoreContent(&restore);
		if( Window() ) Window()->SetCurrentBeep("History Forward");
	}
}

void RealContentView::GoBackward()
{
	if (m_history && m_history->CanGoBack()) {
		History::FramePath deltaPath(64);
		m_history->BackFrameDelta(&deltaPath);
		ContentView *cview;
		if((cview = ContentViewForPath(TopContentView()->m_parent, deltaPath)) != NULL)
			SaveFrameState(cview, m_history, deltaPath);

		BMessage restore;
		m_history->GoBack(&restore);
		RestoreContent(&restore);
		if( Window() ) Window()->SetCurrentBeep("History Backward");
	}
}

void RealContentView::GoOffset(int32 offset)
{
	if (m_history && m_history->CanGoOffset(offset)) {
		History::FramePath deltaPath(64);
		m_history->OffsetFrameDelta(&deltaPath,offset);
		ContentView *cview;
		if((cview = ContentViewForPath(TopContentView()->m_parent, deltaPath)) != NULL)
			SaveFrameState(cview, m_history, deltaPath);

		BMessage restore;
		m_history->GoOffset(&restore,offset);
		RestoreContent(&restore);
		if( Window() )
			 Window()->SetCurrentBeep(offset>=0 ? "History Forward" : "History Backward") ;
	}
}

void RealContentView::GoNearest(const char *nearest)
{
	if(m_history) {

		char urlstr[1024];
		BUrl url;
		BMessage msg;

		for(int32 c=1;;c++) {
			bool breakloop = true;
			if(m_history->CanGoOffset(-c)) {
				breakloop = false;

				m_history->GoOffset(&msg,-c,true);
				url.ExtractFromMessage("url",&msg);
				url.GetString(urlstr,1024);
				if(strstr(urlstr, nearest)) {
					GoOffset(-c);
					return;
				}
			}
			if(m_history->CanGoOffset(c)) {
				breakloop = false;

				m_history->GoOffset(&msg,-c,true);
				url.ExtractFromMessage("url",&msg);
				url.GetString(urlstr,1024);
				if(strstr(urlstr, nearest)) {
					GoOffset(c);
					return;
				}
			}			
			if(breakloop) break;
		}
	}
}

void 
RealContentView::Rewind()
{
	if (!m_history) return;
	BMessage state;

	while (m_history->CanGoBack()) {
		state.MakeEmpty();
		m_history->GoBack(&state);
	}
	m_history->Checkpoint();

	RestoreContent(&state);
}

// =====================================================
// #pragma mark -

void RealContentView::Print()
{
	// The window must be Locked before drawing
	BAutolock autolock(Window());
	if (autolock.IsLocked() == false)
		return;

	// Get the name of the frame to print
	const char *frame_name = NULL;
	const char *header_name = NULL;
	BMessage *print_msg = be_app->CurrentMessage();
	if (print_msg == NULL)
		return;

	BMessage settings;
	frame_name = print_msg->FindString("frame");
	header_name = print_msg->FindString("header");
	print_msg->FindMessage("settings", &settings);

	// Create the Job, get the page size, move it to B_ORIGIN
	BPrintJob job(settings, "Wagner");

	status_t result;
	if ((result = job.InitCheck()) != B_OK)
	{ // should not happen
		PRINT(("Wagner: BPrintJob::InitCheck = %s\n", strerror(result)));
		return;
	}

	if ((result = job.BeginJob()) != B_OK)
	{ // should not happen
		PRINT(("Wagner: BPrintJob::BeginJob = %s\n", strerror(result)));
		return;
	}

	// Find the view to print
	BView *child = NULL;
	BView *headerView = NULL;
	ContentView *viewToPrint = NULL;
	if (frame_name)
	{ // Find the requested view
		child = FindView(frame_name);
		if (child == NULL) {
			PRINT(("Wagner: Frame to print not found (frame=<%s>)\n", frame_name));
			return;
		}
		viewToPrint = dynamic_cast<ContentView *>(child->Parent());
		if ((viewToPrint) && (header_name))
		{ // Check if we are asked to print a header
			headerView = FindView(header_name);		
			if (child == NULL) {
				PRINT(("Wagner: Header to print not found (header=<%s>)\n", header_name));
			}
		}
	}

	if ((child == NULL) || (viewToPrint == NULL)) {
		PRINT(("Wagner: View to print not found (frame=<%s>, header=<%s>)\n", frame_name, header_name));
		return;
	}

	PRINT(("Wagner: printed view = %s\n", child->Name()));

	// Get the size of the content
	uint32 dummy;
	int32 w, h;

	viewToPrint->GetContentInstance()->GetSize(&w, &h, &dummy);
	const float contentWidth = w;
	const float contentHeight = h;
	PRINT(("content width  : %f\n", contentWidth));
	PRINT(("content height : %f\n", contentHeight));

	BRect printable_rect = job.PrintableRect().OffsetToCopy(B_ORIGIN);
	const float scaleFactor = (printable_rect.Width()+1.0f)/contentWidth;
	PRINT(("printing scale factor = %f\n", scaleFactor));


	// This is the page height in screen coord' space
	const float pageWidth = contentWidth;
	const float pageHeight = (printable_rect.Height()+1.0f)/scaleFactor;
	BRect r(0, 0, pageWidth, pageHeight);
	const float repeat =  72.0f*(0.5f/2.54f);	// Repeat the last 0.5 cm of the page (it may have been cut)
	const float offset = pageHeight - repeat;

	if (headerView)
	{ // We must print the header view first
		const BRect b = headerView->Bounds();
		job.SetScale(scaleFactor);
		job.DrawView(headerView, b, B_ORIGIN);
		BRect f = r;
		f.bottom -= b.Height()+1.0f;
		job.DrawView(child, f, b.LeftBottom()*scaleFactor);
		job.SpoolPage();
		r.OffsetBy(0, f.Height() - repeat);
	}

	while (r.top < contentHeight)
	{
		if (r.bottom > contentHeight)
			r.bottom = contentHeight;
		job.SetScale(scaleFactor);
		job.DrawView(child, r, B_ORIGIN);
		job.SpoolPage();
		r.OffsetBy(0, offset);
	} 

	// Let's print!
	job.CommitJob();
}

// =====================================================
// #pragma mark -


void RealContentView::GetURL(BUrl *url)
{
	if (!url) return;	
	if (m_contentInstance) {
		Content *content = m_contentInstance->GetContent();
		if (content) {
			Resource *res = content->GetResource();
			if (res) {
				*url = res->GetURL();
			}
		}
	}
}

void RealContentView::GetPreviousURL(BUrl *url)
{
	if (!url) return;
	BMessage msg;
	if(m_history && m_history->CanGoOffset(-1)) {
		m_history->GoOffset(&msg,-1,true);	
		url->ExtractFromMessage("url",&msg);
	}
}

void RealContentView::GetNextURL(BUrl *url)
{
	if (!url) return;
	if(m_history && m_history->CanGoOffset(1)) {
		BMessage msg;
		m_history->GoOffset(&msg,1,true);
		url->ExtractFromMessage("url",&msg);
	}
}

void RealContentView::GetHistoryLength(int32 *length)
{
	if(!length) return;

	if(m_history) 
		*length = m_history->CountItems();
}

void RealContentView::StopLoading()
{
	if (atomic_and((int32*) &m_flags, ~cvoRequestPending) & cvoRequestPending)
		resourceCache.CancelRequest(m_requestedURL, m_id, m_bridge);

	atomic_add(&m_id, 1);	// Just make it so the ID won't match in case we get the instance

	// Recurse
	int32 childCount = CountChildren();
	for (int32 i = 0; i < childCount; i++) {
		ContentView *view = dynamic_cast<ContentView*>(ChildAt(i));
		if (view)
			view->StopLoading();
	}
}

bool RealContentView::IsPrivate() const
{
	for (const RealContentView *view = this; view; view = dynamic_cast<RealContentView*>(view->m_parent->Parent()))
		if (!strncasecmp(kSpecialFramePrefix, view->Name(), strlen(kSpecialFramePrefix)))
			return false;

	return false;
}

bool RealContentView::IsURLInAncestry(const BUrl &url)
{
	RealContentView *parent = ParentContentView();
	if (parent) {
		if (parent->m_contentInstance) {
			Content *content = parent->m_contentInstance->GetContent();
			if (content) {
				Resource *resource = content->GetResource();
				if (resource) {
					if (url == resource->GetURL()) {
						printf("\nURL\n");
						resource->GetURL().PrintToStream();
						printf("is in ancestry of URL\n");
						url.PrintToStream();
						printf("\n\n");
						return true;
					}
				}
			}
		}
		else {
			return parent->IsURLInAncestry(url);
		}
	}
	return false;
}

/**************************************************************/

ContentView::ContentView(BRect r, const char *name, uint32 follow, uint32 flags, uint32 options, History *history)
	: BView(r,"contentViewContainer",follow,flags|B_WILL_DRAW|B_FRAME_EVENTS|B_SUBPIXEL_PRECISE)
{
	m_flags = options;
	m_scrollH = NULL;
	m_scrollV = NULL;

	r.OffsetTo(0,0);

	m_contentView = new RealContentView(r,name,B_FOLLOW_NONE,flags,options,this,history);
	AddChild(m_contentView);
	
	if (m_flags & cvoHScrollbar) {
		BRect hScrollR = r;
		hScrollR.top = hScrollR.bottom - B_H_SCROLL_BAR_HEIGHT;
		if( hScrollR.right < (hScrollR.left+B_V_SCROLL_BAR_WIDTH*2) ) {
			hScrollR.right = hScrollR.left+B_V_SCROLL_BAR_WIDTH*2;
		}
		m_scrollH = new BScrollBar(hScrollR,"hsb",m_contentView,0,0,B_HORIZONTAL);
		m_scrollH->Hide();
		m_scrollH->SetRange(0, 0);
		m_scrollH->SetResizingMode(B_FOLLOW_NONE);
		AddChild(m_scrollH);
	};

	if (m_flags & cvoVScrollbar) {
		BRect vScrollR = r;
		vScrollR.left = vScrollR.right - B_V_SCROLL_BAR_WIDTH;
		if( vScrollR.bottom < (vScrollR.top+B_H_SCROLL_BAR_HEIGHT*2) ) {
			vScrollR.bottom = vScrollR.top+B_H_SCROLL_BAR_HEIGHT*2;
		}
		m_scrollV = new BScrollBar(vScrollR,"vsb",m_contentView,0,0,B_VERTICAL);
		m_scrollV->Hide();
		m_scrollV->SetRange(0, 0);
		m_scrollV->SetResizingMode(B_FOLLOW_NONE);
		AddChild(m_scrollV);
	};
	
}

status_t ContentView::SetContent(const BUrl &url, uint32 flags, GroupID groupID, BMessage *userData)
{
	return m_contentView->SetContent(url, flags, groupID, userData);
}

status_t ContentView::SetContent(ContentInstance *instance)
{
	return m_contentView->SetContent(instance);
}

ContentView::~ContentView()
{
}

void ContentView::AttachedToWindow()
{
	BRect bounds = Bounds();
	FrameResized(bounds.Width(), bounds.Height());
}

#define HEIGHT_VALUE (B_H_SCROLL_BAR_HEIGHT+1)
#define WIDTH_VALUE (B_V_SCROLL_BAR_WIDTH+1)

void ContentView::GetAvailSize(int32* width, int32* height)
{
	*width = m_width;
	*height = m_height;
}

void ContentView::GetScrollbarDimens(int32* width, int32* height)
{
	*width = (int32)WIDTH_VALUE;
	*height = (int32)HEIGHT_VALUE;
}

void ContentView::SetScrollbarState(bool h, bool v)
{
	// If we don't have a particular scroll bar, then don't make
	// space for it.
	if( !m_scrollV ) v = false;
	if( !m_scrollH ) h = false;
	
	bool curh, curv;
	GetScrollbarState(&curh, &curv);
	if (curh == h && curv == v) return;
	
	LayoutChildren(h, v);
};

void ContentView::GetScrollbarState(bool* h, bool* v)
{
	*h = (m_flags & cvfHScrollbarVisible) ? true : false;
	*v = (m_flags & cvfVScrollbarVisible) ? true : false;
}

void ContentView::FrameResized(float , float )
{
	int32 newWidth = (int32) (Bounds().Width()+1);
	int32 newHeight = (int32) (Bounds().Height()+1);
	
	bool mh=newWidth > m_width, mv=newHeight > m_height;
	if( mh || mv ) m_contentView->MoreSpace(true, true);
	
	m_width = newWidth;
	m_height = newHeight;

	LayoutChildren();
}

void ContentView::LayoutChildren()
{
	bool h, v;
	GetScrollbarState(&h, &v);
	LayoutChildren(h, v);
}

void ContentView::LayoutChildren(bool h, bool v)
{
	if (Window()) Window()->BeginViewTransaction();
	
	int32 subH = m_height;
	int32 subW = m_width;
	if (h) subH -= (int32) HEIGHT_VALUE;
	if (v) subW -= (int32) WIDTH_VALUE;

	m_contentView->ResizeTo(subW-1,subH-1);
	
	// Make sure scroll bars are in correct location.  This is needed
	// because view may be resized while not attached to window.
	
	BRect r(0, 0, m_width-1, m_height-1);
	
	if (m_scrollH) {
		BRect hScrollR = r;
		hScrollR.top = hScrollR.bottom - B_H_SCROLL_BAR_HEIGHT;
		if (v) {
			hScrollR.right -= B_V_SCROLL_BAR_WIDTH;
		}
		if( hScrollR.right < (hScrollR.left+B_V_SCROLL_BAR_WIDTH*2) ) {
			hScrollR.right = hScrollR.left+B_V_SCROLL_BAR_WIDTH*2;
		}
		m_scrollH->MoveTo(hScrollR.LeftTop());
		m_scrollH->ResizeTo(hScrollR.Width(), hScrollR.Height());
	};

	if (m_scrollV) {
		BRect vScrollR = r;
		vScrollR.left = vScrollR.right - B_V_SCROLL_BAR_WIDTH;
		if (h) {
			vScrollR.bottom -= B_H_SCROLL_BAR_HEIGHT;
		}
		if( vScrollR.bottom < (vScrollR.top+B_H_SCROLL_BAR_HEIGHT*2) ) {
			vScrollR.bottom = vScrollR.top+B_H_SCROLL_BAR_HEIGHT*2;
		}
		m_scrollV->MoveTo(vScrollR.LeftTop());
		m_scrollV->ResizeTo(vScrollR.Width(), vScrollR.Height());
	};
	
	if (m_scrollH) {
		if (h && !(m_flags & cvfHScrollbarVisible)) {
			m_scrollH->Show();
			m_flags |= cvfHScrollbarVisible;
		} else if (!h && (m_flags & cvfHScrollbarVisible)) {
			m_scrollH->Hide();
			m_flags &= ~cvfHScrollbarVisible;
		};
	};

	if (m_scrollV) {
		if (v && !(m_flags & cvfVScrollbarVisible)) {
			m_scrollV->Show();
			m_flags |= cvfVScrollbarVisible;
		} else if (!v && (m_flags & cvfVScrollbarVisible)) {
			m_scrollV->Hide();
			m_flags &= ~cvfVScrollbarVisible;
		};
	};
	
	if (Window()) Window()->EndViewTransaction();
}

void ContentView::SetIndex(int32 index)
{
	m_contentView->SetIndex(index);
};

int32 ContentView::Index()
{
	return m_contentView->Index();
};

ContentInstance* ContentView::GetContentInstance() const
{
	return m_contentView ? m_contentView->GetContentInstance() : 0;
}

ContentInstance* ContentView::GetTopContentInstance() const
{
	if (m_contentView) {
		RealContentView *rcv = m_contentView->RealTopContentView();
		return rcv->GetContentInstance();
	}
	else {
		return NULL;
	}
}

ContentInstance* ContentView::GetParentContentInstance() const
{
	if (m_contentView) {
		RealContentView *rcv = m_contentView->ParentContentView();
		if (rcv) {
			return rcv->GetContentInstance();
		}
	}
	return NULL;
}

bool ContentView::IsInContentArea() const
{
	if (m_contentView) {
		RealContentView *rcv = m_contentView->TopContentView();
		return !strncmp(rcv->Name(), "_be:content", 11);
	}
	return false;
}

const char *ContentView::GetTopContentViewName() const
{
	if (m_contentView) {
		RealContentView *rcv = m_contentView->TopContentView();
		return rcv->Name();
	}
	return "_be:content0";
}

bool ContentView::IsInMailAttachmentArea() const
{
	if (m_contentView) {
		return !strncmp(m_contentView->Name(), "_be:mail:read_content", 21);
	}
	return false;
}

void ContentView::RestoreContent(BMessage *restore)
{
	if (m_contentView) m_contentView->RestoreContent(restore);
};

void ContentView::Reload()
{
	if (m_contentView) m_contentView->Reload(true);
}

void ContentView::ClearHistory()
{
	BAutolock looper_lock(Looper());
	
	BView *bv = FindViewCaselessN(this, kSpecialFramePrefix, strlen(kSpecialFramePrefix));
	if (bv) {
		RealContentView *rcv = dynamic_cast<RealContentView *>(bv);
		if (rcv) {
			rcv->ClearHistory();
		}
	}
}

void ContentView::GoForward()
{
	BAutolock looper_lock(Looper());
	
	BView *bv = FindViewCaselessN(this, kSpecialFramePrefix, strlen(kSpecialFramePrefix));
	if (bv) {
		RealContentView *rcv = dynamic_cast<RealContentView *>(bv);
		if (rcv) {
			rcv->GoForward();
		}
	}
}

void ContentView::GoBackward()
{
	BAutolock looper_lock(Looper());
	
	BView *bv = FindViewCaselessN(this, kSpecialFramePrefix, strlen(kSpecialFramePrefix));
	if (bv) {
		RealContentView *rcv = dynamic_cast<RealContentView *>(bv);
		if (rcv) {
			rcv->GoBackward();
		}
	}
}

static BView *FindViewCaselessN(BView *root, const char *name, int length)
{
	if (!root || !name) return NULL;
	
	if (strncasecmp(root->Name(), name, length) == 0) {
		return root;
	}
	
	for (root = root->ChildAt(0); root; root = root->NextSibling()) {
		BView *ret = FindViewCaselessN(root, name, length);
		if (ret) return ret;
	}
	
	return NULL;
}


void 
ContentView::Rewind()
{
	BAutolock looper_lock(Looper());
	
	BView *bv = FindViewCaselessN(this, kSpecialFramePrefix, strlen(kSpecialFramePrefix));
	if (bv) {
		RealContentView *rcv = dynamic_cast<RealContentView *>(bv);
		if (rcv) {
			rcv->Rewind();
		}
	}
}

void ContentView::Print()
{
	if (m_contentView) m_contentView->Print();
}

void ContentView::GetURL(BUrl *url)
{
	if (m_contentView) m_contentView->GetURL(url);
}

void ContentView::GetPreviousURL(BUrl *url)
{
	if (m_contentView) m_contentView->GetPreviousURL(url);
}

void ContentView::GetNextURL(BUrl *url)
{
	if (m_contentView) m_contentView->GetNextURL(url);
}

void ContentView::GetHistoryLength(int32 *length)
{
	if (m_contentView) m_contentView->GetHistoryLength(length);
}

bool ContentView::IsPrivate() const
{
	if (m_contentView)
		return m_contentView->IsPrivate();
		
	return true;
}

void ContentView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case B_COPY:
		case B_SELECT_ALL:
		case bmsgLinkTo:
		//case bmsgLinkToFragment: // XXX: is this needed?
		case bmsgClearHistory:
		case bmsgGoBack:
		case bmsgGoForward:
		case bmsgGoOffset:
		case bmsgGoNearest:
		case bmsgReload:
		case bmsgStop:
		case bmsgJSCloseWindow:
		case bmsgNotifyInstance:
		case bmsgStateChange:
		case bmsgBroadcast:
		case bmsgDeselectAll: {
			BLooper *looper = NULL;
			if (m_contentView && (looper = m_contentView->Looper())) {
				looper->PostMessage(msg, m_contentView);
			}
			break;
		}
		
		case B_MOUSE_WHEEL_CHANGED: {
			BPoint location;
			uint32 buttons;
			GetMouse(&location,&buttons);
			MouseMoved(location,Bounds().Contains(location) ? B_INSIDE_VIEW : B_OUTSIDE_VIEW,0);
		}
		default:
			BView::MessageReceived(msg);
	}
}

void ContentView::StopLoading()
{
	if (Window()->Lock()) {
		if (m_contentView)
			m_contentView->StopLoading();

		Window()->Unlock();
	}
}
