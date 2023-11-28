/* gaplug.h */
/* Copyright © 1998-2001 Be Incorporated. All rights reserved. */

#if !defined(gaplug_h)
#define gaplug_h

#include <module.h>
#include <PCI.h>
#include <ISA.h>
#include <isapnp.h>
#include <game_audio.h>

#define GAPLUG_API_VERSION_1 0x100

#if !defined(GAPLUG_API_VERSION)
#define GAPLUG_API_VERSION GAPLUG_API_VERSION_1
#endif

#if GAPLUG_API_VERSION == GAPLUG_API_VERSION_1

#define GAPLUG_PCI_MODULE_NAME_PREFIX "media/gamedriver/pci/"
#define GAPLUG_PCI_MODULE_NAME_SUFFIX "/v1"
#define GAPLUG_ISA_MODULE_NAME_PREFIX "media/gamedriver/isa/"
#define GAPLUG_ISA_MODULE_NAME_SUFFIX "/v1"

//	plug_	-- 	exported/filled out by plug-in
//	ga_		--	exported by driver (called/used by plug)
//	gaplug_	--	passed from driver to plug
typedef struct plug_api_v1						plug_api;
typedef struct plug_mpu401_info_v1				plug_mpu401_info;
typedef struct plug_gameport_info_v1			plug_gameport_info;
typedef struct gaplug_open_stream_buffer_v1		gaplug_open_stream_buffer;
typedef struct gaplug_set_buffer_v1				gaplug_set_buffer;
typedef struct gaplug_run_stream_info_v1		gaplug_run_stream_info;
typedef struct gaplug_run_streams_v1			gaplug_run_streams;
typedef struct gaplug_looping_buffer_info_v1	gaplug_looping_buffer_info;
typedef struct ga_stream_data_v1				ga_stream_data;
typedef struct ga_utility_funcs_v1				ga_utility_funcs;
typedef union  gaplug_control_info_v1			gaplug_control_info;
typedef struct gaplug_get_mixer_controls_v1		gaplug_get_mixer_controls;
typedef struct gaplug_mixer_control_value_v1	gaplug_mixer_control_value;
typedef struct gaplug_mixer_control_values_v1	gaplug_mixer_control_values;

#endif	//	GAPLUG_API_VERSION == GAPLUG_API_VERSION_1


//	The game audio driver will load your module of type plug_module_info and ask
//	it to accept a card; you should return a plug_api structure with
//	the hooks and information you need. You can use "cookie" whichever way you
//	want; recommended is to declare a struct which ENDS in a plug_api
//	and have "cookie" point to the beginning of your bigger struct. Then return
//	the address of the plug_api field of that struct.
//	The game audio driver will serialize access to your plug-in module, so you
//	only have to worry about interrupt serialization (which typically will need
//	a spinlock).
//	The game audio driver will call the "optional" hooks only if they are non-NULL,
//	thus you can save some cycles by not defining them unless you need them.
//	The required hooks must be non-NULL; the debugging version of the game audio
//	driver will print an error message and reject your module if they aren't.

//#pragma mark -- plug_api --
struct plug_api_v1
{

	size_t				info_size;				//	for versioning

	//	info (data)

	void *				cookie;					//	for use by plug (host doesn't care or touch)
	const char *		short_name;				//	device name stem, like "gizmo" (max 19 chars)
	const char *		vendor_name;			//	set to NULL for default; max 31 chars


	//	Functionality (required)

	//	Undiscovery; the driver is done with this part of the module and is disposing it.
	//	Because you probably malloc()-ed or calloc()-ed your struct in accept_pci(), you
	//	should free() it here (if you follow the recommended practice, that's free(i_info->cookie);)
	//	The game audio driver will already have disposed all streams and buffers, but you
	//	can put in an assert or debugging check to make sure it's so if you're paranoid.
	//	Here's a good place to remove your interrupt handler.
	//	LOCKING: init
	void				(*uninit)(struct plug_api_v1 * i_info);

	//	Set o_adcs to your # of ADCs, and o_dacs to your # of DACs.
	//	In version 1 of the API, this function will get called right after you are
	//	first opened, each time you are first opened (if there are multiple open/close sessions)
	//	LOCKING: init
	void				(*init_codecs)(struct plug_api_v1 * i_info, int * o_adcs, int * o_dacs, int * o_mixers);

	//	The codec infos are already filled out with default values, but you should modify
	//	them to reflect your actual capabilities (since the defaults are a bare minimum).
	//	In version 1 of the API, this function will only get called right after init_codecs,
	//	not at runtime after that.
	//	LOCKING: init
	void				(*get_codec_infos)(struct plug_api_v1 * i_info, game_codec_info * o_adcs, game_codec_info * o_dacs);

