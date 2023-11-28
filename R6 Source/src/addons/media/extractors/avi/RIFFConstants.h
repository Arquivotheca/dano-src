//---------------------------------------------------------------------
//
//	File:	RIFFConstants.h
//
//	Author:	Gene Z. Ragan
//
//	Date:	08.25.98
//
//	Desc:	RIFF Constants
//
//	Copyright ©1998 mediapede Software
//
//---------------------------------------------------------------------

#ifndef __RIFFCONSTANTS_H__
#define __RIFFCONSTANTS_H__

//	RIFF Types
const uint32 kRiff_AVI			= 0x41564920;	//	'AVI '
const uint32 kRiff_WAVE 		= 0x57415645;	//	'WAVE'

//	RIFF Chunk Identifiers
const uint32 kRiff_RIFF_Chunk 	= 0x52494646; 	//	'RIFF'
const uint32 kRiff_RIFX_Chunk 	= 0x52494658;	//	'RIFX'
const uint32 kRiff_LIST_Chunk	= 0x4C495354;	//	'LIST'
const uint32 kRiff_avih_Chunk	= 0x61766968;	//	'avih'
const uint32 kRiff_strd_Chunk 	= 0x73747264;	//	'strd'
const uint32 kRiff_strh_Chunk 	= 0x73747268;	//	'strh'
const uint32 kRiff_strf_Chunk 	= 0x73747266;	//	'strf'
const uint32 kRiff_vedt_Chunk	= 0x76656474;	//	'vedt'
const uint32 kRiff_JUNK_Chunk	= 0x4A554E4B;	//	'JUNK'
const uint32 kRiff_INFO_Chunk	= 0x494e464f;	//	'INFO'
const uint32 kRiff_idx1_Chunk 	= 0x69647831;	//	'idx1'

//	List Types
const uint32 kRiff_movi_Chunk 	= 0x6D6F7669;	//	'movi'
const uint32 kRiff_hdrl_Chunk 	= 0x6864726C;	//	'hdrl'
const uint32 kRiff_strl_Chunk 	= 0x7374726C;	//	'strl'

//	fcc Types
const uint32 kRiff_vids_Chunk 	= 0x76696473;	//	'vids'
const uint32 kRiff_auds_Chunk 	= 0x61756473;	//	'auds'
const uint32 kRiff_pads_Chunk 	= 0x70616473;	//	'pads'
const uint32 kRiff_txts_Chunk 	= 0x74787473;	//	'txts'
 
//	fcc Handlers
const uint32 kRiff_RLE_Chunk 	= 0x524c4520;	//	'RLE '
const uint32 kRiff_msvc_Chunk 	= 0x6D737663;	//	'msvc'
const uint32 kRiff_MSVC_Chunk 	= 0x4d535643;	//	'MSVC'

//	Microsoft WAV 
const uint32 kRiff_fmt_Chunk	= 0x666D7420;	//	'fmt '
const uint32 kRiff_data_Chunk 	= 0x64617461;	//	'data'


//	Chunk Names
const uint32  kRiff_FF00_Chunk	= 0xFFFF0000;	// 'FF00'
const uint32  kRiff_00_Chunk	= 0x30300000;	//	'
const uint32  kRiff_01_Chunk	= 0x30310000;
const uint32  kRiff_02_Chunk	= 0x30320000;
const uint32  kRiff_03_Chunk	= 0x30330000;
const uint32  kRiff_04_Chunk	= 0x30340000;
const uint32  kRiff_05_Chunk	= 0x30350000;
const uint32  kRiff_06_Chunk	= 0x30360000;
const uint32  kRiff_07_Chunk	= 0x30370000;
const uint32  kRiff_00pc_Chunk	= 0x30307063;
const uint32  kRiff_01pc_Chunk	= 0x30317063;
const uint32  kRiff_00db_Chunk	= 0x30306462;
const uint32  kRiff_00dc_Chunk	= 0x30306463;
const uint32  kRiff_01dc_Chunk	= 0x30316463;
const uint32  kRiff_00dx_Chunk	= 0x30306478;
const uint32  kRiff_00xx_Chunk	= 0x30307878;
const uint32  kRiff_00id_Chunk	= 0x30306964;
const uint32  kRiff_00rt_Chunk	= 0x30307274;
const uint32  kRiff_0021_Chunk	= 0x30303231;
const uint32  kRiff_00iv_Chunk	= 0x30306976;
const uint32  kRiff_0031_Chunk	= 0x30303331;
const uint32  kRiff_0032_Chunk	= 0x30303332;
const uint32  kRiff_00vc_Chunk	= 0x30305643;
const uint32  kRiff_00xm_Chunk	= 0x3030786D;
const uint32  kRiff_00wb_Chunk	= 0x30307762;
const uint32  kRiff_01wb_Chunk	= 0x30317762;


