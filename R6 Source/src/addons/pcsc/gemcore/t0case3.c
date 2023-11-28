/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : T0Case3.c
*
* Description : Module which manages transportation of APDUs by T=0 for case 3.
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

#define G_NAME     "T0Case3"
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

INT16 G_DECL G_T0Case3S
(
   const WORD32                Timeout,
   const G4_APDU_COMM G_FAR   *ApduComm,
         G4_APDU_RESP G_FAR   *ApduResp,
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
 - cmd is a buffer which is filled with the five command bytes to send to ICC.
 - resp is a buffer of HT0CASES_MAX_SIZE bytes which receives the IFD response: 
   <Status> [Up to 256 bytes] <SW1> <SW2>
 - rlen is used to call IsoIn function. It indicates the response buffer size at
   call and is updated with the real number of read bytes.
   It is initialized to HT0CASES_MAX_SIZE (259).
 - length is used for sending GET_RESPONSE command in loop.
 - ptr is a huge pointer used to fill ApduResp without wraparound.
 - response holds called function responses.
------------------------------------------------------------------------------*/
BYTE  
   cmd [                5],
   resp[HT0CASES_MAX_SIZE];
WORD16
   rlen = HT0CASES_MAX_SIZE,
   length;
BYTE G_HUGE *
   ptr;
INT16
   response;

/*------------------------------------------------------------------------------
The cmd buffer is initialized with the 4 command bytes found in ApduComm. Then
   Le character is added according to Annex A of ISO/IEC 7816-4: 1993(E).
!! THE CALLER MUST VERIFY THAT ApduComm->LengthExpected IS IN [1 .. 256]      !!
------------------------------------------------------------------------------*/
   _fmemcpy(cmd,ApduComm->Command,COMMAND_LEN);
   cmd[HT0CASES_P3] = (BYTE)ApduComm->LengthExpected;
/*------------------------------------------------------------------------------
The communication phase is triggered by calling IsoOut function.
<= Test the IsoOut status (>=G_OK) and the IFD status (GE_Translate(Status)>=0).
<= Test the response length (>= 3): GE_HI_LEN
------------------------------------------------------------------------------*/
   response = IsoOut(Timeout,cmd,&rlen,resp);
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
According to the SW1 byte:
 - CASE_3S_2, Le definitely not accepted.
   LengthOut field is set to 0.
------------------------------------------------------------------------------*/
   switch(resp[rlen - 2])
   {
      case HT0CASES_CASE_3S_2:
      {
         ApduResp->LengthOut = 0;
         break;
      }
/*------------------------------------------------------------------------------
 - CASE_3S_3, Le not accepted, La indicated.
   The command is re-issued with La assigned to P3 parameter.
<= Test the IsoOut status (>=G_OK) and the IFD status (GE_Translate(Status)>=0).
------------------------------------------------------------------------------*/
      case HT0CASES_CASE_3S_3:
      {
         cmd[HT0CASES_P3] = resp[rlen - 1];
         rlen = HT0CASES_MAX_SIZE;
         response = IsoOut(Timeout,cmd,&rlen,resp);
         if (response >= G_OK)
         {
            response = GE_Translate(resp[0]);
         }
         if (response < G_OK)
         {
            return (response);
         }
/*------------------------------------------------------------------------------
   ApduResp is updated:
      If only three bytes have been received
      Then
         No data are available, LengthOut is set to 0.
      Else
         LengthOut is set to Min(LengthExpected, P3).
         DataOut buffer is updated.
------------------------------------------------------------------------------*/
         if (rlen == 3)
         {
            ApduResp->LengthOut = 0;
         }
         else
         {
            ApduResp->LengthOut=HT0CASES_MIN
            (
               ApduComm->LengthExpected,
               (WORD32)HT0CASES_VAL(cmd[HT0CASES_P3])
            );
            _fmemcpy(ApduResp->DataOut,resp + 1,(size_t)ApduResp->LengthOut);
         }
         break;
      }
/*------------------------------------------------------------------------------
 - CASE_3S_4, Command processed, Lx indicated.
   The received bytes are stored:
      If only three bytes have been received
      Then
         No data are available, length is set to 0.
      Else
         length is set to Min(LengthExpected, P3).
         DataOut buffer is updated.
------------------------------------------------------------------------------*/
      case HT0CASES_CASE_3S_4:
      case HT0CASES_CASE_3S_4_SIM:
      {
         if (rlen == 3)
         {
            length = 0;
         }
         else
         {
            length = (WORD16)HT0CASES_MIN
            (
               ApduComm->LengthExpected,
               (WORD32)HT0CASES_VAL(cmd[HT0CASES_P3])
            );
            _fmemcpy(ApduResp->DataOut,resp + 1,length);
         }
         ApduResp->LengthOut = length;
/*------------------------------------------------------------------------------
   GET RESPONSE command is issued with Lx assigned to P3 parameter:
      The command is built and parameter P1 is updated for chaining
      GET RESPONSE.
------------------------------------------------------------------------------*/
         if (cmd[HT0CASES_INS] == HT0CASES_GET_RESPONSE)
         {
            cmd[HT0CASES_P1] = (BYTE)((cmd[HT0CASES_P1] + 1) % 2);
         }
         else
         {
            cmd[HT0CASES_INS] = HT0CASES_GET_RESPONSE;
            cmd[HT0CASES_P1]  = 0x00;
         }
         cmd[HT0CASES_P2]  = 0x00;
/*------------------------------------------------------------------------------
      While CASE_3S_4 is detected and bytes are still awaited,
         P3 byte is updated with last received Lx,
         The command is sent
<=       Test the IsoOut status (>=G_OK) and the IFD status
         (GE_Translate(Status)>=0).
         ApduResp is updated with the received bytes.
         P1 parameter is updated for the next loop.
------------------------------------------------------------------------------*/
         while 
         (
            (
               (resp[rlen - 2] == HT0CASES_CASE_3S_4) 
            || (resp[rlen - 2] == HT0CASES_CASE_3S_4_SIM)
            )
         && (ApduResp->LengthOut < ApduComm->LengthExpected)
         )
         {
            cmd[HT0CASES_P3]  = resp[rlen - 1]; 
            rlen = HT0CASES_MAX_SIZE;
            response = IsoOut(Timeout,cmd,&rlen,resp);
            if (response == G_OK)
            {
               response = GE_Translate(resp[0]);
            }
            if (response < G_OK)
            {
               return (response);
            }
            if (rlen == 3)
            {
               break;
            }
            else
            {
               length = (WORD16)HT0CASES_MIN
               (
                  ApduComm->LengthExpected,
                  (WORD32)HT0CASES_VAL(cmd[HT0CASES_P3])
               );
               ptr = ApduResp->DataOut + ApduResp->LengthOut;
               _fmemcpy(ptr,resp + 1,length);
               ApduResp->LengthOut += length;
            }
            cmd[HT0CASES_P1] = (BYTE)((cmd[HT0CASES_P1] + 1) % 2);
         }
         break;
      }
/*------------------------------------------------------------------------------
 - Other cases must be CASE_3S_1, Le accepted.
   ApduResp is updated:
      If only three bytes have been received
      Then
         No data are available, LengthOut is set to 0.
      Else
         LengthOut is set to Min(LengthExpected, P3).
         DataOut buffer is updated.
------------------------------------------------------------------------------*/
      default:
      {
         if (rlen == 3)
         {
            ApduResp->LengthOut = 0;
         }
         else
         {
            ApduResp->LengthOut = HT0CASES_MIN
            (
               ApduComm->LengthExpected,
               (WORD32)HT0CASES_VAL(cmd[HT0CASES_P3])
            );
            _fmemcpy(ApduResp->DataOut,resp + 1,(size_t)ApduResp->LengthOut);
         }
      }
   }
/*------------------------------------------------------------------------------
The status part of ApduResp is updated with the two last received bytes.
------------------------------------------------------------------------------*/
   ApduResp->Status = (WORD16) ((resp[rlen - 2]<<8) + resp[rlen - 1]);
/*------------------------------------------------------------------------------
If LengthExpected == LengthOut 
Then
<= G_OK
Else
<= GW_APDU_LE
------------------------------------------------------------------------------*/
   if (ApduComm->LengthExpected == ApduResp->LengthOut)
   {
      return (G_OK);
   }
   else
   {
      return (GW_APDU_LE);
   }
}

