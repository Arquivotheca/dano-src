
#ifndef	_INTERFACE2_SURFACEMANAGER_H_
#define	_INTERFACE2_SURFACEMANAGER_H_

#include <support2/Atom.h>
#include <support2/IInterface.h>
#include <support2/Binder.h>
#include <raster2/RasterDefs.h>
#include <raster2/GraphicsDefs.h>
#include <render2/RenderDefs.h>
#include <interface2/InterfaceDefs.h>
#include <interface2/Surface.h>

namespace B {
namespace Interface2 {

class ISurfaceManager : public IInterface
{
	public:

		B_DECLARE_META_INTERFACE(SurfaceManager)

		virtual	ISurface::ptr		RootSurface() = 0;
		virtual	ISurface::ptr		CreateSurface(int32 width, int32 height, color_space format) = 0;
};

class LSurfaceManager : public LInterface<ISurfaceManager>
{
	public:
		virtual	status_t			Told(value &in);
		virtual	status_t			Asked(const value &outBindings, value &out);
};

} }	// namespace B::Interface2

#endif 	/* _INTERFACE2_SURFACEMANAGER_H_ */
