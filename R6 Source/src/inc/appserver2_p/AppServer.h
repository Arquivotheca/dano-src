
#ifndef	_APPSERVER2_SERVER_H_
#define	_APPSERVER2_SERVER_H_

#include <support2/Locker.h>
#include <render2/RenderPipe.h>
#include <interface2/WimpServer.h>
#include <appserver2_p/AppServerDefs.h>
#include <kernel/OS.h>

namespace B {
namespace AppServer2 {

struct server_heaps {
	area_id rwArea, rwClone;
	area_id roArea, roClone;
	area_id globalROArea, globalROClone;
	void *rwAddress,*roAddress,*globalROAddress;
};

class BAppServer : public IWimpServer
{
	public:
											BAppServer();
											~BAppServer();
											
		virtual	status_t					Acquired(const void* id);
		virtual	status_t					Released(const void* id);

		static	atom_ptr<BAppServer>		Default();

				void *						RWOffs2Ptr(uint32 offs);
				void *						ROOffs2Ptr(uint32 offs);
				void *						GlobalROOffs2Ptr(uint32 offs);

		virtual	viewlist					RootWindow();
		virtual	BBitmap *					CreateBitmap(int32 width, int32 height, pixel_format format);

				BRenderSocket *				CreateSession();

				BRenderSocket *				LockControl();
				void						UnlockControl();

				BRenderSocket *				LockSurface();
				void						UnlockSurface();

	private:
		static	atom_ptr<BAppServer>		g_default;
				void						Connect();
				void						SetupSharedHeaps();

				BLocker						m_controlLock;
				BLocker						m_surfaceLock;
				int32						m_nextSessionID;
				BRenderSocket *				m_controlSession;
				BRenderSocket *				m_surfaceSession;
				port_id						m_sndPort;
				port_id						m_rcvPort;
				port_id						m_msgPort;
				port_id						m_appPort;
				port_id						m_serverPort;
				server_heaps				m_shared;
				atom_ptr<BHandler>			m_dummyHandler;
				int32						m_dummyWindowToken;
				int32						m_dummyWindowControlPort;
				atom_ptr<BAppServerRootWindow>	m_root;
};

class BAppServerControlLink
{
	public:
								BAppServerControlLink(const atom_ptr<BAppServer> &server);
								~BAppServerControlLink();

		BRenderSocket *			control;

	private:
	
		atom_ptr<BAppServer>	m_server;
};

class BAppServerSurfaceLink
{
	public:
								BAppServerSurfaceLink(const atom_ptr<BAppServer> &server);
								~BAppServerSurfaceLink();

		BRenderSocket *			surface;

	private:
	
		atom_ptr<BAppServer>	m_server;
};

} }	// namespace B::AppServer2

using namespace B::AppServer2;

#endif 	/* _APPSERVER2_SERVER_H_ */
