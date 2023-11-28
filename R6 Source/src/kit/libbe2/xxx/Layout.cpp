
#include <Region.h>
#include <Layout.h>
#include <ViewGroup.h>

enum {
	lfCopyOnWrite = 0x00010000
};

BLayoutData::BLayoutData()
{
}

const BRect &
BLayoutData::Bounds() const
{
	if (m_copy != NULL) return m_copy->m_bounds;
	return m_bounds;
}

BView *
BLayoutData::Base() const
{
	return m_base.ptr();
}

/****************************************************************/

const BRect &
BLayoutBuilder::OldBounds() const
{
	static BRect emptyRect;
	if (m_copy != NULL) return m_bounds;
	return emptyRect;
}

status_t 
BLayoutBuilder::PlaceChild(view_ptr child, BRect bounds)
{
	layout_ptr childLayout = NULL;
	if (This()->m_children) childLayout = This()->m_children->Lookup(child->ID());

	if (childLayout == NULL) return B_ERROR;

	if (childLayout->Bounds() != bounds) {
		childLayout->AssertWritable();
		childLayout = childLayout->This();
		childLayout->SetBounds(bounds);
		This()->m_flags |= nfDescend;
		return B_OK;
	}
}

BLayoutBuilder::BLayoutBuilder(const view_ptr &base)
{
	m_children = NULL;
	m_base = base;
	m_flags = 0;
	m_refs = 0;
}

BLayoutBuilder::BLayoutBuilder(BLayoutBuilder &copyFrom)
	: BLayoutData()
{
	m_children = NULL;
	m_bounds = copyFrom.m_bounds;
	m_base = copyFrom.m_base;
	m_flags = copyFrom.m_flags & ~lfCopyOnWrite;
	m_refs = 0;
	if (copyFrom.m_children) {
		m_children = new BLayoutCollection(*copyFrom.m_children);
	}
}

BLayoutBuilder::~BLayoutBuilder()
{
	if (m_children) delete m_children;
}

BLayoutTop *
BLayoutBuilder::This()
{
	if (m_copy != NULL) return (BLayoutTop*)m_copy.ptr();
	return (BLayoutTop*)this;
}

bool
BLayoutBuilder::AssertWritable()
{
	if (!(m_flags & lfCopyOnWrite)) return false;
	if (m_copy == NULL) {
		m_copy = (BLayoutTop*)(new BLayoutBuilder(*this));
		return true;
	}
	return false;
}

int32
BLayoutBuilder::FirstPass(int32 needs)
{
	BLayoutTop *childLayout;

	This()->m_flags |= needs;
//	This()->m_flags |= nfDraw | nfLayout | nfConstrain | nfDescend;
//	printf("BLayout::FirstPass(%s) %08x\n",Base()->DebugName().String(),This()->m_flags);

	if (!(This()->m_flags & ~lfCopyOnWrite)) return 0;

	if (This()->m_flags & nfAddChild) {
		bool descendAsWell = This()->m_flags & nfDescend;
		BViewGroup *group = dynamic_cast<BViewGroup*>(This()->Base());
		BViewList list = group->VisitChildren();
		if (!This()->m_children) This()->m_children = new BLayoutCollection();
		for (int32 i=0;i<list.CountItems();i++) {
			if (!(childLayout = This()->m_children->Lookup(list[i]->ID()).ptr())) {
				AssertWritable();
				childLayout = (BLayoutTop*)(new BLayoutBuilder(list[i]));
				This()->m_children->Insert(list[i]->ID(),childLayout);
				This()->m_flags |= childLayout->FirstPass(list[i]->TendToNeeds())
					| nfDescend | nfConstrain | nfLayout;
			} else if (descendAsWell) {
				This()->m_flags |= childLayout->FirstPass(list[i]->TendToNeeds());
			}
		}
	} else if (This()->m_flags & nfDescend) {
		BLayoutCollection *children = This()->m_children;
		if (children) {
			for (int32 i=0;i<children->Count();i++) {
				childLayout = (*children)[i].value.ptr();
				This()->m_flags |= childLayout->FirstPass(childLayout->Base()->TendToNeeds());
			}
		}
	}

	if (This()->m_flags & nfConstrain) {
		if (This()->Base()->Constrain()) return nfConstrain|nfLayout;
	}

	return 0;
}

