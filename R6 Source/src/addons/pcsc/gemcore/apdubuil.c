/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : ApduBuil.c
*
* Description : Module which builds APDU commands according to the 7816-4
*               standard 1995.
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

#define G_NAME     "ApduBuil"
#define G_RELEASE  "4.31.002"

#ifdef _MSC_VER
#pragma comment(exestr,"Gemplus(c) "G_NAME" Ver "G_RELEASE" "__DATE__)
#endif

#define __MSC

#include <string.h>
#include "gemplus.h"
#include "gemgcr.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif

#include "apdubuil.h"

/*------------------------------------------------------------------------------
Constant section
 - Length of different APDU cases, without data field.
    * CASE_1_LEN   4lu
    * CASE_2S_LEN  5lu
    * CASE_2E_LEN  7lu
    * CASE_3S_LEN  5lu
    * CASE_3E_LEN  7lu
    * CASE_4S_LEN  6lu
    * CASE_4E_LEN  9lu
------------------------------------------------------------------------------*/
#define CASE_1_LEN              4lu
#define CASE_2S_LEN             5lu
#define CASE_2E_LEN             7lu
#define CASE_3S_LEN             5lu
#define CASE_3E_LEN             7lu
#define CASE_4S_LEN             6lu
#define CASE_4E_LEN             9lu

/*------------------------------------------------------------------------------
 - EXTEND_FLAG is a flag used in extended cases.
------------------------------------------------------------------------------*/
#define EXTEND_FLAG             0

