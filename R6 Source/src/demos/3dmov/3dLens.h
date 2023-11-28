#ifndef _3D_LENS_H
#define _3D_LENS_H

#ifndef _3D_DEFS_H 
#include "3dDefs.h"
#endif
#ifndef _3D_THING_H
#include "3dThing.h"
#endif
#ifndef _3D_LOOK_H
#include "3dLook.h"
#endif
#ifndef _3D_MODEL_H
#include "3dModel.h"
#endif
#ifndef _3D_RENDERER_H
#include "3dRenderer.h"
#endif

enum {
	B_SEE_POINT =   0x00000001,
	B_SEE_VECTOR =  0x00000002,
	B_SEE_NORM =    0x00000004,
	B_SEE_CLIP =    0x00000008
};

enum {
	B_CLIPPED_OUT = 0x00000001,
	B_CLIPPED_IN =  0x00000002,
	B_UNCLIPPED =   0x00000003
};

class B3dLensOptions {
public:	
	ulong     Settings;
	B3dVector Origin;
	B3dMatrix Rotation;
	float     hMin;
	float     hMax;
	float     vMin;
	float     vMax;
};

class B3dLensPoint : public B3dVector {
 public:
	long     h;
	long     v;
	float    inv_z;
};

class B3dLensImage {
 public:
	void    *lensPoints;
	long    lensPointSize;
	B3dVector *lensVectors;
	inline void *lensPointAt(long index);
	inline B3dVector *lensVectorAt(long index);
};

void *B3dLensImage::lensPointAt(long index) {
	return (void*)((long)lensPoints+index*lensPointSize);
}

B3dVector *B3dLensImage::lensVectorAt(long index) {
	return lensVectors+index;
}

class B3dLens {
 public:
	virtual void GetOptions(void *options);
	virtual void SetOptions(void *options);
	virtual void See(B3dModel     *model,
					 B3dMatrix    *rotation,
					 B3dVector    *translation,
					 long         status,
					 B3dLensImage *lensImage);
	virtual void CalcAxis(short h, short v, B3dVector *axis);
    virtual long CheckSteric(B3dSteric *gab, B3dMatrix *rotation,
							 B3dVector *translation, float *radius);
	virtual void ClipMap(B3dLensPoint *p1,
						 B3dLensPoint *p2,
						 B3dLensPoint *p3,
						 map_look     *look1,
						 map_look     *look2,
						 map_look     *look3,
						 B_MAP        draw,
						 B3dLook      *looker,
						 void         *bits,
						 long         row,
						 struct map_ref *ref);
};

#endif














