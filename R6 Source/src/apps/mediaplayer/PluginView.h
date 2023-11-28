#ifndef _PLUGIN_VIEW_H
#define _PLUGIN_VIEW_H

#include <String.h>
#include <View.h>

class MediaController;
class VideoView;
class TransportView;

class PluginView : public BView {
public:
	PluginView(BRect rect);
	void MessageReceived(BMessage *message);
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void OpenURL(const char *url);
	virtual void Draw(BRect rect);
	void SetProperty(const char *name, const char *value);

private:
	void SetupView();
	
	MediaController *fController;
	VideoView *fVideoView;
	TransportView *fTransportView;
	
	bool fDisplayError;

	// User parameters
	bool fShowTransport;
	BString fURL;
	bool fLoop;
	bool fAutoPlay;
	float fVolume;
};

#endif


