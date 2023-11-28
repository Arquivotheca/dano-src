/*************************************************************************
/
/	MinitelApp.h
/
/	Written by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#ifndef MINITEL_APP
#define MINITEL_APP

#include <Application.h>


//========================================================================

class MinitelApp : public BApplication
{
	public:
							MinitelApp			();
		void				RefsReceived		(BMessage *message);
};
#endif
