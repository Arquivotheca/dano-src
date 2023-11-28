
#ifndef _INTERFACE2_VIEWEMBEDDEDROOT_H_
#define _INTERFACE2_VIEWEMBEDDEDROOT_H_

#include <Locker.h>
#include <Region.h>
#include <ViewRoot.h>

namespace B {
namespace Interface2 {

class BViewEmbeddedRoot : public BViewRoot
{
	public:
										BViewEmbeddedRoot();
										BViewEmbeddedRoot(const BMessage &attributes);
		virtual							~BViewEmbeddedRoot();

		virtual	void					Clean(layout_t layout, BRegion &outWasDirty);
		virtual	void					MarkDirty(const BRegion &dirty);

	private:
	
		BRegion							m_dirtyRegion;
		BLocker							m_dirtyRegionLock;
};

} } // namespace B::Interface2

using namespace B::Interface2;

#endif	/* _INTERFACE2_VIEWEMBEDDEDROOT_H_ */
