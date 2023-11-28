
#include "MediaPlayerApp.h"
#include "Preferences.h"

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <MessageFilter.h>
#include <RadioButton.h>
#include <StringView.h>
#include <ctype.h>

const int32 kButtonHeight = 20;
const int32 kButtonWidth = 75;

const uint32 M_OK = 'okok';

const uint32 kCheckBoxChanged = 'chkc';

const float kPrefsWindWidth = 280;
const float kPrefsWindHeight = 316;

MediaPlayerSettings::MediaPlayerSettings()
	:	Settings("mediaPlayerSettings", "mediaPlayer")
{
	Add(fAutoPlay = new CheckBoxSettingItem("AutoPlay", "Automatically start playing", true)); 
	Add(fQuitWhenDoneWithMovies = new CheckBoxSettingItem("QuitWhenDoneWithMovies",
		"Close window when done playing movies", false));
	Add(fQuitWhenDoneWithSounds = new CheckBoxSettingItem("QuitWhenDoneWithSounds",
		"Close window when done playing sounds", true));
	Add(fLoopMoviesByDefault = new CheckBoxSettingItem("LoopMoviesByDefault",
		"Loop movies by default", false));
	Add(fLoopSoundsByDefault = new CheckBoxSettingItem("LoopSoundsByDefault",
		"Loop sounds by default", false));
	Add(fBackgroundVolumeSettings = new BackgroundVolumeSettingItem("BackgroundMovieVolume"));
	Add(fMediaNodeDefault = new BooleanValueSetting("MediaNodeDefault", false));
	Add(fEnableHardwareOverlay = new CheckBoxSettingItem("EnableHardwareOverlay",
		"Enable hardware video overlays", true));
	TryReadingSettings();
}


MediaPlayerSettings::~MediaPlayerSettings()
{
	SaveSettings(false /* always save */);
}

void 
MediaPlayerSettings::RunSettingsDialog(BPoint pos)
{
	(new PrefsWindow(this, pos))->Show();
}

const CheckBoxSettingItem *
MediaPlayerSettings::LoopSoundsByDefault() const
{
	return fLoopSoundsByDefault;
}

const CheckBoxSettingItem *
MediaPlayerSettings::LoopMoviesByDefault() const
{
	return fLoopMoviesByDefault;
}

const CheckBoxSettingItem *
MediaPlayerSettings::AutoPlay() const
{
	return fAutoPlay;
}

const CheckBoxSettingItem *
MediaPlayerSettings::QuitWhenDoneWithSounds() const
{
	return fQuitWhenDoneWithSounds;
}

const CheckBoxSettingItem *
MediaPlayerSettings::QuitWhenDoneWithMovies() const
{
	return fQuitWhenDoneWithMovies;
}

const BackgroundVolumeSettingItem *
MediaPlayerSettings::BackgroundVolume() const
{
	return fBackgroundVolumeSettings;
}

const BooleanValueSetting *
MediaPlayerSettings::MediaNodeDefault() const
{
	return fMediaNodeDefault;
}

const CheckBoxSettingItem *
MediaPlayerSettings::EnableHardwareOverlays() const
{
	return fEnableHardwareOverlay;
}

class DialogFilter : public BMessageFilter {
public:
	DialogFilter(BWindow *);
	virtual filter_result Filter(BMessage *, BHandler **);
private:
	BWindow *filterTarget;
};

DialogFilter::DialogFilter(BWindow *targetWindow)
	:	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN),
		filterTarget(targetWindow)
{
}

filter_result 
DialogFilter::Filter(BMessage *message, BHandler **)
{
	uchar ch = 0;

	if (message->FindInt8("byte", (int8 *)&ch) == B_NO_ERROR) {
		int32 modifiers = 0;
		message->FindInt32("modifiers", &modifiers);
		if (ch == B_ESCAPE || toupper(ch) == 'W' && (modifiers & B_COMMAND_KEY)) {
			BView *view = filterTarget->FindView("cancel");
			BButton *button = dynamic_cast<BButton *>(view);
			if (button) {
				button->SetValue(1);
				// give some feedback
				button->Sync();
				snooze(50000);
			}
			filterTarget->PostMessage(B_CANCEL);
			return B_SKIP_MESSAGE;
		}
	}
	return B_DISPATCH_MESSAGE;
}


