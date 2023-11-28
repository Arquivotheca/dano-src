#include "ttf.h"
#include "parser.h"

uint16 glyf_list[] = {
	0x0020,
	0x0021,
	0x0022,
	0x0023,
	0x0024,
	0x0025,
	0x0026,
	0x0027,
	0x0028,
	0x0029,
	0x002a,
	0x002b,
	0x002c,
	0x002d,
	0x002e,
	0x002f,	
	0x0030,
	0x0031,
	0x0032,
	0x0033,
	0x0034,
	0x0035,
	0x0036,
	0x0037,
	0x0038,
	0x0039,
	0x003a,
	0x003b,
	0x003c,
	0x003d,
	0x003e,
	0x003f,
	0x0040,
	0x0041,
	0x0042,
	0x0043,
	0x0044,
	0x0045,
	0x0046,
	0x0047,
	0x0048,
	0x0049,
	0x004a,
	0x004b,
	0x004c,
	0x004d,
	0x004e,
	0x004f,
	0x0050,
	0x0051,
	0x0052,
	0x0053,
	0x0054,
	0x0055,
	0x0056,
	0x0057,
	0x0058,
	0x0059,
	0x005a,
	0x005b,
	0x005d,
	0x005e,
	0x005f,
	0x0060,	
	0x0061,
	0x0062,
	0x0063,
	0x0064,
	0x0065,
	0x0066,
	0x0067,
	0x0068,
	0x0069,
	0x006a,
	0x006b,
	0x006c,
	0x006d,
	0x006e,
	0x006f,
	0x0070,
	0x0071,
	0x0072,
	0x0073,
	0x0074,
	0x0075,
	0x0076,
	0x0077,
	0x0078,
	0x0079,
	0x007a,
	0x007b,
	0x007c,
	0x007d,
	0x007e,
	0x00a0,
	0x00a1,
	0x00a2,
	0x00a3,
	0x00a4,
	0x00a5,
	0x00a6,
	0x00a7,
	0x00a8,
	0x00a9,
	0x00aa,
	0x00ab,
	0x00ac,
	0x00ad,
	0x00ae,
	0x00af,
	0x00b0,
	0x00b1,
	0x00b2,
	0x00b3,
	0x00b4,
	0x00b5,
	0x00b6,
	0x00b7,
	0x00b8,
	0x00b9,
	0x00ba,
	0x00bb,
	0x00bc,
	0x00bd,
	0x00be,
	0x00bf,
	0x00c0,
	0x00c1,
	0x00c2,
	0x00c3,
	0x00c4,
	0x00c5,
	0x00c6,
	0x00c7,
	0x00c8,
	0x00c9,
	0x00ca,
	0x00cb,
	0x00cc,
	0x00cd,
	0x00ce,
	0x00cf,
	0x00d0,
	0x00d1,
	0x00d2,
	0x00d3,
	0x00d4,
	0x00d5,
	0x00d6,
	0x00d7,
	0x00d8,
	0x00d9,
	0x00da,
	0x00db,
	0x00dc,
	0x00dd,
	0x00de,
	0x00df,
	0x00e0,
	0x00e1,
	0x00e2,
	0x00e3,
	0x00e4,
	0x00e5,
	0x00e6,
	0x00e7,
	0x00e8,
	0x00e9,
	0x00ea,
	0x00eb,
	0x00ec,
	0x00ed,
	0x00ee,
	0x00ef,
	0x00f0,
	0x00f1,
	0x00f2,
	0x00f3,
	0x00f4,
	0x00f5,
	0x00f6,
	0x00f7,
	0x00f8,
	0x00f9,
	0x00fa,
	0x00fb,
	0x00fc,
	0x00fd,
	0x00fe,
	0x00ff,
	0x0106,
	0x0107,
	0x010c,
	0x010d,
	0x0111,
	0x011e,
	0x011f,
	0x0130,
	0x0131,
	0x0141,
	0x0142,
	0x0152,
	0x0153,
	0x015e,
	0x015f,
	0x0160,
	0x0161,
	0x0178,
	0x017d,
	0x017e,
	0x0192,
	0x02c6,
	0x02c7,
	0x02d8,
	0x02d9,
	0x02da,
	0x02db,
	0x02dc,
	0x02dd,
	0x2013,
	0x2014,
	0x201a,
	0x201e,
	0x2020,
	0x2021,
	0x2022,
	0x2026,
	0x2030,
	0x2039,
	0x203a,
	0x20a3,
	0x2122,
	0x2202,
	0x2206,
	0x220f,
	0x2211,
	0x2212,
	0x2215,
	0x2219,
	0x221a,
	0x221e,
	0x2248,
	0x2264,
	0x2265,
	0x25ca,		
	0x0000
};

