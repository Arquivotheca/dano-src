#ifndef __CORES_H__
#define __CORES_H__

#include "Bitflinger.h"
#include "asm.h"

extern void __cvMakeIntTables();
extern void __cvMakeMMXTables();

extern void __cvBitGen_x86_Int_Core( cvContext *con, asmContext *c );
extern void __cvBitGen_x86_Float_Core( cvContext *con, asmContext *c );
extern void __cvBitGen_x86_MMX_Core( cvContext *con, asmContext *c );


#endif
