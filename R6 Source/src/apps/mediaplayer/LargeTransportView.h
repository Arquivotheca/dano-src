#ifndef __LARGE_TRANSPORTVIEW_H__
#define __LARGE_TRANSPORTVIEW_H__

#include "TransportView.h"

class TVolumeSlider;
class TransportButton;
class PlayPauseButton;

class LargeTransportView : public TransportView {
public:

	LargeTransportView(BRect rect, uint32 resizingMode);
	virtual void SetEnabled(bool enabled);

protected:
	virtual void AttachToController(MediaController*);	
	virtual void UpdateCommon();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage*);

	virtual void UpdateButtons();

private:
	TransportButton *fStopButton;
	TransportButton	*fFastForwardButton;
	TransportButton *fRewindButton;
	
	bigtime_t lastVolumeUpdate;
	
	typedef TransportView _inherited;
};

#endif
