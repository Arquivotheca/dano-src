/*
/*     Description : Low level Driver Interface Module 
*/

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "usbserial.h"
static   int rdrhandle[4]={0,0,0,0};

FILE* fw;

void put_msg(char *fmt, ...)
{
#if DEBUG
	char buf[200];
	va_list argptr;
	
	if (fw == NULL)
	{
		fw = fopen("gemusb.log", "w");
	}	

	memset(buf, 0, sizeof(buf));
	va_start(argptr, fmt);
	vsprintf(buf, fmt, argptr);
	va_end(argptr);
	
	if ( fw != NULL )
	{
        fprintf(fw, "%s", buf);
        //printf("%s", buf);
        fflush(fw);
	}
#endif
}


RESPONSECODE OpenReader( DWORD dwChannel )

{
	int rdrno;
	put_msg("\n\nLLDInterface.c:Inside OpenReader(): dwchannel value is :%d",dwChannel);
	if (dwChannel==0x20)		rdrno=0;
	else if(dwChannel==0x21)	rdrno=1;
	else						return ERR_INVALID_PARAM;
	if (rdrhandle[rdrno] > 0)
		return ERR_OPENED;

	rdrhandle[rdrno] = OpenUSB(rdrno);

	put_msg("\n\nLLDInterface.c:Inside Open Reader(): rdrhandle is %d",rdrhandle[rdrno]);	
	
	return rdrhandle[rdrno];
}


RESPONSECODE WriteData( DWORD rdrno, DWORD length, unsigned char *buffer )
{
	put_msg("\n\n LLDInterface.c Inside WriteData():%d ",rdrno);

    if (rdrno<0 || rdrno>3)
		return ERR_INVALID_PARAM;
	
    if (rdrhandle[rdrno]<0)
		return ERR_NOT_OPENED;

	put_msg("\n\nCalling Write USB");

	return WriteUSB(rdrno, length, buffer );
}

RESPONSECODE ReadData( DWORD rdrno, DWORD *length, unsigned char *buffer )
{
    if (rdrno<0 || rdrno>3)
		return ERR_INVALID_PARAM;
	
    if (rdrhandle[rdrno]<0)
		return ERR_NOT_OPENED;

	return ReadUSB(rdrno, length, buffer );
}

RESPONSECODE CloseReader( DWORD rdrno )
{
	if (rdrno<0 || rdrno>3)
		return ERR_INVALID_PARAM;

	if (rdrhandle[rdrno]<0)
		return ERR_OPENED;

	return	CloseUSB(rdrno);
}
