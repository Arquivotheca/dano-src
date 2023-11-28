/*******************************************************************************
*                  Copyright (c) 1996 Gemplus Development
*
* Name        : GemDef.h
*
* Description : General definition for GEMPLUS programs.
*
* Release     : 4.31.001
*
* Last Modif  : 13/10/97: V4.31.001  
*               11/04/97: V4.20.000 - Modify conditionnal compilation for 
*                                     Borland compatibility..
*               22/04/96: V4.20.000 - Update structure for 32 bits alignement.
* 	             01/12/95: V4.10.001 - Update to new Gemplus 4.10 Version.
*               15/06/95: V4.01.002
*               06/07/94: V4.00.000
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
   _GEMDEF_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GEMDEF_H
#define _GEMDEF_H

#include <SmartCardTypes.h>

/*------------------------------------------------------------------------------
   - Unix systems
------------------------------------------------------------------------------*/
#ifdef G_UNIX

#define G_FAR
#define G_HUGE
#define G_DECL

typedef long LONG;

#define NOPARITY	0
#define ODDPARITY	8
#define EVENPARITY	24

#define ONESTOPBIT	1
#define TWOSTOPBITS	2

#endif

#ifndef FALSE
#define FALSE           0
#endif
#ifndef TRUE
#define TRUE            1
#endif


/*------------------------------------------------------------------------------
   TLV_TYPE structure holds:
      - a type field   (T) which indicates the V field type:
      - a length field (L) which indicates the size of the V field, in  bytes.
      - a value field  (V) which is the data part of the message.
------------------------------------------------------------------------------*/
typedef struct
{
   WORD8        T;
   WORD8        L;
   void  G_FAR *V;

} TLV_TYPE;


/*------------------------------------------------------------------------------
Constant definitions:
   EOS is the End Of String character.
   MAX_FILE_NAME is the maximal length for a full file name, with path.
   MAX_LINE_LENGTH is the maximal length for an ASCII line in our configuration
      files.
------------------------------------------------------------------------------*/
#define EOS                     '\0'
#define MAX_FILE_NAME           129
#define MAX_LINE_LENGTH         120

#define OFF                      0
#define ON                       1
#define CLEAR                    2

#ifndef NULL
#define NULL                     0
#endif


#endif
