/*
	
	3dView.cpp
	
	Copyright 1996 Be Incorporated, All Rights Reserved.
	
*/

#include <InterfaceDefs.h>

#ifndef _3D_VIEW_H
#include "3dView.h"
#endif
#ifndef _3D_RADIAL_LENS_H
#include "3dRadialLens.h"
#endif
#ifndef _3D_MOV_APP_H
#include "3dmovApp.h"
#endif

static	short	DureeToVitesse[24] = {
	16,	24,	32,	40,	48,	56,	64,	64,
	64,	64,	64,	64,	64,	72,	80,	88,
	96,	104,112,120,128,128,128,128,
};

static B3dMatrix Permut1 = {
	0.0, 1.0, 0.0,
	0.0, 0.0, 1.0,
	1.0, 0.0, 0.0,
};

static B3dMatrix Permut2 = {
	0.0, 0.0, 1.0,
	1.0, 0.0, 0.0,
	0.0, 1.0, 0.0,
};

#define	TestCode(x)		(theKey.key_states[x>>3]&(128>>(x&7)))

B3dView::B3dView(char *name, BRect frame, B3dView *view) :
BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW|B_FRAME_EVENTS) {
	if (view == 0L) {
		myWorld = new B3dEthericWorld("Default");
		myCamera = new B3dCamera(name, myWorld, B_RADIAL_LENS, B_STANDARD_LIGHT);
	}
	else {
		myWorld = view->World();
		myCamera = new B3dCamera(name, myWorld,
								 view->myCamera->ProjectorMode(),
								 view->myCamera->LighterMode());
	}
	camera_id = myWorld->AddCamera(myCamera);
	myRenderer = new B3dRenderer(myCamera->Lighter());
	myBuffer = 0L;
	CheckBuffer();
// default keyboard settings
	cfg.Bas = 0x38;
	cfg.Haut = 0x59;
	cfg.Droit = 0x4A;
	cfg.Gauche = 0x48;		
	cfg.Bouton[0] = 0x61;
	cfg.Bouton[1] = 0x63;
	cfg.Bouton[2] = 0x56;
	cfg.Bouton[3] = 0x01;
	cfg.Tempo[0] = 0L;
	cfg.Tempo[1] = 0L;
	cfg.Tempo[2] = 0L;
	cfg.Tempo[3] = 0L;
// create new thread
	new_width = 0.0;
	new_height = 0.0;
}

void B3dView::SetControlKeys(char *keys) {
	cfg.Bas = keys[0];
	cfg.Haut = keys[1];
	cfg.Droit = keys[2];
	cfg.Gauche = keys[3];		
	cfg.Bouton[0] = keys[4];
	cfg.Bouton[1] = keys[5];
	cfg.Bouton[2] = keys[6];
	cfg.Bouton[3] = keys[7];
}

B3dView::~B3dView() {
	long        i;
	
	delete myRenderer;
	delete myBuffer;
	myWorld->RemoveCamera(camera_id);
	delete myCamera;
	if (myWorld->CameraCount() == 0)
		delete myWorld;
}

void B3dView::AttachedToWindow() {
}

void B3dView::Draw(BRect updateRect) {
	BRect         src;
	
	src.left = updateRect.left;
	src.right = updateRect.right;
	src.top = updateRect.top+1.0;
	src.bottom = updateRect.bottom+1.0;
	DrawBitmap(myBuffer, src, updateRect);
#if 0
	screen_info   the_info;
	BRect         src;
	
	get_screen_info(0, &the_info);
	if (myBuffer != 0L)
		if (myBuffer->ColorSpace() != the_info.mode) {
			new_width = Bounds().Width();
			new_height = Bounds().Height();
		}
	src.left = updateRect.left;
	src.right = updateRect.right;
	src.top = updateRect.top+1.0;
	src.bottom = updateRect.bottom+1.0;
	DrawBitmap(myBuffer, src, updateRect);
#endif
}

void B3dView::MouseDown(BPoint point) {
	ulong          buttons;
	B3dMatrix      rot, rot0, rot1;
	B3dTouchDesc   touch;

//	buttons = Window()->CurrentMessage()->FindLong("buttons");
	buttons = Window()->CurrentMessage()->FindInt32("buttons");
	if (myCamera->GetTouchDesc((long)point.x, (long)point.y, &touch)) {
		((Z3dApplication*)be_app)->mapping_stable = false;
		rot0 = touch.obj->rotation;
		touch.obj->highlightColor.Set(0.4, 0.4, 0.4, 0.6);
		Window()->Unlock();
		while (buttons) {
			((Z3dApplication*)be_app)->mapping_stable = false;
			Window()->Lock();
			GetMouse(&point, &buttons, FALSE);
			Window()->Unlock();
			myCamera->CalcTouchDesc((long)point.x, (long)point.y, 0L, 0L, &touch, &rot);
			rot1 = rot*rot0;
			touch.obj->MoveTo(&touch.obj->origin, &rot1);
			if (!buttons)
				touch.obj->highlightColor.Set(0.0, 0.0, 0.0, 1.0);
			snooze(4e4);
		}
		Window()->Lock();
	}
	MakeFocus();
}

