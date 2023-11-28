
#ifndef	_APPSERVER2_ROOTWINDOW_H_
#define	_APPSERVER2_ROOTWINDOW_H_

#include <support2/SupportDefs.h>
#include <kernel/OS.h>
#include <support2/Atom.h>
#include <support2/Locker.h>
#include <support2/Handler.h>
#include <render2/RenderDefs.h>
#include <raster2/RasterDefs.h>
#include <interface2/ViewParent.h>
#include <interface2/ViewList.h>
#include <appserver2_p/AppServerDefs.h>
#include <appserver2_p/AppServerSurfaceProtocol.h>

namespace B {
namespace AppServer2 {

class BAppServerRootWindow : public BViewList, public CViewParent
{
	public:
										BAppServerRootWindow(
											const atom_ptr<BAppServer> &server,
											int32 workspace = B_CURRENT_WORKSPACE);
		virtual							~BAppServerRootWindow();

				atom_ptr<BAppServer>	Server();
				int32					Workspace();
				BRect					Bounds();

		virtual	status_t				AddChild(const view &child, const BValue &attr);
		virtual	status_t				RemoveChild(const view &child);

		virtual	status_t				ConstrainChild(const view &child, const BLayoutConstraints &constraints);
		virtual	status_t				InvalidateChild(const view &child, BRegion &dirty);
		virtual	void					MarkTraversalPath(int32 type);

	private:

				atom_ptr<BAppServer>	m_server;
				int32					m_workspace;
};

} } // namespace B::AppServer2

using namespace B::AppServer2;

#endif /* _APPSERVER2_ROOTWINDOW_H_ */