layout_ptr
BLayoutBuilder::SecondPass(BRegion &dirty)
{
	layout_ptr oldOne,newOne;
//	printf("BLayout::SecondPass(%s) %08x -- ",Base()->DebugName().String(),This()->m_flags); m_bounds.PrintToStream();
	if ((This()->m_flags & ~lfCopyOnWrite)) {
		if (This()->m_flags & nfRemoveChild) return NULL;
	
		if (This()->m_flags & nfPosition) {
			if (This()->Base()->Position(*this,dirty))
				This()->m_flags |= nfLayout;
		}
	
		if (This()->m_flags & nfLayout) {
			BViewGroup *group = dynamic_cast<BViewGroup*>((BView*)This()->Base());
			if (group) group->Layout(*this);
		}

		if (This()->m_flags & nfDraw)
			This()->Base()->Clean(*This(),dirty);
	
		if (This()->m_flags & nfDescend) {
//			printf("BLayout::SecondPass(%s) descending %08x\n",This()->Base()->DebugName().String(),This()->m_flags);
			if (This()->m_children) {
				for (int32 i=0;i<This()->m_children->Count();i++) {
					oldOne = (*This()->m_children)[i].value;
					newOne = oldOne->SecondPass(dirty);
					if (oldOne != newOne) {
						AssertWritable();
						if (newOne != NULL)	(*This()->m_children)[i].value = newOne;
						else				This()->m_children->RemoveIndex(i--);
					}
				}
			}
		}
	}

	This()->m_flags = lfCopyOnWrite;
	return This();
}

/****************************************************************/

BLayoutTop::BLayoutTop(const view_ptr &base)
	: BLayoutBuilder(base)
{
}

layout_ptr 
BLayoutTop::Layout(int32 rootNeeds, BRegion &dirty)
{
	m_flags |= rootNeeds;
	return SecondPass(dirty);
}

void 
BLayoutTop::SetBounds(const BRect &bounds)
{
	AssertWritable();
	This()->m_bounds = bounds;
	This()->m_flags |= nfPosition;
}

void 
BLayoutTop::Position(const BRect &bounds)
{
	AssertWritable();
	This()->m_bounds = bounds;
}

void 
BLayoutTop::DrawChildren(BDrawable &into, const BRegion &dirty) const
{
	if (m_children) {
		for (int32 i=0;i<m_children->Count();i++) {
			(*m_children)[i].value->Draw(into,dirty);
		}
	}
}

void 
BLayoutTop::Draw(BDrawable &into, const BRegion &dirty) const
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
BLayoutTop::Constrain(int32 rootNeeds)
{
	return FirstPass(rootNeeds);
}

/****************************************************************/

BLayout::BLayout()
{
	m_rootNeeds = 0;
}

BLayout::BLayout(const view_ptr &root)
{
	SetRoot(root);
	m_rootNeeds = 0;
}

void 
BLayout::SetRoot(view_ptr root)
{
	m_lock.Lock();
	if (root != NULL)
		m_layout = new BLayoutTop(root);
	else
		m_layout = NULL;
	m_lock.Unlock();
}

bool 
BLayout::Layout(BRegion &outDirty)
{
	m_lock.Lock();
	layout_ptr layout = m_layout;
	m_lock.Unlock();

	if (layout != NULL) {
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
BLayout::SetRootNeeds(int32 needs)
{
	return atomic_or(&m_rootNeeds,needs);
}

void 
BLayout::SetBounds(const BRect &newBounds)
{
	m_lock.Lock();
	m_layout->Position(newBounds);
	m_lock.Unlock();
	SetRootNeeds(nfLayout);
}

void 
BLayout::Draw(BDrawable &into, const BRegion &dirty)
{
	m_lock.Lock();
	layout_ptr layout = m_layout;
	m_lock.Unlock();

	if (layout != NULL) layout->DrawChildren(into,dirty);
}
