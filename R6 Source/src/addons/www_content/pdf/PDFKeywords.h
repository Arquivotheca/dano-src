/* PDFKeywords.h */

#ifndef _PDF_KEYWORDS_H_
#define _PDF_KEYWORDS_H_

enum  pdf_keys {
	PDF_UNKNOWN_KEY	=	-1,
	PDF_b,					// b:	closepath, fill, and strokepath
	PDF_bstar,				// b*:	closepath, eofill and strokepath
	PDF_B,					// B:	fill and stroke path
	PDF_Bstar,				// B*:	eofill and strokepath
	PDF_BDC,				// BDC:	begin marked content, with a dictionary
	PDF_BI,					// BI:	begin image
	PDF_BMC,				// BMC:	begin marked content
	PDF_BT,					// BT:	begin text object
	PDF_BX,					// BX:	begin section allowing undefined operators
	PDF_c,					// c:	curveto
	PDF_cm,					// cm:	concactenates matrix to current ctm
	PDF_cs,					// cs:	setcolorspace for fill
	PDF_CS,					// CS:	setcolorspace for stroke
	PDF_d,					// d:	setdash
	PDF_d0,					// d0:	setcharwidth for Type 3 font
	PDF_d1,					// d1:	setcachedevice for Type 3 font
	PDF_Do,					// Do:	execute named XObject
	PDF_DP,					// DP:	mark a place in content stream with a dictionary
	PDF_EI,					// EI:	end image
	PDF_EMC,				// EMC:	end marked content
	PDF_ET,					// ET:	end text object
	PDF_EX,					// EX:	end section that allows undefined operators
	PDF_f,					// f:	fillpath
	PDF_fstar,				// f*:	eofill path
	PDF_F,					// F:	fillpath
	PDF_g,					// g:	setgrey for fill
	PDF_gs,					// gs:	set extended graphics state parameters
	PDF_G,					// G:	setgrey for stroke
	PDF_h,					// h:	closepath
	PDF_i,					// i:	setflat
	PDF_ID,					// ID:	begin image data
	PDF_j,					// j:	setlinejoin
	PDF_J,					// J:	setlinecap
	PDF_k,					// k:	setcmykcolor for fill
	PDF_K,					// K:	setcmykcolor for stroke
	PDF_l,					// l:	lineto
	PDF_m,					// m:	moveto
	PDF_M,					// M:	setmiterlimit
	PDF_MP,					// MP:	mark a place in the content stream
	PDF_n,					// n:	end patch without fill or stroke
	PDF_PS,					// PS:	PostScript for printing
	PDF_q,					// q:	save graphics state
	PDF_Q,					// Q:	restore graphics state
	PDF_re,					// re:	rectangle
	PDF_rg,					// rg:	setrgbcolor for fill
	PDF_ri,					// ri:	set the rendering intent
	PDF_RG,					// RG:	setrgbcolor for stroke
	PDF_s,					// s:	closepath and stroke path
	PDF_sc,					// sc:	setcolor for fill
	PDF_scn,				// scn: setcolor for fill in pattern and separation colorspaces
	PDF_sh,					// sh:	shaded fill
	PDF_S,					// S:	stroke path
	PDF_SC,					// SC:	setcolor for stroke
	PDF_SCN,				// SCN:	setcolor for stroke in pattern and separation colorspaces
	PDF_Tc,					// Tc:	set character spacing
	PDF_Td,					// Td:	move text current point
	PDF_TD,					// TD:	move text current point and set leading
	PDF_Tf,					// Tf:	set fone name and size
	PDF_Tj,					// Tj:	show text
	PDF_TJ,					// TJ:	show text allowing individual character spacing
	PDF_TL,					// TL:	set leading
	PDF_Tm,					// Tm:	set text matrix
	PDF_Tr,					// Tr:	set text rendering mode
	PDF_Ts,					// Ts:	set super/subsscripting text rise
	PDF_Tw,					// Tw:	set word spacing
	PDF_Tz,					// Tz:	set horizontal scaling
	PDF_Tstar,				// T*:	move to start of next line
	PDF_v,					// v:	curve to
	PDF_w,					// w:	set line width
	PDF_W,					// W:	clip
	PDF_Wstar,				// W*:	eoclip
	PDF_y,					// y:	curve to
	PDF_tick,				// ':	move to next line and show text
	PDF_ticktick,			// '':	move to next line and show text
	PDF_startarray,			// [:	start an array
	PDF_endarray,			// ]:	end an array
	PDF_startdict,			// <<:	start a dictionary
	PDF_enddict,			// >>:	end a dictionary
	PDF_R,					// R:	build a reference
	PDF_stream,				// stream: begin stream data
	PDF_endstream,			// endstream: end stream data
	PDF_obj,				// obj: begin object body
	PDF_endobj,				// endobj: end object body
	PDF_xref,				// xref subtable token
	PDF_startxref,			// start of xref table marker
	PDF_trailer,			// trailer dictionary marker
};

#endif
