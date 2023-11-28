/*
	
	3dBookView.h
	
	Copyright 1996 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _3D_BOOK_VIEW_H
#define _3D_BOOK_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _3D_BOOK_WORLD_H
#include "3dBookWorld.h"
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _3D_CAMERA_H
#include "3dCamera.h"
#endif
#ifndef _3D_RADIAL_LENS_H
#include "3dRadialLens.h"
#endif
#ifndef _3D_LIGHTER_H
#include "3dLighter.h"
#endif
#ifndef _3D_RENDERER_H
#include "3dRenderer.h"
#endif

typedef struct {
	char		Haut;
	char		Bas;
	char		Gauche;
	char		Droit;		
	char		Bouton[4];
	long 		Tempo[4];
} B3dBookKeyConfig;

class B3dPage;

class B3dBookView : public BView {
	friend long draw_thread2(void *data);
 public:
	B3dBookView(BRect frame, char *name);
	virtual ~B3dBookView();
	virtual void     AttachedToWindow(void);
	virtual	void	 Draw(BRect updateRect);
	virtual void     MouseDown(BPoint point);
	virtual void     FrameResized(float width, float height);
	inline B3dWorld  *World();
	inline B3dCamera *Camera();
	void             Enable(bool yn);
	void             SetControlKeys(char *keys);
	virtual void     Stop();
	virtual void	 MessageReceived(BMessage *message);
	virtual	void	 KeyDown(const char *bytes, int32 numBytes);
	void             SetBook();
	
	float     updateTiming;
	long      myIndex;
	sem_id    kill_sem;
	thread_id draw_frame;

	bool      drag;
	bool      visible;
	float     alpha;
	float     inertia;
	float     traction;
	map_ref   bitmap[4];
	map_ref   numbers[9];
	long      index[4];
	float     elevation;
	float     direction;
	float     zoom;
	BBitmap   *TextNumber;
	void      *base_number;
	
 private:
	B3dWorld         *myWorld;
	B3dCamera        *myCamera;
	long             camera_id;
	BBitmap          *myBuffer;
	B3dRenderer      *myRenderer;
	float            new_width;
	float            new_height;
	B3dPage          *pages[3];
	B3dBookKeyConfig cfg;
	key_info		 MyKey;
	
	bool             CheckBuffer();
	void             MyMoveCamera();
};

B3dWorld *B3dBookView::World() {
	return myWorld;
}

B3dCamera *B3dBookView::Camera() {
	return myCamera;
}

#endif












