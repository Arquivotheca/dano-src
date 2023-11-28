
/*
 *	Definition for a game_audio device driver.
 *	Copyright 2000-2001 Be, Incorporated. All rights reserved.
 *
 *  NOTE: AC97 values often correlate to bit position/register.
 *
 */


#if !defined(game_audio_h)
#define game_audio_h

#include <Drivers.h>
#include <Debug.h>
#include <audio_base.h>

#define B_GAME_DRIVER_BASE (B_AUDIO_DRIVER_BASE+60)
#define GAME_CURRENT_API_VERSION 1

#if !defined(__cplusplus)
typedef enum game_audio_opcode_v1 game_audio_opcode;
typedef enum game_codec_info_stream_flags_v1 game_codec_info_stream_flags;
typedef enum game_codec_format_flags_v1 game_codec_format_flags;
typedef enum game_mixer_control_type_v1 game_mixer_control_type;
typedef enum game_mixer_control_flags_v1 game_mixer_control_flags;
typedef enum game_mixer_level_info_flags_v1 game_mixer_level_info_flags;
typedef enum game_mixer_level_info_type_v1 game_mixer_level_info_type;
typedef enum game_mixer_mux_info_flags_v1 game_mixer_mux_info_flags;
typedef enum game_mixer_mux_info_type_v1 game_mixer_mux_info_type;
typedef enum game_mixer_enable_info_flags_v1 game_mixer_enable_info_flags;
typedef enum game_mixer_enable_info_type_v1 game_mixer_enable_info_type;
typedef enum game_mixer_level_value_flags_v1 game_mixer_level_value_flags;
typedef enum game_mixer_control_value_status_v1 game_mixer_control_value_status;
typedef enum game_open_stream_request_v1 game_open_stream_request;
typedef enum game_stream_state_v1 game_stream_state;
typedef enum game_timing_flags_v1 game_timing_flags;
typedef enum game_stream_volume_mapping_v1 game_stream_volume_mapping;
typedef enum game_set_stream_data_flags_v1 game_set_stream_data_flags;
typedef enum game_close_stream_flags_v1 game_close_stream_flags;
typedef enum game_open_stream_buffer_flags_v1 game_open_stream_buffer_flags;
#else
extern "C" {
#endif

typedef struct game_get_info_v1 game_get_info;
typedef struct game_stream_capabilities_v1 game_stream_capabilities;
typedef struct game_codec_info_v1 game_codec_info;
typedef struct game_get_codec_infos_v1 game_get_codec_infos;
typedef struct game_codec_format_v1 game_codec_format;
typedef struct game_set_codec_formats_v1 game_set_codec_formats;
typedef struct game_mixer_info_v1 game_mixer_info;
typedef struct game_get_mixer_infos_v1 game_get_mixer_infos;
typedef struct game_mixer_control_v1 game_mixer_control;
typedef struct game_get_mixer_controls_v1 game_get_mixer_controls;
typedef struct game_get_mixer_level_info_v1 game_get_mixer_level_info;
typedef struct game_mux_item_v1 game_mux_item;
typedef struct game_get_mixer_mux_info_v1 game_get_mixer_mux_info;
typedef struct game_get_mixer_enable_info_v1 game_get_mixer_enable_info;
typedef struct game_mixer_level_value_v1 game_mixer_level_value;
typedef struct game_mixer_mux_value_v1 game_mixer_mux_value;
typedef struct game_mixer_enable_value_v1 game_mixer_enable_value;
typedef struct game_mixer_control_value_v1 game_mixer_control_value;
typedef struct game_get_mixer_control_values_v1 game_get_mixer_control_values;
typedef struct game_set_mixer_control_values_v1 game_set_mixer_control_values;
typedef struct game_open_stream_v1 game_open_stream;
typedef struct game_stream_timing_v1 game_stream_timing;
typedef struct game_get_timing_v1 game_get_timing;
typedef struct game_stream_controls_v1 game_stream_controls;
typedef struct game_get_stream_controls_v1 game_get_stream_controls;
typedef struct game_set_stream_controls_v1 game_set_stream_controls;
typedef struct game_set_stream_buffer_v1 game_set_stream_buffer;
typedef struct game_run_stream_v1 game_run_stream;
typedef struct game_run_streams_v1 game_run_streams;
typedef struct game_close_stream_v1 game_close_stream;
typedef struct game_open_stream_buffer_v1 game_open_stream_buffer;
typedef struct game_open_stream_buffers_v1 game_open_stream_buffers;
typedef struct game_close_stream_buffer_v1 game_close_stream_buffer;
typedef struct game_close_stream_buffers_v1 game_close_stream_buffers;
typedef struct game_get_device_state_v1 game_get_device_state;
typedef struct game_set_device_state_v1 game_set_device_state;
typedef struct game_get_interface_info_v1 game_get_interface_info;
typedef struct game_get_interfaces_v1 game_get_interfaces;

//#pragma mark game_audio_opcode_v1  (enum)
enum game_audio_opcode_v1  {
	/* get general hardware (card) info */
	GAME_GET_INFO = B_GAME_DRIVER_BASE,
	/* get info on specific inputs/outputs, and configure them */
	GAME_GET_CODEC_INFOS,
	_GAME_UNUSED_OPCODE_0_,
	GAME_SET_CODEC_FORMATS,
	/* mixer information */
	GAME_GET_MIXER_INFOS,
	GAME_GET_MIXER_CONTROLS,
	GAME_GET_MIXER_LEVEL_INFO,
	GAME_GET_MIXER_MUX_INFO,
	GAME_GET_MIXER_ENABLE_INFO,
	/*	we specifically omit specific control connection info for now	*/
	GAME_MIXER_UNUSED_OPCODE_1_,
	/* mixer control values */
	GAME_GET_MIXER_CONTROL_VALUES,
	GAME_SET_MIXER_CONTROL_VALUES,
	/* stream management */
	GAME_OPEN_STREAM,
	GAME_GET_TIMING,
	_GAME_UNUSED_OPCODE_2_,
	GAME_GET_STREAM_CONTROLS,
	GAME_SET_STREAM_CONTROLS,
	_GAME_UNUSED_OPCODE_3_,
	GAME_SET_STREAM_BUFFER,
	GAME_RUN_STREAMS,
	GAME_CLOSE_STREAM,
	/* buffers */
	GAME_OPEN_STREAM_BUFFERS,
	GAME_CLOSE_STREAM_BUFFERS,
	/* save/load state */
	GAME_GET_DEVICE_STATE,
	GAME_SET_DEVICE_STATE,
	/* extensions (interfaces) */
	GAME_GET_INTERFACE_INFO,
	GAME_GET_INTERFACES,

	GAME_FIRST_UNIMPLEMENTED_CODE
};

/*
 *	DACs, ADCs, MIXERs, CONTROLs and BUFFERs all live in the same ID name
 *	space within the device.
 */
#define GAME_IS_DAC(x) (((x)&0xff0000) == 0x440000)
#define GAME_DAC_ORDINAL(x) ((x)&0xffff)
#define GAME_MAKE_DAC_ID(o) (((o)&0xffff) | 0x440000)
#define GAME_MAX_DAC_COUNT 65536

#define GAME_IS_ADC(x) (((x)&0xff0000) == 0x480000)
#define GAME_ADC_ORDINAL(x) ((x)&0xffff)
#define GAME_MAKE_ADC_ID(o) (((o)&0xffff) | 0x480000)
#define GAME_MAX_ADC_COUNT 65536

#define GAME_IS_MIXER(x) (((x)&0xff0000) == 0x4c0000)
#define GAME_MIXER_ORDINAL(x) ((x)&0xffff)
#define GAME_MAKE_MIXER_ID(o) (((o)&0xffff) | 0x4c0000)
#define GAME_MAX_MIXER_COUNT 65536

#define GAME_IS_CONTROL(x) (((x)&0xff0000) == 0x000000)
#define GAME_CONTROL_ORDINAL(x) ((x)&0xffff)
#define GAME_MAKE_CONTROL_ID(o) (o)
#define GAME_MAX_CONTROL_COUNT 65536

#define GAME_IS_STREAM(x) (((x)&0xff0000) == 0x200000)
#define GAME_STREAM_ORDINAL(x) ((x)&0xffff)
#define GAME_MAKE_STREAM_ID(o) ((o) | 0x200000)
#define GAME_MAX_STREAM_COUNT 65536

#define GAME_IS_BUFFER(x) (((x)&0xff0000) == 0x600000)
#define GAME_BUFFER_ORDINAL(x) ((x)&0xffff)
#define GAME_MAKE_BUFFER_ID(o) ((o) | 0x600000)
#define GAME_MAX_BUFFER_COUNT 65536

#define GAME_NO_ID ((int32)-1)

//#pragma mark game_get_info_v1   (struct)
struct game_get_info_v1   {

	size_t	info_size;

	char	name[32];
	char	vendor[32];
	int32	ordinal;
	int32	version;

	int32	dac_count;
	int32	adc_count;
	int32	mixer_count;
	int32	last_opcode;		/*	GAME_FIRST_UNIMPLEMENTED_CODE	*/
};

//#pragma mark game_stream_volume_mapping_v1  (enum)
enum game_stream_volume_mapping_v1  {
	GAME_VOLUME_IN_DB = 0x1,
	GAME_VOLUME_MULTIPLIER = 0x2,
	GAME_VOLUME_MIN_VALUE_IS_MUTE = 0x4
};
//#pragma mark game_codec_info_stream_capabilities_v1  (enum)
enum game_codec_info_stream_capabilities_v1  {
	/* stream control capabilities */
	GAME_STREAMS_HAVE_VOLUME = 0x1,
	GAME_STREAMS_HAVE_PAN = 0x2,
	GAME_STREAMS_HAVE_FRAME_RATE = 0x4,
	GAME_STREAMS_HAVE_FORMAT = 0x8,
	GAME_STREAMS_HAVE_CHANNEL_COUNT = 0x10,
	GAME_STREAMS_ALWAYS_MONO = 0x20,

	/* performance info */
	GAME_STREAM_TIMING_CHUNKY = 0x10000,	/* transfers samples out of buffer in large batches */
	GAME_STREAM_LARGE_BUFFERS = 0x20000		/* supports buffers > 64 kB */
};
//#pragma mark game_stream_capabilities_v1 (struct)
struct game_stream_capabilities_v1  {

	uint32	capabilities;	/*	var-rate, var-volume, var-pan, ...	*/

	uint32	frame_rates;
	float	cvsr_min;
	float	cvsr_max;
	uint32	formats;
	uint32	designations;
	uint32	channel_counts;

	uint32	volume_mapping;		/*	linear-in-dB, linear-in-mult, ...	*/
	int32	min_volume;
	int32	max_volume;
	int32	normal_point;
	float	min_volume_db;
	float	max_volume_db;

	uint32	_reserved_2[5];	/*	for future use	*/
};


//#pragma mark game_codec_info_v1  (struct)
struct game_codec_info_v1  {

	int32	codec_id;
	int32	linked_codec_id;
	int32	linked_mixer_id;

	int32	max_stream_count;
	int32	cur_stream_count;

	size_t	min_chunk_frame_count;			/*	device is not more precise */
	int32	chunk_frame_count_increment;	/*	-1 for power-of-two,
												0 for fixed chunk size */
	size_t	max_chunk_frame_count;
	size_t	cur_chunk_frame_count;

	char	name[32];
	uint32	frame_rates;
	float	cvsr_min;
	float	cvsr_max;
	uint32	formats;
	uint32	designations;
	uint32	channel_counts;		/*	0x1 for mono, 0x2 for stereo,
									0x4 for three channels, ... */

	uint32	cur_frame_rate;
	float	cur_cvsr;
	uint32	cur_format;
	int32	cur_channel_count;
	int32	cur_latency;		/*	CONVERTER and FIFO latency in frames */
								/*	(typically 1-60 depending on converter kind and quality) */

	uint32	_future_[4];

	game_stream_capabilities	stream_capabilities;
};
//#pragma mark game_get_codec_infos_v1  (struct)
struct game_get_codec_infos_v1  {

	size_t	info_size;

	game_codec_info *	info;
	int32	in_request_count;
	int32	out_actual_count;
};


//#pragma mark game_codec_format_flags_v1  (enum)
enum game_codec_format_flags_v1  {
	GAME_CODEC_FAIL_IF_DESTRUCTIVE = 0x1,
	GAME_CODEC_CLOSEST_OK = 0x2,
	GAME_CODEC_SET_CHANNELS = 0x4,
	GAME_CODEC_SET_CHUNK_FRAME_COUNT = 0x8,
	GAME_CODEC_SET_FORMAT = 0x10,
	GAME_CODEC_SET_FRAME_RATE = 0x20,
	GAME_CODEC_SET_ALL = 0x3c
};
//#pragma mark game_codec_format_v1  (struct)
struct game_codec_format_v1  {

	int32	codec;
	uint32	flags;

	int32	channels;
	size_t	chunk_frame_count;
	uint32	format;
	uint32	frame_rate;
	float	cvsr;

	status_t	out_status;

	uint32	_future_[4];
};
//#pragma mark game_set_codec_formats_v1  (struct)
struct game_set_codec_formats_v1  {

	size_t	info_size;

	game_codec_format *	formats;
	int32	in_request_count;
	int32	out_actual_count;
};

//#pragma mark game_mixer_info_v1  (struct)
struct game_mixer_info_v1  {

	int32	mixer_id;
	int32	linked_codec_id;	/*	-1 for none or for multiple (in which
									case you have to check in the codecs) */

	char	name[32];
	int32	control_count;

	uint32	_future_[4];
};
//#pragma mark game_get_mixer_infos_v1  (struct)
struct game_get_mixer_infos_v1  {

	size_t	info_size;

	game_mixer_info *	info;
	int32	in_request_count;
	int32	out_actual_count;
};

//#pragma mark game_mixer_control_type_v1  (enum)
enum game_mixer_control_type_v1  {
	GAME_MIXER_CONTROL_IS_UNKNOWN = 0,
	GAME_MIXER_CONTROL_IS_LEVEL,
	GAME_MIXER_CONTROL_IS_MUX,
	GAME_MIXER_CONTROL_IS_ENABLE
};
//#pragma mark game_mixer_control_flags_v1  (enum)
enum game_mixer_control_flags_v1  {
	GAME_MIXER_CONTROL_ADVANCED = 0x1,	/*	show on "advanced settings" page	*/
	GAME_MIXER_CONTROL_AUXILIARY = 0x2,	/*	put above or below channel strips	*/
	GAME_MIXER_CONTROL_HIDDEN = 0x4		/*  don't display this control			*/
};
//#pragma mark game_mixer_control_v1  (struct)
struct game_mixer_control_v1  {

	int32	control_id;
	int32	mixer_id;

	int32	kind;			/*	unknown, level, mux, enable	*/
	uint32	flags;			/*	advanced, auxiliary, ... (layout control) */
	int32	parent_id;		/*	or NO_ID for no parent	*/

	uint32	_future_[3];
};
//#pragma mark game_get_mixer_controls_v1  (struct)
struct game_get_mixer_controls_v1  {

	size_t	info_size;

	int32	mixer_id;
	int32	from_ordinal;	/*	i e starting with the Nth control for this
								mixer */

	game_mixer_control *	control;
	int32	in_request_count;
	int32	out_actual_count;
};

//#pragma mark game_mixer_level_info_flags_v1  (enum)
enum game_mixer_level_info_flags_v1  {
	GAME_LEVEL_HAS_MUTE = 0x1,
	GAME_LEVEL_VALUE_IN_DB = 0x2,
	GAME_LEVEL_HAS_DISP_VALUES = 0x4,
	GAME_LEVEL_ZERO_MEANS_NEGATIVE_INFINITY = 0x8,
	GAME_LEVEL_IS_PAN = 0x10,
	GAME_LEVEL_IS_EQ = 0x20
};
//#pragma mark game_mixer_level_info_type_v1  (enum)
enum game_mixer_level_info_type_v1  {
	GAME_LEVEL_AC97_MASTER = 0x2,
	GAME_LEVEL_AC97_ALTERNATE = 0x4,
	GAME_LEVEL_AC97_MONO = 0x6,
	GAME_LEVEL_AC97_TREBLE = 0x0008,
	GAME_LEVEL_AC97_BASS = 0x0808,
	GAME_LEVEL_AC97_PCBEEP = 0xA,
	GAME_LEVEL_AC97_PHONE = 0xC,
	GAME_LEVEL_AC97_MIC = 0xE,
	GAME_LEVEL_AC97_LINE_IN = 0x10,
	GAME_LEVEL_AC97_CD = 0x12,
	GAME_LEVEL_AC97_VIDEO = 0x14,
	GAME_LEVEL_AC97_AUX = 0x16,
	GAME_LEVEL_AC97_PCM = 0x18,
	GAME_LEVEL_AC97_RECORD = 0x1C,
	GAME_LEVEL_AC97_RECORD_MIC = 0x1E,
	GAME_LEVEL_AC97_3D_DEPTH = 0x0022,
	GAME_LEVEL_AC97_3D_CENTER = 0x0822,
	GAME_LEVEL_AC97_CENTER_LFE = 0x36,
	GAME_LEVEL_AC97_LR_SURROUND = 0x38,
	GAME_LEVEL_FIRST_NONAC97 = 0x1000,
	GAME_LEVEL_BUILTIN_SYNTH = GAME_LEVEL_FIRST_NONAC97
};

/*
 *	The value-disp mapping works such that at min_value "physical" value,
 *	the min_value_disp value is fed to the sprintf() string for display.
 *	Intermediate values up to max_value/max_value_disp are interpolated.
 */

//#pragma mark game_get_mixer_level_info_v1  (struct)
struct game_get_mixer_level_info_v1  {

	size_t	info_size;

	int32	control_id;
	int32	mixer_id;

	char	label[32];

	uint32	flags;				/*	has mute, value in dB, etc */
	int16	value_count;		/* 2 for a stereo level, 4 for a quad, ... */
	int16	min_value;
	int16	max_value;
	int16	normal_value;
	float	min_value_disp;
	float	max_value_disp;
	char	disp_format[32];	/*	sprintf() format string taking one float */
	uint32	type;				/*	AC97 register number, etc */

	uint32	_reserved_[4];
};

//#pragma mark game_mux_item_v1  (struct)
struct game_mux_item_v1  {

	uint32	mask;
	int32	control;
	char	name[32];

	uint32	_future_[2];
};
//#pragma mark game_mixer_mux_info_flags_v1  (enum)
enum game_mixer_mux_info_flags_v1  {
	GAME_MUX_ALLOWS_MULTIPLE_VALUES = 0x1
};
//#pragma mark game_mixer_mux_info_type_v1  (enum)
enum game_mixer_mux_info_type_v1 {
	GAME_MUX_AC97_RECORD_SELECT = 0x001A,
	GAME_MUX_AC97_MIC_SELECT = 0x0820,
	GAME_MUX_AC97_MONO_SELECT = 0x0920,
	GAME_MUX_FIRST_NONAC97 = 0x1000,
	GAME_MUX_MIC_BOOST = GAME_MUX_FIRST_NONAC97
};
//#pragma mark game_get_mixer_mux_info_v1  (struct)
struct game_get_mixer_mux_info_v1  {

	size_t	info_size;

	int32	control_id;
	int32	mixer_id;

	char	label[32];

	uint32	flags;			/*	multiple allowed, etc	*/
	uint32	normal_mask;	/*	default value(s)	*/
	int32	in_request_count;
	int32	out_actual_count;
	game_mux_item	*items;
	uint32	type;			/*	AC97 register, etc */

	uint32	_reserved_[4];
};

//#pragma mark game_mixer_enable_info_flags_v1  (enum)
enum game_mixer_enable_info_flags_v1  {
	GAME_ENABLE_OFF, GAME_ENABLE_ON
};

//#pragma mark game_mixer_enable_info_type_v1  (enum)
enum game_mixer_enable_info_type_v1 {
	GAME_ENABLE_AC97_MIC_BOOST = 0x060E,
	GAME_ENABLE_AC97_TREBLE    = 0x0008,
	GAME_ENABLE_AC97_BASS      = 0x0808,
	GAME_ENABLE_AC97_GP_LPBK   = 0x0720,
	GAME_ENABLE_AC97_GP_LD     = 0x0C20,
	GAME_ENABLE_AC97_GP_3D     = 0x0D20,
	GAME_ENABLE_AC97_GP_ST     = 0x0E20,
	GAME_ENABLE_AC97_GP_POP    = 0x0F20,
	GAME_ENABLE_FIRST_NONAC97  = 0x1000,
	/*pwr mgmt status bits */
	GAME_ENABLE_AC97_PWR_ST_ADC= 0x0026,
	GAME_ENABLE_AC97_PWR_ST_DAC= 0x0126,
	GAME_ENABLE_AC97_PWR_ST_ANL= 0x0226,
	GAME_ENABLE_AC97_PWR_ST_REF= 0x0326,
	/*pwr mgmt enable bits */
	GAME_ENABLE_AC97_PWR_ADCMUX= 0x0826,
	GAME_ENABLE_AC97_PWR_DACS  = 0x0926,	
	GAME_ENABLE_AC97_PWR_AMVRON= 0x0A26,
	GAME_ENABLE_AC97_PWR_AMVROF= 0x0B26,
	GAME_ENABLE_AC97_PWR_DIGINT= 0x0C26,
	GAME_ENABLE_AC97_PWR_INTCLK= 0x0D26,
	GAME_ENABLE_AC97_PWR_HPAMP = 0x0E26,
	GAME_ENABLE_AC97_PWR_EAPD  = 0x0F26,
	
};
//#pragma mark game_get_mixer_enable_info_v1  (struct)
struct game_get_mixer_enable_info_v1  {

	size_t	info_size;

	int32	control_id;
	int32	mixer_id;

	char	label[32];

	char	normal;		/*	on or off	*/
	uint32	type;		/*	AC97 register #, or some other type	*/

	char	enabled_label[24];
	char	disabled_label[24];

	uint32	_reserved_2[4];
};

//#pragma mark game_mixer_level_value_flags_v1  (enum)
enum game_mixer_level_value_flags_v1  {
	GAME_LEVEL_IS_MUTED = 0x1
};
//#pragma mark game_mixer_level_value_v1  (struct)
struct game_mixer_level_value_v1  {
	int16	values[6];
	uint16	value_count;
	uint16	flags;
};
//#pragma mark game_mixer_mux_value_v1  (struct)
struct game_mixer_mux_value_v1  {
	uint32	mask;
	uint32	_reserved_[3];
};
//#pragma mark game_mixer_enable_value_v1  (struct)
struct game_mixer_enable_value_v1  {
	char	enabled;
	char	_reserved_1[3];
	uint32	_reserved_2[3];
};
//#pragma mark game_mixer_control_value_v1  (struct)
struct game_mixer_control_value_v1  {

	int32	control_id;		/* input */
	int32	mixer_id;		/* output */
	int32	kind;			/* output */
	uint32	_reserved_[3];

	union {
		game_mixer_level_value	level;
		game_mixer_mux_value	mux;
		game_mixer_enable_value	enable;
	}
#if !defined(__cplusplus)
	u
#endif
	;

};
//#pragma mark game_get_mixer_control_values_v1  (struct)
struct game_get_mixer_control_values_v1  {

	size_t	info_size;

	int32	mixer_id;		/* the mixer in which the controls live	*/

	int32	in_request_count;
	int32	out_actual_count;
	game_mixer_control_value *	values;
};

//#pragma mark game_set_mixer_control_values_v1  (struct)
struct game_set_mixer_control_values_v1  {

	size_t	info_size;

	int32	mixer_id;		/* the mixer in which the controls live	*/

	int32	in_request_count;
	int32	out_actual_count;
	game_mixer_control_value *	values;
};

//#pragma mark game_open_stream_request_v1  (enum)
enum game_open_stream_request_v1  {
	GAME_STREAM_VOLUME = 0x1,
	GAME_STREAM_PAN = 0x2,
	GAME_STREAM_FR = 0x4,
	GAME_STREAM_FORMAT = 0x8,
	GAME_STREAM_CHANNEL_COUNT = 0x10
};
//#pragma mark game_open_stream_v1  (struct)
struct game_open_stream_v1  {

	size_t	info_size;

	int32	codec_id;
	uint32	request;	/*	var-rate, var-volume, var-pan, ...	*/

	sem_id	stream_sem;	/*	< 0 if none	*/

	uint32	frame_rate;
	float	cvsr_rate;
	uint32	format;
	uint32	designations;
	int32	channel_count;

	int32	out_stream_id;
};

//#pragma mark game_stream_timing_v1  (struct)
struct game_stream_timing_v1  {
	float		cur_frame_rate;
	uint32		frames_played;
	bigtime_t	at_time;
};
//#pragma mark game_stream_state_v1  (enum)
enum game_stream_state_v1  {
	GAME_STREAM_STOPPED,
	GAME_STREAM_RUNNING
};
enum game_timing_flags_v1 {
	GAME_ACQUIRE_STREAM_SEM = 0x1
};
//#pragma mark game_get_timing_v1  (struct)
struct game_get_timing_v1  {

	size_t				info_size;

	uint32				flags;		/*	GAME_ACQUIRE_STREAM_SEM */
	bigtime_t			timeout_at;

	int32				source;		/*	stream ID, DAC ID or ADC ID	*/
	int32				state;		/*	stopped, running	*/

	game_stream_timing	timing;

};


/*
 *	Stream controls are different from mixer controls in that they are per
 *	individual stream (not per codec). They are also limited to a four-way
 *	pan, volume, and frame rate set.
 */
 struct game_stream_controls_v1 {
	int32	stream;
	uint32	caps;		/*	change-sr, change-volume, change-lr-pan, ... */

	int32	volume;
	int32	lr_pan;		/*	left-right	*/
	int32	fb_pan;		/*	front-back	*/
	int32	_reserved_1;
	float	frame_rate;

	uint32	_future_[2];
};
//#pragma mark game_get_stream_controls_v1  (struct)
struct game_get_stream_controls_v1  {

	size_t	info_size;

	int32	in_request_count;
	int32	out_actual_count;
	game_stream_controls * controls;
};

//#pragma mark game_set_stream_controls_v1  (struct)
struct game_set_stream_controls_v1  {

	size_t	info_size;

	int32	in_request_count;
	int32	out_actual_count;
	game_stream_controls * controls;
	bigtime_t	ramp_time;
};

//#pragma mark game_set_stream_data_flags_v1  (enum)
enum game_set_stream_data_flags_v1  {
										/*  bits 0,1, and 2 reserved for future use	*/
	GAME_BUFFER_PING_PONG = 0x8			/*	This flag means that the stream sem will	*/
										/*	be released after every page_frame_count frames played	*/
};
//#pragma mark game_set_stream_buffer_v1  (struct)
struct game_set_stream_buffer_v1  {

	size_t	info_size;

	int32	stream;
	uint32	flags;
	int32	buffer;
	uint32	frame_count;
	uint32	page_frame_count;	/* only used for PING_PONG */

	size_t	_reserved_[2];

};

/*
 *	When stopping and re-starting, the stream is re-set (frames played starts
 *	at 0).
 */
//#pragma mark game_run_stream_v1  (struct)
struct game_run_stream_v1  {

	int32	stream;
	int32	state;		/*	stopped, running	*/

	game_stream_timing	out_timing;		/*	timing info at which it took
										effect/will take effect (estimate) */
	uint32	_reserved_;
	uint32	_future_[2];
};
//#pragma mark game_run_streams_v1  (struct)
struct game_run_streams_v1  {

	size_t	info_size;

	int32	in_stream_count;
	int32	out_actual_count;
	game_run_stream *	streams;
};

//#pragma mark game_close_stream_flags_v1  (enum)
enum game_close_stream_flags_v1  {
	GAME_CLOSE_DELETE_SEM_WHEN_DONE = 0x2
};
//#pragma mark game_close_stream_v1  (struct)
struct game_close_stream_v1  {

	size_t		info_size;

	int32		stream;
	uint32		flags;			/*	delete-stream-sem?	*/
};

//#pragma mark game_open_stream_buffer_v1  (struct)
struct game_open_stream_buffer_v1  {

	int32		stream;
	size_t		byte_size;
	status_t	out_status;

	area_id		area;			/*	user has to clone this area	*/
	size_t		offset;			/*	and find the buffer this far into it	*/
	int32		buffer;

	uint32		_reserved_2[2];
};

//#pragma mark game_open_stream_buffers_v1
struct game_open_stream_buffers_v1 {

	size_t		info_size;

	int32		in_request_count;
	int32		out_actual_count;

	game_open_stream_buffer *
				buffers;
};

//#pragma mark game_close_stream_buffer_v1  (struct)
struct game_close_stream_buffer_v1  {

	int32		buffer;
	int32		_reserved_;

	uint32		_reserved_2;
};

//#pragma mark game_close_stream_buffers_v1
struct game_close_stream_buffers_v1 {

	size_t		info_size;

	int32		in_request_count;
	int32		out_actual_count;

	game_close_stream_buffer *
				buffers;
};


/*
 *	GET_DEVICE_STATE and SET_DEVICE_STATE are for easy saving and restoring
 *	of the device state (mostly mixer levels) when starting/stopping
 *	the media server (or snd_server, as the case may be).
 *	The device should not change any mixer items when closed/unloaded; this is
 *	so that CD-through, telephone-through etc will still work if set.
 */
//#pragma mark game_get_device_state_v1  (struct)
struct game_get_device_state_v1  {

	size_t	info_size;

	size_t	in_avail_size;
	int32	out_version;
	void *	data;
	size_t	out_actual_size;
};

//#pragma mark game_set_device_state_v1  (struct)
struct game_set_device_state_v1  {

	size_t	info_size;

	size_t	in_avail_size;
	int32	in_version;
	void *	data;
};

/*
 *	For ioctl() interfaces not defined by Be (or even some extra ones defined
 *	later) you can ask whether a certain card supports it (and get information
 *	specific to that interface) by fourcc. You also get a name back, useful
 *	for informational purposes only (as the name may vary by implementation,
 *	but the fourcc is the same for one interface).
 *	"interface" in this context is just a defined set of ioctl() codes and
 *	the data/actions that go with those codes. User-defined codes start at
 *	100000 (or higher) and go up to 0x7fffffff.
 */
//#pragma mark game_get_interface_info_v1  (struct)
struct game_get_interface_info_v1  {

	size_t	info_size;

	uint32	fourcc;
	uchar	_reserved_[16];
	size_t	request_size;
	size_t	actual_size;
	void *	data;
	char	out_name[32];
	int32	first_opcode;
	int32	last_opcode;
};

/*
 *	List all interfaces this device implements (up to request_count items).
 */
//#pragma mark game_get_interfaces_v1  (struct)
struct game_get_interfaces_v1  {

	size_t	info_size;

	int32	request_count;
	int32	actual_count;
	uint32 *	interfaces;	/* fourcc codes */
};

#if defined(__cplusplus)
}	/*	end extern "C"	*/
#endif

#endif	/*	game_audio_h	*/

