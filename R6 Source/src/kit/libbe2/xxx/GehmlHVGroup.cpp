
#include <math.h>
#include "GehmlLayout.h"
#include "GehmlHVGroup.h"

GehmlHVGroup::GehmlHVGroup(BStringMap &attributes, int8 hOrV) : GehmlGroup(attributes)
{
	m_orientation = hOrV;
	ConstraintsChanged();
}

GehmlHVGroup::~GehmlHVGroup()
{
}

void 
GehmlHVGroup::Layout(layoutbuilder_t layout)
{
	float tmp,*thisOne,*nextOne,otherAxis,accum=0;
	ObjectList list = VisitChildren();
	BRect childBounds,bounds = layout.Bounds();
	int32 total=0,axis,inc,need,parts[list.CountItems()];
	GehmlConstraint constraints[list.CountItems()];

//	printf("hvgroup: "); bounds.PrintToStream();

	axis = m_orientation;
	if (m_orientation == HORIZONTAL) {
		thisOne = &bounds.left;
		nextOne = &bounds.right;
		need = (int32)(bounds.right - bounds.left + 1);
		otherAxis = bounds.bottom - bounds.top + 1;
	} else {
		thisOne = &bounds.top;
		nextOne = &bounds.bottom;
		need = (int32)(bounds.bottom - bounds.top + 1);
		otherAxis = bounds.right - bounds.left + 1;
	}
	
	for (int32 i=0;i<list.CountItems();i++) {
		list[i]->GetConstraints(axis,constraints[i]);
		tmp = constraints[i].Pref(need);
		if (isnan(tmp)) tmp = constraints[i].Min(need);
		if (isnan(tmp)) tmp = 0;
		accum += tmp;
		parts[i] = (int32)(accum - total);
		total += parts[i];
	}

//	printf("(%ld/%ld)\n",need,total);

	if (total < need) {
		/* There is extra space to fill. */
	
		for (int32 i=0;i<list.CountItems();i++) {
			tmp = constraints[i].Max(need);
//			printf("max[%ld] = %f ",i,tmp);
//			constraints[i].PrintToStream();
//			printf("\n");
			if (isinf(tmp))
				inc = (int32)(need-total);
			else {
				inc = (int32)tmp;
				inc = inc - parts[i];
				if (inc < 0) inc = 0;
				if ((total+inc) > need) inc = need-total;
			}
			parts[i] += inc;
			total += inc;
			if (total == need) break;
		}
	} else if (total > need) {
		/* There is not enough space to give everyone their preferred size.
		   Scale it back, using the preferred size as a weight.  */
		dimth tmpDimth;
		float thisTotalnm=0,totalnm=0,totalpx=0;
		int32 removed=0,diff = total-need,tmpInt;
		for (int32 i=0;i<list.CountItems();i++) {
			tmpDimth = constraints[i].pref;
			if (tmpDimth.is_relative())
				totalnm += tmpDimth.to_nm();
			else if (tmpDimth.is_absolute())
				totalpx += tmpDimth.to_px();
		}

		for (int32 i=0;i<list.CountItems();i++) {
			tmpDimth = constraints[i].pref;
			if (tmpDimth.is_relative()) {
				tmp = tmpDimth.to_nm();
				thisTotalnm += tmp;
				tmpInt = (int32)((thisTotalnm * diff / totalnm) + 0.5);
				inc = tmpInt - removed;
				tmp = constraints[i].Min(need);
				if ((parts[i]-inc) < tmp) inc = (int32)(parts[i]-tmp);
				if ((total-inc) < need) inc = total-need;
				parts[i] -= inc;
				total -= inc;
				removed += inc;
				if (total == need) break;
			}
		}

		for (int32 i=0;i<list.CountItems();i++) {
			tmp = constraints[i].Min(need);
			inc = (int32)tmp;
			inc = parts[i] - inc;
			if (inc < 0) inc = 0;
			if ((total-inc) < need) inc = total-need;
			parts[i] -= inc;
			total -= inc;
			if (total == need) break;
		}

	}		

	GehmlConstraint cnst;
	for (int32 i=0;i<list.CountItems();i++) {
//		printf("parts[%ld]=%ld\n",i,parts[i]);
		list[i]->GetConstraints(!axis,cnst);
		*nextOne = *thisOne + parts[i] - 1;
		childBounds.top = bounds.top;
		childBounds.bottom = bounds.bottom;
		cnst.Resolve(otherAxis,childBounds.left,childBounds.right);
		childBounds.left += bounds.left;
		childBounds.right += childBounds.left-1;
		layout.PlaceChild(list[i],childBounds);
		*thisOne += parts[i];
	}
}

struct hvgroup_limits {
	float	pxMinUsed,pxMaxUsed,pxPrefUsed;
	float	nmMinUsed,nmMaxUsed,nmPrefUsed;
	hvgroup_limits()
		: pxMinUsed(0),pxMaxUsed(0),pxPrefUsed(0),
		  nmMinUsed(0),nmMaxUsed(0),nmPrefUsed(0) {};
};

