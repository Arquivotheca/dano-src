
#if !defined(_miniplay_h)
#define _miniplay_h

#include <OS.h>
#include <SupportDefs.h>
#include <MediaDefs.h> // for media_multi_audio_format

#if defined(__cplusplus)
extern "C" {
#endif

/*** WARNING: This API is not quite future-proof, so don't publish it ***/
/*** without fixing some things, like how formats are specified ***/
/*** (96/24 kHz/bits is imminent in consumer audio, for instance) ***/
/*** The API is currently deliberately limited to get something small ***/
/*** and fast going for IAD. ***/

int32 mini_new_output_stream(media_multi_audio_format * io_format, uint16 buffer_count = 2);
int32 mini_close_output_stream(int32 stream);
int32 mini_write_output_stream(int32 stream, const void * data, int16 frame_count);

/*** writing directly to a stream:
     mini_acquire_buffer() waits until an output buffer is free (or the optional
       relative timeout has passed.)
     mini_queue_buffer() queues an acquired, COMPLETELY FILLED buffer for playback.
***/

int32 mini_acquire_output_buffer(
	int32 stream_id,
	int32 * o_buffer_id,
	size_t * o_buffer_size,
	void ** o_buffer_data,
	bigtime_t timeout = B_INFINITE_TIMEOUT);

int32 mini_queue_output_buffer(
	int32 buffer_id);

/*** recording
***/	

int32 mini_new_input_stream(media_multi_audio_format * out_format);
int32 mini_close_input_stream(int32 stream);
int32 mini_read_input_stream(int32 stream, const void * data, int16 buffer_count);

/*** direct recording API:
     blocks until the next input buffer is full.  the returned buffer is volatile,
     so handle it fast.
***/

int32 mini_acquire_input_buffer(int32 stream_id, size_t * o_size, void ** o_data);

/*** parameters
***/	

int32 mini_set_volume(int32 stream, float volumeL, float volumeR);
int32 mini_adjust_volume(int32 stream, float volumeL, float volumeR, uint32 flags);
int32 mini_adjust_multi_volume(int32 stream, float * volumeN, uint32 channelCount, uint32 flags);
int32 mini_get_volume(int32 stream, float * volumeL, float * volumeR, bool * outMute);
int32 mini_get_multi_volume(int32 stream, float * outVolume, uint32 channelCount, bool * outMute);
int32 mini_set_adc_source(int32 stream);
int32 mini_get_adc_source(int32 * outStream);
/* special streams for setting/getting the volume to/from */
/* the following are valid arguments to mini_set_adc_source(): */
/* miniMainOut, miniCDIn, miniLineIn, miniMicIn */
enum mini_stream_special {
	miniMainOut = 0,
	miniMonoOut = -1,		/* not typically implemented in drivers */
	miniCDIn = -2,
	miniPhoneIn = -3,
	miniLineIn = -4,
	miniMicIn = -5,
	miniEvent = -6,
	miniCapture = -7
};

/* these flags work for channels -5 through 0 */
#define VOL_SET_VOL 0x1
#define VOL_SET_MUTE 0x2
#define VOL_CLEAR_MUTE 0x4
/* these flags only work for channel 0 (main out) */
#define VOL_LOUDER 0x8
#define VOL_SOFTER 0x10
#define VOL_TOGGLE_MUTE 0x20
#define VOL_MUTE_IF_ZERO 0x40


#if defined(__cplusplus)
}
#endif

#endif	//	_miniplay_h
