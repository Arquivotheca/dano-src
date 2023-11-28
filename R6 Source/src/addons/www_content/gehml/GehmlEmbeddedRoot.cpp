
#include "GehmlEmbeddedRoot.h"

GehmlEmbeddedRoot::GehmlEmbeddedRoot()
{
}

GehmlEmbeddedRoot::GehmlEmbeddedRoot(BStringMap &attr) : GehmlRoot(attr)
{
}

GehmlEmbeddedRoot::~GehmlEmbeddedRoot()
{
}

void 
GehmlEmbeddedRoot::Clean(layout_t layout, BRegion &outWasDirty)
{
	m_dirtyRegionLock.Lock();
	BPoint p = layout.Bounds().LeftTop();
	m_dirtyRegion.OffsetBy(p.x,p.y);
	outWasDirty.Include(&m_dirtyRegion);
	m_dirtyRegion.MakeEmpty();
	m_dirtyRegionLock.Unlock();
}

void 
GehmlEmbeddedRoot::MarkDirty(const BRegion &dirty)
{
	m_dirtyRegionLock.Lock();
	bool propagate = !m_dirtyRegion.CountRects();
	m_dirtyRegion.Include(&dirty);
	m_dirtyRegionLock.Unlock();
	if (propagate) GehmlGroup::PropagateNeeds(nfDraw,this);
}

status_t 
GehmlEmbeddedRoot::Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &group)
{
	child = group = new GehmlEmbeddedRoot(attributes);
	return B_OK;
}
