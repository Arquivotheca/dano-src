//---------------------------------------------------------------------
//
//	File:	AppUtils.h
//
//	Author:	Gene Z. Ragan
//
//	Date:	08.25.98
//
//	Desc:	Application Utilities
//
//	Copyright ©1998 mediapede Software
//
//---------------------------------------------------------------------

#ifndef __APPUTILS_H__
#define __APPUTILS_H__

class BBitmap;

//	Byte swapping Utilities
int32 	ReadIntMsb(BFile *in, int bytes);
int32 	BytesToIntMsb(void *buff, int bytes);
int32 	ReadIntLsb(BFile *in, int bytes);
int32 	BytesToIntLsb(void *buff, int bytes);
void 	SkipBytes(BFile *in, int bytes);
void	WriteIntMsb(BFile *out, int32 l, int bytes);
void 	WriteIntLsb(BFile *out, int32 l, int bytes);

BBitmap *MirrorBitmapV(BBitmap *srcBitmap, bool center);
int32 	GetBitmapPixelSize(color_space colorSpace);

// Keyboard Utilities
bool 	IsKeyDown( char theKey);
bool 	IsShiftKeyDown();
bool 	IsCommandKeyDown();
bool 	IsOptionKeyDown();
bool 	IsControlKeyDown();

BWindow *FindWindow(const char *title);
void 	CenterWindow(BWindow *theWindow);

#endif
