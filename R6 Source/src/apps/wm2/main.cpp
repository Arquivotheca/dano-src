//******************************************************************************
//
//	File:			main.cpp
//
//	Description:	Well, the glue.
//	
//	Written by:		Mathias Agopian
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <content2/Content.h>
#include <support2/StdIO.h>
#include <support2/SupportDefs.h>
#include <support2/Looper.h>
#include <interface2/View.h>

#include "wm.h"

using namespace B::Content2;
using namespace B::Support2;
using namespace B::Interface2;


// ********************************************************************

B::Support2::IBinder::ptr
root(const B::Support2::BValue &/*params*/)
{
	WindowManager *wm = new WindowManager();	
	return static_cast<BContent *>(wm);
}

int main(int , char **)
{
	BValue v;
	BLooper::SetRootObject(root(v));
	return BLooper::Loop();
}

