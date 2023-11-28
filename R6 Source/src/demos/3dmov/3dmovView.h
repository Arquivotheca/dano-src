//*****************************************************************************
//
//	File:		 3dmovView.h
//
//	Description: 3d demo constructor using the 3d Kit.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

#ifndef _3D_MOV_VIEW_H
#define _3D_MOV_VIEW_H

#ifndef _3D_VIEW_H
#include "3dView.h"
#endif
#ifndef _3D_LOOK_H
#include "3dLook.h"
#endif

class B3dStamp;

class Z3dView : public B3dView {

public:
float     updateTiming;
long      myIndex;
sem_id    kill_sem;
thread_id draw_frame;
B3dStamp  *pulse;

Z3dView(BRect frame, char *name, long index);
	friend long draw_thread(void *data);
public:
	~Z3dView();
	virtual void Stop();
	virtual void AttachedToWindow(void);
	virtual void MessageReceived(BMessage *message);
	virtual	void KeyDown(const char *bytes, int32 numBytes);
private:
	void 		MyMoveCamera();
	key_info	MyKey;
};

#endif






