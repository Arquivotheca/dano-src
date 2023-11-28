
#include <render2/Update.h>
#include <interface2/View.h>
#include <interface2/ViewGroup.h>

namespace B {
namespace Interface2 {

BViewGroup::BViewGroup(const BValue &attr) : BViewLayout(attr), m_squeeze(0)
{
	BString s;

	if ((attr.ValueFor("squeeze").GetString(&s) == B_OK) && (s == "true"))	m_squeeze |= 3;
	if ((attr.ValueFor("squeezew").GetString(&s) == B_OK) && (s == "true"))	m_squeeze |= 1;
	if ((attr.ValueFor("squeezeh").GetString(&s) == B_OK) && (s == "true"))	m_squeeze |= 2;
}

BViewGroup::BViewGroup(const BViewGroup &copyFrom)
	:	BViewLayout(copyFrom),
		m_internal_cnst(copyFrom.m_internal_cnst),
		m_squeeze(copyFrom.m_squeeze)
{
}

BView * 
BViewGroup::Copy()
{
	return new BViewGroup(*this);
}

BViewGroup::~BViewGroup()
{
}

BLayoutConstraints 
BViewGroup::InternalConstraints() const
{
	m_internal_cnst = BLayoutConstraints();
	int32 count = _Count();
	for (int32 i = 0; i < count; i++) {
		BLayoutConstraints cnst = _ChildAt(i)->Constraints();
		for (int32 hv = 0; hv < 2; hv++) {
	
			coord min_i = cnst.axis[hv].ImplicitMinSize();
			if (  !isnan(min_i)
			    && (   m_internal_cnst.axis[hv].min.is_undefined()
			        || m_internal_cnst.axis[hv].ImplicitMinSize() < min_i))
			{
				m_internal_cnst.axis[hv].min = dimth(min_i, dimth::pixels);
			}			
			
			coord pref_i = cnst.axis[hv].ImplicitPrefSize();
			if (  !isnan(pref_i)
			    && (   m_internal_cnst.axis[hv].pref.is_undefined()
			        || m_internal_cnst.axis[hv].ImplicitPrefSize() < pref_i))
			{
				m_internal_cnst.axis[hv].pref = dimth(pref_i, dimth::pixels);
			}			
			
			if (m_squeeze & (1<<hv)) { // squeeze
				coord max_i = cnst.axis[hv].ImplicitMaxSize();
				if (  !isnan(max_i)
				    && (   m_internal_cnst.axis[hv].max.is_undefined()
				        || m_internal_cnst.axis[hv].ImplicitMaxSize() < max_i))
				{
					m_internal_cnst.axis[hv].max = dimth(max_i, dimth::pixels);
				}			
			}
		}
	}
	return m_internal_cnst;
}

void 
BViewGroup::Layout()
{
	const BRect bounds = Shape().Bounds();
	const BPoint size(bounds.Width(), bounds.Height());
	const int32 count = _Count();
	for (int32 i = 0; i < count; i++) {
		IView::ptr child = _ChildAt(i);
		const BRect child_bounds = child->Constraints().Resolve(size);
		child->SetLayoutBounds(child_bounds);
	}
}

} }	// namespace B::Interface2
