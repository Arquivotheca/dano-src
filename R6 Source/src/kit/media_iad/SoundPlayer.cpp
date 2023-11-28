
#include <ByteOrder.h>
#include <SoundPlayer.h>
#include <Autolock.h>
#include <MediaRoster.h>
#include <Sound.h>
#include <TimeSource.h>
#include <ParameterWeb.h>
#include <Application.h>

#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include "SoundPlayNode.h"
#include "tr_debug.h"
#include "mixer_i586.h"
#include "trinity_p.h"


#define REALLY_PLAY 1
#define TEST_R4_APPS 1

#if !NDEBUG
#define FPRINTF fprintf
#else
#define FPRINTF
#endif

#define RESAMPLE //FPRINTF
#define LOCATION //FPRINTF
#define NODE //FPRINTF
#define DATA //FPRINTF
#define CONNECT FPRINTF
#define LATENCY //FPRINTF

//	don't take this out; developers need to see this info!
#define DIAGNOSTIC fprintf


sound_error::sound_error(const char * str)
{
	m_str_const = str;
}

const char *
sound_error::what() const
{
	return m_str_const;
}


static void
legacy_check(
	media_raw_audio_format & out)
{
	if (!be_app) return;
	app_info info;
	be_app->GetAppInfo(&info);

#if 0	//	I'd like to enable this code some time...
	FILE * f = fopen("/boot/beos/etc/soundplayer.patch", "r");
	if (!f) return;
	media_raw_audio_format expected;
	char mimesig[256];
	char line[1024];

	while (!feof(f) && !ferror(f)) {
		line[0] = 0;
		fgets(line, 1023, f);
		if (!line[0]) break;
		if (line[0] == '#') continue;
		if (line[0] == '\n') continue;
		if (6 != sscanf(line, "%.255s %f %x %d %d %d", mimesig,
				&expected.frame_rate, &expected.format, &expected.channel_count,
				&expected.byte_order, &expected.buffer_size)) {
			fprintf(stderr, "bad syntax in soundplayer.patch: %s", line);
			continue;
		}
		if (!strcasecmp(mimesig, info.signature)) {
			if (expected.frame_rate > 0) out.frame_rate = expected.frame_rate;
			if (expected.format > 0) out.format = expected.format;
			if (expected.channel_count > 0) out.channel_count = expected.channel_count;
			if (expected.byte_order > 0) out.byte_order = expected.byte_order;
			//	same-order?
			if (expected.byte_order == -1) out.byte_order = B_HOST_IS_BENDIAN ? B_MEDIA_BENDIAN : B_MEDIA_LENDIAN;
			//	opposite-order?
			if (expected.byte_order == -2) out.byte_order = B_HOST_IS_BENDIAN ? B_MEDIA_LENDIAN : B_MEDIA_BENDIAN;
			if (expected.buffer_size > 0) out.buffer_size = expected.buffer_size;
			fclose(f);
			return;
		}
	}
	fclose(f);
#endif
#if TEST_R4_APPS
static const char * r4_float_apps[] = {
	"application/x-vnd.Cycore-Cult3DViewer",
//	"application/x-vnd.WCDesign.Axia",
//	"application/x-vnd.WCDesign.Abuse",
	NULL
};
	for (const char ** ptr = r4_float_apps; *ptr != 0; ptr++) {
		if (!strcasecmp(info.signature, *ptr)) {
			out.format = 0x24;
		}
	}
	if (!strcasecmp(info.signature, "application/x-vnd.WCDesign.Axia") ||
		!strcasecmp(info.signature, "application/x-vnd.Cycore-Cult3DViewer") ||
		!strcasecmp(info.signature, "application/x-vnd.WCDesign.Abuse")) {
		out.format = 0x24;
	}
#endif
#if B_HOST_IS_BENDIAN
	if (!strcasecmp(info.signature, "application/x-vnd.ClaesL-CLAmp")) {
		out.byte_order = B_MEDIA_BIG_ENDIAN;
	}
#endif
}


static void
sanitize(
	const media_multi_audio_format * in, 
	media_multi_audio_format & out)
{
	out = media_raw_audio_format::wildcard;
	if (!in || (((in->frame_rate < 5000.0) || (in->frame_rate > 1600000.0)) && 
			(in->frame_rate != media_raw_audio_format::wildcard.frame_rate))) {
		out.frame_rate = 44100.0;
		if (in != 0) DIAGNOSTIC(stderr, "SoundPlayer: bad frame_rate %g, assuming %g\n", 
				in->frame_rate, out.frame_rate);
	}
	else {
		out.frame_rate = in->frame_rate;
	}
	if (!in || ((in->format != 0x11) && (in->format != 0x2) && (in->format != 0x4) && 
			(in->format != 0x24) && (in->format != 0))) {
		out.format = 0x24;	//	the R4 behaviour was to do float, so we remain compatible
		if (in != 0) DIAGNOSTIC(stderr, "SoundPlayer: bad format 0x%lx, assuming 0x%lx\n", 
				in->format, out.format);
	}
	else {
		out.format = in->format;
	}
	if (!in || ((in->channel_count < 0) || (in->channel_count > 256))) {
		out.channel_count = 2;
		if (in != 0) DIAGNOSTIC(stderr, "SoundPlayer: bad channel_count %ld, assuming %ld\n", 
				in->channel_count, out.channel_count);
	}
	else {
		out.channel_count = in->channel_count;
	}
	if (!in || ((in->byte_order < 0) || (in->byte_order > 2))) {
		out.byte_order = B_HOST_IS_BENDIAN ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
		if (in != 0) DIAGNOSTIC(stderr, "SoundPlayer: bad byte_order %ld, assuming %ld\n", 
				in->byte_order, out.byte_order);
	}
	else {
		out.byte_order = in->byte_order;
	}
	if (!in || ((in->buffer_size < 0) || (in->buffer_size > 65536L))) {
		out.buffer_size = 0;
		if (in != 0) DIAGNOSTIC(stderr, "SoundPlayer: bad buffer_size %ld, assuming %ld\n", 
				in->buffer_size, out.buffer_size);
	}
	else {
		out.buffer_size = in->buffer_size;
	}
	if (in != 0) {
		out.channel_mask = in->channel_mask;
		out.valid_bits = in->valid_bits;
		out.matrix_mask = in->matrix_mask;
	}
FPRINTF(stderr, "pre legacy_check: %g; %x; %d; %d; %d\n", out.frame_rate, out.format,
		out.channel_count, out.byte_order, out.buffer_size);
	legacy_check(out);
FPRINTF(stderr, "post legacy_check: %g; %x; %d; %d; %d\n", out.frame_rate, out.format,
		out.channel_count, out.byte_order, out.buffer_size);
}


