
#include <support2/StdIO.h>
#include <render2/Region.h>
#include <appserver2_p/AppServer.h>
#include <appserver2_p/AppServerSurface.h>
#include <appserver2_p/AppServerRootSurface.h>
#include <appserver2_p/AppServerHostedSurface.h>
#include <appserver2_p/AppServerCommandProtocol.h>

// Stolen.  FIX!!!!!!!!!!!
#warning Need to allow include of Accelerant.h
//#include <Accelerant.h>
typedef struct {
	uint32	pixel_clock;	/* kHz */
	uint16	h_display;		/* in pixels (not character clocks) */
	uint16	h_sync_start;
	uint16	h_sync_end;
	uint16	h_total;
	uint16	v_display;		/* in lines */
	uint16	v_sync_start;
	uint16	v_sync_end;
	uint16	v_total;
	uint32	flags;			/* sync polarity, etc. */
} display_timing;

typedef struct {
	display_timing	timing;			/* CTRC info */
	uint32		space;				/* pixel configuration */
	uint16		virtual_width;		/* in pixels */
	uint16		virtual_height;		/* in lines */
	uint16		h_display_start;	/* first displayed pixel in line */
	uint16		v_display_start;	/* first displayed line */
	uint32		flags;				/* mode flags */
} display_mode;


BAppServerRootWindow::BAppServerRootWindow(const atom_ptr<BAppServer> &server, int32 workspace)
	: m_server(server), m_workspace(workspace)
{
	
}

BAppServerRootWindow::~BAppServerRootWindow()
{
}

atom_ptr<BAppServer> 
BAppServerRootWindow::Server()
{
	return m_server;
}

int32 
BAppServerRootWindow::Workspace()
{
	return m_workspace;
}

BRect 
BAppServerRootWindow::Bounds()
{
	display_mode mode;
	BAppServerControlLink link(Server());
	link.control->write32(GR_GET_DISPLAY_MODE);
	link.control->write32(0);
	link.control->write32(m_workspace);
	link.control->flush();
	int32 result = link.control->read32();
	if (result == B_OK) {
		link.control->read(&mode,sizeof(display_mode));
		return BRect(0,0,mode.virtual_width,mode.virtual_height);
	}
	
	return BRect();
}

status_t 
BAppServerRootWindow::AddChild(const view &_child, const BValue &_attr)
{
	/* In an ideal world, we would add an intermediate view here
	   which would draw the decor, depending on the flags.  Currently,
	   the decor is drawn internally to the app_server, so instead
	   we need to tell it how to draw the decor. */

	view child = _child;
	BAppServerSurface *surface = dynamic_cast<BAppServerSurface*>(child.ptr());
	
	if (!surface) {
		atom_ptr<BAppServerHostedSurface> ashs = new BAppServerHostedSurface(m_server,child,_attr);
		child = surface = new BAppServerSurface(BAppServer::Default(),ashs->ServerToken(),ashs);
		ashs->Acquire();
	}

	int32 tmp;
	BRect bounds;
	BValueMap attr = _attr.AsMap();
	window_look look = B_TITLED_WINDOW_LOOK;
	window_feel feel = B_NORMAL_WINDOW_FEEL;
	uint32 flags = B_WILL_ACCEPT_FIRST_CLICK;

	if (attr.FindInt32("window_look",&tmp) == B_OK) look = (window_look)tmp;
	if (attr.FindInt32("window_feel",&tmp) == B_OK) feel = (window_feel)tmp;
	if (attr.FindInt32("window_flags",&tmp) == B_OK) flags = tmp;
	
	BRegion bogusDirty;
	surface->SetFlags(look,feel,flags);
	if (attr.FindRect("bounds",&bounds) != B_OK) bounds.Set(0,0,200,200);
	StderrTxt << bounds << endl;
	child->SetBounds(bounds);
	child->Show();
	child = child->PostTraversal(bogusDirty);

	return BViewList::AddChild(child,attr);
}

status_t 
BAppServerRootWindow::RemoveChild(const view &child)
{
	return BViewList::RemoveChild(child);
}

status_t 
BAppServerRootWindow::ConstrainChild(const view &, const BLayoutConstraints &)
{
	// This won't do anything, currently
	return B_OK;
}

status_t 
BAppServerRootWindow::InvalidateChild(const view &, BRegion &)
{
	// This won't do anything, currently
	return B_OK;
}

void 
BAppServerRootWindow::MarkTraversalPath(int32 )
{
}
