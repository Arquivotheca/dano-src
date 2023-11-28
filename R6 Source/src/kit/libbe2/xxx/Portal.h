
#ifndef _INTERFACE2_PORTAL_H_
#define _INTERFACE2_PORTAL_H_

#include <ViewRoot.h>

namespace B {
namespace Interface2 {

class BPortal : public BViewRoot
{
	public:
										BPortal();
										BPortal(const BMessage &attr);
		virtual							~BPortal();

		virtual	property				Parent();
		virtual	status_t				SetParent(const binder_node &parent);

		virtual void					Update(BDrawable &into, const BRegion &exposed);
		virtual	void					MarkDirty(const BRegion &dirty);
		virtual	void					ConstraintsChanged();

		virtual	status_t				HandleMessage(BMessage *msg);

		virtual	status_t				OpenProperties(void **cookie, void *copyCookie);
		virtual	status_t				NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t				CloseProperties(void *cookie);

		virtual	put_status_t			WriteProperty(const char *name, const property &prop);
		virtual	get_status_t			ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

		virtual	void					Acquired();

	protected:

		int32							WindowToken();
		void							OpenSession(window_look look, window_feel feel, uint32 flags, int32 workspace);

	private:

		void							Init();
		void							DoUpdate();
		void							DequeueFromServer();

		BLocker							m_drawLock;
		BLocker							m_initLock;
		rgb_color						m_color;
		window_look						m_look;
		int32							m_viewToken;
		int32							m_windowToken;
		port_id							m_controlPort;
		_CEventPort_ *					m_eventPort;
		_view_attr_ *					m_renderState;
		BRenderSession *				m_session;
		property						m_parent;
		bool							m_fInUpdate:1;
};

} } // namespace B::Interface2

using namespace B::Interface2;

#endif	/* _INTERFACE2_PORTAL_H_ */
