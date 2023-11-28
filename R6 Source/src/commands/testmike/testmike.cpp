
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Button.h>
#include <Message.h>
#include <StringView.h>
#include <MessageRunner.h>
#include <errno.h>
#include <Alert.h>
#include <SoundPlayer.h>
#include <Slider.h>
#include <CheckBox.h>
#include <OptionPopUp.h>

#include "game_audio.h"
#include <map>


#define SAMPLESIZE 2
#define SECS 12
#define SAMPLERATE 44100
#define INCHANNELS 2
#define FRAMECOUNT 1024


template<class T> class G : public T {
public:
	G() { memset(this, 0, sizeof(*this)); info_size = sizeof(*this); }
};

template<class T> class H : public T {
public:
	H() { memset(this, 0, sizeof(*this)); }
};


enum
{
	RECORD = 'rec ',
	PLAY = 'play',
	PULSE = 'puls',
	STOP = 'stop',
	CONTROL = 'ctrl'
};


class W : public BWindow
{
		volatile enum
		{
			kStopped,
			kRecording,
			kPlaying
		} m_state;
		BStringView * m_text;
		BMessageRunner * m_runner;
		size_t m_bufSize;
		void * m_buffer;
		size_t m_bufUsed;
		size_t m_bufOffset;
		int m_fd;

		sem_id rec_sem;
		int rec_stream;
		int rec_buf;
		area_id rec_area;
		size_t rec_size;
		void * rec_base;
		size_t rec_chunk;
		int cur_rec_page;
		thread_id rec_thread;

		BSoundPlayer * m_player;

		status_t open_device();
		status_t config_device();
		status_t open_stream();
		status_t alloc_buffer();

		void set_controls(game_mixer_control *);
		void get_controls(game_mixer_control *);
		status_t do_mixer(void (W::*func)(game_mixer_control *));

		static status_t rec_thread_ent(void *);
		void recording();
		static void play_thread_ent(void * cookie, void * buf, size_t size, const media_raw_audio_format & fmt);
		void playing(void * buf, size_t size);

		map<uint16, game_mixer_control_value> m_values;

public:
		W();
		~W();
		bool QuitRequested();
		void MessageReceived(BMessage * msg);

		void Record();
		void Play();
		void Stop();
		void Pulse();
		void Control(BMessage * msg);
};

void 
W::set_controls(game_mixer_control * ctrl)
{
	map<uint16, game_mixer_control_value>::iterator ptr(m_values.find(ctrl->control_id));
	if (ptr != m_values.end())
	{
		G<game_set_mixer_control_values> gsmcvs;
		gsmcvs.in_request_count = 1;
		gsmcvs.values = &(*ptr).second;
fprintf(stderr, "setting value for %d\n", (*ptr).second.control_id);
		if (ioctl(m_fd, GAME_SET_MIXER_CONTROL_VALUES, &gsmcvs) < 0)
		{
			fprintf(stderr, "SET_MIXER_CONTROL_VALUES(%d): %s\n",
				(*ptr).second.control_id, strerror(errno));
		}
	}
}

