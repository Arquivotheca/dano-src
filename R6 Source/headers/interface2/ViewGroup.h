
#ifndef _INTERFACE2_VIEWGROUP_H_
#define _INTERFACE2_VIEWGROUP_H_

#include <interface2/ViewLayout.h>

namespace B {
namespace Render2 {
	class BUpdate;
}}

namespace B {
namespace Interface2 {

/**************************************************************************************/

class BViewGroup : public BViewLayout
{
	public:

										BViewGroup(const BValue &attr = BValue::undefined);
										BViewGroup(const BViewGroup &copyFrom);
		virtual							~BViewGroup();

		virtual	BView *					Copy();

		virtual	BLayoutConstraints		InternalConstraints() const;
		virtual	void					Layout();

	private:
	
		mutable BLayoutConstraints		m_internal_cnst;
				uint32					m_squeeze:2;
};

/**************************************************************************************/

} } // namespace B::Interface2

#endif	/* _INTERFACE2_VIEWGROUP_H_ */
