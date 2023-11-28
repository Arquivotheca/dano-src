#include "ttf.h"

uint8 reset_program1[32] = {
	/* load the constants for the other instructions (in reverse order) */
	0xb9, 0x02, 0x80, 0x01, 0x64,
	0x40, 0x0a, 0x80, 0x40, 0x09, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* the main program */
	0x18, 0x4d, 0x01, 0x10, 0x11, 0x12, 0x1f, 0x1e, 0x16, 0x5f, 0x5e, 0x1a, 0x7e, 0x85, 0x1d 
};

uint8 reset_program2[37] = {
	0x01,
	0xb0, 0x00, 0x10,
	0xb0, 0x00, 0x11,
	0xb0, 0x00, 0x12,
	0xb0, 0x01, 0x16,
	0xb0, 0x40, 0x1a,
	0x18,	
	0xb0, 0x44, 0x1d,
	0xb0, 0x00, 0x1e,
	0xb0, 0x00, 0x1f,
	0x4d,
	0xb0, 0x09, 0x5e,
	0xb0, 0x03, 0x5f,
	0xb8, 0x01, 0x64, 0x85
};

uint8 reset_program3[36] = {
	0x01,
	0xb0, 0x00, 0x10,
	0xb0, 0x00, 0x11,
	0xb0, 0x00, 0x12,
	0xb0, 0x01, 0x16,
	0xb0, 0x40, 0x1a,
	0x18,
	0xb0, 0x44, 0x1d,
	0xb0, 0x00, 0x1e,
	0xb0, 0x00, 0x1f,
	0x4d,
	0xb0, 0x09, 0x5e,
	0xb0, 0x03, 0x5f,
	0xb0, 0x00, 0x85
};

uint8 default_program[27] = {
	/* NPUSHB to load the constants for the other instructions (reverse order) */
	0x40, 0x0b, 0x40, 0x01, 0x03, 0x09, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00,
	/* the main program */
	0x01, 0x10, 0x11, 0x12, 0x18, 0x4d, 0x1d, 0x85, 0x1e, 0x1f, 0x5e, 0x5f, 0x16, 0x1a 
};


uint16 fc_read16(void *adr) {
	return (((ulong)(((uchar*)adr)[0])<<8)+((ulong)((uchar*)adr)[1]));
}

uint32 fc_read32(void *adr) {
	return (((ulong)(((uchar*)adr)[0])<<24)+(((ulong)((uchar*)adr)[1])<<16)+
			((ulong)(((uchar*)adr)[2])<<8)+(ulong)(((uchar*)adr)[3]));
}
	
void fc_write16(void *buf, uint16 value) {
	((uint8*)buf)[0] = (value>>8) & 0xff;
	((uint8*)buf)[1] = value & 0xff;
}

void fc_write32(void *buf, uint32 value) {
	((uint8*)buf)[0] = (value>>24) & 0xff;
	((uint8*)buf)[1] = (value>>16) & 0xff;
	((uint8*)buf)[2] = (value>>8) & 0xff;
	((uint8*)buf)[3] = value & 0xff;
}

