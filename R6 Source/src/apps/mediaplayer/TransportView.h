#ifndef __TRANSPORT_VIEW__
#define __TRANSPORT_VIEW__

#include <Bitmap.h>
#include <Box.h>

#include "MultiThumbMediaSlider.h"
#include "KnobSwitch.h"

const float kMaxTransportHeight = 60;
const float kMinTransportHeight = 26;


const uint32 kBeginning = 'begn';
const uint32 kRewind = 'rwnd';
const uint32 kRecord = 'rcrd';
const uint32 kPlayPause = 'plap';
const uint32 kFastForward = 'ffwd';
const uint32 kDoneSkipping = 'dnff';
const uint32 kEnd = 'endd';
const uint32 kScrub = 'scrb';
const uint32 kStartScrub = 'stsc';
const uint32 kStopScrub = 'spsc';
const uint32 kDoneScrubbing = 'dscr';
const uint32 kInPointChanged = 'kinp';
const uint32 kOutPointChanged = 'koup';
const uint32 kVolumeChanged = 'volc';
const uint32 kVolumeMenuChange = 'mvol';

//const BPoint kPlayButtonSize(40, 20);

class ProgressBar;
class MediaController;
class TransportButton;
class PlayPauseButton;
class TMultiThumbMediaSlider;
class TVolumeSlider;
class BMessageRunner;

class TransportView : public BBox {
public:
	TransportView(BRect rect, uint32 resizingMode);
	virtual void SetEnabled(bool enabled);

	MediaController *Controller() const;
	
	virtual void AttachToController(MediaController*);	

protected:
	virtual void MessageReceived(BMessage*);
	virtual void Pulse();
	virtual void MouseDown(BPoint);
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
		
	virtual void UpdateButtons();
		// called after any interaction with the buttons and
		// from pulse
		// forces the buttons to match current state

	virtual void UpdateCommon();

	PlayPauseButton *fPlayButton;
	TMultiThumbMediaSlider *fTimeSlider;
	ProgressBar *fBufferStatus;
	TVolumeSlider *fVolumeSlider;

private:
	bigtime_t fLastPosition;
	MediaController *fController;
	BMessageRunner *fPulser;

	typedef BView _inherited;
};

#endif
