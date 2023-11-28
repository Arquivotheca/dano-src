#include "SettingsWindow.h"
#include "VideoRecorder.h"

#include <Box.h>
#include <Button.h>
#include <Slider.h>

// AudioJitter Slider

class AudioJitterSlider : public BSlider {
	public:
			AudioJitterSlider(BRect frame, BMessage* message);
			~AudioJitterSlider();
			
			//void DrawText();			
			virtual char *UpdateText() const;
	private:
		char string[64];
};

AudioJitterSlider::AudioJitterSlider(BRect frame, BMessage* message)
	: BSlider(frame, "AudioJitterSlider", "AudioJitter:", message,
	          0, 500000, B_BLOCK_THUMB, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP)
{
}

AudioJitterSlider::~AudioJitterSlider()
{
}

char*
AudioJitterSlider::UpdateText() const
{
	sprintf((char *)string, "%ld \xC2\xB5s", Value());
	return (char*)string;
}

SettingsWindow::SettingsWindow(BWindow *target,
                               ScalarValueSetting *audiojittersetting)
	: BWindow(BRect(100,100,300,200), "VideoRecorder Settings", B_MODAL_WINDOW,
		B_ASYNCHRONOUS_CONTROLS)
	, fTarget(target)
	, fAudioJitterSetting(audiojittersetting)
{
	BRect frame(Bounds());
	BBox* box = new BBox(frame, "bg", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	AddChild(box);
	box->SetFont(be_plain_font); 

	frame.bottom = 15;
	
	fAudioJitterSlider = new AudioJitterSlider(frame, NULL);
	fAudioJitterSlider->SetValue(fAudioJitterSetting->Value());
	box->AddChild(fAudioJitterSlider);
	
	BButton* btn = new BButton(BRect(0,0,50,10), "", "Done",
	                           new BMessage(msg_setup_done),
	                           B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	box->AddChild(btn);
	btn->MoveTo(Bounds().Width() - 60, Bounds().bottom - 10 - btn->Frame().Height());
	
	MoveTo(target->Frame().LeftTop() + BPoint(20, 250));
}

void 
SettingsWindow::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case msg_setup_done: {
			fAudioJitterSetting->ValueChanged(fAudioJitterSlider->Value());
			fTarget->PostMessage(message);
			Lock();
			Quit();
		} break;
			
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

bool 
SettingsWindow::QuitRequested()
{
	return true;
}