BSoundPlayer::BSoundPlayer(
	const char * name,
	void (*PlayBuffer)(void * cookie, void * buffer, size_t size, const media_raw_audio_format & format),
	void (*Notifier)(void * cookie, sound_player_notification what, ...),
	void * cookie) :
	_m_lock(name ? name : "BSoundPlayer")
{
	Init(NULL, NULL, name, NULL, PlayBuffer, Notifier, cookie);
}

BSoundPlayer::BSoundPlayer(
	const media_raw_audio_format * format,
	const char * name,
	void (*PlayBuffer)(void * cookie, void * buffer, size_t size, const media_raw_audio_format & format),
	void (*Notifier)(void * cookie, sound_player_notification what, ...),
	void * cookie) :
	_m_lock(name ? name : "BSoundPlayer")
{
	media_multi_audio_format fmt;
	if (format) static_cast<media_raw_audio_format &>(fmt) = *format;
	Init(NULL, &fmt, name, NULL, PlayBuffer, Notifier, cookie);
}

BSoundPlayer::BSoundPlayer(
	const media_node & toNode, 
	const media_multi_audio_format * format,
	const char * name,
	const media_input * input,
	void (*PlayBuffer)(void * cookie, void * buffer, size_t size, const media_raw_audio_format & format),
	void (*Notifier)(void * cookie, sound_player_notification what, ...),
	void * cookie) :
	_m_lock(name ? name : "BSoundPlayer")
{
	Init(&toNode, format, name, input, PlayBuffer, Notifier, cookie);
}

void BSoundPlayer::Init(
	const media_node * toNode,
	const media_multi_audio_format * format,
	const char * name,
	const media_input * input,
	void (*PlayBuffer)(void * cookie, void * buffer, size_t size, const media_raw_audio_format & format),
	void (*Notifier)(void * cookie, sound_player_notification what, ...),
	void * cookie)
{
	NODE(stderr, "BSoundPlayer::Init()\n");
	_PlayBuffer = PlayBuffer;
	_m_cookie = cookie;
	_Notifier = Notifier;
	_m_sounds = NULL;
	_m_waiting = NULL;
	_m_volume = 0.0;
	_m_gotVolume = 0;
	_m_volumeSlider = 0;
	_m_mix_buffer = NULL;
	_m_mix_buffer_size = 0;
	_m_buf = NULL;
	_m_bufsize = 0;
	_m_has_data = 0;
	_m_node = NULL;

	media_multi_audio_format sane_fmt;
	sanitize(format, sane_fmt);

	_m_init_err = B_OK;
	BMediaRoster * r = BMediaRoster::Roster(&_m_init_err);
	if (r == NULL) {
		return;
	}
	{
		media_node sound_out;
		_StReleaseNode release_sound_out(sound_out);
		if ((_m_init_err = BMediaRoster::Roster()->GetAudioOutput(&sound_out)) != B_OK) {
			DIAGNOSTIC(stderr, "BSoundPlayer: no audio output\n");
			return;
		}
		if (sound_out == media_node::null) {
			DIAGNOSTIC(stderr, "BSoundPlayer: no audio output\n");
			_m_init_err = B_MEDIA_SYSTEM_FAILURE;
			return;
		}
	}

	_m_node = new _SoundPlayNode(name ? name : "BSoundPlayer", this, &sane_fmt);
	NODE(stderr, "RegisterNode()\n");
	BMediaRoster::Roster()->RegisterNode(_m_node);
	media_node source;
//	_StReleaseNode release_source(source);
	if ((toNode != 0) && (toNode->kind & B_TIME_SOURCE)) {
		source = *toNode;
	}
	else if (((BMediaRoster::Roster()->GetTimeSource(&source) < B_OK)) || (source == media_node::null)) {
		BMediaRoster::Roster()->GetSystemTimeSource(&source);
	}
	BMediaRoster::Roster()->SetTimeSourceFor(_m_node->ID(), source.node);
	NODE(stderr, "time source is %ld\n", source.node);
	media_node mixer;
	_StReleaseNode release_mixer(mixer);
	if (toNode != 0) {
		CONNECT(stderr, "Using supplied destination Node\n");
		mixer = *toNode;
		release_mixer.Clear();
	}
	else {
		CONNECT(stderr, "Trying mixer\n");
		if (BMediaRoster::Roster()->GetAudioMixer(&mixer) < B_OK) {
bad_mojo:
			DIAGNOSTIC(stderr, "BSoundPlayer: no audio mixer\n");
			BMediaRoster::Roster()->ReleaseNode(_m_node->Node());
	//		_m_node->Release();
			_m_node = NULL;
	//		throw sound_error("There is no available sound output!");
			_m_init_err = B_MEDIA_SYSTEM_FAILURE;
			return;
		}
	}
	/* hook up */
	{
		status_t err;
		int32 total = 0;
		CONNECT(stderr, "destination: %ld %ld %lx\n", mixer.node, mixer.port, mixer.kind);
		if (input == 0) {
			err = BMediaRoster::Roster()->GetFreeInputsFor(mixer, &m_input, 1, &total, B_MEDIA_RAW_AUDIO);
			if (err < B_OK || !total ) {
				NODE(stderr, "sound output: Can't GetFreeInputsFor(): %lx (%s)\n", err, strerror(err));
				goto bad_mojo;
			}
		}
		else {
			m_input = *input;
		}
		CONNECT(stderr, "input: %ld:%ld\n", m_input.destination.port, m_input.destination.id);
		total = 0;
		err = BMediaRoster::Roster()->GetFreeOutputsFor(_m_node->Node(), &m_output, 1, &total, B_MEDIA_RAW_AUDIO);
		if (err < B_OK || !total) {
			NODE(stderr, "BSoundPlayer: Can't GetFreeOutptusFor(): %lx (%s)\n", err, strerror(err));
			goto bad_mojo;
		}
		CONNECT(stderr, "output: %ld:%ld\n", m_output.source.port, m_output.source.id);
		media_format fmt;
		fmt.type = B_MEDIA_RAW_AUDIO;
		//fmt.u.raw_audio = media_raw_audio_format::wildcard;
		if (!format) {
			fmt.u.raw_audio = media_raw_audio_format::wildcard;
		}
		else {
			fmt.u.raw_audio = sane_fmt;
		}
FPRINTF(stderr, "BEFORE: fmt.u.raw_audio.buffer_size is %ld (format %p)\n", fmt.u.raw_audio.buffer_size, format);
//		fmt.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
		err = BMediaRoster::Roster()->Connect(m_output.source, m_input.destination, &fmt, &m_output, &m_input);
FPRINTF(stderr, "AFTER: fmt.u.raw_audio.buffer_size is %ld\n", fmt.u.raw_audio.buffer_size);
		if (err < B_OK) {
			CONNECT(stderr, "sound player: Can't Connect(): %lx (%s)\n", err, strerror(err));
			char str[256];
			string_for_format(fmt, str, 256);
			NODE(stderr, "fmt is %s\n", str);
			goto bad_mojo;
		}
	}
}

