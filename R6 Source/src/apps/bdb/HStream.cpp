/*
	HStream.cpp
	
	Copyright Hekkelman Programmatuur
	
	Created: 09/15/97 03:53:14
*/

#include "HStream.h"

void ReadCString(BPositionIO& inStream, int inMaxLen, char *outString)
{
	long l = 1;
	
	do inStream.Read(outString++, l);
	while (l && --inMaxLen && outString[-1]);
} // ReadCString

