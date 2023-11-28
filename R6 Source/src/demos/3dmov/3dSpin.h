#ifndef _3D_SPIN_H
#define _3D_SPIN_H

#include <unistd.h>

#ifndef _3D_FACE_H 
#include "3dFace.h"
#endif
#ifndef _3D_SPIN_MODEL_H 
#include "3dSpinModel.h"
#endif

class B3dSpin : public B3dFace {
 public:
	B3dSpin(char *name,
			B2dVector *shape,
			long shapeCount,
			long radiusCount ,
			ulong flags = 0);
	virtual ~B3dSpin();
};

#endif