bool read_ttf(font_file *ff, FILE *fp)
{					   
	int 				  i;
  	sfnt_DirectoryEntry   table;
	uint16				  cTables;
	int					  curseek;
	
  	/* First off, read the initial directory header on the TTF.  We're only
	   interested in the "numOffsets" variable to tell us how many tables
	   are present in this file.  */
	if (fseek(fp, 0, SEEK_SET) != 0) {
		_bad_font_printf1("##### ERROR ##### 1\n");
		return false;
	}
	if (fread((char *)&ff->offsetTable, 1, sizeof(ff->offsetTable) - sizeof(sfnt_DirectoryEntry), fp) !=
		sizeof(ff->offsetTable) - sizeof(sfnt_DirectoryEntry)) {
		_bad_font_printf1("##### ERROR ##### 2\n");
		return false;
	}
  	cTables = (ushort)fc_read16(&ff->offsetTable.numOffsets);
	ff->count = cTables;
	
	for (i = 0; i < cTables; i++) {
		if (fread((char *)&table, 1, sizeof(table), fp) != sizeof(table)) {
			_bad_font_printf1("##### ERROR ##### 3\n");
			goto abort_all;
		}
		curseek = ftell(fp);
		ff->tables[i].length = fc_read32(&table.length);
		ff->tables[i].tag = table.tag;
		ff->tables[i].checkSum = table.checkSum;
		ff->tables[i].buf = (void*)malloc(ff->tables[i].length);
		if (ff->tables[i].buf == NULL) {
			_bad_font_printf1("##### ERROR ##### 4\n");
			goto abort_all;
		}
		if (fseek(fp, fc_read32(&table.offset), SEEK_SET) != 0) {
			_bad_font_printf1("##### ERROR ##### 5\n");
			goto abort_all;
		}
		if (fread((char *)ff->tables[i].buf, 1, ff->tables[i].length, fp) != ff->tables[i].length) {
			_bad_font_printf1("##### ERROR ##### 6\n");
			goto abort_all;
		}
		if (fseek(fp, curseek, SEEK_SET) != 0) {
			_bad_font_printf1("##### ERROR ##### 7\n");
			goto abort_all;
		}
	}
	return true;
abort_all:
	return false;
}

bool write_ttf(font_file *ff, FILE *fp)
{					   
	int 				  i, offset;
	char				  dummy[4];
  	sfnt_DirectoryEntry   table;

	memset(dummy, 0, sizeof(dummy));

  	/* First off, read the initial directory header on the TTF.  We're only
	   interested in the "numOffsets" variable to tell us how many tables
	   are present in this file.  */
	fwrite((char *)&ff->offsetTable, 1, sizeof(ff->offsetTable) - sizeof(sfnt_DirectoryEntry), fp);
	offset = sizeof(ff->offsetTable) - sizeof(sfnt_DirectoryEntry) +
			 sizeof(sfnt_DirectoryEntry)*ff->count;
	for (i = 0; i < ff->count; i++) {
		offset = (offset+3) & 0xffffffc;
		fc_write32(&table.offset, offset);
 		fc_write32(&table.length, ff->tables[i].length);
		table.tag = ff->tables[i].tag;
		table.checkSum = ff->tables[i].checkSum;
		fwrite((char *)&table, 1, sizeof(table), fp);
		offset += ff->tables[i].length;
	}
	for (i = 0; i < ff->count; i++) {
		fwrite((char *)ff->tables[i].buf, 1, ff->tables[i].length, fp);
		offset = (4-ff->tables[i].length)&3;
		if (offset > 0)
			fwrite(dummy, 1, offset, fp);
	}
}

