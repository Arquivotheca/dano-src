
//	question:
//	Where do we document netron-specific settings?
//	Can't really do it in the main settings file, because
//	then other customers will see it, too.

#include <Messenger.h>
#include <Screen.h>
#include <Window.h>
#include <View.h>
#include <Button.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <Input.h>
#include <TextView.h>
#include <Alert.h>
#include <MessageFilter.h>
#include <Entry.h>

#include "ValidateInterface.h"
#include "microphone.h"
#include "String.h"
#include <MediaTrack.h>
#include <MediaFile.h>

ValidateInterface * g_vi;

const uint32 msgTestPassed = 'pass';
const uint32 msgTestFailed = 'fail';

// ---------------------------------------------------------------------------
// Utility for making big alerts
// ---------------------------------------------------------------------------

static void
SetLargeAlertFont(BView* view)
{
	// I guess the monitors sit a long way away from the factory technicians
	// Sony wanted 18 point fonts.
	view->SetFontSize(18.0);
	BTextView* textView = dynamic_cast<BTextView *>(view);
	if (textView != NULL) {
		BFont font(be_plain_font);
		font.SetSize(18.0);
		textView->SetFontAndColor(&font);
		BRect bounds = textView->Bounds();
		// resize textview so that all lines are visible.
		float height = textView->CountLines() * textView->LineHeight();
		float diffY = height - bounds.bottom;
		textView->ResizeTo(bounds.Width(), bounds.Height());
		textView->Window()->ResizeBy(0., diffY);
	}
	for (int i = 0; i < view->CountChildren(); i++) {
		SetLargeAlertFont(view->ChildAt(i));
	}
}

// ---------------------------------------------------------------------------

static void
SetLargeAlertFont(BWindow* aWindow)
{
	if (aWindow->Lock()) {
		aWindow->BeginViewTransaction();
		for (int i = 0; i < aWindow->CountChildren(); i++) {
			SetLargeAlertFont(aWindow->ChildAt(i));
		}
		aWindow->EndViewTransaction();
		aWindow->Unlock();
	}
}

// ---------------------------------------------------------------------------
// Sound utilities for add-on
// ---------------------------------------------------------------------------

typedef void (*PlayHook)(void*, void*, size_t, const media_raw_audio_format&);

class SoundControl {
public:
					SoundControl(const BString& setting, PlayHook hook);
					~SoundControl();
	void			Play();
	void			Stop();
	BString&		SoundName() { return fSoundName; }
	
private:
	BMediaFile*		fMediaFile;
	BSoundPlayer*	fPlayer;
	BString			fSoundName;
};

// ---------------------------------------------------------------------------

float phase = 0;
float phase_inc = 2*M_PI*1000.0/44100.0;
BMediaTrack* track;
bool trackDone = false;

// ---------------------------------------------------------------------------

void
play_func(void* /* cookie */, void* buf, size_t size, const media_raw_audio_format& /* fmt */)
{
	if (track != 0) {
		int64 count = 0;
		if (track->ReadFrames(buf, &count) != B_OK) {
			// Just try to start over... since they can cancel out at any time, this won't hurt anything
			// this allows us to loop even with bad files (like SonySoundTest07.mp3)
			int64 f = 0;
			track->SeekToFrame(&f);
			memset(buf, 0, size);
		}
		return;
	}
	int16 * s = (int16 *)buf;
	int cnt = size/4;
	while (cnt-- > 0) {
		int16 v = int16(32000 * sin(phase));
		phase += phase_inc;
		if (phase >= 2*M_PI) phase -= 2*M_PI;
		s[0] = v;
		s[1] = v;
		s += 2;
	}
}

// ---------------------------------------------------------------------------

SoundControl::SoundControl(const BString& setting, PlayHook hook)
{
	fMediaFile = NULL;
	fPlayer = NULL;
	track = NULL;
	trackDone = false;

	// See if we can open the file specified for this test
	char buf[200] = "";
	const char* fileName = g_vi->get_setting(setting.String(), buf, 200);
	if (fileName != NULL) {
		// if we don't have a full path, prepend our current directory back into fileName
		if (fileName[0] != '/') {
			BString fullName(g_vi->get_current_directory());
			fullName += "/";
			fullName += fileName;
			strcpy(buf, fullName.String());
			fileName = buf;
		}
		entry_ref ref;
		bool file_ok = true;
		if (get_ref_for_path(fileName, &ref) == B_OK) {
			fMediaFile = new BMediaFile(&ref);
			track = fMediaFile->TrackAt(0);
			media_format fmt;
			fmt.type = B_MEDIA_RAW_AUDIO;
			if ((track->DecodedFormat(&fmt) < 0) || (fmt.type != B_MEDIA_RAW_AUDIO)) {
				file_ok = false;
			}
		}
		else {
			file_ok = false;
		}
		
		if (file_ok == false) {
			fprintf(stderr, "%s: not recognized\n", fileName);
			delete fMediaFile;
			track = 0;
			fileName = NULL;
		}
	}

	if (fileName) {
		fSoundName = fileName;
	}
	else {
		fSoundName = "tone";
	}

	media_raw_audio_format fmt(media_raw_audio_format::wildcard);
	if (track != 0) {
		media_format mfmt;
		track->DecodedFormat(&mfmt);
		fmt = mfmt.u.raw_audio;
	}
	else {
		fmt.frame_rate = 44100.0;
		fmt.channel_count = 0x2;
		fmt.format = 0x2;
		fmt.buffer_size = 2048;
	}
	
	fPlayer = new BSoundPlayer(&fmt, "Test Sound", hook);
}

