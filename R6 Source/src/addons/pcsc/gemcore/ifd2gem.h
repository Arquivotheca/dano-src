/*******************************************************************************
*                    Copyright (c) 1991-1997 Gemplus Development
*
* Name        : IFD2Ggem
*
* Description : Translates IFD status code in GemError codes.
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
   _IFD2GEM_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _IFD2GEM_H
#define _IFD2GEM_H

/*------------------------------------------------------------------------------
C++ Section:
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
Prototype section
------------------------------------------------------------------------------*/
INT16 G_DECL GE_Translate(const BYTE IFDStatus);

#ifdef __cplusplus
}
#endif

#endif
