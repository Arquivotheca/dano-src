
#include <Region.h>
#include "GehmlLayout.h"
#include "GehmlGroup.h"

enum {
	lfCopyOnWrite = 0x00010000
};

GehmlLayoutData::GehmlLayoutData()
{
}

const BRect &
GehmlLayoutData::Bounds() const
{
	if (m_copy) return m_copy->m_bounds;
	return m_bounds;
}

GehmlObject *
GehmlLayoutData::Base() const
{
	return m_base;
}

/****************************************************************/

const BRect &
GehmlLayoutBuilder::OldBounds() const
{
	static BRect emptyRect;
	if (m_copy) return m_bounds;
	return emptyRect;
}

status_t 
GehmlLayoutBuilder::PlaceChild(gehml_obj child, BRect bounds)
{
	gehml_layout childLayout = NULL;
	if (This()->m_children) childLayout = This()->m_children->Lookup(child->ID());

	if (!childLayout) return B_ERROR;

	if (childLayout->Bounds() != bounds) {
		childLayout->AssertWritable();
		childLayout = childLayout->This();
		childLayout->SetBounds(bounds);
		This()->m_flags |= nfDescend;
		return B_OK;
	}
}

GehmlLayoutBuilder::GehmlLayoutBuilder(const gehml_obj &base)
{
	m_children = NULL;
	m_base = base;
	m_flags = 0;
	m_refs = 0;
}

GehmlLayoutBuilder::GehmlLayoutBuilder(GehmlLayoutBuilder &copyFrom)
	: GehmlLayoutData()
{
	m_children = NULL;
	m_bounds = copyFrom.m_bounds;
	m_base = copyFrom.m_base;
	m_flags = copyFrom.m_flags & ~lfCopyOnWrite;
	m_refs = 0;
	if (copyFrom.m_children) {
		m_children = new GehmlLayoutCollection(*copyFrom.m_children);
	}
}

GehmlLayoutBuilder::~GehmlLayoutBuilder()
{
	if (m_children) delete m_children;
}

GehmlLayoutTop *
GehmlLayoutBuilder::This()
{
	if (m_copy) return (GehmlLayoutTop*)m_copy;
	return (GehmlLayoutTop*)this;
}

bool
GehmlLayoutBuilder::AssertWritable()
{
	if (!(m_flags & lfCopyOnWrite)) return false;
	if (!m_copy) {
		m_copy = (GehmlLayoutTop*)(new GehmlLayoutBuilder(*this));
		return true;
	}
	return false;
}

int32
GehmlLayoutBuilder::FirstPass(int32 needs)
{
	GehmlLayoutTop *childLayout;

	This()->m_flags |= needs;
//	This()->m_flags |= nfDraw | nfLayout | nfConstrain | nfDescend;
//	printf("GehmlLayout::FirstPass(%s) %08x\n",Base()->DebugName().String(),This()->m_flags);

	if (!(This()->m_flags & ~lfCopyOnWrite)) return 0;

	if (This()->m_flags & nfAddChild) {
		bool descendAsWell = This()->m_flags & nfDescend;
		GehmlGroup *group = dynamic_cast<GehmlGroup*>(This()->Base());
		ObjectList list = group->VisitChildren();
		if (!This()->m_children) This()->m_children = new GehmlLayoutCollection();
		for (int32 i=0;i<list.CountItems();i++) {
			if (!(childLayout = This()->m_children->Lookup(list[i]->ID()))) {
				AssertWritable();
				childLayout = (GehmlLayoutTop*)(new GehmlLayoutBuilder(list[i]));
				This()->m_children->Insert(list[i]->ID(),childLayout);
				This()->m_flags |= childLayout->FirstPass(list[i]->TendToNeeds())
					| nfDescend | nfConstrain | nfLayout;
			} else if (descendAsWell) {
				This()->m_flags |= childLayout->FirstPass(list[i]->TendToNeeds());
			}
		}
	} else if (This()->m_flags & nfDescend) {
		GehmlLayoutCollection *children = This()->m_children;
		if (children) {
			for (int32 i=0;i<children->Count();i++) {
				childLayout = (*children)[i].value;
				This()->m_flags |= childLayout->FirstPass(childLayout->Base()->TendToNeeds());
			}
		}
	}

	if (This()->m_flags & nfConstrain) {
		if (This()->Base()->Constrain()) return nfConstrain|nfLayout;
	}

	return 0;
}

