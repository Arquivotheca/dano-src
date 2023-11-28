#ifndef _SUPPORT2_IROOT_H_
#define _SUPPORT2_IROOT_H_

#include <support2/IBinder.h>
#include <support2/IInterface.h>

namespace B {
namespace Support2 {


class IRoot : public IInterface
{
public:
	B_DECLARE_META_INTERFACE(Root)
		
	virtual IBinder::ptr	LoadObject(BString pathname, const BValue & params) = 0;
};


} } // namespace B::Support2

#endif	/* _SUPPORT2_ROOT_H_ */