	//	A stream is about to be opened -- you can return some error to prevent it.
	//	Stream count and ability flags have already been checked in the separate codec info.
	//	Assign *o_stream_cookie to a cookie that will be passed back to you in operations regarding this stream
	//	LOCKING: general
	status_t			(*init_stream)(struct plug_api_v1 * i_info, game_open_stream * io_request, const struct ga_stream_data_v1 * i_data, void ** o_stream_cookie);

	//	A number of streams are requested to be run, paused or stopped.
	//	The request has been pre-processed by setting all unknown streams to GAME_NO_ID
	//	(and setting the streams entry for that stream to 0)
	//	Returning an error will not mark any of the streams as changed (internally) and will return
	//	the error to the caller. If one stream has errors, set that stream's ID to GAME_NO_ID in
	//	the game_open_stream request list.
	//	Warning: for streams being stopped, you should NOT call any feed function or
	//	access any buffer data returned to you from feed after returning from this call!
	//	LOCKING: general
	status_t			(*run_streams)(struct plug_api_v1 * i_info, const struct gaplug_run_streams_v1 * i_request);



	//	Mixer controls (semi-optional)

	//	If you return a mixer count greater than 0, you should also implement the mixer functions.
	//	In version 1 of the API, these hooks will only be called once when you are loaded, and left
	//	alone after that.
	//	LOCKING: mixer
	void				(*get_mixer_description)(struct plug_api_v1 * i_info, int index, game_mixer_info * o_mixers);
	void				(*get_mixer_controls)(struct plug_api_v1 * i_info, const struct gaplug_get_mixer_controls_v1 * io_controls);

	//	Control requests and changes go through here, bad IDs are already weeded out for you.
	//	LOCKING: mixer
	void				(*get_mixer_control_values)(struct plug_api_v1 * i_info, const struct gaplug_mixer_control_values_v1 * io_request);
	void				(*set_mixer_control_values)(struct plug_api_v1 * i_info, const struct gaplug_mixer_control_values_v1 * io_request);


	//	Driver hooks (optional). These can all be NULL if you don't need to augment the
	//	host driver's behaviour.

	//	These are given first stab at driver requests. If open returns error, the driver
	//	returns error to the user.

	//	LOCKING: init
	status_t			(*open)(struct plug_api_v1 * i_info, const char * path, uint32 flags, const void * opaque);
	void				(*close)(struct plug_api_v1 * i_info, const void * opaque);
	void				(*free)(struct plug_api_v1 * i_info, const void * opaque);

	//	If you want the driver to handle a request, return B_DEV_INVALID_IOCTL. All other
	//	return values mean the call is immediately short-cirtuited back to the user.
	//	LOCKING: general
	status_t			(*control)(struct plug_api_v1 * i_info, uint32 i_code, void * io_data, const void * opaque);




	//	API hooks (optional) These can all be NULL if the host driver's default behaviour
	//	is good enough. Only make them non-NULL if you need to (for efficiency reasons).
	//	o_info has already been filled in fully with data you returned before, but you can modify it.
	//	LOCKING: general
	status_t			(*get_info)(struct plug_api_v1 * i_info, game_get_info * o_info);

	//	io_format has already been sanity checked, and should be acceptable to you according to your
	//	previously returned info. If you fail, return an error. You should always set io_format to
	//	the actual format you are now using before you return. (If the change is a no-op, this hook
	//	will not get called in the first place).
	//	note: "cvsr" will be set the the requested sample rate (if requested) even if it's not B_SR_CVSR.
	//	LOCKING: general
	status_t			(*set_codec_format)(struct plug_api_v1 * i_info, game_codec_format * io_format);

	//	io_buffer contains some information about the allocation (if known) and
	//	the size/lock/protection that gamedriver will use to call create_area() for this
	//	individual buffer. You can either do the buffer management yourself and return
	//	an area ID in io_request->area (in which case gamedriver will immediately return),
	//	return an error (which will de-rail the allocation), or return OK (in which case
	//	the gamedriver will use create_area() to create the buffer).
	//	Note: the gamedriver will assign an ID to the buffer after you return!
	//	LOCKING: general
	status_t			(*open_stream_buffer)(struct plug_api_v1 * i_info, struct gaplug_open_stream_buffer_v1 * io_buffer);

	//	bind_stream_buffer will tell you about the actual buffer ID for an allocated buffer,
	//	post-allocation
	//	LOCKING: general
	void				(*bind_stream_buffer)(struct plug_api_v1 * i_info, const struct gaplug_open_stream_buffer_v1 * i_buffer, int32 id);

