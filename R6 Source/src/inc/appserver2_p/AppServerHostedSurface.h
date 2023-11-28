
#ifndef	_APPSERVER2_HOSTEDSURFACE_H_
#define	_APPSERVER2_HOSTEDSURFACE_H_

#include <kernel/OS.h>
#include <support2/Atom.h>
#include <support2/Locker.h>
#include <support2/Handler.h>
#include <render2/RenderDefs.h>
#include <raster2/RasterDefs.h>
#include <interface2/ViewLayoutRoot.h>
#include <appserver2_p/AppServerDefs.h>
#include <appserver2_p/AppServerSurfaceProtocol.h>

namespace B {
namespace AppServer2 {

class BAppServerHostedSurface : public BViewLayoutRoot {

	public:

										BAppServerHostedSurface(
											const atom_ptr<BAppServer> &server,
											const view &host,
											const BValue &attr);
		virtual							~BAppServerHostedSurface();

		virtual	status_t				Acquired(const void* id);
		virtual	status_t				Released(const void* id);

				atom_ptr<BAppServer>	Server();
				int32					ServerToken();
				
				void					Connect();

		virtual	status_t				HandleMessage(const BMessage &message);

				void					DequeueFromServer();
				BRect					Bounds();

		virtual	status_t				InvalidateChild(const view &child, BRegion &dirty);

		virtual	void					DoConstrain();
		virtual	void					DoLayout();

				bool					Update(render &renderInto);
				void					EndUpdate();

	private:

				BLocker					m_initLock;
				BLocker					m_updateLock;
				atom_ptr<BAppServer>	m_server;
				int32					m_viewToken;
				int32					m_windowToken;
				port_id					m_controlPort;
				BAppServerEventPort *	m_eventPort;
				BRenderSocket *			m_session;
				BRect					m_bounds;
				bool					m_inUpdate:1;
};

} } // namespace B::AppServer2

using namespace B::AppServer2;

#endif /* _APPSERVER2_HOSTEDSURFACE_H_ */
