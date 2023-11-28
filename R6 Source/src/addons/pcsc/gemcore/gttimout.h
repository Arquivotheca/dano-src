/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : GTTIMOUT.H
*
* Description : Interface for time out utility.
*
* Release     : 4.31.001
*
* Last Modif  : 13/10/97: V4.31.001  (GP)
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
Name definition:
   _GTTIMOUT_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GTTIMOUT_H
#define _GTTIMOUT_H

/*------------------------------------------------------------------------------
C++ Section:
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
Function prototype section
------------------------------------------------------------------------------*/
WORD32 G_DECL G_CurrentTime(void);
WORD32 G_DECL G_EndTime    (const WORD32 Timing);
float  G_DECL G_UnitPerSec (void);


// Added by Atul
DWORD G_DECL wait_ms(DWORD ms);


#ifdef __cplusplus
}
#endif

#endif

