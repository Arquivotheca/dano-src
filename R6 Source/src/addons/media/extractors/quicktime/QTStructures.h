// QTStructures.h by Simon Clarke (S.J.Clarke@herts.ac.uk) - Copyright 1996-8

#ifndef __QTSTRUCTURES_H
#define __QTSTRUCTURES_H

// show debug texts?
#if DEBUG
#define DEBUGONE // std debugging
#define DEBUGTWO // show chunk info
#define DEBUG_MOVIEINFO // show codec stuff
#define DEBUGSAVER // show saving info
#endif

#define QT_OK B_OK
#define QT_ERROR B_ERROR

//#define DEBUG 1
#include <Debug.h>

enum qt_track_type {

		QT_VIDEO,
		QT_AUDIO,
		QT_IMAGE,
		QT_TEXT,
		QT_TIMECODE,
		QT_TWEEN,
		QT_3D,
		QT_EFFECT,
		QT_TRANSITION,
		QT_URL,
		QT_UNKNOWN
};

const int32 QT_dcom = 'dcom';
const int32 QT_cmvd = 'cmvd';
const int32 QT_cmov = 'cmov';
const int32 QT_moov = 'moov';
const int32 QT_trak = 'trak';
const int32 QT_mdia = 'mdia';
const int32 QT_minf = 'minf';
const int32 QT_stbl = 'stbl';

const int32 QT_edts = 'edts';
const int32 QT_hdlr = 'hdlr';
const int32 QT_mvhd = 'mvhd';
const int32 QT_tkhd = 'tkhd';
const int32 QT_elst = 'elst';
const int32 QT_mdhd = 'mdhd';
const int32 QT_stsd = 'stsd';
const int32 QT_stts = 'stts';
const int32 QT_stss = 'stss';
const int32 QT_stsc = 'stsc';
const int32 QT_stsz = 'stsz';
const int32 QT_stco = 'stco';
const int32 QT_load = 'load';

// video codec list

const int32 QT_rle  = 'rle ';
const int32 QT_smc  = 'smc ';
const int32 QT_rpza = 'rpza';
const int32 QT_azpr = 'zapr';
const int32 QT_CVID = 'CVID';
const int32 QT_cvid = 'cvid';
const int32 QT_jpeg = 'jpeg';
const int32 QT_SPIG = 'SPIG';
const int32 QT_yuv2 = 'yuv2';
const int32 QT_PGVV = 'PGVV';
const int32 QT_YUV9 = 'YUV9';
const int32 QT_YVU9 = 'YUV9';
const int32 QT_RT21 = 'RT21';
const int32 QT_rt21 = 'rt21';
const int32 QT_IV31 = 'IV31';
const int32 QT_iv31 = 'iv31';
const int32 QT_IV32 = 'IV32';
const int32 QT_iv32 = 'iv32';
const int32 QT_IV41 = 'IV41';
const int32 QT_iv41 = 'iv41';
const int32 QT_kpcd = 'kpcd';
const int32 QT_KPCD = 'KPCD';
const int32 QT_cram = 'cram';
const int32 QT_CRAM = 'CRAM';
const int32 QT_wham = 'wham';
const int32 QT_WHAM = 'WHAM';
const int32 QT_msvc = 'msvc';
const int32 QT_MSVC = 'MSVC';

// audio codec list

#define QT_raw   0x72617720
#define QT_raw00 0x00000000
#define QT_twos  0x74776f73
#define QT_MAC6  0x4d414336
#define QT_ima4  0x696d6134

// misc

#define QT_vmhd 0x766D6864
#define QT_dinf 0x64696e66
#define QT_appl 0x6170706C
#define QT_wide 0x77696465 
#define QT_mdat 0x6D646174
#define QT_smhd 0x736d6864
#define QT_stgs 0x73746773
#define QT_udta 0x75647461
#define QT_skip 0x736B6970
#define QT_gmhd 0x676d6864
#define QT_text 0x74657874
#define QT_clip 0x636C6970   /* clip ??? contains crgn atom  */
#define QT_crgn 0x6372676E   /* crgn ??? contain coordinates?? */
#define QT_ctab 0x63746162   /* ctab: color table for 16/24 anims on 8 bit */

typedef struct {
  	int32 version; 
  	int32 creation; 
  	int32 modtime;
  	int32 timescale;
  	int32 duration;
  	int32 rate;
  	int32 volume;
  	int32  r1;
  	int32  r2;
  	int32 matrix[3][3];
  	int32 r3;
  	int32  r4;
  	int32 pv_time;
  	int32 pv_durat;
  	int32 post_time;
  	int32 sel_time;
  	int32 sel_durat;
  	int32 cur_time;
  	int32 nxt_tk_id;
} qt_mvhd_details;

// QuickTimeMVHD

typedef struct {
  	char version;
  	char trackv1;
  	char trackv2;
  	char trackv3;
  	
  	int32 creation;
  	int32 modtime;
  	int32 trackid;
  	int32 timescale;
  	int32 duration;
  	int32 time_off;
  	int32 priority;
  	int16 layer; // 16
  	int32 alt_group; // 16
  	int32 volume; // 16
  	int32 matrix[3][3];
  	int32 tk_width;
  	int32 tk_height;
  	int32 pad;
} qt_tkhd_details;

//QuickTimeTKHD;

#define TRAK_ENABLED 0x0001
#define TRAK_INMOVIE 0x0002
#define TRAK_INPREVIEW 0x0004
#define TRAK_INPOSTER 0x0008

typedef struct {
	int32 sample_des;
	int32 width;
  	int32 height;
  	int32 depth;
  	int32 compression;
} QuickTimeCODECHDR;

typedef struct {
  	int32 version;
  	int32 creation;
  	int32 modtime;
  	int32 timescale;
  	int32 duration;
  	int32 language; // 16
  	int32 quality; // 16
} qt_mdhd_details;

//QuickTimeMDHD;

typedef struct {
  	int32 type;
  	int32 subtype;
  	int32 vendor;
  	int32 flags;
  	int32 mask;
} qt_hdlr_details;

//QuickTimeHDLR;

typedef struct {
  	int32 cnt;
  	int32 time;
  	int32 timelo;
} QuickTimeT2HDR;

typedef struct {
  	uchar r,g,b,pad;
  	uchar	u, v, y;
} CVID_Color;

typedef struct {
	uchar	r,g,b,a;
} ColourMap;

//typedef struct {
//  	uint32 first;
//  	uint32 num;
//  	uint32 tag;
//} QT_S2CHUNK_HDR;

typedef struct {
  	uint32 first;
  	uint32 num;
  	uint32 tag;
} qt_stsc_details;

typedef struct {
  	uint32 	cnt;
  	float 	time;
} qt_stts_details;

typedef struct {
	int32	duration;
	int32	mtime;
	int32	mrate;
} QuickTimeELST;

typedef struct {
	uint32	chunkoff;
	uint32	soundsize;
} SoundHeader;

// index entry for list
typedef struct {
	int32 	itemsize;
	off_t	itemposition;
} IndexEntry;

extern void _QTdebugprint(const char *format, ...);

#ifdef DEBUGONE
	#define DEBUGTEXT(text) _QTdebugprint(text)
#else
	#define DEBUGTEXT(text) (void)0	
#endif 

#ifdef DEBUGTWO
	#define DEBUGCHUNK(text) _QTdebugprint(text)
#else
	#define DEBUGCHUNK(text) (void)0	
#endif 

#endif
