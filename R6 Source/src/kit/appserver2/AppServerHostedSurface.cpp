
#include <support2/Atom.h>
#include <support2/Debug.h>
#include <support2/Handler.h>
#include <support2/MessageCodes.h>
#include <support2/StdIO.h>
#include <support2/TokenSpace.h>
#include <raster2/RasterRegion.h>
#include <interface2/View.h>
#include <appserver2_p/AppServer.h>
#include <appserver2_p/AppServerHostedSurface.h>
#include <appserver2_p/AppServerCommandProtocol.h>
#include <appserver2_p/AppServerSurfaceProtocol.h>
#include <appserver2_p/AppServerEventPort.h>
#include <appserver2_p/AppServerRenderer.h>
#include <appserver2_p/AppServerLegacy.h>

BAppServerHostedSurface::BAppServerHostedSurface(
	const atom_ptr<BAppServer> &server,
	const view &host,
	const BValue &attr)
	: BViewLayoutRoot(host,attr), m_server(server)
{
	m_eventPort = NULL;
	m_viewToken = NO_TOKEN;
	m_windowToken = NO_TOKEN;
	m_session = NULL;
	m_controlPort = B_BAD_VALUE;
	m_inUpdate = false;
}

status_t
BAppServerHostedSurface::Acquired(const void* id)
{
	BViewLayoutRoot::Acquired(id);
	Connect();
	return B_OK;
}

void BAppServerHostedSurface::Connect()
{
	sem_id clientBlock;
	uint32 firstEvent,atomicVar;
	BRect frame(0,0,63,63);

	m_initLock.Lock();
	m_updateLock.Lock();

	m_session = m_server->CreateSession();
	m_session->write32(GR_CREATE_WINDOW);
	m_session->write32(this_team());

	m_session->writeRect(frame);
	m_session->write32(B_NO_BORDER_WINDOW_LOOK);
	m_session->write32(B_NORMAL_WINDOW_FEEL);
	m_session->write32(B_WILL_ACCEPT_FIRST_CLICK);
	m_session->write32(BHandler::Token());
	m_session->write32(-1);
	m_session->write32(BHandler::Port());
	m_session->write32(B_CURRENT_WORKSPACE);
	m_session->writestr("window");

	m_session->flush();

	m_session->readRect(frame);
	m_windowToken = m_session->read32();
	m_controlPort = m_session->read32();
	m_session->read32(); // what is this?
	m_session->read(&clientBlock,4);
	m_session->read(&firstEvent,4);
	m_session->read(&atomicVar,4);

	m_eventPort = new BAppServerEventPort(
		(int32*)m_server->RWOffs2Ptr(atomicVar),
		(uint32*)m_server->ROOffs2Ptr(firstEvent),
		(uint32)m_server->ROOffs2Ptr(0),
		clientBlock);

	m_session->write32(GR_WINDOW_LIMITS);
	m_session->write32(0);
	m_session->write32(100000000);
	m_session->write32(0);
	m_session->write32(100000000);
	m_session->flush();
	m_session->read32();

	m_session->write32(GR_ADD_VIEW);
	m_session->writeRect(frame);
	m_session->write32(B_SUBPIXEL_PRECISE|B_WILL_DRAW|B_FOLLOW_ALL);
	m_session->write32(0);
	m_session->write32(BHandler::Token());
	m_session->write32(m_windowToken);
	m_session->flush();
	m_viewToken = m_session->read32();

	m_session->write32(GR_PICK_VIEW);
	m_session->write32(m_viewToken);
	m_session->write32(GR_SET_VIEW_COLOR);

	color32 testColor(255,255,0,255);
	m_session->write32(*((int32*)&testColor));
//	m_session->write32(*((int32*)&B_TRANSPARENT_COLOR));

	m_session->flush();

	m_updateLock.Unlock();
	m_initLock.Unlock();
}

status_t
BAppServerHostedSurface::Released(const void* id)
{
	BViewLayoutRoot::Released(id);
}

BAppServerHostedSurface::~BAppServerHostedSurface()
{
	m_session->write32(GR_CLOSE_WINDOW);
	m_session->write32(m_windowToken);
	m_session->flush();
	delete m_session;
}

atom_ptr<BAppServer> 
BAppServerHostedSurface::Server()
{
	return m_server;
}

int32 
BAppServerHostedSurface::ServerToken()
{
	return m_windowToken;
}

void
BAppServerHostedSurface::DequeueFromServer()
{
	BMessage* msg;

	m_initLock.Lock();
	if (m_eventPort) {
		m_eventPort->ProcessPending();
		while ((msg = m_eventPort->GenerateMessage())) PostMessage(*msg);
	}
	m_initLock.Unlock();
}