// ---------------------------------------------------------------------------

SoundControl::~SoundControl()
{
	delete fMediaFile;
	delete fPlayer;
}

// ---------------------------------------------------------------------------

void
SoundControl::Play()
{
	fPlayer->SetHasData(true);
	fPlayer->Start();
}

// ---------------------------------------------------------------------------

void
SoundControl::Stop()
{
	fPlayer->Stop();
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// Email LED control - See Netron ECS
const int8 kEmailCode = 0x0c;
const int8 kEmailOn = 0x01;
const int8 kEmailOff = 0x07;

// Power LED control
const uint8 kPowerStandbyPacket[] = { 0x81, 0x01, 0x00, 0x06, 0xff };
const int32	kPowerStandbySize = sizeof(kPowerStandbyPacket);
const uint8 kPowerOnPacket[] = { 0x81, 0x01, 0x00, 0x02, 0xff };
const int32	kPowerOnSize = sizeof(kPowerOnPacket);

// Raw front panel key codes
enum KeyCode {
	POWER_KEY       = 0,
	EMAIL_KEY       = 1,
	MEDIA_KEY       = 2,
	WEB_KEY         = 3,
	VOLUME_UP_KEY   = 4,
	VOLUME_DOWN_KEY = 5,
};

// ---------------------------------------------------------------------------
// Filter for vendor window to handle alt-q
// ---------------------------------------------------------------------------

class AbortFilter : public BMessageFilter {
public:
							AbortFilter(BWindow* inWindow);
	virtual	filter_result	Filter(BMessage* message, BHandler** target);

private:
	BWindow*				fWindow;
};

AbortFilter::AbortFilter(BWindow* inWindow)
			: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN)
{
	fWindow = inWindow;
}

filter_result
AbortFilter::Filter(BMessage* message, BHandler** /* target */)
{
	filter_result result = B_DISPATCH_MESSAGE;

	if (message->what == B_KEY_DOWN) {
		int32 raw_char = message->FindInt32("raw_char");
		uint32 modifiers = message->FindInt32("modifiers");
		
		if ((modifiers & B_COMMAND_KEY) && (raw_char == 'q' || raw_char == 'Q')) {
			fWindow->PostMessage(B_QUIT_REQUESTED);
			result = B_SKIP_MESSAGE;
		}
	}
	return result;
}

// ---------------------------------------------------------------------------
// V2MouseView - tests for any (or all) mouse buttons
// ---------------------------------------------------------------------------

class V2MouseView : public BView
{
public:
					V2MouseView(const BRect & area);
	virtual void 	MouseDown(BPoint where);
	virtual void 	MessageReceived(BMessage * msg);
	virtual void 	Draw(BRect area);

	virtual void 	AttachedToWindow();
	virtual void 	DetachedFromWindow();
private:
	uint32 			m_buttons;
	uint32 			m_lookFor;
	bool   			m_anyButton;
	enum 			{kAllButtons = 0xf};
};


V2MouseView::V2MouseView(const BRect &area) :
	BView(area, "MouseTest", B_FOLLOW_ALL, B_WILL_DRAW)
{
	m_lookFor = g_vi->get_setting_value("netron.testmouse.buttons", kAllButtons);
	// Any buttons not in m_lookFor are going to be treated as already pressed
	m_buttons = (~m_lookFor)&kAllButtons;
	m_anyButton = (bool) g_vi->get_setting_value("netron.testmouse.passonany", 0);
	BRect r2(area);
	r2.InsetBy(10, 10);
	r2.left = r2.right-90;
	r2.top = r2.bottom-30;
	BButton * btn = new BButton(r2, "fail", "Fail", new BMessage(msgTestFailed));
	AddChild(btn);
	btn->SetFontSize(18.0);
}

void 
V2MouseView::MouseDown(BPoint)
{
	BMessage * m = Window()->CurrentMessage();
	uint32 buttons = 0;
	if (m->FindInt32("buttons", (int32 *)&buttons) != B_OK)
	{
		fprintf(stderr, "buttons not found in message\n");
	}
	uint32 old = m_buttons;
	m_buttons = (m_buttons | buttons);
	if (m_buttons != old) Invalidate();
}

void 
V2MouseView::MessageReceived(BMessage *msg)
{
	if (msg->what == B_MOUSE_WHEEL_CHANGED)
	{
		m_buttons |= 0x8;
		Invalidate();
	}
	else
	{
		BView::MessageReceived(msg);
	}
}

void V2MouseView::AttachedToWindow()
{
	BView::AttachedToWindow();
	this->MakeFocus(true);
}

void V2MouseView::DetachedFromWindow()
{
	this->MakeFocus(false);
	BView::DetachedFromWindow();
}


void 
V2MouseView::Draw(BRect)
{
	SetFontSize(12.);
	BRect buttons[4] = {
		BRect(30.f,90.f,99.f,140.f),
		BRect(230.f,90.f,299.f,140.f),
		BRect(130.f,90.f,199.f,140.f),
		BRect(30.f,190.f,299.f,240.f),
	};
	const char * names[4] = {
		"left",
		"right",
		"middle",
		"scroll",
	};
	for (int ix=0; ix<4; ix++)
	{
		if (m_buttons & (1<<ix))
		{
			SetLowColor(100,180,100);
			FillRect(buttons[ix], B_SOLID_LOW);
		}
		else
		{
			SetLowColor(255, 255, 255);
		}
		StrokeRect(buttons[ix]);
		DrawString(names[ix], buttons[ix].OffsetByCopy(10,-10).LeftBottom());
	}
	SetFontSize(18.);
	SetLowColor(255, 255, 255);
	if (m_anyButton) 
	{
		DrawString("Please press/scroll any button while cursor is within this window", BPoint(10., 40.));
		DrawString("Test will succeed with any button press...", BPoint(10., 64.));
	}
	else 
	{
		DrawString("Please press/scroll all buttons while cursor is within this window", BPoint(10., 40.));
	}
	
	if (m_anyButton && m_buttons != 0) 
	{
		Sync();
		// snooze for just a little so they can see that the correct box was colored
		snooze(250000);
		(Window())->PostMessage(msgTestPassed);
	}
	else if (m_buttons == kAllButtons)
	{
		(Window())->PostMessage(msgTestPassed);
	}
}

