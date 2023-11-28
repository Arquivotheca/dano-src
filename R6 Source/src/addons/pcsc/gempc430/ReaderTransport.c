
#include "./defines.h"
#include "./IFDerror.h"
#include "./usbserial.h"


INT16 Exchange
(	INT16 handle,
	WORD16	CmdLen,
	BYTE	Cmd[],
	WORD16	*RspLen,
	BYTE	Rsp[]
)
{
	RESPONSECODE response;
	WORD16 rlen;
	BYTE  rbuf[256];
	BYTE Command[256];
	int i;
	
	put_msg("\n\nReaderTransport.c: Exchange():");
	
	for(i=0;i<CmdLen;i++)
		put_msg("#%x#",Cmd[i]);
	
	// construct command format
	Command[0] = CmdLen;
	memcpy(Command+1, Cmd, CmdLen);
	response = WriteData(handle, CmdLen+1, Command);
	if (response != STATUS_SUCCESS)
		return IFD_COMMUNICATION_ERROR;
	
	// Get the response
	response = ReadData(handle, &rlen, rbuf);
	if (response != STATUS_SUCCESS)
		return IFD_COMMUNICATION_ERROR;

	// Sanity check
	if (rlen != rbuf[0]+1)
		return IFD_COMMUNICATION_ERROR;
	
	*RspLen = rbuf[0];
	memcpy(Rsp, rbuf+1, *RspLen);
	
	put_msg("ReaderTransport.c The value of response data is :%d", *RspLen);
	{
		int i;
		for(i=0;i<*RspLen;i++)
			put_msg("ReaderTransport.c esponse data is :%x", Rsp[i]);
	}
	
	return IFD_SUCCESS;
}


