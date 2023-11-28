/*******************************************************************************
*                 Copyright (c) 1991-1997 GemPlus Developpement
*
* Name        : ANSI.H
*
* Description : Replace NON-ANSI keywords with ANSI keywords.
*
* Release     : 4.31.001
*
* Last Modif  : 13/10/97: V4.31.001  (GP)
*               18/03/97: V4.30.001  (TF)
*                 - Start of development.
*
********************************************************************************
*
* Warning     : This library may be distributed and compiled with ANSI compiler
*
* Remarks     : Nothing
*
*******************************************************************************/

/*------------------------------------------------------------------------------
Name definition:
   _GEMANSI_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GEMANSI_H
#define _GEMANSI_H

#define _fmemccpy    memccpy
#define _fmemchr     memchr
#define _fmemcmp     memcmp
#define _fmemcpy     memcpy
#define _fmemicmp    memicmp
#define _fmemmove    memmove
#define _fmemset     memset

#define _fstrcat     strcat
#define _fstrchr     strchr
#define _fstrcmp     strcmp
#define _fstrcpy     strcpy
#define _fstrcspn    strcspn
#define _fstrdup     _strdup
#define _fstricmp    _stricmp
#define _fstrlen     strlen
#define _fstrlwr     _strlwr
#define _fstrncat    strncat
#define _fstrncmp    strncmp
#define _fstrncpy    strncpy
#define _fstrnicmp   _strnicmp
#define _fstrnset    _strnset 
#define _fstrpbrk    strpbrk
#define _fstrrchr    strrchr
#define _fstrrev     _strrev
#define _fstrset     _strset
#define _fstrspn     strspn
#define _fstrstr     strstr
#define _fstrtok     strtok
#define _fstrupr     _strupr

#define _fmalloc     malloc
#define _nmalloc     malloc
#define _bmalloc     malloc
#define _frealloc    realloc
#define _nrealloc    realloc
#define _brealloc    realloc
#define _fcalloc     calloc
#define _ncalloc     calloc
#define _bcalloc     calloc
#define _fexpand     _expand
#define _nexpand     _expand
#define _bexpand     _expand
#define _fmsize      msize
#define _nmsize      msize
#define _bmsize      msize
#define _ffree       free
#define _nfree       free
#define _bfree       free

#define _fheapchk    heapchk
#define _nheapchk    heapchk
#define _bheapchk    heapchk
#define _fheapmin    heapmin
#define _nheapmin    heapmin
#define _bheapmin    heapmin
#define _fheapset    heapset
#define _nheapset    heapset
#define _bheapset    heapset
#define _fheapwalk   heapwalk
#define _nheapwalk   heapwalk
#define _bheapwalk   heapwalk


#define _fmblen      mblen
#define _fmbstowcs   mbstowcs
#define _fmbtowc     mbtowc
#define _fwcstombs   wcstombs
         
#endif   /* _GEMANSI_H */

