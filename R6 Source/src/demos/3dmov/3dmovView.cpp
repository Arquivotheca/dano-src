//*****************************************************************************
//
//	File:		 3dmovView.cpp
//
//	Description: 3d demo constructor using the 3d Kit.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

#ifndef _3D_STAMP_MODEL_H
#include "3dStampModel.h"
#endif
#ifndef _3D_MOV_VIEW_H
#include "3dmovView.h"
#endif
#ifndef _3D_WORLD_H
#include "3dWorld.h"
#endif
#ifndef _3D_CUBE_H
#include "3dCube.h"
#endif
#ifndef _3D_STAMP_H
#include "3dStamp.h"
#endif
#ifndef _3D_SPHERE_H
#include "3dSphere.h"
#endif
#ifndef _3D_SPIN_H
#include "3dSpin.h"
#endif
#ifndef _3D_MOV_APP_H
#include "3dmovApp.h"
#endif

#include <Debug.h>

long draw_thread(void *data);

#define	TestCode(key, x)		(key.key_states[x>>3]&(128>>(x&7)))

Z3dView::Z3dView(BRect rect, char *name, long index) : B3dView(name, rect, 0L) {
	updateTiming = 5e4;
	myIndex = index;
	kill_sem = create_sem(0, "Kill 3dmovView");
}

Z3dView::~Z3dView() {
	acquire_sem(kill_sem);
	delete_sem(kill_sem);
}

void Z3dView::AttachedToWindow() {
	int               i, j;
	B3dFace           *Body, *Copy;
	B3dVector         gabarit, origin, axis;
	B3dMatrix         rotation;
	RGBAColor         Color;
	B3dAmbientLight   *aLight;
	B3dParallelLight  *dLight;
	B3dRadialOptions  PPOptions;

//	_sPrintf("Index : %d\n", myIndex);
	if (myIndex == 0) {
		gabarit.Set(1.0, 1.0, 1.0);
		Body = new B3dCube("Cube", &gabarit);
		rotation.Set(0.0, -0.55, 0.0);
		origin.Set(0.0, 0.0, 0.0);
		Body->MoveTo(&origin,&rotation);

//		_sPrintf("calling : %x\n", ((Z3dApplication*)be_app)->map_ref_list);
		((B3dCube*)Body)->SetMapping(((Z3dApplication*)be_app)->map_ref_list+0);
		axis.Set(0.0, 0.0, 2.0);
		Body->LinkTo(&axis, 0.25, B_THING_REFERENCE);
		World()->AddThing(Body, B_STATIC_THING);

		Color.Set(1.0, 1.0, 1.0, 0.0);
		aLight = new B3dAmbientLight("Ambient white", 0.5, &Color);
		World()->AddLight(aLight);

		axis.Set(0.42, 0.43, 0.8);
		dLight = new B3dParallelLight("Powerful white", 0.7, &Color, &axis);
		World()->AddLight(dLight);

		Camera()->MoveTo(-1.7, 0.0, 0.0);
		Camera()->LookAt(Body);
	}
	else if (myIndex == 1) {
		gabarit.Set(1.0, 1.0, 1.0);
		Body = new B3dSphere("Sphere", &gabarit, 500, B_SMOOTH_MODEL);
		rotation.Set(1.57, 0.2, 0.0);
		origin.Set(0.0, 0.0, 0.0);
		Body->MoveTo(&origin,&rotation);

		((B3dSphere*)Body)->SetMapping(((Z3dApplication*)be_app)->map_ref_list+6);
		World()->AddThing(Body, B_STATIC_THING);

		Color.Set(1.0, 1.0, 1.0, 0.0);
		aLight = new B3dAmbientLight("Ambient white", 0.4, &Color);
		World()->AddLight(aLight);

		Color.Set(1.0, 1.0, 1.0, 0.0);
		axis.Set(0.2, -0.9, 0.4);
		dLight = new B3dParallelLight("Powerful white", 0.8, &Color, &axis);
		dLight->LinkTo(0.4, 0.3, 1.0);
		World()->AddLight(dLight);

		Camera()->MoveTo(-1.5, 0.0, 0.0);
		Camera()->LookAt(Body);
	}
	else if (myIndex == 2) {
		gabarit
			.Set(1.0, 1.0, 1.0);
		pulse = new B3dStamp("Stamp", 1.0, 12, B_SMOOTH_MODEL);
		rotation.Set(0.0, -0.8, 0.0);
		origin.Set(0.0, 0.0, 0.0);
		pulse->MoveTo(&origin,&rotation);

		pulse->SetMapping(((Z3dApplication*)be_app)->map_ref_list+7);
		World()->AddThing(pulse, B_STATIC_THING);

		Color.Set(1.0, 1.0, 1.0, 0.0);
		aLight = new B3dAmbientLight("Ambient white", 0.28, &Color);
		World()->AddLight(aLight);

		axis.Set(0.42, 0.43, 0.8);
		dLight = new B3dParallelLight("Powerful white", 0.8, &Color, &axis);
		dLight->LinkTo(0.2, 0.3, 0.17);
		World()->AddLight(dLight);

		Camera()->MoveTo(-1.2, 0.0, 0.0);
		Camera()->LookAt(pulse);
	}

	Camera()->Lens()->GetOptions(&PPOptions);
	PPOptions.zoom = 400.0;
	Camera()->Lens()->SetOptions(&PPOptions);

	draw_frame = spawn_thread((thread_entry)draw_thread,
							  "3d draw thread",
							  B_NORMAL_PRIORITY,
							  (void*)this);
	resume_thread(draw_frame);
	MakeFocus();
}

