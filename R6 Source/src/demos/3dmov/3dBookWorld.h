#ifndef _3D_BOOK_WORLD_H
#define _3D_BOOK_WORLD_H

#ifndef _3D_ETHERIC_WORLD_H 
#include "3dEthericWorld.h"
#endif

class B3dBookWorld : public B3dEthericWorld {
 public:
	B3dBookWorld(char *name, B3dUniverse *universe = 0);
	~B3dBookWorld();

	virtual void        RenderHook(B3dCamera *camera, B3dRenderer *renderer);
	
 private:
	long         SortThings(B3dVector *camera, B3dMatrix *rotation);
};

#endif