	//	return error from close_stream_buffer to not have the gamedriver delete_area the area
	//	(the error will not be propagated, and the buffer will be considered "gone" anyway)
	//	LOCKING: general
	status_t			(*close_stream_buffer)(struct plug_api_v1 * i_info, game_open_stream_buffer * io_buffer);

	//	this is your chance of disposing the stream cookie you might have allocated in init_stream()
	//	LOCKING: general
	void				(*remove_stream)(struct plug_api_v1 * i_info, game_open_stream * i_stream, void * i_stream_cookie);

	//	A buffer is being set. Make arrangements to start pulling data out of this buffer when
	//	the stream is later started (the stream is not running at this point).
	//	LOCKING: general
	status_t			(*set_stream_buffer)(struct plug_api_v1 * i_info, const struct gaplug_set_buffer_v1 * i_buf);

	//	The control change requests have already been sanitized, and any bad stream ID or control
	//	values have been tagged with GAME_NO_ID for the stream in the request, and the corresponding
	//	stream cookies and infos are set to NULL.
	//	For all good streams, apply the stream control values as requested.
	//	LOCKING: general
	status_t			(*set_stream_controls)(struct plug_api_v1 * i_info, game_set_stream_controls * io_request, void ** stream_cookies, game_open_stream ** stream_infos);

	//	For this stream, call update_timing() with fresh data (if available) and return from the hook when done.
	//	LOCKING: general
	status_t			(*get_stream_timing)(struct plug_api_v1 * i_info,  void * i_stream_cookie);

	//	For devices with an MPU-401 interface fill out the plug_mpu401_info structure.
	//	LOCKING: general
	status_t			(*get_mpu401_info)(struct plug_api_v1 * i_info, plug_mpu401_info * o_mpu401_info);

	//	The interrupt_hook should be called with the supplied cookie when an MPU-401
	//	interrupt occurs.  The hook returns true if the interrupt was handled.
	//	LOCKING: general
	status_t			(*set_mpu401_callback)(struct plug_api_v1 * i_info, bool (*i_interrupt_hook)(void*), void* i_cookie);

	//	For devices with a joystick interface fill out the plug_gameport_info structure.
	//	LOCKING: general
	status_t			(*get_gameport_info)(struct plug_api_v1 * i_info, plug_gameport_info * o_gameport_info);

	uint32				_reserved_[5];

};

//#pragma mark -- gaplug_open_stream_buffer --
struct gaplug_open_stream_buffer_v1
{
	game_open_stream_buffer *	io_request;
	game_codec_info *			in_codec;
	game_open_stream *			in_stream;		/* if bound */
	void *						in_stream_cookie;	/* if bound */
	/* if you return OK and don't assign an area ID in io_request, the fields below will be used to call create_area() */
	size_t						io_size;		/* default rounded up to frame count * frame_size and then up to B_PAGE_SIZE */
	uint32						io_lock;		/* default B_FULL_LOCK */
	uint32						io_protection;	/* default 0 (because user will clone, and we only DMA) */
	uint32						_reserved_;
};

//#pragma mark -- gaplug_set_buffer --
struct gaplug_set_buffer_v1
{
	game_set_stream_buffer *	io_request;
	game_open_stream *			in_stream;
	void *						in_stream_cookie;
	area_id						in_area;
	void *						in_base;
	size_t						in_offset;
	size_t						in_size;
	uint32						_reserved_;
};

//#pragma mark -- gaplug_looping_buffer_info --
struct gaplug_looping_buffer_info_v1
{
	void *							address;
	size_t							frame_count;
	size_t							page_frame_count;
	int32							buffer;
	uint32							_reserved_[2];
};

//#pragma mark -- gaplug_run_stream_info --
struct gaplug_run_stream_info_v1
{
	game_run_stream *				request;
	game_open_stream *				stream;
	void *							stream_cookie;

	struct gaplug_looping_buffer_info_v1		looping;
};

//#pragma mark -- gaplug_run_streams --
struct gaplug_run_streams_v1
{
	int								total_slots;
	game_run_streams *				io_request;
	const struct gaplug_run_stream_info_v1 *	info;
};

//#pragma mark -- gaplug_control_info --
union gaplug_control_info_v1
{
	const void *						other;
	const game_get_mixer_mux_info *		mux;
	const game_get_mixer_level_info *	level;
	const game_get_mixer_enable_info *	enable;
};
//#pragma mark -- gaplug_get_mixer_controls --
struct gaplug_get_mixer_controls_v1
{
	const game_mixer_info * 			i_mixer;		/* copy of info you returned previously */
	game_mixer_control *				o_controls;		/* copy your info into here */
	union gaplug_control_info_v1 *		o_infos;		/* assign pointers to your static structs into this array */
											/* note: the mixer/control ID are not checked; the index is */
											/* used (i e o_controls[ix] must match o_infos[ix]). */
	void **								o_cookies;		/* assign one cookie per control for your own use */
};

