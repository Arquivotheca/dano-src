
#ifndef _INTERFACE2_PORTALPROXY_H_
#define _INTERFACE2_PORTALPROXY_H_

#include <Binder.h>
#include <View.h>

namespace B {
namespace Interface2 {

class BRenderSession;

class BPortalProxy : public BView, public BBinderListener
{
	public:
										BPortalProxy(binder_node window, BStringMap &attributes);
		virtual							~BPortalProxy();

		virtual	status_t				HandleMessage(BMessage *msg);

		virtual	bool					Constrain();
		virtual	bool					Position(layoutbuilder_t layout, BRegion &dirty);
		virtual	void					GetConstraints(int32 axis, BLayoutConstraint &constraint) const;
		virtual	void					Acquired();

		virtual	status_t				Overheard(binder_node node, uint32 event, BString name);

	private:

		void							Init();
		
		BLocker							m_lock;
		property						m_window;
		int32							m_token;
		int32							m_dummyToken;
		int32							m_parentToken;
		port_id							m_controlPort;
		BRect							m_windowBounds;
		BRenderSession *				m_session;
};

} } // namespace B::Interface2

using namespace B::Interface2;

#endif	/* _INTERFACE2_PORTALPROXY_H_ */
