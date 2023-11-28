/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : ApduSpli.c
*
* Description : Module which splits APDU commands for T=0 protocole.
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
* Warning     : The 7616-4 standard indicates that only the transmission system
*               can initiate GET_RESPONSE and ENVELOPE command. This two
*               commands cannot be transmitted by this module.
*
* Remark      : The true cases 2E and 4E.2 are not supported because the command
*               ENVELOPE is not supported by this module.
*
*******************************************************************************/

#define G_UNIX


#define G_NAME     "ApduSpli"
#define G_RELEASE  "4.31.002"

#ifdef _MSC_VER
#pragma comment(exestr,"Gemplus(c) "G_NAME" Ver "G_RELEASE" "__DATE__)
#endif
//-----------------------------------------------------------------
// Compatibility section
// - __MSC is define for compatibily with MSC under Borland environment.
// --------------------------------------------------------------
#define __MSC

#include "gemplus.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif
#include "gemgcr.h"
#include "t0cases.h"

#include "apduspli.h"

INT16 G_DECL ApduSpliter
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
		),
		INT16        (G_DECL *IsoOut)
		(
		const WORD32        Timeout,
		const WORD8  G_FAR  Command[5],
		WORD16 G_FAR *RespLen,
		BYTE   G_FAR  RespBuff[]
		)

)
{
   if (ApduComm->LengthExpected == 0)
   {
      if (ApduComm->LengthIn == 0)
      {
         return
         (
            G_T0Case1(Timeout,ApduComm, ApduResp, IsoIn)
         );
      }
      else if (ApduComm->LengthIn <= HT0CASES_LIN_SHORT_MAX)
      {
         return(G_T0Case2S(Timeout,ApduComm,ApduResp,IsoIn));
      }
      ApduResp->LengthOut = 0;
      ApduResp->Status    = HT0CASES_WRONG_LENGTH;
      return (G_OK);
   }
   else if (ApduComm->LengthIn == 0)
   {
      if (ApduComm->LengthExpected <= HT0CASES_LEX_SHORT_MAX)
      {
         return (G_T0Case3S(Timeout,ApduComm,ApduResp,IsoOut));
      }
      else
      {
         return (G_T0Case3E(Timeout,ApduComm,ApduResp,IsoOut));
      }
   }
   else if (   (ApduComm->LengthIn        <= HT0CASES_LIN_SHORT_MAX) 
            && (ApduComm->LengthExpected  <= HT0CASES_LEX_SHORT_MAX)
           )
   {
      return(G_T0Case4S(Timeout,ApduComm,ApduResp,IsoIn,IsoOut));
   }
   else
   {
      return(G_T0Case4E(Timeout,ApduComm,ApduResp,IsoIn,IsoOut));
   }
}




