
#include <appserver2_p/AppServerBitmap.h>
#include <stdlib.h>
#include <kernel/OS.h>
#include <support2/Atom.h>
#include <support2/Handler.h>
#include <render2/Color.h>
#include <appserver2_p/AppServer.h>
#include <render2/RenderPipe.h>
#include <interface2/ViewParent.h>
#include <appserver2_p/AppServerPipe.h>
#include <appserver2_p/AppServerInitProtocol.h>
#include <appserver2_p/AppServerSurfaceProtocol.h>
#include <appserver2_p/AppServerCommandProtocol.h>
#include <appserver2_p/AppServerSurface.h>
#include <appserver2_p/AppServerRootSurface.h>
#include <appserver2_p/AppServerHostedSurface.h>

const color32 B_TRANSPARENT_COLOR(0x77, 0x74, 0x77, 0x00);

atom_ptr<BAppServer> BAppServer::g_default = NULL;

atom_ptr<BAppServer>
BAppServer::Default()
{
	if (g_default == NULL) g_default = new BAppServer;
	return g_default;
}

atom_ptr<IWimpServer> 
IWimpServer::Default()
{
	return BAppServer::Default();
}

BAppServer::BAppServer()
{
	m_nextSessionID = 1;
}

status_t
BAppServer::Acquired(const void* id)
{
	IWimpServer::Acquired(id);
	Connect();
	return B_OK;
}

status_t
BAppServer::Released(const void* id)
{
	return IWimpServer::Released(id);
}

BAppServer::~BAppServer()
{
}

void *
BAppServer::RWOffs2Ptr(uint32 offs)
{
	if (offs == 0xFFFFFFFF) return NULL;
	return ((uint8*)m_shared.rwAddress) + offs;
}

void *
BAppServer::ROOffs2Ptr(uint32 offs)
{
	if (offs == 0xFFFFFFFF) return NULL;
	return ((uint8*)m_shared.roAddress) + offs;
}

void *
BAppServer::GlobalROOffs2Ptr(uint32 offs)
{
	if (offs == 0xFFFFFFFF) return NULL;
	return ((uint8*)m_shared.globalROAddress) + offs;
}

void
BAppServer::SetupSharedHeaps()
{
	BAppServerControlLink link(this);

	area_id rwArea,roArea,globalROArea;
	void *addr;

	link.control->write32(GR_GET_SERVER_HEAP_AREA);
	link.control->flush();
	rwArea = link.control->read32();
	roArea = link.control->read32();
	globalROArea = link.control->read32();

	if (rwArea >= 0) {
		addr = (void*)0xD0000000;
		m_shared.rwArea = rwArea;
		m_shared.rwClone =
			clone_area("rw_server_area",&addr,B_BASE_ADDRESS,
				B_READ_AREA|B_WRITE_AREA,rwArea);
		m_shared.rwAddress = addr;

		addr = (void*)0xDE000000;
		m_shared.roArea = roArea;
		m_shared.roClone =
			clone_area("ro_server_area",&addr,B_BASE_ADDRESS,
				B_READ_AREA|B_WRITE_AREA,roArea);
		m_shared.roAddress = addr;

		addr = (void*)0xDF000000;
		m_shared.globalROArea = globalROArea;
		m_shared.globalROClone =
			clone_area("global_ro_server_area",&addr,B_BASE_ADDRESS,
				B_READ_AREA,globalROArea);
		m_shared.globalROAddress = addr;
	};
}

