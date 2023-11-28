/*
	BmpUtils.h
	A couple bitmap utility functions.
	LoadBitmap - a replacement for BTranslationUtils::GetBitmapFile,
		which fails on some image formats on some computers.
	SetIconImages - loads an image then uses it to create custom
		Tracker icons for the file.
*/

#ifndef BMPUTILS_H
#define BMPUTILS_H

#include <Bitmap.h>

BBitmap *LoadBitmap(const char *pathname);
void SetImageIcons(const char *pathname);

#endif
