/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : Or32GEm.c
*
* Description : The implemented function translates the IFD status in GemError.h
*               codes.
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


/*------------------------------------------------------------------------------
Information section
 - G_NAME is set to "Or32Gem"
 - G_RELEASE is set to "4.31.002"
------------------------------------------------------------------------------*/
#define G_NAME     "Or32Gem"
#define G_RELEASE  "4.31.002"
/*------------------------------------------------------------------------------
Pragma section
 - comment is called if _MSC_VER is defined.
------------------------------------------------------------------------------*/
#ifdef _MSC_VER
#pragma comment(exestr,"Gemplus(c) "G_NAME" Ver "G_RELEASE" "__DATE__)
#endif
/*------------------------------------------------------------------------------
Compatibility section
 - __MSC is define for compatibily with MSC under Borland environment.
------------------------------------------------------------------------------*/
#define __MSC
/*------------------------------------------------------------------------------
Include section
   Environment include:
    - windows.h gives general Windows 3.1 macros, values and functions.
      STRICT keyword is used to verify stricly variable types.
      This file is include only if windows version is required.
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
   Compiler include:
   Gemplus includes:
    - gemplus.h is used to define general macros and values.
    - gemgcr.h holds readers definitions
    - gemansi.h is used to redefine functions for an Ansi code
------------------------------------------------------------------------------*/
#include "gemplus.h"
#include "gemgcr.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif
/*------------------------------------------------------------------------------
   Module public interface.
    - ifd2gem.h
------------------------------------------------------------------------------*/
#include "ifd2gem.h"

/*------------------------------------------------------------------------------
Function definitions section
------------------------------------------------------------------------------*/
/*******************************************************************************
* INT16 GE_Translate(const BYTE IFDStatus)
*
* Description :
* -------------
* Translate IFD status in GemError codes.
*
* Remarks     :
* -------------
* Nothing.
* 
* In          :
* -------------
* IFDStatus is the value to translate.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* A GemError.h code.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL GE_Translate(const BYTE IFDStatus)
{
   switch (IFDStatus)
   {
      case 0x00 : return G_OK;
      case 0x01 : return GE_IFD_FN_UNKNOWN;
      case 0x02 : return GE_IFD_FN_UNKNOWN;
      case 0x03 : return GE_IFD_FN_FORMAT;
      case 0x04 : return GE_IFD_TIMEOUT;
      case 0x05 : return GE_HI_CMD_LEN;
      case 0x09 : return GE_HI_FORMAT;
      case 0x10 : return GE_II_ATR_TS;
      case 0x11 : return GE_II_INS;
      case 0x12 : return GE_HI_CMD_LEN;
      case 0x13 : return GE_II_COMM;
      case 0x14 : return GE_ICC_UNKNOWN;
      case 0x15 : return GE_ICC_NOT_POWER;
      case 0x16 : return GE_IFD_FN_PROG;
      case 0x17 : return GE_II_PROTOCOL;
      case 0x18 : return GE_II_PROTOCOL;
      case 0x19 : return GE_IFD_FN_DEF;
      case 0x1A : return GE_HI_LEN;
      case 0x1B : return GE_IFD_FN_FORMAT;
      case 0x1C : return GE_IFD_FN_DEF;
      case 0x1D : return GE_II_ATR_TCK;
      case 0x1E : return GE_IFD_FN_DEF;
      case 0x1F : return GE_IFD_FN_DEF;
      case 0x20 : return GE_IFD_FN_UNKNOWN;
      case 0x30 : return GE_IFD_TIMEOUT;
      case 0xA0 : return GW_ATR;
      case 0xA1 : return GE_II_PROTOCOL;
      case 0xA2 : return GE_ICC_MUTE;
      case 0xA3 : return GE_II_PARITY;
      case 0xA4 : return GE_ICC_ABORT;
      case 0xA5 : return GE_IFD_ABORT;
      case 0xA6 : return GE_IFD_RESYNCH;
      case 0xA7 : return GE_II_PTS;
      case 0xCF : return GE_IFD_OVERSTRIKED;
      case 0xE4 : return GE_II_PROC_BYTE;
      case 0xE5 : return GW_APDU_LE;
      case 0xE7 : return G_OK;
      case 0xF7 : return GE_ICC_PULL_OUT;
      case 0xF8 : return GE_ICC_INCOMP;
      case 0xFB : return GE_ICC_ABSENT;
      default   : return ((INT16) (GE_UNKNOWN_PB - IFDStatus));
   };
}