status_t 
BAppServerHostedSurface::HandleMessage(const BMessage &msg)
{
	switch (msg.What()) {
		case B_QUIT_REQUESTED:
		case B_SCREEN_CHANGED:
		case B_WINDOW_ACTIVATED:
		case B_WINDOW_MOVED:
		case B_WORKSPACES_CHANGED:
		case B_WORKSPACE_ACTIVATED:
		case B_MINIMIZE:
		case B_ZOOM:
			/* These are probably all unneccessary. */
			break;

		case _EVENTS_PENDING_: {
			berr << "_EVENTS_PENDING_" << endl;
			DequeueFromServer();
		} break;

		case _UPDATE_: {
			render into;
			if (Update(into)) {
				Child()->Draw(into);
				into->Flush();
			}
		} break;

		case B_WINDOW_RESIZED: {
			TriggerTraversal(layout);
		} break;

		case B_MOUSE_DOWN:
		case B_MOUSE_MOVED:
		case B_MOUSE_UP:
		case B_KEY_UP:
		case B_KEY_DOWN: {
			/* These are all in the class of messages
			   that I need to dispatch to views. */
//			printf("ui msg: "); msg->PrintToStream();
			BPoint pt;
			if (msg.FindPoint("be:view_where",&pt) == B_OK)
				Child()->DispatchEvent(msg,pt);
		} break;

		default:
			return BViewLayoutRoot::HandleMessage(msg);
	}
	
	return B_OK;
}

BRect 
BAppServerHostedSurface::Bounds()
{
	BRect r;
	m_updateLock.Lock();

		if (m_session) {
			m_session->write32(GR_PICK_VIEW);
			m_session->write32(m_viewToken);
			m_session->write32(GR_GET_VIEW_BOUND);
			m_session->flush();
			m_session->readRect(r);
		}
	
	m_updateLock.Unlock();

	return r;
}

void 
BAppServerHostedSurface::DoConstrain()
{
//	Constrain(m_child->Constraints());
}

void 
BAppServerHostedSurface::DoLayout()
{
	Child()->SetBounds(Bounds());
}

status_t 
BAppServerHostedSurface::InvalidateChild(const view &, BRegion &dirty)
{
	if (dirty.IsEmpty()) return B_OK;

	m_updateLock.Lock();

		if (m_session) {
			m_session->write32(GR_PICK_VIEW);
			m_session->write32(m_viewToken);
			m_session->write32(GR_INVAL_REGION);
			m_session->writeRasterRegion(dirty);
			m_session->flush();
		}
	
	m_updateLock.Unlock();

	return B_OK;
}

bool 
BAppServerHostedSurface::Update(render &renderInto)
{
	int32			viewCount;
	uint32			viewList,notShared;
	update_info		oneInfo;

	m_updateLock.Lock();
	berr << "Starting update of window (" << (void*)m_windowToken << ")" << endl;

	m_session->write32(E_START_DRAW_WIND);
	m_session->flush();
	viewCount = m_session->read32();
	notShared = viewCount & 0x80000000;
	viewCount &= 0x7FFFFFFF; // view count should now always be 1 in gehml land
	
	if (!viewCount) {
		m_session->drain(4);
		berr << "Finished update of window (" << (void*)m_windowToken << ")" << ": Nothing to update." << endl;
		m_updateLock.Unlock();
		return false;
	}

	if (notShared) {
		m_session->read(&oneInfo,sizeof(update_info));
		if (viewCount > 1) m_session->drain(sizeof(update_info)*(viewCount-1));
	} else {
		m_session->read(&viewList,4);
		oneInfo = *((update_info *)m_server->ROOffs2Ptr(viewList));
	}

	m_session->write32(E_START_DRAW_VIEW);
	m_session->write32(GR_PICK_VIEW);
	m_session->write32(m_viewToken);

	BRegion updateRegion;
	updateRegion.Include(oneInfo.update_rect);
	BAppServerUpdateRenderer *asur = new BAppServerUpdateRenderer(this,m_session->Output());
	renderInto = asur;
	asur->SetClippingRegion(updateRegion);
	m_inUpdate = true;
	return true;
}

void 
BAppServerHostedSurface::EndUpdate()
{
	m_session->write32(E_END_DRAW_WIND);
	m_session->flush();
	
	berr << "Finished update of window " << " (" << (void*)m_windowToken << ")" << endl;

	m_inUpdate = false;
	m_updateLock.Unlock();
}
