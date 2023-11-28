
#ifndef	_APPSERVER2_SURFACE_H_
#define	_APPSERVER2_SURFACE_H_

#include <render2/RenderDefs.h>
#include <interface2/View.h>
#include <appserver2_p/AppServerDefs.h>
#include <appserver2_p/AppServerSurfaceProtocol.h>

namespace B {
namespace AppServer2 {

class BAppServerSurface : public LView
{
	public:

										BAppServerSurface(
											const atom_ptr<BAppServer> &server,
											int32 token,
											const viewparent &host);
										BAppServerSurface(const BAppServerSurface &copyFrom);
		virtual							~BAppServerSurface();

				atom_ptr<BAppServer>	Server();
				int32					ServerToken();
				void					SetFlags(window_look look, window_feel feel, uint32 flags);

		virtual	void					PreTraversal();
		virtual	view					PostTraversal(BRegion &outDirty);

		virtual	status_t				SetParent(const viewparent &parent);
		virtual	void					SetBounds(BRect bounds);
		virtual	void					Hide();
		virtual	void					Show();

		virtual	BLayoutConstraints		Constraints() const;
		virtual	viewparent				Parent() const;
		virtual	BRect					Bounds() const;
		virtual	bool					IsHidden() const;
				
		virtual	void					Draw(const render &into);

		virtual	status_t				DispatchEvent(const BMessage &msg, BPoint where);

	private:

				void					MoveTo(BPoint upperLeft);
				void					ResizeTo(BPoint dimensions);

				void					Movesize(uint32 opcode, float h, float v);

				viewparent_ref			m_parent;
				atom_ptr<BAppServer>	m_server;
				int32					m_parentToken;
				viewparent				m_host;
				int32					m_token;
				BRect					m_bounds;
				bool					m_hidden:1;
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::AppServer2

using namespace B::AppServer2;

#endif /* _APPSERVER2_SURFACE_H_ */