//	Video Codecs
#define RIFF_DIB  0x44494220
#define RIFF_cram 0x6372616D
#define RIFF_CRAM 0x4352414D
#define RIFF_wham 0x7768616d
#define RIFF_WHAM 0x5748414d
#define RIFF_rgb  0x00000000
#define RIFF_RGB  0x52474220
#define RIFF_rle8 0x01000000
#define RIFF_RLE8 0x524c4538
#define RIFF_rle4 0x02000000
#define RIFF_RLE4 0x524c4534
#define RIFF_none 0x0000FFFF
#define RIFF_NONE 0x4e4f4e45
#define RIFF_pack 0x0100FFFF
#define RIFF_PACK 0x5041434b
#define RIFF_tran 0x0200FFFF
#define RIFF_TRAN 0x5452414e
#define RIFF_ccc  0x0300FFFF
#define RIFF_CCC  0x43434320
#define RIFF_cyuv 0x63797576
#define RIFF_CYUV 0x43595556
#define RIFF_jpeg 0x0400FFFF
#define RIFF_JPEG 0x4A504547
#define RIFF_MJPG 0x4d4a5047
#define RIFF_mJPG 0x6d4a5047
#define RIFF_IJPG 0x494a5047
#define RIFF_rt21 0x72743231
#define RIFF_RT21 0x52543231
#define RIFF_iv31 0x69763331
#define RIFF_IV31 0x49563331
#define RIFF_iv32 0x69763332
#define RIFF_IV32 0x49563332
#define RIFF_iv41 0x69763431
#define RIFF_IV41 0x49563431
#define RIFF_cvid 0x63766964
#define RIFF_CVID 0x43564944
#define RIFF_ulti 0x554c5449
#define RIFF_ULTI 0x756c7469
#define RIFF_YUV9 0x59565539
#define RIFF_YVU9 0x59555639
#define RIFF_XMPG 0x584D5047
#define RIFF_xmpg 0x786D7067
#define RIFF_VDOW 0x56444f57
#define RIFF_MVI1 0x4D564931
#define RIFF_V422 0x56343232
#define RIFF_mvi1 0x6D766931
#define RIFF_MPIX 0x04006931		// MotionPixels munged their id

#define RIFF_YUY2 0x59555932
#define RIFF_YUV8 0x59555638
#define RIFF_WINX 0x57494e58
#define RIFF_WPY2 0x57505932

#define RIFF_Q1_0 0x51312e30
#define RIFF_SFMC 0x53464D43
#define RIFF_rpza 0x72707a61
#define RIFF_azpr 0x617a7072	//	Swapped Road Pizza

//	FND Chunk in Motion JPEG
#define RIFF_ISFT 0x49534654
#define RIFF_IDIT 0x49444954

const uint32 kRiff_00AM_Chunk		= 0x3030414d;	// 	'00AM'
const uint32 kRiff_DISP_Chunk 		= 0x44495350;	//	'DISP'
const uint32 kRiff_ISBJ_Chunk		= 0x4953424a;	//	'ISBJ'

const uint32 kRiff_rec_Chunk		= 0x72656320;		//	'rec '

//	Flags for AVIIndex Struct
const uint32 AVIIF_LIST          	= 0x00000001L;
const uint32 AVIIF_TWOCC         	= 0x00000002L;
        // keyframe doesn't need previous info to be decompressed
const uint32 AVIIF_KEYFRAME      	= 0x00000010L;
        // this chunk needs the frames following it to be used
const uint32 AVIIF_FIRSTPART     	= 0x00000020L;
        // this chunk needs the frames before it to be used
const uint32 AVIIF_LASTPART      	= 0x00000040L;
const uint32 AVIIF_MIDPART      	= (AVIIF_LASTPART|AVIIF_FIRSTPART);
        // this chunk doesn't affect timing ie palette change
const uint32 AVIIF_NOTIME        	= 0x00000100L;
const uint32 AVIIF_COMPUSE       	= 0x0FFF0000L;

		// XXXdbg this is totally non-standard but I need it...
const uint32 AVIIF_MUST_FREE		= 0x80000000L;


//	AVIHeader Flags        
const uint32  kAVIHasIndexFlag		 = 0x00000010;	// Has idx1 chunk
const uint32  kAVIMustUseIndexFlag	 = 0x00000020;	// Must use idx1 chunk to determine order
const uint32  kAVIIsInterleavedFlag	 = 0x00000100;	// File is interleaved        
const uint32  kAVIWasCaptureFileFlag = 0x00010000;	// Specially allocated used for capturing real time video	
const uint32  kAVICopyrightedFlag	 = 0x00020000;	// Contains copyrighted data

//	AVIStreamHeader Flags
const uint32  AVISF_DISABLED		 = 0x00000001;
const uint32  AVISF_VIDEO_PALCHANGES = 0x00010000;


//	Microsoft Audio Types
#define WAVE_FORMAT_UNKNOWN			(0x0000)
#define WAVE_FORMAT_PCM				(0x0001)
#define WAVE_FORMAT_ADPCM			(0x0002)
#define WAVE_FORMAT_IBM_CVSD		(0x0005)
#define WAVE_FORMAT_ALAW			(0x0006)
#define WAVE_FORMAT_MULAW			(0x0007)
#define WAVE_FORMAT_OKI_ADPCM		(0x0010)
#define WAVE_FORMAT_DVI_ADPCM		(0x0011)
#define WAVE_FORMAT_DIGISTD			(0x0015)
#define WAVE_FORMAT_DIGIFIX			(0x0016)
#define WAVE_FORMAT_YAMAHA_ADPCM	(0x0020)
#define WAVE_FORMAT_DSP_TRUESPEECH  (0x0022)
#define WAVE_FORMAT_GSM610			(0x0031)
#define WAVE_FORMAT_MPEG			(0x0055)
#define IBM_FORMAT_MULAW			(0x0101)
#define IBM_FORMAT_ALAW				(0x0102)
#define IBM_FORMAT_ADPCM			(0x0103)

#endif
