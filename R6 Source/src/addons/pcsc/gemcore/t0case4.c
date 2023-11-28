/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : T0Case4.c
*
* Description : Module which manages transportation of APDUs by T=0 for case 4.
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


#define G_NAME     "T0Case4"
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

INT16 G_DECL G_T0Case4S
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
/*------------------------------------------------------------------------------
Local variables:
 - apdu_in is used to call GET_RESPONSE Command.
 - response holds called function responses.
------------------------------------------------------------------------------*/
G4_APDU_COMM
   apdu_in;
INT16
   response;

/*------------------------------------------------------------------------------
The first part of the command is sent by calling G_T0Case2S.
<= Test G_T0Case2S status (>=0).
------------------------------------------------------------------------------*/
   response = G_T0Case2S(Timeout,ApduComm,ApduResp,IsoIn);
   if (response < G_OK)
   {
      return (response);
   }
/*------------------------------------------------------------------------------
GET_RESPONSE command is buit in apdu_in.
------------------------------------------------------------------------------*/
   apdu_in.Command[0] = ApduComm->Command[0];
   apdu_in.Command[1] = HT0CASES_GET_RESPONSE;
   apdu_in.Command[2] = 0;
   apdu_in.Command[3] = 0;
   apdu_in.LengthIn   = 0;
/*------------------------------------------------------------------------------
According to the SW1 byte:
 - CASE_4S_2, command accepted.
   GET_RESPONSE is sent with Le in parameter 3 by calling G_T0Case3S command.
<= G_T0Case3S status.
------------------------------------------------------------------------------*/
   switch(HIBYTE(ApduResp->Status))
   {
      case HT0CASES_CASE_4S_2:
      {
         apdu_in.LengthExpected = ApduComm->LengthExpected;
         return (G_T0Case3S(Timeout,&apdu_in,ApduResp,IsoOut));
      }
/*------------------------------------------------------------------------------
 - CASE_4S_3, command accepted with information added.
   GET_RESPONSE is sent with minimum of Le and LengthExpected in parameter 3 by
      calling G_T0Case3S command..
------------------------------------------------------------------------------*/
      case HT0CASES_CASE_4S_3:
      case HT0CASES_CASE_4S_3_SIM:
      {
         apdu_in.LengthExpected = HT0CASES_MIN
         (
            ApduComm->LengthExpected,
            (WORD32)HT0CASES_VAL(LOBYTE(ApduResp->Status))
         );
         response = G_T0Case3S(Timeout,&apdu_in,ApduResp,IsoOut);
/*------------------------------------------------------------------------------
   If the call fails
<=    G_T0Case3S status.   
------------------------------------------------------------------------------*/
         if (response < G_OK)
         {
            return (response);
         }
/*------------------------------------------------------------------------------
   Else if we have read the expected number of bytes
<=    G_OK
------------------------------------------------------------------------------*/
         else if (ApduComm->LengthExpected == ApduResp->LengthOut)
         {
            return (G_OK);
         }
/*------------------------------------------------------------------------------
   Else
<=    GW_APDU_LE
------------------------------------------------------------------------------*/
         else
         {
            return (GW_APDU_LE);
         }
      }
   }
/*------------------------------------------------------------------------------
All others cases (CASE 4S.1 in particular) return
<= GW_APDU_LE
------------------------------------------------------------------------------*/
   return (GW_APDU_LE);
}

INT16 G_DECL G_T0Case4E
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
/*------------------------------------------------------------------------------
Local variables:
 - apdu_in is used to call GET_RESPONSE Command.
 - response holds called function responses.
------------------------------------------------------------------------------*/
G4_APDU_COMM
   apdu_in;
INT16
   response;

/*------------------------------------------------------------------------------
CASE_4E_2 is not supported:
   If Lc > 255
   Then
<=    a builded WRONG LENGTH Apdu is returned with GW_APDU_LE.
------------------------------------------------------------------------------*/
   if (ApduComm->LengthIn > 255)
   {
      ApduResp->LengthOut = 0;
      ApduResp->Status    = HT0CASES_WRONG_LENGTH;
      return (GW_APDU_LE);
   }
/*------------------------------------------------------------------------------
CASE_4E_1 is treated:
   The first part of the command is sent by calling G_T0Case2S.
<=    Test G_T0Case2S status (>=0).
------------------------------------------------------------------------------*/
   response = G_T0Case2S(Timeout,ApduComm,ApduResp,IsoIn);
   if (response < G_OK)
   {
      return (response);
   }
/*------------------------------------------------------------------------------
   GET_RESPONSE command is buit in apdu_in.
------------------------------------------------------------------------------*/
   apdu_in.Command[0] = ApduComm->Command[0];
   apdu_in.Command[1] = HT0CASES_GET_RESPONSE;
   apdu_in.Command[2] = 0;
   apdu_in.Command[3] = 0;
   apdu_in.LengthIn   = 0;
/*------------------------------------------------------------------------------
According to the SW1 byte:
 - CASE_4E_1B, command accepted.
   GET_RESPONSE is sent with 0 in parameter 3 because LengthExpected is greater
      than 256 (True case 4E !). This command is sent by G_T0Case3E.
<= G_T0Case3E status.      
------------------------------------------------------------------------------*/
   switch(HIBYTE(ApduResp->Status))
   {
      case HT0CASES_CASE_4E_1B:
      {
         apdu_in.LengthExpected = ApduComm->LengthExpected;
         return (G_T0Case3E(Timeout,&apdu_in,ApduResp,IsoOut));
      }
/*------------------------------------------------------------------------------
 - CASE_4E_1C, command accepted with information added.
   If the number of available bytes is lower than 256
   Then
      The needed bytes are read by calling G_T0Case3S.
      If the call fails
<=       G_T0Case3S status.   
      Else
<=       GW_APDU_LE (True 4E => LengthExpected > 255 !).
------------------------------------------------------------------------------*/
      case HT0CASES_CASE_4E_1C:
      case HT0CASES_CASE_4E_1C_SIM:
      {
         if (HT0CASES_VAL(LOBYTE(ApduResp->Status)) < 256)
         {
            apdu_in.LengthExpected = LOBYTE(ApduResp->Status);
            response = G_T0Case3S(Timeout,&apdu_in,ApduResp,IsoOut);
            if (response < G_OK)
            {
               return (response);
            }
            else
            {
               return (GW_APDU_LE);
            }
         }
/*------------------------------------------------------------------------------
   Else
      The needed bytes are read by calling G_T0Case3E.
<=    G_T0Case3E status.   
------------------------------------------------------------------------------*/
         else
         {
            apdu_in.LengthExpected = ApduComm->LengthExpected;
            return (G_T0Case3E(Timeout,&apdu_in,ApduResp,IsoOut));
         }
      }
   }
/*------------------------------------------------------------------------------
 - others cases (in particular CASE 4E.1A), we have not read the needed bytes
   so
<=    GW_APDU_LE (True 4E => LengthExpected > 255 !).
------------------------------------------------------------------------------*/
   return (GW_APDU_LE);
}
