//
// BurnControlView.h
//
//  Contains the controls for starting/stopping a burn and progress bars
//
//  by Nathan Schrenk (nschrenk@be.com)
//

#ifndef BURN_CONTROL_VIEW_H_
#define BURN_CONTROL_VIEW_H_

#include <Box.h>
#include <String.h>

class BButton;
class BMenu;
class BMenuField;
class BMessageRunner;
class BMessenger;
class BurnProgress;
class BurnerWindow;
class CDDriver;
class EditWidget;
class FIFOStatus;
class SCSIDevice;

class BurnControlView : public BBox
{
public:
				BurnControlView(BRect frame, uint32 resizingMode);
	virtual		~BurnControlView();

	virtual void MessageReceived(BMessage *message);
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void Draw(BRect update);
	virtual void FrameResized(float width, float height);
	
	void		SetEnabled(bool enabled);
	void		SetStatusMessage(const char *status, bool immediate = false);
	
	BMenu*		GetDeviceMenu();
	BButton*	GetBurnButton();
	bool		CanBurn();
	
private:
	
	status_t	StartBurn();
	status_t	EndBurn();
	void 		UpdateBurnStatus();

	BString			fStatusMessage;
	BRect			fMessageRect;
	EditWidget		*fEditWidget;
	BMenuField		*fDeviceMenuField;
	BMenu			*fDeviceMenu;
	BurnProgress	*fProgress;
	FIFOStatus		*fFIFOStatus;
	BButton			*fBurnButton;
	CDDriver		*fDriver;
	SCSIDevice		*fDevice;
	BMessageRunner	*fRunner;
	BurnerWindow	*fWindow;
	bool			fBurnStarted;
	bool			fEnabled;
};

#endif // BURN_CONTROL_VIEW_H_
