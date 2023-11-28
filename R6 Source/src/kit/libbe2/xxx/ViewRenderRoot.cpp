
#include <ViewRenderRoot.h>

enum {
	bmsgDraw = '_drw'
}

BViewRenderRoot::BViewRenderRoot(const BMessage &attr) : BViewLayoutRoot(attr)
{
}

BViewRenderRoot::~BViewRenderRoot()
{
}

status_t 
BViewRenderRoot::HandleMessage(BMessage *message)
{
	switch (msg->what) {
		case _UPDATE_: {
			render into;
			m_surface->Update(into);
			BLayoutRenderRoot::Draw(m_oldBounds,into);
		} break;
		case B_WINDOW_RESIZED: {
			
			SetSize(r.RightBottom() + BPoint(1,1));
			handled = true;
		} break;

		default:
			return BViewLayoutRoot::HandleMessage(msg);
	}
	
	return B_OK;
}

void 
BViewRenderRoot::Draw(BRect bounds, const render &into)
{
	/*	In a world in which this kit is used by the app_server,
		this would be a direct call from the app_server.  We could
		then grab the IRender interface and asynchonously update our
		window.  For now, though, we should ignore the renderer
		being passed to us because it belongs to our parent window.
		The HostedSurface will take care of sending us an update message. */
}

void 
BViewRenderRoot::NeedDraw(const BRegion &dirty)
{
	m_surface->Invalidate(dirty);
}

