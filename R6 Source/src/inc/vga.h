//*****************************************************************************
//
//	File:		vga.h
//
//	Description:
//
//	Written by:	Robert Polic
//
//	Copyright 1995, Be Incorporated
//
//	Change History:
//
//
//******************************************************************************/

#ifndef _VGAH_
#define _VGAH_

#ifndef _SUPPORT_DEFS_H
#include <SupportDefs.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif

#include <GraphicsDefs.h>

// A BRUTAL hack...
//#define frame_buffer_info old_frame_buffer_info
#include <GraphicsCard.h>
//#undef frame_buffer_info

#define	BUS_SIZE		4		// bus size in bytes

#define vga640x480		1		// resolutions
#define vga800x600		2
#define vga1024x768		3
#define vga1280x1024    4
#define vga1600x1200    5
#define vga640x400      99

#define USE_PATTERN		0x80
#define	COPY_8			0x00
#define	COPY_PAT_8		COPY_8 | USE_PATTERN
#define	AND_8			0x01
#define	AND_PAT_8		AND_8 | USE_PATTERN
#define OR_8			0x02
#define	OR_PAT_8		OR_8 | USE_PATTERN
#define	COPY_24			0x03
#define	COPY_PAT_24		COPY_24 | USE_PATTERN
#define	AND_24			0x04
#define	AND_PAT_24		AND_24 | USE_PATTERN
#define OR_24			0x05
#define	OR_PAT_24		OR_24 | USE_PATTERN

typedef struct {
	long     res;
	long     version;
	float    rates[32];
	uchar    HPosition[32];
	uchar    HSize[32];
	uchar    VPosition[32];
	uchar    VSize[32];
} vga_settings;

typedef struct {
    area_id		memory_area;
    area_id		io_area;
	char		path[64+B_FILE_NAME_LENGTH];
} direct_screen_info;

#define INDEX_SET_CURSOR_SHAPE  0
#define INDEX_MOVE_CURSOR       1
#define INDEX_SHOW_CURSOR       2
#define INDEX_LINE_8            3
#define INDEX_LINE_24           4

#define INDEX_RECT_8            5
#define INDEX_RECT_24           6
#define INDEX_BLIT              7
#define INDEX_ARRAY_LINE_8      8
#define INDEX_ARRAY_LINE_24     9

#define INDEX_SYNCHRO           10
#define INDEX_INVERT_RECT       11

#define INDEX_LINE_16       	12
#define INDEX_RECT_16       	13
#define INDEX_ARRAY_LINE_16     14

typedef long (*VGA_CTRL)(ulong,void *);

typedef long (*VGA_SET_CURSOR_SHAPE)(uchar *,uchar *,long,long,long,long);
typedef long (*VGA_MOVE_CURSOR)(long,long);
typedef long (*VGA_SHOW_CURSOR)(long);
typedef long (*VGA_LINE_8)(long,long,long,long,uchar,bool,short,short,short,short);
typedef long (*VGA_LINE_24)(long,long,long,long,ulong,bool,short,short,short,short);
typedef long (*VGA_RECT_8)(long,long,long,long,uchar);
typedef long (*VGA_RECT_24)(long,long,long,long,ulong);
typedef long (*VGA_BLIT)(long,long,long,long,long,long);
typedef long (*VGA_ARRAY_LINE_8)(rgb_color_line*,long,bool,short,short,short,short);
typedef long (*VGA_ARRAY_LINE_24)(indexed_color_line*,long,bool,short,short,short,short);
typedef long (*VGA_SYNCHRO)(void);
typedef long (*VGA_INVERT_RECT_24)(long,long,long,long);
typedef long (*VGA_LINE_16)(long,long,long,long,ulong,bool,short,short,short,short);
typedef long (*VGA_RECT_16)(long,long,long,long,ulong);
typedef long (*VGA_ARRAY_LINE_16)(indexed_color_line*,long,bool,short,short,short,short);

#ifdef __cplusplus
extern "C" {
#endif

extern VGA_SET_CURSOR_SHAPE   set_cursor_shape_jmp;
extern VGA_MOVE_CURSOR        move_cursor_jmp;
extern VGA_SHOW_CURSOR        show_cursor_jmp;
extern VGA_LINE_8             line_8_jmp;
extern VGA_LINE_24            line_24_jmp;
extern VGA_RECT_8             rect_8_jmp;
extern VGA_RECT_24            rect_24_jmp;
extern VGA_BLIT               blit_jmp;
extern VGA_ARRAY_LINE_8       array_line_8_jmp;
extern VGA_ARRAY_LINE_24      array_line_24_jmp;
extern VGA_SYNCHRO            synchro_jmp;
extern VGA_INVERT_RECT_24     invert_rect_24_jmp;

#ifdef __cplusplus
}
#endif


#endif












