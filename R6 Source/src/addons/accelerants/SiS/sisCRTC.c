#include "sisCRTC.h"
#include "accelerant.h"
#include "sis630defs.h"

static data_ioctl_sis_CRT_settings sis_CRT_settings ;


///////////////////////////
// Computes CRT settings //
///////////////////////////

struct data_ioctl_sis_CRT_settings *prepare_CRT(display_mode *dm, uint32 screen_start_address, uint32 pixels_per_row) {

	uchar DoubleScan,t;
	ulong original_dot_clock;

	ulong h_front_p, h_sync, h_back_p;
	ulong h_vis, v_vis, v_front_p, v_sync, v_back_p;
	ulong v_blank;
	uchar v_blank_e, hvidmid;
	ulong hTotal, hDispEnableEnd, hBlankStart, calcHorizBlankEnd, hSyncStart, hRetraceEnd;
	ulong vTotal, vSyncStart, vDisplayEnable, vRetraceEnd, offset, vBlankStart, linecompare;
	int is_interlaced;
	int vclkhtotal, vclkhvis, vclkhsync, vclkhbackp;
	int vclkhblank, vclkhfrontp;
	int temp;

	// ------------------------

	h_front_p	= (ulong)dm->timing.h_sync_start - dm->timing.h_display;
	h_sync		= (ulong)dm->timing.h_sync_end - dm->timing.h_sync_start;
	h_back_p	= (ulong)dm->timing.h_total - dm->timing.h_sync_end;
	h_vis		= (ulong)dm->timing.h_display;

	v_front_p	= (ulong)dm->timing.v_sync_start - dm->timing.v_display;
	v_sync		= (ulong)dm->timing.v_sync_end - dm->timing.v_sync_start;
	v_back_p	= (ulong)dm->timing.v_total - dm->timing.v_sync_end;
	v_vis		= (ulong)dm->timing.v_display;

	is_interlaced	= (ulong)dm->timing.flags & B_TIMING_INTERLACED;
	vddprintf(("Interlaced?:  %s\n", (is_interlaced ? "YES" : "NO")));

	/*---------------- calculation of reg htotal --------------------*/
	temp = h_vis + h_front_p + h_sync + h_back_p;
	vclkhtotal = temp >> 3;
	hTotal = vclkhtotal - 5;
	vddprintf(("hTotal: %d 0x%02x\n", hTotal, hTotal));

	/*---- calculation of the horizontal display enable end reg ----*/
	vclkhvis = h_vis >> 3;
	hDispEnableEnd = (uchar)vclkhvis-1;

	/*----- calculation of the horizontal blanking start register ----*/
	hBlankStart = vclkhvis - 1;
	vddprintf(("hBlankStart: %d 0x%02x\n", hBlankStart, hBlankStart));

	/*----- calculation of horizontal blanking end register -------*/
	vclkhfrontp	= h_front_p >> 3;
	vclkhsync	= h_sync >> 3;
	vclkhbackp	= h_back_p >> 3;
	vclkhblank	= vclkhfrontp + vclkhbackp + vclkhsync;
	calcHorizBlankEnd = (vclkhvis + vclkhblank) - 1;
	vddprintf(("calcHorizBlankEnd: %d 0x%02x\n", calcHorizBlankEnd, calcHorizBlankEnd));

	/*----- calculation of the horizontal retrace start register -----*/
	hSyncStart = (vclkhvis + vclkhfrontp) - 1;
	vddprintf(("hSyncStart: %d 0x%02x\n", hSyncStart, hSyncStart));

	/*----- calculation of the horizontal retrace end register -------*/
	hRetraceEnd =  ((vclkhvis + vclkhfrontp + vclkhsync) - 1);
	vddprintf(("retrace end: %d 0x%02x\n", hRetraceEnd, hRetraceEnd));

	/*----- calculation of the vertical total register -------------*/
	vTotal = (v_vis + v_front_p + v_sync + v_back_p) - 2;

	/*------ calculation of the vertical retrace start register -----*/
	vSyncStart = (v_vis + v_front_p) - 1;

	/*------ calculation of the vertical retrace end register ------*/
	vRetraceEnd = (v_vis + v_front_p + v_sync - 1) & 0x0f;

