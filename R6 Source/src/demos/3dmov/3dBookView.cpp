/*
	
	3dView.cpp
	
	Copyright 1996 Be Incorporated, All Rights Reserved.
	
*/

#include <InterfaceDefs.h>
#include <Entry.h>

#ifndef _3D_BOOK_VIEW_H
#include "3dBookView.h"
#endif
#ifndef _3D_RADIAL_LENS_H
#include "3dRadialLens.h"
#endif
#ifndef _3D_WORLD_H
#include "3dWorld.h"
#endif
#ifndef _3D_CUBE_H
#include "3dCube.h"
#endif
#ifndef _3D_SPHERE_H
#include "3dSphere.h"
#endif
#ifndef _3D_SPIN_H
#include "3dSpin.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif
#ifndef _3D_MOV_APP_H
#include "3dmovApp.h"
#endif
#ifndef _3D_PAGE_H
#include "3dPage.h"
#endif

#include <Debug.h>

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

#define	TestCode(key, x)		(key.key_states[x>>3]&(128>>(x&7)))

long draw_thread2(void *data);

B3dBookView::B3dBookView(BRect frame, char *name) :
BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW|B_FRAME_EVENTS) {
	myWorld = new B3dBookWorld("Default");
	myCamera = new B3dCamera(name, myWorld, B_RADIAL_LENS, B_STANDARD_LIGHT);

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

void B3dBookView::SetControlKeys(char *keys) {
	cfg.Bas = keys[0];
	cfg.Haut = keys[1];
	cfg.Droit = keys[2];
	cfg.Gauche = keys[3];		
	cfg.Bouton[0] = keys[4];
	cfg.Bouton[1] = keys[5];
	cfg.Bouton[2] = keys[6];
	cfg.Bouton[3] = keys[7];
}

B3dBookView::~B3dBookView() {
	long        i;
	
	acquire_sem(kill_sem);
	delete_sem(kill_sem);

	delete TextNumber;
	delete myRenderer;
	delete myBuffer;
	myWorld->RemoveCamera(camera_id);
	delete myCamera;
	if (myWorld->CameraCount() == 0)
		delete myWorld;
}

void B3dBookView::AttachedToWindow() {
	int               i;
	BFont             *font;
	BView             *view;
	BRect             rect;
	BPoint            point;
	B3dVector         origin, axis;
	B3dMatrix         rotation;
	RGBAColor         Color;
	B3dAmbientLight   *aLight;
	B3dParallelLight  *dLight;
//	BFont			fnt;
	
	pages[0] = new B3dPage("Page0", 1.0, 12, B_FRONT_FACES|B_REAR_FACES|B_SMOOTH_MODEL);
	rotation.Set(0.0, 0.7, 3.14);
	origin.Set(0.0, 0.0, 0.0);
	pages[0]->MoveTo(&origin,&rotation);
	pages[0]->SetMapping(bitmap+1, numbers+2);
	World()->AddThing(pages[0], B_STATIC_THING);

	pages[1] = new B3dPage("Page1", 1.0, 12, B_FRONT_FACES|B_REAR_FACES|B_SMOOTH_MODEL);
	rotation.Set(0.0, 0.7, 3.14);
	origin.Set(0.0, 0.0, 0.0);
	pages[1]->MoveTo(&origin,&rotation);
	pages[1]->SetMapping(bitmap+2, numbers+4);
	((B3dPageModel*)(pages[1]->desc.look->model))->SetShape(-1.0, 0.0, 0.0);
	World()->AddThing(pages[1], B_STATIC_THING);

	pages[2] = new B3dPage("Page2", 1.0, 12, B_FRONT_FACES|B_REAR_FACES|B_SMOOTH_MODEL);
	rotation.Set(0.0, 0.7, 3.14);
	origin.Set(0.0, 0.0, 0.0);
	pages[2]->MoveTo(&origin,&rotation);
	pages[2]->SetMapping(bitmap+0, numbers+0);
	World()->AddThing(pages[2], B_STATIC_THING);

	Color.Set(1.0, 1.0, 1.0, 0.0);
	aLight = new B3dAmbientLight("Ambient white", 0.4, &Color);
	World()->AddLight(aLight);

	axis.Set(0.43, 0.8, 0.42);
	dLight = new B3dParallelLight("Powerful white", 0.7, &Color, &axis);
	World()->AddLight(dLight);

	Camera()->MoveTo(-1.7, 0.0, 0.0);
	Camera()->LookAt(pages[0]);
	rotation.Set(0.0, -0.31, 0.0);
	rotation = Permut2*rotation*Permut1;
	myCamera->rotation *= rotation;

    rect.Set(0.0, 0.0, 127.0, 23.0);
	TextNumber = new BBitmap(rect, B_RGB_32_BIT, TRUE);
	view = new BView(TextNumber->Bounds(), "", 0L, 0L);
	TextNumber->Lock();
	TextNumber->AddChild(view);
	view->SetLowColor(255.0, 255.0, 255.0, 255.0);
	view->SetHighColor(0.0, 0.0, 0.0, 0.0);
	view->FillRect(rect, B_SOLID_LOW);
//	view->SetFontName("Courier New Bold");
//	fnt.SetFamilyAndStyle("Courier 10 BT", "Roman");
	font = new BFont(be_plain_font);
	font->SetFlags(B_DISABLE_ANTIALIASING);
	font->SetSize(18.0);
	view->SetFont(font);
	delete font;
	point.Set(4.0, 14.0);
	view->DrawChar('-', point);
	point.x += 16.0;
	view->DrawChar('1', point);
	point.x += 16.0;
	view->DrawChar('2', point);
	point.x += 16.0;
	view->DrawChar('3', point);
	point.x += 16.0;
	view->DrawChar('4', point);
	TextNumber->RemoveChild(view);
	TextNumber->Unlock();
	delete view;
	
	drag = FALSE;
	visible = FALSE;
	alpha = 1.0;
	inertia = 0.0;
	traction = 0.0;
	elevation = 0.7;
	direction = 0.0;
	zoom = 450.0;

	index[0] = 9;
	index[1] = 11;
	index[2] = 12;
	index[3] = 10;

	for (i=0; i<4; i++) {
		bitmap[i].size_h = 8;
		bitmap[i].size_v = 8;
	}
	
	base_number = TextNumber->Bits();
	for (i=0; i<9; i++) {
		numbers[i].buf = base_number;
		numbers[i].size_h = 7;
		numbers[i].size_v = 4;
	}
	
	SetBook();

	updateTiming = 5e4;
	myIndex = 2;
	kill_sem = create_sem(0, "Kill 3dmovView");
	draw_frame = spawn_thread((thread_entry)draw_thread2,
							  "3d draw thread2",
							  B_NORMAL_PRIORITY,
							  (void*)this);
	resume_thread(draw_frame);
	
	MakeFocus();
}

void B3dBookView::SetBook() {
	int               i, temp;
	bool              new_visi;
	float             alpha0;
	float			  inertia2, traction2, alpha2;
	B3dVector         origin;
	B3dMatrix         rotation;
	B3dRadialOptions  PPOptions;

	rotation.Set(direction, elevation, 3.1416);
	new_visi = ((alpha != 1.0) && (alpha != -1.0)) ||
		(inertia != 0.0) || (traction != 0.0) || drag;
	if (!new_visi)
		origin.Set(-100.0, 0.0, 0.0);
	else {
		origin.Set(0.0, 0.0, 0.0);
		((B3dPageModel*)(pages[0]->desc.look->model))->SetShape(alpha, inertia, traction);
	}

	if (visible && !new_visi) {
		if (alpha == 1.0) {
			temp = index[0];
			index[0] = index[1];
			index[1] = temp;
		}
		else {
			temp = index[2];
			index[2] = index[3];
			index[3] = temp;
		}
	}
	visible = new_visi;
	
	pages[0]->MoveTo(&origin,&rotation);

	origin.Set(0.0, 0.0, 0.0);
	pages[1]->MoveTo(&origin,&rotation);
	pages[2]->MoveTo(&origin,&rotation);
	
	Camera()->Lens()->GetOptions(&PPOptions);
	PPOptions.zoom = zoom;
	Camera()->Lens()->SetOptions(&PPOptions);

	inertia2 = inertia;
	traction2 = traction;
	alpha2 = alpha;

	if (inertia > 0.04)
		inertia -= 0.04+(inertia-0.04)*0.2;
	else if (inertia < -0.04)
		inertia += 0.04+(0.04-inertia)*0.2;
	else
		inertia = 0.0;

	alpha0 = alpha;
    alpha += alpha*0.05;
	if (alpha > 1.0)
		alpha = 1.0;
	else if (alpha < -1.0)
		alpha = -1.0;
	inertia += (alpha0-alpha)*1.0;
	if (inertia > 1.0)
		inertia = 1.0;
	else if (inertia < -1.0)
		inertia = -1.0;
	
	if (traction > 0.04)
		traction -= 0.04+(traction-0.04)*0.2;
	else if (traction < -0.04)
		traction += 0.04+(0.04-traction)*0.2;
	else
		traction = 0.0;

 	if ((alpha != alpha2) || (inertia != inertia2) || (traction != traction2))
		((Z3dApplication*)be_app)->book_stable = false;
	else
		((Z3dApplication*)be_app)->book_stable = true;

   for (i=0; i<4; i++) {
		bitmap[i].buf = ((Z3dApplication*)be_app)->map_ref_list[index[i]].buf;
		numbers[1+2*i].buf = (void*)((ulong)base_number+4*16*(index[i]-8));
	}
}

void B3dBookView::Draw(BRect updateRect) {
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

void B3dBookView::MouseDown(BPoint point) {
	int            mode, count, temp;
	float          dir0, elev0, alpha0, move, tract;
	ulong          buttons;
	BPoint         point2;
	B3dTouchDesc   touch;

//	buttons = Window()->CurrentMessage()->FindLong("buttons");
	buttons = Window()->CurrentMessage()->FindInt32("buttons");
	if (myCamera->GetTouchDesc((long)point.x, (long)point.y, &touch)) {
		((Z3dApplication*)be_app)->mapping_stable = false;
		count = ((B3dFaceModel*)(((B3dBody*)touch.obj)->desc.look->model))->faceCount;
		if (touch.index_face < (count/3)) {
			mode = 0;
			dir0 = direction;
			elev0 = elevation;
		}
		else if (touch.index_face > (2*count/3)) {
			if ((touch.obj != pages[0]) && visible)
				mode = 2;
			else {
				drag = TRUE;
				if (touch.obj == pages[1]) {
					alpha = -1.0;
					temp = index[2];
					index[2] = index[3];
					index[3] = temp;
				}
				else if (touch.obj == pages[2]) {
					alpha = 1.0;
					temp = index[0];
					index[0] = index[1];
					index[1] = temp;
				}
				alpha0 = alpha;
				mode = 1;
				tract = (2.0/47.0)*(23.5-(float)(touch.index_face % 48));
			}
		}
		else
			mode = 2;
		Window()->Unlock();
		while (buttons) {
			((Z3dApplication*)be_app)->mapping_stable = false;
			Window()->Lock();
			GetMouse(&point2, &buttons, FALSE);
			Window()->Unlock();
			if (mode == 0) {
				direction = (dir0+(point.x-point2.x)/(0.4*zoom));
				elevation = (elev0+(point.y-point2.y)/(0.32*zoom));
				if (direction > 0.3)
					direction = 0.3;
				else if (direction < -0.3)
					direction = -0.3;
				if (elevation > 1.2)
					elevation = 1.2;
				else if (elevation < 0.3)
					elevation = 0.3;
			}
			else if (mode == 1) {
				move = (point.x-point2.x)*b_cos(alpha*1.5708)+
					   (point2.y-point.y)*b_sin(alpha*1.5708);
				alpha = alpha0 + move/(0.54*zoom);
				if (alpha > 1.0)
					alpha = 1.0;
				else if (alpha < -1.0)
					alpha = -1.0;
				inertia += (alpha0-alpha);
				if (inertia > 1.0)
					inertia = 1.0;
				else if (inertia < -1.0)
					inertia = -1.0;
				alpha0 = alpha;
				point = point2;
				traction = tract;
			}
			snooze(40000);
		}
		drag = FALSE;
		Window()->Lock();
	}
	MakeFocus();
}

void B3dBookView::FrameResized(float width, float height) {
	new_width = width;
	new_height = height;
}

bool B3dBookView::CheckBuffer() {
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

void B3dBookView::Enable(bool active) {
	myWorld->Lock();
	myWorld->CameraOn(camera_id, active);
	myWorld->Unlock();
}

void B3dBookView::MyMoveCamera() {
	int				i;
	char            button[2];

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
		zoom *= 1.04;
	if (button[1])
	    zoom *= 0.96;
	if (zoom > 1000.0)
		zoom = 1000.0;
	if (zoom < 50.0)
		zoom = 50.0;
}


void B3dBookView::MessageReceived(BMessage *msg) {
	long		 ind;
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
					if (touch.obj == pages[0]) {
						if (touch.index_face&2)
							ind = index[2];
						else
							ind = index[1];
					}
					else if (touch.obj == pages[1])
						ind = index[3];
					else
						ind = index[0];
					((Z3dApplication*)be_app)->SelectVideoChannel(ind, tref);
				}
			}
			else delete tref; // memory leak potential here!!!
		}
		break;
	default:
		BView::MessageReceived(msg);
	}
}

void B3dBookView::Stop() {
	draw_frame = -1;
}

void B3dBookView::KeyDown(const char *bytes, int32 numBytes) {
	((Z3dApplication*)be_app)->mapping_stable = false;
}

long draw_thread2(void *data) {
	int			i, dbg=0;
	BRect		bound;
	B3dBookView	*myView;
	B3dWorld		*world;
	B3dEraseOptions	opt;

	myView = (B3dBookView*)data;
	while (myView->Window() == 0L)
		snooze(50000);
	world = (B3dWorld*)myView->myWorld;
	opt.color.Set(0.0, 0.0, 0.0, 0.0);
	((Z3dApplication*)be_app)->EnableSync();
	while (TRUE) {
		world->Update();
		((Z3dApplication*)be_app)->Sync();
		myView->myRenderer->EraseBuffer(&opt);
		myView->SetBook();
		world->Lock();
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