// ---------------------------------------------------------------------------
// V2NetronDisplayView - utility view for interaction with the input server
// ---------------------------------------------------------------------------

class V2NetronDisplayView : public BView
{
public:
						V2NetronDisplayView(const BRect& area);
		virtual			~V2NetronDisplayView();
		virtual void 	AttachedToWindow();
		virtual void 	DetachedFromWindow();

protected:
		BInputDevice*	Device()					{ return m_device; }
		void 			SendMessage(uint32 code);
		void 			SendMessage(BMessage* msg);

private:
		BInputDevice*	m_device;


};

// ---------------------------------------------------------------------------

V2NetronDisplayView::V2NetronDisplayView(const BRect& area) 
					: BView(area, "InputServerTestView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	m_device = find_input_device("NetronDisplay");
	if (m_device == NULL) {
		fprintf(stderr, "NetronDisplay -- input device not found!\n");
	}
}

// ---------------------------------------------------------------------------

V2NetronDisplayView::~V2NetronDisplayView()
{
	delete m_device;
}

// ---------------------------------------------------------------------------

void
V2NetronDisplayView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SendMessage('INIT');
}

// ---------------------------------------------------------------------------

void
V2NetronDisplayView::DetachedFromWindow()
{
	SendMessage('UNIN');
	BView::DetachedFromWindow();
}

// ---------------------------------------------------------------------------

void
V2NetronDisplayView::SendMessage(BMessage* msg)
{
	if (Device() == NULL || Window() == NULL) {
		fprintf(stderr, "Netron: SendMessage(%.4s) with no window or device\n", (char *)&msg->what);
		return;
	}

	BMessage reply('repl');
	reply.AddInt32("request_type", msg->what);
	msg->AddMessage("reply_message", &reply);
	msg->AddMessenger("reply_messenger", BMessenger(this, Window()));
	status_t err = m_device->Control('NETR', msg);
	if (err != B_OK) {
		fprintf(stderr, "messaging error: %s\n", strerror(err));
	}
}

// ---------------------------------------------------------------------------

void
V2NetronDisplayView::SendMessage(uint32 code)
{
	BMessage msg(code);
	this->SendMessage(&msg);
}

// ---------------------------------------------------------------------------
// V2FrontButtonView - tests the buttons on front of unit
// ---------------------------------------------------------------------------

class V2FrontButtonView : public V2NetronDisplayView
{
public:
					V2FrontButtonView(const BRect & area);
	virtual			~V2FrontButtonView();
	virtual void	MessageReceived(BMessage * msg);
	virtual void	Draw(BRect area);
private:
	uint32			m_buttons;
	uint32			m_lookFor;
	enum			{kAllButtons=0x1f};
};


V2FrontButtonView::V2FrontButtonView(const BRect &area) : V2NetronDisplayView(area)
{
	m_buttons = 0;
	m_lookFor = g_vi->get_setting_value("netron.testbuttons.buttons", kAllButtons);
	// Any buttons not in m_lookFor are going to be treated as already pressed
	m_buttons = (~m_lookFor)&kAllButtons;
	BRect r2(area);
	r2.InsetBy(10, 10);
	r2.left = r2.right-90;
	r2.top = r2.bottom-30;
	BButton * btn = new BButton(r2, "fail", "Fail", new BMessage(msgTestFailed));
	AddChild(btn);
	btn->SetFontSize(18.0);
}

V2FrontButtonView::~V2FrontButtonView()
{
}

