//*****************************************************************************
//
//	File:		 3dmovWindow.h
//
//	Description: Window object used to include 3dView demo of the 3d kit.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

#ifndef _3D_MOV_WINDOW_H
#define _3D_MOV_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _3D_MOV_VIEW_H
#include "3dmovView.h"
#endif
#ifndef _3D_BOOK_VIEW_H
#include "3dBookView.h"
#endif

#include <InterfaceKit.h>

class Z3dWindow : public BWindow {
	public:
		Z3dWindow(long index); 
		long		myIndex;
		BView	*myView;
		virtual bool	QuitRequested();
// BACO - added menu stuff here
		BMenuBar	*mBar;
		BMenuItem	*mItems[5];
};

#endif