BSoundPlayer::~BSoundPlayer()
{
	//	If the media roster already went away, or was never created, don't crash.
	//	This change was provoked by CL-Amp.
	if (BMediaRoster::CurrentRoster() == 0) {
		return;
	}
	assert(!_m_lock.IsLocked());
	if (_m_node != NULL) {
		status_t err1 = BMediaRoster::Roster()->StopNode(_m_node->Node(), 0, true);
		status_t err2 = BMediaRoster::Roster()->Disconnect(m_output.node.node, m_output.source, m_input.node.node, m_input.destination);
		status_t err3 = BMediaRoster::Roster()->PrerollNode(_m_node->Node());	//	sync
		NODE(stderr, "%ld: %p: %s: BSoundPlayer::~BSoundPlayer(): Node is now definitely stopped\n", find_thread(NULL),
			_m_node, _m_node->Name());
		NODE(stderr, "err1: %lx  err2: %lx  err3: %lx\n", err1, err2, err3);
//		_m_node->Release();
		BMediaRoster::CurrentRoster()->ReleaseNode(_m_node->Node());
	}
	BAutolock lock(_m_lock);
	NODE(stderr, "BSoundPlayer::~BSoundPlayer(): disposing sounds\n");
	for (_playing_sound **ptr = &_m_sounds; *ptr != NULL;) {
		_playing_sound * del = *ptr;
		ptr = &del->next;
		del->sound->ReleaseRef();
		delete_sem(del->id);
//		NotifySoundDone(del->id, true);
		delete del;
	}
	for (_waiting_sound **ptr = &_m_waiting; *ptr != NULL;) {
		_waiting_sound * del = *ptr;
		ptr = &del->next;
		del->sound->ReleaseRef();
		delete_sem(del->id);
//		NotifySoundDone(del->id, false);
		delete del;
	}
	free(_m_buf);
//	delete[] _m_mix_buffer;
	rtm_free(_m_mix_buffer);
	if (_m_volumeSlider) delete _m_volumeSlider->Web();
}

void
BSoundPlayer::get_volume_slider()
{
	if (_m_volumeSlider) return;
	BMediaRoster * r = BMediaRoster::CurrentRoster();
	if (!r) return;
	BParameterWeb * web = 0;
	media_node mixer;
	_StReleaseNode release(mixer);
	status_t err = r->GetAudioMixer(&mixer);
	if (err < B_OK) {
		DIAGNOSTIC(stderr, "BSoundPlayer: cannot get the mixer: %s\n", strerror(err));
		return;
	}
	err = r->GetParameterWebFor(mixer, &web);
	if (err < B_OK) {
		DIAGNOSTIC(stderr, "BSoundPlayer: cannot get volume slider from mixer: %s\n",
				strerror(err));
		return;
	}
	for (int ix=0; ix<web->CountParameters(); ix++) {
		BParameter * bp = web->ParameterAt(ix);
		if ((bp->Type() == BParameter::B_CONTINUOUS_PARAMETER) &&
				!strcmp(bp->Kind(), B_GAIN) &&
				((bp->ID()>>16) == m_input.destination.id)) {
			BContinuousParameter * bcp = dynamic_cast<BContinuousParameter *>(bp);
			if (bcp != 0) {
				_m_volumeSlider = bcp;
				return;
			}
		}
	}
	DIAGNOSTIC(stderr, "BSoundPlayer: did not find gain slider for ID %d in mixer\n",
			m_input.destination.id);
	delete web;
}

