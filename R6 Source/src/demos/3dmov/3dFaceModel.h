#ifndef _3D_FACE_MODEL_H
#define _3D_FACE_MODEL_H

#ifndef _3D_MODEL_H
#include "3dModel.h"
#endif

enum {
	B_SMOOTH_MODEL =  0x00000001,
	
	B_OWN_FACE_LIST = 0x00000002,
	B_OWN_FACE_DESC = 0x00000004
};

typedef struct {
	short    points[3];
	short    norm;
} B3dFaceDesc;

typedef struct {
	long            pointCount;
	long            faceCount;
	B3dVector       *points;
	B3dFaceDesc     *faces;
	void            *extension;
} B3dFaceModelDesc;

class B3dFaceModel : public B3dModel {
 public:
	short       faceCount;
	ushort      status;
	ushort      *sortedList;
	B3dFaceDesc *faces;
	void        *extension;
	
	B3dFaceModel(B3dFaceModelDesc *desc, ulong flags, ulong OwnFlag = 0);
	virtual ~B3dFaceModel();
	virtual void CalcSort(B3dLensImage *lensImage, void **sortDesc);
	virtual void Debug();
};

#endif

 






