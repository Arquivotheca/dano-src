
#ifndef _INTERFACE2_VIEWHVGROUP_H_
#define _INTERFACE2_VIEWHVGROUP_H_

#include <interface2/ViewLayout.h>

namespace B {
namespace Render2 {
	class BUpdate;
}}

namespace B {
namespace Interface2 {

/**************************************************************************************/

class BViewHVGroup : public BViewLayout
{
	public:

		/*
			Effective arguments for the BViewHVGroup(BValue) constructor (keys in
			attr whose values we pay attention to):
			
			"orientation"	:	orientation enum as an int32.  Should be either
			                     B_HORIZONTAL (default) or B_VERTICAL.
		*/
										BViewHVGroup(const BValue &attr = BValue::undefined);
										BViewHVGroup(const BViewHVGroup &copyFrom);
		virtual							~BViewHVGroup();

		virtual	BView *					Copy();

		virtual	BLayoutConstraints		InternalConstraints() const;
		virtual	void					Layout();

	private:
	
		mutable BLayoutConstraints		m_internal_cnst;
				orientation				m_orientation;
				bool					m_squeeze;
};

/**************************************************************************************/

} } // namespace B::Interface2

#endif	/* _INTERFACE2_VIEWHVGROUP_H_ */
