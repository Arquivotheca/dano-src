//******************************************************************************
//
//	File:			craw.cpp
//
//	Description:	ConvertRaw application.
//
//	Written by:		Steve Horowitz, Eric Knight
//
//	Copyright 1993-96, Be Incorporated
//
//******************************************************************************

#define FULL_MASK	0

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Debug.h>
#include <OS.h>
#include <Application.h>
#include <View.h>
#include <Window.h>
#include <Bitmap.h>
#include <Resources.h>
#include <VolumeRoster.h>
#include <Entry.h>
#include <Directory.h>
#include <Volume.h>
#include <Screen.h>

class TCrawApp : public BApplication {

friend class TStatusView;

public:
		 			TCrawApp();
virtual	void		ArgvReceived(int32 argc, char** argv);
virtual void		ReadyToRun();
};

/*------------------------------------------------------------*/

void MakeTransparent(uchar* pixmap_shared, uchar* the_mask, BRect*);
void TransLargeIcon(uchar* pixmap_shared, uchar* the_mask);
void TransSmallIcon(uchar* pixmap_shared, uchar* the_mask);

/*------------------------------------------------------------*/
int
main()
{
	TCrawApp*		app;

	app = new TCrawApp();
	app->Run();
	delete(app);

	return 0;
}

// ---------------------------------------------------------------

TCrawApp::TCrawApp()
		 :BApplication("application/x-vnd.Be-cmd-CRAW")
{
}

// ---------------------------------------------------------------

void TCrawApp::ReadyToRun()
{
	Quit();
}

/*------------------------------------------------------------*/

void TCrawApp::ArgvReceived(int32 argc, char** argv)
{
	BRect			bitsRect;
	BRect			windRect;
	char			maskName[32];
	char			resName[5];
	const char*		name;
	uchar*			bits_buf;
	uchar*			base;
	BBitmap*		beMap;
	long			buf_size;
	FILE*			maskFile;
	FILE*			outFile;
	FILE*			inFile;
	BResources		resFile;
	BFile			file;
	BEntry			entry;
	int32			id;
	size_t			size;
	long			bits_length;
	long			mask_row_bytes;
	long			i;
	type_code			resType;
	long			bits_per_pixel;
	color_space		src_space;
	color_space		dest_space;
	bool			resCreate = FALSE;
	BScreen			screen( B_MAIN_SCREEN_ID );
	

	if (argc < 4) {
		printf("craw: wrong number of args.\ncraw w h filename\n");
		exit(1);
	}

	if ((argc == 5)  && (strcmp(argv[4], "res") == 0))
		resCreate = TRUE;

	// Read in the rectangle
	bitsRect.Set(0, 0, atoi(argv[1]) - 1, atoi(argv[2]) - 1);

	if ((argc == 5)  && (strcmp(argv[4], "mono") == 0)) {
		src_space = B_MONOCHROME_1_BIT;
		dest_space = B_MONOCHROME_1_BIT;
		buf_size = ((bitsRect.Width() + 1) / 8) * (bitsRect.Height() + 1);
	} else {
		src_space = B_RGB_32_BIT;
		dest_space = screen.ColorSpace();
		buf_size = ((bitsRect.Width() + 1) * (bitsRect.Height() + 1)) * 3;
		PRINT(("bufsize = %d\n", buf_size));
	}

	inFile = fopen(argv[3], "r");
	if (inFile == NULL) {
		printf("craw: Couldn't open input file\n");
		exit(1);
	}

	windRect = bitsRect;
	windRect.OffsetTo(BPoint(100, 100));
	windRect.InsetBy(-5, -5);
	BWindow* convertWindow = new BWindow(windRect, "Conversion", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE);
	windRect.OffsetTo(B_ORIGIN);
	BView* convertView = new BView(windRect, "ConvertRawView", B_FOLLOW_LEFT + B_FOLLOW_TOP, B_WILL_DRAW);
	convertWindow->AddChild(convertView);
	convertWindow->Show();

	beMap = new BBitmap(bitsRect, dest_space);

	// Read in the bits
	uchar* buffer = new uchar[buf_size];
	fread(buffer, 1, buf_size, inFile);

	// draw converted bitmap
	beMap->SetBits(buffer, buf_size, 0, src_space);

	// check for mask and make bits transparent
	strcpy(maskName, argv[3]);
	if (maskFile = fopen(strcat(maskName, ".msk"), "r")) {
		uchar* buffer2 = new uchar[buf_size];
		mask_row_bytes = beMap->BytesPerRow() / 8;
		printf("doing a mask\n");
#if FULL_MASK
		fread(buffer2, 1, buf_size, maskFile);
#else
		fread(buffer2, mask_row_bytes, bitsRect.Height() + 1, maskFile);
#endif
		MakeTransparent((uchar*)beMap->Bits(), buffer2, &bitsRect);
		fclose(maskFile);
		delete(buffer2);
	}

	convertWindow->Lock();
	convertView->DrawBitmap(beMap, BPoint(5, 5));
	convertWindow->Unlock();

	// don't use converted bits in 32 bit color (BGRA won't work for SetBits)
	if ((dest_space == B_RGB_32_BIT) || (dest_space == B_MONOCHROME_1_BIT)) {
		bits_buf = buffer;
		bits_length = buf_size;
	} else {
		bits_buf = (uchar*)beMap->Bits();
		bits_length = beMap->BitsLength();
	}

	// create resource file if necessary
	if (resCreate) {
		if (file.SetTo("/boot/icon_res", O_CREAT | O_RDWR) != B_NO_ERROR)
			return;

		status_t error = resFile.SetTo(&file, true);
		if (error != B_NO_ERROR) {
			PRINT(("error %x opening resource file\n", error));
			return;
		}
		
		resType = 'PICT';

		if (dest_space == B_COLOR_8_BIT) {
			if (bitsRect.Width() == 31)
				resType = 'ICON';
			else
				if (bitsRect.Width() == 15)
					resType = 'MICN';
		}

		memcpy(resName, &resType, 4);
		resName[4] = 0;
		id = 0;
		while (resFile.GetResourceInfo(resType, id, &name, &size))
			id++;

		if (resFile.AddResource(resType, id, bits_buf, bits_length, resName) != B_NO_ERROR)
			printf("craw: error calling AddResource\n");

//##	resFile.SetTypeAndApp('IRES', 'IWLD');
	} else {
		outFile = fopen("iconfile", "a+");
		fseek(outFile, 0, SEEK_END);

		fprintf(outFile, "unsigned char %s[] = {", argv[3]);

		base = bits_buf;
		for (i = 0; i < bits_length - 1; i++) {
			if (i % 16 == 0)
				fprintf(outFile, "\n");
			fprintf(outFile, "0x%.2x,", base[i]++);
		}
		fprintf(outFile, "0x%.2x};\n", base[i]);
		fclose(outFile);
	}

	snooze(6000000);	

	delete beMap;
	delete buffer;

	fclose(inFile);
}

