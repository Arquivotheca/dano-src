
#ifndef _GEHMLPORTAL_H_
#define _GEHMLPORTAL_H_

#include "GehmlObject.h"

class GehmlSession;

class GehmlPortal : public GehmlObject, public BinderListener
{
	public:
										GehmlPortal(binder_node window, BStringMap &attributes);
		virtual							~GehmlPortal();

		virtual	status_t				HandleMessage(BMessage *msg);

		virtual	bool					Constrain();
		virtual	bool					Position(layoutbuilder_t layout, BRegion &dirty);
		virtual	void					GetConstraints(int32 axis, GehmlConstraint &constraint) const;
		virtual	void					Acquired();

		virtual	status_t				Overheard(binder_node node, uint32 event, BString name);

	private:

		void							Init();
		
		Gehnaphore						m_lock;
		property						m_window;
		int32							m_token;
		int32							m_dummyToken;
		int32							m_parentToken;
		port_id							m_controlPort;
		BRect							m_windowBounds;
		GehmlSession *					m_session;
};

#endif
