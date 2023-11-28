#ifndef _3D_ETHERIC_WORLD_H
#define _3D_ETHERIC_WORLD_H

#ifndef _3D_WORLD_H 
#include "3dWorld.h"
#endif

typedef struct Node {
	B3dThing    *obj;
	long        next;
} Node;

class B3dEthericWorld : public B3dWorld {
 public:
	B3dEthericWorld(char *name, B3dUniverse *universe = 0);
	~B3dEthericWorld();

	virtual void        AddThing(B3dThing *thing, ulong flags);
	inline long         CountThings();
	virtual B3dThing    *GetThing(long id);
	virtual long        GetThingId(char *name);
	virtual long        GetNthThingId(long index);
	virtual void        RemoveThing(long id);
	virtual void        CheckNewThings();
	virtual void        UpdateThings();

	virtual bool        GetTouch(B3dVector *origin, B3dVector *axis, B3dTouchDesc *touch);

	virtual void        RenderHook(B3dCamera *camera, B3dRenderer *renderer);
	
	struct Node  *ObjList;
	long         ListCount;
	long         ObjUsed;
	long         ObjFree;
	long         *OrderList;
	float        *DepthList;
	
 private:
	void         ExtendList();
	long         SortThings(B3dVector *camera, B3dMatrix *rotation);
};

#endif















