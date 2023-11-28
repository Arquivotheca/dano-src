/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : GTTIMOUT.C
*
* Description : This module manages time out under DOS and WINDOWS.
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

/*------------------------------------------------------------------------------
Information section
 - G_NAME is set to "GtTimOut"
 - G_RELEASE is set to "4.31.002"
------------------------------------------------------------------------------*/
#define G_NAME     "GtTimOut"
#define G_RELEASE  "4.31.002"

#define G_UNIX

#include "config.h"



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
 - CLK_TCK, clock_t and clock() are redefined to have a single writing for both
   DOS and WINDOWS environment.
------------------------------------------------------------------------------*/
#define __MSC
#ifdef G_WINDOWS
#define CLK_TCK 1000.0
#define clock_t WORD32
#define clock() GetTickCount()
#endif
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
    - time.h is used for clock function under DOS environment.
      This file is include only if windows version is disabled.
------------------------------------------------------------------------------*/
#ifndef G_WINDOWS
#include <time.h>
#endif
/*------------------------------------------------------------------------------
   Gemplus includes:
    - gemplus.h is used to define general macros and values.
    - gemansi.h is used to redefine functions for an Ansi code
------------------------------------------------------------------------------*/
#include "gemplus.h"
#if (defined WIN32) || (defined G_UNIX) || (defined G_OS2)
#include "gemansi.h"
#endif

#ifdef G_UNIX
#include <sys/times.h>
// #include <sys/poll.h>
#endif

/*------------------------------------------------------------------------------
   Module public interface.
    - gttimout.h
------------------------------------------------------------------------------*/
#include "gttimout.h"
/*------------------------------------------------------------------------------
Function definitions section
------------------------------------------------------------------------------*/
/*******************************************************************************
* WORD32 G_DECL G_EndTime(const WORD32 Timing)
*
* Description :
* -------------
* This function returns a value to test with G_CurrentTime function to check if
* the Timing has been overlapped.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Timing is a value in milli-seconds which indicates the available time for 
*    an operation.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* The response is the value which will be returned by G_CurrentTime when Timing 
*    milli-seconds will be passed.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
WORD32 G_DECL G_EndTime(const WORD32 Timing)
{
   return 
   (
      (WORD32)
      (   
         clock() 
       + (clock_t)(((float)Timing * (float)CLK_TCK / 1000.0) + 0.5)
      )
   );
}

/*******************************************************************************
* WORD32 G_DECL G_CurrentTime(void)
*
* Description :
* -------------
* This function returns the current time according to an internal unit. This
* function has to be used with G_EndTime.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
* Nothing.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* The response is the current timing according to an internal unit.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
WORD32 G_DECL G_CurrentTime(void)
{
   return ((WORD32)clock());
}

/*******************************************************************************
* float G_DECL G_UnitPerSec (void)
*
* Description :
* -------------
* This function returns the number of units per second.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
* Nothing.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* The number of units per second.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
float  G_DECL G_UnitPerSec (void)
{
   return ((float) CLK_TCK);
}

static struct tms tm;

DWORD G_DECL wait_ms(DWORD ms)
{
	snooze(ms*1000);

//struct pollfd filedes[1];
//	
//	filedes[0].fd = 0;
//	filedes[0].events = POLLNVAL;
//	poll(filedes, 1, ms);
	return ms;
}


