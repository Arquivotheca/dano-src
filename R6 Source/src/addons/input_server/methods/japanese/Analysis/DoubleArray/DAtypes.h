/************************************************************************
ファイル名	:	DATypes.h

作成日	:	97.7.27
************************************************************************/

#ifndef __DA_TYPES_HEADER__
#define __DA_TYPES_HEADER__

#include"DoubleArray.h"

#define kData2Bytes			1		//	1の時，データのサイズは２バイトで表す
								//	0の時，１バイト

#ifndef true
#define true	1
#define false	0

typedef unsigned char	Boolean;
#endif

#if kData2Bytes
typedef unsigned short DataType;
#else
typedef unsigned char DataType;
#endif

#endif //__DA_TYPES_HEADER__
