#ifndef _3D_WORLD_H
#define _3D_WORLD_H

#include <OS.h>

#ifndef _3D_DEFS_H 
#include "3dDefs.h"
#endif
#ifndef _3D_MATRIX_H 
#include "3dMatrix.h"
#endif
#ifndef _3D_UNIVERSE_H 
#include "3dUniverse.h"
#endif

class B3dCamera;
class B3dLight;
class B3dThing;
class B3dLens;
class B3dLighter;
class B3dRenderer;
class B3dEraseOptions;
class B3dTouchDesc;

enum {
	B_STATIC_THING =  0x00000001,
	B_MOVED_THING  =  0x00000002,
	B_RESIZED_THING = 0x00000004
};

// private struct 
typedef struct {
	B3dCamera *camera;
	sem_id    cam_sem;
	long      next;
	bool      On;
	bool      pipo[3];
} WTCamera;

class B3dWorld {
	friend long refresh_deamon(void *data);
	friend class B3dView;
 public:
	B3dWorld(char *name, B3dUniverse *universe = 0);
	virtual ~B3dWorld();
	inline const char   *Name();
	
	virtual void        AddThing(B3dThing *thing, ulong flags);
	inline long         ThingCount();
	virtual B3dThing    *GetThing(long id);
    inline B3dThing     *GetThing(char *name);
	virtual long        GetThingId(char *name);
	virtual long        GetNthThingId(long index);
	virtual void        RemoveThing(long id);
	inline void         RemoveThing(char *name);
	virtual void        CheckNewThings();
	virtual void        UpdateThings();

	inline B3dUniverse  *Universe();

	void                AddLight(B3dLight *light);
	inline long         LightCount();
	B3dLight            *GetLight(char *name);
	B3dLight            *GetNthLight(long index);
	void                RemoveLight(B3dLight *light);
	virtual void        UpdateLights();

	long                AddCamera(B3dCamera *camera);
	inline long         CameraCount();
	B3dCamera           *GetCamera(long id);
	inline B3dCamera    *GetCamera(char *name);
	long                GetCameraId(char *name);
	long                GetNthCameraId(long index);
	void                RemoveCamera(long id);
	inline void         RemoveCamera(char *name);
	void                CameraOn(long id, bool state);
	virtual void        UpdateCameras();
	virtual bool        GetTouch(B3dVector *origin, B3dVector *axis, B3dTouchDesc *touch);
	void                CalcTouch(B3dVector *origin,
								  B3dVector *axis,
								  B3dVector *origin0,
								  B3dVector *axis0,
								  B3dTouchDesc *touch,
								  B3dMatrix *mat);
	
	virtual void        RenderHook(B3dCamera *camera, B3dRenderer *renderer);
	void                SetUpdateRate(float rate);
	virtual void        Update();
	virtual void        Lock();
	virtual void        Unlock();
	virtual bool        IsLocked();
	
// private variables
	long                render_lock;
	double              currentFrameTime, prevFrameTime;

 private:
	long                CameraCnt;
	long                LightCnt;
	long                MaxCamera;
	long                MaxLight;
	long                ObjectCnt;
	long                free_camera;
	WTCamera            *CameraList;
	B3dLight            **LightList;
	B3dUniverse         *myUnivers;
	long                world_lock, kill_lock;
	sem_id              world_sem, kill_sem;
	char                wName[B_3D_NAME_LENGTH];
	double              UpdateTiming;
	thread_id           refresh_thread;
	long                please_die;
	virtual void        LockUpdate();
	virtual void        UnlockUpdate();
};

B3dUniverse *B3dWorld::Universe() {
	return myUnivers;
}

B3dThing *B3dWorld::GetThing(char *name) {
	return GetThing(GetThingId(name));
}

void B3dWorld::RemoveThing(char *name) {
	RemoveThing(GetThingId(name));
}

B3dCamera *B3dWorld::GetCamera(char *name) {
	return GetCamera(GetCameraId(name));
}

void B3dWorld::RemoveCamera(char *name) {
	RemoveCamera(GetCameraId(name));
}

long B3dWorld::ThingCount() {
	return ObjectCnt;
}

long B3dWorld::CameraCount() {
	return CameraCnt;
}

long B3dWorld::LightCount() {
	return LightCnt;
}

#endif





