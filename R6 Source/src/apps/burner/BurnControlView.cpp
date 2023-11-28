//
// BurnControlView.cpp
//
//  by Nathan Schrenk (nschrenk@be.com)
//

#include "BurnControlView.h"

#include <Alert.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <StatusBar.h>
#include <String.h>
#include <TextControl.h>
#include "Burner.h"
#include "BurnProgress.h"
#include "BurnerWindow.h"
#include "CDDriver.h"
#include "CDPlayerView.h"
#include "SCSIDevice.h"
#include "CDTrack.h"
#include "FIFOStatus.h"
#include "TrackListView.h"
#include "EditWidget.h"
#include "AudioWrapperDataSource.h"

#include <stdio.h>

static const uint32 kBurnButtonMessage 			= 'bBRN';
static const uint32 kUpdateBurnStatusMessage	= 'uPBR';

static const char *kBurnButtonStartLabel	= "Burn Now!";
static const char *kBurnButtonAbortLabel	= "Abort";
static const char *kDeviceMenuLabel			= "CD-R Drive:";

const int32 kFIFODisplayWidth	= 32;

//-------------------------------------------------------------------------------


BurnControlView::BurnControlView(BRect frame, uint32 resizingMode)
	: BBox(frame, "BurnControlView", resizingMode, B_WILL_DRAW | B_FRAME_EVENTS)
{
//	SetLabel("CDR Controls");
	BRect rect(frame);
	rect.OffsetTo(0, 0);
	rect.InsetBy(5, 10);

	rect.top = rect.bottom - 20;
	rect.left = rect.right - 60;
	fBurnButton = new BButton(rect, NULL, kBurnButtonStartLabel,
		new BMessage(kBurnButtonMessage), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fBurnButton->SetEnabled(false);
	AddChild(fBurnButton);
	
	rect = frame;
	rect.OffsetTo(0, 0);
	rect.InsetBy(5, 5);
	rect.bottom = fBurnButton->Frame().top - 5;


	fEditWidget = new EditWidget(rect, B_FOLLOW_ALL_SIDES);
	AddChild(fEditWidget);
	
	rect.top = rect.bottom + 5;
	rect.bottom = fBurnButton->Frame().bottom;
	rect.left = frame.left + 5;
	rect.right = rect.left + 350;
	
	fDeviceMenu = new BPopUpMenu("[Scanning devices...]");
	fDeviceMenuField = new BMenuField(rect, NULL, kDeviceMenuLabel, fDeviceMenu,
									  B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fDeviceMenuField->SetDivider(fDeviceMenuField->StringWidth(kDeviceMenuLabel) + 5);
	AddChild(fDeviceMenuField);
	
	fMessageRect.Set(rect.right + 5, rect.top, fBurnButton->Frame().left - 5, rect.bottom);

	fBurnStarted = false;
	fDriver = NULL;
	fDevice = NULL;
}

BurnControlView::~BurnControlView()
{
	if (fBurnStarted) {
		// XXX: do tear down
		EndBurn();
	}
	delete fDriver;
}

void BurnControlView::MessageReceived(BMessage *message)
{
	bool updateBurnButton = false;
	
	switch (message->what) {
	case kBurnButtonMessage:
		fBurnButton->SetEnabled(false); // button is re-enabled by the window
		if (!fBurnStarted) {
			// start burn
			if (StartBurn() != B_OK) {
				// burn failed to start properly, so end it
				EndBurn();
			}
		} else {
			// abort burn
			EndBurn();
		}
		break;
	case kUpdateBurnStatusMessage:
		if (fBurnStarted) {
			if (fDriver->Burning()) {
				UpdateBurnStatus();
			} else {
				// XXX: should change status message to indicate success or something,
				//      and perhaps eject newly burned CD?
				EndBurn();
			}
		}
		break;
	case kDeviceSelectMessage:
		{
			CDRMenuItem *item;
			if (message->FindPointer("source", (void**)&item) == B_OK) {
				fDriver = item->Driver();
//				BString status("'");
//				status << fDriver->DeviceName() << "' selected.";
//				SetStatusMessage(status.String());
				SetStatusMessage("CDR drive selected");
			} else {
				SetStatusMessage("CDR drive select error!");
			}
	
			updateBurnButton = true;
		} break;
	case BurnerWindow::TRACK_ADDED:		// fall through
	case BurnerWindow::TRACK_DELETED:
		updateBurnButton = true;
		break;
	default:
		BBox::MessageReceived(message);
	}

	// set burn button state appropriately
	if (updateBurnButton) {
		fBurnButton->SetEnabled(CanBurn());
	}
}

void BurnControlView::AttachedToWindow()
{
	BWindow *window(Window());
	fBurnButton->SetTarget(this, window);

	// subscribe to track messages
	fWindow = dynamic_cast<BurnerWindow *>(Window());
	if (fWindow) {
		fWindow->AddTrackListener(this);
	}
}

void BurnControlView::DetachedFromWindow()
{
	// unsubscribe from track messages
	if (fWindow) {
		fWindow->RemoveTrackListener(this);
		fWindow = NULL;
	}
}


void BurnControlView::Draw(BRect update)
{
	BBox::Draw(update); // have the superclass do its drawing
	
	BFont font;
	GetFont(&font); // save font
	SetFont(be_plain_font);
	// XXX: the string truncation should only be done on resize or when
	// the status message changes, not every time!!!
	const char *inString = fStatusMessage.String();
	char *outString = (char *)malloc(strlen(inString) + 3);
	be_plain_font->GetTruncatedStrings(&inString, 1, B_TRUNCATE_END,
		fMessageRect.Width(), &outString);

	font_height fh;
	GetFontHeight(&fh);
	DrawString(outString, fMessageRect.LeftTop()
		+ BPoint(1, fh.ascent + (fMessageRect.Height() - (fh.ascent + fh.descent + fh.leading)) / 2));
	free(outString);
	SetFont(&font); // restore font
}

void BurnControlView::FrameResized(float /*width*/, float /*height*/)
{
	BRect menuRect(fDeviceMenuField->Frame());
	BRect buttonRect(fBurnButton->Frame());
	fMessageRect.Set(menuRect.right + 5, fMessageRect.top,
					 buttonRect.left - 5, fMessageRect.bottom);
	Invalidate();
}


void BurnControlView::SetEnabled(bool enabled)
{
	if (fEnabled != enabled) {
		if (fBurnButton->IsEnabled() != enabled) {
			fBurnButton->SetEnabled(enabled);
		}
	}
}

bool BurnControlView::CanBurn()
{
	TrackListView *tlView = dynamic_cast<TrackListView *>(Window()->FindView("TrackListView"));
	bool canBurn = (	fDriver && fDriver->IsBlankDisc() &&
						tlView &&
						(tlView->GetTrackList() != NULL));
//	if (!canBurn) {
//		printf("can't burn -- fDriver = %p, fDriver->IsBlankDisc() = %s, tlView = %p\n",
//			fDriver, fDriver ? (fDriver->IsBlankDisc() ? "true" : "false") : "(nil)", tlView);
//	}
	return canBurn;
}

status_t BurnControlView::StartBurn()
{
	fBurnStarted = true;

	BWindow *win(Window());

	fBurnButton->SetLabel(kBurnButtonAbortLabel);
	SetStatusMessage("Starting burn...", true);
	
	CDPlayerView *cdPlayer = dynamic_cast<CDPlayerView *>(win->FindView(CDPLAYERVIEWNAME));
	if (cdPlayer != NULL) {
		cdPlayer->SetControlsEnabled(false);
	}
	
	CDTrack *trackList = fEditWidget->TrackList();

	// remove the EditWidget from the view and add the FIFO and BurnProgress
	// display widgets
	BRect rect(fEditWidget->Frame());
	BRect rect2(rect);
	rect2.right = rect2.left + kFIFODisplayWidth;
	fFIFOStatus = new FIFOStatus(rect2, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	
	rect2.left = rect2.right + 5;
	rect2.right = rect.right;
	fProgress = new BurnProgress(rect2, trackList, fEditWidget->ResizingMode());

	win->BeginViewTransaction();
	fEditWidget->RemoveSelf();
	AddChild(fProgress);
	AddChild(fFIFOStatus);
	win->EndViewTransaction();
	
	// Enable the gain conversion on all AudioWrapperDataSources in the track list
	AudioWrapperDataSource *src;
	CDTrack *tmpTrack = trackList;
	while (tmpTrack != NULL) {
		src = dynamic_cast<AudioWrapperDataSource *>(tmpTrack->DataSource());
		if (src != NULL) {
			src->SetGainEnabled(true);
		}
		tmpTrack = tmpTrack->Next();
	}
	
	BString str;
	if (fDriver->Check(trackList) != B_OK) {
		str.SetTo("The CDR driver rejected the current track arrangement.\n\n");
		str << "Error: " << fDriver->GetError(); 
		BAlert *alert = new BAlert("Rejected", str.String(), "Sorry",
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return B_ERROR;
	}
	
	if (fDriver->Start(trackList)) {
		str.SetTo("The CDR driver cannot start the burn.\n\n");
		str << "Error: " << fDriver->GetError(); 
		BAlert *alert = new BAlert("Burn Failure", str.String(), "Sorry",
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return B_ERROR;
	}

	// construct a BMessageRunner to send an update message every half-second
	// until it's stopped
	BMessenger messenger(this, Window());
	BMessage burnStatusMsg(kUpdateBurnStatusMessage);
	fRunner = new BMessageRunner(messenger, &burnStatusMsg, (bigtime_t)500000, -1);
	
	win->PostMessage(kBurnStarting, win);

	return B_OK;
}


status_t BurnControlView::EndBurn()
{
	if (!fBurnStarted) {
		return B_ERROR;
	}

	fBurnStarted = false;
	BWindow *win(Window());
	win->PostMessage(kBurnEnding, win);

	BRect rect(fProgress->Frame());
	win->BeginViewTransaction();
	fProgress->RemoveSelf();
	fFIFOStatus->RemoveSelf();
	AddChild(fEditWidget);	// XXX: for some reason the view color doesn't get set right here
	fEditWidget->ResizeTo(rect.Width() + kFIFODisplayWidth + 5,
						  fEditWidget->Frame().Height());
	win->EndViewTransaction();
	fEditWidget->Invalidate();
	
	delete fProgress;
	fProgress = NULL;
	delete fFIFOStatus;
	fFIFOStatus = NULL;
	
	if (fDriver->Burning()) { // cancel a burn in progress
		fDriver->Abort();
	}
	if (fRunner) {
		delete fRunner;
		fRunner = NULL;
	}

	// Disable the gain conversion on all AudioWrapperDataSources in the track list
	TrackListView *tlv = dynamic_cast<TrackListView *>(win->FindView("TrackListView"));
	if (tlv) {
		CDTrack *track = tlv->GetTrackList();
		AudioWrapperDataSource *src;
		while (track != NULL) {
			src = dynamic_cast<AudioWrapperDataSource *>(track->DataSource());
			if (src != NULL) {
				src->SetGainEnabled(false);
			}
			track = track->Next();
		}
	}
	
	fBurnButton->SetLabel(kBurnButtonStartLabel);

	BString alertText("Burn result: ");
	alertText << fDriver->GetError();// << "\n";
//	BAlert *doneAlert = new BAlert("Burn Ended", alertText.String(), "Continue");
//	doneAlert->Go();
	SetStatusMessage(alertText.String(), true);

	CDPlayerView *cdPlayer = dynamic_cast<CDPlayerView *>(win->FindView(CDPLAYERVIEWNAME));
	if (cdPlayer != NULL) {
		cdPlayer->SetControlsEnabled(true);
	}
	
	return B_OK;
}


void BurnControlView::UpdateBurnStatus()
{
	if (!fBurnStarted) {
		return;
	}
//	char buf[16];
	float val;//, cur_val;

		
//	static float count = 0.0f;
//	val = count += 0.01f;
	val = fDriver->PercentFifo();	
	fFIFOStatus->SetPercentFull(val);

	val = fDriver->PercentDone();
	fProgress->SetPercentComplete(val);
}

void BurnControlView::SetStatusMessage(const char *status, bool immediate)
{
	fStatusMessage.SetTo(status);
	if (immediate) {
		BRegion clipRegion(fMessageRect);
		ConstrainClippingRegion(&clipRegion);
		FillRect(fMessageRect, B_SOLID_LOW);
		Draw(fMessageRect);
		ConstrainClippingRegion(NULL);
		Sync();
	} else {
		Invalidate(fMessageRect);
	}
}

BMenu *BurnControlView::GetDeviceMenu()
{
	return fDeviceMenu;
}

BButton *BurnControlView::GetBurnButton()
{
	return fBurnButton;
}


