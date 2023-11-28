
#include <Message.h>
#include <TokenSpace.h>
#include <StreamIO.h>

#include <Portal.h>
#include <RenderPipeDrawable.h>
#include <InterfaceUtils.h>
#include <ViewNamespace.h>

#include <app_server_p/messages.h>

#define WINDOW_NAME "ik2window"
#define COMPILE_FOR_IAD 1

/*---------------------------------------------------------------*/

const char *windowBinderProps[] = {
	"constraints",
	"token",
	NULL
};

/*---------------------------------------------------------------*/


/*---------------------------------------------------------------*/


/*---------------------------------------------------------------*/

GehmlWindow::GehmlWindow()
{
	Init();
}

GehmlWindow::GehmlWindow(BStringMap &attr) : GehmlRoot(attr)
{
	BString *s;

	Init();
	
	if ((s=attr.Find("color"))) m_color = decode_color(s->String());
	if ((s=attr.Find("decor"))) {
		if (*s == "true") m_look = B_TITLED_WINDOW_LOOK;
	}
}

GehmlWindow::~GehmlWindow()
{
}

void GehmlWindow::Acquired()
{
	GehmlRoot::Acquired();
	OpenSession(m_look,B_NORMAL_WINDOW_FEEL,0,B_CURRENT_WORKSPACE);
}

void GehmlWindow::Init()
{
	m_session = NULL;
	m_eventPort = NULL;
	m_hackedView = NULL;
	m_windowToken = m_viewToken = NO_TOKEN;
	m_BWindow = NULL;
	m_look = B_NO_BORDER_WINDOW_LOOK;
	m_color = B_TRANSPARENT_COLOR;
}

BinderNode::property 
GehmlWindow::Parent()
{
	property parent;
	Lock();
	if (m_parent.IsObject()) parent = m_parent;
	else parent = GehmlRoot::Parent();
	Unlock();
	return parent;
}

status_t 
GehmlWindow::SetParent(const binder_node &parent)
{
	if (!GehmlRoot::SetParent(parent)) return B_OK;

	property This = this;

	Lock();
	if (!m_parent.IsUndefined()) m_parent["removeChild"](&This,NULL);
	if (parent) m_parent = parent;
	else m_parent.Undefine();
	if (m_namespace) {
		m_namespace->RenounceAncestry();
		if (!m_parent.IsUndefined()) m_namespace->InheritFrom(m_parent->Property("namespace"));
	}
	if (!m_parent.IsUndefined()) m_parent["addChild"](&This,NULL);
	Unlock();

	return B_OK;
}

void 
GehmlWindow::OpenSession(window_look look, window_feel feel, uint32 flags, int32 workspace)
{
	sem_id clientBlock;
	uint32 firstEvent,atomicVar;
	BRect frame(0,0,63,63);
	int32 dummy;

	m_initLock.Lock();

	m_session = new (nothrow) GehmlSession(app_server_port(), "");
	m_session->swrite_l(GR_CREATE_WINDOW);
	m_session->swrite_l(_find_cur_team_id_());

	m_session->swrite_rect(&frame);
	m_session->swrite_l(look);
	m_session->swrite_l(feel);
	m_session->swrite_l(flags);
	m_session->swrite_l(Token());
	m_session->swrite_l(-1);
	m_session->swrite_l(Port());
	m_session->swrite_l(workspace);
	m_session->swrite_l(strlen(WINDOW_NAME));
	m_session->swrite(strlen(WINDOW_NAME), (char*)WINDOW_NAME);

	m_session->flush();

	m_session->sread_rect(&frame);
	m_session->sread(4, &m_windowToken);
	m_session->sread(4, &m_controlPort);
	m_session->sread(4, &dummy);
	m_session->sread(4, &clientBlock);
	m_session->sread(4, &firstEvent);
	m_session->sread(4, &atomicVar);

	m_eventPort = new (nothrow) _CEventPort_(
		(int32*)rw_offs_to_ptr(atomicVar),
		(uint32*)ro_offs_to_ptr(firstEvent),
		(uint32)ro_offs_to_ptr(0),
		clientBlock);

	m_session->swrite_l(GR_WINDOW_LIMITS);
	m_session->swrite_l(0);
	m_session->swrite_l(100000000);
	m_session->swrite_l(0);
	m_session->swrite_l(100000000);
	m_session->flush();
	m_session->sread(4,&dummy);

	m_session->swrite_l(GR_ADD_VIEW);
	m_session->swrite_rect(&frame);
	m_session->swrite_l(B_SUBPIXEL_PRECISE|B_WILL_DRAW|B_FOLLOW_ALL);
	m_session->swrite_l(0);
	m_session->swrite_l(Token());
	m_session->swrite_l(m_windowToken);
	m_session->flush();
	m_session->sread(4,&m_viewToken);

	m_session->swrite_l(GR_PICK_VIEW);
	m_session->swrite_l(m_viewToken);
	m_session->swrite_l(GR_SET_VIEW_COLOR);
	m_session->swrite_l(*((int32*)&m_color));
	m_session->flush();

	m_hackedView = new BView(BRect(0,0,0,0),"hack",B_FOLLOW_ALL,B_SUBPIXEL_PRECISE);
	m_BWindow = (BWindow*)malloc(sizeof(BWindow));
	m_BWindow->fLastViewToken = m_viewToken;
	m_BWindow->a_session = m_session;
	m_hackedView->owner = m_BWindow;
	free(m_hackedView->fState);

	m_initLock.Unlock();

//	((GroupNamespace*)((BinderNode*)Namespace().Object()))->AddProperty("windowID",m_windowToken);
}