bool extract_cmap(char *cmap, fc_cmap_segment **list, int32 *seg_count, uint16 **glyphArray)
{
	typedef struct {
		uint16			format;
		uint16			length;
		uint16			version;
		uint16			segCountX2;
		uint16			searchRange;
		uint16			entrySelector;
		uint16			rangeShift;
	} sfnt_CmapSubTable4;

	int						i, j, count, loop_count, offset, size;
	uint16					*buf;
	uint16					*glyphs;
	fc_cmap_segment			*segs;
	sfnt_CmapSubTable4		*h;
	sfnt_CmapHeader			cHeader;
	sfnt_CmapList			cList;

	memcpy((char*)&cHeader, cmap, sizeof(cHeader));
	cmap += sizeof(cHeader);

	count = fc_read16(&cHeader.listCount);
	for (j=0; j<count; j++) {
		memcpy((char *)&cList, cmap, sizeof(cList));
		cmap += sizeof(cList);
		if ((fc_read16(&cList.platformID) == 3) && (fc_read16(&cList.encodingID) == 1)) {
			cmap += (fc_read32(&cList.offset) - (sizeof(cHeader) + (j+1)*sizeof(cList)));
			break;
		}
	}

	/* access and check the validity of the fixed part of the table header */
	h = (sfnt_CmapSubTable4*)cmap;
	cmap += sizeof(sfnt_CmapSubTable4);

	if (fc_read16(&h->format) != 4) {
		printf("ec 1\n");
		return false;
	}
	/* calculate the size of the dynamic array */
	count = fc_read16(&h->segCountX2)/2;
	if ((count >= 8192) || (count <= 0)) {
		printf("ec 2\n");
		return false;
	}

	*seg_count = count;
	/* allocate the segment array */
	segs = (fc_cmap_segment*)malloc(count*sizeof(fc_cmap_segment));
	if (segs == NULL) {
		printf("ec 3\n");
		return false;
	}

	*list = segs;
	glyphs = NULL;
	/* read and convert the dynamic tables in the segment array */
	offset = 0;
	while ((loop_count = (((count-offset)>256)?256:(count-offset))) != 0) {
		buf = (uint16*)cmap;
		cmap += loop_count*sizeof(uint16);
		for (i=0; i<loop_count; i++)
			segs[i+offset].endCount = fc_read16(buf+i);
		offset += loop_count;
	}
	cmap += sizeof(uint16);
	offset = 0;
	while ((loop_count = (((count-offset)>256)?256:(count-offset))) != 0) {
		buf = (uint16*)cmap;
		cmap += loop_count*sizeof(uint16);
		for (i=0; i<loop_count; i++)
			segs[i+offset].startCount = fc_read16(buf+i);
		offset += loop_count;
	}
	offset = 0;
	while ((loop_count = (((count-offset)>256)?256:(count-offset))) != 0) {
		buf = (uint16*)cmap;
		cmap += loop_count*sizeof(uint16);
		for (i=0; i<loop_count; i++)
			segs[i+offset].idDelta = fc_read16(buf+i);
		offset += loop_count;
	}
	offset = 0;
	while ((loop_count = (((count-offset)>256)?256:(count-offset))) != 0) {
		buf = (uint16*)cmap;
		cmap += loop_count*sizeof(uint16);
		for (i=0; i<loop_count; i++)
			segs[i+offset].idRangeOffset = fc_read16(buf+i);
		offset += loop_count;
	}
	/* calculate the size of the glyphArray */
	size = 0;
	for (i=0; i<count; i++)
		if (segs[i].idRangeOffset != 0) {
			size += (segs[i].endCount - segs[i].startCount + 1);
			/* update the idRangeOffset to be relative to the start of the glyphArray-1 */
			segs[i].idRangeOffset -= (count-i-1)*sizeof(uint16);
			segs[i].idRangeOffset /= sizeof(uint16);
		}
	for (i=0; i<count; i++)
		if (segs[i].idRangeOffset != 0)
			if ((segs[i].idRangeOffset > size) ||
				(segs[i].idRangeOffset+segs[i].endCount-segs[i].startCount > size)) {
				printf("ec 4\n");
				goto false_exit;
			}
	/* allocate the glyphArray buffer */
	if (size != 0) {
		glyphs = (uint16*)malloc((size+1)*sizeof(uint16));
		if (glyphs == NULL) {
			printf("ec 5\n");
			goto false_exit;
		}
		*glyphArray = glyphs;
		/* read and convert the glyphArray buffer. Read it offset by 1, so that the glyphs
		   buffer was properly aligned to be directly used with the idRangeOffset. */
		memcpy((char*)(glyphs+1), cmap, size*sizeof(uint16));
		cmap += size*sizeof(uint16);
		for (i=0; i<size; i++)
			glyphs[i] = fc_read16(glyphs+i);
	}
	else
		*glyphArray = NULL;
	return true;
		
false_exit:
	return false;
}

