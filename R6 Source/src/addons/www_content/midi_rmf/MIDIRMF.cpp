
#include <Locker.h>
#include <Autolock.h>
#include <list>
#include <algorithm>
#include <Alert.h>
#include <TellBrowser.h>
#include <stdio.h>

#include "Content.h"
#include "BAE.h"

#define ALERT_ON_OVERLOAD 0

#if 1
#define FUNC() printf("%s (%p)\n", __PRETTY_FUNCTION__, this)
#define FUNC2() puts(__PRETTY_FUNCTION__)
#else
#define FUNC() (void)0
#define FUNC2() (void)0
#endif

using namespace Wagner;

#include <list.h>

extern "C" void __pure_virtual() {
	debugger("pure virtual method called");
}

class MIDIRMFContent;

static void stop_midi();

class MIDIRMFContentInstance : public ContentInstance {

	friend class MIDIRMFContent;

public:

	MIDIRMFContentInstance(
							MIDIRMFContent * parent,
							GHandler * h,
							const BMessage & msg);
	virtual ~MIDIRMFContentInstance();
	virtual	status_t GetSize(
							int32 *x,
							int32 *y,
							uint32 *outResizeFlags);
private:

	void ReadConfig(
							const BMessage & msg);

	MIDIRMFContent * m_parent;
	bool m_autoStart;
	bool m_loop;
	bool m_playing;
};

class MIDIRMFContent : public Content {

	friend void stop_midi();

public:

	MIDIRMFContent(
							void* handle,
							bool isRMF);
	virtual ~MIDIRMFContent();
	virtual ssize_t Feed(
							const void *buffer,
							ssize_t bufferLen,
							bool done);
	virtual size_t GetMemoryUsage();
	void Remove(
							MIDIRMFContentInstance * instance);

	void StartInstance(
							MIDIRMFContentInstance * instance,
							bool looping);

private:

	virtual status_t CreateInstance(
							ContentInstance **outInstance,
							GHandler *handler,
							const BMessage & msg);
	void Start();
	void Stop();

	BMallocIO m_data;
	int m_playingCount;
	bool m_done;
	bool m_isRMF;
	bool m_looping;
	BAEMidiSong * m_song;
	BLocker m_lock;

};

static BAEOutputMixer * g_mixer;
static BLocker g_locker("g_BAEAudioMixer");
static int g_refCount;

static bigtime_t g_lastOverload;
extern bool _bae_overload;
extern void (*_bae_overloadHook)(void *);
static int g_overloadCnt = 0;
static BLocker g_listLock("g_listLock");	//	can't use g_locker for deadlocks
static list<MIDIRMFContent *> g_list;

static void
stop_midi()
{
FUNC2();
	for (list<MIDIRMFContent *>::iterator ptr(g_list.begin());
			ptr != g_list.end(); ptr++) {
		(*ptr)->Stop();
	}
}

static void
OverloadHook(
	void *)
{
FUNC2();
	status_t locked = g_listLock.LockWithTimeout(1000000LL);
	if (locked < 0) {
		//	this should not happen, but you can't be too careful
		//	with 3ed party code involved
		fprintf(stderr, "uh-oh! deadlock in OverloadHook()\n");
	}
	_bae_overload = false;
	//	If we have 6 overloads, each within 3 seconds of the previous one,
	//	we're overloading the system with MIDI and must stop it.
	bigtime_t now = system_time();
	if (now-g_lastOverload < 3000000LL) {
		if (g_overloadCnt++ > 6) {
			puts("MIDI overload detected; stop it!");
			stop_midi();
			g_overloadCnt = 0;
#if ALERT_ON_OVERLOAD
			BMessage msg_alert(TB_OPEN_ALERT);
			msg_alert.AddString("itemplate","Alerts/midi.txt");
			msg_alert.AddString("template","midioverload");
			TellTellBrowser(&msg_alert);
#endif
		}
	}
	else {
		g_overloadCnt = 1;
	}
	g_lastOverload = now;
	if (locked >= 0) g_listLock.Unlock();
}

MIDIRMFContentInstance::MIDIRMFContentInstance(
	MIDIRMFContent * parent,
	GHandler * h,
	const BMessage & msg) :
	ContentInstance(parent, h),
	m_playing(false)
{
FUNC();
	BAutolock lock(g_locker);
	m_parent = parent;
	if (g_refCount++ == 0) {
		_bae_overloadHook = OverloadHook;
		g_mixer = new BAEOutputMixer;
		int res = g_mixer->Open((void*)"/boot/beos/etc/synth/Patches.hsb", BAE_22K);
		if (res != 0) {
			fprintf(stderr, "BAE Open(): %d\n", res);
		}
	}
	ReadConfig(msg);
	if (m_autoStart) {
		parent->StartInstance(this, m_loop);
	}
}

static bool
read_bool(
	const BMessage & msg,
	const char * name,
	bool def)
{
FUNC2();
	bool ret = def;
	const char * b;
	int32 i;

	if (msg.FindBool(name, &ret)) {
		ret = def;
		if (!msg.FindString(name, &b) && b) {
			if (!strcasecmp(b, "") || !strncasecmp(b, "false", 5) ||
					!strncasecmp(b, "no", 2) || !strncasecmp(b, "off", 3) ||
					!strncasecmp(b, "0", 1)) {
				ret = false;
			}
			else {
				ret = true;
			}
		}
		else if (!msg.FindInt32(name, &i)) {
			ret = (i != 0);
		}
	}
	return ret;
}

void
MIDIRMFContentInstance::ReadConfig(
	const BMessage & msg)
{
FUNC();
	m_autoStart = read_bool(msg, "autostart", true);
	m_loop = read_bool(msg, "loop", false) || read_bool(msg, "looping", false);
}

