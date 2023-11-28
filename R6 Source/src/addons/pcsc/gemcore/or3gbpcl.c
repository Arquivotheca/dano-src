/*******************************************************************************
*                    Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : OR3GBPCl.C
*
* Description : Implement the function which closes a communication channel with
*               an GemCore >= 1.x IFD through Gemplus Block Protocl.
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
*                 - Manage the logical channel and not the physical port.
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

/*------------------------------------------------------------------------------
Information section:
 - G_NAME is set to "Or3GbpCl"
 - G_RELEASE is set to "4.31.002"
------------------------------------------------------------------------------*/
#define G_NAME     "Or3GbpCl"
#define G_RELEASE  "4.31.002"

#define G_UNIX

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
#ifdef G_WINDOWS   
#define STRICT
#include <windows.h>
#endif
#ifdef G_OS2
#include <os2.h>
#endif
/*------------------------------------------------------------------------------
   Compiler include:
    - string.h for _fmemcmp function.
------------------------------------------------------------------------------*/
#include <string.h>
/*------------------------------------------------------------------------------
   Gemplus includes:
    - gemplus.h is used to define general macros and values.
    - gemgcr.h holds readers definitions
    - gtser.h manages the serial communication.
    - gtgbp.h manages the GBP protocol.
    - gemansi.h is used to redefine functions for an Ansi code
------------------------------------------------------------------------------*/
#include "gemplus.h"
#include "gemgcr.h"
#include "gtser.h"
#include "gtgbp.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif
/*------------------------------------------------------------------------------
   Module public interface.
    - or3gll.h

------------------------------------------------------------------------------*/
#include "or3gll.h"


/*------------------------------------------------------------------------------
Function definition section:

------------------------------------------------------------------------------*/
/*******************************************************************************
* INT16 G_DECL G_Oros3CloseComm
* (
* )
*
* Description :
* -------------
* This function closes a communication channel with an OROS 3.x IFD.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the value returned by the function which has opened the channel.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*  - G_OK
* If an error condition is raised:
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*  - GE_HOST_PORT_CLOSE (-412) if the selected port is already closed.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3CloseComm
(
)
{
/*------------------------------------------------------------------------------
Local Variable:
   - portcom is the com associate with a logical channel
------------------------------------------------------------------------------*/
INT16
   portcom;

/*------------------------------------------------------------------------------
   Associate Handle ( ChannelNb) with potcom
------------------------------------------------------------------------------*/
   portcom = G_GBPChannelToPortComm();
/*------------------------------------------------------------------------------
Closes the communication channel.
   No action is required on an Oros3.x based IFD.
<= G_SerPortClose status.   
------------------------------------------------------------------------------*/
   G_GBPClose();
   return (G_SerPortClose(portcom));
}

