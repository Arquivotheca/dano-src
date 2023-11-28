//*****************************************************************************
//
//	File:		 Scale.cpp
//
//	Description: bitmap scaling header
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************
#if ! defined SCALE_INCLUDED
#define SCALE_INCLUDED

class BBitmap;

void ddascale32(BBitmap *src, BBitmap *dest, float xratio, float yratio, volatile const bool *terminate);

#endif