INT16 G_DECL ApduBuilder
(
   const G4_APDU_COMM G_FAR  *ApduComm,
         WORD8        G_HUGE *Buffer,
         WORD32       G_FAR  *Length
)
{
/*------------------------------------------------------------------------------
Local variables:
 - ptr is a huge pointer used to avoid wraparound.
------------------------------------------------------------------------------*/
WORD8 G_HUGE *
   ptr;

/*------------------------------------------------------------------------------
The command bytes are copied in the given buffer.
<= Test available space (>= COMMAND_LEN): GE_HI_CMD_LEN
   The command byte are copied.
------------------------------------------------------------------------------*/
   if (*Length < COMMAND_LEN)
   {
      return (GE_HI_CMD_LEN);
   }
   _fmemcpy(Buffer,ApduComm->Command,COMMAND_LEN);
/*------------------------------------------------------------------------------
APDU cases are decoded and the buffer is filled according to the recognized 
   case.
   If not data are sent
   Then
      If no data are awaited
      Then APDU case 1 is recognized: 
<=       Test available space (>= CASE_1_LEN): GE_HI_CMD_LEN
         Updates the Length parameter.
------------------------------------------------------------------------------*/
   if (ApduComm->LengthIn == 0)
   {
      if (ApduComm->LengthExpected == 0)
      {
         if (*Length < CASE_1_LEN)
         {
            return (GE_HI_CMD_LEN);
         }
         *Length = CASE_1_LEN;
      }
/*------------------------------------------------------------------------------
      Else
         If the data awaited length is lower or equal to 256
         Then APDU case 3S is recognized:
<=          Test available space (>= CASE_3S_LEN): GE_HI_CMD_LEN
            Adds the awaited length low byte (0 if val = 256, val if val<256)
               and updates the Length parameters.
------------------------------------------------------------------------------*/
      else
      {
         if (ApduComm->LengthExpected <= 256)
         {
            if (*Length < CASE_3S_LEN)
            {
               return (GE_HI_CMD_LEN);
            }
            *(Buffer + COMMAND_LEN) = LOBYTE(ApduComm->LengthExpected);
            *Length = CASE_3S_LEN;
         }
/*------------------------------------------------------------------------------
         Else APDU case 3E is recognized:
<=          Test available space (>= CASE_3E_LEN): GE_HI_CMD_LEN
<=          Test the awaited data size (<= 65536): GE_APDU_LE
            Adds the extended flag and
            Adds the awaited length low word (0 if val=65536, val if val<65536)
               and updates the Length parameters.
------------------------------------------------------------------------------*/
         else
         {
            if (*Length < CASE_3E_LEN)
            {
               return (GE_HI_CMD_LEN);
            }
            if (ApduComm->LengthExpected > 65536lu)
            {
               return (GE_APDU_LE);
            }
            *(Buffer + COMMAND_LEN) = EXTEND_FLAG;
            *(Buffer + COMMAND_LEN + 1) = HIBYTE(ApduComm->LengthExpected);
            *(Buffer + COMMAND_LEN + 2) = LOBYTE(ApduComm->LengthExpected);
            *Length = CASE_3E_LEN;
         }
      }
   }
/*------------------------------------------------------------------------------
   Else
      If no data are awaited
      Then
         If sending data length is lower than 256
         Then APDU case 2S is recognized: 
<=          Test available space (>= CASE_2S_LEN + sending data length):
               GE_HI_CMD_LEN
            Adds the sending data length low byte (<256), adds the data and 
               updates the Length parameter.
------------------------------------------------------------------------------*/
   else
   {
      if (ApduComm->LengthExpected == 0)
      {
         if (ApduComm->LengthIn < 256)
         {
            if (*Length < CASE_2S_LEN + ApduComm->LengthIn)
            {
               return (GE_HI_CMD_LEN);
            }
            *(Buffer + COMMAND_LEN) = LOBYTE(ApduComm->LengthIn);
            ptr = Buffer + CASE_2S_LEN;
            _fmemcpy(ptr,ApduComm->DataIn,(size_t)ApduComm->LengthIn);
            *Length = CASE_2S_LEN + ApduComm->LengthIn;
         }
/*------------------------------------------------------------------------------
         Else APDU case 2E is recognized: 
<=          Test available space (>= CASE_2E_LEN + sending data length): 
               GE_HI_CMD_LEN
<=          Test sending data length (< 65536): GE_APDU_LE
            Adds the extend flag, adds the sending data length low WORD 
               (<65536), adds the data and updates the Length parameter.
------------------------------------------------------------------------------*/
         else
         {
            if (*Length < CASE_2E_LEN + ApduComm->LengthIn)
            {
               return (GE_HI_CMD_LEN);
            }
            if (ApduComm->LengthIn > 65535)
            {
               return (GE_APDU_LE);
            }
            *(Buffer + COMMAND_LEN) = EXTEND_FLAG;
            *(Buffer + COMMAND_LEN + 1) = HIBYTE(ApduComm->LengthIn);
            *(Buffer + COMMAND_LEN + 2) = LOBYTE(ApduComm->LengthIn);
            ptr = Buffer + CASE_2E_LEN;
            _fmemcpy(ptr,ApduComm->DataIn,(size_t)ApduComm->LengthIn);
            *Length = CASE_2E_LEN + ApduComm->LengthIn;
         }
      }
/*------------------------------------------------------------------------------
      Else
         If sending data length is lower than 256 and awaited data length is 
            lower or equal to 256
         Then APDU case 4S is recognized: 
<=          Test available space (>= CASE_4S_LEN + sending data length): 
               GE_HI_CMD_LEN
            Adds the sending data length low byte (<256), adds the data, adds
               the awaited length low byte (0 if val = 256, val is val < 256)
               and  updates the Length parameter.
------------------------------------------------------------------------------*/
      else
      {
         if ((ApduComm->LengthIn < 256) && (ApduComm->LengthExpected <= 256))
         {
            if (*Length < CASE_4S_LEN + ApduComm->LengthIn)
            {
               return (GE_HI_CMD_LEN);
            }
            *(Buffer + COMMAND_LEN) = LOBYTE(ApduComm->LengthIn);
            ptr = Buffer + COMMAND_LEN + 1;
            _fmemcpy(ptr,ApduComm->DataIn,(size_t)ApduComm->LengthIn);
            ptr += ApduComm->LengthIn;
            *ptr = LOBYTE(ApduComm->LengthExpected);
            *Length = CASE_4S_LEN + ApduComm->LengthIn;
         }
/*------------------------------------------------------------------------------
         Else APDU case 4E is recognized: 
<=          Test available space (>= CASE_4E_LEN + sending data length): 
               GE_HI_CMD_LEN
<=          Test the awaited length (<= 65536): GE_APDU_LE
            Adds the extend flag, adds the sending data length low word 
               (<65535), adds the data, adds the awaited length low word 
               (0 if val=65536, val is val < 65536) and  updates the Length 
               parameter.
------------------------------------------------------------------------------*/
         else
         {
            if
            (   
               (ApduComm->LengthIn > 0xFFFFFFFFlu - CASE_4E_LEN)
            || (*Length < CASE_4E_LEN + ApduComm->LengthIn)
            )
            {
               return (GE_HI_CMD_LEN);
            }
            if (ApduComm->LengthExpected > 65536L)
            {
               return (GE_APDU_LE);
            }
            *(Buffer + COMMAND_LEN) = EXTEND_FLAG;
            *(Buffer + COMMAND_LEN + 1) = HIBYTE(ApduComm->LengthIn);
            *(Buffer + COMMAND_LEN + 2) = LOBYTE(ApduComm->LengthIn);
            ptr = Buffer + COMMAND_LEN + 3;
            _fmemcpy(ptr,ApduComm->DataIn,(size_t)ApduComm->LengthIn);
            ptr += ApduComm->LengthIn;
            *ptr++ = HIBYTE(ApduComm->LengthExpected);
            *ptr   = LOBYTE(ApduComm->LengthExpected);
            *Length = CASE_4E_LEN + ApduComm->LengthIn;
         }
      }
   }
/*------------------------------------------------------------------------------
<= G_OK.
------------------------------------------------------------------------------*/
   return (G_OK);
}
