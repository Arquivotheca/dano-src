#if !defined(MSIPL_DEF_TEMPARG)
#define MSIPL_DEF_TEMPARG
#endif
#include <set>

#include <Application.h>
#include <Window.h>
#include <Box.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Button.h>
#include <PictureButton.h>
#include <PopUpMenu.h>
#include <FilePanel.h>
#include <Screen.h>
#include <Alert.h>
#include <ListItem.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <MediaRoster.h>
#include <MenuItem.h>
#include <InterfaceDefs.h>
#include <StringView.h>
#include <Slider.h>
#include <Debug.h>

#include <MediaFiles.h>
#include <Beep.h>
#include <PlaySound.h>
#include <MenuField.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if !defined(_PR3_COMPATIBLE_)
	inline operator<(const entry_ref & a, const entry_ref & b) {
		return (a.device < b.device) || (a.device == b.device && 
			(a.directory < b.directory || (a.directory == b.directory &&
				(strcmp(a.name, b.name) < 0))));
	}
#endif



// Uncomment to enable removing files from events
// This currently commented out, because removing a file
// from an event removes the event name, which is bad
// (because you couldn't set an event for it again).
#define ENABLE_REMOVE_FILE 1

const int32 OTHER_PRESSED = 'othr';
const int32 PLAY_PRESSED = 'plpr';
const int32 STOP_PRESSED = 'stpr';
const int32 EVENT_SELECTED = 'itsl';
const int32 FILE_SELECTED = 'fisl';
const int32 GAIN_CHANGED = 'gchg';
const int32 GAIN_CHANGING = 'gchi';

#if ENABLE_REMOVE_FILE
	const int32 SET_NONE = 'setn';
#endif

const char *kNoneLabel = "<none>";
const char *kFileMenuLabel = "Sound File:";
const float kButtonWidth = 80;
const float kButtonHeight = 20;
const float kLabelHeight = 25;
const float kEventMarginWidth = 15;

const rgb_color kListHighlightColor = { 205, 205, 205, 255 };
const rgb_color kWhite = { 255, 255, 255, 255 };
const rgb_color kBlack = { 0, 0, 0, 255 };

class EventItem : public BListItem {
public:
	
	EventItem(const BString &eventName, const BString &fileName, float audioGain, float* _eventWidth, float* _fileWidth);

	const BString&	GetEventName();
	float			AudioGain() const;
	void			SetAudioGain(float gain);
	void 			SetFileName(const BString &name);
	virtual void 	DrawItem(BView*, BRect, bool = false);

private:

	BString eventName;
	BString	fileName;
	float audioGain;

	float*	eventWidth;
	float*	fileWidth;
};

EventItem::EventItem(const BString &newEventName, const BString &newFileName, float newAudioGain, float* _eventWidth, float* _fileWidth)
	:	eventName(newEventName),
		fileName(newFileName),
		audioGain(newAudioGain),
		eventWidth(_eventWidth),
		fileWidth(_fileWidth)
{
}


void EventItem::SetFileName(const BString &name)
{
	fileName = name;
}

const BString& EventItem::GetEventName()
{
	return eventName;
}

float 
EventItem::AudioGain() const
{
	return audioGain;
}

void 
EventItem::SetAudioGain(float gain)
{
	audioGain = gain;
}

void EventItem::DrawItem(BView *view, BRect rect, bool complete)
{
	complete=complete;
	// Draw background
	if (IsSelected())
		view->SetHighColor(kListHighlightColor);
	else
		view->SetHighColor(kWhite);

	view->FillRect(rect);	

	// Draw text
	view->SetHighColor(kBlack);
	if (IsSelected()) 
		view->SetLowColor(kListHighlightColor);
	else 
		view->SetLowColor(kWhite);

	BFont font;
	view->GetFont(&font);
	font_height height;
	font.GetHeight(&height);

	float maxHeight = height.ascent + height.descent;
	float gap = (rect.Height() - maxHeight) / 2;
	if (gap < 0)
		gap = 0;

	float baseline = rect.bottom - gap - height.descent;

	view->MovePenTo(rect.left + 5, baseline);
	view->DrawString(eventName.String());
	view->MovePenTo(rect.left + 5 + *eventWidth + kEventMarginWidth, baseline);
	view->DrawString(fileName.String());
}

