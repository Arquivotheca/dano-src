
#include <interface2/ViewParent.h>
#include <interface2/View.h>
#include <render2/Update.h>
#include <support2/StdIO.h>

namespace B {
namespace Interface2 {

static const BValue g_keyInvalidate("b:inval");
static const BValue g_keyMarkTraversalPath("b:mtp");
static const BValue g_keyChild("b:child");
static const BValue g_keyUpdate("b:upd");

/**************************************************************************************/

class RViewParent : public RInterface<IViewParent>
{
	public:

									RViewParent(IBinder::arg remote) : RInterface<IViewParent>(remote) {};

		virtual	status_t			ConstrainChild(	const IView::ptr &child,
													const BLayoutConstraints &constraints);
		virtual	status_t			InvalidateChild(const IView::ptr &child,
													const BUpdate &update);
		virtual	void				MarkTraversalPath(int32 type);
};

status_t 
RViewParent::ConstrainChild(const IView::ptr &child, const BLayoutConstraints &constraints)
{
#warning need implementation
	return B_UNSUPPORTED;
}

status_t 
RViewParent::InvalidateChild(const IView::ptr &child, const BUpdate &update)
{
	Remote()->Put(BValue(g_keyInvalidate,
		BValue(g_keyChild, child->AsBinder())
		.Overlay(g_keyUpdate, update.AsValue()) ));
	return B_OK;
}

void 
RViewParent::MarkTraversalPath(int32 type)
{
	Remote()->Put(BValue(g_keyMarkTraversalPath, BValue::Int32(type)));
}

/**************************************************************************************/

B_IMPLEMENT_META_INTERFACE(ViewParent)

/**************************************************************************************/

status_t 
LViewParent::Told(BValue &in)
{
	#warning Method needs implementation!
	BValue val = in[g_keyInvalidate];
	if (val) {
		IView::ptr child(IView::AsInterface(val[g_keyChild]));
		InvalidateChild(child, BUpdate(val[g_keyUpdate]));
	}
	val = in[g_keyMarkTraversalPath];
	if (val) {
		MarkTraversalPath(val.AsInteger());
	}
	return B_OK;
}

} }	// namespace B::Interface2
