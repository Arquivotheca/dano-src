
#include "../support2/Static.cpp"
#include <render2/Color.h>
#include <render2/Point.h>
#include <render2/Rect.h>
#include <render2/Region.h>
#include <render2/Render.h>
#include <render2/2dTransform.h>

namespace B {

namespace Private {

const BValue g_keyIsOpaque("opaque");
const BValue g_keyDoesClipParent("clip");
const BValue g_keyBranch("Branch");
const BValue g_keyDraw("Draw");
const BValue g_keyDisplay("Display");
const BValue g_keyBounds("Bounds");
const BValue g_keyInvalidate("Invalidate");

}

namespace Render2 {

const BValue IRender::descriptor(BValue::TypeInfo(typeid(IRender)));
const BValue IVisual::descriptor(BValue::TypeInfo(typeid(IVisual)));

class print_register
{
public:
	print_register()
	{
		value_ref::register_print_func(B_COLOR_TYPE, BColor::printer);
		value_ref::register_print_func(B_COLOR_32_TYPE, BColor32::printer);
		value_ref::register_print_func(B_POINT_TYPE, BPoint::printer);
		value_ref::register_print_func(B_RECT_TYPE, BRect::printer);
		value_ref::register_print_func(B_TRANSFORM_2D_TYPE, B2dTransform::printer);
		value_ref::register_print_func(B_REGION_TYPE, BRegion::printer);
	}
};

static print_register register_em;

} }	// namespace B::Render2
