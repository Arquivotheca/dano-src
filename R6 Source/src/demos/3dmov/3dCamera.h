#ifndef _3D_CAMERA_H
#define _3D_CAMERA_H

#ifndef _3D_DEFS_H 
#include "3dDefs.h"
#endif
#ifndef _3D_THING_H
#include "3dThing.h"
#endif
#ifndef _3D_WORLD_H
#include "3dWorld.h"
#endif
#ifndef _3D_LENS_H
#include "3dLens.h"
#endif
#ifndef _3D_LIGHTER_H
#include "3dLighter.h"
#endif

enum {
	B_RADIAL_LENS
};

enum {
	B_STANDARD_LIGHT
};

class B3dTouchDesc {
 public:
	long            index_face;
	float           side;
	B3dVector       touch;
	B3dThing        *obj;
};

class B3dCamera : public B3dThing {
 public:
	B3dCamera(char *name, B3dWorld *theWorld, ushort ProjectorMode, ushort LighterMode);
	virtual ~B3dCamera();
    long          SetProjectorMode(ushort ProjectorMode);
	long          SetLighterMode(ushort LighterMode);
	void          GetProjectorOptions(void *options);
	void          SetProjectorOptions(void *options);
	void          GetLighterOptions(void *options);
	void          SetLighterOptions(void *options);
    ushort        ProjectorMode();
	ushort        LighterMode();
	void          SetFrame(long Hmin, long Hmax, long Vmin, long Vmax);
	bool          GetTouchDesc(short h, short v, B3dTouchDesc *touch);
	void          CalcTouchDesc(short h, short v,
								B3dVector *origin0, B3dVector *axis0,
								B3dTouchDesc *touch, B3dMatrix *mat);
	inline B3dLighter* Lighter();
	inline B3dLens* Lens();
	
 private:
	short         ProjMode;
	short         LightMode;
	B3dLens       *myProjector;
	B3dWorld      *world;
	B3dLighter    *myLighter;
};

B3dLighter *B3dCamera::Lighter() {
	return myLighter;
}

B3dLens *B3dCamera::Lens() {
	return myProjector;
}

#endif












