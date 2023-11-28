#ifndef _2D_VECTOR_H
#define _2D_VECTOR_H

#include <SupportDefs.h>
#include <stdio.h>

class B2dVector {
 public:
	float   x;
	float   y;

	inline float operator* (const B2dVector& v2) const;
	inline B2dVector operator* (const float k) const;
	inline B2dVector operator- (const B2dVector& v2) const;
	inline void operator-= (const B2dVector v2);
	inline B2dVector operator+ (const B2dVector& v2) const;
	inline void operator+= (const B2dVector v2);
	inline float operator^ (const B2dVector& v2) const;
	inline void operator<<= (const B2dVector v2);
	inline void operator>>= (const B2dVector v2);
	inline void Set(float new_x, float new_y);
	inline float Square();
	float Length() const;  
	float Norm(bool return_prev = FALSE);  
	void Debug();
};

extern B2dVector B_2D_VECTOR_NULL;

float B2dVector::operator* (const B2dVector& v2) const {
	return x*v2.x + y*v2.y;
}

B2dVector B2dVector::operator* (const float k) const {
	B2dVector w;

	w.x = x*k;
	w.y = y*k;
	return w;
}

B2dVector B2dVector::operator- (const B2dVector& v2) const {
	B2dVector w;

	w.x = x-v2.x;
	w.y = y-v2.y;
	return w;
}

void B2dVector::operator-= (const B2dVector v2) {
	x -= v2.x;
	y -= v2.y;
}

B2dVector B2dVector::operator+ (const B2dVector& v2) const {
	B2dVector w;

	w.x = x+v2.x;
	w.y = y+v2.y;
	return w;
}

void B2dVector::operator+= (const B2dVector v2) {
	x += v2.x;
	y += v2.y;
}

float B2dVector::operator^ (const B2dVector& v2) const {
	return x*v2.y-y*v2.x;
}

void B2dVector::operator<<= (const B2dVector v2) {
	if (v2.x < x) x = v2.x;
	if (v2.y < y) y = v2.y;
}

void B2dVector::operator>>= (const B2dVector v2) {
	if (v2.x > x) x = v2.x;
	if (v2.y > y) y = v2.y;
}

void B2dVector::Set(float new_x, float new_y) {
	x = new_x;
	y = new_y;
}

float B2dVector::Square() {
	return (x*x + y*y);
}

#endif







