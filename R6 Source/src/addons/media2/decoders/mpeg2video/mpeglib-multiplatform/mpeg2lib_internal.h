#ifndef C_MPEG2_LIB_INTERNAL_H

#define C_MPEG2_LIB_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SupportDefs.h>
void yuv2YCbCr422_inner (const uint8 *y, const uint8 *u, const uint8 *v,
							uint8 *dest, int width_by_8);

#ifdef __cplusplus
}
#endif

#endif
