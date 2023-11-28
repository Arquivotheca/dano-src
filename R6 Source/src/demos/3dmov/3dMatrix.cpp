#ifndef _3D_DEFS_H
#include "3dDefs.h"
#endif
#ifndef _3D_MATRIX_H
#include "3dMatrix.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

B3dMatrix B_MATRIX_ID = {
	1.0, 0.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 0.0, 1.0,
};

B3dMatrix B3dMatrix::operator* (const B3dMatrix& m2) const {
	float   a1,a2,a3,b1,b2,b3;
	float	r11,r12,r13,r21,r22,r23,r31,r32,r33;
	B3dMatrix m;
	
	a1 = m11;
	a2 = m12;
	a3 = m13;
	b1 = m2.m11;
	b2 = m2.m21;
	b3 = m2.m31;
	
	r11 = a1*b1;
	r21 = a1*b2;
	r31 = a1*b3;
	r12 = a2*b1;
	r22 = a2*b2;
	r32 = a2*b3;
	r13 = a3*b1;
	r23 = a3*b2;
	r33 = a3*b3;
	
	a1 = m21;
	a2 = m22;
	a3 = m23;
	b1 = m2.m12;
	b2 = m2.m22;
	b3 = m2.m32;
	
	r11 += a1*b1;
	r21 += a1*b2;
	r31 += a1*b3;
	r12 += a2*b1;
	r22 += a2*b2;
	r32 += a2*b3;
	r13 += a3*b1;
	r23 += a3*b2;
	r33 += a3*b3;
	
	a1 = m31;
	a2 = m32;
	a3 = m33;
	b1 = m2.m13;
	b2 = m2.m23;
	b3 = m2.m33;
	
	r11 += a1*b1;
	r21 += a1*b2;
	r31 += a1*b3;
	r12 += a2*b1;
	r22 += a2*b2;
	r32 += a2*b3;
	r13 += a3*b1;
	r23 += a3*b2;
	r33 += a3*b3;
	
	m.m11 = r11;
	m.m21 = r21;
	m.m31 = r31;
	m.m12 = r12;
	m.m22 = r22;
	m.m32 = r32;
	m.m13 = r13;
	m.m23 = r23;
	m.m33 = r33;

	return m;
}

void B3dMatrix::operator*= (const B3dMatrix& m2) {
	float   a1,a2,a3,b1,b2,b3;
	float	r11,r12,r13,r21,r22,r23,r31,r32,r33;
	
	a1 = m11;
	a2 = m12;
	a3 = m13;
	b1 = m2.m11;
	b2 = m2.m21;
	b3 = m2.m31;
	
	r11 = a1*b1;
	r21 = a1*b2;
	r31 = a1*b3;
	r12 = a2*b1;
	r22 = a2*b2;
	r32 = a2*b3;
	r13 = a3*b1;
	r23 = a3*b2;
	r33 = a3*b3;
	
	a1 = m21;
	a2 = m22;
	a3 = m23;
	b1 = m2.m12;
	b2 = m2.m22;
	b3 = m2.m32;
	
	r11 += a1*b1;
	r21 += a1*b2;
	r31 += a1*b3;
	r12 += a2*b1;
	r22 += a2*b2;
	r32 += a2*b3;
	r13 += a3*b1;
	r23 += a3*b2;
	r33 += a3*b3;
	
	a1 = m31;
	a2 = m32;
	a3 = m33;
	b1 = m2.m13;
	b2 = m2.m23;
	b3 = m2.m33;
	
	r11 += a1*b1;
	r21 += a1*b2;
	r31 += a1*b3;
	r12 += a2*b1;
	r22 += a2*b2;
	r32 += a2*b3;
	r13 += a3*b1;
	r23 += a3*b2;
	r33 += a3*b3;
	
	m11 = r11;
	m21 = r21;
	m31 = r31;
	m12 = r12;
	m22 = r22;
	m32 = r32;
	m13 = r13;
	m23 = r23;
	m33 = r33;
}

B3dMatrix B3dMatrix::operator+ (const B3dMatrix& m2) const {
	B3dMatrix  m;

	m.m11 = m11 + m2.m11;
	m.m21 = m21 + m2.m21;
	m.m31 = m31 + m2.m31;
	m.m12 = m12 + m2.m12;
	m.m22 = m22 + m2.m22;
	m.m32 = m32 + m2.m32;
	m.m13 = m13 + m2.m13;
	m.m23 = m23 + m2.m23;
	m.m33 = m33 + m2.m33;
	return m;
}

void B3dMatrix::operator+= (const B3dMatrix& m2) {
	m11 += m2.m11;
	m21 += m2.m21;
	m31 += m2.m31;
	m12 += m2.m12;
	m22 += m2.m22;
	m32 += m2.m32;
	m13 += m2.m13;
	m23 += m2.m23;
	m33 += m2.m33;
}

