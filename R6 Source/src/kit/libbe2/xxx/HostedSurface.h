
#ifndef	_RENDER2_HOSTEDSURFACE_H_
#define	_RENDER2_HOSTEDSURFACE_H_

#include <Atom.h>
#include <RasterDefs.h>
#include <RenderDefs.h>

namespace B {
namespace Render2 {

class BHostedSurface : virtual public BAtom {

	public:

							BHostedSurface();
		virtual				~BHostedSurface();

		virtual	BRect		Bounds();
		virtual	status_t	Invalidate(const BRegion &dirty);
		virtual	status_t	Update(render &renderInto);
};

} } // namespace B::Render2

using namespace B::Render2;

#endif /* _RENDER2_HOSTEDSURFACE_H_ */
