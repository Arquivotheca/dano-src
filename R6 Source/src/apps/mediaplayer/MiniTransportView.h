#ifndef __MINI_TRANSPORTVIEW_H__
#define __MINI_TRANSPORTVIEW_H__

#include "TransportView.h"


class TimeDisplay;
class BSlider;
class SliderView;

class MiniTransportView : public TransportView {
public:

	MiniTransportView(BRect rect, uint32 resizingMode, bool showModeSwitch = true);
	virtual void SetEnabled(bool enabled);
	virtual void AttachToController(MediaController*);	

protected:
	virtual void MessageReceived(BMessage *);
	virtual void AttachedToWindow();
	virtual void UpdateButtons();

private:
	BRect fSliderRect;
	typedef TransportView _inherited;
};


#endif