PrefsWindow::PrefsWindow(MediaPlayerSettings *settings, BPoint pos)
	:	BWindow(BRect(pos.x, pos.y, kPrefsWindWidth + pos.x, kPrefsWindHeight + pos.y),
			"MediaPlayer Preferences",
			B_MODAL_WINDOW ,
			B_FRAME_EVENTS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_WILL_DRAW,
			B_CURRENT_WORKSPACE),
		target(settings)
{
	BRect bounds(Bounds());
	BBox *backgroundBox = new BBox(bounds, "", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	backgroundBox->SetFont(be_plain_font);
	AddChild(backgroundBox);

	bounds.InsetBy(12, 10);
	bounds.bottom -= 40;
	BBox *box = new BBox(bounds, "box", B_FOLLOW_ALL, B_WILL_DRAW);
	box->SetLabel("MediaPlayer Preferences");
	box->SetFont(be_bold_font);
	box->SetFontSize(12);
	backgroundBox->AddChild(box);

	
	BRect labelRect(box->Bounds());
	labelRect.InsetBy(10, 20);
	labelRect.bottom = labelRect.top + 16;
	labelRect.right = labelRect.left + 160;
	
	box->AddChild(new BStringView(labelRect, "", "Play Mode:"));
	
	BRect checkBoxRect(labelRect);

	checkBoxRect.OffsetBy(5, 20);
	target->fAutoPlay->AddPreferenceItem(box, checkBoxRect.LeftTop());

	checkBoxRect.OffsetBy(5, 20);
	target->fQuitWhenDoneWithMovies->AddPreferenceItem(box, checkBoxRect.LeftTop());
	checkBoxRect.OffsetBy(0, 20);
	target->fQuitWhenDoneWithSounds->AddPreferenceItem(box, checkBoxRect.LeftTop());

	checkBoxRect.OffsetBy(-5, 25);
	target->fLoopMoviesByDefault->AddPreferenceItem(box, checkBoxRect.LeftTop());
	checkBoxRect.OffsetBy(0, 20);
	target->fLoopSoundsByDefault->AddPreferenceItem(box, checkBoxRect.LeftTop());
	checkBoxRect.OffsetBy(0, 20);
	target->fEnableHardwareOverlay->AddPreferenceItem(box, checkBoxRect.LeftTop());

	labelRect.OffsetTo(labelRect.left, checkBoxRect.bottom + 10);
	box->AddChild(new BStringView(labelRect, "", "Play background movies at:"));
	checkBoxRect.OffsetTo(labelRect.left + 5, labelRect.bottom + 5);
	target->fBackgroundVolumeSettings->AddPreferenceItem(box, checkBoxRect.LeftTop());
	
	BRect buttonRect(Bounds().Width()-kButtonWidth - 13,
		Bounds().Height()-kButtonHeight-19,
		Bounds().Width() - 13,
		Bounds().Height() - 19);
	BButton *okButton = new BButton(buttonRect, "", "OK", new BMessage(M_OK),
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	backgroundBox->AddChild(okButton);

	buttonRect.OffsetBy(- buttonRect.Width() - 13, 0);
	backgroundBox->AddChild(new BButton(buttonRect, "cancel", "Cancel", new BMessage(B_CANCEL),
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT));
	SetDefaultButton(okButton);

	AddCommonFilter(new DialogFilter(this));

	UpdateItems();
}

void 
PrefsWindow::UpdateItems()
{
	target->fQuitWhenDoneWithSounds->check->SetEnabled(target->fAutoPlay->check->Value());
	target->fQuitWhenDoneWithMovies->check->SetEnabled(target->fAutoPlay->check->Value());
}

void 
PrefsWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case M_OK:
			target->fAutoPlay->SetValueToPreferenceItem();
			target->fQuitWhenDoneWithSounds->SetValueToPreferenceItem();
			target->fQuitWhenDoneWithMovies->SetValueToPreferenceItem();
			target->fLoopSoundsByDefault->SetValueToPreferenceItem();
			target->fLoopMoviesByDefault->SetValueToPreferenceItem();
			target->fBackgroundVolumeSettings->SetValueToPreferenceItem();
			target->fEnableHardwareOverlay->SetValueToPreferenceItem();

			be_app->PostMessage(kPrefsChanged);
			// fall thru

		case B_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case kCheckBoxChanged:
			UpdateItems();
			break;

		default:
			_inherited::MessageReceived(message);
	}
}


