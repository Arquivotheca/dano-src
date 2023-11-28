#include "Sounds.h"

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
#include <FilePanel.h>
#include <Screen.h>
#include <Alert.h>
#include <ListItem.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <MediaRoster.h>
#include <Path.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <InterfaceDefs.h>
#include <StringView.h>
#include <Debug.h>

#include <MediaFiles.h>
#include <PlaySound.h>
#include <Sound.h>
#include <MenuField.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PPAddOn.h"

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

const int32 READING_DONE = 'rdon';
const int32 OTHER_PRESSED = 'othr';
const int32 PLAY_PRESSED = 'plpr';
const int32 STOP_PRESSED = 'stpr';
const int32 EVENT_SELECTED = 'itsl';
const int32 FILE_SELECTED = 'fisl';

#if ENABLE_REMOVE_FILE
	const int32 SET_NONE = 'setn';
#endif

const char *kNoneLabel = "<none>";
const char *kFileMenuLabel = "Sound File:";
const float kButtonWidth = 80;
const float kButtonHeight = 20;
const float kLabelHeight = 25;
const float kEventColumnWidth = 100;

const rgb_color kListHighlightColor = { 205, 205, 205, 255 };
const rgb_color kWhite = { 255, 255, 255, 255 };
const rgb_color kBlack = { 0, 0, 0, 255 };

class EventItem : public BListItem {
public:
	
	EventItem(const BString &eventName, const BString &fileName);

	const BString&	GetEventName();
	void 			SetFileName(const BString &name);
	virtual void 	DrawItem(BView*, BRect, bool = false);

private:

	BString eventName;
	BString	fileName;

};

EventItem::EventItem(const BString &newEventName, const BString &newFileName)
	:	eventName(newEventName),
		fileName(newFileName)
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

void EventItem::DrawItem(BView *view, BRect rect, bool /*complete*/)
{
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
	view->MovePenTo(rect.left + 5 + kEventColumnWidth, baseline);
	view->DrawString(fileName.String());
}


status_t SoundsView::file_getter(void * castToSoundWindow)
{
	SoundsView *soundWindow = (SoundsView*) castToSoundWindow;

	soundWindow->m_media = new BMediaFiles;
	status_t err = soundWindow->m_media->RewindRefs(BMediaFiles::B_SOUNDS);
	BMessage msg(READING_DONE);
	BString buf;
	msg.AddInt32("sounds:error", err);

	entry_ref ref;
	if (err == B_OK)
		while (soundWindow->m_media->GetNextRef(&buf, &ref) == B_OK) {
			msg.AddString("sounds:event", buf.String());
			BEntry ent;
			if (ref.name != 0 && ref.name[0] != 0 && ent.SetTo(&ref) == B_OK) {
				msg.AddRef("sounds:ref", &ref);
			} 
		}

	if (err == B_OK) {
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
				msg.AddRef("sounds:ref", &ref);
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
					msg.AddRef("sounds:ref", &ref);
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
				msg.AddRef("sounds:ref", &ref);
			}
	}
	
	soundWindow->m_thread = -1;
	BMessenger m(soundWindow);
	m.SendMessage(&msg);
	return 0;
}


