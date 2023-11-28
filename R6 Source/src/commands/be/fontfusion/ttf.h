#ifndef _TTF_H
#define _TTF_H

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <SupportDefs.h>

#define _bad_font_printf1 printf

typedef long sfnt_TableTag;
typedef short FWord;
typedef ushort uFWord;
typedef struct {
	uint32 bc;
	uint32 ad;
} BigDate;

typedef struct {
	sfnt_TableTag       tag;
	uint32              checkSum;
	uint32              offset;
	uint32              length;
} sfnt_DirectoryEntry;

typedef struct {
	int32               version;                  /* 0x10000 (1.0) */
	uint16              numOffsets;              /* number of tables */
	uint16              searchRange;             /* (max2 <= numOffsets)*16 */
	uint16              entrySelector;           /* log2 (max2 <= numOffsets) */
	uint16              rangeShift;              /* numOffsets*16-searchRange*/
	sfnt_DirectoryEntry table[1];   /* table[numOffsets] */
} sfnt_OffsetTable;

typedef struct {
	ushort				version;
	ushort				listCount;
} sfnt_CmapHeader;

typedef struct {
	ushort				platformID;
	ushort				encodingID;
	ulong				offset;
} sfnt_CmapList;

typedef struct {
	uint16		endCount;
	uint16		startCount;
	uint16		idDelta;
	uint16		idRangeOffset;
} fc_cmap_segment;

typedef struct {
	sfnt_TableTag   tag;
	uint32          checkSum;
	void			*buf;
	int32			length;
} font_table;
	
typedef struct {
	sfnt_OffsetTable	offsetTable;
	int32				count;
	font_table			tables[40];
} font_file;

enum {
	tag_prep  = 0x70726570L,        /* 'prep' */
	tag_cvt   = 0x63767420L,        /* 'cvt ' */
	tag_loca  = 0x6c6f6361L,        /* 'loca' */
	tag_glyf  = 0x676c7966L,        /* 'glyf' */
	tag_cmap  = 0x636d6170L,        /* 'cmap' */
	tag_fpgm  = 0x6670676dL,        /* 'fpgm' */
	tag_maxp  = 0x6d617870L			/* 'maxp' */
};

extern uint8 reset_program1[32];
extern uint8 reset_program2[37];
extern uint8 reset_program3[36];
extern uint8 default_program[27];

uint16		fc_read16(void *adr);
uint32		fc_read32(void *adr);
void		fc_write16(void *buf, uint16 value);
void		fc_write32(void *buf, uint32 value);

bool		read_ttf(font_file *ff, FILE *fp);
bool		write_ttf(font_file *ff, FILE *fp);
bool		extract_cmap(char *cmap, fc_cmap_segment **list, int32 *seg_count, uint16 **glyphArray);
uint16		get_glyph_id(fc_cmap_segment *segs, int32 seg_count, uint16 *glyphArray, uint16 value);
uint16		get_unicode(fc_cmap_segment *segs, int32 seg_count, uint16 *glyphArray, uint16 glyph_id);
font_table	*get_table(font_file *ff, int32 tag);
bool		parse_glyf(	uint8 *glyf_ptr, int32 size, uint8 **ptr_array, uint8 **end_array,
						fc_cmap_segment *to_segs, int32 to_seg_count, uint16 *to_glyphArray,
						fc_cmap_segment *from_segs, int32 from_seg_count, uint16 *from_glyphArray);
void		repack_glyf(uint8 **ptr_array, uint8 **end_array, uint8 **output2);
void		maximize_maxp(uint16 *to, uint16 *from);

#endif
