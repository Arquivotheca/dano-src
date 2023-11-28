/* global.h, global variables                                               */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include "mpegdec.h"

/* choose between declaration (GLOBAL undefined)
 * and definition (GLOBAL defined)
 * GLOBAL is defined in exactly one file mpeg2dec.c)
 */

#ifndef GLOBAL
#define EXTERN extern
#else
#define EXTERN
#endif

#define RDBFS 2048

struct video_data;

/* prototypes of global functions */

/* getbits.c */
void Initialize_Buffer(struct video_data *);
void Fill_Buffer(struct video_data *);
unsigned int Show_Bits(int n,struct video_data *);
void Flush_Buffer(int n,struct video_data *);
unsigned int Get_Bits(int n,struct video_data *);
void Flush_Buffer32(video_data *);
unsigned int Get_Bits32(video_data *);

/* getblk.c */
void Decode_MPEG1_Intra_Block(int comp, int dc_dct_pred[],struct video_data *);
void Decode_MPEG1_Non_Intra_Block(int comp,struct video_data *);

/* gethdr.c */
int Get_Hdr(struct video_data *);
void next_start_code(struct video_data *);
int slice_header(struct video_data *);
void marker_bit(char *text,struct video_data *);

/* getpic.c */
void Decode_Picture(struct video_data *ld);

/* getvlc.c */
int Get_macroblock_type(struct video_data *);
int Get_motion_code(struct video_data *);
int Get_dmvector(struct video_data *);
int Get_coded_block_pattern(struct video_data *);
int Get_macroblock_address_increment(struct video_data *);
int Get_Luma_DC_dct_diff(struct video_data *);
int Get_Chroma_DC_dct_diff(struct video_data *);

/* idct.c */
int Fast_IDCT(short *block);

/* mpegdec.c */
void Error(char *text);
void Warning(char *text);
void Print_Bits(int code, int bits, int len);
void Initialize_Sequence(struct video_data *ld);
void Deinitialize_Sequence(struct video_data *ld);
int checkVheader(struct video_data *ld);
void deleteMVideo(struct video_data *ld);
struct video_data *newMVideo(void *obj,int (*rf)(void*,void*,int));

/* recon.c */
void form_predictions(int bx, int by, int macroblock_type, 
  int PMV[2][2], struct video_data *ld);

/* global variables */

EXTERN char Version[]
#ifdef GLOBAL
  ="mpeg2decode V1.2a, 96/07/19"
#endif
;

EXTERN char Author[]
#ifdef GLOBAL
  ="(C) 1996, MPEG Software Simulation Group"
#endif
;


/* zig-zag scan pattern */

EXTERN unsigned char scan[64]
#ifdef GLOBAL
=
{
    0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
    12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
    35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
}
#endif
;

/* zig-zag scan pattern */
/*MMX -> transposition */
/*
EXTERN unsigned char scan[64]
#ifdef GLOBAL
=
{
    0,8,1,2,9,16,24,17,10,3,4,11,18,25,32,40,33,26,19,12,5,
    6,13,20,27,34,41,48,56,49,42,35,28,21,14,7,15,22,29,36,43,50,57,
    58,51,44,37,30,23,31,38,45,52,59,60,53,46,39,47,54,61,62,55,63
}
#endif
;

*/
/*
EXTERN unsigned char scan_mmx[64]
#ifdef GLOBAL
=
{
    0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
    12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
    35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
}
#endif
;
*/

EXTERN unsigned char scan_mmx[64]
#ifdef GLOBAL
=
{
    0,4,8,16,12,1,5,9,20,24,32,28,17,13,2,
    6,10,21,25,36,40,48,44,33,29,18,14,3,7,11,22,26,37,41,52,56,
    60,49,45,34,30,19,15,23,27,38,42,53,57,
    61,50,46,35,31,39,43,54,58,62,51,47,55,59,63
    
}
#endif
;

extern void (*idct)(short*);

/* color space conversion coefficients
 * for YCbCr -> RGB mapping
 *
 * entries are {crv,cbu,cgu,cgv}
 *
 * crv=(255/224)*65536*(1-cr)/0.5
 * cbu=(255/224)*65536*(1-cb)/0.5
 * cgu=(255/224)*65536*(cb/cg)*(1-cb)/0.5
 * cgv=(255/224)*65536*(cr/cg)*(1-cr)/0.5
 *
 * where Y=cr*R+cg*G+cb*B (cr+cg+cb=1)
 */

/* ISO/IEC 13818-2 section 6.3.6 sequence_display_extension() */

//EXTERN int Inverse_Table_6_9[8][4]
//#ifdef GLOBAL
//=
//{
//  {117504, 138453, 13954, 34903}, /* no sequence_display_extension */
//  {117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
//  {104597, 132201, 25675, 53279}, /* unspecified */
//  {104597, 132201, 25675, 53279}, /* reserved */
//  {104448, 132798, 24759, 53109}, /* FCC */
//  {104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
//  {104597, 132201, 25675, 53279}, /* SMPTE 170M */
//  {117579, 136230, 16907, 35559}  /* SMPTE 240M (1987) */
//}
//#endif
//;


/* buffers for multiuse purposes */
EXTERN char Error_Text[256];

struct video_data {
	/* bit input */
	unsigned char *Rdptr;
	unsigned char Rdbfr[RDBFS];
	unsigned int Bfr;
	int Incnt;
	int Bitcnt;
	
	/* sequence header and quant_matrix_extension() */
	int intra_quantizer_matrix[64];
	int non_intra_quantizer_matrix[64];
	
	/* slice/macroblock */
	int quantizer_scale;
	//short block[12][64];
	short ** block;
	short ** ppblock;
	
	int (*streamRead)(void *obj,void *data,int len);
	void *streamObj;
	int Fault_Flag;
	
	/* pointers to generic picture buffers */
	unsigned char *backward_reference_frame[4];
	unsigned char *forward_reference_frame[4];
	
	unsigned char *auxframe[4];
	unsigned char *current_frame[3];
	
	/* non-normative variables derived from normative elements */
	int Coded_Width;
	int Coded_Height;
	int Chroma_Width;
	int Chroma_Height;
	
	/* normative derived variables (as per ISO/IEC 13818-2) */
	int horizontal_size;
	int vertical_size;
	int mb_width;
	int mb_height;
	int bit_rate;
	double frame_rate; 
	int temporal_reference;
	int picture_coding_type;
	int full_pel_forward_vector;
	int forward_f_code;
	int full_pel_backward_vector;
	int backward_f_code;	
	int True_Framenum;
	int Ref_Framenum;
	
	// gethdr.c
	int Temporal_Reference_Base;
	int True_Framenum_max;
	int Temporal_Reference_GOP_Reset;
	int temporal_reference_wrap;
	int temporal_reference_old;
	int vbv_buffer_size;

	int picture_num;
	struct {
		long long t;
		int baseframe;
	} timebase;
};

typedef struct video_data video_data;

#define	xShow_Bits(N,ld) (ld->Bfr >> (32-N))
