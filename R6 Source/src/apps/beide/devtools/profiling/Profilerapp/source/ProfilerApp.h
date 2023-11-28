#pragma syspath_once on
#pragma once on
#include <iostream>
#include <fstream>
#include "MWAbout.h"
#include "MWWindow.h"
#include "MWAboutView.h"
#include "MWListItem.h"

BFilePanel	*OpenDialog;
bool		AboutBox=false;
class ProfilerApp: public BApplication
{
	public:
						ProfilerApp();
		virtual bool	QuitRequested();
		virtual void	ReadyToRun();
		virtual void 	ArgvReceived(int32 argc, char **argv);
		virtual void 	RefsReceived(BMessage* message );
		virtual void 	MessageReceived(BMessage* message);
	private:
		bool fArgvCalled;
};
