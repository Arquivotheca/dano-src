
#if !defined(_miniproto_h)
#define _miniproto_h

#include <OS.h>
#include "miniplay.h"


//--- COMMS PROTOCOL ---

#define PORT_NAME "be:snd_server"

enum {
	MAKE_MIX_CHANNEL = -1,
	REMOVE_MIX_CHANNEL = -2,
	GET_STATUS = -3,
	SET_VOLUME = -4,
	GET_VOLUME = -5,
	SUSPEND_OPERATION = -6,
	RESUME_OPERATION = -7,
	TOGGLE_OPERATION = -8,
	MAKE_CAPTURE_CHANNEL = -9,
	REMOVE_CAPTURE_CHANNEL = -10,
	SET_ADC_SOURCE = -11,
	GET_ADC_SOURCE = -12
};

#define MAX_MIXCHANNEL_BUFFERS 16
#define MAX_CAPTURECHANNEL_BUFFERS 16

struct minisnd_buffer {
	int32			id;
	area_id			area;
	size_t			offset;
	size_t			size;
};

struct new_mixchannel_info {
	status_t		o_error;
	int32			o_id;
	sem_id			i_sem;
	media_multi_audio_format	io_format;
	char			i_name[64];
	uint16			io_buffer_count;
	minisnd_buffer	o_buffers[MAX_MIXCHANNEL_BUFFERS];
};

struct new_capturechannel_info {
	status_t		o_error;
	int32			o_id;
	sem_id			i_sem;
	media_multi_audio_format	o_format;
	char			i_name[64];
	uint16			io_buffer_count;
	uint16			o_first_buffer_index;
	minisnd_buffer	o_buffers[MAX_CAPTURECHANNEL_BUFFERS];
};

struct set_volume_info {
	int32			i_id;
	float			i_vols[6];
	uint32			i_flags;
};

struct get_volume_info {
	int32			i_id;
	port_id			i_port;
};

struct get_vols_reply {
	float			vols[6];
	bool			mute;
};

#define SND_TIMEOUT 2000000LL

// --- MiniApp protocol ---

enum {
	maStartEvent = 'MASV',  //	be:event string
	maStartFile = 'MASF',	  //	be:file ref		return be:handle sem_id
	//	be:speaker:volume (float, 0.0-1.0)
	//	be:speaker:mute (string, "true" or "false")
	//	be:modem:volume (float, 0.0-1.0)
	//	be:modem:mute (string, "true" or "false")
	//	be:CD:volume (float, 0.0-1.0)
	//	be:CD:mute (string, "true" or "false")
	//	be:event:volume (flaot, 0.0-1.0)
	//	be:event:mute (string, "true" or "false")
	maSetVolume = 'MAVS',
	maGetVolume = 'MAVG',
	maResponse = 'MARE',
	_maLast = 0
};

#endif	//	_proto_h