int main(int argc, char	*argv[]) {
	bool			err, res;
	FILE			*fp;
	func			*func_table;
	int32			i, j, k, count, func_offset, cvt_offset, copy_count;
	int32			cur_instr, cur_ass, cur_cst, cur_level;	
	int32			cur_label, cur_offset, offset, new_offset;
	int32			to_seg_count, from_seg_count, size;
	uint8			*cst_status, *buffer_ptr, *test_buffer;
	uint8			*output, *glyf_ptr;
	uint8			*instr_ptr, *instr_end;
	uint8			*ptr_array[8], *end_array[8];
	int32			stats[3][8];
	uint16			*to_glyphArray, *from_glyphArray;
	uint32			*cst;
	uint16			tmp;
	uint16			*unicodes;
	uint16			*to_glyph_ids;
	uint16			*from_glyph_ids;
	uint32			*to_offset;
	uint16			*from_offset;
	ass_item		*ass_list;
	font_file		bastard, roman;
	font_table		*cmap, *cvt, *fpgm, *glyf, *loca, *prep, *maxp;
	font_table		*cmap2, *cvt2, *fpgm2, *glyf2, *loca2, *prep2, *maxp2;
	font_table		new_cvt, new_fpgm, new_glyf, new_prep;
	fc_cmap_segment	*to_list, *from_list;

	fprintf(stderr, "\n*************************************************\n");
	fprintf(stderr, "** FontFusion tool, (C) 1998 Be, Incorporated\n");
	fprintf(stderr, "**   by Pierre Raynaud-Richard\n");
	fprintf(stderr, "*************************************************\n\n");
/* load both fonts */
	fprintf(stderr, "Loading Bastard.ttf.............................. ");
	fp = fopen("Bastard.ttf", "rb");
	res = read_ttf(&bastard, fp);
	fclose(fp);
	if (!res) {
		fprintf(stderr, "Fatal Error !\n");
		return 1;
	}
	fprintf(stderr, "Done\n");

	fprintf(stderr, "Loading Roman.ttf................................ ");
	fp = fopen("Roman.ttf", "rb");
	res = read_ttf(&roman, fp);
	fclose(fp);
	if (!res) {
		fprintf(stderr, "Fatal Error !\n");
		return 1;
	}
	fprintf(stderr, "Done\n");

/* get the descriptor of all useful tables */
	fprintf(stderr, "Extracting tables from Bastard.ttf............... ");
	cmap = get_table(&bastard, tag_cmap);
	cvt  = get_table(&bastard, tag_cvt);
	fpgm = get_table(&bastard, tag_fpgm);
	glyf = get_table(&bastard, tag_glyf);
	loca = get_table(&bastard, tag_loca);
	maxp = get_table(&bastard, tag_maxp);
	prep = get_table(&bastard, tag_prep);
	if ((cmap == NULL) || (cvt == NULL) || (fpgm == NULL) || (maxp == NULL) ||
		(glyf == NULL) || (loca == NULL) || (prep == NULL)) {
		fprintf(stderr, "Fatal Error !\n");
		return 1;
	}
	fprintf(stderr, "Done\n");

	fprintf(stderr, "Extracting tables from Roman.ttf................. ");
	cmap2 = get_table(&roman, tag_cmap);
	cvt2  = get_table(&roman, tag_cvt);
	fpgm2 = get_table(&roman, tag_fpgm);
	glyf2 = get_table(&roman, tag_glyf);
	loca2 = get_table(&roman, tag_loca);
	maxp2 = get_table(&bastard, tag_maxp);
	prep2 = get_table(&roman, tag_prep);
	if ((cmap2 == NULL) || (cvt2 == NULL) || (fpgm2 == NULL) || (maxp2 == NULL) ||
		(glyf2 == NULL) || (loca2 == NULL) || (prep2 == NULL)) {
		fprintf(stderr, "Fatal Error !\n");
		return 1;
	}
	fprintf(stderr, "Done\n");

/* extract the cmap information */
	fprintf(stderr, "Extracting cmap from Bastard.ttf................. ");
	if (!extract_cmap((char*)cmap->buf, &to_list, &to_seg_count, &to_glyphArray)) {
		fprintf(stderr, "Fatal Error !\n");
		return 1;
	}
	fprintf(stderr, "Done\n");
	fprintf(stderr, "Extracting cmap from Roman.ttf................... ");
	if (!extract_cmap((char*)cmap2->buf, &from_list, &from_seg_count, &from_glyphArray)) {
		fprintf(stderr, "Fatal Error !\n");
		return 1;
	}
	fprintf(stderr, "Done\n");
	stats[0][4] = loca->length/4-1;
	stats[1][4] = loca2->length/4-1;
	stats[2][4] = loca->length/4-1;
	
/* initialise the merging mechanism */
	fprintf(stderr, "Initialising parsing buffers..................... ");
	cst_status = (uint8*)malloc(8000*sizeof(uint8));
	cst = (uint32*)malloc(8000*sizeof(uint32));
	ass_list = (ass_item*)malloc(20000*sizeof(ass_item));
	func_table = (func*)malloc(100*sizeof(func));
	if ((cst_status == NULL) || (ass_list == NULL) ||
		(ass_list == NULL) || (func_table == NULL)) {
		fprintf(stderr, "Fatal Error !\n");
		return 1;
	}
	fprintf(stderr, "Done\n");
	
/* merge the cvt tables */
	fprintf(stderr, "Merging the cvt tables........................... ");
	cvt_offset = cvt->length/2;
	new_cvt.length = cvt->length+cvt2->length;
	new_cvt.buf = (void*)malloc(new_cvt.length);
	if (new_cvt.buf == NULL) {
		fprintf(stderr, "Fatal Error !\n");
		return 1;
	}
	memcpy(new_cvt.buf, cvt->buf, cvt->length);
	memcpy(((char*)new_cvt.buf)+cvt->length, cvt2->buf, cvt2->length);
	fprintf(stderr, "Done\n");
	stats[0][0] = cvt->length/2;
	stats[1][0] = cvt2->length/2;
	stats[2][0] = new_cvt.length/2;
	
/* merge the fpgm tables */
	/* check the function count in the destination */
	fprintf(stderr, "Merging the fpgm tables.......................... ");
	for (i=0; i<100; i++)
		func_table[i].first = NULL;
	cur_level = 1;
	cur_label = 1;
	cur_ass = 0;
	cur_cst = 0;
	cur_offset = 0;
	dump_code(	(uint8*)fpgm->buf,
				(uint8*)fpgm->buf+fpgm->length,
				ass_list, cst, cst_status, func_table,
				&cur_ass, &cur_cst, &cur_label, &cur_level, &cur_offset, 0);
	for (i=0; i<100; i++)
		if (func_table[i].first != NULL)
			func_offset = i+1;
	/* allocate the new table and copy the from fpgm */
	new_fpgm.length = fpgm->length+fpgm2->length+1000;
	new_fpgm.buf = (void*)malloc(new_fpgm.length);
	memcpy(new_fpgm.buf, fpgm->buf, fpgm->length);
	/* parse the to fpgm */
	for (i=0; i<50; i++)
		func_table[i].first = NULL;
	cur_level = 1;
	cur_label = 1;
	cur_ass = 0;
	cur_cst = 0;
	cur_offset = 0;
	dump_code(	(uint8*)fpgm2->buf,
				(uint8*)fpgm2->buf+fpgm2->length,
				ass_list, cst, cst_status, func_table,
				&cur_ass, &cur_cst, &cur_label, &cur_level, &cur_offset, 0);
	/* reindex the constant table */
	offset_cst(cst, cst_status, cur_cst, cvt_offset, func_offset); 
	/* assemble the resulting fpgm */
	output = ((uint8*)new_fpgm.buf)+fpgm->length;
	assemble_code(ass_list, ass_list+cur_ass, cst, &output);
	new_fpgm.length = output-(uint8*)new_fpgm.buf;
	fprintf(stderr, "Done\n");
	stats[0][1] = func_offset;
	for (i=0; i<100; i++)
		if (func_table[i].first != NULL)
			stats[1][1] = i+1;

/* merge the prep tables */
	/* allocate the new table and copy the from prep */
	fprintf(stderr, "Merging the prep tables.......................... ");
	new_prep.length = prep->length+prep2->length+5000;
	new_prep.buf = (void*)malloc(new_prep.length);
	memcpy(new_prep.buf, prep->buf, prep->length);
	output = ((uint8*)new_prep.buf)+prep->length;
	/* insert the default reset program */
	memcpy(output, default_program, sizeof(default_program));
	output += sizeof(default_program);
	/* parse the to fpgm */
	cur_level = 1;
	cur_label = 1;
	cur_ass = 0;
	cur_cst = 0;
	cur_offset = 0;
	err = dump_code((uint8*)prep2->buf,
					(uint8*)prep2->buf+prep2->length,
					ass_list, cst, cst_status, func_table,
					&cur_ass, &cur_cst, &cur_label, &cur_level, &cur_offset, 0);
	if (err) {
		fprintf(stderr, "Fatal Error !\n");
		fp = fopen("PrepError", "wb");
		print_code(fp, ass_list, cur_ass, cst, cst_status);
		fclose(fp);
		return 1;
	}
	/* reindex the constant table */
	offset_cst(cst, cst_status, cur_cst, cvt_offset, func_offset); 
	/* assemble the resulting fpgm */
	assemble_code(ass_list, ass_list+cur_ass, cst, &output);
	new_prep.length = output-(uint8*)new_prep.buf;
	fprintf(stderr, "Done\n");
	stats[0][2] = prep->length;
	stats[1][2] = prep2->length;
	stats[2][2] = new_prep.length;
	
/* merge the glyf tables */
	/* allocate glyph index processing buffers */
	fprintf(stderr, "Merging the glyf tables.......................... ");
	copy_count = 0;
	while (glyf_list[copy_count] != 0x0000)
		copy_count++;
	copy_count++;
	to_glyph_ids = (uint16*)malloc(copy_count*sizeof(uint16));
	from_glyph_ids = (uint16*)malloc(copy_count*sizeof(uint16));
	unicodes = (uint16*)malloc(copy_count*sizeof(uint16));
	/* allocate new glyph buffer */
	new_glyf.length = glyf->length+glyf2->length+copy_count*300+2000;
	new_glyf.buf = (void*)malloc(new_glyf.length);
	/* process the glyph_ids tables */
	copy_count = 0;
	while (glyf_list[copy_count] != 0x0000) {
		to_glyph_ids[copy_count] =
			get_glyph_id(to_list, to_seg_count, to_glyphArray, glyf_list[copy_count]);
		from_glyph_ids[copy_count] =
			get_glyph_id(from_list, from_seg_count, from_glyphArray, glyf_list[copy_count]);
		unicodes[copy_count] = glyf_list[copy_count];
		copy_count++;	
	}
	/* sortint by acsending order on to_glyph_ids */
	for (i=0; i<copy_count-2; i++)
		for (j=0; j<copy_count-1-i; j++)
			if (to_glyph_ids[j] > to_glyph_ids[j+1]) {
				tmp = to_glyph_ids[j];
				to_glyph_ids[j] = to_glyph_ids[j+1];
				to_glyph_ids[j+1] = tmp;
				tmp = from_glyph_ids[j];
				from_glyph_ids[j] = from_glyph_ids[j+1];
				from_glyph_ids[j+1] = tmp;
				tmp = unicodes[j];
				unicodes[j] = unicodes[j+1];
				unicodes[j+1] = tmp;
			}
	to_glyph_ids[copy_count] = 0x0000;
	/* merge glyphs of both tables */
	count = loca->length/4;
	output = (uint8*)new_glyf.buf;
	to_offset = (uint32*)loca->buf;
	from_offset = (uint16*)loca2->buf;
	j = 0;
	for (i=0; i<count-1; i++) {
		new_offset = output-(uint8*)new_glyf.buf;
		/* just copy the old glyph */
		if (i != to_glyph_ids[j]) {
			size = fc_read32(&to_offset[i+1])-fc_read32(&to_offset[i]);
			glyf_ptr = ((uint8*)glyf->buf)+fc_read32(&to_offset[i]);
			memcpy(output, glyf_ptr, size);
			output += size;
		}
		/* transcode the new glyph */
		else {
			k = from_glyph_ids[j];
 			size = (fc_read16(&from_offset[k+1])-fc_read16(&from_offset[k]))*2;
			if (size != 0) {
				/* extract the original glyph program */
				glyf_ptr = ((uint8*)glyf2->buf)+((uint32)fc_read16(&from_offset[k]))*2;
				err = parse_glyf(	glyf_ptr, size, ptr_array, end_array,
									to_list, to_seg_count, to_glyphArray,
									from_list, from_seg_count, from_glyphArray);
				if (err) {
					fprintf(stderr, "Fatal Error (Composite Glyph) !\n");
					return 1;
				}
				/* There is a program : parse the original glyf program */
				cur_level = 1;
				cur_label = 1;
				cur_ass = 0;
				cur_cst = 0;
				cur_offset = 0;
				err = dump_code(ptr_array[1], end_array[1],
								ass_list, cst, cst_status, func_table,
								&cur_ass, &cur_cst, &cur_label, &cur_level,
								&cur_offset, 0);
				if (err) {
					fprintf(stderr, "Fatal Error [%d/%d:%d]!\n",
							i, count-1, j);
					fp = fopen("GlyfError", "wb");
					print_code(fp, ass_list, cur_ass, cst, cst_status);
					fclose(fp);
					return 1;
				}
				/* reindex the constant table */
				offset_cst(cst, cst_status, cur_cst, cvt_offset, func_offset); 
				/* assemble the resulting glyf program */
				assemble_code(ass_list, ass_list+cur_ass, cst, &end_array[2]);
				/* rewrap the glyf structure */
				repack_glyf(ptr_array, end_array, &output);
			}
			j++;
		}
		fc_write32(&to_offset[i], new_offset);
	}
	fc_write32(&to_offset[count-1], output-(uint8*)new_glyf.buf);
	/* adjust real length */
	new_glyf.length = output-(uint8*)new_glyf.buf;
	/* free temporary buffers */
	free(unicodes);
	free(to_glyph_ids);
	free(from_glyph_ids);
	fprintf(stderr, "Done\n");
	stats[0][3] = glyf->length;
	stats[1][3] = glyf2->length;
	stats[2][3] = new_glyf.length;

/* merging the maxp tables */
	fprintf(stderr, "Merging the maxp table........................... ");
	maximize_maxp((uint16*)maxp->buf, (uint16*)maxp2->buf);
	fprintf(stderr, "Done\n");	

/* check the resulting merged font */
	/* check the fpgm */
	fprintf(stderr, "Checking the resulting fpgm table................ ");
	for (i=0; i<100; i++)
		func_table[i].first = NULL;
	cur_level = 1;
	cur_label = 1;
	cur_ass = 0;
	cur_cst = 0;
	cur_offset = 0;
	dump_code(	(uint8*)new_fpgm.buf,
				(uint8*)new_fpgm.buf+new_fpgm.length,
				ass_list, cst, cst_status, func_table,
				&cur_ass, &cur_cst, &cur_label, &cur_level, &cur_offset, 0);
	fprintf(stderr, "Done\n");
	for (i=0; i<100; i++)
		if (func_table[i].first != NULL)
			stats[2][1] = i+1;
	/* check the prep */
	fprintf(stderr, "Checking the resulting prep table................ ");
	cur_level = 1;
	cur_label = 1;
	cur_ass = 0;
	cur_cst = 0;
	cur_offset = 0;
	err = dump_code((uint8*)new_prep.buf,
					(uint8*)new_prep.buf+new_prep.length,
					ass_list, cst, cst_status, func_table,
					&cur_ass, &cur_cst, &cur_label, &cur_level, &cur_offset, 0);
	if (err) {
		fprintf(stderr, "Fatal Error !\n");
		fp = fopen("PrepError", "wb");
		print_code(fp, ass_list, cur_ass, cst, cst_status);
		fclose(fp);
		return 1;
	}
	fprintf(stderr, "Done\n");
	/* check the glyf programs */
	fprintf(stderr, "Checking the resulting glyf table................ ");
	count = loca->length/4;
	to_offset = (uint32*)loca->buf;
	for (i=0; i<count-1; i++) {
		size = fc_read32(&to_offset[i+1])-fc_read32(&to_offset[i]);
		glyf_ptr = ((uint8*)new_glyf.buf)+fc_read32(&to_offset[i]);
		err = parse_glyf(	glyf_ptr, size, ptr_array, end_array,
							to_list, to_seg_count, to_glyphArray,
							to_list, to_seg_count, to_glyphArray);
		if (err) {
			fprintf(stderr, "Fatal Error (Composite Glyph) !\n");
			return 1;
		}
		/* There is a program : parse the original glyf program */
		cur_level = 1;
		cur_label = 1;
		cur_ass = 0;
		cur_cst = 0;
		cur_offset = 0;
		err = dump_code(ptr_array[1], end_array[1],
						ass_list, cst, cst_status, func_table,
						&cur_ass, &cur_cst, &cur_label, &cur_level,
						&cur_offset, 0);
		if (err) {
			fprintf(stderr, "Fatal Error [%d/%d]!\n",
					i, count-1);
			fp = fopen("GlyfError", "wb");
			print_code(fp, ass_list, cur_ass, cst, cst_status);
			fclose(fp);
			return 1;
		}
		free(ptr_array[2]);
	}
	fprintf(stderr, "Done\n");

/* set the new tables */
	fprintf(stderr, "Repacking the resulting font..................... ");
	free(cvt->buf);
	free(fpgm->buf);
	free(glyf->buf);
	free(prep->buf);
	cvt->buf = new_cvt.buf;
	cvt->length = new_cvt.length;
	fpgm->buf = new_fpgm.buf;
	fpgm->length = new_fpgm.length;
	glyf->buf = new_glyf.buf;
	glyf->length = new_glyf.length;
	prep->buf = new_prep.buf;
	prep->length = new_prep.length;
	fprintf(stderr, "Done\n");
	
/* free the merging mechanism */
	fprintf(stderr, "Cleaning the mess................................ ");
	free(cst_status);
	free(cst);
	free(ass_list);	
	fprintf(stderr, "Done\n");
	
/* write the resulting font */
	fprintf(stderr, "Writing Result.ttf............................... ");
	fp = fopen("Result.ttf", "wb");
	write_ttf(&bastard, fp);
	fclose(fp);
	fprintf(stderr, "Done\n\n");
	fprintf(stderr, "Statistics      Bastard.ttf    Roman.ttf     Result.ttf\n");
	fprintf(stderr, "cvt count   :      %4d           %4d          %4d\n",
			stats[0][0], stats[1][0], stats[2][0]);
	fprintf(stderr, "fct count   :      %4d           %4d          %4d\n",
			stats[0][1], stats[1][1], stats[2][1]);
	fprintf(stderr, "prep length :      %4d           %4d          %4d\n",
			stats[0][2], stats[1][2], stats[2][2]);
	fprintf(stderr, "glyf length :   %7d        %7d       %7d\n",
			stats[0][3], stats[1][3], stats[2][3]);
	fprintf(stderr, "glyph count :     %5d          %5d         %5d\n",
			stats[0][4], stats[1][4], stats[2][4]);
	fprintf(stderr, "\n");
	return 0;
}