gehml_layout 
GehmlLayoutBuilder::SecondPass(BRegion &dirty)
{
	gehml_layout oldOne,newOne;
//	printf("GehmlLayout::SecondPass(%s) %08x -- ",Base()->DebugName().String(),This()->m_flags); m_bounds.PrintToStream();
	if ((This()->m_flags & ~lfCopyOnWrite)) {
		if (This()->m_flags & nfRemoveChild) return NULL;
	
		if (This()->m_flags & nfPosition) {
			if (This()->Base()->Position(*this,dirty))
				This()->m_flags |= nfLayout;
		}
	
		if (This()->m_flags & nfLayout) {
			GehmlGroup *group = dynamic_cast<GehmlGroup*>((GehmlObject*)This()->Base());
			if (group) group->Layout(*this);
		}

		if (This()->m_flags & nfDraw)
			This()->Base()->Clean(*This(),dirty);
	
		if (This()->m_flags & nfDescend) {
//			printf("GehmlLayout::SecondPass(%s) descending %08x\n",This()->Base()->DebugName().String(),This()->m_flags);
			if (This()->m_children) {
				for (int32 i=0;i<This()->m_children->Count();i++) {
					oldOne = (*This()->m_children)[i].value;
					newOne = oldOne->SecondPass(dirty);
					if (oldOne != newOne) {
						AssertWritable();
						if (newOne)	(*This()->m_children)[i].value = newOne;
						else		This()->m_children->RemoveIndex(i--);
					}
				}
			}
		}
	}

	This()->m_flags = lfCopyOnWrite;
	return This();
}

/****************************************************************/

GehmlLayoutTop::GehmlLayoutTop(const gehml_obj &base)
	: GehmlLayoutBuilder(base)
{
}

gehml_layout 
GehmlLayoutTop::Layout(int32 rootNeeds, BRegion &dirty)
{
//	m_flags |= rootNeeds;
	return SecondPass(dirty);
}

void 
GehmlLayoutTop::SetBounds(const BRect &bounds)
{
	AssertWritable();
	This()->m_bounds = bounds;
	This()->m_flags |= nfPosition;
}

void 
GehmlLayoutTop::Position(const BRect &bounds)
{
	AssertWritable();
	This()->m_bounds = bounds;
}

void 
GehmlLayoutTop::DrawChildren(BDrawable &into, const BRegion &dirty) const
{
	if (m_children) {
		for (int32 i=0;i<m_children->Count();i++) {
			(*m_children)[i].value->Draw(into,dirty);
		}
	}
}

void 
GehmlLayoutTop::Draw(BDrawable &into, const BRegion &dirty) const
{
/*
	if (!strcmp(Base()->DebugName().String(),"windowBackground")) {
		debugger("woohoo");
	}
*/
	bool draw = dirty.Intersects(m_bounds);
//	printf("::Draw(%s,%s) ",Base()->DebugName().String(),draw?"true":"false");
//	m_bounds.PrintToStream();
	if (draw) {
		m_base->Draw(*this,into,dirty);
		DrawChildren(into,dirty);
	}
}

int32
GehmlLayoutTop::Constrain(int32 rootNeeds)
{
	return FirstPass(rootNeeds);
}

/****************************************************************/

GehmlLayout::GehmlLayout()
{
	m_rootNeeds = 0;
}

GehmlLayout::GehmlLayout(const gehml_obj &root)
{
	SetRoot(root);
	m_rootNeeds = 0;
}

void 
GehmlLayout::SetRoot(gehml_obj root)
{
	m_lock.Lock();
	if (root)
		m_layout = new GehmlLayoutTop(root);
	else
		m_layout = NULL;
	m_lock.Unlock();
}

bool 
GehmlLayout::Layout(BRegion &outDirty)
{
	m_lock.Lock();
	gehml_layout layout = m_layout;
	m_lock.Unlock();

	if (layout) {
		int32 rootNeeds = atomic_and(&m_rootNeeds,0);
		rootNeeds |= layout->Constrain(rootNeeds);
		layout = layout->Layout(rootNeeds,outDirty);
	
		m_lock.Lock();
		m_layout = layout;
		m_lock.Unlock();
		
		if (rootNeeds & nfConstrain) return true;
	}
	
	return false;
}

int32 
GehmlLayout::SetRootNeeds(int32 needs)
{
	return atomic_or(&m_rootNeeds,needs);
}

void 
GehmlLayout::SetBounds(const BRect &newBounds)
{
	m_lock.Lock();
	m_layout->Position(newBounds);
	m_lock.Unlock();
	SetRootNeeds(nfLayout);
}

void 
GehmlLayout::Draw(BDrawable &into, const BRegion &dirty)
{
	m_lock.Lock();
	gehml_layout layout = m_layout;
	m_lock.Unlock();

	if (layout) layout->DrawChildren(into,dirty);
}
