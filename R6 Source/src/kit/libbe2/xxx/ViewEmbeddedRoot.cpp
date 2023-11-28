
#include <ViewEmbeddedRoot.h>

BViewEmbeddedRoot::BViewEmbeddedRoot()
{
}

BViewEmbeddedRoot::BViewEmbeddedRoot(const BMessage &attr) : BViewRoot(attr)
{
}

BViewEmbeddedRoot::~BViewEmbeddedRoot()
{
}

void 
BViewEmbeddedRoot::Clean(layout_t layout, BRegion &outWasDirty)
{
	m_dirtyRegionLock.Lock();
	BPoint p = layout.Bounds().LeftTop();
	m_dirtyRegion.OffsetBy(p.x,p.y);
	outWasDirty.Include(m_dirtyRegion);
	m_dirtyRegion.MakeEmpty();
	m_dirtyRegionLock.Unlock();
}

void 
BViewEmbeddedRoot::MarkDirty(const BRegion &dirty)
{
	m_dirtyRegionLock.Lock();
	bool propagate = m_dirtyRegion.IsEmpty();
	m_dirtyRegion.Include(dirty);
	m_dirtyRegionLock.Unlock();
	if (propagate) BViewRoot::PropagateNeeds(nfDraw,this);
}
