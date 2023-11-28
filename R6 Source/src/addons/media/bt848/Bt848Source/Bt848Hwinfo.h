/*
	
	Bt848Hwinfo.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
	
*/

#ifndef BT848_HWINFO_H
#define BT848_HWINFO_H

/* Teletext */
#define HEE_TELETEXT_YES     0x01
#define HEE_TELETEXT_NO      0x00

/* Teletext Models/IDs */
#define HEE_TELETEXT_None	0
#define HEE_TELETEXT_Unknown	1	//some model does exist but can't be determined which
#define HEE_TELETEXT_SAA5246	2	// 'Original'
#define HEE_TELETEXT_SAA5284	3

/* Decoder values. */
#define HEE_DECODER1_RESERV1  0x00
#define HEE_DECODER1_RESERV2  0xFF
#define HEE_DECODER1_BT815       1
#define HEE_DECODER1_BT817       2
#define HEE_DECODER1_BT819       3
#define HEE_DECODER1_BT815A      4
#define HEE_DECODER1_BT817A      5
#define HEE_DECODER1_BT819A      6
#define HEE_DECODER1_BT827       7
#define HEE_DECODER1_BT829       8
#define HEE_DECODER1_BT848       9
#define HEE_DECODER1_BT848A      10
#define HEE_DECODER1_BT849A      11
#define HEE_DECODER1_BT829A      12
#define HEE_DECODER1_BT827A		13
#define HEE_DECODER1_BT878		14
#define HEE_DECODER1_BT879		15
#define HEE_DECODER1_BT880		16

/* UPDATE HEE.CPP sDecoderModels IF MAKE CHANGES HERE */


/* Video Formats */
#define HEE_VidForm_NTSC_M            0x01
#define HEE_VidForm_PAL_BGHIDK        0x02
#define HEE_VidForm_SECAM_L           0x04
#define HEE_VidForm_PAL_M             0x08
#define HEE_VidForm_PAL_N             0x10
#define HEE_VidForm_NTSC_443          0x20
#define HEE_VidForm_PAL_NCOMBO        0x40
#define HEE_VidForm_NTSC_PAL_SECAM    0x7F  // FGR - default format for all non-848 boards
#define HEE_VidForm_NTSC              (HEE_VidForm_NTSC_M | HEE_VidForm_NTSC_443)
#define HEE_VidForm_PAL               (HEE_VidForm_PAL_BGHIDK | HEE_VidForm_PAL_M | HEE_VidForm_PAL_N | HEE_VidForm_PAL_NCOMBO)
#define HEE_VidForm_SECAM		      (HEE_VidForm_SECAM_L)
#define HEE_VidForm_M				  (HEE_VidForm_NTSC_M | HEE_VidForm_PAL_M)
#define HEE_VidForm_BGHIDKNL443		  (HEE_VidForm_PAL_BGHIDK|HEE_VidForm_PAL_N/*|HEE_VidForm_PAL_NCOMBO*/|HEE_VidForm_SECAM_L|HEE_VidForm_NTSC_443)


/* Tuner Types */
#define HEE_TUNERTYPE_NONE           0x00
#define HEE_TUNERTYPE_OTHER          0x01
#define HEE_TUNERTYPE_FM             0x02 /* radio */
#define HEE_TUNERTYPE_BG             0x04
#define HEE_TUNERTYPE_MN             0x08
#define HEE_TUNERTYPE_I              0x10
#define HEE_TUNERTYPE_L              0x20 /* includes L' */
#define HEE_TUNERTYPE_DK             0x40

