#if !defined(__L2TABLE_H__)
#define __L2TABLE_H__

#include "mpeg.h"

// bit-allocation tables (layer 2)

struct sb_alloc_entry {
	unsigned int    steps;
	unsigned int    bits;
	unsigned int    group;
	unsigned int    quant;
};

struct sb_alloc_table {
	int             sblimit;
	sb_alloc_entry  data[SBLIMIT][16];
};

extern const sb_alloc_table sBitAllocTables[5];

// dequant/denormalize tables (layer 2)

#if defined(ENABLE_FLOATING_POINT)
extern const double sDequantOffset[17];
extern const double sDequantScale[17];
extern const double sDenormalizeScale[64];
#endif

#if defined(ENABLE_FIXED_POINT)
extern const int32 sDequantOffsetInt[17];
extern const int32 sDequantScaleInt[17];
extern const int32 sDenormalizeScaleInt[64];
#endif

#endif