SoundsView::SoundsView(PPAddOn *adn) 
 : BView(BRect(0, 0, 350, 310), "Sounds", B_FOLLOW_NONE, B_WILL_DRAW),
			addon(adn), m_file_panel(0),
			m_sound_sem(-1), m_sound_started(false), m_selected_is_valid(false)
{
	try {
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

		m_thread = spawn_thread(file_getter, "file_getter", B_NORMAL_PRIORITY, this);
		resume_thread(m_thread);
//		BRect r(BScreen().Frame());
//		float width = r.Width()-30;
//		float height = r.Height()-30;
//		SetSizeLimits(300,width, 200,height);
//		SetZoomLimits((width < 500) ? width : 500, height);

		// Main BBox
		BRect mainViewRect(Bounds());
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		// Event List
		BRect eventRect(mainViewRect);
//		eventRect.InsetBy(13, 13);
		eventRect.InsetBy(3, 3);
		eventRect.bottom -= 100;
		eventRect.right -= B_V_SCROLL_BAR_WIDTH;
		eventRect.top = kLabelHeight + 3;
	
		// 'Event' title
		BRect eventTitleRect(eventRect.left + 3, 5, eventRect.left + 50, 
			kLabelHeight - 3);
		BStringView *eventTitleLabel = new BStringView(eventTitleRect, "", "Event");
		eventTitleLabel->SetViewColor(222, 222, 222);
		eventTitleLabel->SetFont(be_bold_font);
		AddChild(eventTitleLabel);
	
		// 'File' title	
		BRect fileTitleRect(eventRect.left + kEventColumnWidth + 3, 
			5, eventRect.left + kEventColumnWidth + 50, kLabelHeight - 3);
		BStringView *fileTitleLabel = new BStringView(fileTitleRect, "", "Sound");
		fileTitleLabel->SetViewColor(222, 222, 222);
		fileTitleLabel->SetFont(be_bold_font);
		AddChild(fileTitleLabel);
	
		// Event List View
		eventList = new BListView(eventRect, "Event List",
			B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
		eventList->SetSelectionMessage(new BMessage(EVENT_SELECTED));
		BScrollView *eventScroller = new BScrollView("Event Scroller", 
			eventList, B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
		AddChild(eventScroller);
	
		// Control BBox
		BRect controlRect(mainViewRect);
//		controlRect.InsetBy(12, 12);
		controlRect.InsetBy(2, 2);
		controlRect.top = controlRect.bottom - 87;
		BBox *controlView = new BBox(controlRect, "Control Box", 
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
		AddChild(controlView);
	
		// Sound file menu field
		soundFileMenu = new BPopUpMenu(kNoneLabel);
		BRect menuFieldRect(controlRect);
		menuFieldRect.OffsetTo(0, 0);
		menuFieldRect.InsetBy(15, 10);
		menuFieldRect.bottom = menuFieldRect.top + 15;
		menuField = new BMenuField(menuFieldRect, "FileList", kFileMenuLabel,
			soundFileMenu);
		menuField->SetDivider(be_plain_font->StringWidth(kFileMenuLabel) + 10);
		controlView->AddChild(menuField);
	
		// Determine if this machine has some sort of sound output
		// (like a sound card).  If not, disable play and stop buttons.
		bool hasOutputNode = false;
		BMediaRoster *roster = BMediaRoster::Roster();
		media_node outputNode;
		live_node_info outputNodeInfo;
		err = roster->GetAudioOutput(&outputNode);
		if (err == B_OK) {
			err = roster->GetLiveNodeInfo(outputNode, &outputNodeInfo);
			if (err == B_OK)
				hasOutputNode = (outputNodeInfo.name != 0);
		}	
	
		// Stop button	
		BRect stopRect(controlRect);
		stopRect.OffsetTo(0, 0);
		stopRect.InsetBy(12, 19);
		stopRect.left = stopRect.right - kButtonWidth;
		stopRect.top = stopRect.bottom - kButtonHeight;
		stopButton = new BButton(stopRect, "stop button", "Stop", 
			new BMessage(STOP_PRESSED), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		controlView->AddChild(stopButton);
		if (!hasOutputNode)
			stopButton->SetEnabled(false);
		
		// Play button	
		BRect playRect(stopRect);
		playRect.OffsetBy(-(kButtonWidth + 10), 0);
		playButton = new BButton(playRect, "play button", "Play", 
			new BMessage(PLAY_PRESSED), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		controlView->AddChild(playButton);
		if (!hasOutputNode)
			playButton->SetEnabled(false);
	} catch(exception & e) {
		fprintf(stderr, "class exception.\n");
		(new BAlert("Exception", e.what(), "Quit",
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
// TODO: remove this panel
//		app.PostMessage(B_QUIT_REQUESTED);
	} catch(...) {
		fprintf(stderr, "Other exception.\n");
		(new BAlert("Error", "Something failed in the media system.", "Quit",
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
// TODO: remove this panel
//		app.PostMessage(B_QUIT_REQUESTED);
	}
}

SoundsView::~SoundsView()
{
	if (m_sound_started) {
		m_sound_started = false;
		stop_sound(m_sound_sem);
		m_sound_sem = -1;
	}

	status_t s;
	if (m_thread >= 0)
		wait_for_thread(m_thread, &s);

	delete m_file_panel;
}

void SoundsView::AttachedToWindow()
{
	soundFileMenu->SetTargetForItems(this);
	stopButton->SetTarget(this);
	playButton->SetTarget(this);
	eventList->SetTarget(this);
}

void SoundsView::MessageReceived(BMessage * message)
{
	if (message->WasDropped() || message->what == B_REFS_RECEIVED) {
		entry_ref fileRef;
		if (message->FindRef("refs", &fileRef) == B_OK) {
			BMenuItem *item = SelectSoundFile(fileRef);
			ASSERT(item != 0);
			MessageReceived(item->Message());
		}
		return;		
	}

	switch (message->what) {
	case READING_DONE:
		ReadingDone(message);
		break;
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
	case 'doit' :
		addon->SwitchToPanel();
		break;

#if ENABLE_REMOVE_FILE
	case SET_NONE: {
		int32 eventIndex = eventList->CurrentSelection();
		EventItem *item = dynamic_cast<EventItem*>(eventList->ItemAt(	
			eventIndex));
		ASSERT(item != 0);
		const char *eventName = item->GetEventName().String();

		entry_ref oldRef;	
		if (m_media->GetRefFor(BMediaFiles::B_SOUNDS, eventName, &oldRef) == B_OK)  {
			m_media->RemoveRefFor(BMediaFiles::B_SOUNDS, eventName, oldRef);
		}

		PRINT(("Removed event for event name %s\n", eventName));

		// Set the ref in the item
		item->SetFileName(kNoneLabel);
		eventList->Invalidate();
		
		// set flag to note that currentSelectedFile is now invalid
		m_selected_is_valid = false;
		}
		break;
#endif

	default:
		BView::MessageReceived(message);
		break;
	}
}


void SoundsView::ReadingDone(BMessage * message)
{
	int32 err = 0;
	m_thread = -1;
	message->FindInt32("sounds:error", &err);
	if (err < B_OK) {
		(new BAlert("", strerror(err), "OK"))->Go();
// TODO: close panel
//		PostMessage(B_QUIT_REQUESTED);
	} else {

		// Fill in event list
		const char *eventName = "";
		for (int ix=0; message->FindString("sounds:event", ix, &eventName) == B_OK; ix++) {
			const char *fileName = kNoneLabel;
			entry_ref ref;
			if ((m_media->GetRefFor(BMediaFiles::B_SOUNDS, eventName, &ref) == B_OK)
				&& ref.name && ref.name[0])
			{
				fileName = ref.name;
			}
			eventList->AddItem(new EventItem(eventName, fileName));
		}

		if (eventList->CountItems()) {
			eventList->Select(0L);
		}

		// Fill in sound file list
		entry_ref ref;
		set<entry_ref> s;
		for (int ix=0; !message->FindRef("sounds:ref", ix, &ref); ix++)
			s.insert(ref);					
		
		if (!s.empty()) {
			currentSelectedFile = *(s.begin());
			m_selected_is_valid = true;	
		}

		for (set<entry_ref>::iterator ptr = s.begin(); ptr != s.end(); ptr++) {
			BMessage *invokeMessage = new BMessage(FILE_SELECTED);
			invokeMessage->AddRef("sounds:ref", &(*ptr)); 
			soundFileMenu->AddItem(new BMenuItem((*ptr).name, invokeMessage));
		}

		soundFileMenu->AddSeparatorItem();

#if ENABLE_REMOVE_FILE
		soundFileMenu->AddItem(new BMenuItem(kNoneLabel, new BMessage(SET_NONE)));
#endif

		soundFileMenu->AddItem(new BMenuItem("Other"B_UTF8_ELLIPSIS, 
			new BMessage(OTHER_PRESSED)));

		soundFileMenu->SetTargetForItems(this);
	}
}

void SoundsView::OtherPressed(BMessage * message)
{
	message = message;

	if(! m_file_panel)
		m_file_panel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, 0, false);

	if(!m_file_panel->IsShowing()) {
		m_file_panel->Show();
	}
	else {
		m_file_panel->Window()->Activate(true);
	}
	m_file_panel->SetTarget(BMessenger(this));

	// Set it back to what it was, just in case the user
	// cancels out of the file panel.
	SelectSoundFile(currentSelectedFile);
}

void SoundsView::PlayPressed()
{
	if (m_sound_started) {
		stop_sound(m_sound_sem);
		m_sound_sem = -1;
		m_sound_started = false;
	}

	if (!m_selected_is_valid || !currentSelectedFile.name || !currentSelectedFile.name[0]) {
		return;
	}

	m_sound_started = true;
	m_sound_sem = play_sound(&currentSelectedFile, true, false, true); // *ref, mix, queue, background
}

void SoundsView::StopPressed()
{
	if (m_sound_started) {
		stop_sound(m_sound_sem);
		m_sound_started = false;
		m_sound_sem = -1;
	}
}


void SoundsView::EventSelected()
{
	status_t s;
	if (m_thread >= 0)
		wait_for_thread(m_thread, &s);

	int32 sel = eventList->CurrentSelection();
	if (sel >= 0) {
		EventItem* event = dynamic_cast<EventItem*>(eventList->ItemAt(sel));
		ASSERT(event != 0);
		entry_ref ref;

		status_t err = m_media->GetRefFor(BMediaFiles::B_SOUNDS, 
			event->GetEventName().String(), &ref);

		if (err < B_OK) {
			(new BAlert("", strerror(err), "OK"))->Go();
		}
		else {
			if (ref.name && ref.name[0]) {
				SelectSoundFile(ref);
			} else {
				PRINT(("Bogus ref returned from BMediaFiles::GetRefFor()\n"));
				event->SetFileName(kNoneLabel);
				m_selected_is_valid = false;
				BMenuItem *noneItem = soundFileMenu->FindItem(kNoneLabel);
				if (noneItem != NULL) {
					noneItem->SetMarked(true);
				}
			}
		}
	}
}
			
			
void SoundsView::SoundFileSelected(BMessage *message)
{
	status_t s;
	if (m_thread >= 0)
		wait_for_thread(m_thread, &s);

	entry_ref fileRef;
	if (message->FindRef("sounds:ref", &fileRef) == B_OK) {
		currentSelectedFile = fileRef;
		m_selected_is_valid = true;
		// Determine which event we should attach a sound to
		int32 eventIndex = eventList->CurrentSelection();
		if (eventIndex == -1)
			return;		// Nothing to set

		EventItem *item = dynamic_cast<EventItem*>(eventList->ItemAt(	
			eventIndex));
		ASSERT(item != 0);
		const char *eventName = item->GetEventName().String();

		status_t err = m_media->SetRefFor(BMediaFiles::B_SOUNDS, 
			eventName, fileRef);
		if (err < B_OK) 
			(new BAlert("", strerror(err), "OK"))->Go();

		PRINT(("Set file %s for event name %s\n", fileRef.name,
			eventName));

		// Set the ref in the item
		item->SetFileName(fileRef.name);
		eventList->Invalidate();
	}
}


BMenuItem* SoundsView::SelectSoundFile(const entry_ref & ref)
{
	BMenuItem *item = NULL;
	int32 numFiles = soundFileMenu->CountItems();
	for (int32 file = 0; file < numFiles; file++) {
		BMenuItem *tempItem = dynamic_cast<BMenuItem*>(soundFileMenu->ItemAt(file));
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
		soundFileMenu->AddItem(item,  0);
	} 

	soundFileMenu->SetTargetForItems(this);

	currentSelectedFile = ref;
	m_selected_is_valid = (ref.name && ref.name[0]);
	item->SetMarked(true);
	return item;
}
