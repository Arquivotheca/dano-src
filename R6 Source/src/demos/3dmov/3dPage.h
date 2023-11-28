#ifndef _3D_PAGE_H
#define _3D_PAGE_H

#include <unistd.h>

#ifndef _3D_FACE_H 
#include "3dFace.h"
#endif
#ifndef _3D_PAGE_MODEL_H 
#include "3dPageModel.h"
#endif

class B3dPage : public B3dFace {
 public:
	B3dPage(char *name, float size, long step, ulong flags = 0L);
	virtual ~B3dPage();
	void    SetMapping(struct map_ref *list_map, struct map_ref *list_number);
};

#endif