status_t
BSoundPlayer::Preroll()
{
	if (!_m_node) {
		return _m_init_err ? _m_init_err : B_ERROR;
	}
	return BMediaRoster::Roster()->PrerollNode(_m_node->Node());
};

status_t
BSoundPlayer::Start()
{
	if (!_m_node) {
		return _m_init_err ? _m_init_err : B_ERROR;
	}
//fixme:	RACE!!!	(TimeSource())
	if (!_m_node->TimeSource()->IsRunning()) {
		NODE(stderr, "Starting system time source.\n");
//fixme:	RACE!!!	(TimeSource())
		BMediaRoster::Roster()->StartTimeSource(_m_node->TimeSource()->Node(), BTimeSource::RealTime());
	}
	bigtime_t lat = 0;
	BMediaRoster::Roster()->GetLatencyFor(_m_node->Node(), &lat);
//	lat = 50000LL;	//	for good measure, since the current thread is not real-time

	LATENCY("SoundPlayerNode latency: %Ld\n", lat);

//fixme:	RACE!!!	(TimeSource())
	bigtime_t now = _m_node->TimeSource()->Now()+lat;
//	BMediaRoster::Roster()->SeekNode(_m_node->Node(), 0, now);

	LATENCY(stderr, "Telling SoundPlayNode to start: %Ld @ %Ld (%Ld @ %Ld)\n",
		now, _m_node->TimeSource()->Now(), _m_node->TimeSource()->RealTimeFor(now, 0), BTimeSource::RealTime());

	status_t r = BMediaRoster::Roster()->StartNode(_m_node->Node(), now);
	if (_Notifier) {
		(*_Notifier)(_m_cookie, B_STARTED, this);
	} else {
		Notify(B_STARTED, this);
	}
	SetHasData(true);
	return r;
}

void BSoundPlayer::Stop(
	bool block,
	bool flush)
{
	//	Fix for programs which delete sound players in global destructors.
	//	B_DONT_DO_THAT.
	if (BMediaRoster::CurrentRoster() == 0) {
		_m_node = NULL;
		return;
	}
	//	did we initialize properly?
	if (!_m_node) {
		return;
	}
//fixme:	RACE!!!	(TimeSource())
	BMediaRoster::Roster()->StopNode(_m_node->Node(), _m_node->TimeSource()->Now(), block);
	if (!_m_lock.Lock()) {
		NODE(stderr, "A crash narrowly avoided!\n");
		return;
	}
	if (flush) {
		for (_playing_sound *ptr = _m_sounds; ptr != NULL;) {
			_playing_sound * del = ptr;
			ptr = del->next;
			del->sound->ReleaseRef();
			delete_sem(del->id);
			NotifySoundDone(del->id, true);
			delete del;
		}
		for (_waiting_sound *ptr = _m_waiting; ptr != NULL;) {
			_waiting_sound * del = ptr;
			ptr = del->next;
			del->sound->ReleaseRef();
			delete_sem(del->id);
			NotifySoundDone(del->id, false);
			delete del;
		}
		_m_sounds = NULL;
		_m_waiting = NULL;
	}
	if (_Notifier) {
		(*_Notifier)(_m_cookie, B_STOPPED, this);
	}
	else {
		Notify(B_STOPPED, this);
	}
	_m_lock.Unlock();
}

BSoundPlayer::BufferPlayerFunc BSoundPlayer::BufferPlayer() const
{
	return _PlayBuffer;
}

void BSoundPlayer::SetBufferPlayer(
	void (*PlayBuffer)(void * cookie, void * buffer, size_t size, const media_raw_audio_format & format))
{
	BAutolock lock(_m_lock);
	_PlayBuffer = PlayBuffer;
}

BSoundPlayer::EventNotifierFunc BSoundPlayer::EventNotifier() const
{
	return _Notifier;
}

void BSoundPlayer::SetNotifier(
	void (*Notifier)(void * cookie, sound_player_notification what, ...))
{
	BAutolock lock(_m_lock);
	_Notifier = Notifier;
}

void * BSoundPlayer::Cookie() const
{
	return _m_cookie;
}

void BSoundPlayer::SetCookie(void * cookie)
{
	BAutolock lock(_m_lock);
	_m_cookie = cookie;
}

void BSoundPlayer::SetCallbacks(
	void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
	void (*Notifier)(void *, sound_player_notification what, ...),
	void * cookie)
{
	BAutolock lock(_m_lock);
	_PlayBuffer = PlayBuffer;
	_Notifier = Notifier;
	_m_cookie = cookie;
}

bigtime_t BSoundPlayer::CurrentTime()
{
	if (!_m_node) {
		return _m_init_err ? _m_init_err : B_ERROR;
	}
	return _m_node->Now();
}

bigtime_t BSoundPlayer::PerformanceTime()
{
	if (!_m_node) {
		return _m_init_err ? _m_init_err : B_ERROR;
	}
	return _m_perfTime;
}

class _stereo_tag_class {
public:
	_stereo_tag_class() { }
};
static _stereo_tag_class stereo_tag;
class _mono_tag_class {
public:
	_mono_tag_class() { }
};
static _mono_tag_class mono_tag;

