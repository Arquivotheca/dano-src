
#include <Message.h>
#include <ViewGroup.h>
#include <Layout.h>

BViewGroup::BViewGroup()
{
	m_minAbsSize[0] = m_maxAbsSize[0] = m_prefAbsSize[0] = NAN;
	m_minAbsSize[1] = m_maxAbsSize[1] = m_prefAbsSize[1] = NAN;
	m_groupFlags = 0;
}

BViewGroup::BViewGroup(const BMessage &attr, int32 flags) : BView(attr)
{
	BString s;

	m_minAbsSize[0] = m_maxAbsSize[0] = m_prefAbsSize[0] = NAN;
	m_minAbsSize[1] = m_maxAbsSize[1] = m_prefAbsSize[1] = NAN;
	
	m_groupFlags = flags;
	if ((attr.FindString("squeeze",s) == B_OK) && (s == "true"))
		m_groupFlags |= gfSqueezeH | gfSqueezeV;
	if ((attr.FindString("squeezew",s) == B_OK) && (s == "true"))
		m_groupFlags |= gfSqueezeH;
	if ((attr.FindString("squeezeh",s) == B_OK) && (s == "true"))
		m_groupFlags |= gfSqueezeV;
}

BViewGroup::~BViewGroup()
{
}

void 
BViewGroup::AddChild(const view_ptr &child, const BString &name)
{
	m_lock.Lock();
	m_children.AddItem(child);
	if (name.Length()) m_childMap.Insert(name,child);
	m_lock.Unlock();
	child->SetParent(this);
}

BViewList 
BViewGroup::VisitChildren()
{
	BAutolock _auto(m_lock);
	return m_children;
}

void 
BViewGroup::GetConstraints(int32 axis, BLayoutConstraint &cnst) const
{
	BView::GetConstraints(axis,cnst);

	if (!isnan(m_minAbsSize[axis]) && cnst.min.is_undefined())
		cnst.min = dimth(m_minAbsSize[axis],dimth::pixels);

	if (!isnan(m_maxAbsSize[axis]) && cnst.max.is_undefined())
		cnst.max = dimth(m_maxAbsSize[axis],dimth::pixels);

	if (!isnan(m_prefAbsSize[axis]) && cnst.pref.is_undefined())
		cnst.pref = dimth(m_prefAbsSize[axis],dimth::pixels);
}

bool 
BViewGroup::Position(layoutbuilder_t , BRegion &)
{
	return true;
}

void 
BViewGroup::Layout(layoutbuilder_t layout)
{
	BRect rect,bounds = layout.Bounds();
	BViewList list = VisitChildren();
	BLayoutConstraints constraints;
	BPoint size(bounds.right-bounds.left+1,bounds.bottom-bounds.top+1);

//	printf("group: "); bounds.PrintToStream();

	for (int32 i=0;i<list.CountItems();i++) {
		list[i]->GetXYConstraints(constraints);
		rect = constraints.Resolve(size);
		rect.OffsetBy(bounds.LeftTop());
		layout.PlaceChild(list[i],rect);
	}
}

bool
BViewGroup::Constrain()
{
	BLayoutConstraint cnst;
	BViewList list = VisitChildren();

	bool constraintsChanged = false;
	
	for (int32 axis=0;axis<2;axis++) {
		float tmp,altMin=NAN,altMax=NAN,altPref=NAN;
//		int32 prefCount=0;
		for (int32 i=0;i<list.CountItems();i++) {
			cnst.Clear();
			list[i]->GetConstraints(axis,cnst);
	
			tmp = cnst.ImplicitMinSize();
			if (isnan(altMin) || (!isnan(tmp) && (tmp > altMin))) altMin = tmp;

			if (m_groupFlags & (axis+1)) { // squeeze
				tmp = cnst.ImplicitMaxSize();
				if (isnan(altMax) || (!isnan(tmp) && (tmp < altMax))) altMax = tmp;
			}
/*
			// This is slim grounds on which to prefer a size... lets not do it.
			tmp = cnst.ImplicitPrefSize();
			if (!isnan(tmp)) {
				if (isnan(altPref)) altPref = tmp;
				else altPref += tmp;
				prefCount++;
			}
*/
		}

//		if (!isnan(altPref)) altPref /= prefCount;

		constraintsChanged = constraintsChanged ||
			(m_minAbsSize[axis] != altMin) ||
			(m_maxAbsSize[axis] != altMax) ||
			(m_prefAbsSize[axis] != altPref);
	
		m_minAbsSize[axis] = altMin;
		m_maxAbsSize[axis] = altMax;
//		m_prefAbsSize[axis] = altPref;
	}

	return constraintsChanged;
}

void 
BViewGroup::RemoveChild(const view_ptr &)
{
}

void 
BViewGroup::Released()
{
	m_children.MakeEmpty();
	m_childMap.MakeEmpty();
	BView::Released();
}

#if DEBUG_LAYOUT
void 
BViewGroup::DumpInfo(int32 indent)
{
	BView::DumpInfo(indent);	
	BViewList list = VisitChildren();
	for (int32 i=0;i<list.CountItems();i++)
		list[i]->DumpInfo(indent+1);
}
#endif
