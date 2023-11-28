
#ifndef _GEHMLWINDOW_H_
#define _GEHMLWINDOW_H_

#include <Window.h>
#include "GehmlRoot.h"

class _CEventPort_;
class _view_attr_;
class GehmlSession;

class GehmlWindow : public GehmlRoot
{
	public:
										GehmlWindow();
										GehmlWindow(BStringMap &attr);
		virtual							~GehmlWindow();

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

		static	status_t				Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &group);

		virtual	void					Acquired();

	protected:

		int32							WindowToken();
		void							OpenSession(window_look look, window_feel feel, uint32 flags, int32 workspace);

	private:

		void							Init();
		void							DoUpdate();
		void							DequeueFromServer();

		Gehnaphore						m_drawLock;
		Gehnaphore						m_initLock;
		rgb_color						m_color;
		window_look						m_look;
		int32							m_viewToken;
		int32							m_windowToken;
		port_id							m_controlPort;
		_CEventPort_ *					m_eventPort;
		_view_attr_ *					m_renderState;
		GehmlSession *					m_session;
		property						m_parent;
		::BView *						m_hackedView;
		::BWindow *						m_BWindow;
		bool							m_fInUpdate:1;
};

#endif