uint16 get_glyph_id(fc_cmap_segment *segs, int32 seg_count, uint16 *glyphArray, uint16 value) {
	uint16		offset;
	int		j = 0;

	for (j=0; j<seg_count; j++)
		if ((segs[j].startCount <= value) && (value <= segs[j].endCount)) {
			if (segs[j].idRangeOffset == 0) {
				return segs[j].idDelta + value;
			}
			else {
				offset = glyphArray[segs[j].idRangeOffset+(value-segs[j].startCount)];
				if (offset == 0) {
					printf("Unmapped glyph type 2\n");
					return 0x0000;
				}
				else
					return segs[j].idDelta + offset;
			}
		}
	printf("Unmapped glyph type 1\n");
	return 0x0000;
}

uint16 get_unicode(fc_cmap_segment *segs, int32 seg_count, uint16 *glyphArray, uint16 glyph_id) {
	int32		i, j;
	uint16		offset, value;

	for (j=0; j<seg_count; j++) {
		if (segs[j].idRangeOffset == 0) {
			value = glyph_id - segs[j].idDelta;
			if ((segs[j].startCount <= value) && (value <= segs[j].endCount))
				return value;
		}
		else for (i=0; i<=(segs[j].endCount-segs[j].startCount); i++) {
			offset = glyphArray[segs[j].idRangeOffset+i];
			if (offset != 0)
				if (glyph_id == (segs[j].idDelta + offset))
					return segs[j].startCount + i;
		}
	}
	printf("Not a valid glyph id !\n");
	return 0x0000;
}

font_table *get_table(font_file *ff, int32 tag) {
	int		i;
	
	for (i=0; i<ff->count; i++)
		if (fc_read32(&ff->tables[i].tag) == tag)
			return ff->tables+i;
	return NULL;
}

bool parse_glyf(uint8 *glyf_ptr, int32 size, uint8 **ptr_array, uint8 **end_array,
				fc_cmap_segment *to_segs, int32 to_seg_count, uint16 *to_glyphArray,
				fc_cmap_segment *from_segs, int32 from_seg_count, uint16 *from_glyphArray) {
	int32		contour_count, instruction_length;
	uint8		*inst;
	uint16		*scan;
	uint16		flags, index;

	enum {
		ARG_1_AND_2_ARE_WORDS	 = 1,
		WE_HAVE_A_SCALE			 = 8,
		WE_HAVE_AN_X_AND_Y_SCALE = 64 ,
		WE_HAVE_A_TWO_BY_TWO	 = 128,
		MORE_COMPONENTS			 = 32,
		WE_HAVE_INSTRUCTIONS	 = 256
	};

/* instruction program analysis */
	contour_count = fc_read16(glyf_ptr+0);
	/* single glyph description */
	if (contour_count != 0xffff)
		inst = (uint8*)(glyf_ptr+12+contour_count*2);
	/* composite glyph */
	else {
		scan = (uint16*)(glyf_ptr+10);
		do {
			flags = fc_read16(scan);
			scan += 1;
			/* convert the glyph_id between the 2 fonts */
			index = fc_read16(scan);
			index = get_unicode(from_segs, from_seg_count, from_glyphArray, index);
			if (index == 0)
				return true;
			index = get_glyph_id(to_segs, to_seg_count, to_glyphArray, index);
			if (index == 0)
				return true;
			fc_write16(scan, index);
			scan += 1;
			if (flags & ARG_1_AND_2_ARE_WORDS)
				scan += 2;
			else
				scan += 1;
			if (flags & WE_HAVE_A_SCALE)
				scan += 1;
			else if (flags & WE_HAVE_AN_X_AND_Y_SCALE)
				scan += 2;
			else if (flags & WE_HAVE_A_TWO_BY_TWO)
				scan += 4;
		} while (flags & MORE_COMPONENTS);
		if ((flags & WE_HAVE_INSTRUCTIONS) == 0) {
			/* set 0 describes the header before the program */
			ptr_array[0] = glyf_ptr;
			end_array[0] = (uint8*)scan;
			/* set 1 describes the original program */
			ptr_array[1] = end_array[0];
			end_array[1] = end_array[0];
			/* set 2 describes the translated program */
			ptr_array[2] = (uint8*)malloc(32);
			end_array[2] = ptr_array[2];
			/* set 3 describes the tail of the glyf */
			ptr_array[3] = end_array[1];
			end_array[3] = glyf_ptr+size;
			return false;
		}
		inst = (uint8*)(scan+1);
	}
/* set the arrays of pointers */
	/* set 0 describes the header before the program */
	ptr_array[0] = glyf_ptr;
	end_array[0] = inst;
	/* set 1 describes the original program */
	instruction_length = fc_read16(inst-2);
	ptr_array[1] = inst;
	end_array[1] = inst+instruction_length;
	/* set 2 describes the translated program */
	ptr_array[2] = (uint8*)malloc(instruction_length*2+1000);
	end_array[2] = ptr_array[2];
	/* set 3 describes the tail of the glyf */
	ptr_array[3] = end_array[1];
	end_array[3] = glyf_ptr+size;
	return false;
}

