
#ifndef	_INTERFACE2_SURFACE_H_
#define	_INTERFACE2_SURFACE_H_

#include <support2/Atom.h>
#include <support2/IInterface.h>
#include <support2/Binder.h>
#include <raster2/RasterDefs.h>
#include <render2/RenderDefs.h>
#include <interface2/InterfaceDefs.h>
#include <interface2/View.h>
#include <interface2/ViewParent.h>

namespace B {
namespace Interface2 {

class ISurface : public IInterface
{
	public:

		B_DECLARE_META_INTERFACE(Surface)

		virtual	status_t			SetHost(const IView::ptr& hostView) = 0;
		virtual	IViewParent::ptr	ViewParent() = 0;
};

class LSurface : public LInterface<ISurface>
{
	public:
		virtual	status_t			Told(BValue &in);
		virtual	status_t			Asked(const BValue &outBindings, BValue &out);
};

} }	// namespace B::Interface2

#endif 	/* _INTERFACE2_SURFACE_H_ */
