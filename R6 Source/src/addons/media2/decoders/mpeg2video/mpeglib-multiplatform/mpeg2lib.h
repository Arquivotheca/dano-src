#ifndef C_MPEG2_LIB_H

#define C_MPEG2_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SupportDefs.h>

struct complex_t;

void yuv2YCbCr422 (uint8 *dst, const uint8 *y, const uint8 *u, const uint8 *v,
						int32 width_by_8, int32 height_by_2);

void MotionCompensationDispatch (uint8 *dst, const uint8 *src, size_t skip,
									uint8 w, uint8 h,
									int32 x, int32 y, int32 dx, int32 dy,
									bool add);
									
void add_block (uint8 *ptr, const int16 *block, size_t stride);
void copy_block (uint8 *ptr, const int16 *block, size_t stride);

void idct (int16 *block);

void downmix_5_plus_1 (int16 *output, float *channel0,
						const float *center, const float *surround, const float *c);

void multiply_complex(struct complex_t *dst, const struct complex_t *src, int32 count_by_2);
void interleaved_mult_imre(float *dst, const struct complex_t *src1, const struct complex_t *src2, const float *w, int32 count_by_2);
void interleaved_mult_reim(float *dst, const struct complex_t *src1, const struct complex_t *src2, const float *w, int32 count_by_2);

#ifdef __cplusplus
}
#endif

#endif
