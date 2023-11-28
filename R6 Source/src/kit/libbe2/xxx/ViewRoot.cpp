
#include <Region.h>
#include <ViewRoot.h>
#include <Drawable.h>

enum {
	rfLayoutPending = 0x00000001
};

BViewRoot::BViewRoot()
{
}

BViewRoot::BViewRoot(const BMessage &attr) : BViewGroup(attr)
{
}

BViewRoot::~BViewRoot()
{
}

BPoint 
BViewRoot::Size()
{
	return m_size;
}

void 
BViewRoot::SetSize(BPoint size)
{
	m_lock.Lock();
	m_size = size;
	m_lock.Unlock();
	NeedLayout();
}

BPoint 
BViewRoot::Position()
{
	return m_position;
}

void 
BViewRoot::SetPosition(BPoint location)
{
	m_lock.Lock();
	m_position = location;
	m_lock.Unlock();
	NeedLayout();
}

void 
BViewRoot::Draw(layout_t , BDrawable &into, const BRegion &dirty)
{
	m_layout.Draw(into,dirty);
}

bool 
BViewRoot::Position(layoutbuilder_t layout, BRegion &)
{
	BRect r = layout.Bounds();
	m_size = BPoint(r.Width(),r.Height());
	m_position = r.LeftTop();
	NeedLayout();
	return false; // We want to be asynchronously layed-out
}

int32 
BViewRoot::PropagateNeeds(int32 needFlags, const view_ptr &)
{
	int32 oldNeeds = m_layout.SetRootNeeds(needFlags);
	if (!oldNeeds) PostMessage(BMessage('trav'));
	return oldNeeds;
}

bool
BViewRoot::Traverse(BRegion &outDirty)
{
	BRegion dirty;

	m_lock.Lock();
	BRect bounds(m_position.x,m_position.y,m_position.x+m_size.x,m_position.y+m_size.y);
	m_layout.SetBounds(bounds);
	m_lock.Unlock();

	return m_layout.Layout(outDirty);
}

BLayout &
BViewRoot::GetLayout()
{
	return m_layout;
}

void 
BViewRoot::Acquired()
{
	BView::Acquired();
	BHandler::Acquired();
	m_layout.SetRoot(this);
}

status_t
BViewRoot::HandleMessage(BMessage *msg)
{
	switch (msg->what) {
		case 'trav': {
			BRegion dirty;
			ResumeScheduling();
			m_traversalLock.Lock();
			bool constaintsChanged = Traverse(dirty);
			m_traversalLock.Unlock();
			if (constaintsChanged) ConstraintsChanged();
			MarkDirty(dirty);
		} break;
		default:
			return BHandler::HandleMessage(msg);
	}
	
	return B_OK;
}

void 
BViewRoot::Released()
{
	m_layout.SetRoot(NULL);
	BViewGroup::Released();
	BHandler::Released();
}