BSoundPlayer::play_id BSoundPlayer::StartPlaying(
	BSound * sound,
	bigtime_t at_time)
{
	if (!_m_node) {
		return _m_init_err ? _m_init_err : B_ERROR;
	}
	return StartPlaying(sound, at_time, 1.0);
}

BSoundPlayer::play_id BSoundPlayer::StartPlaying(
	BSound * sound,
	bigtime_t at_time,
	float with_volume)
{
	if (!_m_node) {
		return _m_init_err ? _m_init_err : B_ERROR;
	}
	switch (sound->Format().format) {
	case media_raw_audio_format::B_AUDIO_UCHAR:
	case media_raw_audio_format::B_AUDIO_SHORT:
	case media_raw_audio_format::B_AUDIO_INT:
	case media_raw_audio_format::B_AUDIO_FLOAT:
		/* OK */
		break;
	default:
		/* cannot play other formats */
		return B_MEDIA_BAD_FORMAT;
	}
	int chans = sound->Format().channel_count;
	if (chans < 1 || chans > 2) {
		/* can only play mono and stereo */
		return B_MEDIA_BAD_FORMAT;
	}
	BAutolock lock(_m_lock);
	_waiting_sound **ptr = &_m_waiting;
	while (*ptr && ((*ptr)->start_time <= at_time)) {
		ptr = &(*ptr)->next;
	}
	_waiting_sound * n = new _waiting_sound;
	n->next = *ptr;
	n->sound = sound->AcquireRef();
	n->start_time = at_time;
	n->id = create_sem(0, "BSound completion");
	n->rate = (int32)sound->Format().frame_rate;
	n->volume = with_volume;
	*ptr = n;
	sound->_m_checkStopped = false;
	SetHasData(true);
	return n->id;
}

status_t
BSoundPlayer::SetSoundVolume(
	play_id sound,
	float volume)
{
	if (!_m_node) {
		return _m_init_err ? _m_init_err : B_ERROR;
	}
	status_t err = B_BAD_VALUE;
	BAutolock lock(_m_lock);
	//	try playing sounds
	_playing_sound **ps = &_m_sounds;
	while (*ps && ((*ps)->id != sound)) {
		ps = &(*ps)->next;
	}
	if (*ps) {
		(*ps)->volume = volume;
		err = B_OK;
	}
	else {
		//	try waiting sounds
		_waiting_sound **ptr = &_m_waiting;
		while (*ptr && ((*ptr)->id != sound)) {
			ptr = &(*ptr)->next;
		}
		if (*ptr) {
			(*ptr)->volume = volume;
			err = B_OK;
		}
	}
	return err;
}

void
BSoundPlayer::SetHasData(
	bool has_data)
{
	if (!_m_node) {
		return;
	}
	if (has_data) {
		if (!atomic_or(&_m_has_data, 1))
			write_port_etc(m_output.node.port, 0x60000001L, NULL, 0, 0, 0);
	}
	else {
		if (atomic_and(&_m_has_data, 0))
			write_port_etc(m_output.node.port, 0x60000002L, NULL, 0, 0, 0);
	}
}

bool
BSoundPlayer::HasData()
{
	return _m_has_data;
}

bool BSoundPlayer::IsPlaying(
	play_id id)
{
	if (!_m_node) {
		return false;
	}
	BAutolock lock(_m_lock);
	for (_playing_sound * ptr = _m_sounds; ptr; ptr = ptr->next) {
		if (ptr->id == id) return true;
	}
	for (_waiting_sound * ptr = _m_waiting; ptr; ptr = ptr->next) {
		if (ptr->id == id) return true;
	}
	return false;
}

status_t BSoundPlayer::StopPlaying(
	play_id id)
{
	if (!_m_node) {
		return _m_init_err ? _m_init_err : B_ERROR;
	}
	BAutolock lock(_m_lock);
	for (_playing_sound ** ptr = &_m_sounds; *ptr; ptr = &(*ptr)->next) {
		if ((*ptr)->id == id) {
			(*ptr)->sound->ReleaseRef();
			delete_sem((*ptr)->id);
			_playing_sound * del = *ptr;
			*ptr = del->next;
			delete del;
			NotifySoundDone(id, true);
			return B_OK;
		}
	}
	for (_waiting_sound ** ptr = &_m_waiting; *ptr; ptr = &(*ptr)->next) {
		if ((*ptr)->id == id) {
			(*ptr)->sound->ReleaseRef();
			delete_sem((*ptr)->id);
			_waiting_sound * del = *ptr;
			*ptr = del->next;
			delete del;
			NotifySoundDone(id, false);
			return B_OK;
		}
	}
	return B_BAD_VALUE;
}

void
BSoundPlayer::NotifySoundDone(
	play_id id,
	bool did_play)
{
	if (_Notifier) {
		(*_Notifier)(_m_cookie, B_SOUND_DONE, id, did_play);
	}
	else {
		Notify(B_SOUND_DONE, id, did_play);
	}
}

status_t
BSoundPlayer::WaitForSound(
	play_id id)
{
	if (!_m_node) {
		return _m_init_err ? _m_init_err : B_ERROR;
	}
	{
		BAutolock lock(_m_lock);
		for (_playing_sound * ptr = _m_sounds; ptr; ptr = ptr->next) {
			if (ptr->id == id) {
				goto doit;
			}
		}
		for (_waiting_sound * ptr = _m_waiting; ptr; ptr = ptr->next) {
			if (ptr->id == id) {
				goto doit;
			}
		}
		return B_BAD_VALUE;
	}
doit:
	acquire_sem(id);
	return B_OK;
}

static float map_from_db(float db)
{
	float ret = exp(db/6*log(2));
	return ret;
}

