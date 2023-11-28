/*
	
	3dView.h
	
	Copyright 1996 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _3D_VIEW_H
#define _3D_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _3D_ETHERIC_WORLD_H
#include "3dEthericWorld.h"
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
} B3dKeyConfig;

class B3dView : public BView {
	friend long draw_thread(void *data);
 public:
	B3dView(char *name, BRect frame, B3dView *view = 0L);
	virtual ~B3dView();
	virtual void     AttachedToWindow(void);
	virtual	void	 Draw(BRect updateRect);
	virtual void     MouseDown(BPoint point);
	virtual void     FrameResized(float width, float height);
	inline B3dWorld  *World();
	inline B3dCamera *Camera();
	void             Enable(bool yn);
	void             SetControlKeys(char *keys);
	virtual void     Stop();
 protected:
	B3dWorld        *myWorld;
	B3dCamera       *myCamera;
	long            camera_id;
	BBitmap         *myBuffer;
	B3dRenderer     *myRenderer;
	float           new_width;
	float           new_height;
	B3dKeyConfig    cfg;
 private:
	bool            CheckBuffer();
	void            MoveCamera();
};

B3dWorld *B3dView::World() {
	return myWorld;
}

B3dCamera *B3dView::Camera() {
	return myCamera;
}

#endif