	/*----- calculation of the vertical display enable register ---*/
	vDisplayEnable = v_vis - 1;

	/*----- calculation of the offset register ------------------*/
	vddprintf(("dm->virtual_width: %d\n", dm->virtual_width));
	vddprintf(("pixels_per_row: %d\n", pixels_per_row));
	/* offset = (uint32)dm->virtual_width / (64 / bits_per_pixel); */
	if (dm->space==B_RGB32_LITTLE)		offset = pixels_per_row / 2 ;
	else if (dm->space==B_RGB16_LITTLE)	offset = pixels_per_row / 4 ;
	else if (dm->space==B_CMAP8)		offset = pixels_per_row / 8 ;
	vddprintf(("sis: CRT offset = %d\n", offset));

	/*-----------calculation of the vertical blanking start register ----------*/
	/*FFB OK */
	vBlankStart = v_vis - 1;

	/*-----------calculation of the vertical blanking end register ------------*/
	/*FFB OK */
	v_blank = v_back_p + v_sync + v_front_p;
	v_blank_e= (uchar)(v_vis + v_blank) - 1;

	/*---- line compare register ---------*/

	linecompare = 0x3ff; // 0x0ff to 0x3ff
	
	// = 0xff; /* linecomp (the 0x0ff from 0x3ff) */
	//linecompare = (uchar)(vBlankStart & 0x3ff);
	
	if (vTotal < 300) {
		DoubleScan=0x80;
		vTotal *= 2;
		}
	else DoubleScan=0x00;

	vddprintf(("sis: Calculated refresh rate %d\n",(int) (1000*dm->timing.pixel_clock)/(hTotal*vTotal*8) )); 
	
	/* Encode CRT parameters */
	
	vddprintf(("sis:Htotal: %d, Hdisp: %d\n",(int)hTotal,(int)hDispEnableEnd));
	vddprintf(("sis:Hstart-blank: %d, Hend-blank %d\n",(int)hBlankStart,(int)calcHorizBlankEnd));
	vddprintf(("sis:Hstart-sync: %d, Hend-sync %d\n",(int)hSyncStart,(int)hRetraceEnd));
	
	if (ci->ci_DeviceId == SIS630_DEVICEID) {
		vddprintf(("sis630: - warning - hSyncStart += 5\n"));
		hSyncStart += 5; // experimental result...
		}

	vddprintf(("sis: vTotal: %d, vSyncStart: %d, vDisplayEnable: %d\n",(int)vTotal, (int)vSyncStart, (int)vDisplayEnable));
	vddprintf(("sis: vRetraceEnd: %d, offset: %d, vBlankStart: %d\n",(int)vRetraceEnd, (int)offset, (int)vBlankStart));
	vddprintf(("sis: linecompare: %d\n",linecompare));
	
	calcHorizBlankEnd=(calcHorizBlankEnd & 0x00ff);
	hRetraceEnd=(hRetraceEnd & 0x003f);
	
	vRetraceEnd=(vRetraceEnd&15);
	v_blank_e=(v_blank_e&255);
	
	///////////////////////////
	// Assign CRT parameters //
	///////////////////////////
	
