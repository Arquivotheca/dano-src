#ifndef _3D_FACE_LOOK_H
#define _3D_FACE_LOOK_H

#ifndef _3D_MODEL_H
#include "3dModel.h"
#endif
#ifndef _3D_LOOK_H
#include "3dLook.h"
#endif

class B3dLightProc;

enum {
	B_BODY_COLOR,
	B_FACE_COLOR,
	B_CORNER_COLOR
};

class B3dMapFaceLook : public B3dLook {
 public:
	float           *pt_refs;
	struct map_desc *map_list;
	int             curClip;
	map_look        LookClip[8];

	B3dMapFaceLook(char *name, B3dModel *Model, float *pt_refs, map_desc *map_list);
	virtual ~B3dMapFaceLook();
	virtual void *ClipHook(void *look1, float rel1, void *look2, float rel2);
};

class B3dMap1FaceLook : public B3dMapFaceLook {
 public:
	B3dMap1FaceLook(char *name, B3dModel *Model, float *pt_refs, map_desc *map_list);
	virtual ~B3dMap1FaceLook();
	virtual void Draw(ulong           flags,
					  B3dMatrix       *rotation,
					  B3dVector       *translation,
					  B3dLens         *lens,
					  B3dRenderer     *renderer,
					  B3dLightProc    *lightProc);
};

class B3dMap2FaceLook : public B3dMapFaceLook {
 public:
	B3dMap2FaceLook(char *name, B3dModel *Model, float *pt_refs, map_desc *map_list);
	virtual ~B3dMap2FaceLook();
	virtual void Draw(ulong         flags,
					  B3dMatrix     *rotation,
					  B3dVector     *translation,
					  B3dLens       *lens,
					  B3dRenderer   *renderer,
					  B3dLightProc  *lightProc);
 private:
	ulong    *BufferLook;
};

#endif

 









