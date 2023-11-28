#ifndef _RGBA_COLOR_H
#define _RGBA_COLOR_H

class RGBAColor {
 public:
	float  red;
	float  green;
	float  blue;
	float  alpha;
	inline void Set(float r, float g, float b, float a);
	inline RGBAColor operator* (const RGBAColor& c) const;
	inline void operator*= (const RGBAColor& c);
	inline RGBAColor operator* (const float f) const;
	inline void operator*= (const float f);
	inline RGBAColor operator+ (const RGBAColor& c) const;
	inline void operator+= (const RGBAColor& c);
	inline RGBAColor operator- (const RGBAColor& c) const;
	inline void operator-= (const RGBAColor& c);
	inline RGBAColor operator^ (const RGBAColor& c) const;
	inline void operator^= (const RGBAColor& c);
};

void RGBAColor::Set(float r, float g, float b, float a) {
	red = r;
	green = g;
	blue = b;
	alpha = a;
}

RGBAColor RGBAColor::operator* (const RGBAColor& c) const {
	RGBAColor    c2;

	c2.red = red*c.red;
	c2.green = green*c.green;
	c2.blue = blue*c.blue;
	c2.alpha = alpha*c.alpha;
	return c2;
}

void RGBAColor::operator*= (const RGBAColor& c) {
	red *= c.red;
	green *= c.green;
	blue *= c.blue;
	alpha *= c.alpha;
}

RGBAColor RGBAColor::operator* (const float f) const {
	RGBAColor    c2;

	c2.red = red*f;
	c2.green = green*f;
	c2.blue = blue*f;
	c2.alpha = alpha*f;
	return c2;
}

void RGBAColor::operator*= (const float f) {
	red *= f;
	green *= f;
	blue *= f;
	alpha *= f;
}

RGBAColor RGBAColor::operator+ (const RGBAColor& c) const {
	RGBAColor    c2;

	c2.red = red+c.red;
	c2.green = green+c.green;
	c2.blue = blue+c.blue;
	c2.alpha = alpha+c.alpha;
	return c2;
}

void RGBAColor::operator+= (const RGBAColor& c) {
	red += c.red;
	green += c.green;
	blue += c.blue;
	alpha += c.alpha;
}

RGBAColor RGBAColor::operator- (const RGBAColor& c) const {
	RGBAColor    c2;

	c2.red = red-c.red;
	c2.green = green-c.green;
	c2.blue = blue-c.blue;
	c2.alpha = alpha-c.alpha;
	return c2;
}

void RGBAColor::operator-= (const RGBAColor& c) {
	red -= c.red;
	green -= c.green;
	blue -= c.blue;
	alpha -= c.alpha;
}

RGBAColor RGBAColor::operator^ (const RGBAColor& c) const {
	RGBAColor    c2;

	c2.red = red*c.alpha+c.red;
	c2.green = green*c.alpha+c.green;
	c2.blue = blue*c.alpha+c.blue;
	c2.alpha = alpha;
	return c2;
}

#endif

 










