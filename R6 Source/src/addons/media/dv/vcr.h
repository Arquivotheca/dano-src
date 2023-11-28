/* vcr.h
 *
 * AV/C VCR subunit definitions.
 *
 */

#ifndef VCR_H
#define VCR_H

#ifdef __cplusplus
extern "C" {
#endif

#define VCR_ANALOG_AUDIO_OUTPUT_MODE	0x70
#define VCR_AREA_MODE					0x72
#define VCR_ABSOLUTE_TRACK_NUMBER		0x52
#define VCR_AUDIO_MODE					0x71
#define VCR_BACKWARD					0x56
#define VCR_BINARY_GROUP				0x5a
#define VCR_EDIT_MODE					0x40
#define VCR_FORWARD						0x55

#define VCR_INPUT_SIGNAL_MODE			0x79

int32 vcr_get_input_signal_mode(
		avc_subunit *subunit,
		uchar *signal_mode,
		bigtime_t timeout);

#define VCR_LOAD_MEDIUM					0xc1
#define VCR_MARKER						0xca
#define VCR_MEDIUM_INFO					0xda
#define VCR_OPEN_MIC					0x60
#define VCR_OUTPUT_SIGNAL_MODE			0x78

int32 vcr_get_output_signal_mode(
		avc_subunit *subunit,
		uchar *signal_mode,
		bigtime_t timeout);

#define VCR_RECORD						0xc2
/* operand[0] = one of the following */
#define VCR_RECORD_AREA23_INSERT		0x31
#define VCR_RECORD_AREA1_INSERT			0x32
#define VCR_RECORD_AREA123_INSERT		0x33
#define VCR_RECORD_AREA3_INSERT			0x34
#define VCR_RECORD_AREA2_INSERT			0x36
#define VCR_RECORD_AREA12_INSERT		0x37
#define VCR_RECORD_AREA13_INSERT		0x38
#define VCR_RECORD_AREA23_INSERT_PAUSE	0x41
#define VCR_RECORD_AREA1_INSERT_PAUSE	0x42
#define VCR_RECORD_AREA123_INSERT_PAUSE	0x43
#define VCR_RECORD_AREA3_INSERT_PAUSE	0x44
#define VCR_RECORD_AREA2_INSERT_PAUSE	0x46
#define VCR_RECORD_AREA12_INSERT_PAUSE	0x47
#define VCR_RECORD_AREA13_INSERT_PAUSE	0x48
#define VCR_RECORD_RECORD				0x75
#define VCR_RECORD_PAUSE				0x7d

int32 vcr_record(
		avc_subunit *subunit,
		uchar record_mode,
		bigtime_t timeout);

#define VCR_PLAY						0xc3
/* operand[0] = one of the following */
#define VCR_PLAY_NEXT_FRAME				0x30
#define VCR_PLAY_SLOWEST_FORWARD		0x31
#define VCR_PLAY_SLOW_FORWARD_6			0x32
#define VCR_PLAY_SLOW_FORWARD_5			0x33
#define VCR_PLAY_SLOW_FORWARD_4			0x34
#define VCR_PLAY_SLOW_FORWARD_3			0x35
#define VCR_PLAY_SLOW_FORWARD_2			0x36
#define VCR_PLAY_SLOW_FORWARD_1			0x37
#define VCR_PLAY_X1						0x38
#define VCR_PLAY_FAST_FORWARD_1			0x39
#define VCR_PLAY_FAST_FORWARD_2			0x3a
#define VCR_PLAY_FAST_FORWARD_3			0x3b
#define VCR_PLAY_FAST_FORWARD_4			0x3c
#define VCR_PLAY_FAST_FORWARD_5			0x3d
#define VCR_PLAY_FAST_FORWARD_6			0x3e
#define VCR_PLAY_FASTEST_FORWARD		0x3f
#define VCR_PLAY_PREVIOUS_FRAME			0x40
#define VCR_PLAY_SLOWEST_REVERSE		0x41
#define VCR_PLAY_SLOW_REVERSE_6			0x42
#define VCR_PLAY_SLOW_REVERSE_5			0x43
#define VCR_PLAY_SLOW_REVERSE_4			0x44
#define VCR_PLAY_SLOW_REVERSE_3			0x45
#define VCR_PLAY_SLOW_REVERSE_2			0x46
#define VCR_PLAY_SLOW_REVERSE_1			0x47
#define VCR_PLAY_X1_REVERSE				0x48
#define VCR_PLAY_FAST_REVERSE_1			0x49
#define VCR_PLAY_FAST_REVERSE_2			0x4a
#define VCR_PLAY_FAST_REVERSE_3			0x4b
#define VCR_PLAY_FAST_REVERSE_4			0x4c
#define VCR_PLAY_FAST_REVERSE_5			0x4d
#define VCR_PLAY_FAST_REVERSE_6			0x4e
#define VCR_PLAY_FASTEST_REVERSE		0x4f
#define VCR_PLAY_REVERSE				0x65
#define VCR_PLAY_REVERSE_PAUSE			0x6d
#define VCR_PLAY_FORWARD				0x75
#define VCR_PLAY_FORWARD_PAUSE			0x7d

int32 vcr_play(
		avc_subunit *subunit,
		uchar play_mode,
		bigtime_t timeout);

#define VCR_PRESET						0x45
#define VCR_READ_MIC					0x61
#define VCR_RECORD						0xc2
#define VCR_RECORDING_DATE				0x53
#define VCR_RECORDING_SPEED				0xdb
#define VCR_RECORDING_TIME				0x54
#define VCR_RELATIVE_TIME_COUNTER		0x57
#define VCR_SEARCH_MODE					0x50
#define VCR_SMPTE_EBU_RECORDING_TIME	0x5c
#define VCR_SMPTE_EBU_TIME_CODE			0x59
#define VCR_TAPE_PLAYBACK_FORMAT		0xd3
#define VCR_TAPE_RECORDING_FORMAT		0xd2

#define VCR_TIME_CODE					0x51
typedef struct {
	uchar	hours;
	uchar	minutes;
	uchar	seconds;
	uchar	frames;
} vcr_time_code;

int32 vcr_get_time_code(
		avc_subunit *su,
		vcr_time_code *time,
		bigtime_t timeout);

#define VCR_TRANSPORT_STATE				0xd0

int32 vcr_transport_state(
		avc_subunit *su,
		uchar *mode,
		uchar *state,
		bigtime_t timeout);

#define VCR_WIND						0xc4
/* operand[0] = one of the following */
#define VCR_WIND_HIGH_SPEED_REWIND		0x45
#define VCR_WIND_STOP					0x60
#define VCR_WIND_REWIND					0x65
#define VCR_WIND_FAST_FORWARD			0x75

int32 vcr_wind(
		avc_subunit *subunit,
		uchar wind_mode,
		bigtime_t timeout);

#define VCR_WRITE_MIC					0x62

#ifdef __cplusplus
}
#endif

#endif /* VCR_H */
