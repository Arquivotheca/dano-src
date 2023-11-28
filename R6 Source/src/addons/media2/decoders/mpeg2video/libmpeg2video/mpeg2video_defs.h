#ifndef C_MPEG2_VIDEO_DEFS_H

#define C_MPEG2_VIDEO_DEFS_H

#define C_PACKET_START_CODE_PREFIX 1

#define C_MPEG_PROGRAM_END_CODE 0x1b9
#define C_SEQUENCE_HEADER_CODE 0x1b3
#define C_EXTENSION_START_CODE 0x1b5
#define C_GOP_START_CODE 0x1b8
#define C_PICTURE_START_CODE 0x100
#define C_SEQUENCE_END_CODE 0x1b7
#define C_USER_DATA_START_CODE 0x1b2

#define C_EXTENSION_STARTCODE_SEQUENCE 					1
#define C_EXTENSION_STARTCODE_SEQUENCE_DISPLAY			2
#define C_EXTENSION_STARTCODE_QUANT_MATRIX				3
#define C_EXTENSION_STARTCODE_COPYRIGHT					4
#define C_EXTENSION_STARTCODE_SEQUENCE_SCALABLE			5
#define C_EXTENSION_STARTCODE_PICTURE_DISPLAY			7
#define C_EXTENSION_STARTCODE_PICTURE_CODING			8
#define C_EXTENSION_STARTCODE_PICTURE_SPATIAL_SCALABLE	9
#define C_EXTENSION_STARTCODE_PICTURE_TEMPORAL_SCALABLE	10

#define C_DATA_PARTITIONING 	0
#define C_SPATIAL_SCALABILITY 	1
#define C_SNR_SCALABILITY 		2
#define C_TEMPORAL_SCALABILITY 	3

typedef enum mpeg2_picture_coding_
{
	NONE	  = -1,
	I_PICTURE = 1,
	P_PICTURE = 2,
	B_PICTURE = 3
} mpeg2_picture_coding_t;

typedef enum mpeg2_macroblock_
{
	SPATIAL_TEMPORAL_WEIGHT_CODE_FLAG 	= 1,
	MACROBLOCK_INTRA					= 2,
	MACROBLOCK_PATTERN					= 4,
	MACROBLOCK_MOTION_BACKWARD			= 8,
	MACROBLOCK_MOTION_FORWARD			= 0x10,
	MACROBLOCK_QUANT					= 0x20
} mpeg2_macroblock_t;

typedef enum mpeg2_picture_structure_
{
	TOP_FIELD		= 1,
	BOTTOM_FIELD	= 2,
	FRAME_PICTURE 	= 3
} mpeg2_picture_structure_t;

typedef enum mpeg2_motion_vector_format_
{
	FIELD_MOTION_VECTOR = 1,
	FRAME_MOTION_VECTOR = 2
} mpeg2_motion_vector_format_t;

typedef enum mpeg2_chroma_format_
{
	YUV420 = 1,
	YUV422 = 2,
	YUV444 = 3
} mpeg2_chroma_format_t;

typedef enum mpeg2_fieldframe_motion_
{
	FIELD_BASED		= 1,
	FRAME_BASED		= 2,		// for frame pictures
	MC16x8			= 2,		// for field pictures
	DUAL_PRIME		= 3
} mpeg2_fieldframe_motion_t;

typedef enum mpeg2_special_code_
{
	MPEG2VIDEO_ZERO_FILLER = -1,
	MPEG2VIDEO_FIRST_NONRESERVED_CODE = 1,
	MPEG2VIDEO_SHUTDOWN = MPEG2VIDEO_FIRST_NONRESERVED_CODE,
	MPEG2VIDEO_FLUSH,
	MPEG2VIDEO_REPEAT_LAST,
	MPEG2VIDEO_LATE
	
} mpeg2_special_code_t;

#endif