INT16 G_DECL G_T0Case3E
(
   const WORD32                Timeout,
   const G4_APDU_COMM G_FAR   *ApduComm,
         G4_APDU_RESP G_FAR   *ApduResp,
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
 - cmd is a buffer which is filled with the five command bytes to send to ICC.
 - resp is a buffer of HT0CASES_MAX_SIZE bytes which receives the IFD response: 
   <Status> [Up to 256 bytes] <SW1> <SW2>
 - rlen is used to call IsoIn function. It indicates the response buffer size at
   call and is updated with the real number of read bytes.
   It is initialized to HT0CASES_MAX_SIZE (259).
 - length is used for sending GET_RESPONSE command in loop.
 - ptr is a huge pointer used to fill ApduResp without wraparound.
 - response holds called function responses.
------------------------------------------------------------------------------*/
BYTE  
   cmd [                5],
   resp[HT0CASES_MAX_SIZE];
WORD16
   rlen = HT0CASES_MAX_SIZE,
   length;
BYTE G_HUGE *
   ptr;
INT16
   response;

/*------------------------------------------------------------------------------
The cmd buffer is initialized with the 4 command bytes found in ApduComm. Then
   0 is added according to Annex A of ISO/IEC 7816-4: 1993(E).
!! THE CALLER MUST VERIFY THAT ApduComm->LengthExpected IS IN > 256]          !!
------------------------------------------------------------------------------*/
   _fmemcpy(cmd,ApduComm->Command,COMMAND_LEN);
   cmd[HT0CASES_P3] = 0;
/*------------------------------------------------------------------------------
The communication phase is triggered by calling IsoOut function.
<= Test the IsoOut status (>=G_OK) and the IFD status (GE_Translate(Status)>=0).
------------------------------------------------------------------------------*/
   response = IsoOut(Timeout,cmd,&rlen,resp);
   if (response >= G_OK)
   {
      response = GE_Translate(resp[0]);
   }
   if (response < G_OK)
   {
      return (response);
   }
/*------------------------------------------------------------------------------
According to the SW1 byte:
 - CASE_3E_2A, the card rejected the command because of wrong length
   LengthOut field is set to 0.
------------------------------------------------------------------------------*/
   switch(resp[rlen - 2])
   {
      case HT0CASES_CASE_3E_2A:
      {
         ApduResp->LengthOut = 0;

         break;
      }
/*------------------------------------------------------------------------------
 - CASE_3E_2B, wrong length but right length is indicated in La
   The command is re-issued with La assigned to P3 parameter.
<= Test the IsoOut status (>=G_OK) and the IFD status (GE_Translate(Status)>=0).
------------------------------------------------------------------------------*/
      case HT0CASES_CASE_3E_2B:
      {
         cmd[HT0CASES_P3] = resp[rlen - 1];
         rlen = HT0CASES_MAX_SIZE;
         response = IsoOut(Timeout,cmd,&rlen,resp);
         if (response >= G_OK)
         {
            response = GE_Translate(resp[0]);
         }
         if (response < G_OK)
         {
            return (response);
         }
/*------------------------------------------------------------------------------
   ApduResp is updated:
      If only three bytes have been received
      Then
         No data are available, LengthOut is set to 0.
      Else
         LengthOut is set to Min(LengthExpected, P3).
         DataOut buffer is updated.
------------------------------------------------------------------------------*/
         if (rlen == 3)
         {
            ApduResp->LengthOut = 0;
         }
         else
         {
            ApduResp->LengthOut= HT0CASES_MIN
            (
               ApduComm->LengthExpected,
               (WORD32)HT0CASES_VAL(cmd[HT0CASES_P3])
            );
            _fmemcpy(ApduResp->DataOut,resp + 1,(size_t)ApduResp->LengthOut);
         }
         break;
      }
/*------------------------------------------------------------------------------
 - CASE_3E_2D, Lx indicates the extra amount of bytes availables.
   The received bytes are stored:
      If only three bytes have been received
      Then
         No data are available, length is set to 0.
      Else
         length is set to Min(LengthExpected, P3).
         DataOut buffer is updated.
------------------------------------------------------------------------------*/
      case HT0CASES_CASE_3E_2D:
      case HT0CASES_CASE_3E_2D_SIM:
      {
         if (rlen == 3)
         {
            length = 0;
         }
         else
         {
            length = (WORD16) HT0CASES_MIN
            (
               ApduComm->LengthExpected,
               (WORD32)HT0CASES_VAL(cmd[HT0CASES_P3])
            );
            _fmemcpy(ApduResp->DataOut,resp + 1,length);
         }
         ApduResp->LengthOut = length;
/*------------------------------------------------------------------------------
   GET RESPONSE command is issued with Lx assigned to P3 parameter:
      The command is built and parameter P1 is updated for chaining 
      GET RESPONSE.
------------------------------------------------------------------------------*/
         if (cmd[HT0CASES_INS] == HT0CASES_GET_RESPONSE)
         {
            cmd[HT0CASES_P1] = (BYTE)((cmd[HT0CASES_P1] + 1) % 2);
         }
         else
         {
            cmd[HT0CASES_INS] = HT0CASES_GET_RESPONSE;
            cmd[HT0CASES_P1]  = 0x00;
         }
         cmd[HT0CASES_P2]  = 0x00;
/*------------------------------------------------------------------------------
      While CASE_3E_2D is detected and bytes are still awaited,
         P3 byte is updated with last received Lx,
         The command is sent
<=       Test the IsoOut status (>=G_OK) and the IFD status 
         (GE_Translate(Status)>=0).
         ApduResp is updated with the received bytes.
         P1 parameter is updated for the next loop.
------------------------------------------------------------------------------*/
         while 
         (
            (
               (resp[rlen - 2] == HT0CASES_CASE_3E_2D) 
            || (resp[rlen - 2] == HT0CASES_CASE_3E_2D_SIM)
            )
         && (ApduResp->LengthOut < ApduComm->LengthExpected)
         )
         {
            cmd[HT0CASES_P3]  = resp[rlen - 1]; 
            rlen = HT0CASES_MAX_SIZE;
            response = IsoOut(Timeout,cmd,&rlen,resp);
            if (response == G_OK)
            {
               response = GE_Translate(resp[0]);
            }
            if (response < G_OK)
            {
               return (response);
            }
            if (rlen == 3)
            {
               break;
            }
            else
            {
               length = (WORD16) HT0CASES_MIN
               (
                  ApduComm->LengthExpected,
                  (WORD32)HT0CASES_VAL(cmd[HT0CASES_P3])
               );
               ptr = ApduResp->DataOut + ApduResp->LengthOut;
               _fmemcpy(ptr,resp + 1,length);
               ApduResp->LengthOut += length;
            }
            cmd[HT0CASES_P1] = (BYTE)((cmd[HT0CASES_P1] + 1) % 2);
         }
         break;
      }
/*------------------------------------------------------------------------------
 - Other cases must be CASE_3E_2C, No more byte to receive.
   ApduResp is updated:
      If only three bytes have been received
      Then
         No data are available, LengthOut is set to 0.
      Else
         LengthOut is set to Min(LengthExpected, P3).
         DataOut buffer is updated.
------------------------------------------------------------------------------*/
      default:
      {
         if (rlen == 3)
         {
            ApduResp->LengthOut = 0;
         }
         else
         {
            ApduResp->LengthOut = HT0CASES_MIN
            (
               ApduComm->LengthExpected,
               (WORD32)HT0CASES_VAL(cmd[HT0CASES_P3])
            );
            _fmemcpy(ApduResp->DataOut,resp + 1,(size_t)ApduResp->LengthOut);
         }
      }
   }
/*------------------------------------------------------------------------------
The status part of ApduResp is updated with the two last received bytes.
------------------------------------------------------------------------------*/
   ApduResp->Status = (WORD16) ((resp[rlen - 2]<<8) + resp[rlen - 1]);
/*------------------------------------------------------------------------------
If LengthExpected == LengthOut 
Then
<= G_OK
Else
<= GW_APDU_LE
------------------------------------------------------------------------------*/
   if (ApduComm->LengthExpected == ApduResp->LengthOut)
   {
      return (G_OK);
   }
   else
   {
      return (GW_APDU_LE);
   }
}