	sis_CRT_settings.CRT_data[0x00]=hTotal&255;
	sis_CRT_settings.CRT_data[0x01]=hDispEnableEnd&255;
	sis_CRT_settings.CRT_data[0x02]=hBlankStart&255;
	sis_CRT_settings.CRT_data[0x03]=(calcHorizBlankEnd&31)|128;  /* 128 : ? reserved */
	sis_CRT_settings.CRT_data[0x04]=hSyncStart&255;
	sis_CRT_settings.CRT_data[0x05]=(hRetraceEnd&31)|((calcHorizBlankEnd&32)<<2);
	sis_CRT_settings.CRT_data[0x06]=vTotal&255;
	sis_CRT_settings.CRT_data[0x07]=((vTotal&256)>>8) |
									((vDisplayEnable&256)>>7) |
									((vSyncStart&256)>>6) |
									((vBlankStart&256)>>5) |
									((linecompare&256)>>4) |
									((vTotal&512)>>4) |
									((vDisplayEnable&512)>>3) |
									((vSyncStart&512)>>2);
	sis_CRT_settings.CRT_data[0x08]=0x00; 
	sis_CRT_settings.CRT_data[0x09]=DoubleScan | ((linecompare&512)>>3) | ((vBlankStart&512)>>4);
	sis_CRT_settings.CRT_data[0x0A]=0x20; // text-cursor off
	sis_CRT_settings.CRT_data[0x0B]=0x00;
	sis_CRT_settings.CRT_data[0x0C]=(screen_start_address & 0xff00) >> 8; /* Screen start address bit */
	sis_CRT_settings.CRT_data[0x0D]=(screen_start_address & 0x00ff);
	//sis_CRT_settings.CRT_data[0x0E]=; 	//cursor position
	//sis_CRT_settings.CRT_data[0x0F]=;
	sis_CRT_settings.CRT_data[0x10]=vSyncStart&255;
	sis_CRT_settings.CRT_data[0x11]=0x80|(vRetraceEnd&15); /* Lock CRT0-7,enable and clear vertical interrupt */
	sis_CRT_settings.CRT_data[0x12]=vDisplayEnable&255;
	sis_CRT_settings.CRT_data[0x13]=(offset & 0x00ff);
	sis_CRT_settings.CRT_data[0x14]=0x40; // DoubleWord mode enable (0x40) and count-by-4 (0x20)
	sis_CRT_settings.CRT_data[0x15]=vBlankStart&255;
	sis_CRT_settings.CRT_data[0x16]=v_blank_e&255;
	sis_CRT_settings.CRT_data[0x17]=0xe3; /* 0xe3 : byte refresh // a3 ?*/
	sis_CRT_settings.CRT_data[0x18]=linecompare&255;

//	if (ci->ci_DeviceId != SIS630_DEVICEID) {
		sis_CRT_settings.extended_CRT_overflow=
							 ((offset>>4)&0xf0)
							|((vSyncStart&1024)>>7)
							|((vBlankStart&1024)>>8)
							|((vDisplayEnable&1024)>>9)
							|((vTotal&1024)>>10);
		vddprintf(("sis: sisCRT_overflow 0x%02x\n",sis_CRT_settings.extended_CRT_overflow));
						
		sis_CRT_settings.extended_horizontal_overflow=
							 ((calcHorizBlankEnd&64)>>2)
							|((hSyncStart&256)>>5)
							|((hBlankStart&256)>>6)
							|((hDispEnableEnd&256)>>7)
							|((hTotal&256)>>8);
		vddprintf(("sis: sisCRT_hor_overflow 0x%02x\n",sis_CRT_settings.extended_horizontal_overflow));
//		}
//	else {
		sis_CRT_settings.sis630_ext_vertical_overflow=
							 ((vRetraceEnd		& (1<<4 )) << 1)
							|((v_blank_e		& (1<<8 )) >> 4)
							|((vSyncStart		& (1<<10)) >> 7)
							|((vBlankStart		& (1<<10)) >> 8)
							|((vDisplayEnable	& (1<<10)) >> 9)
							|((vTotal			& (1<<10)) >>10) ;

		sis_CRT_settings.sis630_ext_horizontal_overflow1=
							 ((hSyncStart		& (3<<8 )) >> 2)
							|((hBlankStart		& (3<<8 )) >> 4)
							|((hDispEnableEnd	& (3<<8 )) >> 6)
							|((hTotal			& (3<<8 )) >> 8) ;

		sis_CRT_settings.sis630_ext_horizontal_overflow2=
							 ((hRetraceEnd		& (1<<5 )) >> 3)
							|((calcHorizBlankEnd& (3<<6 )) >> 6) ;
							
		sis_CRT_settings.sis630_ext_pitch =
							 ((offset			& 0x0f00 ) >> 8) ;
		sis_CRT_settings.sis630_ext_starting_address =
							 ((screen_start_address >> 16) & 0xff);
		sis_CRT_settings.sis630_display_line_width = 
							640/4 ;							
//		}

	vddprintf(("sis:prepare CRT finished\n"));
	return(&sis_CRT_settings);
	}

