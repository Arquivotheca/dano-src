#ifndef _3D_STAMP_H
#define _3D_STAMP_H

#include <unistd.h>

#ifndef _BITMAP_H 
#include "Bitmap.h"
#endif
#ifndef _VIEW_H 
#include "View.h"
#endif
#ifndef _3D_FACE_H 
#include "3dFace.h"
#endif
#ifndef _3D_STAMP_MODEL_H 
#include "3dStampModel.h"
#endif

class B3dStamp : public B3dFace {
 public:
	B3dStamp(char *name, float size, long step, ulong flags = 0L);
	virtual ~B3dStamp();
	void    SetMapping(struct map_ref *list_map);

	long    step0;
	float   size0;
};

#endif







