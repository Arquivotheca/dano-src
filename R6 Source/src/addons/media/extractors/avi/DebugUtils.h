//---------------------------------------------------------------------
//
//	File:	DebugUtils.h
//
//	Author:	Gene Z. Ragan
//
//	Date:	01.31.98
//
//	Desc:	Debugging Routines
//
//	Copyright ©1998 mediapede Software
//
//---------------------------------------------------------------------

#ifndef __DEBUGUTILS_H__
#define __DEBUGUTILS_H__

#include "RIFFTypes.h"

//	Function Prototypes
void	DebugAlert(char *theString);
void	DumpAVIHeader(AVIHeader *theHeader);
void 	DumpAVIStreamHeader(AVIStreamHeader *theHeader);
void 	DumpVIDSHeader(AVIVIDSHeader *theHeader);
void 	DumpAUDSHeader(AVIAUDSHeader *theHeader);
void 	DumpRIFFID(int32 theID);
void 	DumpAVIHeaderFlags(AVIHeader *theHeader);
void 	DumpAudioType(uint16 type);

#endif