/* Tuner values. */
#define HEE_TUNER_NONE               0
#define HEE_TUNER_EXTERNAL           1
#define HEE_TUNER_OTHER              2
#define HEE_TUNER_FI1216             3 /* BG */
#define HEE_TUNER_FI1216MF           4 /* BGLL' */
#define HEE_TUNER_FI1236             5 /* MN */
#define HEE_TUNER_FI1246             6 /* I */
#define HEE_TUNER_FI1256             7 /* DK */
#define HEE_TUNER_FI1216_MK2         8 /* BG */
#define HEE_TUNER_FI1216MF_MK2       9 /* BGLL' */
#define HEE_TUNER_FI1236_MK2         10 /* MN */
#define HEE_TUNER_FI1246_MK2         11 /* I */
#define HEE_TUNER_FI1256_MK2         12 /* DK */
#define HEE_TUNER_4032FY5			13 /* NTSC - Temic */
#define HEE_TUNER_4002FH5			14 /* BG - Temic */
#define HEE_TUNER_4062FY5            15 /* I */
#define HEE_TUNER_FR1216_MK2         16 /* BG */
#define HEE_TUNER_FR1216MF_MK2       17 /* BGLL' */
#define HEE_TUNER_FR1236_MK2         18 /* MN */
#define HEE_TUNER_FR1246_MK2         19 /* I */
#define HEE_TUNER_FR1256_MK2         20 /* DK */
#define HEE_TUNER_FM1216         	21 /* BG */
#define HEE_TUNER_FM1216MF       	22 /* BGLL' */
#define HEE_TUNER_FM1236         	23 /* MN */
#define HEE_TUNER_FM1246				24 /* I */
#define HEE_TUNER_FM1256				25 /* DK */
#define HEE_TUNER_4036FY5			26 /* MN - temic FI1236 MK2 clone */

#define HEE_TUNER_TCPN9082D			27 /* MN - SAMSUNG */
#define HEE_TUNER_TCPM9092P			28 /* Pal BG/I/DK SAMSUNG*/
#define HEE_TUNER_4006FH5			29 /* BG - temic clone */
#define HEE_TUNER_TCPN9085D			30 /* MN/Radio - SAMSUNG */
#define HEE_TUNER_TCPB9085P			31 /* Pal BG/I/DK / Radio SAMSUNG*/
#define HEE_TUNER_TCPL9091P			32 /* Pal BG & Secam L/L' Samsung */

//#define HEE_TUNER_4042FM5			 /* BGILL' */
/* UPDATE HEE.CPP sTunerModels IF MAKE CHANGES HERE */


/* Balun values. */
#define HEE_BALUN_NONE				0
#define HEE_BALUN_TVR101			1  /* Paragon */
#define HEE_BALUN_BOB				2
#define HEE_BALUN_BOBPlus			3
/* UPDATE HEE.CPP sBalunModels IF MAKE CHANGES HERE */

/* FM Radio values */
#define HEE_FM_NO	0x00
#define HEE_FM_YES	0x01
#define HEE_RDBS_NO	0x00
#define HEE_RDBS_YES	0x01

/* IR values */
#define HEE_IR_NO	0x00
#define HEE_IR_YES	0x01

/* VBI Sampling values */
#define HEE_VBISAMPLING_NO	0x00
#define HEE_VBISAMPLING_YES	0x01

/* Audio Hardware Features. */
#define HEE_AUDIO_NONE               0
#define HEE_AUDIO_No_IC				0
#define HEE_AUDIO_TEA6300            1
#define HEE_AUDIO_TEA6320            2
#define HEE_AUDIO_TDA9850            3
#define HEE_AUDIO_MSP3400C			4
#define HEE_AUDIO_MSP3410D			5
#define HEE_AUDIO_MSP3415			6
#define HEE_AUDIO_MSP3430			7
#define HEE_AUDIO_INTERNAL_OUT_YES   0x01
#define HEE_AUDIO_INTERNAL_OUT_NO    0x00
#define HEE_AUDIO_BACKPANEL_OUT_YES  0x02
#define HEE_AUDIO_BACKPANEL_OUT_NO   0x00

/* External Inputs and SVideo */
#define HEE_MASK_EXTINPUT   0x03
#define HEE_External1      0x01
#define HEE_External2      0x02
#define HEE_MASK_SVIDEO     0x03
#define HEE_SVideo1        0x01
#define HEE_SVideo2        0x02

/* Quality Control */
#define HEE_QC_LOCATION     0x07
#define HEE_QC_PERSONNEL    0x1F

#endif
