
#include <Region.h>
#include "GehmlRoot.h"
#include "BDrawable.h"

enum {
	rfLayoutPending = 0x00000001
};

GehmlRoot::GehmlRoot()
{
}

GehmlRoot::GehmlRoot(BStringMap &attr) : GehmlGroup(attr)
{
}

GehmlRoot::~GehmlRoot()
{
}

BPoint 
GehmlRoot::Size()
{
	return m_size;
}

void 
GehmlRoot::SetSize(BPoint size)
{
	m_lock.Lock();
	m_size = size;
	m_lock.Unlock();
	NeedLayout();
}

BPoint 
GehmlRoot::Position()
{
	return m_position;
}

void 
GehmlRoot::SetPosition(BPoint location)
{
	m_lock.Lock();
	m_position = location;
	m_lock.Unlock();
	NeedLayout();
}

void 
GehmlRoot::Draw(layout_t layout, BDrawable &into, const BRegion &dirty)
{
	BRegion reg(dirty);
	BPoint p(layout.Bounds().LeftTop());
	p.x = floor(p.x);
	p.y = floor(p.y);
	into.MoveOrigin(p);
	reg.OffsetBy(-p.x,-p.y);
	into.PushState();
	m_layout.Draw(into,reg);
	into.PopState();
	into.MoveOrigin(BPoint(-p.x,-p.y));
}

bool 
GehmlRoot::Position(layoutbuilder_t layout, BRegion &)
{
	BRect r = layout.Bounds();
	m_size = BPoint(floor(r.right-r.left+1),floor(r.bottom-r.top+1));
	m_position = BPoint(floor(r.left),floor(r.top));
	NeedLayout();
	return false; // We want to be asynchronously layed-out
}

int32 
GehmlRoot::PropagateNeeds(int32 needFlags, const gehml_obj &)
{
	int32 oldNeeds = m_layout.SetRootNeeds(needFlags);
	if (!oldNeeds) PostMessage(BMessage('trav'));
	return oldNeeds;
}

bool
GehmlRoot::Traverse(BRegion &outDirty)
{
	BRegion dirty;

	m_lock.Lock();
	BRect bounds(0,0,m_size.x-1,m_size.y-1);
	m_layout.SetBounds(bounds);
	m_lock.Unlock();

	return m_layout.Layout(outDirty);
}

GehmlLayout &
GehmlRoot::GetLayout()
{
	return m_layout;
}

void 
GehmlRoot::Acquired()
{
	GehmlObject::Acquired();
	m_layout.SetRoot(this);
}

status_t
GehmlRoot::HandleMessage(BMessage *msg)
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
			return GHandler::HandleMessage(msg);
	}
	
	return B_OK;
}

void 
GehmlRoot::Cleanup()
{
	m_layout.SetRoot(NULL);
	GehmlGroup::Cleanup();
}
