#ifndef _3D_MATRIX_H
#define _3D_MATRIX_H

#ifndef _3D_VECTOR_H
#include "3dVector.h"
#endif

#include <stdio.h>

class B3dMatrix {
 public:
	float 	m11;
	float 	m12;
	float 	m13;
	float 	m21;
	float 	m22;
	float 	m23;
	float 	m31;
	float 	m32;
	float 	m33;

	inline B3dVector operator* (const B3dVector& v) const;
	inline B3dVector& X();
	inline B3dVector& Y();
	inline B3dVector& Z();
	inline void SetX(B3dVector& v);
	inline void SetY(B3dVector& v);
	inline void SetZ(B3dVector& v);
	inline void SetX(float new_x, float new_y, float new_z);
	inline void SetY(float new_x, float new_y, float new_z);
	inline void SetZ(float new_x, float new_y, float new_z);
	inline void Set(B3dVector& vx, B3dVector& vy, B3dVector& vz);
	B3dMatrix operator* (const B3dMatrix& m2) const;
	void operator*= (const B3dMatrix& m2);
	B3dMatrix operator+ (const B3dMatrix& m2) const;
	void operator+= (const B3dMatrix& m2);
	B3dMatrix operator- (const B3dMatrix& m2) const;
	void operator-= (const B3dMatrix& m2);
	void operator/= (const B3dMatrix& m2);
	float Determinant() const;
	void Set(const float alpha, const float theta, const float phi);
	void Set(const float alpha, const float theta, const float phi,
			 const float scale_x, const float scale_y, const float scale_z);
	void Set(const B3dVector *axis, const float alpha);
	void GetAxialRotateZ(B3dVector *axis, float *alpha);
	void Resize(B3dVector *size, B3dMatrix *dest);
	void Norm();
	void Debug();
};

extern B3dMatrix B_MATRIX_ID;

B3dVector B3dMatrix::operator* (const B3dVector& v) const {
	B3dVector    res;

	res.x = m11*v.x;
	res.y = m12*v.x;
	res.z = m13*v.x;
	res.x += m21*v.y;
	res.y += m22*v.y;
	res.z += m23*v.y;
	res.x += m31*v.z;
	res.y += m32*v.z;
	res.z += m33*v.z;
	return res;
}

B3dVector& B3dMatrix::X() {
	return *(B3dVector*)(&m11);
}

B3dVector& B3dMatrix::Y() {
	return *(B3dVector*)(&m21);
}

B3dVector& B3dMatrix::Z() {
	return *(B3dVector*)(&m31);
}

void B3dMatrix::SetX(B3dVector& v) {
	m11 = v.x;
	m12 = v.y;
	m13 = v.z;
}

void B3dMatrix::SetY(B3dVector& v) {
	m21 = v.x;
	m22 = v.y;
	m23 = v.z;
}

void B3dMatrix::SetZ(B3dVector& v) {
	m31 = v.x;
	m32 = v.y;
	m33 = v.z;
}

void B3dMatrix::SetX(float new_x, float new_y, float new_z) {
	m11 = new_x;
	m12 = new_y;
	m13 = new_z;
}

void B3dMatrix::SetY(float new_x, float new_y, float new_z) {
	m21 = new_x;
	m22 = new_y;
	m23 = new_z;
}

void B3dMatrix::SetZ(float new_x, float new_y, float new_z) {
	m31 = new_x;
	m32 = new_y;
	m33 = new_z;
}

void B3dMatrix::Set(B3dVector& vx, B3dVector& vy, B3dVector& vz) {
	m11 = vx.x;
	m12 = vx.y;
	m13 = vx.z;
	m21 = vy.x;
	m22 = vy.y;
	m23 = vy.z;
	m31 = vz.x;
	m32 = vz.y;
	m33 = vz.z;
}

#endif









