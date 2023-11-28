#include "PluginView.h"
#include "MediaPlayerApp.h"
#include "PlayerWindow.h"
#include "MiniTransportView.h"
#include "MediaController.h"
#include "MediaTrackController.h"
#include "VideoView.h"
#include "URL.h"
#include "PluginView.h"

#ifdef __GNUC
	_EXPORT class PluginView;
#else
	class _EXPORT PluginView;
#endif


PluginView::PluginView(BRect rect)
	: 	BView(rect, "media_player", B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_FRAME_EVENTS),
		fController(0),
		fVideoView(0),
		fTransportView(0),
		fDisplayError(false),
		fShowTransport(true),
		fLoop(false),
		fAutoPlay(true),
		fVolume(1.0)
{
}

void PluginView::AttachedToWindow()
{
	OpenURL(fURL.String());
}

void PluginView::DetachedFromWindow()
{
	fController->Close();
	BView::DetachedFromWindow();
}

void PluginView::Draw(BRect)
{
	if (fDisplayError) {
		BRect bounds(Bounds());
		SetHighColor(0, 0, 0);
		StrokeLine(BPoint(bounds.left, bounds.top), BPoint(bounds.right, bounds.bottom));
		StrokeLine(BPoint(bounds.left, bounds.bottom), BPoint(bounds.right, bounds.top));
	}
}

void PluginView::OpenURL(const char *urlString)
{
	status_t result = B_OK;
	BString failureDescription;

	URL url(urlString);
	if (url.IsValid())
		fController = MediaTrackController::Open(url, "", BMessenger(this), &result);
}

void PluginView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case kFileReady: {
			status_t error = message->FindInt32("be:error_code");
			if (error < 0) {
				fDisplayError = true;
				Invalidate();
			} else
				SetupView();
	
			break;
		}
	}
}

void PluginView::SetupView()
{
	if (fController->HasVideo()) {
		BPoint videoSize(fController->VideoSize().Width(), fController->VideoSize().Height());
		BRect videoRect(Bounds());
		if (fShowTransport)
			videoRect.bottom -= (kMinTransportHeight + 1);

		fVideoView = new VideoView(videoRect, videoSize, "video_view", B_FOLLOW_NONE);
		AddChild(fVideoView);
		fController->ConnectVideoOutput(fVideoView);
	}

	if (fShowTransport) {
		BRect transportRect(Bounds());
		transportRect.top = transportRect.bottom - kMinTransportHeight;
		fTransportView = new MiniTransportView(transportRect, B_FOLLOW_LEFT_RIGHT |
			B_FOLLOW_BOTTOM, false);
		AddChild(fTransportView);
		fTransportView->AttachToController(fController);
	}
	
	fController->SetAutoLoop(fLoop);
	fController->SetVolume(fVolume);
	if (fAutoPlay)
		fController->Play();

	if (fTransportView)
		fTransportView->SetEnabled(true);
}

void PluginView::SetProperty(const char *name, const char *value)
{
	if (strcasecmp(name, "src") == 0) {
		// SRC
		fURL = value;
	} else if (strcasecmp(name, "loop") == 0) {
		// LOOP
		fLoop = strcasecmp(value, "true") == 0;
	} else if (strcasecmp(name, "controller") == 0) {
		// CONTROLLER
		fShowTransport = strcasecmp(value, "true") == 0;
	} else if (strcasecmp(name, "autoplay") == 0) {	
		// AUTOPLAY
		fAutoPlay = strcasecmp(value, "true") == 0;
	} else if (strcasecmp(name, "volume") == 0) {
		// VOLUME
		float volume = atof(value);
		if (volume > 1)
			volume = 1;
		else if (volume < 0)
			volume = 0;

		fVolume = volume;
	}
}
