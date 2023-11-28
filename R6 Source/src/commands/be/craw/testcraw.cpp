//******************************************************************************
//
//	File:		main.cpp
//
//	Description:	ConvertRaw application.
//
//	Written by:	Steve Horowitz
//
//	Copyright 1993, Be Incorporated
//
//******************************************************************************

#include "stdio.h"
#include "string.h"
#include <Debug.h>

#include "InterfaceDefs.h"
#include "ResourceFile.h"
#include "Directory.h"
#include "Volume.h"
#include "Application.h"
#include "Bitmap.h"
#include "Window.h"
#include "View.h"


#include "iconfile"

/*------------------------------------------------------------*/

main(int argc, char* argv[])
{
	BBitmap*		bmap;
	long			bitslength;
	long			size;
	BResourceFile	resfile;
	BDirectory		dir;
	void*			bits;

	BApplication* app = new BApplication('CRW2');

	BRect wRect(0, 0, 139, 137);
	BRect wBox = wRect;
	
	bitslength = ((wBox.Width() + 1)) * (wBox.Height() + 1);
	wRect.OffsetBy(100, 100);

	BWindow* convertWindow = new BWindow(wRect, "IIConversion", B_TITLED_WINDOW, 0);
	BView* convertView = new BView(wBox, "ConvertRawView", B_FOLLOW_LEFT, B_WILL_DRAW);
	convertWindow->AddChild(convertView);
	convertWindow->Show();

	bmap = new BBitmap(wBox, B_RGB_32_BIT);
/*
	boot_volume().GetRootDirectory(&dir);
	dir.GetFile("icon_res", &resfile);
	resfile.Open(B_READ_ONLY);
	bits = resfile.FindResource('PICT', "PICT", &size);
*/
	
	bmap->SetBits(test2, bitslength, 0, B_COLOR_8_BIT);
//	resfile.Close();

	// draw converted bitmap
	convertWindow->Lock();
	convertView->DrawBitmap(bmap, BPoint(0, 0));
	convertWindow->Unlock();

	snooze(8000000);	

	delete bmap;
	delete app;
}
