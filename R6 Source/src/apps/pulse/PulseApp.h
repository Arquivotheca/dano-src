//****************************************************************************************
//
//	File:		PulseApp.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef PULSEAPP_H
#define PULSEAPP_H

#include <app/Application.h>
#include "Prefs.h"

extern "C" _EXPORT BView* instantiate_deskbar_item();
bool LastEnabledCPU(int my_cpu);
int GetMinimumViewWidth();
bool LoadInDeskbar();
void Usage();

class PulseApp : public BApplication {
	public:
		PulseApp(int argc, char **argv);
		~PulseApp();
		
		Prefs *prefs;

	private:
		void BuildPulse(bool quit_if_deskbar_present);
};

#endif