CheckBoxSettingItem::CheckBoxSettingItem(const char *name, const char *title, bool defaultValue)
	:	BooleanValueSetting(name, defaultValue),
		title(title)
{
}

bool 
CheckBoxSettingItem::AddPreferenceItem(BView *parentView, BPoint where)
{
	BRect rect(0, 0, 90, 16);
	rect.OffsetTo(where);
	
	check = new BCheckBox(rect, Name(), Title(), new BMessage(kCheckBoxChanged));
	parentView->AddChild(check);
	check->ResizeToPreferred();
	
	SetPreferenceItemToValue();

	return true;
}

void 
CheckBoxSettingItem::SetPreferenceItemToValue()
{
	check->SetValue(fValue);
}

void 
CheckBoxSettingItem::SetValueToPreferenceItem()
{
	fValue = check->Value();
}

const char *kBackgroundVolumeValues[] = {
	"fullBackgroundVolume",
	"halfBackgroundVolume",
	"mutedBackgroundVolume",
	0
};

BackgroundVolumeSettingItem::BackgroundVolumeSettingItem(const char *name)
	:	EnumeratedStringValueSetting(name, kBackgroundVolumeValues[1],
			kBackgroundVolumeValues, "value expected", "wrong value")
{
}

BackgroundVolumeSettingItem::Values 
BackgroundVolumeSettingItem::EnumValue() const
{
	if (strcmp(Value(), kBackgroundVolumeValues[0]) == 0)
		return kFullVolume;
	else if (strcmp(Value(), kBackgroundVolumeValues[1]) == 0)
		return kHalfVolume;
	else
		return kMuted;
}

bool 
BackgroundVolumeSettingItem::AddPreferenceItem(BView *parentView, BPoint where)
{
	BRect rect(0, 0, 90, 16);
	rect.OffsetTo(where);
	
	fullRadio = new BRadioButton(rect, Name(), "Full Volume", 0);
	fullRadio->ResizeToPreferred();
	parentView->AddChild(fullRadio);
	rect.OffsetBy(0, 20);
	halfRadio = new BRadioButton(rect, Name(), "Half Volume", 0);
	halfRadio->ResizeToPreferred();
	parentView->AddChild(halfRadio);
	rect.OffsetBy(0, 20);
	mutedRadio = new BRadioButton(rect, Name(), "Muted", 0);
	mutedRadio->ResizeToPreferred();
	parentView->AddChild(mutedRadio);
	
	SetPreferenceItemToValue();

	return true;
}

void 
BackgroundVolumeSettingItem::SetPreferenceItemToValue()
{
	fullRadio->SetValue(strcmp(Value(), kBackgroundVolumeValues[0]) == 0);
	halfRadio->SetValue(strcmp(Value(), kBackgroundVolumeValues[1]) == 0);
	mutedRadio->SetValue(strcmp(Value(), kBackgroundVolumeValues[2]) == 0);
}

void 
BackgroundVolumeSettingItem::SetValueToPreferenceItem()
{
	if (fullRadio->Value())
		ValueChanged(kBackgroundVolumeValues[0]);
	else if (halfRadio->Value())
		ValueChanged(kBackgroundVolumeValues[1]);
	else
		ValueChanged(kBackgroundVolumeValues[2]);
}

