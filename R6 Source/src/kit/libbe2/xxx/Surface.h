
#ifndef	_RENDER2_SURFACE_H_
#define	_RENDER2_SURFACE_H_

#include <Atom.h>
#include <RenderDefs.h>

namespace B {
namespace Render2 {

class BSurface : virtual public BAtom {

	public:

										BSurface();
		virtual							~BSurface();

		virtual	BRect					Bounds();

		virtual	status_t				GetParent(surface_ptr &parent);
		virtual	status_t				SetParent(const surface_ptr &parent);

		virtual	status_t				MoveTo(BPoint upperLeft);
		virtual	status_t				ResizeTo(BPoint dimensions);

		virtual	status_t				StartHosting(BHandler *handler, hostedsurface_ptr &host);
};

} } // namespace B::Render2

using namespace B::Render2;

#endif /* _RASTER2_SURFACE_H_ */
