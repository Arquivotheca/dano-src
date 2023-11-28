//******************************************************************************
//
//	File:			IApp.h
//
//	Description:	Be Installer application header.
//
//	Written by:		Steve Horowitz
//
//	Copyright 1994, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#ifndef IAPP_H
#define IAPP_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef ENGINE_H
#include "Engine.h"
#endif

class TIApp : public BApplication {


public:
					TIApp();
virtual				~TIApp();
virtual	void		ReadyToRun();
virtual bool		QuitRequested();
virtual	void		ArgvReceived(int32 argc, char **argv);
virtual void		MessageReceived(BMessage *);

private:
		TEngine*	fEngine;
		typedef BApplication inherited;
};

#endif
