#include "mpeg2lib.h"
#include "mpeg2lib_internal.h"
#include "complex.h"

#include <Debug.h>

#if _PROCESSOR_MMX_

#define	mmx_ri(op,reg,imm) \
	asm (#op " %0, %%" #reg \
			      : /* nothing */ \
			      : "X" (imm) )

#define	mmx_rm(op,reg,mem) \
	asm (#op " %0, %%" #reg \
			      : /* nothing */ \
			      : "X" (mem))

#define	mmx_mr(op,mem,reg) \
	asm (#op " %%" #reg ", %0" \
			      : "=X" (mem) \
			      : /* nothing */ )

#define	mmx_rr(op,regd,regs) \
	asm (#op " %" #regs ", %" #regd)


#define movq_rm(reg,mem) mmx_rm(movq,reg,mem)
#define movq_mr(mem,reg) mmx_mr(movq,mem,reg)

#if _PROCESSOR_CMOV_ // this should be something like _PROCESSOR_MMX_EXTENSIONS_ instead
#define pavgusb_rm(reg,mem) mmx_rm(pavgb,reg,mem)
#define pavgusb_rr(regd,regs) mmx_rr(pavgb,regd,regs)
#elif _PROCESSOR_3DNOW_ 
#define pavgusb_rm(reg,mem) mmx_rm(pavgusb,reg,mem)
#define pavgusb_rr(regd,regs) mmx_rr(pavgusb,regd,regs)
#else
#error "cannot deal without MMX-extensions yet"
#endif

#define MotionCompensationCopy_x_y(w,h)									\
void Copy_##w##x##h##_x_y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		movq_rm(mm0,*src);												\
																		\
		if (w==16)														\
			movq_rm(mm1,*(src+8));										\
																		\
		src+=skip;														\
																		\
		movq_mr(*dst,mm0);												\
																		\
		if (w==16)														\
			movq_mr(*(dst+8),mm1);										\
																		\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationCopy_X_y(w,h)									\
void Copy_##w##x##h##_X_y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		movq_rm(mm0,*src);												\
		pavgusb_rm(mm0,*(src+1));										\
																		\
		if (w==16)														\
		{																\
			movq_rm(mm1,*(src+8));										\
			pavgusb_rm(mm1,*(src+8+1));									\
		}																\
																		\
		src+=skip;														\
																		\
		movq_mr(*dst,mm0);												\
																		\
		if (w==16)														\
			movq_mr(*(dst+8),mm1);										\
																		\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationCopy_x_Y(w,h)									\
void Copy_##w##x##h##_x_Y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	if (w==16)															\
		skip-=8;														\
																		\
	movq_rm(mm0,*src);													\
																		\
	if (w==16)															\
	{																	\
		src+=8;															\
		movq_rm(mm1,*src);												\
	}																	\
																		\
	src+=skip;															\
																		\
	for (uint8 i=0;i<h/2;++i)											\
	{																	\
		movq_rm(mm2,*src);												\
		pavgusb_rr(mm0,mm2);											\
																		\
		if (w==16)														\
		{																\
			src+=8;														\
			movq_rm(mm3,*src);											\
			pavgusb_rr(mm1,mm3);										\
		}																\
																		\
		src+=skip;														\
																		\
		movq_mr(*dst,mm0);												\
																		\
		if (w==16)														\
		{																\
			dst+=8;														\
			movq_mr(*dst,mm1);											\
		}																\
																		\
		dst+=skip;														\
																		\
		movq_rm(mm0,*src);												\
		pavgusb_rr(mm2,mm0);											\
																		\
		if (w==16)														\
		{																\
			src+=8;														\
			movq_rm(mm1,*src);											\
			pavgusb_rr(mm3,mm1);										\
		}																\
																		\
		src+=skip;														\
																		\
		movq_mr(*dst,mm2);												\
																		\
		if (w==16)														\
		{																\
			dst+=8;														\
			movq_mr(*dst,mm3);											\
		}																\
																		\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationCopy_X_Y(w,h)									\
void Copy_##w##x##h##_X_Y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	if (w==16)															\
		skip-=8;														\
																		\
	movq_rm(mm0,*src);													\
	pavgusb_rm(mm0,*(src+1));											\
																		\
	if (w==16)															\
	{																	\
		src+=8;															\
		movq_rm(mm1,*src);												\
		pavgusb_rm(mm1,*(src+1));										\
	}																	\
																		\
	src+=skip;															\
																		\
	for (uint8 i=0;i<h/2;++i)											\
	{																	\
		movq_rm(mm2,*src);												\
		pavgusb_rm(mm2,*(src+1));										\
		pavgusb_rr(mm0,mm2);											\
																		\
		if (w==16)														\
		{																\
			src+=8;														\
			movq_rm(mm3,*src);											\
			pavgusb_rm(mm3,*(src+1));									\
			pavgusb_rr(mm1,mm3);										\
		}																\
																		\
		src+=skip;														\
																		\
		movq_mr(*dst,mm0);												\
																		\
		if (w==16)														\
		{																\
			dst+=8;														\
			movq_mr(*dst,mm1);											\
		}																\
																		\
		dst+=skip;														\
																		\
		movq_rm(mm0,*src);												\
		pavgusb_rm(mm0,*(src+1));										\
		pavgusb_rr(mm2,mm0);											\
																		\
		if (w==16)														\
		{																\
			src+=8;														\
			movq_rm(mm1,*src);											\
			pavgusb_rm(mm1,*(src+1));									\
			pavgusb_rr(mm3,mm1);										\
		}																\
																		\
		src+=skip;														\
																		\
		movq_mr(*dst,mm2);												\
																		\
		if (w==16)														\
		{																\
			dst+=8;														\
			movq_mr(*dst,mm3);											\
		}																\
																		\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationAverage_x_y(w,h)									\
void Average_##w##x##h##_x_y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		movq_rm(mm0,*src);												\
																		\
		if (w==16)														\
			movq_rm(mm1,*(src+8));										\
																		\
		src+=skip;														\
																		\
		pavgusb_rm(mm0,*dst);											\
		movq_mr(*dst,mm0);												\
																		\
		if (w==16)														\
		{																\
			pavgusb_rm(mm1,*(dst+8));									\
			movq_mr(*(dst+8),mm1);										\
		}																\
																		\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationAverage_X_y(w,h)									\
void Average_##w##x##h##_X_y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		movq_rm(mm0,*src);												\
		pavgusb_rm(mm0,*(src+1));										\
																		\
		if (w==16)														\
		{																\
			movq_rm(mm1,*(src+8));										\
			pavgusb_rm(mm1,*(src+8+1));									\
		}																\
																		\
		src+=skip;														\
																		\
		pavgusb_rm(mm0,*dst);											\
		movq_mr(*dst,mm0);												\
																		\
		if (w==16)														\
		{																\
			pavgusb_rm(mm1,*(dst+8));									\
			movq_mr(*(dst+8),mm1);										\
		}																\
																		\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationAverage_x_Y(w,h)									\
void Average_##w##x##h##_x_Y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	if (w==16)															\
		skip-=8;														\
																		\
	movq_rm(mm0,*src);													\
																		\
	if (w==16)															\
	{																	\
		src+=8;															\
		movq_rm(mm1,*src);												\
	}																	\
																		\
	src+=skip;															\
																		\
	for (uint8 i=0;i<h/2;++i)											\
	{																	\
		movq_rm(mm2,*src);												\
		pavgusb_rr(mm0,mm2);											\
																		\
		if (w==16)														\
		{																\
			src+=8;														\
			movq_rm(mm3,*src);											\
			pavgusb_rr(mm1,mm3);										\
		}																\
																		\
		src+=skip;														\
																		\
		pavgusb_rm(mm0,*dst);											\
		movq_mr(*dst,mm0);												\
																		\
		if (w==16)														\
		{																\
			dst+=8;														\
			pavgusb_rm(mm1,*dst);										\
			movq_mr(*dst,mm1);											\
		}																\
																		\
		dst+=skip;														\
																		\
		movq_rm(mm0,*src);												\
		pavgusb_rr(mm2,mm0);											\
																		\
		if (w==16)														\
		{																\
			src+=8;														\
			movq_rm(mm1,*src);											\
			pavgusb_rr(mm3,mm1);										\
		}																\
																		\
		src+=skip;														\
																		\
		pavgusb_rm(mm2,*dst);											\
		movq_mr(*dst,mm2);												\
																		\
		if (w==16)														\
		{																\
			dst+=8;														\
			pavgusb_rm(mm3,*dst);										\
			movq_mr(*dst,mm3);											\
		}																\
																		\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationAverage_X_Y(w,h)									\
void Average_##w##x##h##_X_Y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	if (w==16)															\
		skip-=8;														\
																		\
	movq_rm(mm0,*src);													\
	pavgusb_rm(mm0,*(src+1));											\
																		\
	if (w==16)															\
	{																	\
		src+=8;															\
		movq_rm(mm1,*src);												\
		pavgusb_rm(mm1,*(src+1));										\
	}																	\
																		\
	src+=skip;															\
																		\
	for (uint8 i=0;i<h/2;++i)											\
	{																	\
		movq_rm(mm2,*src);												\
		pavgusb_rm(mm2,*(src+1));										\
		pavgusb_rr(mm0,mm2);											\
																		\
		if (w==16)														\
		{																\
			src+=8;														\
			movq_rm(mm3,*src);											\
			pavgusb_rm(mm3,*(src+1));									\
			pavgusb_rr(mm1,mm3);										\
		}																\
																		\
		src+=skip;														\
																		\
		pavgusb_rm(mm0,*dst);											\
		movq_mr(*dst,mm0);												\
																		\
		if (w==16)														\
		{																\
			dst+=8;														\
			pavgusb_rm(mm1,*dst);										\
			movq_mr(*dst,mm1);											\
		}																\
																		\
		dst+=skip;														\
																		\
		movq_rm(mm0,*src);												\
		pavgusb_rm(mm0,*(src+1));										\
		pavgusb_rr(mm2,mm0);											\
																		\
		if (w==16)														\
		{																\
			src+=8;														\
			movq_rm(mm1,*src);											\
			pavgusb_rm(mm1,*(src+1));									\
			pavgusb_rr(mm3,mm1);										\
		}																\
																		\
		src+=skip;														\
																		\
		pavgusb_rm(mm2,*dst);											\
		movq_mr(*dst,mm2);												\
																		\
		if (w==16)														\
		{																\
			dst+=8;														\
			pavgusb_rm(mm3,*dst);										\
			movq_mr(*dst,mm3);											\
		}																\
																		\
		dst+=skip;														\
	}																	\
}																		\

static void Copy_16x16_x_y_HandTuned (uint8 *dst, const uint8 *src, size_t skip)
{
	skip-=8;

	for (int32 i=0;i<4;++i)
	{	
		movq_rm(mm0,*src);
		src+=8;
		movq_rm(mm1,*src);
		src+=skip;
		movq_rm(mm2,*src);
		src+=8;
		movq_rm(mm3,*src);
		src+=skip;	
		movq_rm(mm4,*src);
		src+=8;
		movq_rm(mm5,*src);
		src+=skip;	
		movq_rm(mm6,*src);
		src+=8;
		movq_rm(mm7,*src);
		src+=skip;

		movq_mr(*dst,mm0);
		dst+=8;
		movq_mr(*dst,mm1);
		dst+=skip;
		movq_mr(*dst,mm2);
		dst+=8;
		movq_mr(*dst,mm3);
		dst+=skip;
		movq_mr(*dst,mm4);
		dst+=8;
		movq_mr(*dst,mm5);
		dst+=skip;
		movq_mr(*dst,mm6);
		dst+=8;
		movq_mr(*dst,mm7);
		dst+=skip;
	}
}

static void Copy_8x8_x_y_HandTuned (uint8 *dst, const uint8 *src, size_t skip)
{
	movq_rm(mm0,*src);
	src+=skip;
	movq_rm(mm1,*src);
	src+=skip;
	movq_rm(mm2,*src);
	src+=skip;	
	movq_rm(mm3,*src);
	src+=skip;
	movq_rm(mm4,*src);
	src+=skip;	
	movq_rm(mm5,*src);
	src+=skip;
	movq_rm(mm6,*src);
	src+=skip;
	movq_rm(mm7,*src);

	movq_mr(*dst,mm0);
	dst+=skip;
	movq_mr(*dst,mm1);
	dst+=skip;
	movq_mr(*dst,mm2);
	dst+=skip;
	movq_mr(*dst,mm3);
	dst+=skip;
	movq_mr(*dst,mm4);
	dst+=skip;
	movq_mr(*dst,mm5);
	dst+=skip;
	movq_mr(*dst,mm6);
	dst+=skip;
	movq_mr(*dst,mm7);
	dst+=skip;
}

static void Copy_16x16_X_Y_HandTuned (uint8 *dst, const uint8 *src, size_t skip)
{
	skip-=8;

	movq_rm(mm0,*src);
	pavgusb_rm(mm0,*(src+1));
	src+=8;
	movq_rm(mm1,*src);
	pavgusb_rm(mm1,*(src+1));
	src+=skip;
		
	for (int32 i=0;i<8;++i)
	{	
		movq_rm(mm2,*src);
		pavgusb_rm(mm2,*(src+1));
		pavgusb_rr(mm0,mm2);		
		src+=8;
		movq_rm(mm3,*src);
		pavgusb_rm(mm3,*(src+1));
		pavgusb_rr(mm1,mm3);		
		src+=skip;

		movq_mr(*dst,mm0);
		dst+=8;
		movq_mr(*dst,mm1);
		dst+=skip;
		
		movq_rm(mm0,*src);
		pavgusb_rm(mm0,*(src+1));
		pavgusb_rr(mm2,mm0);		
		src+=8;
		movq_rm(mm1,*src);
		pavgusb_rm(mm1,*(src+1));
		pavgusb_rr(mm3,mm1);		
		src+=skip;

		movq_mr(*dst,mm2);
		dst+=8;
		movq_mr(*dst,mm3);
		dst+=skip;
	}
}

#else // !_PROCESSOR_MMX_

void idct (int16 *)
{
#warning * IMPLEMENT idct for non-MMX systems
}

void 
yuv2YCbCr422 (uint8 *, const uint8 *, const uint8 *, const uint8 *,
				int32 , int32 )
{
#warning * IMPLEMENT yuv2YCbCr422 for non-MMX systems
}

#define MotionCompensationCopy_x_y(w,h)									\
void Copy_##w##x##h##_x_y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		for (uint8 j=0;j<w;++j)											\
			dst[j]=src[j];												\
																		\
		src+=skip;														\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationCopy_X_y(w,h)									\
void Copy_##w##x##h##_X_y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		for (uint8 j=0;j<w;++j)											\
			dst[j]=(uint16(src[j])+uint16(src[j+1])+1)>>1;								\
																		\
		src+=skip;														\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationCopy_x_Y(w,h)									\
void Copy_##w##x##h##_x_Y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		for (uint8 j=0;j<w;++j)											\
			dst[j]=(uint16(src[j])+uint16(src[j+skip])+1)>>1;							\
																		\
		src+=skip;														\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationCopy_X_Y(w,h)									\
void Copy_##w##x##h##_X_Y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		for (uint8 j=0;j<w;++j)											\
			dst[j]=(uint16(src[j])+uint16(src[j+1])						\
					+ uint16(src[j+skip])+uint16(src[j+skip+1])+2)>>2;	\
																		\
		src+=skip;														\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationAverage_x_y(w,h)									\
void Average_##w##x##h##_x_y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		for (uint8 j=0;j<w;++j)											\
			dst[j]=(uint16(dst[j])+uint16(src[j])+1)>>1;								\
																		\
		src+=skip;														\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationAverage_X_y(w,h)									\
void Average_##w##x##h##_X_y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		for (uint8 j=0;j<w;++j)											\
			dst[j]=(uint16(dst[j])+((uint16(src[j])+uint16(src[j+1])+1)>>1)+1)>>1;				\
																		\
		src+=skip;														\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationAverage_x_Y(w,h)									\
void Average_##w##x##h##_x_Y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		for (uint8 j=0;j<w;++j)											\
			dst[j]=(uint16(dst[j])+((uint16(src[j])+uint16(src[j+skip])+1)>>1)+1)>>1;			\
																		\
		src+=skip;														\
		dst+=skip;														\
	}																	\
}																		\

#define MotionCompensationAverage_X_Y(w,h)									\
void Average_##w##x##h##_X_Y (uint8 *dst, const uint8 *src, size_t skip)	\
{																		\
	for (uint8 i=0;i<h;++i)												\
	{																	\
		for (uint8 j=0;j<w;++j)											\
			dst[j]=(uint16(dst[j])								\
						+((uint16(src[j])+uint16(src[j+1])				\
						+uint16(src[j+skip])+uint16(src[j+skip+1])+2)>>2) + 1)>>1;	\
																		\
		src+=skip;														\
		dst+=skip;														\
	}																	\
}																		\

void 
add_block(uint8 *ptr, const int16 *block, size_t stride)
{
	for (uint8 i=0;i<8;++i)
	{
		for (uint8 j=0;j<8;++j)
		{
			int16 val=int16(ptr[j])+(*block++);
			
			if (val<0)
				val=0;
			else if (val>255)
				val=255;
			
			ptr[j]=val;
		}
		
		ptr+=stride;
	}
}

void 
copy_block(uint8 *ptr, const int16 *block, size_t stride)
{
	for (uint8 i=0;i<8;++i)
	{
		for (uint8 j=0;j<8;++j)
		{
			int16 val=*block++;
			
			if (val<0)
				val=0;
			else if (val>255)
				val=255;
			
			ptr[j]=val;
		}
		
		ptr+=stride;
	}
}

#endif

#define MotionCompensationCopy(w,h)		\
MotionCompensationCopy_x_y(w,h) 		\
MotionCompensationCopy_X_y(w,h) 		\
MotionCompensationCopy_x_Y(w,h) 		\
MotionCompensationCopy_X_Y(w,h) 		\

#define MotionCompensationAverage(w,h)	\
MotionCompensationAverage_x_y(w,h) 		\
MotionCompensationAverage_X_y(w,h) 		\
MotionCompensationAverage_x_Y(w,h) 		\
MotionCompensationAverage_X_Y(w,h) 		\

#define MotionCompensation(w,h)			\
MotionCompensationCopy(w,h)				\
MotionCompensationAverage(w,h)			\

MotionCompensation(8,4)
MotionCompensation(16,4)

MotionCompensation(8,8)
MotionCompensation(16,8)

MotionCompensation(8,16)
MotionCompensation(16,16)

typedef void (*MotionCompensationFunc)(uint8 *, const uint8 *, size_t);

void
MotionCompensationDispatch (uint8 *dst, const uint8 *src, size_t skip,
					uint8 w, uint8 h,
					int32 x, int32 y, int32 dx, int32 dy, bool add)
{
	const uint8 *s = src + skip * (y + (dy >> 1)) + x + (dx >> 1);
	uint8 *d = dst + skip * y + x;

	if (add)
	{
		static const MotionCompensationFunc table[24] =
		{
			Average_8x4_x_y,
			Average_16x4_x_y,
			Average_8x4_X_y,
			Average_16x4_X_y,
			Average_8x4_x_Y,
			Average_16x4_x_Y,
			Average_8x4_X_Y,
			Average_16x4_X_Y,

			Average_8x8_x_y,
			Average_16x8_x_y,
			Average_8x8_X_y,
			Average_16x8_X_y,
			Average_8x8_x_Y,
			Average_16x8_x_Y,
			Average_8x8_X_Y,
			Average_16x8_X_Y,

			Average_8x16_x_y,
			Average_16x16_x_y,
			Average_8x16_X_y,
			Average_16x16_X_y,
			Average_8x16_x_Y,
			Average_16x16_x_Y,
			Average_8x16_X_Y,
			Average_16x16_X_Y,
		};

		int32 index=w|((dx&1)<<1)|((dy&1)<<2)|((h>>3)<<3);
		
		(*(table[index]))(d,s,skip);
	}
	else
	{
		static const MotionCompensationFunc table[24] =
		{
			Copy_8x4_x_y,
			Copy_16x4_x_y,
			Copy_8x4_X_y,
			Copy_16x4_X_y,
			Copy_8x4_x_Y,
			Copy_16x4_x_Y,
			Copy_8x4_X_Y,
			Copy_16x4_X_Y,

#if _PROCESSOR_MMX_
			Copy_8x8_x_y_HandTuned,
#else			
			Copy_8x8_x_y,
#endif

			Copy_16x8_x_y,
			Copy_8x8_X_y,
			Copy_16x8_X_y,
			Copy_8x8_x_Y,
			Copy_16x8_x_Y,
			Copy_8x8_X_Y,
			Copy_16x8_X_Y,

			Copy_8x16_x_y,

#if _PROCESSOR_MMX_
			Copy_16x16_x_y_HandTuned,
#else
			Copy_16x16_x_y,
#endif

			Copy_8x16_X_y,
			Copy_16x16_X_y,
			Copy_8x16_x_Y,
			Copy_16x16_x_Y,
			Copy_8x16_X_Y,

#if _PROCESSOR_MMX_
			Copy_16x16_X_Y_HandTuned
#else
			Copy_16x16_X_Y
#endif
		};

		int32 index=w|((dx&1)<<1)|((dy&1)<<2)|((h>>3)<<3);
		
		(*(table[index]))(d,s,skip);
	}
}

#if __PROCESSOR_KATMAI__==0

void 
downmix_5_plus_1 (int16 *output, float *channel0,
					const float *center, const float *surround, const float *c)
{
	for (uint16 i=0;i<256;++i)
	{
		*output++=int16((*c)*((channel0[i]
					+channel0[512+i]*(*center)
					+channel0[512*3+i]*(*surround)
					+channel0[512*5+i]*0.707)));
	
		*output++=int16((*c)*((channel0[512*2+i]
					+channel0[512+i]*(*center)
					+channel0[512*4+i]*(*surround)
					+channel0[512*5+i]*0.707)));
	}
}

void multiply_complex (complex_t *dst, const complex_t *src, int32 count_by_2)
{
	count_by_2*=2;
	for (int32 i=0;i<count_by_2;++i)
		dst[i]*=src[i];
}

void interleaved_mult_imre (float *dst,
								const complex_t *src1,
								const complex_t *src2,
								const float *w,
								int32 count_by_2)
{
	count_by_2*=2;
	
	for (int32 i=0;i<count_by_2;++i)
	{
		dst[2*i]=-src1[i].im*w[2*i];
		dst[2*i+1]=src2[-i].re*w[2*i+1];
	}
}

void interleaved_mult_reim (float *dst,
								const complex_t *src1,
								const complex_t *src2,
								const float *w,
								int32 count_by_2)
{
	count_by_2*=2;
	
	for (int32 i=0;i<count_by_2;++i)
	{
		dst[2*i]=-src1[i].re*w[2*i];
		dst[2*i+1]=src2[-i].im*w[2*i+1];
	}
}
								
#endif
