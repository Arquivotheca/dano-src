
#include "../render2/Static.cpp"
#include <interface2/Surface.h>
#include <interface2/SurfaceManager.h>
#include <interface2/View.h>
#include <interface2/ViewParent.h>

namespace B {
namespace Interface2 {

const BValue ISurface::descriptor(BValue::TypeInfo(typeid(ISurface)));
const BValue ISurfaceManager::descriptor(BValue::TypeInfo(typeid(ISurfaceManager)));
const BValue IView::descriptor(BValue::TypeInfo(typeid(IView)));
const BValue IViewParent::descriptor(BValue::TypeInfo(typeid(IViewParent)));

} }	// namespace B::Interface2