void 
W::get_controls(game_mixer_control * ctrl)
{
	switch (ctrl->kind)
	{
	case GAME_MIXER_CONTROL_IS_LEVEL:
		{
			G<game_get_mixer_level_info> ggmli;
			ggmli.control_id = ctrl->control_id;
			ggmli.mixer_id = ctrl->mixer_id;
			if (ioctl(m_fd, GAME_GET_MIXER_LEVEL_INFO, &ggmli) < 0)
			{
				fprintf(stderr, "GET_MIXER_LEVEL_INFO(%d): %s\n", ctrl->control_id,
					strerror(errno));
			}
			else if ((ggmli.type == GAME_LEVEL_AC97_RECORD) ||
				(ggmli.type == GAME_LEVEL_AC97_RECORD_MIC))
			{
				G<game_get_mixer_control_values> ggmcvs;
				H<game_mixer_control_value> gmcv;
				ggmcvs.in_request_count = 1;
				ggmcvs.values = &gmcv;
				gmcv.control_id = ctrl->control_id;
				gmcv.mixer_id = ctrl->mixer_id;
				if (ioctl(m_fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0)
				{
					fprintf(stderr, "GAME_GET_MIXER_CONTROL_VALUES(%d): %s\n",
						ctrl->control_id, strerror(errno));
				}
				else
				{
fprintf(stderr, "m_values[%d]\n", ggmli.control_id);
					m_values[ggmli.control_id] = gmcv;
					BRect r(Bounds());
					r.top = r.bottom+1;
					r.bottom = r.top+47;
					r.left += 10;
					r.right -= 10;
					BMessage msg(CONTROL);
					msg.AddInt32("control", ctrl->control_id);
					msg.AddInt32("mixer", ctrl->mixer_id);\
					msg.AddInt32("type", ggmli.type);
					BSlider * slid = new BSlider(r, ggmli.label, ggmli.label,
						new BMessage(msg), ggmli.min_value, ggmli.max_value);
					slid->SetModificationMessage(new BMessage(msg));
					slid->SetValue(gmcv.level.values[0]);
					AddChild(slid);
					if (ggmli.flags & GAME_LEVEL_HAS_MUTE)
					{
						r.top = r.bottom+1;
						r.bottom = r.top+17;
						msg.AddBool("has_mute", true);
						BCheckBox * cb = new BCheckBox(r, "Mute", "Mute",
							new BMessage(msg));
						cb->SetValue(gmcv.level.flags & 1);
						AddChild(cb);
					}
					ResizeBy(0, r.bottom-Bounds().bottom+10);
				}
			}
		}
		break;
	case GAME_MIXER_CONTROL_IS_MUX:
		{
			G<game_get_mixer_mux_info> ggmmi;
			H<game_mux_item> gmi[16];
			ggmmi.control_id = ctrl->control_id;
			ggmmi.mixer_id = ctrl->mixer_id;
			ggmmi.items = gmi;
			ggmmi.in_request_count = 16;
			if (ioctl(m_fd, GAME_GET_MIXER_MUX_INFO, &ggmmi) < 0)
			{
				fprintf(stderr, "GET_MIXER_MUX_INFO(%d): %s\n", ctrl->control_id,
					strerror(errno));
			}
			else if ((ggmmi.type == GAME_MUX_AC97_RECORD_SELECT) ||
				(ggmmi.type == GAME_MUX_AC97_MIC_SELECT) ||
				(ggmmi.type == GAME_MUX_AC97_MONO_SELECT))
			{
				G<game_get_mixer_control_values> ggmcvs;
				H<game_mixer_control_value> gmcv;
				ggmcvs.in_request_count = 1;
				ggmcvs.values = &gmcv;
				gmcv.control_id = ctrl->control_id;
				gmcv.mixer_id = ctrl->mixer_id;
				if (ioctl(m_fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0)
				{
					fprintf(stderr, "GAME_GET_MIXER_CONTROL_VALUES(%d): %s\n",
						ctrl->control_id, strerror(errno));
				}
				else
				{
fprintf(stderr, "m_values[%d]\n", ggmmi.control_id);
					m_values[ggmmi.control_id] = gmcv;
					BRect r(Bounds());
					r.top = r.bottom+1;
					r.bottom = r.top+23;
					r.left += 10;
					r.right -= 10;
					BMessage msg(CONTROL);
					msg.AddInt32("control", ctrl->control_id);
					msg.AddInt32("mixer", ctrl->mixer_id);\
					msg.AddInt32("type", ggmmi.type);
					BOptionPopUp * oc = new BOptionPopUp(r, ggmmi.label, ggmmi.label,
						new BMessage(msg));
					for (int ix=0; ix<ggmmi.out_actual_count; ix++)
					{
						oc->AddOption(gmi[ix].name, gmi[ix].mask);
					}
					oc->SetValue(gmcv.mux.mask);
					AddChild(oc);
					ResizeBy(0, r.bottom-Bounds().bottom+10);
				}
			}
		}
		break;
	case GAME_MIXER_CONTROL_IS_ENABLE:
		{
			G<game_get_mixer_enable_info> ggmei;
			ggmei.control_id = ctrl->control_id;
			ggmei.mixer_id = ctrl->mixer_id;
			if (ioctl(m_fd, GAME_GET_MIXER_ENABLE_INFO, &ggmei) < 0)
			{
				fprintf(stderr, "GET_MIXER_ENABLE_INFO(%d): %s\n", ctrl->control_id,
					strerror(errno));
			}
			else if ((ggmei.type == GAME_ENABLE_AC97_MIC_BOOST))
			{
				G<game_get_mixer_control_values> ggmcvs;
				H<game_mixer_control_value> gmcv;
				ggmcvs.in_request_count = 1;
				ggmcvs.values = &gmcv;
				gmcv.control_id = ctrl->control_id;
				gmcv.mixer_id = ctrl->mixer_id;
				if (ioctl(m_fd, GAME_GET_MIXER_CONTROL_VALUES, &ggmcvs) < 0)
				{
					fprintf(stderr, "GAME_GET_MIXER_CONTROL_VALUES(%d): %s\n",
						ctrl->control_id, strerror(errno));
				}
				else
				{
fprintf(stderr, "m_values[%d]\n", ggmei.control_id);
					m_values[ggmei.control_id] = gmcv;
					BRect r(Bounds());
					r.top = r.bottom+1;
					r.bottom = r.top+17;
					r.left += 10;
					r.right -= 10;
					BMessage msg(CONTROL);
					msg.AddInt32("control", ctrl->control_id);
					msg.AddInt32("mixer", ctrl->mixer_id);\
					msg.AddInt32("type", ggmei.type);
					BCheckBox * oc = new BCheckBox(r, ggmei.label, ggmei.label,
						new BMessage(msg));
					oc->SetValue(gmcv.enable.enabled ? 1 : 0);
					AddChild(oc);
					ResizeBy(0, r.bottom-Bounds().bottom+10);
				}
			}
		}
		break;
	default:
		fprintf(stderr, "unknown control kind %d for %d\n", ctrl->kind, ctrl->control_id);
	}
}

status_t
W::do_mixer(
	void (W::*func)(game_mixer_control *))
{
	status_t err = B_OK;
	for (int m = 0; true; m++)
	{
		G<game_get_mixer_infos> ggmis;
		H<game_mixer_info> gmi;
		gmi.mixer_id = GAME_MAKE_MIXER_ID(m);
		ggmis.info = &gmi;
		ggmis.in_request_count = 1;
		if (ioctl(m_fd, GAME_GET_MIXER_INFOS, &ggmis) < 0)
		{
			fprintf(stderr, "GET_MIXER_INFOS(%d): %s\n", m, strerror(errno));
			break;
		}
		fprintf(stderr, "mixer %d (%s) with %d controls\n", gmi.mixer_id, gmi.name,
			gmi.control_count);
		if (gmi.mixer_id == GAME_NO_ID) break;
		for (int o = 0; o < gmi.control_count; o += 32)
		{
			G<game_get_mixer_controls> ggmcs;
			H<game_mixer_control> gmc[32];
			ggmcs.mixer_id = GAME_MAKE_MIXER_ID(m);
			ggmcs.from_ordinal = o;
			ggmcs.control = gmc;
			ggmcs.in_request_count = ((gmi.control_count-o) > 32) ? 32 : (gmi.control_count-o);
			err = (ioctl(m_fd, GAME_GET_MIXER_CONTROLS, &ggmcs) < 0) ? errno : B_OK;
			fprintf(stderr, "GET_MIXER_CONTROLS 0x%x\n", err);
			if (err < 0)
			{
				break;
			}
			for (int c = 0; c < ggmcs.out_actual_count; c++)
			{
				(this->*func)(&gmc[c]);
			}
		}
	}
	return err;
}

status_t
W::config_device()
{
	game_codec_format gcf;
	memset(&gcf, 0, sizeof(gcf));
	G<game_set_codec_formats> gscfs;
	gscfs.formats = &gcf;
	gscfs.in_request_count = 1;
	gcf.codec = GAME_MAKE_ADC_ID(0);
	gcf.flags = GAME_CODEC_SET_ALL;
	gcf.channels = INCHANNELS;
	gcf.chunk_frame_count = FRAMECOUNT;
	gcf.format = B_FMT_16BIT;
	gcf.frame_rate = B_SR_44100;
	status_t err = (ioctl(m_fd, GAME_SET_CODEC_FORMATS, &gscfs) < 0) ? errno : B_OK;
	fprintf(stderr, "SET_CODEC_FORMATS 0x%x\n", err);
	if (err < 0) return err;

	err = do_mixer(&set_controls);
	return err;
}

status_t 
W::open_stream()
{
	rec_sem = create_sem(0, "rec_stream_sem");
	G<game_open_stream> gos;
	gos.codec_id = GAME_MAKE_ADC_ID(0);
	gos.request = 0;
	gos.stream_sem = rec_sem;
	status_t err = ioctl(m_fd, GAME_OPEN_STREAM, &gos);
	if (err < 0) err = errno; else err = B_OK;
	rec_stream = gos.out_stream_id;
	fprintf(stderr, "GAME_OPEN_STREAM 0x%x stream %d\n", err, rec_stream);
	return err;
}

status_t 
W::alloc_buffer()
{
	H<game_open_stream_buffer> gosb;
	G<game_open_stream_buffers> gosbs;
	gosb.stream = rec_stream;
fprintf(stderr, "binding buffer to %d\n", gosb.stream);
//gosb.stream = gosb.stream;
	rec_chunk = FRAMECOUNT*SAMPLESIZE*INCHANNELS;
	rec_size = rec_chunk*2;
	gosb.byte_size = rec_size;
	gosbs.in_request_count = 1;
	gosbs.buffers = &gosb;
	status_t err = (ioctl(m_fd, GAME_OPEN_STREAM_BUFFERS, &gosbs) < 0) ? errno : B_OK;
	fprintf(stderr, "GAME_OPEN_STREAM_BUFFERS 0x%x\n", err);
	if (err < 0) return err;
	rec_buf = gosb.buffer;
	void * d;
	rec_area = clone_area("buffers (pingpong)", &d,
			B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, gosb.area);
	if (rec_area < 0) return rec_area;
	fprintf(stderr, "rec_area 0x%x\n", rec_area);
	rec_base = d;
	return B_OK;
}

W::W() :
	BWindow(BRect(100,100,300,170), "Recording", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	m_state(kStopped),
	m_bufUsed(0),
	m_bufOffset(0)
{
	m_fd = -1;
	AddChild(new BButton(BRect(10,10,95,34), "rec", "Record",
		new BMessage(RECORD)));
	AddChild(new BButton(BRect(105,10,190,34), "play", "Play",
		new BMessage(PLAY)));
	AddChild(m_text = new BStringView(BRect(10,44,190,60), "", "Stopped"));
	BMessage puls(PULSE);
	m_runner = new BMessageRunner(BMessenger(this), &puls, 235000LL);
	m_bufSize = SECS*SAMPLERATE*SAMPLESIZE*INCHANNELS;
	m_buffer = new char[m_bufSize];
	if (m_buffer == NULL)
	{
		(new BAlert("", "Not enough memory!", "Stop"))->Go();
	}

	if (open_device() >= 0)
	{
		do_mixer(&get_controls);
		close(m_fd);
		m_fd = -1;
	}
	media_raw_audio_format fmt;
	fmt.frame_rate = SAMPLERATE;
	fmt.format = 0x2;
	fmt.channel_count = INCHANNELS;
	fmt.byte_order = B_MEDIA_HOST_ENDIAN;
	fmt.buffer_size = FRAMECOUNT*SAMPLESIZE*INCHANNELS;
	m_player = new BSoundPlayer(&fmt, "Recorder Playback", play_thread_ent, NULL, this);
	m_player->SetHasData(false);
	m_player->Start();
}

W::~W()
{
	Stop();
	delete m_player;
	delete m_runner;
	free(m_buffer);
	be_app->PostMessage(B_QUIT_REQUESTED);
}

status_t
W::open_device()
{
	status_t err = B_OK;
	m_fd = open("/dev/audio/game/xpress/1", O_RDWR);
	if (m_fd < 0)
	{
		err = errno;
		char msg[200];
		sprintf(msg, "Error opening device: %s", strerror(errno));
		(new BAlert("", msg, "Stop"))->Go();
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	else
	{
		err = config_device();
		if (!err) err = open_stream();
		if (!err) err = alloc_buffer();
		if (err < 0) {
			char msg[200];
			sprintf(msg, "Error configuring device: %s", strerror(err));
			(new BAlert("", msg, "Stop"))->Go();
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
	}
	return err;
}

bool
W::QuitRequested()
{
	Stop();
	return true;
}

void
W::MessageReceived(
	BMessage * msg)
{
//fprintf(stderr, "%c%c%c%c\n", msg->what>>24, msg->what>>16, msg->what>>8, msg->what);
	int pState = m_state;
	switch (msg->what)
	{
	case RECORD:
		if (pState != kStopped)
			Stop();
		if (pState != kRecording)
			Record();
		break;
	case PLAY:
		if (pState != kStopped)
			Stop();
		if (pState != kPlaying)
			Play();
		break;
	case STOP:
		Stop();
		break;
	case PULSE:
		Pulse();
		break;
	case CONTROL:
		Control(msg);
		break;
	default:
		BWindow::MessageReceived(msg);
		break;
	}
}

void
W::Control(
	BMessage * msg)
{
	int32 control;
	int32 value;
//fprintf(stderr, "W::Control()\n");
	if ((msg->FindInt32("control", &control) < 0) ||
		(msg->FindInt32("be:value", &value) < 0))
	{
		msg->PrintToStream();
		fprintf(stderr, "can't find type or be:value\n");
		return;
	}
	map<uint16, game_mixer_control_value>::iterator ptr(m_values.find(control));
	if (ptr == m_values.end())
	{
		fprintf(stderr, "unknown control: %d\n", control);
		return;
	}
	bool m;
	switch ((*ptr).second.kind)
	{
	case GAME_MIXER_CONTROL_IS_LEVEL:
		if (msg->FindBool("has_mute", &m) < 0)
		{
			(*ptr).second.level.values[0] = value;
			(*ptr).second.level.values[1] = value;
			(*ptr).second.level.values[2] = value;
			(*ptr).second.level.values[3] = value;
			(*ptr).second.level.values[4] = value;
			(*ptr).second.level.values[5] = value;
		}
		else
		{
			(*ptr).second.level.flags = (value ? 1 : 0);
		}
		break;
	case GAME_MIXER_CONTROL_IS_MUX:
		(*ptr).second.mux.mask = value;
		break;
	case GAME_MIXER_CONTROL_IS_ENABLE:
		(*ptr).second.enable.enabled = value;
		break;
	default:
		fprintf(stderr, "unknown level/mux/enable type %d\n", (*ptr).second.kind);
	}
	if (m_fd >= 0)
	{
fprintf(stderr, "m_fd is %d\n", m_fd);
		G<game_set_mixer_control_values> gsmcvs;
		gsmcvs.in_request_count = 1;
		gsmcvs.values = &(*ptr).second;
		if (ioctl(m_fd, GAME_SET_MIXER_CONTROL_VALUES, &gsmcvs) < 0)
		{
			fprintf(stderr, "SET_MIXER_CONTROL_VALUES(%d): %s\n",
				(*ptr).second.control_id, strerror(errno));
		}
	}
}

status_t
W::rec_thread_ent(
	void * arg)
{
	((W *)arg)->recording();
	return B_OK;
}

void
W::recording()
{
	status_t err = B_OK;

	m_bufOffset = 0;
	m_bufUsed = 0;
	cur_rec_page = 0;
	if (err == B_OK) {
		G<game_set_stream_buffer> gssb;
		gssb.stream = rec_stream;
		gssb.flags = GAME_BUFFER_PING_PONG;
		gssb.buffer = rec_buf;
		gssb.frame_count = rec_size/(SAMPLESIZE*INCHANNELS);
		gssb.page_frame_count = rec_chunk/(SAMPLESIZE*INCHANNELS);
fprintf(stderr, "Q: %d 0x%x %d %ld %ld\n", gssb.stream, gssb.flags, 
gssb.buffer, gssb.frame_count, gssb.page_frame_count);
		err = (ioctl(m_fd, GAME_SET_STREAM_BUFFER, &gssb) < 0) ? errno : B_OK;
		fprintf(stderr, "QUEUE_SET_STREAM_BUFFER 0x%x\n", err);
	}
	if (err == B_OK) {
		H<game_run_stream> grs1;
		grs1.stream = rec_stream;
		grs1.state = GAME_STREAM_RUNNING;
		G<game_run_streams> grs;
		grs.in_stream_count = 1;
		grs.streams = &grs1;
		err = (ioctl(m_fd, GAME_RUN_STREAMS, &grs) < 0) ? errno : B_OK;
		fprintf(stderr, "RUN_STREAMS 0x%x\n", err);
	}
	while (m_state == kRecording)
	{
		if (m_bufOffset + rec_chunk > m_bufSize)
		{
			break;
		}
		err = acquire_sem_etc(rec_sem, 1, B_RELATIVE_TIMEOUT, 1000000LL);
		if (err < 0)
		{
			fprintf(stderr, "acquire_sem(): %s\n", strerror(err));
			break;
		}
		memcpy((char *)m_buffer + m_bufOffset, (char *)rec_base + rec_chunk*cur_rec_page,
			rec_chunk);
		cur_rec_page ^= 1;
		m_bufOffset += rec_chunk;
		if (m_bufOffset > m_bufUsed)
			m_bufUsed = m_bufOffset;
	}
	if (err < 0)
	{
		char msg[200];
		sprintf(msg, "recording error: %s", strerror(err));
		(new BAlert("", msg, "Stop"))->Go();
	}
#if 0
	{
		H<game_run_stream> grs1;
		grs1.stream = rec_stream;
		grs1.state = GAME_STREAM_STOPPED;
		G<game_run_streams> grs;
		grs.in_stream_count = 1;
		grs.streams = &grs1;
		err = (ioctl(m_fd, GAME_RUN_STREAMS, &grs) < 0) ? errno : B_OK;
		fprintf(stderr, "stop stream: %s\n", strerror(err));
	}
#endif
	PostMessage(STOP);
}

void 
W::play_thread_ent(void *cookie, void *buf, size_t size, const media_raw_audio_format &)
{
	((W *)cookie)->playing(buf, size);
}

void 
W::playing(void *buf, size_t size)
{
	bool stop = false;
	ssize_t togo = size;

	if (m_state != kPlaying)
	{
		stop = true;
		togo = 0;
	}
	if (togo + m_bufOffset > m_bufUsed)
	{
		togo = m_bufUsed - m_bufOffset;
	}
	if (togo <= 0)
	{
		stop = true;
		togo = 0;
	}
	else
	{
		memcpy(buf, ((char *)m_buffer)+m_bufOffset, togo);
		m_bufOffset += togo;
	}
	if (togo < size)
	{
		memset(((char *)buf)+togo, 0, size-togo);
	}
	if (stop)
	{
		m_player->SetHasData(false);
		PostMessage(STOP);
	}
}

void 
W::Record()
{
	if (m_state != kStopped)
	{
		fprintf(stderr, "error: entering recording while not stopped\n");
		return;
	}
	if (open_device() < 0) return;
	m_state = kRecording;
	rec_thread = spawn_thread(rec_thread_ent, "record thread", 110, this);
	resume_thread(rec_thread);
}

void 
W::Play()
{
	if (m_state != kStopped)
	{
		fprintf(stderr, "error: entering playing while not stopped\n");
		return;
	}
	m_bufOffset = 0;
	m_state = kPlaying;
	m_player->SetHasData(true);
}

void 
W::Stop()
{
	if (m_state == kPlaying)
	{
		m_state = kStopped;
		m_player->SetHasData(false);
	}
	else if (m_state == kRecording)
	{
		m_state = kStopped;
		status_t s;
		wait_for_thread(rec_thread, &s);
		close(m_fd);
		m_fd = -1;
	}
}

void 
W::Pulse()
{
	char msg[200];
	if (m_state == kStopped)
		strcpy(msg, "Stopped");
	else if (m_state == kPlaying)
		sprintf(msg, "Playing %.1f s", (double)m_bufOffset/(SAMPLERATE*INCHANNELS*SAMPLESIZE));
	else if (m_state == kRecording)
		sprintf(msg, "Recording %.1f s", (double)m_bufOffset/(SAMPLERATE*INCHANNELS*SAMPLESIZE));
	else
		strcpy(msg, "Unknown???");
	if (strcmp(m_text->Text(), msg))
		m_text->SetText(msg);
}

class A : public BApplication
{
public:
		A();
		void ArgvReceived(int32 argc, char ** argv);
};

A::A() :
	BApplication("application/x-vnd.be.testmike")
{
}

void
A::ArgvReceived(
	int32 argc,
	char ** argv)
{
	for (int ix=0; ix<argc; ix++)
	{
		if (!strcmp(argv[ix], "--play"))
		{
			WindowAt(0)->PostMessage(PLAY);
		}
		else if (!strcmp(argv[ix], "--record"))
		{
			WindowAt(0)->PostMessage(RECORD);
		}
		else if (!strcmp(argv[ix], "--stop"))
		{
			WindowAt(0)->PostMessage(STOP);
		}
		else if (!strcmp(argv[ix], "--quit"))
		{
			WindowAt(0)->PostMessage(B_QUIT_REQUESTED);
		}
		else
		{
			fprintf(stderr, "unknown argument: %s\n", argv[ix]);
		}
	}
}

int
main()
{
	A app;
	(new W)->Show();
	app.Run();
	return 0;
}
