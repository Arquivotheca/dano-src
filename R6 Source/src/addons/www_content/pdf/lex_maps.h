#if !defined(__LEX_MAPS_H_)
#define __LEX_MAPS_H_

#include <be_prim.h>
extern uint32 whitemap[8];
#define InMap(map, x) ((map)[(x) >> 5] & (1 << ((x) & 31)))

#endif