static float map_to_db(float straight)
{
	if (straight < 0.000001) straight = 0.000001;
	float ret = 6*log(straight)/log(2);
	return ret;
}

float BSoundPlayer::Volume()
{
	return map_from_db(VolumeDB(false));
}

float BSoundPlayer::VolumeDB(bool forcePoll)
{
	if (!forcePoll && (system_time() < _m_gotVolume + 2000000LL)) {
		return _m_volume;
	}
	get_volume_slider();
	if (!_m_volumeSlider) return 0.0;
	float array[8];
	size_t ioSize = sizeof(array);
	bigtime_t when;
	status_t err = _m_volumeSlider->GetValue(array, &ioSize, &when);
	if (err < B_OK) return 0.0;
	if (ioSize < sizeof(float)) return 0.0;
	//	calculate average
	float volume = 0;
	for (int ix=0; ix<ioSize/sizeof(float); ix++) {
		volume += array[ix];
	}
	volume = volume/(ioSize/sizeof(float));
	_m_gotVolume = system_time();
	_m_volume = volume;
	return volume;
}

void BSoundPlayer::SetVolume(
	float new_volume)
{
	SetVolumeDB(map_to_db(new_volume));
}

void BSoundPlayer::SetVolumeDB(
	float new_volume)
{
	get_volume_slider();
	if (!_m_volumeSlider) return;
	float volume = new_volume;
	if (volume < _m_volumeSlider->MinValue()) {
		volume = _m_volumeSlider->MinValue();
	}
	else if (volume > _m_volumeSlider->MaxValue()) {
		volume = _m_volumeSlider->MaxValue();
	}
	if (fabs(volume-_m_volume) < 0.001) return;
	_m_gotVolume = system_time();
	_m_volume = volume;
	float array[8];
	int32 ioSize = sizeof(array);
	for (int ix=0; ix<8; ix++) {
		array[ix] = volume;
	}
	status_t err = _m_volumeSlider->SetValue(array, _m_volumeSlider->CountChannels()*sizeof(float), -1LL);
}

status_t BSoundPlayer::GetVolumeInfo(
	media_node * out_node,
	int32 * out_parameter,
	float * out_min_db,
	float * out_max_db)
{
	get_volume_slider();
	if (!_m_volumeSlider) return B_MEDIA_SYSTEM_FAILURE;
	if (out_node) *out_node = _m_volumeSlider->Web()->Node();
	if (out_parameter) *out_parameter = _m_volumeSlider->ID();
	if (out_min_db) *out_min_db = _m_volumeSlider->MinValue();
	if (out_max_db) *out_max_db = _m_volumeSlider->MaxValue();
	return B_OK;
}

void BSoundPlayer::Notify(
	sound_player_notification what,
	...)
{
	va_list l;
	va_start(l, what);

	switch (what) {
	case B_STARTED:
		break;
	case B_STOPPED:
		break;
	case B_SOUND_DONE: {
		//	notify Node that we have no more data -- important to save cycles!
		//	now done on demand in PlayBuffer
//		if (!_m_sounds && !_m_waiting) {
//			SetHasData(false);
//		}
		} break;
	default:
		/* huh? */
		break;
	}

	va_end(l);
}

inline float
_get_float(
	const uint8 * data)
{
	return ((int)*data-128)/127.0;
}

inline float
_get_float(
	const int16 * data)
{
	return *data/32767.0;
}

inline float
_get_float(
	const int32 * data)
{
	return *data/2147483647.0;
}

inline float
_get_float(
	const float * data)
{
	return *data;
}


template<class Samp>
int _resample_mix(
	const Samp * data,
	int cnt,
	float * out,
	int phase,
	int rate,
	int target,
	_stereo_tag_class)
{
	RESAMPLE(stderr, "STEREO %s: data %x cnt %x out %x rate %d target %d", typeid(Samp).name(), data, cnt, out, rate, target);
	while (cnt > 0) {
		while ((phase >= target) && (cnt > 0)) {	/* Drop sample conversion! */
			cnt-=sizeof(Samp);
			phase -= target;
			data++;
		}
		if (cnt < 0) {
			break;
		}
		*(out++) = _get_float(data);
		phase += rate;
	}
	return phase;
}

template<class Samp>
int _resample_mix(
	const Samp * data,
	int cnt,
	float * out,
	int phase,
	int rate,
	int target,
	_mono_tag_class)
{
	RESAMPLE(stderr, "MONO %s: data %x cnt %x out %x rate %d target %d", typeid(Samp).name(), data, cnt, out, rate, target);
	while (true) {
		while (phase >= target) {	/* Drop sample conversion! */
			cnt-=sizeof(Samp);
			phase -= target;
			data++;
		}
		if (cnt <= 0) {
			break;
		}
		float f = _get_float(data);
		*(out++) = f;
		*(out++) = f;	/* put in center */
		phase += rate;
	}
	return phase;
}

inline void _write_samp(float f, uint8 * out)
{
	if (f < -1.0) *out = 1;
	else if (f > 1.0) *out = 255;
	else *out = (unsigned char)(f*127+128);
}
inline void _write_samp(float f, int16 * out)
{
	if (f < -1.0) *out = -32767;
	else if (f > 1.0) *out = 32767;
	else *out = (short)(f*32767);
}
inline void _write_samp(float f, int32 * out)
{
	if (f < -1.0) *out = -2147483647;
	else if (f > 1.0) *out = 2147483647;
	else *out = (int32)(f*2147483647);
}
template<class Samp>
void _copy_audio(
	const float * in,
	Samp * out,
	size_t size)
{
	while (size > 0) {
		_write_samp(*in++, out++);
		size -= sizeof(Samp);
	}
}

