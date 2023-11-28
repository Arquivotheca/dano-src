
#ifndef _INTERFACE2_LAYOUTCONSTRAINTS_H_
#define _INTERFACE2_LAYOUTCONSTRAINTS_H_

#include <render2/Point.h>
#include <render2/Rect.h>
#include <support2/Value.h>
#include <interface2/InterfaceDefs.h>

namespace B {
namespace Interface2 {

using namespace Support2;

struct BLayoutConstraint
{
	enum {
		posIsRelativeToStart = 0x0001
	};

	uint32	flags;
	dimth	pos;
	dimth	pref;
	dimth	min,max;
	BValue	hints;
	
	BLayoutConstraint() : flags(posIsRelativeToStart) {};
	BLayoutConstraint(const BValue& o);

	BValue AsValue() const;
	inline operator BValue() const { return AsValue(); }
	
	void Clear() {
		flags = posIsRelativeToStart;
		min = max = pos = pref = dimth::undefined;
	}

	bool operator ==(const BLayoutConstraint &o) const {
		return
			(flags == o.flags) &&
			(pos == o.pos) &&
			(pref == o.pref) &
			(min == o.min) &&
			(max == o.max);
	}

	bool operator !=(const BLayoutConstraint &o) const {
		return
			(flags != o.flags) ||
			(pos != o.pos) ||
			(pref != o.pref) ||
			(min != o.min) ||
			(max != o.max);
	}

	BLayoutConstraint operator <<(const BLayoutConstraint &o) const {
		BLayoutConstraint r;

		if (o.pos != dimth::undefined) {
			r.pos = o.pos;
			r.flags = o.flags;
		} else {
			r.pos = pos;
			r.flags = flags;
		}

		if (o.pref != dimth::undefined)	r.pref = o.pref;
		else							r.pref = pref;

		if (o.min != dimth::undefined)	r.min = o.min;
		else							r.min = min;

		if (o.max != dimth::undefined)	r.max = o.max;
		else							r.max = max;

		return r;
	}

	BLayoutConstraint& operator <<=(const BLayoutConstraint &o) {

		if (o.pos != dimth::undefined) {
			pos = o.pos;
			flags = o.flags;
		}

		if (o.pref != dimth::undefined)	pref = o.pref;
		if (o.min != dimth::undefined)	min = o.min;
		if (o.max != dimth::undefined)	max = o.max;
		
		return *this;
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

struct BLayoutConstraints
{
	BLayoutConstraint axis[2];

	void Clear() {
		axis[B_HORIZONTAL].Clear();
		axis[B_VERTICAL].Clear();
	}

	bool operator ==(const BLayoutConstraints &o) const {
		return
			(axis[0] == o.axis[0]) &&
			(axis[1] == o.axis[1]) ;
	}

	bool operator !=(const BLayoutConstraints &o) const {
		return
			(axis[0] != o.axis[0]) ||
			(axis[1] != o.axis[1]) ;
	}

	BLayoutConstraints operator <<(const BLayoutConstraints &o) const {
		BLayoutConstraints r;
		r.axis[0] = axis[0] << o.axis[0];
		r.axis[1] = axis[1] << o.axis[1];
		return r;
	}

	BLayoutConstraints& operator <<=(const BLayoutConstraints &o) {
		axis[0] <<= o.axis[0];
		axis[1] <<= o.axis[1];
		return *this;
	}

	void PrintToStream() {
		printf("width:  "); axis[B_HORIZONTAL].PrintToStream(); printf("\n");
		printf("height: "); axis[B_VERTICAL].PrintToStream(); printf("\n");
	}

	void SetLeft(dimth left) {
		axis[B_HORIZONTAL].SetStart(left);
	}

	void SetRight(dimth right) {
		axis[B_HORIZONTAL].SetStop(right);
	}

	void SetTop(dimth top) {
		axis[B_VERTICAL].SetStart(top);
	}

	void SetBottom(dimth bottom) {
		axis[B_VERTICAL].SetStop(bottom);
	}

	void SetWidth(dimth width, bool hard=true) {
		axis[B_HORIZONTAL].SetSize(width,hard);
	}

	void SetHeight(dimth height, bool hard=true) {
		axis[B_VERTICAL].SetSize(height,hard);
	}

	BPoint PrefSize(BPoint size) {
		return BPoint(axis[B_HORIZONTAL].Pref(size.x),axis[B_VERTICAL].Pref(size.y));
	}

	BPoint MinSize(BPoint size) {
		return BPoint(axis[B_HORIZONTAL].Min(size.x),axis[B_VERTICAL].Min(size.y));
	}

	BPoint MaxSize(BPoint size) {
		return BPoint(axis[B_HORIZONTAL].Max(size.x),axis[B_VERTICAL].Max(size.y));
	}

	BRect Resolve(BPoint size) {
		BRect r;
		axis[B_HORIZONTAL].Resolve(size.x, r.left, r.right);
		axis[B_VERTICAL].Resolve(size.y, r.top, r.bottom);
		r.right += r.left;
		r.bottom += r.top;
		return r;
	}
};

} } // namespace B::Interface2

#endif