class SoundsWindow : public BWindow {
public:
	
				SoundsWindow();
	virtual		~SoundsWindow();
	virtual		void MessageReceived(BMessage * message);

private:

	BMediaFiles 	m_media;
	BListView 		*m_eventList;
	BMenu 			*m_soundFileMenu;
	BFilePanel 		m_file_panel;
//	BFileGameSound	*m_activeSound;
	int32			m_activeSound;
	bool			m_selected_is_valid;
	entry_ref 		m_currentSelectedFile;
	BMenuField		*menuField;
	BSlider			*m_gainSlider;
	float			m_currentGain;
	float			m_maxEventWidth;
	float			m_maxFileWidth;

	void OtherPressed(BMessage * message);
	void StopActiveSound();
	void PlayPressed();
	void StopPressed();
	void EventSelected();
	void SoundFileSelected(BMessage *message);
	BMenuItem *SelectSoundFile(const entry_ref & ref);
	void GainChanged();
	void GainChanging();
	void UpdateGain(float gain);

	status_t Populate();
};


SoundsWindow::SoundsWindow() 
	: 	BWindow(BRect(200,150, 500, 500), "Sounds", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS), 
			m_file_panel(B_OPEN_PANEL, new BMessenger(this), NULL, 0, false), 
			m_activeSound(-1), m_selected_is_valid(false), m_currentGain(1.0)
{
	BRect r(BScreen(this).Frame());
	float width = r.Width()-30;
	float height = r.Height()-30;
	SetSizeLimits(300,width, 200,height);
	SetZoomLimits((width < 500) ? width : 500, height);

	BFont font(be_plain_font);
	font_height fh;
	font.GetHeight(&fh);

	// Main BBox
	BRect mainViewRect(Bounds());
	mainViewRect.OffsetTo(0,0);
	BBox *mainView = new BBox(mainViewRect, "SoundsView", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	AddChild(mainView);

	// Event List
	BRect eventRect(mainViewRect);
	eventRect.InsetBy(13, 13);
	eventRect.bottom -= 100;
	eventRect.right -= B_V_SCROLL_BAR_WIDTH;
	eventRect.top = kLabelHeight + 3;

	// Event List View
	m_eventList = new BListView(eventRect, "Event List",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	m_eventList->SetSelectionMessage(new BMessage(EVENT_SELECTED));
	BScrollView *eventScroller = new BScrollView("Event Scroller", 
		m_eventList, B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	mainView->AddChild(eventScroller);

	// Control BBox
	BRect controlRect(mainViewRect);
	controlRect.InsetBy(12, 12);
	controlRect.right += 1.0;
	controlRect.top = controlRect.bottom - 92.0;
	BBox *controlView = new BBox(controlRect, "Control Box", 
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	mainView->AddChild(controlView);

	// Sound file menu field
	m_soundFileMenu = new BPopUpMenu(kNoneLabel);
	BRect menuFieldRect(controlRect);
	menuFieldRect.OffsetTo(0, 0);
	menuFieldRect.InsetBy(15, 10);
	menuFieldRect.bottom = menuFieldRect.top + 15;
	menuField = new BMenuField(menuFieldRect, "FileList", kFileMenuLabel,
		m_soundFileMenu);
	menuField->SetDivider(be_plain_font->StringWidth(kFileMenuLabel) + 10);
	controlView->AddChild(menuField);

	// Determine if this machine has some sort of sound output
	// (like a sound card).  If not, disable play and stop buttons.
	bool hasOutputNode = false;
	BMediaRoster *roster = BMediaRoster::Roster();
	media_node outputNode;
	live_node_info outputNodeInfo;
	status_t err = roster->GetAudioOutput(&outputNode);
	if (err == B_OK) {
		err = roster->GetLiveNodeInfo(outputNode, &outputNodeInfo);
		if (err == B_OK)
			hasOutputNode = (outputNodeInfo.name != 0);
	}	

	// Gain control
	BRect gainRect(menuFieldRect);
	gainRect.left += menuField->Divider();
	gainRect.top = gainRect.bottom + 10;
	gainRect.bottom = gainRect.top + 20;
	m_gainSlider = new BSlider(gainRect, "gain slider", 0,
		new BMessage(GAIN_CHANGED), 0, 100, B_HORIZONTAL, B_BLOCK_THUMB,
		B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	m_gainSlider->SetModificationMessage(new BMessage(GAIN_CHANGING));
	controlView->AddChild(m_gainSlider);
	BRect labelRect(gainRect);
	labelRect.right = labelRect.left;
	labelRect.left = menuFieldRect.left;
	labelRect.bottom -= 4.0;
	BStringView* gainLabel = new BStringView(labelRect, "gain label", "Volume:");
	controlView->AddChild(gainLabel);

	// Stop button	
	BRect stopRect(gainRect);
	stopRect.top = stopRect.bottom + 2.0;
	stopRect.bottom = stopRect.top + kButtonHeight;
//	stopRect.right -= 5;
	stopRect.left = stopRect.right - kButtonWidth;
	BButton *stopButton = new BButton(stopRect, "stop button", "Stop", 
		new BMessage(STOP_PRESSED), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	controlView->AddChild(stopButton);
	if (!hasOutputNode)
		stopButton->SetEnabled(false);
	
	// Play button	
	BRect playRect(stopRect);
	playRect.OffsetBy(-(kButtonWidth + 10), 0);
	BButton *playButton = new BButton(playRect, "play button", "Play", 
		new BMessage(PLAY_PRESSED), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	controlView->AddChild(playButton);
	if (!hasOutputNode)
		playButton->SetEnabled(false);

	// init state & finish up layout
	m_media.RewindRefs(BMediaFiles::B_SOUNDS);
	Populate();
	
	// 'Event' title
	BRect eventTitleRect(eventRect.left + 2, 5, eventRect.left + 50, 
		kLabelHeight - 3);
	BStringView *eventTitleLabel = new BStringView(eventTitleRect, "", "Event");
	eventTitleLabel->SetViewColor(222, 222, 222);
	eventTitleLabel->SetFont(be_bold_font);
	mainView->AddChild(eventTitleLabel);

	// 'File' title	
	BRect fileTitleRect(eventRect.left + m_maxEventWidth + kEventMarginWidth + 2, 
		5, eventRect.left + m_maxEventWidth + kEventMarginWidth + 50, kLabelHeight - 3);
	BStringView *fileTitleLabel = new BStringView(fileTitleRect, "", "Sound");
	fileTitleLabel->SetViewColor(222, 222, 222);
	fileTitleLabel->SetFont(be_bold_font);
	mainView->AddChild(fileTitleLabel);
	
	Show();
}


SoundsWindow::~SoundsWindow()
{
	StopActiveSound();

	be_app->PostMessage(B_QUIT_REQUESTED);
}

void SoundsWindow::MessageReceived(BMessage * message)
{
	if (message->WasDropped() || message->what == B_REFS_RECEIVED) {
		entry_ref fileRef;
		if (message->FindRef("refs", &fileRef) == B_OK) {
			BMenuItem *item = SelectSoundFile(fileRef);
			ASSERT(item != 0);
			PostMessage(item->Message());
		}
		return;		
	}

	switch (message->what) {
	case OTHER_PRESSED:
		OtherPressed(message);
		break;
	case PLAY_PRESSED:
		PlayPressed();
		break;
	case STOP_PRESSED:
		StopPressed();
		break;
	case EVENT_SELECTED:
		EventSelected();
		break;
	case FILE_SELECTED:
		SoundFileSelected(message);
		break;

#if ENABLE_REMOVE_FILE
	case SET_NONE: {
		int32 eventIndex = m_eventList->CurrentSelection();
		EventItem *item = dynamic_cast<EventItem*>(m_eventList->ItemAt(	
			eventIndex));
		ASSERT(item != 0);
		const char *eventName = item->GetEventName().String();

		entry_ref oldRef;	
		if (m_media.GetRefFor(BMediaFiles::B_SOUNDS, eventName, &oldRef) == B_OK)  {
			m_media.RemoveRefFor(BMediaFiles::B_SOUNDS, eventName, oldRef);
		}

		PRINT(("Removed event for event name %s\n", eventName));

		// Set the ref in the item
		item->SetFileName(kNoneLabel);
		m_eventList->Invalidate();
		
		// set flag to note that m_currentSelectedFile is now invalid
		m_selected_is_valid = false;
		}
		break;
#endif

	case GAIN_CHANGED:
		GainChanged();
		break;
	case GAIN_CHANGING:
		GainChanging();
		break;
	default:
		BWindow::MessageReceived(message);
		break;
	}
}

status_t 
SoundsWindow::Populate()
{
	BFont font;
	m_maxEventWidth = 0.0f;
	m_maxFileWidth = 0.0f;

	set<entry_ref> files;

	BString event;
	entry_ref ref;
	while (m_media.GetNextRef(&event, &ref) == B_OK)
	{
		float w = font.StringWidth(event.String());
		if (w > m_maxEventWidth) m_maxEventWidth = w;

		BEntry ent;
		const char *fileName = kNoneLabel;
		if (ref.name && *ref.name && ent.SetTo(&ref) == B_OK)
		{
			fileName = ref.name;
			files.insert(ref);
		}
		float gain = 1.0;
		m_media.GetAudioGainFor(BMediaFiles::B_SOUNDS, event.String(), &gain);
		m_eventList->AddItem(new EventItem(event, fileName, gain, &m_maxEventWidth, &m_maxFileWidth));
	}
	
	if (m_eventList->CountItems()) {
		m_eventList->Select(0L);
	}

	BDirectory dir;
	BPath path;
	if (find_directory(B_USER_SOUNDS_DIRECTORY, &path, true) != B_OK) 
		path.SetTo("/boot/home/config/sounds");

	PRINT(("Searching for sound files in %s\n", path.Path()));

	mkdir(path.Path(), 0755);
	BEntry ent;
	dir.SetTo(path.Path());
	while (dir.GetNextEntry(&ent, true) == B_OK) {
		if (ent.IsFile() && ent.GetRef(&ref) == B_OK) {
			PRINT(("Adding ref %s\n", ref.name));
			files.insert(ref);
		}
	}

	BPath p2;
	if (find_directory(B_COMMON_SOUNDS_DIRECTORY, &p2, true) != B_OK)
		p2.SetTo("/boot/home/config/sounds");

	PRINT(("Searching for sound files in %s\n", p2.Path()));

	if (strcmp(p2.Path(), path.Path())) {
		mkdir(path.Path(), 0755);
		dir.SetTo(path.Path());
		while (!dir.GetNextEntry(&ent, true)) 
			if (ent.IsFile() && ent.GetRef(&ref) == B_OK) {
				PRINT(("\tAdding %s\n", ref.name));	
				files.insert(ref);
			}
	}

	if (find_directory(B_BEOS_SOUNDS_DIRECTORY, &path, true)) 
		path.SetTo("/boot/beos/etc/sounds");

	PRINT(("Searching for sound files in %s\n", path.Path()));

	mkdir(path.Path(), 0755);
	dir.SetTo(path.Path());
	while (!dir.GetNextEntry(&ent, true)) 
		if (ent.IsFile() && ent.GetRef(&ref) == B_OK)  {
			PRINT(("\tAdding %s\n", ref.name));	
			files.insert(ref);
		}

	if (!files.empty()) {
		m_currentSelectedFile = *(files.begin());
		m_selected_is_valid = true;	
	}

	for (set<entry_ref>::iterator ptr = files.begin(); ptr != files.end(); ptr++) {
		BMessage *invokeMessage = new BMessage(FILE_SELECTED);
		invokeMessage->AddRef("sounds:ref", &(*ptr)); 
		m_soundFileMenu->AddItem(new BMenuItem((*ptr).name, invokeMessage));
		
		float w = font.StringWidth((*ptr).name);
		if (w > m_maxFileWidth) m_maxFileWidth = w;
	}

	m_soundFileMenu->AddSeparatorItem();

#if ENABLE_REMOVE_FILE
	m_soundFileMenu->AddItem(new BMenuItem(kNoneLabel, new BMessage(SET_NONE)));
#endif

	m_soundFileMenu->AddItem(new BMenuItem("Other"B_UTF8_ELLIPSIS, 
		new BMessage(OTHER_PRESSED)));

	// expand window to fit longest event/file string
	
	float minListWidth = m_maxEventWidth + m_maxFileWidth + kEventMarginWidth;
	float listWidth = m_eventList->Frame().Width();
	if (listWidth < minListWidth)
	{
		ResizeBy(minListWidth - listWidth, 0.0f);
	}

	return B_OK;
}

void SoundsWindow::OtherPressed(BMessage * message)
{
	message = message;
	if (!m_file_panel.IsShowing()) {
		m_file_panel.Show();
	}
	else {
		m_file_panel.Window()->Activate(true);
	}
	m_file_panel.SetTarget(BMessenger(0, this));

	// Set it back to what it was, just in case the user
	// cancels out of the file panel.
	SelectSoundFile(m_currentSelectedFile);
}

void SoundsWindow::StopActiveSound()
{
	if (m_activeSound) {
		release_sem(m_activeSound);
		m_activeSound = -1;
	}
}

void SoundsWindow::PlayPressed()
{
	StopActiveSound();
	int32 eventIndex = m_eventList->CurrentSelection();
	if (eventIndex >= 0)
	{
		// play selected event
		EventItem* item = dynamic_cast<EventItem*>(m_eventList->ItemAt(eventIndex));
		ASSERT(item);
		m_activeSound = system_beep(item->GetEventName().String());
	}
	else
	{
		// play selected file, if any
		if (!m_selected_is_valid || !m_currentSelectedFile.name || !m_currentSelectedFile.name[0]) {
			return;
		}
		m_activeSound = play_sound(&m_currentSelectedFile, true, m_currentGain);
	}
}

void SoundsWindow::StopPressed()
{
	StopActiveSound();
}


void SoundsWindow::EventSelected()
{
	PRINT(("+ SoundsWindow::EventSelected()\n"));

	int32 sel = m_eventList->CurrentSelection();
	if (sel >= 0) {
		EventItem* event = dynamic_cast<EventItem*>(m_eventList->ItemAt(sel));
		ASSERT(event != 0);
		entry_ref ref;
		float gain;

		status_t err = m_media.GetRefFor(BMediaFiles::B_SOUNDS, 
			event->GetEventName().String(), &ref);
		if (err == B_OK)
		{
			err = m_media.GetAudioGainFor(BMediaFiles::B_SOUNDS,
				event->GetEventName().String(), &gain);
		}

		if (err < B_OK) {
			(new BAlert("", strerror(err), "OK"))->Go();
		}
		else {
			if (ref.name && ref.name[0]) {
				SelectSoundFile(ref);
				UpdateGain(gain);
			} else {
				PRINT(("Bogus ref returned from BMediaFiles::GetRefFor()\n"));
				event->SetFileName(kNoneLabel);
				m_selected_is_valid = false;
				UpdateGain(1.0);
				BMenuItem *noneItem = m_soundFileMenu->FindItem(kNoneLabel);
				if (noneItem != NULL) {
					noneItem->SetMarked(true);
				}
			}
		}
	}
}
			
			
void SoundsWindow::SoundFileSelected(BMessage *message)
{
	entry_ref fileRef;
	if (message->FindRef("sounds:ref", &fileRef) == B_OK) {
		m_currentSelectedFile = fileRef;
		m_selected_is_valid = true;
		// Determine which event we should attach a sound to
		int32 eventIndex = m_eventList->CurrentSelection();
		if (eventIndex == -1)
			return;		// Nothing to set

		EventItem *item = dynamic_cast<EventItem*>(m_eventList->ItemAt(	
			eventIndex));
		ASSERT(item != 0);
		const char *eventName = item->GetEventName().String();

		status_t err = m_media.SetRefFor(BMediaFiles::B_SOUNDS, 
			eventName, fileRef);
		if (err < B_OK) 
			(new BAlert("", strerror(err), "OK"))->Go();

		PRINT(("Set file %s for event name %s\n", fileRef.name,
			eventName));

		// Set the ref in the item
		item->SetFileName(fileRef.name);
		m_eventList->Invalidate();
	}
}


BMenuItem* SoundsWindow::SelectSoundFile(const entry_ref & ref)
{
	PRINT(("+ SelectSoundFile(%s)\n", ref.name));
	BMenuItem *item = NULL;
	int32 numFiles = m_soundFileMenu->CountItems();
	for (int32 file = 0; file < numFiles; file++) {
		BMenuItem *tempItem = dynamic_cast<BMenuItem*>(m_soundFileMenu->ItemAt(file));
		if (tempItem && strcmp(tempItem->Label(), ref.name) == 0) {
			// Already opened this file
			item = tempItem;
			break;
		}
	}

	if (item == NULL) {
		// Not found, add it to the list
		BMessage *invokeMessage = new BMessage(FILE_SELECTED);
		invokeMessage->AddRef("sounds:ref", &ref); 
		item = new BMenuItem(ref.name, invokeMessage);
		m_soundFileMenu->AddItem(item,  0);
	} 

	m_currentSelectedFile = ref;
	m_selected_is_valid = (ref.name && ref.name[0]);
	item->SetMarked(true);
	PRINT(("+ SelectSoundFile(%s) OK\n", m_currentSelectedFile.name));
	return item;
}

void 
SoundsWindow::GainChanged()
{
	m_currentGain = float(m_gainSlider->Value()) / 100.0;
	if (!m_selected_is_valid) return;
	// find event whose volume needs to be changed	
	int32 eventIndex = m_eventList->CurrentSelection();
	if (eventIndex == -1)
		return;		// Nothing to set
	EventItem *item = dynamic_cast<EventItem*>(m_eventList->ItemAt(eventIndex));
	ASSERT(item != 0);
	const char *eventName = item->GetEventName().String();

	status_t err = m_media.SetAudioGainFor(BMediaFiles::B_SOUNDS, 
		eventName, m_currentGain);
	if (err < B_OK) 
		(new BAlert("", strerror(err), "OK"))->Go();
}

void 
SoundsWindow::GainChanging()
{
}

void 
SoundsWindow::UpdateGain(float gain)
{
	m_currentGain = gain;
	m_gainSlider->SetValue(int32(gain * 100.0));
}

int main()
{
	BApplication app("application/x-vnd.Be.SoundsPrefs");
	status_t err = 0;
	if (BMediaRoster::Roster(&err) == NULL) {
		char buf[300];
		sprintf(buf, "The media_server does not seem to be running.\n");
		if (err < B_OK) {
			strcat(buf, strerror(err));
	
			(new BAlert("", buf, "Quit",
				NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		}
		
		exit(1);
	}

	try {
		new SoundsWindow;
	} catch(std::exception & e) {
		fprintf(stderr, "class exception.\n");
		(new BAlert("Exception", e.what(), "Quit",
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		app.PostMessage(B_QUIT_REQUESTED);
	} catch(...) {
		fprintf(stderr, "Other exception.\n");
		(new BAlert("Error", "Something failed in the media system.", "Quit",
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		app.PostMessage(B_QUIT_REQUESTED);
	}
	
	app.Run();
	return 0;
}

