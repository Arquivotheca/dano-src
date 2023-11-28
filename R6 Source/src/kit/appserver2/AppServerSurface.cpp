
#include <support2/Message.h>
#include <support2/TokenSpace.h>
#include <interface2/ViewParent.h>
#include <appserver2_p/AppServer.h>
#include <appserver2_p/AppServerRenderer.h>
#include <appserver2_p/AppServerSurface.h>
#include <appserver2_p/AppServerSurfaceProtocol.h>
#include <appserver2_p/AppServerCommandProtocol.h>

BAppServerSurface::BAppServerSurface(const atom_ptr<BAppServer> &server, int32 token, const viewparent &host)
	: m_server(server), m_parentToken(NO_TOKEN), m_host(host), m_token(token)
{
}

BAppServerSurface::BAppServerSurface(const BAppServerSurface &copyFrom) : CView()
{
	m_server = copyFrom.m_server;
	m_token = copyFrom.m_token;
	m_hidden = copyFrom.m_hidden;
	m_bounds = copyFrom.m_bounds;
	m_host = copyFrom.m_host;
}

BAppServerSurface::~BAppServerSurface()
{
}

atom_ptr<BAppServer> 
BAppServerSurface::Server()
{
	return m_server;
}

int32 
BAppServerSurface::ServerToken()
{
	return m_token;
}

void 
BAppServerSurface::SetFlags(window_look look, window_feel feel, uint32 flags)
{
	BAppServerSurfaceLink link(Server());
	link.surface->write32(GR_SET_WINDOW_FLAGS);
	link.surface->write32(m_token);
	link.surface->write32(look);
	link.surface->write32(feel);
	link.surface->write32(flags);
	link.surface->flush();
}

status_t 
BAppServerSurface::SetParent(const viewparent &parent)
{
	m_parent = parent;
	if (m_parent == NULL) {
		BAppServerSurfaceLink link(Server());
		link.surface->write32(GR_REMOVE_WINDOW_CHILD);
		link.surface->write32(m_token);
		link.surface->flush();
		link.surface->read32();
		m_parentToken = NO_TOKEN;
	}

	return B_OK;
}

void 
BAppServerSurface::SetBounds(BRect bounds)
{
	if ((bounds.Width() != m_bounds.Width()) ||
		(bounds.Height() != m_bounds.Height()))
		ResizeTo(bounds.Size());
	if ((bounds.left != m_bounds.left) ||
		(bounds.top != m_bounds.top))
		MoveTo(bounds.LeftTop());

	m_bounds = bounds;
}

void 
BAppServerSurface::Hide()
{
//	if (!m_hidden) {
		BAppServerSurfaceLink link(m_server);
		link.surface->write32(GR_HIDE);
		link.surface->write32(m_token);
		link.surface->flush();
		m_hidden = true;
//	}
}

void 
BAppServerSurface::Show()
{
//	if (m_hidden) {
		BAppServerSurfaceLink link(m_server);
		link.surface->write32(GR_SHOW);
		link.surface->write32(m_token);
		link.surface->flush();
		m_hidden = false;
//	}
}

BLayoutConstraints 
BAppServerSurface::Constraints() const
{
	#warning constraints are not correctly propagated!
	return BLayoutConstraints();
}

viewparent 
BAppServerSurface::Parent() const
{
	return m_parent.promote();
}

BRect 
BAppServerSurface::Bounds() const
{
	return m_bounds;
}

bool 
BAppServerSurface::IsHidden() const
{
	return m_hidden;
}

void 
BAppServerSurface::PreTraversal()
{
	// Do nothing!
}

view 
BAppServerSurface::PostTraversal(BRegion &)
{
	// Do nothing!
	return this;
}

void 
BAppServerSurface::Draw(const render &into)
{
	BAppServerUpdateRenderer *asr = dynamic_cast<BAppServerUpdateRenderer*>(into.ptr());
	int32 newParentToken = asr?asr->Surface()->ServerToken():NO_TOKEN;

	if (newParentToken != m_parentToken) {
		int32 replies = 0;
		BAppServerSurfaceLink link(Server());

		link.surface->write32(GR_HIDE);
		link.surface->write32(m_token);

		if (m_parentToken != NO_TOKEN) {
			link.surface->write32(GR_REMOVE_WINDOW_CHILD);
			link.surface->write32(m_token);
			replies++;
		}

		m_parentToken = newParentToken;

		if (m_parentToken != NO_TOKEN) {
			link.surface->write32(GR_ADD_WINDOW_CHILD);
			link.surface->write32(m_parentToken);
			link.surface->write32(m_token);
			replies++;
		}

		link.surface->write32(GR_SHOW);
		link.surface->write32(m_token);

		link.surface->flush();
		while (replies--) link.surface->read32();
	}
}

status_t 
BAppServerSurface::DispatchEvent(const BMessage &, BPoint )
{
	/*	This is how event handling will work in the future... it
		will propagate through clients.  For now, this should never
		get called because the app_server should intercept any events
		at this location and route them to the surface host. */
	return B_ERROR;
}

void 
BAppServerSurface::Movesize(uint32 opcode, float h, float v)
{
	BAppServerSurfaceLink link(m_server);
	link.surface->write32(opcode);
	link.surface->write32(m_token);
	link.surface->write32(floor(h+0.5));
	link.surface->write32(floor(v+0.5));
	link.surface->flush();
}

void 
BAppServerSurface::MoveTo(BPoint upperLeft)
{
	Movesize(GR_MOVETO_WINDOW, upperLeft.x, upperLeft.y);
}

void 
BAppServerSurface::ResizeTo(BPoint dimensions)
{
	Movesize(GR_SIZETO_WINDOW, dimensions.x, dimensions.y);
}