bigtime_t lastResize = 0;

void
GehmlWindow::DequeueFromServer()
{
	BMessage* msg;

	m_initLock.Lock();
	if (m_eventPort) {
		m_eventPort->ProcessPending();
		while ((msg = m_eventPort->GenerateMessage())) PostMessage(msg);
	}
	m_initLock.Unlock();
}

void 
GehmlWindow::Update(BDrawable &into, const BRegion &exposed)
{
	GetLayout().Draw(into,exposed);
}

typedef	struct {
	int32	view_token;
	uint32	flags;
	BRect	current_bound;
	BRect	update_rect;
} update_info;

void
GehmlWindow::DoUpdate()
{
	int32			view_count;
	uint32			view_list,notShared;
	update_info		*infoBase = NULL;
	update_info		*infos=NULL;

	m_fInUpdate = true;

/*
	BErr << "Starting update of window " << Name() << " ("
		 << (void*)m_windowToken << ")" << endl;
*/
	
	// Drawing could take a while, so we want the window to continue
	// handling messages while it happens.
	ResumeScheduling();
	m_drawLock.Lock();

	m_session->swrite_l(E_START_DRAW_WIND);
	m_session->flush();
	m_session->sread(4,&view_count);
	notShared = view_count & 0x80000000;
	view_count &= 0x7FFFFFFF; // view count should now always be 1 in gehml land
	
	if (!view_count) {
		m_session->sread(4,&view_list);
/*
		BErr << "Finished update of window " << Name() << " ("
			 << (void*)m_windowToken << ")" << ": Nothing to update." << endl;
*/
		m_drawLock.Unlock();
		return;
	}

	if (notShared) {
		infoBase = infos = (update_info *)malloc(sizeof(update_info)*view_count);
		m_session->sread(sizeof(update_info)*view_count,infos);
	} else {
		m_session->sread(4,&view_list);
		infos = (update_info *)ro_offs_to_ptr(view_list);
	}

	m_session->swrite_l(E_START_DRAW_VIEW);
	m_session->swrite_l(GR_PICK_VIEW);
	m_session->swrite_l(m_viewToken);

	BAppSessionDrawable context(m_session, m_viewToken, m_hackedView);
	BRegion updateRegion(infos[0].update_rect);
	m_BWindow->fOwner = find_thread(NULL);
	Update(context,updateRegion);

	m_session->swrite_l(E_END_DRAW_WIND);
	m_session->flush();

	m_fInUpdate = false;
	if (notShared) free(infoBase);
/*	
	BErr << "Finished update of window " << Name() << " ("
		 << (void*)m_windowToken << ")" << endl;
*/

	m_drawLock.Unlock();
}

