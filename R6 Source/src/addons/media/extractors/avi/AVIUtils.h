//---------------------------------------------------------------------
//
//	File:	AVIUtils.h
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

#if !defined(__AVIUTILS_H__)
#define __AVIUTILS_H__

#include "RIFFConstants.h"
#include "RIFFTypes.h"

//	Byte swapping Utilities
int32 	ReadIntMsb(FILE *in, int bytes);
int32 	ReadIntMsb(BPositionIO *in, int bytes);
int32 	BytesToIntMsb(void *buff, int bytes);
int32 	ReadIntLsb(FILE *in, int bytes);
int32 	ReadIntLsb(BPositionIO *in, int bytes);
int32 	BytesToIntLsb(void *buff, int bytes);

ssize_t WriteInt16Lsb(FILE *theFile, uint16 data);
ssize_t WriteInt32Lsb(FILE *theFile, uint32 data);
ssize_t WriteInt32Msb(FILE *theFile, uint32 data);

//	RIFF Utilities
void	DebugAlert(char *theString);
void	DumpAVIHeader(AVIHeader *theHeader);
void 	DumpAVIStreamHeader(AVIStreamHeader *theHeader);
void 	DumpVIDSHeader(AVIVIDSHeader *theHeader);
void 	DumpAUDSHeader(AVIAUDSHeader *theHeader);
void 	DumpRIFFID(int32 theID);
void 	DumpAVIHeaderFlags(AVIHeader *theHeader);
void 	DumpAudioType(uint16 type);

#endif
