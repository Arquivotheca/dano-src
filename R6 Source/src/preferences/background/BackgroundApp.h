//*****************************************************************************
//
//	File:		 BackgroundApp.h
//
//	Description: Application header for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#if ! defined BACKGROUNDAPP_INCLUDED
#define BACKGROUNDAPP_INCLUDED

#include <Application.h>

class BackgroundApp : public BApplication
{
public:
			BackgroundApp(const char *sig);
	void	ReadyToRun();
	void	RefsReceived(BMessage *refs);
};

#endif
