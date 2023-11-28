/* :ts=8 bk=0
 *
 * gfxbuf.h:	Definitions for graphics buffers (surfaces?) and how to
 *		allocate them.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.10.12
 */
#ifndef _GFXBUF_H
#define	_GFXBUF_H

#ifndef _GRAPHIC_DRIVER_H_
#include <device/graphic_driver.h>
#endif
#ifndef _GENPOOL_H
#include <surface/genpool.h>
#endif


/*****************************************************************************
 * Graphics buffer definition.  (Surface?)
 *
 * This attempts to describe a graphics buffer in a hardware-independent
 * manner (and probably fails misterably).  The client requests the buffer
 * dimensions, type, layout, and intended usage, and the driver hands back a
 * filled-out structure pointing to allocated memory having characteristics
 * appropriate for the device and usage.
 */
typedef struct GfxBuf {
	/*  Set these fields to describe the kind of buffer you want.  */
	uint32		gb_ColorSpace;
	uint16		gb_Width;	/*  In pixels			*/
	uint16		gb_Height;
	uint8		gb_Layout;	/*  packed_linear, tiled, etc.	*/
	uint8		gb_Usage;	/*  Display, texture, overlay...*/
	uint8		gb_NMips;	/*  # of mipmaps for textures	*/
	uint8		__pad;
	void		*gb_ExtraData;	/*  In case the driver needs it	*/

	/*  Set by allocator based on above fields; don't touch.  */
	BMemSpec	gb_Alloc;
	volatile uint8	*gb_BaseAddr;	/*  32-bit CPU address		*/
	uint32		gb_BaseAddr_PCI;/*  PCI address			*/
	uint32		gb_BaseAddr_AGP;/*  AGP address			*/
	uint16		gb_FullWidth;	/*  HW-quantized dimensions	*/
	uint16		gb_FullHeight;
	uint16		gb_BytesPerRow;	/*  Meaningless in tiled bufs	*/
	uint8		gb_PixelBytes;
	uint8		gb_PixelBits;	/*  Depth  */
} GfxBuf;
#define	gb_BaseOffset	gb_Alloc.ms_MR.mr_Offset
#define	gb_Size		gb_Alloc.ms_MR.mr_Size

/*  gb_Usage bit definitions  */
#define	GFXBUF_USAGE_DISPLAY		1
#define	GFXBUF_USAGE_OVERLAY		(1<<1)
#define	GFXBUF_USAGE_2DSRC		(1<<2)
#define	GFXBUF_USAGE_2DDEST		(1<<3)
#define	GFXBUF_USAGE_3D_TEXTURE		(1<<4)
#define	GFXBUF_USAGE_3D_COLORBUF	(1<<5)
#define	GFXBUF_USAGE_3D_ZBUF		(1<<6)
#define	GFXBUF_USAGE_CMDBUF		(1<<7)	/*  Mutually exclusive	*/

/*  Valid ONLY when ORed in with GFXBUF_USAGE_CMDBUF  */
#define	GFXBUF_CMDBUF_BATCH		0
#define	GFXBUF_CMDBUF_RING		1

/*  gb_Layout definitions  */
#define	GFXBUF_LAYOUT_PACKEDLINEAR	1
#define	GFXBUF_LAYOUT_I810TILED		2


/*  Driver ioctl()s; common to all graphics drivers supporting GfxBufs  */
enum gfxbuf_ioctls {
	GFX_IOCTL_ALLOCBYMEMSPEC = B_GRAPHIC_DRIVER_PRIVATE_BASE,
	GFX_IOCTL_FREEBYMEMSPEC,
	GFX_IOCTL_FORMATMEMSPEC,
	GFX_IOCTL_ALLOCGFXBUF,		/*  Implies _FORMATMEMSPEC	*/
	GFX_IOCTL_FREEGFXBUF,		/*  NULLs gb_BaseAddr field	*/
	MAXIOCTL_GFXBUF
};


#endif	/*  _GFXBUF_H  */
