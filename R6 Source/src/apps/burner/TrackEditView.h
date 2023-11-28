//
// TrackEditView.h
//
//  by Nathan Schrenk (nschrenk@be.com)

#ifndef TRACK_EDIT_VIEW_H_
#define TRACK_EDIT_VIEW_H_

#include <View.h>


// message constants
const int32 kPregapChangedMessage	= 'pgpC';
const int32 kStartChangedMessage	= 'strC';
const int32 kEndChangedMessage		= 'endC';
const int32 kFadeInChangedMessage	= 'finC';
const int32 kFadeOutChangedMessage	= 'fotC';
const int32 kGainChangedMessage		= 'ganC';

// conversion multiple between gain levels and slider values
const float kGainConversion		= 10.0f;

class BButton;
class BCheckBox;
class BSlider;
class NumericTextControl;
class BurnerWindow;
class CDTrack;

class TrackEditView : public BView
{
public:
	TrackEditView(BRect frame, uint32 resizingMode = B_FOLLOW_ALL);
	virtual ~TrackEditView();
	
	void Populate(CDTrack *track);
	void SetEnabled(bool);
	bool IsEnabled();

protected:
	virtual void	MessageReceived(BMessage *message);
	virtual void 	AttachedToWindow();
	virtual void 	DetachedFromWindow();
	virtual void 	Draw(BRect update);
	virtual void 	AllAttached();
	virtual void 	FrameResized(float, float);

private:
	void		SetControlsEnabled(bool textEnabled, bool gainEnabled);
	
	BPoint			fGainLabelPos;
	BPoint			fGainMaxLabelPos;
	BPoint			fGainMinLabelPos;
	BPoint			fGainMidLabelPos;
	BPoint			fPregapLabelPos;
	BPoint			fFileLabelPos;

	CDTrack			*fTrack;
	NumericTextControl	*fStartText;
	NumericTextControl	*fEndText;
	NumericTextControl	*fFadeInText;
	NumericTextControl	*fFadeOutText;
	NumericTextControl	*fPregapText;

	BSlider			*fGainSlider;
	BurnerWindow	*fWindow;	
	bool	fEnabled;
};


#endif // TRACK_EDIT_VIEW_H_
