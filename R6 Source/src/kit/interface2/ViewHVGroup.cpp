#include <sys/param.h>

#include <render2/Update.h>
#include <interface2/View.h>
#include <interface2/ViewHVGroup.h>

namespace B {
namespace Interface2 {

BViewHVGroup::BViewHVGroup(const BValue &attr) : BViewLayout(attr), m_squeeze(0)
{
	BString s;
	int32 axis;

	if (attr.ValueFor("orientation").GetInt32(&axis) == B_OK) {
		if (axis == B_VERTICAL) {
			m_orientation = B_VERTICAL;
		}
		else {
			m_orientation = B_HORIZONTAL;
		}
	}
	if ((attr.ValueFor("squeeze").GetString(&s) == B_OK) && (s == "true"))	m_squeeze = true;
	if ((attr.ValueFor("squeezew").GetString(&s) == B_OK) && (s == "true")
	    && (m_orientation == B_VERTICAL)) m_squeeze = true;
	if ((attr.ValueFor("squeezeh").GetString(&s) == B_OK) && (s == "true")
	    && (m_orientation == B_HORIZONTAL))	m_squeeze = true;
}

BViewHVGroup::BViewHVGroup(const BViewHVGroup &copyFrom)
	:	BViewLayout(copyFrom),
		m_internal_cnst(copyFrom.m_internal_cnst),
		m_orientation(copyFrom.m_orientation),
		m_squeeze(copyFrom.m_squeeze)
{
}

BView * 
BViewHVGroup::Copy()
{
	return new BViewHVGroup(*this);
}

BViewHVGroup::~BViewHVGroup()
{
}

struct hvgroup_limits {
	float	pxMinUsed,pxMaxUsed,pxPrefUsed;
	float	nmMinUsed,nmMaxUsed,nmPrefUsed;
	hvgroup_limits()
		: pxMinUsed(0),pxMaxUsed(0),pxPrefUsed(0),
		  nmMinUsed(0),nmMaxUsed(0),nmPrefUsed(0) {};
};

BLayoutConstraints 
BViewHVGroup::InternalConstraints() const
{
	bool havePref = false;
	BLayoutConstraints cnst;
	hvgroup_limits limits;
	float tmp, min, max, pref, altMin = NAN, altMax = NAN, altPref = 0;
	int32 prefCount = 0;
	orientation other_axis = (m_orientation == B_VERTICAL) ? B_HORIZONTAL : B_VERTICAL;
	
	m_internal_cnst = BLayoutConstraints();
	int32 count = _Count();
	for (int32 i = 0; i < count; i++) {
		cnst = _ChildAt(i)->Constraints();

		if (cnst.axis[m_orientation].min.is_relative())
			limits.nmMinUsed += cnst.axis[m_orientation].min.to_nm();
		else if (cnst.axis[m_orientation].min.is_absolute())
			limits.pxMinUsed += cnst.axis[m_orientation].min.to_px();

		if (!isinf(limits.pxMaxUsed)) {
			if (cnst.axis[m_orientation].max.is_relative())
				limits.nmMaxUsed += cnst.axis[m_orientation].max.to_nm();
			else if (cnst.axis[m_orientation].max.is_absolute())
				limits.pxMaxUsed += cnst.axis[m_orientation].max.to_px();
			else
				limits.pxMaxUsed = INFINITY;
		}

		if (!cnst.axis[m_orientation].pref.is_undefined()) {
			havePref = true;
			if (cnst.axis[m_orientation].pref.is_relative())
				limits.nmPrefUsed += cnst.axis[m_orientation].pref.to_nm();
			else
				limits.pxPrefUsed += cnst.axis[m_orientation].pref.to_px();
		} else {
			if (cnst.axis[m_orientation].min.is_relative())
				limits.nmPrefUsed += cnst.axis[m_orientation].min.to_nm();
			else if (cnst.axis[m_orientation].min.is_absolute())
				limits.pxPrefUsed += cnst.axis[m_orientation].min.to_px();
		}
		
		tmp = cnst.axis[other_axis].ImplicitMinSize();
		if (isnan(altMin) || (!isnan(tmp) && (tmp > altMin))) altMin = tmp;

		if (m_squeeze) { // squeeze
			tmp = cnst.axis[other_axis].ImplicitMaxSize();
			if (isnan(altMax) || (!isnan(tmp) && (tmp < altMax))) altMax = tmp;
		}

		tmp = cnst.axis[other_axis].ImplicitPrefSize();
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

//	printf("hvgroup constrain %f,%f,%f\n",min,max,pref);

	m_internal_cnst.axis[m_orientation].min = dimth(min, dimth::pixels);
	m_internal_cnst.axis[m_orientation].pref = dimth(pref, dimth::pixels);
	m_internal_cnst.axis[m_orientation].max = dimth(max, dimth::pixels);

	m_internal_cnst.axis[other_axis].min = dimth(altMin, dimth::pixels);
	m_internal_cnst.axis[other_axis].pref = dimth(altPref, dimth::pixels);
	m_internal_cnst.axis[other_axis].max = dimth(altMax, dimth::pixels);

	return m_internal_cnst;
}

void 
BViewHVGroup::Layout()
{
	float tmp, other_axis_size, accum = 0;
	const BRect bounds = Shape().Bounds();
	coord BRect::*axis_min;
	coord BRect::*axis_max;
	coord BRect::*other_min;
	coord BRect::*other_max;
	int32 count = _Count();
	int32 total = 0, inc, need, parts[count]; // gcc-ism
	BLayoutConstraints constraints[count];
	orientation other_axis;

//	printf("hvgroup: "); bounds.PrintToStream();

	if (m_orientation == B_VERTICAL) {
		axis_min  = &BRect::top;
		axis_max  = &BRect::bottom;
		other_min = &BRect::left;
		other_max = &BRect::right;
		other_axis = B_HORIZONTAL;
	} else {
		axis_min  = &BRect::left;
		axis_max  = &BRect::right;
		other_min = &BRect::top;
		other_max = &BRect::bottom;
		other_axis = B_VERTICAL;
	}
	need = static_cast<int32>(ceil(bounds.*axis_max - bounds.*axis_min));
	other_axis_size = ceil(bounds.*other_max - bounds.*other_min);
	
	for (int32 i = 0; i < count; i++) {
		constraints[i] = _ChildAt(i)->Constraints();
		if (!(constraints[i].axis[m_orientation].pref.is_undefined())) {
			tmp = constraints[i].axis[m_orientation].Pref(need);
		}
		else if (!(constraints[i].axis[m_orientation].min.is_undefined())) {
			tmp = constraints[i].axis[m_orientation].Min(need);
		}
		else if (!(constraints[i].axis[m_orientation].max.is_undefined())) {
			tmp = constraints[i].axis[m_orientation].Max(need);
		}
		else {
			tmp = 0;
		}
		accum += tmp;
		parts[i] = (int32)(accum - total);
		total += parts[i];
	}

//	printf("(%ld/%ld)\n",need,total);

	if (total < need) {
		/* There is extra space to fill. */
		int32 room = need - total;
		int32 unlimited_count = 0;
		int32 unlimited_extra;
		int32 sum_extra = 0;
		int32 extra_max = 0;
		
		for (int32 i = 0; i < count; i++) {
			tmp = constraints[i].axis[m_orientation].Max(need);
//			printf("max[%ld] = %f ",i,tmp);
//			constraints[i].axis[m_orientation].PrintToStream();
//			printf("\n");
			if (isnan(tmp)) {
				tmp = 0;
			}
			
			if (isinf(tmp)) {
				++unlimited_count;
			}
			else {
				int32 extra = static_cast<int32>(tmp) - parts[i];
				if (extra > extra_max) {
					extra_max = extra;
				}
				sum_extra += extra;
			}
		}
		
		if (unlimited_count != 0) {
			unlimited_extra = MAX((room - sum_extra + unlimited_count - 1) / unlimited_count,
			                      extra_max);
		}
		
		int32 total_extra = sum_extra + unlimited_count*unlimited_extra;
		if (room > total_extra) {
			// Prevent growing Views past their maximum sizes:
			room = total_extra;
		}
		
		for (int32 i = 0; i < count; i++) {
			if (total_extra <= 0)
				break;
				
			tmp = constraints[i].axis[m_orientation].Max(need);
			
			int32 extra;
			if (isinf(tmp)) {
				extra = unlimited_extra;
			}
			else {
				extra = static_cast<int32>(tmp) - parts[i];
			}
			
			inc = (extra * room) / total_extra;
			parts[i] += inc;
			room -= inc;
			total_extra -= extra;
		}
	} else if (total > need) {
		/* There is not enough space to give everyone their preferred size.
		   Scale it back, using the amount of shrinking-room we have to
		   play with as a weight.  */
		int32 dearth = total - need;
		int32 sum_room = 0;
		
		for (int32 i = 0; i < count; i++) {
			tmp = constraints[i].axis[m_orientation].Min(need);
			if (isnan(tmp)) {
				tmp = 0;
			}
			
			sum_room += parts[i] - static_cast<int32>(tmp);
		}
		
		if (dearth > sum_room) {
			// Prevent shrinking Views past their minimum sizes:
			dearth = sum_room;
		}
		
		for (int32 i = 0; i < count; i++) {
			if (sum_room <= 0)
				break;
	
			tmp = constraints[i].axis[m_orientation].Min(need);
			
			int32 room = parts[i] - static_cast<int32>(tmp);
			
			inc = (room * dearth) / sum_room;
			parts[i] -= inc;
			dearth -= inc;
			sum_room -= room;
		}
	}		

	BRect childBounds;
	childBounds.*axis_min = 0;
	for (int32 i = 0; i < count; i++) {
//		printf("parts[%ld]=%ld\n",i,parts[i]);
		IView::ptr child = _ChildAt(i);
		float other_size;
		
		childBounds.*axis_max = childBounds.*axis_min + parts[i];
		constraints[i].axis[other_axis].Resolve(other_axis_size, childBounds.*other_min, other_size);
		childBounds.*other_max = childBounds.*other_min + other_size;
		child->SetLayoutBounds(childBounds);
		
		childBounds.*axis_min = childBounds.*axis_max;
	}
}

} }	// namespace B::Interface2