void BAppServer::Connect()
{
	int32 err,pid,msgCode;
	int32 msg[16];

	char *asName = getenv("APP_SERVER_NAME");
	if (!asName) asName = "picasso";

	m_dummyHandler = new BHandler();
	m_appPort = create_port(1,"as_0");
	pid = find_thread(asName);
	msg[0] = HELLO;
	msg[1] = m_appPort;
	send_data(pid,0,msg,sizeof(msg));
	while ((err = read_port(m_appPort, &msgCode, msg, sizeof(msg))) == B_INTERRUPTED);
	m_serverPort = msg[1];
	fprintf(stdout,"init1 got %ld, %ld\n",err,m_serverPort);

	m_sndPort = create_port(10,"snd");
	m_rcvPort = create_port(10,"rcv");
	m_msgPort = create_port(10,"msg"); // a hack, for now

	msg[0] = GR_NEW_APP;
	msg[1] = m_sndPort;
	msg[2] = m_rcvPort;
	msg[3] = m_msgPort;
	msg[4] = this_team();
	printf("msg[4] == %ld\n",msg[4]);
	while ((err = write_port(m_serverPort, 0, msg, sizeof(msg))) == B_INTERRUPTED);
	fprintf(stdout,"init2 got %ld\n",err);

	msg[0] = INIT_SESSION;
	msg[1] = atomic_add(&m_nextSessionID,1);
	msg[2] = m_rcvPort;
	while ((err = write_port(m_sndPort, 0, msg, sizeof(msg))) == B_INTERRUPTED);
	fprintf(stdout,"init3 got %ld\n",err);

	m_controlSession = new BRenderSocket(
		new BAppServerInputPipe(m_rcvPort,msg[1]),
		new BAppServerOutputPipe(m_sndPort,msg[1],1024)
	);

	SetupSharedHeaps();

	m_surfaceSession = CreateSession();
	m_surfaceSession->write32(GR_CREATE_WINDOW);
	m_surfaceSession->write32(this_team());

	m_surfaceSession->writeRect(BRect(0,0,63,63));
	m_surfaceSession->write32(B_NO_BORDER_WINDOW_LOOK);
	m_surfaceSession->write32(B_NORMAL_WINDOW_FEEL);
	m_surfaceSession->write32(0);
	m_surfaceSession->write32(m_dummyHandler->Token());
	m_surfaceSession->write32(-1);
	m_surfaceSession->write32(m_dummyHandler->Port());
	m_surfaceSession->write32(B_CURRENT_WORKSPACE);
	m_surfaceSession->writestr("dummySurface");

	m_surfaceSession->flush();

	m_surfaceSession->drain(16);
	m_dummyWindowToken = m_surfaceSession->read32();
	m_dummyWindowControlPort = m_surfaceSession->read32();
	m_surfaceSession->drain(16);
	
	m_root = new BAppServerRootWindow(this,B_CURRENT_WORKSPACE);
}

BRenderSocket *
BAppServer::CreateSession()
{
	int32 err,msg[16];
	msg[0] = INIT_SESSION;
	msg[1] = atomic_add(&m_nextSessionID,1);
	msg[2] = create_port(10, "session_rcv");
	msg[3] = create_port(10, "session_snd");

	while ((err = write_port(m_serverPort,0,msg,sizeof(msg))) == B_INTERRUPTED);
	fprintf(stdout,"init4 got %ld\n",err);

	BRenderSocket *sock = new BRenderSocket(
		new BAppServerInputPipe(msg[2],msg[1]),
		new BAppServerOutputPipe(msg[3],msg[1],1024)
	);
	
	return sock;
}

viewlist 
BAppServer::RootWindow()
{
	return m_root;
}

BBitmap *
BAppServer::CreateBitmap(int32 width, int32 height, pixel_format format)
{
	return new BAppServerBitmap(this,width,height,format);
}

BRenderSocket *
BAppServer::LockControl()
{
	m_controlLock.Lock();
	return m_controlSession;
}

void 
BAppServer::UnlockControl()
{
	m_controlLock.Unlock();
}

BRenderSocket *
BAppServer::LockSurface()
{
	m_surfaceLock.Lock();
	return m_surfaceSession;
}

void 
BAppServer::UnlockSurface()
{
	m_surfaceLock.Unlock();
}

/****************************************************************/

BAppServerControlLink::BAppServerControlLink(const atom_ptr<BAppServer> &server)
	: control(server->LockControl()), m_server(server)
{
}

BAppServerControlLink::~BAppServerControlLink()
{
	m_server->UnlockControl();
}

/****************************************************************/

BAppServerSurfaceLink::BAppServerSurfaceLink(const atom_ptr<BAppServer> &server)
	: surface(server->LockSurface()), m_server(server)
{
}

BAppServerSurfaceLink::~BAppServerSurfaceLink()
{
	m_server->UnlockSurface();
}

