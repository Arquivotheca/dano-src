//========================================================================
//	MKeyIcons.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MKEYICONS_H
#define _MKEYICONS_H

#include "MList.h"

#include <InterfaceDefs.h>

struct KeyBinding;
class MKeyBindingManager;
class BView;
class BBitmap;

class MKeyIcons {
public:

	static void						DrawKeyBinding(
										BView*		inView,
										BRect		inArea,
										KeyBinding&	inBinding,
										const MKeyBindingManager&	inManager,
										alignment	inAlignment = B_ALIGN_LEFT);
	static float					DrawKeyBinding(
										BView*		inView,
										BRect		inArea,
										KeyBinding&	inBinding,
										alignment	inAlignment = B_ALIGN_LEFT);
	static void						InitBitmaps();

private:

	static MList<BBitmap*>		sBitmapList;
	static bool					sBitmapInited;
};

#endif
