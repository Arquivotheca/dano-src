#ifndef _ISOIO_H
#define _ISOIO_H
#include "./defines.h"


INT16 IsoInput
(
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD8    Command[5],
   const WORD8    Data[],
         WORD16  *RespLen,
         BYTE     RespBuff[]
);


INT16 IccIsoInput
(
   const WORD32        Timeout,
   const WORD8    Command[5],
   const WORD8    Data[],
         WORD16  *RespLen,
         BYTE     RespBuff[]
);


INT16 IsoOutput
(
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD8   Command[5],
         WORD16  *RespLen,
         BYTE    RespBuff[]
);

INT16 IccIsoOutput
(
   const WORD32        Timeout,
   const WORD8    Command[5],
         WORD16 *RespLen,
         BYTE     RespBuff[]
);
#endif