B3dMatrix B3dMatrix::operator- (const B3dMatrix& m2) const {
	B3dMatrix  m;

	m.m11 = m11 - m2.m11;
	m.m21 = m21 - m2.m21;
	m.m31 = m31 - m2.m31;
	m.m12 = m12 - m2.m12;
	m.m22 = m22 - m2.m22;
	m.m32 = m32 - m2.m32;
	m.m13 = m13 - m2.m13;
	m.m23 = m23 - m2.m23;
	m.m33 = m33 - m2.m33;
	return m;
}

void B3dMatrix::operator-= (const B3dMatrix& m2) {
	m11 -= m2.m11;
	m21 -= m2.m21;
	m31 -= m2.m31;
	m12 -= m2.m12;
	m22 -= m2.m22;
	m32 -= m2.m32;
	m13 -= m2.m13;
	m23 -= m2.m23;
	m33 -= m2.m33;
}

void B3dMatrix::operator/= (const B3dMatrix& m2) {
	float	f;
	
	f = 1.0/m2.Determinant();
	m11 = (m2.m22*m2.m33 - m2.m23*m2.m32)*f;
	m12 = (m2.m13*m2.m32 - m2.m12*m2.m33)*f;
	m13 = (m2.m12*m2.m23 - m2.m13*m2.m22)*f;
	m21 = (m2.m23*m2.m31 - m2.m21*m2.m33)*f;
	m22 = (m2.m11*m2.m33 - m2.m13*m2.m31)*f;
	m23 = (m2.m13*m2.m21 - m2.m11*m2.m23)*f;
	m31 = (m2.m21*m2.m32 - m2.m22*m2.m31)*f;
	m32 = (m2.m12*m2.m31 - m2.m11*m2.m32)*f;
	m33 = (m2.m11*m2.m22 - m2.m12*m2.m21)*f;
}

void B3dMatrix::Set(const float alpha, const float theta, const float phi) {
	float		cD,sD,cI,sI,cA,sA;
	
// trigo
	b_get_cos_sin(alpha, &cD, &sD);
	b_get_cos_sin(theta, &cI, &sI);
	b_get_cos_sin(phi, &cA, &sA);
// rotation matrix
	m11 = cD*cI;
	m21 = -sD*cI;
	m31 = -sI;
	m12 = sD*cA-cD*sI*sA;
	m22 = cD*cA+sD*sI*sA;
	m32 = -sA*cI;
	m13 = sD*sA+cD*cA*sI;
	m23 = cD*sA-sD*sI*cA;
	m33 = cI*cA;
}

void B3dMatrix::Set(const B3dVector *axis, const float alpha) {
	float		cosA, sinA;
	B3dMatrix   rot, rot2, m;  

// trigo
	b_get_cos_sin(alpha, &cosA, &sinA);
	m.X().Set(cosA, sinA, 0.0);
	m.Y().Set(-sinA, cosA, 0.0);
	m.Z().Set(0.0, 0.0, 1.0);

	rot2.Z() = *axis;
	rot2.Z().Norm();
	if ((rot2.Z().x > 0.5) || (rot2.Z().x < -0.5))
		rot2.X() = rot2.Z().VectorielY();
	else
		rot2.X() = rot2.Z().VectorielX();
	rot2.X().Norm();
	rot2.Y() = rot2.Z()^rot2.X();
	
	rot /= rot2;

	m = rot2*m*rot;
	
	Set(m.X(), m.Y(), m.Z());
}

void B3dMatrix::GetAxialRotateZ(B3dVector *axis, float *alpha) {
	axis->Set(-m32, m31, 0.0);
	if (axis->Norm(TRUE) < 1e-5)
		axis->Set(1.0, 0.0, 0.0);
	*alpha = b_arccos(m33);
}

float B3dMatrix::Determinant () const {
	return	m11 * m22 * m33 +
	    	m12 * m23 * m31 +
	    	m13 * m21 * m32 -
	    	m11 * m23 * m32 -
	    	m12 * m21 * m33 -
	    	m13 * m22 * m31;
}

void B3dMatrix::Resize(B3dVector *size, B3dMatrix *dest) {
	dest->m11 = m11*size->x;
	dest->m12 = m12*size->x;
	dest->m13 = m13*size->x;
	dest->m21 = m21*size->y;
	dest->m22 = m22*size->y;
	dest->m23 = m23*size->y;
	dest->m31 = m31*size->z;
	dest->m32 = m32*size->z;
	dest->m33 = m33*size->z;
}

void B3dMatrix::Norm() {
	X().Norm();
	Z() = X()^Y();
	Z().Norm();
	Y() = Z()^X();
	Y().Norm();
}

void B3dMatrix::Debug() {
	fprintf(stderr,"  Matrix[%f,%f,%f]\n",m11,m21,m31);
	fprintf(stderr,"        [%f,%f,%f]\n",m12,m22,m32);
	fprintf(stderr,"        [%f,%f,%f]\n",m13,m23,m33);
}











