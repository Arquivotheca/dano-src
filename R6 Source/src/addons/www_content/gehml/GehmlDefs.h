
#ifndef _GEHMLDEFS_H_
#define _GEHMLDEFS_H_

#include <stdio.h>
#include <InterfaceDefs.h>
#include <Atom.h>

#define DEBUG_LAYOUT 1

enum {
	nfDraw			= 0x0001,
	nfConstrain		= 0x0002,
	nfPosition		= 0x0004,
	nfLayout		= 0x0008,
	nfDescend		= 0x0010,
	nfAddChild		= 0x0020,
	nfRemoveChild	= 0x0040
};

enum {
	HORIZONTAL=0,
	VERTICAL,
	ORIENTATION_COUNT
};

enum {
	MIN_PARAM = 0,
	MAX_PARAM,
	PARAM_COUNT
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
	
	bool is_undefined() {
		return (units == nil);
	}

	bool is_absolute() {
		return (units < percent) && (units != nil);
	}

	bool is_relative() {
		return (units >= percent) && (units != nil);
	}
	
	bool operator ==(const dimth &o) {
		return (units == o.units) && ((units == nil) || (value == o.value));
	}

	bool operator !=(const dimth &o) {
		return (units != o.units) || ((value != o.value) && (units != nil));
	}

	float to_nm() {
		if (units == norms) return value;
		if (units == percent) return value / 100.0;
		return NAN;
	}

	float to_px(device &d=device::screen) {
		if (units == pixels) return value;
		if (units == points) return value * d.dpi / 72.0;
		if (units == centimeters) return value * d.dpi / 2.2;
		return NAN;
	}

	float to_px(float size, device &d=device::screen) {
		if (is_absolute()) return to_px(d);
		else if (is_relative()) return to_nm() * size;
		return NAN;
	}
};

struct GehmlConstraint
{
	enum {
		posIsRelativeToStart = 0x0001
	};

	uint8	flags;
	dimth	pos;
	dimth	pref;
	dimth	min,max;

	GehmlConstraint() : flags(posIsRelativeToStart) {};

	void Clear() {
		flags = posIsRelativeToStart;
		min = max = pos = pref = dimth::undefined;
	}

	void PrintToStream();

	void SetStart(dimth start) {
		flags |= posIsRelativeToStart;
		pos = start;
	}

	void SetStop(dimth stop) {
		flags &= ~posIsRelativeToStart;
		pos = stop;
	}

	void SetSize(dimth size, bool hard=true) {
		pref = size;
		if (hard) min = max = size;
	}

	float Min(float size) {
		float r = min.to_px(size);
		if (isnan(r)) r = 0;
		return r;
	}

	float Max(float size) {
		float r = max.to_px(size);
		if (isnan(r)) r = INFINITY;
		return r;
	}

	float Pref(float size) {
		float r = pref.to_px(size);
		if (isnan(r)) r = size;
		return r;
	}

	float ImplicitSize(dimth param) {
		float tmp;
	
		if (param.is_absolute()) {
			tmp = param.to_px();
			if (pos.is_absolute())
				return tmp + pos.to_px();
			else if (pos.is_relative()) 
				return tmp / (1.0-pos.to_nm());
			return tmp;
		} else if (param.is_relative() && pos.is_absolute())
			return pos.to_px() / (1.0-param.to_nm());

		return NAN;
	}

	float ImplicitMinSize() {
		return ImplicitSize(min);
	}

	float ImplicitMaxSize() {
		if (!(flags & posIsRelativeToStart)) return NAN;
		return ImplicitSize(max);
	}

	float ImplicitPrefSize() {
		return ImplicitSize(pref);
	}

	void Resolve(float size, float &outPosition, float &outSize) {
		float min = Min(size);
		float max = Max(size);
		float r = Pref(size);
		if (r > max) {
//			printf("conflict between pref(%f) and max(%f); max wins\n",r,max);
			r = max;
			if (r < min) {
//				printf("conflict between max(%f) and min(%f), min wins\n",r,min);
				r = min;
			}
		} else if (r < min) {
//			printf("conflict between pref(%f) and min(%f); min wins\n",r,min);
			r = min;
			if (r > max) {
//				printf("conflict between max(%f) and min(%f), min wins\n",max,r);
			}
		}

		outSize = r;
		r = pos.to_px(size);
		if (isnan(r)) r = 0;
		outPosition = (flags & posIsRelativeToStart) ? r : (size - r - outSize);
	}
};

struct GehmlConstraints
{
	GehmlConstraint axis[2];

	void Clear() {
		axis[HORIZONTAL].Clear();
		axis[VERTICAL].Clear();
	}

	void PrintToStream() {
		printf("width:  "); axis[HORIZONTAL].PrintToStream(); printf("\n");
		printf("height: "); axis[VERTICAL].PrintToStream(); printf("\n");
	}

	void SetLeft(dimth left) {
		axis[HORIZONTAL].SetStart(left);
	}

	void SetRight(dimth right) {
		axis[HORIZONTAL].SetStop(right);
	}

	void SetTop(dimth top) {
		axis[VERTICAL].SetStart(top);
	}

	void SetBottom(dimth bottom) {
		axis[VERTICAL].SetStop(bottom);
	}

	void SetWidth(dimth width, bool hard=true) {
		axis[HORIZONTAL].SetSize(width,hard);
	}

	void SetHeight(dimth height, bool hard=true) {
		axis[VERTICAL].SetSize(height,hard);
	}

	BPoint PrefSize(BPoint size) {
		return BPoint(axis[HORIZONTAL].Pref(size.x),axis[VERTICAL].Pref(size.y));
	}

	BPoint MinSize(BPoint size) {
		return BPoint(axis[HORIZONTAL].Min(size.x),axis[VERTICAL].Min(size.y));
	}

	BPoint MaxSize(BPoint size) {
		return BPoint(axis[HORIZONTAL].Max(size.x),axis[VERTICAL].Max(size.y));
	}

	BRect Resolve(BPoint size) {
		BRect r;
		axis[HORIZONTAL].Resolve(size.x, r.left, r.right);
		axis[VERTICAL].Resolve(size.y, r.top, r.bottom);
		r.right += r.left-1;
		r.bottom += r.top-1;
		return r;
	}
};

typedef atom<class GehmlObject> gehml_obj;
typedef atom<class GehmlObject> gehml_ref;
typedef atom<class GehmlGroup> gehml_group;
typedef atomref<class GehmlGroup> gehml_group_ref;
typedef atom<class GehmlLayoutTop> gehml_layout;

//namespace BPrivate {
//	class AppSession;
	class _BSession_;
//}

class BDrawable;

#define checkpoint printf("thid=%ld (%08lx) -- %s:%d -- %s\n",(int32)find_thread(NULL),(uint32)this,__FILE__,__LINE__,__PRETTY_FUNCTION__);

#endif
