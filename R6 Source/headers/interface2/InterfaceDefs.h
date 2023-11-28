
#ifndef _INTERFACE2_DEFS_H_
#define _INTERFACE2_DEFS_H_

#include <support2/Atom.h>
#include <stdio.h>
#include <math.h>

#define DEBUG_LAYOUT 1

namespace B {
namespace Raster2 { }
namespace Render2 { }
namespace Interface2 {

using namespace Support2;
using namespace Raster2;
using namespace Render2;

class IView;
class LView;
class IViewParent;
class LViewParent;
class BViewInfo;
class IViewLayout;
class BLayoutConstraints;

class BWindow;

typedef IBinderVector<IView> IViewList;
typedef LBinderVector<IView> LViewList;
typedef BBinderVector<IView> BViewList;

typedef atom_ptr<IView>			view;
typedef atom_ref<IView>			view_ref;
typedef atom_ptr<IViewList>		viewlist;
typedef atom_ref<IViewList>		viewlist_ref;
typedef atom_ref<IViewParent>	viewparent_ref;
typedef atom_ptr<IViewParent>	viewparent;
typedef atom_ptr<IViewLayout>	viewlayout;
typedef atom_ptr<BViewInfo>		viewinfo;

enum {
	B_ENTERED_VIEW = 0,
	B_INSIDE_VIEW,
	B_EXITED_VIEW,
	B_OUTSIDE_VIEW
};

enum {
	MIN_PARAM = 0,
	MAX_PARAM,
	PARAM_COUNT
};

enum orientation {
	B_HORIZONTAL,
	B_VERTICAL
};

enum event_dispatch_result {
	B_EVENT_ABSORBED = 0,
	B_EVENT_FALLTHROUGH
};

struct device {
	static device screen;

	float dpi;
};

struct dimth {
	static dimth undefined;

	enum unit {
		unset=-1,
		nil=0,
		pixels,
		points,
		centimeters,
		percent,
		norms
	} units;
	float value;
	
	dimth() : units(nil), value(0.0) {};
	dimth(float v, unit u=pixels) : units(u), value(v) {};

	void PrintToStream();
	
	bool is_undefined() const {
		return (units == nil);
	}

	bool is_absolute() const {
		return (units < percent) && (units != nil);
	}

	bool is_relative() const {
		return (units >= percent) && (units != nil);
	}
	
	bool operator ==(const dimth &o) const {
		return (units == o.units) && ((units == nil) || (value == o.value));
	}

	bool operator !=(const dimth &o) const {
		return (units != o.units) || ((value != o.value) && (units != nil));
	}

	float to_nm() const {
		if (units == norms) return value;
		if (units == percent) return value / 100.0;
		return NAN;
	}

	float to_px(device &d=device::screen) const {
		if (units == pixels) return value;
		if (units == points) return value * d.dpi / 72.0;
		if (units == centimeters) return value * d.dpi / 2.54;
		return NAN;
	}

	float to_px(float size, device &d=device::screen) const {
		if (is_absolute()) return to_px(d);
		else if (is_relative()) return to_nm() * size;
		return NAN;
	}
};

} } // namespace B::Interface2

#endif
