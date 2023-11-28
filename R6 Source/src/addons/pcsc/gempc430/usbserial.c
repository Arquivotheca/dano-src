/*       Title: usbserial.c
/      Purpose: Abstracts usb API to serial like calls
*/

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include "usbserial.h"
static int handle[4]={0,0,0,0};

char str[30]="/dev/smartcard/usb/gempc430/0";

RESPONSECODE OpenUSB(DWORD lun)
{
	if (lun<0 || lun>3)
        return ERR_INVALID_PARAM;

	if (handle[lun]>0)
		return ERR_OPENED;

	str[strlen(str)-1] = '0'+lun;
        
	if ((handle[lun] = open(str, O_RDWR)) < 0)
		return ERR_IO_COMM;		

	put_msg("\nusbserial.c::OpenUSB():device file name : %s, handle ret:%d",str,handle[lun]);		
   	return STATUS_SUCCESS;
}

RESPONSECODE WriteUSB(DWORD lun, DWORD length, unsigned char *buffer )
{
	size_t written;
	
    if (lun<0 || lun>3)
        return ERR_INVALID_PARAM;
	
    if (handle[lun]<0)
		return ERR_NOT_OPENED;

    put_msg("\n usbserial.c:WriteUSB: Command:"); 
    {
		int i;
		for(i=0 ; i<length ; i++)
			put_msg(" # %2x #", buffer[i]);
    }
	put_msg("\n usbserial.c: WriteUSB: handle[lun] value is : %d", handle[lun]);

	written = write(handle[lun], buffer, length);
    if (written < length)
	{
		put_msg("error");
	   	return ERR_IO_COMM;
	}
    
    return STATUS_SUCCESS;
}

RESPONSECODE ReadUSB( DWORD lun, DWORD *length, unsigned char *buffer )
{
    if (lun<0 || lun>3)
        return ERR_INVALID_PARAM;
	
    if (handle[lun]<0)
		return ERR_NOT_OPENED;

	*length = read(handle[lun], buffer, 64);

    if (*length < 0)
	   	return ERR_IO_COMM;

    return STATUS_SUCCESS;
}

RESPONSECODE CloseUSB( DWORD lun )
{
    if (lun<0 || lun>3)
        return ERR_INVALID_PARAM;
	
    if (handle[lun]<0)
		return ERR_NOT_OPENED;

	if (close(handle[lun]) != 0)
		return ERR_IO_COMM;		
	handle[lun] = 0;
    return STATUS_SUCCESS;
}
