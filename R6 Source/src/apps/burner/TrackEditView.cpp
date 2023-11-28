//
// TrackEditView.cpp
//
//  by Nathan Schrenk (nschrenk@be.com)

#include <stdlib.h>	// for strtod()
#include <errno.h>
#include <Slider.h>
#include <TextControl.h>
#include "AudioWrapperDataSource.h"
#include "BurnerWindow.h"
#include "CDPlayerView.h"
#include "CDTrack.h"
#include "EditWidget.h"
#include "GfxUtils.h"
#include "TrackEditView.h"
#include "TrackListView.h"

#include <stdio.h>

const char kGainLabel[]			= "Gain";
const char kGainMaxLabel[]		= "2.0";
const char kGainMinLabel[]		= "0.0";
const char kGainMidLabel[]		= "1.0";

const char kStartLabel[]		= "Start:";
const char kEndLabel[]			= "End:";
const char kFadeInLabel[]		= "Fade In:";
const char kFadeOutLabel[]		= "Fade Out:";
const char kPregapLabel[]		= "Pregap:";

const float kBytesPerSecond		= 75.0 * 2352;

//----------------------------------------------------------------------------------------

class NumericTextControl : public BTextControl
{
public:
					NumericTextControl(BRect frame, const char *name, const char *label,
						float value, BMessage *message, uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
						uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
	virtual			~NumericTextControl();

	void			SetFloatValue(float value);
	float			FloatValue();

	static status_t	StringToFloat(const char *text, float *f);
};

NumericTextControl::NumericTextControl(BRect frame, const char *name, const char *label, float value,
	BMessage *message, uint32 resizingMode, uint32 flags)
	: BTextControl(frame, name, label, B_EMPTY_STRING, message, resizingMode, flags)
{
	SetFloatValue(value);
}

NumericTextControl::~NumericTextControl()
{
}

void NumericTextControl::SetFloatValue(float value)
{
	BString str;
	str << value;
	if (strcmp(str.String(), Text())) {
		SetText(str.String());
	}
}

float NumericTextControl::FloatValue()
{
	float f;
	if (StringToFloat(Text(), &f) == B_OK) {
		return f;
	} else {
		return -1;
	}
}

status_t NumericTextControl::StringToFloat(const char *text, float *f)
{
	errno = 0;
	*f = (float)strtod(text, NULL);
	return ((errno == 0) ? B_OK : B_ERROR);

}


//----------------------------------------------------------------------------------------

//class PreGapSelector : public TListSelector
//{
//public:
//	PreGapSelector(BRect frame, uint32 resizingMode);
//	virtual ~PreGapSelector();
//	
//	uint32 PreGap();
//};
//
//PreGapSelector::PreGapSelector(BRect frame, uint32 resizingMode)
//	: TListSelector(frame, B_EMPTY_STRING, NULL)
//{
//	SetResizingMode(resizingMode);
//	AddList(0, 73, ":", new BMessage(kPregapMessage));
//	AddList(0, 59, ":", new BMessage(kPregapMessage));
//	AddList(0, 74, NULL, new BMessage(kPregapMessage));
//	SetAlignment(B_ALIGN_RIGHT, 0);
//	SetAlignment(B_ALIGN_RIGHT, 1);
//	SetAlignment(B_ALIGN_RIGHT, 2);
//}
//
//
//PreGapSelector::~PreGapSelector()
//{
//	// XXX: clean up?
//}
//
//uint32 PreGapSelector::PreGap()
//{
//	uint32 preGap = Selection(0) * 60 * 75;
//	preGap += Selection(1) * 75;
//	preGap += Selection(2);
//	return preGap;
//}



//----------------------------------------------------------------------------------------

TrackEditView::TrackEditView(BRect frame, uint32 resizingMode)
	: BView(frame, "TrackEditView", resizingMode,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
	fTrack = NULL;
	fWindow = NULL;
	fEnabled = true;
	
	frame.OffsetTo(0, 0);
	BRect rect(frame);
	rect.InsetBy(5, 5);
	rect.bottom = rect.top + 20;
	
	fStartText = new NumericTextControl(rect, "StartText", kStartLabel, 0,
		new BMessage(kStartChangedMessage));
	
	rect = fStartText->Frame();
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	
	fEndText = new NumericTextControl(rect, "EndText", kEndLabel, 0,
		new BMessage(kEndChangedMessage));
	
	rect = fEndText->Frame();
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	
	fFadeInText = new NumericTextControl(rect, "FadeInText", kFadeInLabel, 0,
		new BMessage(kFadeInChangedMessage));
	
	rect = fFadeInText->Frame();
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	
	fFadeOutText = new NumericTextControl(rect, "FadeOutText", kFadeOutLabel, 0,
		new BMessage(kFadeOutChangedMessage));

	rect = fFadeOutText->Frame();
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	
	fPregapText = new NumericTextControl(rect, "PregapText", kPregapLabel, 0,
		new BMessage(kPregapChangedMessage));

	float width = (float)ceil(fFadeOutText->StringWidth(kFadeOutLabel)) + 15;
	fStartText->SetDivider(width);
	fEndText->SetDivider(width);
	fFadeInText->SetDivider(width);
	fFadeOutText->SetDivider(width);
	fPregapText->SetDivider(width);

	// XXX: right alignment on the text would be better, but it is totally broken!
	fStartText->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fEndText->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fFadeInText->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fFadeOutText->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fPregapText->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	
	rect = fPregapText->Frame();
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	
	fGainSlider = new BSlider(rect, "GainSlider", kGainLabel, new BMessage(kGainChangedMessage),
		0, 2 * kGainConversion, B_HORIZONTAL, B_BLOCK_THUMB, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fGainSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGainSlider->SetHashMarkCount(3);
	fGainSlider->SetLimitLabels("0.0", "2.0");
	fGainSlider->SetValue(floor(1 * kGainConversion));
	
	AddChild(fStartText);
	AddChild(fEndText);
	AddChild(fFadeInText);
	AddChild(fFadeOutText);
	AddChild(fPregapText);
	AddChild(fGainSlider);
	
	SetControlsEnabled(false, false);
}


TrackEditView::~TrackEditView()
{
}

void TrackEditView::Populate(CDTrack *track)
{
	fTrack = track;
	AudioWrapperDataSource *src(NULL);
	if (track != NULL) {
		src = dynamic_cast<AudioWrapperDataSource *>(track->DataSource());
	}
	if (src != NULL) {
		// set slider to correct values
		fGainSlider->SetValue(floor(src->Gain() * kGainConversion));

		// set text widgets to correct values
		uint32 dataPregap, emptyPregap;
		track->PreGap(&dataPregap, &emptyPregap);

		fPregapText->SetFloatValue(dataPregap / 75.0);
		fStartText->SetFloatValue(src->GetStart() / kBytesPerSecond);
		fEndText->SetFloatValue(src->GetEnd() / kBytesPerSecond);
		fFadeInText->SetFloatValue(src->GetFadeIn() / kBytesPerSecond);
		fFadeOutText->SetFloatValue(src->GetFadeOut() / kBytesPerSecond);

		SetControlsEnabled(fEnabled, true);
	} else {
		fGainSlider->SetValue(floor(1 * kGainConversion));
		fPregapText->SetFloatValue(0);
		fStartText->SetFloatValue(0);
		fEndText->SetFloatValue(0);
		fFadeInText->SetFloatValue(0);
		fFadeOutText->SetFloatValue(0);
		
		SetControlsEnabled(false, false);
	}
}

void TrackEditView::SetEnabled(bool enabled)
{
	if (fEnabled == enabled) {
		return;
	}

	bool enableSlider = ((fTrack != NULL) && !fTrack->IsData());
	bool enableText = enabled && enableSlider;
	SetControlsEnabled(enableText, enableSlider);

	fEnabled = enabled;
	Invalidate();
}

void TrackEditView::SetControlsEnabled(bool textEnabled, bool gainEnabled)
{
	fGainSlider->SetEnabled(gainEnabled);
	fStartText->SetEnabled(textEnabled);
	fEndText->SetEnabled(textEnabled);
	fFadeInText->SetEnabled(textEnabled);
	fFadeOutText->SetEnabled(textEnabled);
	fPregapText->SetEnabled(textEnabled);
}


bool TrackEditView::IsEnabled()
{
	return fEnabled;
}


void TrackEditView::MessageReceived(BMessage *message)
{
//	CDTrack *track;
	TrackListView *listView;
	EditWidget *editWidget;
	AudioWrapperDataSource *src;
	int32 preGap;
	bool invalidateOthers = false;
	
	switch (message->what) {
	case kPregapChangedMessage:	// the pregap changed
		preGap = (int32)floor(fPregapText->FloatValue() * 75);
		if (fTrack) {
			uint32 len = fTrack->DataSource()->Length();
			uint32 dataPregap, emptyPregap;
			fTrack->PreGap(&dataPregap, &emptyPregap);
			if ((preGap * fTrack->FrameSize()) > len) {
				preGap = len;
			} else if (preGap < 0) {
				preGap = 0;
			}
			fPregapText->SetFloatValue(preGap / 75.0);
			if ((int32)dataPregap != preGap) {
				fTrack->SetPreGap(preGap + emptyPregap, emptyPregap);
				invalidateOthers = true;
			}
		}
		break;
	case kStartChangedMessage:
		if (fTrack != NULL) {
			int64 start = (uint64)floor(fStartText->FloatValue() * kBytesPerSecond);
			int64 oldStart;
			src = dynamic_cast<AudioWrapperDataSource *>(fTrack->DataSource());
			if (src) {
				oldStart = src->GetStart();
				uint32 maxLen = src->Source()->Length(); 
				if (start >= maxLen) {
					start = maxLen - 1;
				} else if (start < 0) {
					start = 0;
				}
				fStartText->SetFloatValue(start / kBytesPerSecond);
				if (start != oldStart) {
					src->SetStart(start);
					invalidateOthers = true;
				}	
			}
		}
		break;
	case kEndChangedMessage:
		if (fTrack != NULL) {
			uint64 end = (uint64)floor(fEndText->FloatValue() * kBytesPerSecond);
			uint64 oldEnd;
			src = dynamic_cast<AudioWrapperDataSource *>(fTrack->DataSource());
			if (src) {
				oldEnd = src->GetEnd();
				uint32 maxLen = src->Source()->Length();
				uint64 minEnd = src->GetStart(); 
				if (end >= maxLen) {
					end = maxLen - 1;
				} else if (end <= minEnd) {
					end = minEnd + 1;
				}
				fEndText->SetFloatValue(end / kBytesPerSecond);
				if (end != oldEnd) {
					src->SetEnd(end);
					invalidateOthers = true;
				}
			}
		}
		break;
	case kFadeInChangedMessage:
		if (fTrack != NULL) {
			uint32 fadeIn = (uint32)floor(fFadeInText->FloatValue() * kBytesPerSecond);
			uint32 oldFadeIn;
			src = dynamic_cast<AudioWrapperDataSource *>(fTrack->DataSource());
			if (src) {
				oldFadeIn = src->GetFadeIn();
				uint32 maxFadeIn = src->GetEnd() - src->GetStart() - src->GetFadeOut();
				uint32 minFadeIn = 0; 
				if (fadeIn >= maxFadeIn) {
					fadeIn = maxFadeIn;
				} else if (fadeIn < minFadeIn) {
					fadeIn = minFadeIn;
				}
				fFadeInText->SetFloatValue(fadeIn / kBytesPerSecond);
				if (fadeIn != oldFadeIn) {
					src->SetFadeIn(fadeIn);
					invalidateOthers = true;
				}	
			}
		}
		break;
	case kFadeOutChangedMessage:
		if (fTrack != NULL) {
			uint32 fadeOut = (uint32)floor(fFadeOutText->FloatValue() * kBytesPerSecond);
			uint32 oldFadeOut;
			src = dynamic_cast<AudioWrapperDataSource *>(fTrack->DataSource());
			if (src) {
				oldFadeOut = src->GetFadeOut();
				uint32 maxFadeOut = src->GetEnd() - (src->GetStart() + src->GetFadeIn());
				uint32 minFadeOut = 0;
				if (fadeOut >= maxFadeOut) {
					fadeOut = maxFadeOut;
				} else if (fadeOut < minFadeOut) {
					fadeOut = minFadeOut;
				}
				fFadeOutText->SetFloatValue(fadeOut / kBytesPerSecond);
				if (fadeOut != oldFadeOut) {
					src->SetFadeOut(fadeOut);
					invalidateOthers = true;
				}
			}
		}
		break;
	case kGainChangedMessage:	// the gain changed
		if (fTrack != NULL) {
			float gain = fGainSlider->Value() / kGainConversion;
			src = dynamic_cast<AudioWrapperDataSource *>(fTrack->DataSource());
			if (src != NULL) {
				src->SetGain(gain);
			}
			BWindow *win = Window();
			message->AddFloat("gain", gain);
			CDPlayerView *cdPlayer = dynamic_cast<CDPlayerView *>(win->FindView(CDPLAYERVIEWNAME));
			if (cdPlayer != NULL) {
				cdPlayer->TrackGainChanged(fTrack, gain);
			}
		}
		// NOTE: if in the future we display the gain elsewhere, we must
		//       set invalidateOthers to true here.
		break;
	case BurnerWindow::TRACK_SELECTION_CHANGED:
		{
			if (message->FindPointer("tracklist", (void **)&listView) == B_OK) {
				TrackRow *row = dynamic_cast<TrackRow *>(listView->CurrentSelection());
				if (row) {
					Populate(row->Track());
				} else {
					Populate(NULL);
				}
			}
		}
		break;
	default:
		BView::MessageReceived(message);
		break;
	}
	
	// If some track attributes have changed, the list view and the edit view are
	// stale and must be redrawn.
	if (invalidateOthers) {		
		listView = dynamic_cast<TrackListView *>(Window()->FindView("TrackListView"));
		if (listView) {
			listView->TrackUpdated(fTrack);
		}
		editWidget = dynamic_cast<EditWidget *>(Window()->FindView("EditWidget"));
		if (editWidget) {
			editWidget->InvalidateBuffer();
		}
		// XXX: should dirty the project since something has changed
	}
}

void TrackEditView::AttachedToWindow()
{
	BView *parent = Parent();
	if (parent) {
		SetColorsFromParent();
		BFont font;
		parent->GetFont(&font);
		SetFont(&font);	
	}
	
	fGainSlider->SetTarget(this);
	fStartText->SetTarget(this);
	fEndText->SetTarget(this);
	fFadeInText->SetTarget(this);
	fFadeOutText->SetTarget(this);
	fPregapText->SetTarget(this);
	
	// subscribe to track messages
	fWindow = dynamic_cast<BurnerWindow *>(Window());
	if (fWindow) {
		fWindow->AddTrackListener(this);
	}
}

void TrackEditView::DetachedFromWindow()
{
	// unsubscribe from track messages
	if (fWindow) {
		fWindow->RemoveTrackListener(this);
		fWindow = NULL;
	}
}

void TrackEditView::Draw(BRect update)
{
	FillRect(update, B_SOLID_LOW);
}

void 
TrackEditView::AllAttached()
{
	SetViewColor(B_TRANSPARENT_COLOR);
}

void TrackEditView::FrameResized(float /*x*/, float /*y*/)
{
	// do anything?
}




