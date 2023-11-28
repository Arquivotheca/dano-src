
#if !defined(audio_base_h)
#define audio_base_h

/* sample rate values */
/* various fixed sample rates we support (for hard-sync clocked values) */
#define B_SR_8000 0x1
#define B_SR_11025 0x2
#define B_SR_12000 0x4
#define B_SR_16000 0x8
#define B_SR_22050 0x10
#define B_SR_24000 0x20
#define B_SR_32000 0x40
#define B_SR_44100 0x80
#define B_SR_48000 0x100
#define B_SR_64000 0x200
#define B_SR_88200 0x400
#define B_SR_96000 0x800
#define B_SR_176400 0x1000
#define B_SR_192000 0x2000
#define B_SR_384000 0x4000
#define B_SR_1536000 0x10000
/* continuously variable sample rate (typically board-generated) */
#define B_SR_CVSR 0x10000000UL
/* sample rate parameter global to all channels (input and output rates respectively) */
#define B_SR_IS_GLOBAL 0x80000000UL
/* output sample rate locked to input sample rate (output_rates only; the common case!) */
#define B_SR_SAME_AS_INPUT 0x40000000UL


/* format values */
/* signed char */
#define B_FMT_8BIT_S 0x01
/* unsigned char -- this is special case */
#define B_FMT_8BIT_U 0x02
/* traditional 16 bit signed format (host endian) */
#define B_FMT_16BIT 0x10
/* left-adjusted in 32 bit signed word */
#define B_FMT_18BIT 0x20
#define B_FMT_20BIT 0x40
#define B_FMT_24BIT 0x100
#define B_FMT_32BIT 0x1000
/* 32-bit floating point, -1.0 to 1.0 */
#define B_FMT_FLOAT 0x20000
/* 64-bit floating point, -1.0 to 1.0 */
#define B_FMT_DOUBLE 0x40000
/* 80-bit floating point, -1.0 to 1.0 */
#define B_FMT_EXTENDED 0x80000
/* bit stream */
#define B_FMT_BITSTREAM 0x1000000
/* format parameter global to all channels (input and output formats respectively) */
#define B_FMT_IS_GLOBAL 0x80000000UL
/* output format locked to input format (output_formats) */
#define B_FMT_SAME_AS_INPUT 0x40000000UL

/* designation values */
/* mono channels have no designation */
#define B_CHANNEL_LEFT 0x1
#define B_CHANNEL_RIGHT 0x2
#define B_CHANNEL_CENTER 0x4			/* 5.1+ or fake surround */
#define B_CHANNEL_SUB 0x8				/* 5.1+ */
#define B_CHANNEL_REARLEFT 0x10			/* quad surround or 5.1+ */
#define B_CHANNEL_REARRIGHT 0x20		/* quad surround or 5.1+ */
#define B_CHANNEL_FRONT_LEFT_CENTER 0x40
#define B_CHANNEL_FRONT_RIGHT_CENTER 0x80
#define B_CHANNEL_BACK_CENTER 0x100		/* 6.1 or fake surround */
#define B_CHANNEL_SIDE_LEFT 0x200
#define B_CHANNEL_SIDE_RIGHT 0x400
#define B_CHANNEL_TOP_CENTER 0x800
#define B_CHANNEL_TOP_FRONT_LEFT 0x1000
#define B_CHANNEL_TOP_FRONT_CENTER 0x2000
#define B_CHANNEL_TOP_FRONT_RIGHT 0x4000
#define B_CHANNEL_TOP_BACK_LEFT 0x8000
#define B_CHANNEL_TOP_BACK_CENTER 0x10000
#define B_CHANNEL_TOP_BACK_RIGHT 0x20000

enum media_multi_matrix {
	B_MATRIX_PROLOGIC_LR = 0x1,
	B_MATRIX_AMBISONIC_WXYZ = 0x4
};

#endif	/* audio_base_h */