void 
GehmlWindow::MarkDirty(const BRegion &dirty)
{
	m_initLock.Lock();
	bool ok = m_session;
	m_initLock.Unlock();
	if (!ok) return;

	if (const_cast<BRegion&>(dirty).CountRects()) {
		ResumeScheduling();
		m_drawLock.Lock();
		if (m_session) {
			m_session->swrite_l(GR_PICK_VIEW);
			m_session->swrite_l(m_viewToken);
			m_session->swrite_l(GR_INVAL_REGION);
			m_session->swrite_region(&dirty);
			m_session->flush();
		}
		m_drawLock.Unlock();
	}
}

void 
GehmlWindow::ConstraintsChanged()
{
	NotifyListeners(B_PROPERTY_CHANGED,"constraints");
}

status_t 
GehmlWindow::OpenProperties(void **cookie, void *copyCookie)
{
	int32 *i = new int32;
	if (copyCookie) *i = *((int32*)copyCookie);
	else *i = 0;
	*cookie = i;
	return B_OK;
}

status_t 
GehmlWindow::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	int32 *i = (int32*)cookie;
	if (!windowBinderProps[*i]) return ENOENT;
	strncpy(nameBuf,windowBinderProps[*i],*len);
	nameBuf[*len - 1] = 0;
	return B_OK;
}

status_t 
GehmlWindow::CloseProperties(void *cookie)
{
	int32 *i = (int32*)cookie;
	delete i;
	return B_OK;
}

put_status_t 
GehmlWindow::WriteProperty(const char *name, const property &prop)
{
	return GehmlObject::WriteProperty(name,prop);
}

get_status_t 
GehmlWindow::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (!strcmp(name,"token")) {
		prop = property(B_INT32_TYPE,&m_windowToken,4);
	} else if (!strcmp(name,"constraints")) {
		GehmlConstraints cnst;
		GetXYConstraints(cnst);
		prop = property(B_RAW_TYPE,&cnst,sizeof(cnst));
	} else
		return GehmlObject::ReadProperty(name,prop,args);
		
	return B_OK;
}

int32 
GehmlWindow::WindowToken()
{
	return m_windowToken;
}

status_t
GehmlWindow::HandleMessage(BMessage *msg)
{
	BMessage	reply(B_REPLY);
	bool 		handled = false;

//	printf("GehmlWindow handing "); msg->PrintToStream();
	
	switch (msg->what) {
		case B_QUIT_REQUESTED:
		case B_SCREEN_CHANGED:
		case B_WINDOW_ACTIVATED:
		case B_WINDOW_MOVED:
		case B_WORKSPACES_CHANGED:
		case B_WORKSPACE_ACTIVATED:
		case B_MINIMIZE:
		case B_ZOOM:
			/* These are all in the class of messages
			   that a derived object should handle (not me).
			   They for the most part only apply to children
			   of the root window. */
			break;

		case _EVENTS_PENDING_: {
			DequeueFromServer();
			handled = true;
		} break;

		case _UPDATE_: {
			DoUpdate();
			handled = true;
		} break;

		case B_WINDOW_RESIZED: {
			BRect r;
			m_drawLock.Lock();
			m_session->swrite_l(GR_PICK_VIEW);
			m_session->swrite_l(m_viewToken);
			m_session->swrite_l(GR_GET_VIEW_BOUND);
			m_session->flush();
			m_session->sread_rect_a(&r);
			m_drawLock.Unlock();
			SetSize(r.RightBottom() + BPoint(1,1));
/*
			int32 width,height;
			msg->FindInt32("width", &width);
			msg->FindInt32("height", &height);
			SetSize(BPoint(width+1,height+1));
*/
			handled = true;
		} break;

		case B_MOUSE_DOWN:
			DumpInfo(0);
		case B_MOUSE_MOVED:
		case B_MOUSE_UP:
		case B_KEY_UP:
		case B_KEY_DOWN:
			/* These are all in the class of messages
			   that I need to dispatch to views. */
//			printf("ui msg: "); msg->PrintToStream();
			break;

		default:
//			printf("other msg: "); msg->PrintToStream();
			break;
	}

	if (!handled || reply.IsEmpty())
		return GehmlRoot::HandleMessage(msg);

	msg->SendReply(&reply);
	return B_OK;
}

status_t 
GehmlWindow::Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &group)
{
	GehmlWindow *window = new GehmlWindow(attributes);
	child = new GehmlPortal(window,attributes);
	group = window;
	return B_OK;
}
