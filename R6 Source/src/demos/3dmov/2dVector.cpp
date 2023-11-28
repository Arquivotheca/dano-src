#ifndef _2D_VECTOR_H
#include "2dVector.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

B2dVector B_2D_VECTOR_NULL = { 0.0, 0.0 };

float B2dVector::Length() const {
	float		f;
	
	f = x*x + y*y;
	if (f != 0.0)
		f = b_sqrt(f);
	return f;
}

float B2dVector::Norm(bool return_prev) {
	float		f2, f;

	f2 = x*x + y*y;
	if (return_prev) {
		if (f2 != 0.0) {
			f2 = b_sqrt(f2);
			f = 1.0/(f2);
			x *= f;
			y *= f;
		}
		return f2;
	}
	else {
		if (f2 != 0.0) {
			f2 = b_sqrt_inv(f2);
			x *= f2;
			y *= f2;
		}
		return 0.0;
	}
}

void B2dVector::Debug() {
	fprintf(stderr,"  Vector:(%f,%f\n",x,y);
}