void 
V2FrontButtonView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'repl':
			fprintf(stderr, "-->  Reply message\n");
			break;
		case 'KEYP': 
		{
			bool keyDown = msg->FindBool("down");
			// only trigger on key-up
			if (keyDown == false) {
				uint8 code = 0;
				msg->FindInt8("keycode", (int8 *)&code);
				fprintf(stderr, "front-panel key message code 0x%02x\n", code);
				m_buttons |= (1<<(code-1));
				Invalidate();
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

void 
V2FrontButtonView::Draw(BRect)
{
	if (Device() == NULL)
	{
		Window()->PostMessage(msgTestFailed);
	}
	SetFontSize(18.);
	BRect buttons[5] = {
		BRect(350.f,100.f,420.f,150.f),
		BRect(280.f,100.f,340.f,150.f),
		BRect(190.f,100.f,260.f,150.f),
		BRect(110.f,100.f,180.f,150.f),
		BRect(30.f,100.f,100.f,150.f),
	};
	const char * names[5] = {
		"mail", "Villa", "web", "+", "-"
	};
	SetHighColor(0, 0, 0);
	SetDrawingMode(B_OP_COPY);
	for (int ix=0; ix<5; ix++)
	{
		if (m_buttons & (1<<ix))
		{
			SetLowColor(100, 100, 200);
			FillRect(buttons[ix], B_SOLID_LOW);
		}
		else
		{
			SetLowColor(255, 255, 255);
		}
		StrokeRect(buttons[ix]);
		DrawString(names[ix], buttons[ix].OffsetByCopy(10, -10).LeftBottom());
	}
	SetLowColor(255, 255, 255);
	DrawString("Please push front-panel buttons", BPoint(10., 40.));
	if (m_buttons == kAllButtons)
	{
		(Window())->PostMessage(msgTestPassed);
	}
}

// ---------------------------------------------------------------------------
// AGVibrationView - plays an MP3 file with an all-white screen
// (except for the pass/fail buttons)
// ---------------------------------------------------------------------------

class AGVibrationView : public BView
{
public:
					AGVibrationView(const BRect & area);
	virtual void	AttachedToWindow();
	virtual void 	DetachedFromWindow();
	
private:
	SoundControl*	fSound;
};

// ---------------------------------------------------------------------------

AGVibrationView::AGVibrationView(const BRect & area)
				: 	BView(area, "ag_vibration", B_FOLLOW_ALL, B_WILL_DRAW)
{
	// While nothing will fit in the current window size, this will all
	// fit when the window is resized in AttachedToWindow
	
	BRect r2 = BScreen().Frame();
	r2.InsetBy(10, 10);
	r2.left = r2.right-90;
	r2.top = r2.bottom-30;
	BButton* failButton = new BButton(r2, "fail", "Fail", new BMessage(msgTestFailed));
	AddChild(failButton);
	failButton->SetFontSize(18.0);
	r2.OffsetTo(10, r2.top);
	BButton* okButton = new BButton(r2, "ok", "Pass", new BMessage(msgTestPassed));
	AddChild(okButton);
	okButton->SetFontSize(18.0);
	okButton->MakeDefault(true);
}

// ---------------------------------------------------------------------------

void
AGVibrationView::AttachedToWindow()
{
	// Start the sound - and make sure the window fills the entire screen
	fSound = new SoundControl("netron.agvibration.filename", &play_func);
	fSound->Play();

	BRect r = BScreen().Frame();
	this->Window()->MoveTo(BPoint(0., 0.));
	this->Window()->ResizeTo(r.right-r.left, r.bottom-r.top);	 

	BView::AttachedToWindow();

}

// ---------------------------------------------------------------------------

void
AGVibrationView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
	fSound->Stop();
	delete fSound;
}

// ---------------------------------------------------------------------------
// V2EmailLEDView - tests the E-mail LED turning on/off
// ---------------------------------------------------------------------------

class V2EmailLEDView : public V2NetronDisplayView
{
public:
					V2EmailLEDView(const BRect & area);
	virtual void	AttachedToWindow();
	virtual void 	DetachedFromWindow();
	
	virtual void	MessageReceived(BMessage * msg);
	virtual void	Draw(BRect area);

private:
	bool			m_key_pressed;
};

// ---------------------------------------------------------------------------

V2EmailLEDView::V2EmailLEDView(const BRect &area) : V2NetronDisplayView(area)
{
	m_key_pressed = false;

	BRect r2(area);
	r2.InsetBy(10, 10);
	r2.left = r2.right-90;
	r2.top = r2.bottom-30;
	BButton* failButton = new BButton(r2, "fail", "Fail", new BMessage(msgTestFailed));
	AddChild(failButton);
	failButton->SetFontSize(18.0);
	r2.OffsetTo(10, r2.top);
	BButton* okButton = new BButton(r2, "ok", "Pass", new BMessage(msgTestPassed));
	AddChild(okButton);
	okButton->SetFontSize(18.0);
	okButton->MakeDefault(true);
}

// ---------------------------------------------------------------------------

void
V2EmailLEDView::AttachedToWindow()
{
	V2NetronDisplayView::AttachedToWindow();
	
	// Send message to light email button after device is initialized
	BMessage message('SETR');
	message.AddInt8("reg", kEmailCode);
	message.AddInt8("value", kEmailOn);
	this->SendMessage(&message);
}

// ---------------------------------------------------------------------------

void
V2EmailLEDView::DetachedFromWindow()
{	
	BMessage message('SETR');
	message.AddInt8("reg", kEmailCode);
	message.AddInt8("value", kEmailOff);
	this->SendMessage(&message);

	V2NetronDisplayView::DetachedFromWindow();
}


// ---------------------------------------------------------------------------

void 
V2EmailLEDView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'repl':
			break;
		case 'KEYP':
		{
			// Check for key-up of email key
			bool keyDown = msg->FindBool("down");
			int8 code = 0;
			msg->FindInt8("keycode", &code);
			if (keyDown == false && code == EMAIL_KEY) {
				m_key_pressed = true;
				Invalidate();
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

// ---------------------------------------------------------------------------

void 
V2EmailLEDView::Draw(BRect)
{
	if (Device() == NULL) {
		Window()->PostMessage(msgTestFailed);
	}
	SetFontSize(18.);
	SetHighColor(0, 0, 0);
	SetDrawingMode(B_OP_COPY);
	SetLowColor(255, 255, 255);
	DrawString("Please verify that E-mail LED is now on.", BPoint(10., 40.));
	DrawString("Click 'Pass' button if LED is on, otherwise click 'Fail' button.", BPoint(10., 64.));
	
	if (m_key_pressed) {
		(Window())->PostMessage(msgTestPassed);
	}	
}

// ---------------------------------------------------------------------------
// V2PowerLEDView - tests the power button switching to standby mode
// ---------------------------------------------------------------------------

class V2PowerLEDView : public V2NetronDisplayView
{
public:
					V2PowerLEDView(const BRect & area);

	virtual void	AttachedToWindow();
	
	virtual void	MessageReceived(BMessage * msg);
	virtual void	Draw(BRect area);

private:
	void			SendPowerMessage(const uint8* packet, int32 size);
	static status_t	TriggerStandby(void* arg);
	
	int32			m_state;
	enum			{ kStart = 1, kStandby = 2, kEnd = 3 };
	BButton*		m_okButton;
};

// ---------------------------------------------------------------------------



V2PowerLEDView::V2PowerLEDView(const BRect &area) : V2NetronDisplayView(area)
{
	m_state = kStart;
	
	BRect r2(area);
	r2.InsetBy(10, 10);
	r2.left = r2.right-90;
	r2.top = r2.bottom-30;
	BButton * btn = new BButton(r2, "fail", "Fail", new BMessage(msgTestFailed));
	AddChild(btn);
	btn->SetFontSize(18.0);
	r2.OffsetTo(10, r2.top);
	m_okButton = new BButton(r2, "ok", "Pass", new BMessage(msgTestPassed));
	m_okButton->SetEnabled(false);
	AddChild(m_okButton);
	m_okButton->SetFontSize(18.0);
}

// ---------------------------------------------------------------------------

void
V2PowerLEDView::AttachedToWindow()
{
	V2NetronDisplayView::AttachedToWindow();

	thread_id thread = spawn_thread(TriggerStandby, "standby_trigger", B_NORMAL_PRIORITY, this);
	resume_thread(thread);
}

// ---------------------------------------------------------------------------

status_t 
V2PowerLEDView::TriggerStandby(void* arg)
{
	V2PowerLEDView* myView = (V2PowerLEDView*) arg;

	// snooze for about 1 second and then trigger the power to switch to standby	
	snooze(1250000);
	
	BMessage msg('doit');
	myView->Window()->PostMessage(&msg, myView);
	
	return B_OK;
}

// ---------------------------------------------------------------------------

void
V2PowerLEDView::SendPowerMessage(const uint8* packet, int32 size)
{
	// Send message to switch power mode
	BMessage message('RAWC');
	message.AddData("command", B_ANY_TYPE, packet, size);
	message.AddInt32("replylen", 4);
	this->SendMessage(&message);
}

// ---------------------------------------------------------------------------

void 
V2PowerLEDView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'repl':
			break;
		case 'doit':
			this->SendPowerMessage(kPowerStandbyPacket, kPowerStandbySize);
			m_state = kStandby;
			m_okButton->SetEnabled(true);
			m_okButton->MakeDefault(true);
			Invalidate();
			break;		
		case 'KEYP':
		{
			// we'll take any key to power back up
			// but we still want the key-up rather than the key-down
			bool keyDown = msg->FindBool("down");
			if (keyDown == false) {
				this->SendPowerMessage(kPowerOnPacket, kPowerOnSize);
				m_state = kEnd;	
				Invalidate();
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

// ---------------------------------------------------------------------------

void 
V2PowerLEDView::Draw(BRect)
{
	if (Device() == NULL) {
		Window()->PostMessage(msgTestFailed);
	}
	SetFontSize(18.);
	SetHighColor(0, 0, 0);
	SetDrawingMode(B_OP_COPY);
	SetLowColor(255, 255, 255);
	if (m_state == kStart) {
		DrawString("Testing Stand-by mode in 1 second...", BPoint(10., 40.));	
	}
	else if (m_state == kStandby) {
		DrawString("Display should be off, if you can see this message, click 'Fail' button.", BPoint(10., 40.));
	}
	else if (m_state == kEnd) {
		DrawString("Please verify that stand-by mode worked.", BPoint(10., 30.));
		DrawString("Click 'Pass' button if stand-by mode worked, otherwise click 'Fail' button.", BPoint(10., 64.));
	}
}

// ---------------------------------------------------------------------------
// V2BlockDeviceView - tests the USB/MemoryStick
// ---------------------------------------------------------------------------

class V2BlockDeviceView : public BView
{
public:
		V2BlockDeviceView(const BRect & area);
		~V2BlockDeviceView();
		void MessageReceived(BMessage * msg);
		void Draw(BRect area);
		void DetachedFromWindow();
		virtual void AttachedToWindow();

		static status_t poller_thread(void *);
		void DoPoll();

protected:
		enum {
			MAX_DEVICES = 10
		};
		int32 m_deviceCount;
		const char * m_devices[MAX_DEVICES];
		const char * m_userNames[MAX_DEVICES];
		bool m_gotThem[MAX_DEVICES];

private:
		sem_id m_sem;
		thread_id m_thread;
		BMessenger fMessenger;
};


V2BlockDeviceView::V2BlockDeviceView(const BRect &area) :
	BView(area, "blockdevices", B_FOLLOW_ALL, B_WILL_DRAW)
{
	BRect r2(area);
	r2.InsetBy(10, 10);
	r2.left = r2.right-90;
	r2.top = r2.bottom-30;
	BButton * btn = new BButton(r2, "fail", "Fail", new BMessage(msgTestFailed));
	AddChild(btn);
	btn->SetFontSize(18.0);
	m_sem = create_sem(0, "poller_sleep");
	
	if( m_sem < B_OK )
	{
		BString errMsg("Error creating semaphore, Test will not be done\n\nError is: ");
		
		errMsg << m_sem;
		errMsg << " (";
		errMsg += strerror(m_sem);
		errMsg += ")";
		BAlert* alert = new BAlert("Something bad happened", errMsg.String(), "Dang");
		SetLargeAlertFont(alert);
		alert->Go();
	}
	else
	{
		m_thread = spawn_thread(poller_thread, "poller_thread", B_NORMAL_PRIORITY, this);
		resume_thread(m_thread);
	}
}

V2BlockDeviceView::~V2BlockDeviceView()
{
}

void 
V2BlockDeviceView::AttachedToWindow()
{
	fMessenger = BMessenger(this);
}


void
V2BlockDeviceView::DetachedFromWindow()
{
	delete_sem(m_sem);
	status_t s;
	wait_for_thread(m_thread, &s);
	BView::DetachedFromWindow();
}


status_t 
V2BlockDeviceView::poller_thread(void * arg)
{
	bigtime_t endTime = system_time() + 31000000LL;
	V2BlockDeviceView * bdv = (V2BlockDeviceView *)arg;
	while (acquire_sem_etc(bdv->m_sem, 1, B_RELATIVE_TIMEOUT, 500000LL) == B_TIMED_OUT)
	{
		bdv->DoPoll();
		if (system_time() > endTime)
		{
			BMessage msg('quit');
			bdv->Window()->PostMessage(&msg, bdv);
			break;
		}
	}
	return B_OK;
}

//	Sony explicitly only wanted to see that there was "something"
//	there; not checking for good partitions or anything.

void 
V2BlockDeviceView::DoPoll()
{
	bool alldone = true;

	for (int ix=0; ix<m_deviceCount; ix++)
	{
		bool donethis = m_gotThem[ix];
		alldone = alldone && donethis;
		if (!donethis)
		{
			int fd = open(m_devices[ix], O_RDONLY);
			if (fd >= 0)
			{
				char block[512];
				if (read(fd, block, 512) == 512)
				{
					fprintf(stderr, "%s: found (%s)\n", m_devices[ix], m_userNames[ix]);
					m_gotThem[ix] = true;
					
					BMessage msg('done');
					msg.AddInt32("index", ix);
					
					fMessenger.SendMessage(&msg);
				}
				close(fd);
			}
		}
	}

	if (alldone)
	{
		fMessenger.SendMessage('quit');
	}
}

void 
V2BlockDeviceView::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
	case 'quit': {
			bool alldone = true;
			for (int ix=0; alldone && (ix<m_deviceCount); ix++)
			{
				alldone = alldone && m_gotThem[ix];
			}
			fprintf(stderr, "block device test %s\n", alldone ? "OK" : "Failed");
			(Window())->PostMessage(alldone ? msgTestPassed : msgTestFailed);
		}
		break;
	case 'done':
		Invalidate();
		break;
	default:
		BView::MessageReceived(msg);
		break;
	}
}

void 
V2BlockDeviceView::Draw(BRect)
{
	SetFontSize(18.);
	SetDrawingMode(B_OP_COPY);
	DrawString("Please insert:", BPoint(10., 40.));
	BRect r(10.,100.,290.,140.);
	for (int ix=0; ix<m_deviceCount; ix++)
	{
		if (m_gotThem[ix])
		{
			SetLowColor(100, 180, 100);
			FillRect(r, B_SOLID_LOW);
		}
		else
		{
			SetLowColor(255, 255, 255);
		}
		StrokeRect(r);
		DrawString(m_userNames[ix], r.OffsetByCopy(10., 30.).LeftTop());
		r.OffsetBy(0., 50.);
	}
	SetLowColor(255, 255, 255);
}


// ---------------------------------------------------------------------------
// V2MemoryStickView - tests the MemoryStick
// ---------------------------------------------------------------------------

class V2MemoryStickView : public V2BlockDeviceView
{
public:
		V2MemoryStickView(const BRect& area);
};

// ---------------------------------------------------------------------------

V2MemoryStickView::V2MemoryStickView(const BRect& area) : V2BlockDeviceView(area)
{
	// Populate the devices with what we are looking for
	// (Only the MemoryStick)
	m_deviceCount = 0;
	m_devices[m_deviceCount] = "/dev/disk/memorystick/parallel/0/raw";	//	memorystick
	m_userNames[m_deviceCount] = "MemoryStick";
	m_gotThem[m_deviceCount] = false;
	m_deviceCount++;

}

// ---------------------------------------------------------------------------
// V2USBView - tests the USB ports
// ---------------------------------------------------------------------------

class V2USBView : public V2BlockDeviceView
{
public:
		V2USBView(const BRect& area);
};

// ---------------------------------------------------------------------------

V2USBView::V2USBView(const BRect& area) : V2BlockDeviceView(area)
{
	// Populate the devices with what we are looking for
	m_deviceCount = 0;
	m_devices[m_deviceCount] = "/dev/disk/scsi/0/0/0/raw";					//	USB disk 1
	m_userNames[m_deviceCount] = "USB Zip Disk 1";
	m_gotThem[m_deviceCount] = false;
	m_deviceCount++;
	m_devices[m_deviceCount] = "/dev/disk/scsi/0/1/0/raw";					//	USB disk 2
	m_userNames[m_deviceCount] = "USB Zip Disk 2";
	m_gotThem[m_deviceCount] = false;
	m_deviceCount++;
}

// ---------------------------------------------------------------------------
// V2MicrophoneView - tests the microphone
// ---------------------------------------------------------------------------

class V2MicrophoneView : public BView {
public:
					V2MicrophoneView(const BRect &area);

	virtual void 	AttachedToWindow();
	virtual void 	DetachedFromWindow();

	virtual void 	Draw(BRect area);

private:
	MicrophoneTest*	m_mic;
};

// ---------------------------------------------------------------------------

V2MicrophoneView::V2MicrophoneView(const BRect &area) : BView(area, "MicrophoneTest", B_FOLLOW_ALL, B_WILL_DRAW)
{
	BRect r2(area);
	r2.InsetBy(10, 10);
	r2.left = r2.right-90;
	r2.top = r2.bottom-30;
	BButton* failButton = new BButton(r2, "fail", "Fail", new BMessage(msgTestFailed));
	AddChild(failButton);
	failButton->SetFontSize(18.0);
}

// ---------------------------------------------------------------------------

void
V2MicrophoneView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BAlert* alert = new BAlert("", "Please insert microphone loop back connector.", "OK");
	SetLargeAlertFont(alert);
	alert->Go();
	m_mic = new MicrophoneTest;
	m_mic->Begin(new BMessenger(NULL, Window()), new BMessage(msgTestPassed));
}

// ---------------------------------------------------------------------------

void
V2MicrophoneView::DetachedFromWindow()
{
	delete m_mic;
	m_mic = NULL;
	BAlert* alert = new BAlert("", "Please remove microphone loop back connector.", "OK");
	SetLargeAlertFont(alert);
	alert->Go();
	
	BView::DetachedFromWindow();
}

// ---------------------------------------------------------------------------

void 
V2MicrophoneView::Draw(BRect r)
{
	BView::Draw(r);

	SetFontSize(18.);
	SetHighColor(0, 0, 0);
	SetDrawingMode(B_OP_COPY);
	SetLowColor(255, 255, 255);
	DrawString("Testing Microphone... (will pass automatically in about 1-2 seconds)", BPoint(10., 40.));
}

// ---------------------------------------------------------------------------
// alternative tests - just make a view to sit in a generic test window
// ---------------------------------------------------------------------------


BView*
make_mouse_button_view(const BRect &r, ValidateInterface* vi)
{
	g_vi = vi;
	BRect bounds(r);
	bounds.OffsetTo(0,0);
	return new V2MouseView(bounds);
}

BView*
make_usb_device_view(const BRect &r, ValidateInterface* vi)
{
	g_vi = vi;
	BRect bounds(r);
	bounds.OffsetTo(0,0);
	return new V2USBView(bounds);
}

BView*
make_memorystick_view(const BRect &r, ValidateInterface* vi)
{
	g_vi = vi;
	BRect bounds(r);
	bounds.OffsetTo(0,0);
	return new V2MemoryStickView(bounds);
}

BView*
make_microphone_view(const BRect &r, ValidateInterface* vi)
{
	g_vi = vi;
	BRect bounds(r);
	bounds.OffsetTo(0,0);
	return new V2MicrophoneView(bounds);
}

BView*
make_front_button_view(const BRect &r, ValidateInterface* vi)
{
	g_vi = vi;
	BRect bounds(r);
	bounds.OffsetTo(0,0);
	return new V2FrontButtonView(bounds);
}

BView*
make_email_led_view(const BRect &r, ValidateInterface* vi)
{
	g_vi = vi;
	BRect bounds(r);
	bounds.OffsetTo(0,0);
	return new V2EmailLEDView(bounds);
}

BView*
make_standby_led_view(const BRect &r, ValidateInterface* vi)
{
	g_vi = vi;
	BRect bounds(r);
	bounds.OffsetTo(0,0);
	return new V2PowerLEDView(bounds);
}

BView*
make_agvibration_view(const BRect &r, ValidateInterface* vi)
{
	g_vi = vi;
	BRect bounds(r);
	bounds.OffsetTo(0,0);
	return new AGVibrationView(bounds);
}

stage_t avail_stages[] = {
//	{ 1, "Netron Specific Tests", "netron", NULL, run_test },
	{ 1, "Mouse Buttons/Wheel Test", "netron.testmouse", NULL, make_mouse_button_view, 0, 0 },
	{ 2, "USB Ports Test", "netron.testusb", NULL, make_usb_device_view, 0, 0 },
	{ 3, "MemoryStick Test", "netron.testmemorystick", NULL, make_memorystick_view, 0, 0 },
	{ 4, "Microphone Test", "netron.testmicrophone", NULL, make_microphone_view, 0, 0 },
	{ 5, "Front Panel Buttons Test", "netron.testbuttons", NULL, make_front_button_view, 0, 0 },
	{ 6, "E-mail LED Test", "netron.testemail", NULL, make_email_led_view, 0, 0 },
	{ 7, "Stand-by LED/mode Test", "netron.teststandby", NULL, make_standby_led_view, 0, 0 },
	{ 8, "AG Vibration Test", "netron.agvibration", NULL, make_agvibration_view, 0, 0 },
	{ 0, NULL, NULL, NULL, NULL, 0, 0 }
};


// ---------------------------------------------------------------------------
//	MonitorInformationReceiver
//	A helper class to gather all the monitor information that we need to display
//	Because Control responds back to a BMessenger, we set up a BLooper 
//	to gather all the information we need from the device
//  (See GetVendorVersionInfo)
// ---------------------------------------------------------------------------

class MonitorInformationReceiver : public BLooper {
public:
					MonitorInformationReceiver();
	virtual			~MonitorInformationReceiver();
	
	void			GetInformation(BMessage* versionInfo);
	
	virtual	void	MessageReceived(BMessage* msg);

	bool			InitCheck();
	
private:
	void			SendMessage(int32 infoType);
	void			HandleMonitorData(BMessage* msg);

	enum {kModelName = 0, kSerialNumber, kProductionWeek, kProductionYear, kETI, kDoneState = kETI+1};
	
	BInputDevice*	fDevice;
	int32 			fState;
	BMessage*		fInformation;
	BString			fProductionWeek;
};

// ---------------------------------------------------------------------------

MonitorInformationReceiver::MonitorInformationReceiver()
{
	fState = kModelName;
	fDevice = find_input_device("NetronDisplay");
	if (fDevice == NULL) {
		fprintf(stderr, "NetronDisplay -- input device not found!\n");
	}
}

// ---------------------------------------------------------------------------

MonitorInformationReceiver::~MonitorInformationReceiver()
{
	delete fDevice;
}

// ---------------------------------------------------------------------------

bool
MonitorInformationReceiver::InitCheck()
{
	return fDevice != NULL;
}

// ---------------------------------------------------------------------------

void
MonitorInformationReceiver::GetInformation(BMessage* versionInfo)
{
	fInformation = versionInfo;
	this->SendMessage(fState);
}

// ---------------------------------------------------------------------------

static BString 
ConvertETIData(unsigned char* bytes)
{
	// ETI data gives total elapsed time of machine powered on (in 30 minute units)
	// For example:	000101H means 128.5 hours
	// We need to display in 10 digit hours + "." + 1 digit half hour
	// ie: 0000000128.5 

	// First, convert the bytes into an integer		
	uint32 time = bytes[0]<<16 | bytes[1]<<8 | bytes[2];
	
	// Set the 1/2 hour according to low bit
	uint32 halfhour = (time & 1) ? 5 : 0;
	
	// Convert 30 minute units into hours (losing bottom bit)
	time >>= 1;
	
	// Now we are ready to conver to ascii
	char buffer[64];
	sprintf(buffer, "%10.10ld.%1.1ld", time, halfhour);
	
	BString data = buffer;
	return data;
}

// kDataLength[i] gives length of data where i must correspond to kModelName -> kETI
static int32 kDataLength[] = {10, 8, 2, 4, 3};

void
MonitorInformationReceiver::HandleMonitorData(BMessage* msg)
{
	// For the format of the returned information - see ECS pg 20
	// The bytes are split into a nibble per byte
	// upper nibble byte 1, lower nibble byte 1, ... upper nibble byte n, lower nibble byte n
	// For each message - convert to bytes and then interprete the data

	BString data;	
	int32 numNibbles;
	const unsigned char* nibbleData = NULL;
	status_t err = msg->FindData("data", B_ANY_TYPE, (const void**) &nibbleData, &numNibbles);

	// If we didn't get the correct information, take care of the error condition
	// otherwise, convert the nibbles into bytes and terminate with a NULL
	if (err != B_OK || ((numNibbles >> 1) != kDataLength[fState])) {
		data = "<not available>";
	}
	else {	
		unsigned char bytes[64];
		for (int i = 0; i < kDataLength[fState]; i++) {
			int32 nib = i*2;
			bytes[i] = (nibbleData[nib]<<4) | nibbleData[nib+1];
		}
		bytes[kDataLength[fState]] = '\0';
		
		// just store away the ASCII bytes (unless we are working with ETI)
		if (fState != kETI) {
			data = (char*) bytes;
		}
		else {
			data = ConvertETIData(bytes);
		}
	}

	// Now build the final user visible string.
	// At this point, the data is just ASCII, so just blast a header and the data together
	// oh, and we have to worry about concating week and year so store away
	// the week until we come back through here with the year
	
	BString userInfo;
	bool complete = true;
	switch (fState) {
		case kModelName:
			userInfo = "Model Name: ";
			userInfo += data;
			break;
		case kSerialNumber:
			userInfo = "Serial #: ";
			userInfo += data;
			break;
		case kProductionWeek:
			fProductionWeek = data;
			complete = false;
			break;
		case kProductionYear:
			// now we can take the week and year
			userInfo = "Manufacture year/week: ";
			userInfo += data;
			userInfo += "/";
			userInfo += fProductionWeek;
			break;
		case kETI:
			userInfo = "ETI: ";
			userInfo += data;
			break;
		default:
			break;		
	}

	if (complete) {
		fInformation->AddString("info", userInfo.String());
	}
}

// ---------------------------------------------------------------------------

void 
MonitorInformationReceiver::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'repl':
			this->HandleMonitorData(msg);
			fState += 1;
			if (fState == kDoneState) {
				this->Quit();
			}
			else {
				this->SendMessage(fState);
			}
			break;
		default:
			BLooper::MessageReceived(msg);
			break;
	}
}

