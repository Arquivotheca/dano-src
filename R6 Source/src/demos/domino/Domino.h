//*****************************************************************************
//
//	File:		 Domino.h
//
//	Description: Application class for domino game demo, headers.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

#ifndef __Domino
#define __Domino

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif

#include "DomWindow.h"

// pointer to the texture buffer.
extern uchar   *bitmap;

class DominoApp : public BApplication {
// WindowScreen for the dominos
	DomWindow  *window;
	
public:
					DominoApp ();
		void		ReadyToRun ();
};

#endif










