#ifndef _3D_DEFS_H
#include "3dDefs.h"
#endif
#ifndef _3D_VECTOR_H
#include "3dVector.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

B3dVector B_VECTOR_NULL = { 0.0, 0.0, 0.0 };

B3dVector B3dVector::Rotate(B3dVector *axis, float alpha) {
	float		C1,C2,C3;
	float		nX,nY,nZ;
	B3dVector   v;
	
	nX = axis->x;
	nY = axis->y;
	nZ = axis->z;
	b_get_cos_sin(alpha, &C1, &C3);
	C2 = (1.0-C1)*(x*nX+y*nY+z*nZ);
// calcule le vecteur resultat
	v.x = C1*x+C2*nX+C3*(nY*z-nZ*y);
	v.y = C1*y+C2*nY+C3*(nZ*x-nX*z);
	v.z = C1*z+C2*nZ+C3*(nX*y-nY*x);	
	return v;
}

float B3dVector::Length() const {
	float		f;
	
	f = x*x + y*y + z*z;
	if (f != 0.0)
		f = b_sqrt(f);
	return f;
}

float B3dVector::Norm(bool return_prev) {
	float		f2, f;
	
	f2 = x*x + y*y + z*z;
	if (!return_prev) {
		if (f2 != 0.0) {
			f2 = b_sqrt_inv(f2);
			x *= f2;
			y *= f2;
			z *= f2;
		}
	}
	else {
		if (f2 != 0.0) {
			f2 = b_sqrt(f2);
			f = 1.0/(f2);
			x *= f;
			y *= f;
			z *= f;
		}
		return f2;
	}
}

void B3dVector::Debug() {
	fprintf(stderr,"  Vector:(%f,%f,%f\n",x,y,z);
}