// ---------------------------------------------------------------------------

void
MonitorInformationReceiver::SendMessage(int32 infoType)
{
	BMessage msg;
	if (infoType == kETI) {
		msg.what = 'GETE';
	}
	else {
		msg.what = 'GETM';
		msg.AddInt8("subcode", infoType);
	}
	
	BMessage reply('repl');
	reply.AddInt32("request_type", msg.what);
	reply.AddInt32("replylen", 64);
	msg.AddMessage("reply_message", &reply);
	msg.AddMessenger("reply_messenger", BMessenger(this));
	status_t err = fDevice->Control('NETR', &msg);
	if (err != B_OK) {
		fprintf(stderr, "messaging error getting machine information - bailing out so we don't hang: %s\n", strerror(err));
		this->Quit();
	}
}

// ---------------------------------------------------------------------------

extern "C" BMessage* GetVendorVersionInfo()
{
	BMessage* versionInfo = new BMessage;
	MonitorInformationReceiver* receiver = new MonitorInformationReceiver;
	if (receiver->InitCheck()) {
		thread_id id = receiver->Run();
		receiver->GetInformation(versionInfo);
		status_t exitValue;
		status_t status = wait_for_thread(id, &exitValue);
		if (status != B_OK) {
			fprintf(stderr, "Internal error getting machine information - thread communication: %s\n", strerror(status));
		}
	}
	else {
		delete receiver;
	}
	return versionInfo;
}
