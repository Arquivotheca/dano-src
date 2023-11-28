#ifndef _3D_VECTOR_H
#define _3D_VECTOR_H

#include <stdio.h>

class B3dVector {
 public:
	float   x;
	float   y;
	float   z;

	inline float operator* (const B3dVector& v2) const;
	inline B3dVector operator* (const float k) const;
	inline void operator*= (const float k);
	inline B3dVector operator- (const B3dVector& v2) const;
	inline void operator-= (const B3dVector v2);
	inline B3dVector operator+ (const B3dVector& v2) const;
	inline void operator+= (const B3dVector v2);
	inline B3dVector operator^ (const B3dVector& v2) const;
	inline void operator^= (const B3dVector v2);
	inline void operator<<= (const B3dVector v2);
	inline void operator>>= (const B3dVector v2);
	inline void Set(float new_x, float new_y, float new_z);
	inline float Square();
	inline B3dVector VectorielX() const;
	inline B3dVector VectorielY() const;
	inline B3dVector VectorielZ() const;
	B3dVector Rotate(B3dVector *axis, float alpha);
	float Length() const;  
	float Norm(bool return_prev = FALSE);  
	void Debug();
};

extern B3dVector B_VECTOR_NULL;

float B3dVector::operator* (const B3dVector& v2) const {
	return x*v2.x + y*v2.y + z*v2.z;
}

B3dVector B3dVector::operator* (const float k) const {
	B3dVector v;

	v.x = x*k;
	v.y = y*k;
	v.z = z*k;
	return v;
}

void B3dVector::operator*= (const float k) {
	x *= k;
	y *= k;
	z *= k;
}

B3dVector B3dVector::operator- (const B3dVector& v2) const {
	B3dVector v;

	v.x = x-v2.x;
	v.y = y-v2.y;
	v.z = z-v2.z;
	return v;
}

void B3dVector::operator-= (const B3dVector v2) {
	x -= v2.x;
	y -= v2.y;
	z -= v2.z;
}

B3dVector B3dVector::operator+ (const B3dVector& v2) const {
	B3dVector v;

	v.x = x+v2.x;
	v.y = y+v2.y;
	v.z = z+v2.z;
	return v;
}

void B3dVector::operator+= (const B3dVector v2) {
	x += v2.x;
	y += v2.y;
	z += v2.z;
}

B3dVector B3dVector::operator^ (const B3dVector& v2) const {
	B3dVector v;

	v.x = y*v2.z - z*v2.y;
	v.y = z*v2.x - x*v2.z;
	v.z = x*v2.y - y*v2.x;
	return v;
}

void B3dVector::operator^= (const B3dVector v2) {
	B3dVector v;

	v.x = y*v2.z - z*v2.y;
	v.y = z*v2.x - x*v2.z;
	v.z = x*v2.y - y*v2.x;
	x = v.x;
	y = v.y;
	z = v.z;
}

B3dVector B3dVector::VectorielX() const {
	B3dVector v;

	v.x = 0.0;
	v.y = z;
	v.z = -y;
	return v;
}

B3dVector B3dVector::VectorielY() const {
	B3dVector v;

	v.x = -z;
	v.y = 0.0;
	v.z = x;
	return v;
}

B3dVector B3dVector::VectorielZ() const {
	B3dVector v;

	v.x = y;
	v.y = -x;
	v.z = 0.0;
	return v;
}

void B3dVector::operator<<= (const B3dVector v2) {
	if (v2.x < x) x = v2.x;
	if (v2.y < y) y = v2.y;
	if (v2.z < z) z = v2.z;
}

void B3dVector::operator>>= (const B3dVector v2) {
	if (v2.x > x) x = v2.x;
	if (v2.y > y) y = v2.y;
	if (v2.z > z) z = v2.z;
}

void B3dVector::Set(float new_x, float new_y, float new_z) {
	x = new_x;
	y = new_y;
	z = new_z;
}

float B3dVector::Square() {
	return (x*x + y*y + z*z);
}

#endif










