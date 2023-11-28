
#ifndef	_INTERFACE2_WIMPSERVER_H_
#define	_INTERFACE2_WIMPSERVER_H_

#include <support2/Atom.h>
#include <raster2/RasterDefs.h>
#include <render2/RenderDefs.h>
#include <interface2/InterfaceDefs.h>

namespace B {
namespace Interface2 {

class IWimpServer : virtual public BAtom
{
	public:

		static	atom_ptr<IWimpServer>	Default();

		virtual	viewlist				RootWindow() = 0;
		virtual	BBitmap *				CreateBitmap(int32 width, int32 height, pixel_format format) = 0;
};

} }	// namespace B::Interface2

#endif 	/* _INTERFACE2_WIMPSERVER_H_ */
