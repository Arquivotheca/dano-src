/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : T0Case2.c
*
* Description : Module which manages transportation of APDUs by T=0 for case 2.
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

#define G_NAME     "T0Case2"
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

INT16 G_DECL G_T0Case2S
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
/*------------------------------------------------------------------------------
Local variables:
 - cmd is a buffer which is filled with the five command bytes to send to ICC.
   One more byte could be used for NULL_LEN command transport.
 - resp is a buffer of 3 bytes which receives the IFD response: 
   <Status> <SW1> <SW2>
 - rlen is used to call IsoIn function. It indicates the response buffer size at
   call and is updated with the real number of read bytes.
   It is initialized to 3.
 - response holds called function responses.
------------------------------------------------------------------------------*/
BYTE  
   cmd [6],
   resp[3];
WORD16
   rlen = 3;
INT16
   response;

/*------------------------------------------------------------------------------
The cmd buffer is initialized with the 4 command bytes found in ApduComm. Then
   Lc character is added according to Annex A of ISO/IEC 7816-4: 1993(E).
!! THE CALLER MUST VERIFY THAT ApduComm->LengthIn IS IN [1 .. 255]            !!
------------------------------------------------------------------------------*/
   _fmemcpy(cmd,ApduComm->Command,COMMAND_LEN);
   cmd[HT0CASES_P3] = (BYTE)ApduComm->LengthIn;
/*------------------------------------------------------------------------------
The communication phase is triggered by calling IsoIn function.
<= Test the IsoIn status (>=G_OK) and the IFD status (GE_Translate(Status)>=0).
<= Test the response length (>= 3): GE_HI_LEN
------------------------------------------------------------------------------*/
   response = IsoIn(Timeout,cmd,ApduComm->DataIn,&rlen,resp);
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
/*------------------------------------------------------------------------------
The ApduResp structure is updated (LengthOut and Status) and
<= G_OK.
------------------------------------------------------------------------------*/
   ApduResp->LengthOut = 0;
   ApduResp->Status    = (WORD16)((resp[rlen - 2]<<8) + resp[rlen - 1]);
   return (G_OK);
}



