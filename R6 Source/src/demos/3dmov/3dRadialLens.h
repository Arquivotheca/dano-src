#ifndef _3D_RADIAL_LENS_H
#define _3D_RADIAL_LENS_H

#ifndef _3D_LENS_H
#include "3dLens.h"
#endif

enum {
	B_CLIP_HMIN = 0x0001,
	B_CLIP_HMAX = 0x0002,
	B_CLIP_VMIN = 0x0004,
	B_CLIP_VMAX = 0x0008,
	B_CLIP_ZMIN = 0x0010
};

class B3dRadialLens;

class B3dRadialOptions : public B3dLensOptions {
	friend class B3dRadialLens;
	
 public:
	float    zoom;
	void     CheckOptions();
 private:
	float    OffsetH;
	float    OffsetV;
	float    XZMin;
	float    XZMax;
	float    YZMin;
	float    YZMax;
	float    NormXZMin;
	float    NormXZMax;
	float    NormYZMin;
	float    NormYZMax;
}; 

class B3dRadialPoint : public B3dLensPoint {
 public:
	long     visible;
};

class B3dRadialLens : public B3dLens {
 public:
	B3dRadialLens();
	virtual ~B3dRadialLens();
	virtual void GetOptions(void *options);
	virtual void SetOptions(void *options);
	virtual void See(B3dModel   *model,
					 B3dMatrix  *rotation,
					 B3dVector  *translation,
					 long       status,
					 B3dLensImage *lensImage);
    virtual long CheckSteric(B3dSteric *Gab,
							 B3dMatrix *rotation,
							 B3dVector *translation,
							 float     *radius);
	virtual void CalcAxis(short h, short v, B3dVector *axis);
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
	
 private:
	long           maxVector;
	long           maxPt;
	B3dVector      *lensVectors;
	B3dLensPoint   *lensPoints;
	long           ClipNbPt;
	void           *ClipLook[7];
	B3dVector      ClipNorm;
	B3dRadialPoint ClipList[8];
	B3dRadialPoint *curClipList;
	B3dRadialPoint *ClipPt[7];
	B3dRadialOptions Options;
	
	void         CheckListSize(long nbVector, long nbPt);
	void         ClipPlan(B3dLook *looker);
	void         ClipPlan2();
	void         CalculeNewPt(B3dRadialPoint *pt1, float rel1,
							  B3dRadialPoint *pt2, float rel2,
							  B3dRadialPoint *newPt);
};

#endif