//#pragma mark -- gaplug_mixer_control_value --
struct gaplug_mixer_control_value_v1
{
	void *								i_cookie;		/* your cookie */
	union gaplug_control_info_v1		i_info;			/* info about control */
	game_mixer_control_value *			io_value;		/* in for set, out for get */
};
//#pragma mark -- gaplug_mixer_control_values --
struct gaplug_mixer_control_values_v1
{
	int									i_count;
	const struct gaplug_mixer_control_value_v1 *
										i_values;
};


//#pragma mark -- ga_stream_data --
struct ga_stream_data_v1
{
	//	Pass this cookie back to the host whenever you call feed, ack or int_done.
	//	You should NOT touch that value.
	void *				cookie;

	//	Call this function once your interrupt is done and all spinlocks are released,
	//	if the interrupt was from your device.
	//	Return its status as your interrupt routine's return value.
	long				(*int_done)(
								void * cookie);

	//	If a plug chooses to optimize the looping buffer case, it should not use feed or
	//	ack for that stream. Instead, it should call update_timing() with timing info.
	//	Still call int_done() when you're done.
	void				(*update_timing)(
								void * cookie,
								size_t frames_played,
								bigtime_t at_time);

	uint32				_reserved_[3];
};

//	These functions are passed to the plug in accept_pci(). The plug can stash
//	the pointer to this struct and use the funcs instead of implementing its own.
//#pragma mark -- ga_utility_funcs --
struct ga_utility_funcs_v1
{
	//	Size of this struct, used for versioning.
	size_t				func_size;

	//	Given a format code (say, B_FMT_8BIT_U) find the size in bytes of the sample.
	uint32				(*find_sample_size)(uint32 formatCode);

	//	Given a rate code (say, B_SR_48000) find the actual sampling rate.
	float				(*find_frame_rate)(uint32 rateCode);

	//	Given a sampling rate (say, 48000.0), find the corresponding code (say, B_SR_48000).
	//	If there if no code for the specified rate, return B_SR_CVSR. "deviation" is an
	//	absolute measure in Hz of how accurate the match should be (pass 0.0 for exact match).
	uint32				(*find_frame_rate_code)(float rate, float deviation);

	uint32				_reserved_[2];
};


//	If the plug implements get_mpu401_info, it fills out this struct
//	the first time the midi device is opened.
//#pragma mark -- plug_mpu401_info --
struct plug_mpu401_info_v1
{
	//	MPU-401 base I/O address (typically 0x300 or 0x330)
	uint32				port;

	//	Cookie to be passed as the first arg to the functions below
	void*				cookie;

	//	Required hooks for controlling the MPU-401 interrupt
	void				(*enable_mpu401_int)(void* cookie);
	void				(*disable_mpu401_int)(void* cookie);

	//	Optional hooks to override access to the MPU-401 I/O port
	//	These can be NULL
	uchar				(*read_func)(void* cookie, int addr);
	void				(*write_func)(void* cookie, int addr, uchar data);

	uint32				_reserved_[2];
};


//	If the plug implements get_gameport_info, it fills out this struct
//	the first time the joystick device is opened.
//#pragma mark -- plug_gameport_info --
struct plug_gameport_info_v1
{
	//	Cookie to be passed as the first arg to the functions below
	void*				cookie;

	//	Required hooks to access to the joystick I/O port
	uchar				(*read_func)(void* cookie);
	void				(*write_func)(void* cookie, uchar data);

	uint32				_reserved_[2];
};


//	The game audio driver will load your module and call the appropriate accept function
//	if it finds a card which matches your module name. You are then asked to approve the
//	card and return an appropriately initialized plug_api struct. Or return NULL.
//	Here is NOT a good place to do lengthy initialization and/or DSP code downloads;
//	do that by overriding the open() hook instead. (this hook gets called almost
//	every rescan in the audio directory...)
typedef struct plug_module_info_v1 plug_module_info;
//#pragma mark -- plug_module_info --
struct plug_module_info_v1 {

	module_info				module;

	struct plug_api_v1 *	(*accept_pci)(const pci_info * info, pci_module_info * module, const struct ga_utility_funcs_v1 * funcs);
	struct plug_api_v1 *	(*accept_isa)(const struct isa_device_info * info, isa_module_info * module, const struct ga_utility_funcs_v1 * funcs, const config_manager_for_driver_module_info * pnp, uint64 cookie);

	uint32					_reserved_[5];
};

#endif	//	gaplug_h
