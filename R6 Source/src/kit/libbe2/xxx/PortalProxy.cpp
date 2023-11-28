
#include "DirtyHacks.h"
#include <Message.h>
#include <interface_p/interface_misc.h>
#include <app_server_p/messages.h>
#include <app_p/token.h>
#include <support/StreamIO.h>
#include <Binder.h>

#include "BAppSessionDrawable.h"
#include "GehmlPortal.h"
#include "GehmlSession.h"

#define WINDOW_NAME "control"

GehmlPortal::GehmlPortal(binder_node window, BStringMap &attributes) : GehmlObject(attributes)
{
	BRect r;
	int32 dummy;
	property prop;
	m_parentToken = NO_TOKEN;
	m_window = window;
	prop = m_window["token"];
	if (prop.TypedRaw()) m_token = *((int32*)prop.TypedRaw()->buffer);

	m_session = new (nothrow) GehmlSession(app_server_port(), "");
	m_session->swrite_l(GR_CREATE_WINDOW);
	m_session->swrite_l(_find_cur_team_id_());

	m_session->swrite_rect(&r);
	m_session->swrite_l(B_NO_BORDER_WINDOW_LOOK);
	m_session->swrite_l(B_NORMAL_WINDOW_FEEL);
	m_session->swrite_l(0);
	m_session->swrite_l(Token());
	m_session->swrite_l(-1);
	m_session->swrite_l(Port());
	m_session->swrite_l(B_CURRENT_WORKSPACE);
	m_session->swrite_l(strlen(WINDOW_NAME));
	m_session->swrite(strlen(WINDOW_NAME), (char*)WINDOW_NAME);

	m_session->flush();

	m_session->sread_rect(&r);
	m_session->sread(4, &m_dummyToken);
	m_session->sread(4, &m_controlPort);
	m_session->sread(4, &dummy);
	m_session->sread(4, &dummy);
	m_session->sread(4, &dummy);
	m_session->sread(4, &dummy);
}

GehmlPortal::~GehmlPortal()
{
	m_lock.Lock();
	m_session->swrite_l(GR_CLOSE_WINDOW);
	m_session->swrite_l(m_token);
	m_session->swrite_l(GR_CLOSE_WINDOW);
	m_session->swrite_l(m_dummyToken);
	m_session->flush();
	m_session->sclose();
	m_lock.Unlock();
}

void 
GehmlPortal::Acquired()
{
	GehmlObject::Acquired();
	m_window["parent"] = this;
	StartListening(m_window);
	StartListening(Namespace(),B_PROPERTY_CHANGED,"windowID");
}

status_t 
GehmlPortal::HandleMessage(BMessage *msg)
{
	return GHandler::HandleMessage(msg);
}

status_t 
GehmlPortal::Overheard(binder_node from, uint32 , BString name)
{
	if (from == m_window) {
		if (name == "constraints") NeedConstrain();
	} else {
		// from namespace
		int32 newParentToken=NO_TOKEN,result;
		property prop = Namespace()["windowID"];
		if (prop.IsNumber()) newParentToken = (int32)prop.Number();

		m_lock.Lock();
		if (newParentToken != m_parentToken) {
			if (m_parentToken != NO_TOKEN) {
				m_session->swrite_l(GR_REMOVE_WINDOW_CHILD);
				m_session->swrite_l(m_token);
				m_session->flush();
				m_session->sread(4, &result);
			}

			m_parentToken = newParentToken;
	
			if (m_parentToken != NO_TOKEN) {
				m_session->swrite_l(GR_ADD_WINDOW_CHILD);
				m_session->swrite_l(m_parentToken);
				m_session->swrite_l(m_token);
				m_session->flush();
				m_session->sread(4, &result);
			}
		}
		m_lock.Unlock();
	}

	return B_OK;
}

bool 
GehmlPortal::Constrain()
{
	property prop = m_window["constraints"];
	if (prop.TypedRaw()->buffer && (prop.TypedRaw()->len == sizeof(GehmlConstraints))) {
		*Constraints() = *((GehmlConstraints*)(prop.TypedRaw()->buffer));
		return true;
	}
	return false;
}

bool 
GehmlPortal::Position(layoutbuilder_t layout, BRegion &)
{
	BRect bounds = layout.Bounds();

	m_lock.Lock();
	m_session->swrite_l(GR_MOVETO_WINDOW);
	m_session->swrite_l(m_token);
	m_session->swrite_l(floor(bounds.left+0.5));
	m_session->swrite_l(floor(bounds.top+0.5));
	m_session->swrite_l(GR_SIZETO_WINDOW);
	m_session->swrite_l(m_token);
	m_session->swrite_l(floor(bounds.right-bounds.left+1));
	m_session->swrite_l(floor(bounds.bottom-bounds.top+1));
	if (!m_windowBounds.IsValid()) {
		m_session->swrite_l(GR_SHOW);
		m_session->swrite_l(m_token);
	}
	m_session->flush();
	m_windowBounds = bounds;
	m_lock.Unlock();
	
	return false;
}	

void 
GehmlPortal::GetConstraints(int32 axis, GehmlConstraint &constraint) const
{
	GehmlObject::GetConstraints(axis,constraint);
}
