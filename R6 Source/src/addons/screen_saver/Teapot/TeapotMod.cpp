#include "TeapotMod.h"
#include "ObjectView.h"
#include <InterfaceKit.h>
#include <stdio.h>
#include <StringView.h>
#include <stdlib.h>
#include <string.h>

#define STEPS 50

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Teapot(message, image);
}

void Teapot::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "Teapot: a BDirectWindow test"));
}

status_t Teapot::StartSaver(BView *v, bool preview)
{
	if(preview)
	{
		bgl = 0;
		return B_ERROR;
	}
	else
	{
		GLenum type = BGL_RGB | BGL_DEPTH | BGL_DOUBLE;
	
		SetTickSize(1000);
	
		bgl = new ObjectView(v->Bounds(), "objectView", B_FOLLOW_NONE, type);
		v->AddChild(bgl);
	
		bgl->LockGL();
		glDrawBuffer(GL_FRONT);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawBuffer(GL_BACK);
		bgl->UnlockGL();
		return B_OK;
	}
}

void Teapot::StopSaver()
{
	if(bgl)
		bgl->EnableDirectMode(false);
}

void Teapot::DirectConnected(direct_buffer_info *info)
{
	bgl->DirectConnected(info);
	bgl->EnableDirectMode(true);
}

void Teapot::DirectDraw(int32)
{
//	bgl->Pulse();
}

Teapot::Teapot(BMessage *message, image_id id)
 : BScreenSaver(message, id)
{
}
