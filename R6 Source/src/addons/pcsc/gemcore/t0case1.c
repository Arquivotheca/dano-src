/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : T0Case1.c
*
* Description : Module which manages transportation of APDUs by T=0 for case 1.
*
* Author      : E. PLET
*               G. PAUZIE
*               T. FABRE
*
* Compiler    : Microsoft C PDS 6.0 / 7.0 /8.0
*               Borland   C++   3.x / 4.0
*               Microsoft C++ 1.5 for Windows 16 bits
*               Microsoft C++ 2.0 for Windows 32 bits
*               Borland   C++ 2.0 pour OS/2
*
* Host        : IBM PC and compatible machines under MS/DOS 3.1 and upper.
*               IBM PC and compatible machines under Windows 3.x.
*               IBM PC and compatible machines under Windows 32 bits (W95 or WNT).
*               IBM PC and compatible machines under OS/2 Warp.
*
* Release     : 4.31.002
*
* Last Modif  : 24/08/98: V4.31.002  (GP)
*               13/10/97: V4.31.001  (GP)
*               18/03/97: V4.30.001  (TF)
*                 - Start of development.
*
********************************************************************************
*
* Warning     : 
*
* Remark      :
*
*******************************************************************************/

#define G_UNIX


#define G_NAME     "T0Case1"
#define G_RELEASE  "4.31.002"

#ifdef _MSC_VER
#pragma comment(exestr,"Gemplus(c) "G_NAME" Ver "G_RELEASE" "__DATE__)
#endif

#define __MSC

#include <string.h>

#include "gemplus.h"

#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif
#include "gemgcr.h"
#include "ifd2gem.h"

#include "t0cases.h"

INT16 G_DECL G_T0Case1 
(
   const WORD32                Timeout,
   const G4_APDU_COMM G_FAR   *ApduComm,
         G4_APDU_RESP G_FAR   *ApduResp,
         INT16        (G_DECL *IsoIn)
         (
            const WORD32        Timeout,
            const WORD8  G_FAR  Command[5],
            const WORD8  G_FAR  Data[],
                  WORD16 G_FAR *RespLen,
                  BYTE   G_FAR  RespBuff[]
         )
)
{
BYTE  
   cmd [5],
   resp[3];
WORD16
   rlen = 3;
INT16
   response;

   _fmemcpy(cmd,ApduComm->Command,COMMAND_LEN);
   cmd[HT0CASES_P3] = 0;

   response = IsoIn(Timeout,cmd,NULL,&rlen,resp);
   if (response >= G_OK)
   {
      response = GE_Translate(resp[0]);
   }
   if (response < G_OK)
   {
      return (response);
   }
   if (rlen < 3)
   {
      return (GE_HI_LEN);
   }
   ApduResp->LengthOut = 0;
   ApduResp->Status    = (WORD16)((resp[rlen - 2]<<8) + resp[rlen - 1]);
   return (G_OK);
}



