#ifndef _3D_LINK_H
#define _3D_LINK_H

#ifndef _3D_DEFS_H 
#include "3dDefs.h"
#endif
#ifndef _3D_MATRIX_H 
#include "3dMatrix.h"
#endif

enum {
	B_WORLD_REFERENCE,
	B_THING_REFERENCE
};

class B3dThing;

class B3dLink {
 public:
	long    referenceCount;
	B3dLink();
	virtual ~B3dLink();
	virtual void DoLink(B3dThing *Object, double time0, double time1);
};

class B3dLink0:B3dLink {
 public:
	B3dLink0(B3dVector *axis,
			 float speed,
			 long mode = B_WORLD_REFERENCE,
			 B3dVector *origin = 0L);
	virtual ~B3dLink0();
	virtual void DoLink(B3dThing *Object, double time0, double time1);
 private:
	short     Mode;
	B3dVector Axis;
	float     Speed;
	B3dVector *Origin;
};

class B3dLink1:B3dLink {
 public:
	B3dLink1(float speed_alpha,
			 float speed_theta,
			 float speed_phi,
			 B3dVector *origin = 0L);
	virtual ~B3dLink1();
	virtual void DoLink(B3dThing *Object, double time0, double time1);
 private:
	float     Pos[3];
	float     Speed[3];
	B3dVector Port;
	B3dVector *Origin;
};

class B3dLink2:B3dLink {
 public:
	B3dLink2(B3dThing *from, B3dVector *to, B3dMatrix *to_mat, float speed);
	virtual ~B3dLink2();
	virtual void DoLink(B3dThing *Object, double time0, double time1);
 private:
	float     term;
	B3dVector Move;
	B3dVector Axis;
	float     Speed;
};

#endif






