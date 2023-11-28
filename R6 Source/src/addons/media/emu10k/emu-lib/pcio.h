

/*********************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* Filename: pcio.h
*
* Description: include file for low level 386 PC specific read/write
*              functions.
*
*
* Visible Routines: 
*		            
*
*
*******************************************************************************
*/
     
#ifndef __PCIO_H
#define __PCIO_H
     
#include "datatype.h"

#ifdef __BOUND_16BIT
DWORD _indwa(WORD addr);
void _outdwa(WORD addr, DWORD data);
#endif

#define EmuOutW EmuOut
#define EmuInW  EmuIn

#ifndef USE_INLINE

BEGINEMUCTYPE

void  EmuOutB(DWORD dwAddr, BYTE byData);
BYTE  EmuInB (DWORD dwAddr);
void  EmuOut (DWORD dwAddr, WORD dwData);
WORD  EmuIn  (DWORD dwAddr);
void  EmuOutD(DWORD dwAddr, DWORD dwData);
DWORD EmuInD (DWORD dwAddr);

ENDEMUCTYPE

#else

#  include <conio.h>

#  ifndef TESTMODE

#    define EmuOut(dwAddr, dwData)  _outpw((unsigned short)(dwAddr), (unsigned short) (dwData))
#    define EmuIn(dwAddr)           _inpw ((unsigned short)(dwAddr)) 
#    define EmuOutB(dwAddr, byData) _outp ((unsigned short)(dwAddr), (int) (byData))
#    define EmuInB(dwAddr)          _inp  ((unsigned short)(dwAddr))

#    ifdef __BOUND_32BIT
#      define EmuInD(dwAddr)		_inpd((unsigned short)(dwAddr))
#      define EmuOutD(dwAddr, dwData)	_outpd((unsigned short)(dwAddr), (dwData))
#    else
#      define EmuInD(dwAddr)		_indwa((WORD)dwAddr)
#      define EmuOutD(dwAddr, dwData)	_outdwa((WORD)dwAddr, dwData)
#    endif

#  else
#  include <stdio.h>
#  define BNIO "Butt naked I/O"
#    define EmuOut(dwAddr, dwData)	printf("%s: %lx, %lx", BNIO, dwAddr, dwData)
#    define EmuIn(dwAddr)		0xbadd	
#    define EmuOutB(dwAddr, byData) printf("%s: %lx, %lx", BNIO, dwAddr, byData)
#    define EmuInB(dwAddr)		0xaa	
#    define EmuOutD(dwAddr, dwData)	printf("%s: %lx, %lx", BNIO, dwAddr, dwData)
#    define EmuInD(dwAddr)		0xbeefface	

#  endif

#endif

#endif /* __PCIO_H */

