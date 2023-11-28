#define G_UNIX

#include <stdio.h>


#include "gemplus.h"
#include "gemgcr.h"
#include "or3comm.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif

#include "or3gll.h"

INT16 G_DECL G_Oros3ReadMemory
(
   const WORD32        Timeout,
   const WORD16        MemoryType,
   const WORD16        Address,
   const WORD16        Length,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
//--------------------------------------------------------------
// Local variables:
// - cmd holds the read memory command whose format is
//      <22h> <Type> <ADH> <ADL> <LN>
//--------------------------------------------------------------

WORD8
   cmd[5] = { HOR3GLL_IFD_CMD_MEM_RD};

   cmd[1] = (WORD8)MemoryType;
   cmd[2] = HIBYTE(Address);
   cmd[3] = LOBYTE(Address);
   cmd[4] = (WORD8)Length;
   return (G_Oros3Exchange(Timeout,5,cmd,RespLen,RespBuff));
}




INT16 G_DECL G_Oros3String
(
         WORD16 G_FAR *OsLength,
         char   G_FAR *OsString
)
{
   return
   (
      G_Oros3ReadMemory
      (
         HOR3GLL_LOW_TIME,
         HOR3GLL_IFD_TYP_VERSION,
         HOR3GLL_IFD_ADD_VERSION,
         HOR3GLL_IFD_LEN_VERSION,
         OsLength,
         (WORD8 G_FAR *)OsString
      )
   );
}

INT16 G_DECL G_Oros3BufferSize
(
         WORD16 G_FAR *Length,
         WORD8  G_FAR *Buffer
)
{
WORD8
   cmd[1] = { 0x0A };

   return (G_Oros3Exchange(HOR3GLL_LOW_TIME,1,cmd,Length,Buffer));
}




