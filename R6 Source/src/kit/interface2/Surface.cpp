
#include <interface2/Surface.h>

namespace B {
namespace Interface2 {

static const BValue g_keyhost("b:host");

class RSurface : public RInterface<ISurface>
{
	public:
									RSurface(IBinder::arg remote) : RInterface<ISurface>(remote) {};	
		virtual	status_t			SetHost(const IView::ptr& hostView);
		virtual	IViewParent::ptr	ViewParent();
};

status_t 
RSurface::SetHost(const IView::ptr& hostView)
{
	Remote()->Put(BValue(g_keyhost, BValue::Binder(hostView->AsBinder())));
}

IViewParent::ptr 
RSurface::ViewParent()
{
	return IViewParent::AsInterface(Remote());
}

B_IMPLEMENT_META_INTERFACE(Surface)

status_t 
LSurface::Told(BValue &in)
{
	BValue host = in[g_keyhost];
	if (host) {
		IView::ptr p = IView::AsInterface(host);
		if (p != NULL) SetHost(p);
	}
}

status_t 
LSurface::Asked(const BValue &outBindings, BValue &out)
{
	out += (outBindings *
			(	BValue(descriptor, BValue::Binder(this))
				.Overlay(BValue(IViewParent::descriptor, ViewParent()->AsBinder())) ) );
}

} }	// namespace B::Interface2