void B3dView::FrameResized(float width, float height) {
	new_width = width;
	new_height = height;
}

bool B3dView::CheckBuffer() {
	BRect         bound;
	
	bound = Bounds();
	bound.OffsetTo(0.0, 0.0);
	if (myBuffer != 0L) {
		if ((myBuffer->ColorSpace() != B_COLOR_8_BIT) ||
			(bound != myBuffer->Bounds()))
			delete myBuffer;
		else
			return FALSE;
	}
	bound.top -= 1.0;
	bound.bottom += 1.0;
	myBuffer = new BBitmap(bound, B_COLOR_8_BIT);
	bound.top += 1.0;
	bound.bottom -= 1.0;
	myCamera->SetFrame(bound.left, bound.right, bound.top, bound.bottom);
	myRenderer->SetBuffer(myBuffer->Bits(),
						  myBuffer->BytesPerRow(),
						  myBuffer->ColorSpace(),
						  myBuffer->Bounds().Width(),
						  myBuffer->Bounds().Height());
	return TRUE;
#if 0
	BRect         bound;
	screen_info   the_info;
	
	bound = Bounds();
	bound.OffsetTo(0.0, 0.0);
	get_screen_info(0, &the_info);
	if (myBuffer != 0L) {
		if ((myBuffer->ColorSpace() != the_info.mode) ||
			(bound != myBuffer->Bounds()))
			delete myBuffer;
		else
			return FALSE;
	}
	bound.top -= 1.0;
	bound.bottom += 1.0;
	myBuffer = new BBitmap(bound, the_info.mode);
	bound.top += 1.0;
	bound.bottom -= 1.0;
	myCamera->SetFrame(bound.left, bound.right, bound.top, bound.bottom);
	myRenderer->SetBuffer(myBuffer->Bits(),
						  myBuffer->BytesPerRow(),
						  myBuffer->ColorSpace(),
						  myBuffer->Bounds().Width(),
						  myBuffer->Bounds().Height());
	return TRUE;
#endif
}

void B3dView::Enable(bool active) {
	myWorld->Lock();
	myWorld->CameraOn(camera_id, active);
	myWorld->Unlock();
}

void B3dView::MoveCamera() {
	int				i;
	char            button[4];
	long			duree, deltaV, deltaH, Tempo;
	key_info		theKey;
	B3dMatrix       rot;
	B3dKeyConfig	*Conf;

	if (!Window()->IsActive()) return;
	Tempo = system_time()*1e-3;
	Conf = &cfg;
//	GetKeys(&theKey,FALSE);
	get_key_info(&theKey);
	deltaV = 0;
    deltaH = 0;
	if (TestCode(Conf->Bas)!=0) {
		if (Conf->Tempo[0] == 0L)
			Conf->Tempo[0] = Tempo;
		if ((duree = (Tempo-Conf->Tempo[0])>>7) > 23) duree = 23;
		deltaV -= DureeToVitesse[duree];
	}
	else Conf->Tempo[0] = 0L;
	if (TestCode(Conf->Haut)!=0) {
		if (Conf->Tempo[1] == 0L)
			Conf->Tempo[1] = Tempo;
		if ((duree = (Tempo-Conf->Tempo[1])>>7) > 23) duree = 23;
		deltaV += DureeToVitesse[duree];
	}
	else Conf->Tempo[1] = 0L;
	if (TestCode(Conf->Gauche)!=0) {
		if (Conf->Tempo[2] == 0L)
			Conf->Tempo[2] = Tempo;
		if ((duree = (Tempo-Conf->Tempo[2])>>7) > 23) duree = 23;
		deltaH -= DureeToVitesse[duree];
	}
	else Conf->Tempo[2] = 0L;
	if (TestCode(Conf->Droit)!=0) {
		if (Conf->Tempo[3] == 0L)
			Conf->Tempo[3] = Tempo;
		if ((duree = (Tempo-Conf->Tempo[3])>>7) > 23) duree = 23;
		deltaH += DureeToVitesse[duree];
	}
	else Conf->Tempo[3] = 0L;
	for (i=0;i<4;i++) {
		if (TestCode(Conf->Bouton[i])!=0)
			button[i] = 1;
		else
			button[i] = 0;
	}
	if ((deltaH != 0) || (deltaV != 0)) {
		rot.Set(((float)deltaH)*0.001,((float)deltaV)*0.001,0.0);
		rot = Permut2*rot*Permut1;
		myCamera->rotation *= rot;
	}
	if (button[0])
		myCamera->origin += myCamera->rotation.Z()*0.15;
	if (button[1])
		myCamera->origin -= myCamera->rotation.Z()*0.15;
	if (button[2])
		myCamera->LookAt(myWorld->GetThing(0L));
}

void B3dView::Stop() {
}












