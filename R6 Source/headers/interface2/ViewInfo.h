
#ifndef _INTERFACE2_VIEWINFO_H_
#define _INTERFACE2_VIEWINFO_H_

#include <interface2/InterfaceDefs.h>

namespace B {
namespace Interface2 {

/**************************************************************************************/

class BViewInfo : public BAtom
{
	public:

		view				child;
		BString				name;
		uint32				flags;
		BLayoutConstraints	constraints;
};

/**************************************************************************************/

} } // namespace B::Interface2

#endif	/* _INTERFACE2_VIEWINFO_H_ */