void Z3dView::MessageReceived(BMessage *msg) {
	long		 index;
	BPoint	     point, screen;
	B3dTouchDesc touch;
	entry_ref	 *tref;
	
	switch (msg->what) {
	case B_SIMPLE_DATA:
		if (msg->HasRef("refs")) {
			tref = new entry_ref();
			msg->FindRef("refs", tref);
			if (msg->WasDropped()) {
				screen = msg->DropPoint();
				point = (BPoint)ConvertFromScreen(screen);
				if (Camera()->GetTouchDesc((long)point.x, (long)point.y, &touch)) {
					if (myIndex == 0)
						index = touch.index_face/2;
					else if (myIndex == 1)
						index = 6;
					else if (myIndex == 2)
						index = 7+(touch.index_face&1);
					((Z3dApplication*)be_app)->SelectVideoChannel(index, tref);
				}
			}
			else delete tref; // memory leak potential here!!!
		}
	}
}

void Z3dView::KeyDown(const char *bytes, int32 numBytes) {
	((Z3dApplication*)be_app)->mapping_stable = false;
}

void Z3dView::Stop() {
	draw_frame = -1;
}

void Z3dView::MyMoveCamera() {
	int				i;
	char            button[2];
	key_info		theKey;

	if (!Window()->IsActive()) return;
	get_key_info(&MyKey);
	for (i=0;i<2;i++) {
		if (TestCode(MyKey, cfg.Bouton[i])!=0) {
			((Z3dApplication*)be_app)->mapping_stable = false;
			button[i] = 1;
		}
		else
			button[i] = 0;
	}
	if (button[0]) 
		myCamera->origin += myCamera->rotation.Z()*0.15;
	if (button[1])
		myCamera->origin -= myCamera->rotation.Z()*0.15;
}

long draw_thread(void *data) {
	int             i;
	BRect           bound;
	Z3dView         *myView;
	B3dWorld        *world;
	B3dEraseOptions opt;

	myView = (Z3dView*)data;
	while (myView->Window() == 0L)
		snooze(5e4);
	world = (B3dWorld*)myView->myWorld;
	opt.color.Set(0.0, 0.0, 0.0, 0.0);
	((Z3dApplication*)be_app)->EnableSync();
	while (TRUE) {
		world->Update();
		((Z3dApplication*)be_app)->Sync();
		myView->myRenderer->EraseBuffer(&opt);
		world->Lock();
		if (myView->myIndex == 2)
			((B3dStampModel*)(myView->pulse->desc.look->model))
				->SetShape(world->Universe()->Time());
		world->RenderHook(myView->myCamera, myView->myRenderer);
		world->Unlock();
		if (myView->draw_frame == -1) {
			release_sem(myView->kill_sem);
			break;
		}
		myView->Window()->Lock();
		myView->Draw(myView->Bounds());
		if ((myView->new_width != 0.0) && (myView->new_height != 0.0)) {
			myView->CheckBuffer();
			myView->new_width = 0.0;
			myView->new_height = 0.0;
		}
		myView->MyMoveCamera();
		myView->Window()->Unlock();
		if (myView->draw_frame == -1) {
			release_sem(myView->kill_sem);
			break;
		}
	}
	((Z3dApplication*)be_app)->DisableSync();
	return 0;
}