void BSoundPlayer::PlayBuffer(
	void * buffer,
	size_t size,
	const media_raw_audio_format & format)
{
	if (!_m_lock.Lock()) {
		DIAGNOSTIC(stderr, "BSoundPlayer::PlayBuffer(): Lock gone?\n");
		memset(buffer, format.format == 0x11 ? 0x80 : 0x00, size);
		return;	//	lock gone?
	}
	if (!_m_waiting && !_m_sounds) {
		memset(buffer, format.format == 0x11 ? 0x80 : 0x00, size);
		SetHasData(false);
		_m_lock.Unlock();
		return;
	}

	try {
		// start sounds in the queue
		bigtime_t now = _m_node->Now();
		for (_waiting_sound ** w = &_m_waiting; *w; ) {
			_waiting_sound * del = *w;
			if (del->start_time <= now) {
				*w = del->next;
				int32 sem_count;
				//	sem_count > 0 means someone called stop on the sound from user program
				if ((get_sem_count(del->id, &sem_count) == B_OK) && (sem_count <= 0)) {
					_playing_sound * p = new _playing_sound;
					p->next = _m_sounds;
					p->cur_offset = 0;
					p->sound = del->sound;
					p->id = del->id;
					p->delta = 0;
					p->rate = del->rate;
					p->volume = del->volume;
					_m_sounds = p;
				}
				else {
					NotifySoundDone(del->id, false);
				}
				delete del;
			}
			else {
				w = &(*w)->next;
			}
		}
		int nframes = size/((format.format&0xf)*format.channel_count);
		size_t out_size = nframes*sizeof(float)*2;
		if (!_m_mix_buffer || _m_mix_buffer_size != out_size) {
			assert(sizeof(float) == 4);
			// the if() shouldn't be needed
//			if (_m_mix_buffer) delete[] _m_mix_buffer;
			rtm_free(_m_mix_buffer);
//			_m_mix_buffer = new float[out_size/4];
			_m_mix_buffer = (float *)rtm_alloc(NULL, out_size);
			_m_mix_buffer_size = out_size;
		}
		float * the_mix_buffer = _m_mix_buffer;
		if (format.format == media_raw_audio_format::B_AUDIO_FLOAT) {
			the_mix_buffer = (float *)buffer;
		}
		/* for each playing sound */
		bool first_sound = true;
		size_t played = 0;
		for (_playing_sound ** ptr = &_m_sounds; *ptr != NULL;) {
			_playing_sound * it = *ptr;
		/* get sound data */
			const media_raw_audio_format & dfmt(it->sound->Format());
			int framesize = dfmt.channel_count*(dfmt.format & 0xf);
			size_t needed = nframes*framesize;
			/* realloc so it'll fit */
			if (needed > _m_bufsize) {
				_m_buf = realloc(_m_buf, needed);
				ASSERT(_m_buf != NULL || needed == 0);
				_m_bufsize = needed;
			}
			/* get the bits */
			size_t used = 0;
			DATA(stderr, "GetDataAt()\n");
			if (it->sound->GetDataAt(it->cur_offset, _m_buf, needed, &used)) {
				it->cur_offset += used;
				DATA(stderr, "GetDataAt() returned %ld\n", used);
				LOCATION(stderr, "offset now: %Ld (needed %ld, used %ld)\n", it->cur_offset, needed, used);
				used /= framesize;
				/* switch on data format */
				/* this loop assumes we're playing the same number of channels as the sound is */
				int16 remdr = used & 3;
				size_t r_used = used;
				used = used - remdr;
				remdr *= dfmt.channel_count;
				int16 ccnt = dfmt.channel_count*used;
				float volume = /* _m_volume * */ it->volume;
				switch (dfmt.format) {
				case media_raw_audio_format::B_AUDIO_UCHAR: {
					DATA(stderr, "Converting UCHAR data\n");
					if (first_sound) {
						unsignedByteToFloat1(the_mix_buffer, (uint8 *)_m_buf, &volume, ccnt);
						while (remdr--) {
							the_mix_buffer[ccnt+remdr] = ((short)((uint8 *)_m_buf)[ccnt+remdr]-128)/127.0*volume;
						}
					}
					else {
						unsignedByteToFloatAccum1(the_mix_buffer, (uint8 *)_m_buf, &volume, dfmt.channel_count*used);
						while (remdr--) {
							the_mix_buffer[ccnt+remdr] += ((short)((uint8 *)_m_buf)[ccnt+remdr]-128)/127.0*volume;
						}
					}
					} break;
				case media_raw_audio_format::B_AUDIO_SHORT: {
					DATA(stderr, "Converting SHORT data\n");

					#if B_HOST_IS_BENDIAN
					if (dfmt.byte_order != B_MEDIA_BIG_ENDIAN)
					#else
					if (dfmt.byte_order != B_MEDIA_LITTLE_ENDIAN)
					#endif
						swap_data(B_INT16_TYPE, (void *)_m_buf, used*framesize, B_SWAP_ALWAYS);

					if (first_sound) {
						wordToFloat1(the_mix_buffer, (int16 *)_m_buf, &volume, dfmt.channel_count*used);
						while (remdr--) {
							the_mix_buffer[ccnt+remdr] = ((int16 *)_m_buf)[ccnt+remdr]/32767.0*volume;
						}
					}
					else {
						wordToFloatAccum1(the_mix_buffer, (int16 *)_m_buf, &volume, dfmt.channel_count*used);
						while (remdr--) {
							the_mix_buffer[ccnt+remdr] += ((int16 *)_m_buf)[ccnt+remdr]/32767.0*volume;
						}
					}
					} break;
				case media_raw_audio_format::B_AUDIO_INT: {
					DATA(stderr, "Converting INT data\n");

					#if B_HOST_IS_BENDIAN
					if (dfmt.byte_order != B_MEDIA_BIG_ENDIAN)
					#else
					if (dfmt.byte_order != B_MEDIA_LITTLE_ENDIAN)
					#endif
						swap_data(B_INT32_TYPE, (void *)_m_buf, used*framesize, B_SWAP_ALWAYS);

					if (first_sound) {
						intToFloat1(the_mix_buffer, (int32 *)_m_buf, &volume, dfmt.channel_count*used);
						while (remdr--) {
							the_mix_buffer[ccnt+remdr] = ((int32 *)_m_buf)[ccnt+remdr]/(float)LONG_MAX*volume;
						}
					}
					else {
						intToFloatAccum1(the_mix_buffer, (int32 *)_m_buf, &volume, dfmt.channel_count*used);
						while (remdr--) {
							the_mix_buffer[ccnt+remdr] += ((int32 *)_m_buf)[ccnt+remdr]/(float)LONG_MAX*volume;
						}
					}
					} break;
				case media_raw_audio_format::B_AUDIO_FLOAT: {
					DATA(stderr, "Converting FLOAT data\n");

					#if B_HOST_IS_BENDIAN
					if (dfmt.byte_order != B_MEDIA_BIG_ENDIAN)
					#else
					if (dfmt.byte_order != B_MEDIA_LITTLE_ENDIAN)
					#endif
						swap_data(B_FLOAT_TYPE, (void *)_m_buf, used*framesize, B_SWAP_ALWAYS);

					if (first_sound) {
						floatToFloat1(the_mix_buffer, (float *)_m_buf, &volume, dfmt.channel_count*used);
						while (remdr--) {
							the_mix_buffer[ccnt+remdr] = ((float *)_m_buf)[ccnt+remdr]*volume;
						}
					}
					else {
						floatToFloatAccum1(the_mix_buffer, (float *)_m_buf, &volume, dfmt.channel_count*used);
						while (remdr--) {
							the_mix_buffer[ccnt+remdr] += ((float *)_m_buf)[ccnt+remdr]*volume;
						}
					}
					} break;
				default:
					DIAGNOSTIC(stderr, "Unknown raw audio format %lx played in SoundPlayer", dfmt.format);
					goto retire;
				}
				if (r_used > played) {
					played = r_used;
				}
				ptr = &(*ptr)->next;
			}
			else {
		retire:
				DATA(stderr, "GetDataAt() -- retiring sound\n");
				/* retire sound */
				play_id id = it->id;
				*ptr = it->next;
				it->sound->ReleaseRef();
				delete_sem(it->id);
				NotifySoundDone(id, true);
				delete it;
			}
			if (first_sound && (played < nframes)) {
				ssize_t nn = nframes * format.channel_count - 1;
				ssize_t pp = played * format.channel_count - 1;
				while (pp < nn) {
					the_mix_buffer[++pp] = 0.0;
				}
				played = nframes;
			}
			first_sound = false;
		}
		DATA(stderr, "All mixing done for this buffer\n");

		if (nframes > played) {
			ssize_t nn = nframes * format.channel_count - 1;
			ssize_t pp = played * format.channel_count - 1;
			while (pp < nn) {
				the_mix_buffer[++pp] = 0.0;
			}
			played = nframes;
		}

		#if B_HOST_IS_BENDIAN
		bool needSwap = (format.byte_order != B_MEDIA_BIG_ENDIAN);
		#else
		bool needSwap = (format.byte_order != B_MEDIA_LITTLE_ENDIAN);
		#endif

		/* switch on output format and store to buffer */
		switch (format.format) {
		case media_raw_audio_format::B_AUDIO_UCHAR:
			DATA(stderr, "Writing UCHAR data\n");
			_copy_audio(the_mix_buffer, (uint8 *)buffer, played*format.channel_count);
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			DATA(stderr, "Writing SHORT data\n");
			convertBufferFloatToShort((int16 *)buffer, the_mix_buffer, played*format.channel_count, 32767.0);
			if (needSwap) swap_data(B_INT16_TYPE, (void *)buffer, size, B_SWAP_ALWAYS);
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			DATA(stderr, "Writing INT data\n");
			_copy_audio(the_mix_buffer, (int32 *)buffer, played*format.channel_count*(format.format & 0xf));
			if (needSwap) swap_data(B_INT32_TYPE, (void *)buffer, size, B_SWAP_ALWAYS);
			break;
		case media_raw_audio_format::B_AUDIO_FLOAT:
//			the output buffer is our mix buffer
//			memcpy(buffer, _m_mix_buffer, size);
			if (needSwap) swap_data(B_FLOAT_TYPE, (void *)buffer, size, B_SWAP_ALWAYS);
			break;
		default:
			DATA(stderr, "Unknown output format %lx in SoundPlayer", format.format);
			break;
		}
	}
	catch (...) {
		DIAGNOSTIC(stderr, "SoundPlayer threw an exception\n");
		_m_lock.Unlock();
		throw;
	}
	_m_lock.Unlock();
}


status_t
BSoundPlayer::InitCheck()
{
	return _m_init_err;
}

void
BSoundPlayer::SetInitError(
	status_t in_error)
{
	_m_init_err = in_error;
}


media_raw_audio_format
BSoundPlayer::Format() const
{
	return m_output.format.u.raw_audio;
}





status_t BSoundPlayer::_Reserved_SoundPlayer_0(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_1(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_2(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_3(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_4(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_5(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_6(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_7(void *, ...) { return B_ERROR; }