bool 
GehmlHVGroup::Constrain()
{
	bool havePref = false;
	ObjectList list = VisitChildren();
	GehmlConstraint cnst;
	hvgroup_limits limits;
	float tmp,min,max,pref,altMin=NAN,altMax=NAN,altPref=0;
	int32 prefCount=0;
	
	for (int32 i=0;i<list.CountItems();i++) {
		cnst.Clear();
		list[i]->GetConstraints(m_orientation,cnst);

		if (cnst.min.is_relative())
			limits.nmMinUsed += cnst.min.to_nm();
		else if (cnst.min.is_absolute())
			limits.pxMinUsed += cnst.min.to_px();

		if (!isinf(limits.pxMaxUsed)) {
			if (cnst.max.is_relative())
				limits.nmMaxUsed += cnst.max.to_nm();
			else if (cnst.max.is_absolute())
				limits.pxMaxUsed += cnst.max.to_px();
			else
				limits.pxMaxUsed = INFINITY;
		}

		if (!cnst.pref.is_undefined()) {
			havePref = true;
			if (cnst.pref.is_relative())
				limits.nmPrefUsed += cnst.pref.to_nm();
			else
				limits.pxPrefUsed += cnst.pref.to_px();
		} else {
			if (cnst.min.is_relative())
				limits.nmPrefUsed += cnst.min.to_nm();
			else if (cnst.min.is_absolute())
				limits.pxPrefUsed += cnst.min.to_px();
		}
		
		cnst.Clear();
		list[i]->GetConstraints(!m_orientation,cnst);

		tmp = cnst.ImplicitMinSize();
		if (isnan(altMin) || (!isnan(tmp) && (tmp > altMin))) altMin = tmp;

		if (m_groupFlags & ((!m_orientation)+1)) { // squeeze
			tmp = cnst.ImplicitMaxSize();
			if (isnan(altMax) || (!isnan(tmp) && (tmp < altMax))) altMax = tmp;
		}

		tmp = cnst.ImplicitPrefSize();
		if (!isnan(tmp)) {
			altPref += tmp;
			prefCount++;
		}
	}
/*
	printf("limits.nmMinUsed = %f\n",limits.nmMinUsed);
	printf("limits.pxMinUsed = %f\n",limits.pxMinUsed);
	printf("limits.nmMaxUsed = %f\n",limits.nmMaxUsed);
	printf("limits.pxMaxUsed = %f\n",limits.pxMaxUsed);
*/
	// Calculate the minimum pixel size implied by these constraints
	if (limits.nmMinUsed >= 1.0) {
		if (limits.pxMinUsed) printf("unable to satisfy minimum size constraints\n");
		min = limits.pxMinUsed;
	} else if (limits.pxMinUsed)
		min = limits.pxMinUsed / (1.0 - limits.nmMinUsed);
	else
		min = NAN;

	// Calculate the maximum pixel size implied by these constraints
	if (limits.nmMaxUsed >= 1.0) {
		if (limits.pxMaxUsed) printf("unable to satisfy maximum size constraints\n");
		max = NAN;
	} else {
		if (isinf(limits.pxMaxUsed))
			max = NAN;	
		else
			max = limits.pxMaxUsed / (1.0 - limits.nmMaxUsed);
	}
	
	// Calculate the preferred pixel size implied by these constraints
	if (havePref) {
		if (limits.nmPrefUsed >= 1.0) {
			if (limits.pxPrefUsed) printf("unable to satisfy preferred size constraints\n");
			pref = NAN;
		} else
			pref = limits.pxPrefUsed / (1.0 - limits.nmPrefUsed);
	} else
		pref = NAN;

	if (prefCount) altPref = altPref/prefCount;
	else altPref = NAN;

	bool constraintsChanged =
		(m_minAbsSize[m_orientation] != min) ||
		(m_maxAbsSize[m_orientation] != max) ||
		(m_prefAbsSize[m_orientation] != pref) ||
		(m_minAbsSize[!m_orientation] != altMin) ||
		(m_maxAbsSize[!m_orientation] != altMax) ||
		(m_prefAbsSize[!m_orientation] != altPref);

//	printf("hvgroup constrain %f,%f,%f\n",min,max,pref);

	m_minAbsSize[m_orientation] = min;
	m_maxAbsSize[m_orientation] = max;
	m_prefAbsSize[m_orientation] = pref;

	m_minAbsSize[!m_orientation] = altMin;
	m_maxAbsSize[!m_orientation] = altMax;
	m_prefAbsSize[!m_orientation] = altPref;
	
	return constraintsChanged;
}

status_t 
GehmlHVGroup::HConstructor(BStringMap &attributes, gehml_obj &child, gehml_group &group)
{
	child = group = new GehmlHVGroup(attributes,HORIZONTAL);
	return B_OK;
}

status_t 
GehmlHVGroup::VConstructor(BStringMap &attributes, gehml_obj &child, gehml_group &group)
{
	child = group = new GehmlHVGroup(attributes,VERTICAL);
	return B_OK;
}
