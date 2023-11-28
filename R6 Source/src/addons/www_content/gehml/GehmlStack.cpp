
#include "GehmlLayout.h"
#include "GehmlStack.h"

GehmlStack::GehmlStack(BStringMap &attr) : GehmlGroup(attr)
{
}

GehmlStack::~GehmlStack()
{
}

bool
GehmlStack::Constrain()
{
	GehmlConstraint cnst;
	ObjectList list = VisitChildren();

	bool constraintsChanged = false;
	
	for (int32 axis=0;axis<2;axis++) {
		float tmp,altMin=NAN,altMax=NAN,altPref=NAN;
		for (int32 i=0;i<list.CountItems();i++) {
			cnst.Clear();
			list[i]->GetConstraints(axis,cnst);
	
			if (cnst.min.is_absolute()) {
				tmp = cnst.min.to_px();
				if (isnan(altMin) || (!isnan(tmp) && (tmp > altMin))) altMin = tmp;
			}

			if (cnst.max.is_absolute()) {
				tmp = cnst.max.to_px();
				if (isnan(altMax) || (!isnan(tmp) && (tmp < altMax))) altMax = tmp;
			}

/*
			// This is slim grounds on which to prefer a size... let's not do it.
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
GehmlStack::Layout(layoutbuilder_t layout)
{
	BRect bounds = layout.Bounds();
	ObjectList list = VisitChildren();
	for (int32 i=0;i<list.CountItems();i++)
		layout.PlaceChild(list[i],bounds);
}

status_t 
GehmlStack::Constructor(BStringMap &attributes, gehml_obj &child, gehml_group &group)
{
	child = group = new GehmlStack(attributes);
	return B_OK;
}
