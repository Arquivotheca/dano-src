/* ++++++++++

   FILE:  3dWorld.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_WORLD_H 
#include "3dWorld.h"
#endif
#ifndef _3D_BODY_H 
#include "3dBody.h"
#endif
#ifndef _3D_LENS_H 
#include "3dLens.h"
#endif
#ifndef _3D_CAMERA_H 
#include "3dCamera.h"
#endif
#ifndef _3D_RENDERER_H 
#include "3dRenderer.h"
#endif
#ifndef _3D_FACE_H 
#include "3dFace.h"
#endif
#ifndef _3D_MATH_LIB_H 
#include "3dMathLib.h"
#endif

#include <stdio.h>

B3dWorld::B3dWorld(char *name, B3dUniverse *univers) {
	long     i;
	
// name
	for (i=0;i<B_3D_NAME_LENGTH-1;i++)
		wName[i] = name[i];
	wName[B_3D_NAME_LENGTH-1] = 0;
	if (univers == 0)
		myUnivers = new B3dUniverse("Default");
	else
		myUnivers = univers;
	myUnivers->AddWorld(this);
// init default container
	ObjectCnt = 0;
	
	CameraCnt = 0;
	MaxCamera = 4;
	CameraList = (WTCamera*)malloc(sizeof(WTCamera)*MaxCamera);
	free_camera = 0;
	for (i=0;i<MaxCamera-1;i++) {
		CameraList[i].camera = 0L;
		CameraList[i].On = FALSE;
		CameraList[i].next = i+1;
	}
	CameraList[MaxCamera-1].next = -1;
	CameraList[MaxCamera-1].camera = 0L;
	
	LightCnt = 0;
	MaxLight = 8;
	LightList = (B3dLight**)malloc(sizeof(B3dLight*)*MaxLight);
// init benaphore protection
	world_lock = 0;
	world_sem = create_sem(0,"World lock");
	kill_lock = 0;
	kill_sem = create_sem(0,"Kill World lock");
	render_lock = 0;
	currentFrameTime = -1.0;
	UpdateTiming = 1e20;
}

B3dWorld::~B3dWorld() {
	long    i;

	delete_sem(world_sem);
	delete myUnivers;
	for (i=0;i<MaxCamera;i++)
		RemoveCamera(i);
	while (LightCnt > 0)
		RemoveLight(GetNthLight(0));
	free (CameraList);
	free (LightList);
}

void B3dWorld::AddThing(B3dThing *object, ulong flags) {
}

void B3dWorld::RemoveThing(long id) {
}

B3dThing *B3dWorld::GetThing(long id) {
	return 0L;
}

long B3dWorld::GetThingId(char *Name) {
	return 0L;
}

long B3dWorld::GetNthThingId(long index) {
	return 0L;
}

long B3dWorld::AddCamera(B3dCamera *camera) {
	long       i, id;
	WTCamera   *newList;
	
	if (free_camera == -1) {
		MaxCamera *= 2;
		newList = (WTCamera*)malloc(sizeof(WTCamera)*MaxCamera);
		memcpy(newList, CameraList, sizeof(WTCamera)*CameraCnt);
		free (CameraList);
		CameraList = newList;
		free_camera = CameraCnt;
		for (i=CameraCnt;i<MaxCamera-1;i++) {
			CameraList[i].camera = 0L;
			CameraList[i].On = FALSE;
			CameraList[i].next = i+1;
		}
		CameraList[MaxCamera-1].camera = 0L;
		CameraList[MaxCamera-1].next = -1;
	}
	id = free_camera;
	free_camera = CameraList[id].next;
	CameraList[id].camera = camera;
	CameraList[id].cam_sem = create_sem(0,"Camera lock");
	CameraList[id].On = TRUE;
	CameraCnt++;
	return id;
}

B3dCamera *B3dWorld::GetCamera(long id) {
	if ((id >= 0) && (id < MaxCamera))
		return CameraList[id].camera;
	else
		return 0L;
}

long B3dWorld::GetCameraId(char *name) {
	long        i, j;
	const char  *cName;

	for (i=0;i<MaxCamera;i++) {
		if (CameraList[i].camera == 0L) continue;
		cName = CameraList[i].camera->Name();
		j = -1;
		do {
			j++;
			if (name[j] != cName[j]) goto next_i;
		} while (name[j] != 0);
		return i;
	next_i:;
	}
	return 0L;
}

long B3dWorld::GetNthCameraId(long index) {
	long    i;

	for (i=0;i<MaxCamera;i++) {
		if (CameraList[i].camera == 0L) continue;
		if (index == 0) return i;
		index--;
	}
	return -1;
}

void B3dWorld::RemoveCamera(long id) {
	if ((id >= 0) && (id < MaxCamera)) {
		if (CameraList[id].camera != 0L) {
			delete_sem(CameraList[id].cam_sem);
			CameraList[id].next = free_camera;
			CameraList[id].camera = 0L;
			CameraList[id].On = FALSE;
			free_camera = id;
			CameraCnt--;
		}
	}
}

void B3dWorld::UpdateCameras() {
	long          i;
	B3dThing      *obj;

	for (i=0;i<MaxCamera;i++) {
		if (CameraList[i].camera == 0L) continue;
		obj = CameraList[i].camera;
		if (obj->link != 0L)
			obj->link->DoLink(obj, currentFrameTime, prevFrameTime);
	}
}

void B3dWorld::CameraOn(long id, bool state) {
	if ((id >= 0) && (id < MaxCamera))
		if (CameraList[id].camera != 0L)
			CameraList[id].On = state;
}

bool B3dWorld::GetTouch(B3dVector *origin, B3dVector *axis, B3dTouchDesc *touch) {
	return FALSE;
}

void B3dWorld::CalcTouch(B3dVector *origin,
						 B3dVector *axis,
						 B3dVector *origin0,
						 B3dVector *axis0,
						 B3dTouchDesc *touch,
						 B3dMatrix *mat) {
	float      scal, d, d1;
	B3dThing   *obj;
	B3dVector  ecart, trans, or0, ax0, ref0, ref1, or1, ax1;
	B3dMatrix  rot, cv, cvR;

	obj = touch->obj;
	or0 = (*origin)-obj->origin;
	ax0 = *axis;
	ref0 = touch->touch-obj->origin;
	if (origin0 != 0L)
		or1 = (*origin0)-obj->origin;
	else
		or1.Set(0.0, 0.0, 0.0);
// look for best axial rotation approximation
	if (axis0 != 0L) {
		ax1 = *axis0;
		ecart = ref0-or1;
		scal = ax1*ecart;
		ecart -= ax1*scal;
		or1 += ax1*scal;
		d1 = ecart.Length();
		trans = ax0^ax1;
		ecart = or1-or0;
		if (trans.Norm(TRUE) < 1e-4) {
			d = ecart*ax1;
			trans = ecart-(ax1*d);
			trans.Norm();
		}
		d = trans*ecart;
		if (d < 0.0) {
			trans = trans*(-1.0);
			d = -d;
		}
		ref1 = or1-(trans*d1);
	}
// look for best central rotation approximation
	else {
		ecart = or1-ref0;
		d1 = ecart.Length();
		ecart = or1-or0;
		scal = ax0*ecart;
		ecart -= ax0*scal;
		d = ecart.Length();
		if (d >= d1)
			ref1 = or1-(ecart*(d1/d));
		else {
			scal = b_sqrt(d1*d1-d*d);
			if (touch->side > 0.0)
				ref1 = (or1-ecart)+(ax0*scal);
			else
				ref1 = (or1-ecart)-(ax0*scal);
		}
	}
// convert move to rotation matrix
	ref0 -= or1;
	ref1 -= or1;
	ref0.Norm();
	ref1.Norm();
	ax1 = (ref0^ref1);
	scal = ax1.Norm(TRUE);
	if (scal < 1e-4) {
		mat->m11 = 1.0;
		mat->m12 = 0.0;
		mat->m13 = 0.0;
		mat->m21 = 0.0;
		mat->m22 = 1.0;
		mat->m23 = 0.0;
		mat->m31 = 0.0;
		mat->m32 = 0.0;
		mat->m33 = 1.0;
		return;
	}
	ax0 = ax1^ref0;
	ax0.Norm();
	d = ref1*ref0;
	d1 = ref1*ax0;
	rot.SetX(d, d1, 0.0);
	rot.SetY(-d1, d, 0.0);
	rot.SetZ(0.0, 0.0, 1.0);
	cv.SetX(ref0);
	cv.SetY(ax0);
	cv.SetZ(ax1);
	cvR /= cv;
	*mat = cv*rot*cvR;
}

void B3dWorld::AddLight(B3dLight *light) {
	if (LightCnt == MaxLight) {
		MaxLight *= 2;
		free(LightList);
		LightList = (B3dLight**)malloc(sizeof(B3dLight*)*MaxLight);
	}
	LightList[LightCnt++] = light;
}

B3dLight *B3dWorld::GetLight(char *name) {
	long       i, j;
	const char *lName;

	for (i=0;i<LightCnt;i++) {
		lName = LightList[i]->Name();
		j = -1;
		do {
			j++;
			if (name[j] != lName[j]) goto next_i;
		} while (name[j] != 0);
		return LightList[i];
	next_i:;
	}
	return 0L;
}

B3dLight *B3dWorld::GetNthLight(long index) {
	if ((index >= 0) && (index < LightCnt))
		return LightList[index];
	else
		return 0L;
}

void B3dWorld::RemoveLight(B3dLight *light) {
	long    i;

	for (i=0;i<LightCnt;i++)
		if (LightList[i] == light) {
			LightCnt--;
			LightList[i] = LightList[LightCnt];
			delete light;
			return;
		}
}

void B3dWorld::UpdateLights() {
	long          i;
	B3dThing      *obj;

	for (i=0;i<LightCnt;i++) {
		obj = LightList[i];
		if (obj->link != 0L)
			obj->link->DoLink(obj, currentFrameTime, prevFrameTime);
	}
}

void B3dWorld::RenderHook(B3dCamera *camera, B3dRenderer *buffer) {
}

void B3dWorld::CheckNewThings() {
}

void B3dWorld::SetUpdateRate(float rate) {
	if (rate > 0.0)
		UpdateTiming = 1e6/rate;
	else
		UpdateTiming = 1e20;
}

void B3dWorld::Update() {
	long    i, cnt;

// do the update
	if (currentFrameTime < 0.0)
		prevFrameTime = Universe()->Time();
	else
		prevFrameTime = currentFrameTime;
	currentFrameTime = Universe()->Time();
	Lock();
	CheckNewThings();
	UpdateThings();
	UpdateLights();
	UpdateCameras();
	Unlock();
}

void B3dWorld::UpdateThings() {	
}

void B3dWorld::Lock() {
	long    old_value;
	
	old_value = atomic_add(&world_lock, 1);
	if (old_value > 0)
		acquire_sem(world_sem);
}

void B3dWorld::Unlock() {
	long    old_value;

	old_value = atomic_add (&world_lock, -1);
	if (old_value > 1)
		release_sem(world_sem);
}

void B3dWorld::LockUpdate() {
	long    old_value;
	
	old_value = atomic_add(&kill_lock, 1);
	if (old_value > 0)
		acquire_sem(kill_sem);
}

void B3dWorld::UnlockUpdate() {
	long    old_value;

	old_value = atomic_add (&kill_lock, -1);
	if (old_value > 1)
		release_sem(kill_sem);
}

bool B3dWorld::IsLocked() {
	return (world_lock > 0);
}












