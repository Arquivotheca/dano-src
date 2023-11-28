#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "iel.h"


unsigned long IEL_t1, IEL_t2, IEL_t3, IEL_t4;
U32  IEL_tempc;
U64  IEL_et1, IEL_et2;
U128 IEL_ext1, IEL_ext2, IEL_ext3, IEL_ext4, IEL_ext5;
S128 IEL_ts1, IEL_ts2;

U128 IEL_POSINF = IEL_CONST128(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x7fffffff);
U128 IEL_NEGINF = IEL_CONST128( 0,  0,  0, 0x80000000);
U128 IEL_MINUS1 = IEL_CONST128(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF); /* added by myself, since there are
																				   references to the variable in
																				   iel.h (iel.h.base).              */
