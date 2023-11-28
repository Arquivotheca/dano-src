
#ifndef _READERMANAGEMENT
#define _READERMANAGEMENT

#include "./defines.h"

//internal DS to hold device handle

INT16 handle;

//prototypes

INT16 EstablishConnection(DWORD ChannelID);
INT16 CloseConnection();
INT16 PowerICC(DWORD ActionRequested);
INT16 ApduSplitter();
INT16 ApduBuilder();

INT16 ICCIsoT1(const WORD32  Timeout,const WORD16 ApduLen,
   const WORD8    ApduCommand[],
         WORD16  *RespLen,
         BYTE     RespBuff[]
		);
#endif