//-------------------------------------------------------------
void MakeTransparent(uchar* pixmap_shared, uchar* the_mask, BRect* r) 
{
	if (r->Width() == 31)
		TransLargeIcon(pixmap_shared, the_mask);
	else
		TransSmallIcon(pixmap_shared, the_mask);
}

//-------------------------------------------------------------
void TransLargeIcon(uchar* pixmap_shared, uchar* the_mask) 
{
#if	FULL_MASK

	long	i;
	long	tmp;
	const long	WHITE = 0xffffff00;

	for (i = 0; i < (32*32); i++) {
		tmp = *((long *)the_mask);
		if ((tmp & WHITE) == WHITE)
			*pixmap_shared = B_TRANSPARENT_8_BIT;
		pixmap_shared++;
		the_mask += 3;
	}

#else
	long	i;
	long	j;
	ulong	tmp;

	for (i = 0; i < 32; i++) {
		tmp = *((ulong*)the_mask);
		the_mask += 4;

		for (j = 0; j < 32; j++) {
			if ((tmp & 0x80000000) == 0)
				*pixmap_shared = B_TRANSPARENT_8_BIT;
			pixmap_shared++;
			tmp <<= 1;
		}
	}
#endif
}

//-------------------------------------------------------------
void TransSmallIcon(uchar* pixmap_shared, uchar* the_mask) 
{
#if	FULL_MASK

	long	i;
	long	tmp;
	const long	WHITE = 0xffffff00;

	for (i = 0; i < (16*16); i++) {
		tmp = *((long *)the_mask);
		if ((tmp & WHITE) == WHITE)
			*pixmap_shared = B_TRANSPARENT_8_BIT;
		pixmap_shared++;
		the_mask += 3;
	}

#else

	long	i;
	long	j;
	ushort	tmp;

	for (i = 0; i < 16; i++) {
		tmp = *((ushort*)the_mask);
		the_mask += 2;

		for (j = 0; j < 16; j++) {
			if ((tmp & 0x8000) == 0)
				*pixmap_shared = B_TRANSPARENT_8_BIT;
			pixmap_shared++;
			tmp <<= 1;
		}
	}
#endif
}
