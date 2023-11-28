/*******************************************************************************
*                   Copyright (c) 1991-1997 Gemplus developpement
*
* Name        : ApduBuil.h
*
* Description : Module which builds APDU commands according to the 7816-4
*               standard 1995(E).
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
   _APDUBUIL_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _APDUBUIL_H
#define _APDUBUIL_H

/*------------------------------------------------------------------------------
C++ Section:
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
Prototype section
------------------------------------------------------------------------------*/
INT16 G_DECL ApduBuilder
(
   const G4_APDU_COMM G_FAR  *ApduComm,
         WORD8        G_HUGE *Buffer,
         WORD32       G_FAR  *Length
);

#ifdef __cplusplus
}
#endif

#endif