void repack_glyf(uint8 **ptr_array, uint8 **end_array, uint8 **output2) {
	int32		count;
	uint8		*output;
	
/* glyph copy */
	output = *output2;
	/* copy the start header, set 0 (unchanged) */
	count = end_array[0]-ptr_array[0];
	memcpy(output, ptr_array[0], count);
	fc_write16(output+count-2, end_array[2]-ptr_array[2]);
	output += count;
	/* insert the reset program */
//	memcpy(output, reset_program2, sizeof(reset_program2));
//	output += sizeof(reset_program2);
	/* copy the new program, set 2 */
	count = end_array[2]-ptr_array[2];
	if (count > 0) {
		memcpy(output, ptr_array[2], count);
		output += count;
	}
	/* copy the tail buffer, set 3 (unchanged) */
	count = end_array[3]-ptr_array[3];
	if (count > 0) {
		memcpy(output, ptr_array[3], count);
		output += count;
	}
	/* clean-up */
	free(ptr_array[2]);
	*output2 = output;
}

uint16 max16(uint16 a, uint16 b);

uint16 max16(uint16 a, uint16 b) {
	if (a > b)
		return a;
	return b;
}

void maximize_maxp(uint16 *to, uint16 *from) {
	/* maxPoints */
	fc_write16(to+3, max16(fc_read16(to+3), fc_read16(from+3)));
	/* maxContours */
	fc_write16(to+4, max16(fc_read16(to+4), fc_read16(from+4)));
	/* maxCompositePoints */
	fc_write16(to+5, max16(fc_read16(to+5), fc_read16(from+5)));
	/* maxCompositeContours */
	fc_write16(to+6, max16(fc_read16(to+6), fc_read16(from+6)));
	/* maxZones */
	fc_write16(to+7, max16(fc_read16(to+7), fc_read16(from+7)));
	/* maxTwilightPoints */
	fc_write16(to+8, max16(fc_read16(to+8), fc_read16(from+8)));
	/* maxStorage */
	fc_write16(to+9, max16(fc_read16(to+9), fc_read16(from+9)));
	/* maxFunctionDefs */
	fc_write16(to+10, fc_read16(to+10)+fc_read16(from+10));
	/* maxInstructionDefs */
	fc_write16(to+11, fc_read16(to+11)+fc_read16(from+11));
	/* maxStackElements */
	fc_write16(to+12, max16(fc_read16(to+12), fc_read16(from+12)));
	/* maxSizeofInstructions */
	fc_write16(to+13, max16(fc_read16(to+13), fc_read16(from+13)));
	/* maxComponentElements */
	fc_write16(to+14, max16(fc_read16(to+14), fc_read16(from+14)));
	/* maxComponentDepth */
	fc_write16(to+15, max16(fc_read16(to+15), fc_read16(from+15)));
}