MIDIRMFContentInstance::~MIDIRMFContentInstance()
{
FUNC();
	BAutolock lock(g_locker);
	m_parent->Remove(this);
	if (--g_refCount == 0) {
		delete g_mixer;
		g_mixer = 0;
		_bae_overloadHook = 0;
	}
}

status_t 
MIDIRMFContentInstance::GetSize(
	int32 *x,
	int32 *y,
	uint32 * outResizeFlags)
{
FUNC();
	*x = *y = 0;
	*outResizeFlags = 0;
	return B_OK;
}





MIDIRMFContent::MIDIRMFContent(
	void* handle,
	bool isRMF)
	: Content(handle)
{
FUNC();
	m_playingCount = 0;
	m_done = false;
	m_isRMF = isRMF;
	m_looping = false;
	m_song = 0;

	BAutolock lock(g_listLock);
	g_overloadCnt = 0;
	g_list.push_back(this);
}


MIDIRMFContent::~MIDIRMFContent()
{
FUNC();
	{
		BAutolock lock(g_listLock);
		list<MIDIRMFContent *>::iterator ptr(find(g_list.begin(), g_list.end(), this));
		if (ptr != g_list.end()) {
			g_list.erase(ptr);
		}
		g_overloadCnt = 0;
	}
	if (m_song != 0) fprintf(stderr, "~MIDIRMFContent(): m_song is not NULL\n");
	delete m_song;
}

ssize_t 
MIDIRMFContent::Feed(
	const void *buffer,
	ssize_t bufferLen,
	bool done)
{
FUNC();
	BAutolock lock(m_lock);
	m_data.Write(buffer, bufferLen);
	if (done) {
		m_done = true;
		if (m_playingCount) {
			Start();
		}
	}
	return bufferLen;
}

size_t 
MIDIRMFContent::GetMemoryUsage()
{
FUNC();
	return m_data.BufferLength();
}

status_t 
MIDIRMFContent::CreateInstance(
	ContentInstance **outInstance,
	GHandler *handler,
	const BMessage & msg)
{
FUNC();
	BAutolock lock(g_listLock);
	*outInstance = new MIDIRMFContentInstance(this, handler, msg);
	return B_OK;
}

void
MIDIRMFContent::Remove(
	MIDIRMFContentInstance * instance)
{
FUNC();
	BAutolock lock(m_lock);
	if (instance->m_playing && --m_playingCount == 0 && m_done) {
		Stop();
	}
}

void 
MIDIRMFContent::StartInstance(
	MIDIRMFContentInstance *instance,
	bool looping)
{
	BAutolock lock(m_lock);
	m_looping = m_looping || looping;
	m_playingCount++;
	if (m_done) {
		if (m_playingCount == 1) {
			Start();
		}
		else {
			m_song->SetLoopFlag(m_looping);	//	if it changed
		}
	}
	instance->m_playing = true;
}

void
MIDIRMFContent::Start()
{
FUNC();
	g_overloadCnt = 0;
	if (g_mixer == 0) {
		fprintf(stderr, "MIDIRMFContent::Start() but no Mixer?\n");
		return;
	}
	if (m_song != 0) {
		fprintf(stderr, "MIDIRMFContent::Start() but already have a m_song\n");
		return;
	}
	if (m_isRMF) {
		m_song = new BAERmfSong(g_mixer);
	}
	else {
		m_song = new BAEMidiSong(g_mixer);
	}
	int r = m_song->LoadFromMemory(m_data.Buffer(), m_data.BufferLength(), false);
	m_song->SetLoopFlag(m_looping);
	r = m_song->Start();
}

void
MIDIRMFContent::Stop()
{
FUNC();
	if (m_song == 0) {
		fprintf(stderr, "MIDIRMFContent::Stop() with no m_song?\n");
	}
	else {
		m_song->Stop();
	}
	delete m_song;
	m_song = 0;
}

// ----------------------- MIDIRMFContentFactory -----------------------

class MIDIRMFContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(
							BMessage* into)
	{
		// BE AWARE: Any changes you make to these identifiers should
		// also be made in the 'addattr' command in the makefile.
		into->AddString(S_CONTENT_MIME_TYPES, "audio/midi");
		into->AddString(S_CONTENT_MIME_TYPES, "audio/x-midi");
		into->AddString(S_CONTENT_EXTENSIONS, "midi");
		into->AddString(S_CONTENT_EXTENSIONS, "mid");
		into->AddString(S_CONTENT_MIME_TYPES, "audio/rmf");
		into->AddString(S_CONTENT_MIME_TYPES, "audio/x-rmf");
		into->AddString(S_CONTENT_EXTENSIONS, "rmf");
	}
	
	virtual Content* CreateContent(
							void* handle,
							const char* mime,
							const char* extension)
	{
		// This is kind-of hideous -- we really should be implementing
		// this as two different content factories, by the ContentManager
		// doesn't yet support that...  so try to figure out from the
		// mime type or extension what type this is.
		bool is_rmf = false;
		if ((mime && (!strcasecmp(mime, "audio/rmf") ||
					!strcasecmp(mime, "audio/x-rmf"))) ||
				(extension && !strcasecmp(extension, "rmf"))) {
			is_rmf = true;
		}
		return new MIDIRMFContent(handle, is_rmf);
	}
};

extern "C" _EXPORT ContentFactory*
make_nth_content(
	int32 n,
	image_id you,
	uint32 flags, ...)
{
	if (n == 0) {
		return new MIDIRMFContentFactory;
	}
	return 0;
}
