//
// TrackListView.cpp
//
//  See TrackListView.h
//
//  by Nathan Schrenk (nschrenk@be.com)

#include <Alert.h>
#include <Application.h>
#include <String.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Window.h>
#include "AudioWrapperDataSource.h"
#include "BurnerWindow.h"
#include "CDDataSource.h"
#include "CDPlayerView.h"
#include "GfxUtils.h"
#include "MediaFileDataSource.h"
#include "TrackListView.h"

const rgb_color kSelectionColor		= {63, 184, 77, 255};
const rgb_color kBlackColor			= {0, 0, 0, 255};

const uint32 kDragRowMessage		= 'dROW';
const uint32 kDeleteRowMessage		= 'delR';

//----------------------------------------------------------------------------------------



TrackLengthColumn::TrackLengthColumn(const char *title, float width, float minWidth, float maxWidth)
	: BSizeColumn(title, width, minWidth, maxWidth)
{
}

void TrackLengthColumn::DrawField(BField *field, BRect rect, BView *parent)
{
	TrackLengthField *tlField = dynamic_cast<TrackLengthField *>(field);
	if (tlField && !tlField->fIsData) {
		char formatted[16];
		uint32 len = tlField->Size();
		// print minutes:seconds:frames into the buffer
		sprintf(formatted, "%02lu:%02lu:%02lu",	len / (60 * 75),
												((len % (60 * 75)) / 75),
												(len % 75));
		font_height fh;
		parent->GetFontHeight(&fh);
		parent->SetHighColor(kBlackColor);
		parent->MovePenTo(rect.left + 8, rect.bottom - FontHeight());			
		parent->DrawString(formatted);
	} else {
		BSizeColumn::DrawField(field, rect, parent);
	}
}

//----------------------------------------------------------------------------------------

// TrackRow takes ownership of the track, and deletes the track when the row is deleted

TrackRow::TrackRow(CDTrack *track)
	: BRow(), fTrack(track), fDeleteTrack(true)
{
	// track #, type, length, pregap, filename
	bool data = track->IsData();
	SetField(new BIntegerField(0), 0);
	SetField(new BStringField(B_EMPTY_STRING), 1);
	SetField(new TrackLengthField(0, data), 2);
//	SetField(new TrackLengthField(0, data), 3);
//	SetField(new BStringField(B_EMPTY_STRING), 4);
	SetField(new BStringField(B_EMPTY_STRING), 3);
	SyncWithTrack();
}

TrackRow::~TrackRow()
{
	if (fDeleteTrack) {
		delete fTrack;
	}
}

CDTrack *TrackRow::Track() const
{
	return fTrack;
}

void TrackRow::SyncWithTrack()
{
	bool data = fTrack->IsData();
	(dynamic_cast<BIntegerField *>(GetField(0)))->SetValue(fTrack->Index());
	(dynamic_cast<BStringField *>(GetField(1)))->SetString(data ? "Data" : "Audio");
	if (data) {
		(dynamic_cast<TrackLengthField *>(GetField(2)))->SetSize(fTrack->Length() * fTrack->FrameSize());
//		(dynamic_cast<TrackLengthField *>(GetField(3)))->SetSize(fTrack->PreGap() * fTrack->FrameSize());
	} else {
		(dynamic_cast<TrackLengthField *>(GetField(2)))->SetSize(fTrack->Length());
//		(dynamic_cast<TrackLengthField *>(GetField(3)))->SetSize(fTrack->PreGap());
	}
//	(dynamic_cast<BStringField *>(GetField(4)))->SetTo(fTrack->DataSource()->Description());
	char *text = fTrack->DataSource()->Description();
	(dynamic_cast<BStringField *>(GetField(3)))->SetString(text);
	free(text);
}

void TrackRow::SetDeleteTrack(bool deleteIt)
{
	fDeleteTrack = deleteIt;
}

bool TrackRow::WillDeleteTrack() const
{
	return fDeleteTrack;
}


//----------------------------------------------------------------------------------------

class TrackListStatusView : public BView
{
public:
				TrackListStatusView(BRect frame);
	virtual		~TrackListStatusView();

	void		Update();
protected:

	virtual void Draw(BRect updateArea);
	virtual void MessageReceived(BMessage *message);
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	
private:
	BurnerWindow	*fWindow;
	TrackListView	*fParent;
	int32			fTrackCount;
	int32			fTotalFrames;
};

TrackListStatusView::TrackListStatusView(BRect frame)
	: BView(frame, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	fTrackCount = 0;
	fTotalFrames = 0;
	fWindow = NULL;
	fParent = NULL;
	SetDoubleBuffering(B_UPDATE_RESIZED | B_UPDATE_INVALIDATED);
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
}

TrackListStatusView::~TrackListStatusView()
{
}

void TrackListStatusView::Update()
{
	CDTrack *track;
	if (fParent != NULL) {		
		fTrackCount = 0;
		fTotalFrames = 0;
		track = fParent->GetTrackList();
		while (track != NULL) {
			fTrackCount++;
			fTotalFrames += track->Length();
			track = track->Next();
		}
		Invalidate();
	}
}


void TrackListStatusView::Draw(BRect /*updateArea*/)
{
	BRect rect(Bounds());
	char formatted[28];
	uint32 len = fTotalFrames;
	// write MM:SS into the buffer
	sprintf(formatted, "Tracks: %ld, Length: %02lu:%02lu", fTrackCount,
			len / (60 * 75), ((len % (60 * 75)) / 75));
	font_height fh;
	GetFontHeight(&fh);
	MovePenTo(rect.left + (rect.Width() - StringWidth(formatted)) / 2,
			  floor(rect.bottom - fh.descent));			
	DrawString(formatted);
}

void TrackListStatusView::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case BurnerWindow::TRACK_ADDED:		// fall through
	case BurnerWindow::TRACK_DELETED:
		Update();
		break;
	case kDeleteRowMessage:
		{
			TrackRow *row(NULL);
			if (message->FindPointer("row", (void **)&row) == B_OK) {
				delete row;
			}
		}
	default:
		BView::MessageReceived(message);
	}
}

void TrackListStatusView::AttachedToWindow()
{
	fWindow = dynamic_cast<BurnerWindow *>(Window());
	if (fWindow != NULL) {
		fWindow->AddTrackListener(this);
	}
	fParent = dynamic_cast<TrackListView *>(Parent());

	// set font to 10 point regular
	BFont font;
	GetFont(&font);
	font.SetSize(10);
	font.SetFace(B_REGULAR_FACE);
	SetFont(&font);
}

void TrackListStatusView::DetachedFromWindow()
{
	fParent = NULL;
	if (fWindow != NULL) {
		fWindow->RemoveTrackListener(this);
		fWindow = NULL;
	}
}


//----------------------------------------------------------------------------------------

TrackListView::TrackListView(BRect frame, uint32 resizeMode)
	: BColumnListView(frame, "TrackListView", resizeMode, B_NAVIGABLE)
{
	
	SetSortingEnabled(false);
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
	SetSelectionColor(kSelectionColor);
	AddColumn(new BIntegerColumn("Track", 42, 42, 42), 0);
	AddColumn(new BStringColumn("Type", 40, 40, 100, B_ALIGN_LEFT), 1);
	AddColumn(new TrackLengthColumn("Length", 54, 54, 100), 2);
//	AddColumn(new TrackLengthColumn("Pre-gap", 60, 60, 100), 3);
//	AddColumn(new BStringColumn("File", 200, 200, 500, B_ALIGN_LEFT), 4);
	AddColumn(new BStringColumn("File", 200, 200, 500, B_ALIGN_LEFT), 3);
	
	BFont font;
	GetFont(&font);
	font.SetSize(10);
	BRect rect(0, 0, font.StringWidth("Tracks: MM, Length: MM:MM") + 4, 10);
	fStatusView = new TrackListStatusView(rect);
	AddStatusView(fStatusView);
	SetMouseTrackingEnabled(true); // show an insertion point for drag-n-drop
	fEditable = true;
}


TrackListView::~TrackListView()
{
}


void TrackListView::AddTrack(CDTrack *track, int16 index)
{
	int16 defaultIndex;
	map<CDTrack *, TrackRow *, ltTrack>::reverse_iterator tIter;
	
	if (tracks.empty()) {
		defaultIndex = 1;
	} else {
		// find the last track, and use the last index + 1
		tIter = tracks.rbegin();
		defaultIndex = tIter->first->Index() + 1;
	}

	if (index < 1 || index > defaultIndex) {
		index = defaultIndex;
	}
	
	// 99 tracks is the maximum allowed on a CD
	if (index > 99) {
		BString str("A CD may not contain more than 99 tracks.  If you wish ");
		str << "to add another track to this project, you must "
			<< " remove one or more of the existing tracks.\n";
		BAlert *alert = new BAlert("Too Many Tracks", str.String(), "Continue",
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go(NULL);
		delete track;
		return;
	}
	
	if (index != defaultIndex) {
		// adding track before end, so must renumber all tracks after it
		tIter = tracks.rbegin();
		while (tIter != tracks.rend()) {
			int32 i = tIter->first->Index();
			if (i >= index) {
				tIter->first->SetIndex(i + 1);
			} 
			++tIter;
		}
	}
	
	track->SetIndex(index);
	TrackRow *row = new TrackRow(track);
	tracks[track] = row;
	AddRow(row, index - 1);
	NumberTracks();

	// broadcast add message to listeners
	BurnerWindow *win = dynamic_cast<BurnerWindow *>(Window());
	if (win) {
		BMessage addMsg(BurnerWindow::TRACK_ADDED);
		addMsg.AddPointer("tracklist", this);
		win->SendTrackMessage(&addMsg);
	}
}

status_t TrackListView::RemoveTrack(CDTrack *track)
{
	status_t ret = B_OK;
	TrackRow *row = tracks[track];
	if (row != NULL) {
		tracks.erase(track);
		NumberTracks();
		RemoveRow(row);
		
		// broadcast delete message to listeners
		BurnerWindow *win = dynamic_cast<BurnerWindow *>(Window());
		if (win) {
			BMessage delMsg(BurnerWindow::TRACK_DELETED);
			delMsg.AddPointer("tracklist", this);
			win->SendTrackMessage(&delMsg);

			// have to delete row after all the listeners are informed, otherwise there
			// is a race whereby some listeners can attempt to use the track to draw, etc,
			// after it is deleted.
			BMessage deleteRowMsg(kDeleteRowMessage);
			deleteRowMsg.AddPointer("row", row);
			win->PostMessage(&deleteRowMsg, this);
		}
		
		
	} else {
		ret = B_ERROR;
	}
	return ret;
}

void TrackListView::RemoveAllTracks()
{
	if (!tracks.empty()) {
		
		tracks.clear(); // empties the map
		Clear(); // deletes all the TrackRows, which deletes the tracks
		
		// broadcast delete message to listeners
		BurnerWindow *win = dynamic_cast<BurnerWindow *>(Window());
		if (win) {
			BMessage delMsg(BurnerWindow::TRACK_DELETED);
			delMsg.AddPointer("tracklist", this);
			win->SendTrackMessage(&delMsg);
		}
	}
}

void TrackListView::NumberTracks()
{
	if (!tracks.empty()) {
		CDTrack *track(NULL), *prev(NULL);
		map<CDTrack *, TrackRow *, ltTrack>::iterator tIter(tracks.begin());			
		int32 i = 1;
		while (tIter != tracks.end()) {
			prev = track;
			track = tIter->first;
			track->SetIndex(i++);
			if (prev != NULL) {
				prev->SetNext(track);
			}
			tIter->second->SyncWithTrack();
			UpdateRow(tIter->second);
			++tIter;
		}		
		track->SetNext(NULL);
	}	
}


void TrackListView::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);	
}

void TrackListView::MessageDropped(BMessage *msg, BPoint point)
{
	switch (msg->what) {
	case B_SIMPLE_DATA: // handle file drags from Tracker
		if (fEditable) {
			BWindow *win(Window());
			win->DetachCurrentMessage();
			msg->what = B_REFS_RECEIVED;
			msg->RemoveName("window");
			msg->AddPointer("window", win);

			BRow *row = RowAt(point);
			if (row != NULL) {
				int32 index = IndexOf(row);
				if (index >= 0) {
					BRect rowRect;
					GetRowRect(row, &rowRect);
//		printf("was dropped && row = %p, point = (%f, %f), rect = (%f, %f, %f, %f)\n",
//			row, dropPoint.x, dropPoint.y, rowRect.left, rowRect.top, rowRect.right, rowRect.bottom);
					if (point.y > rowRect.top + (row->Height() / 2)) {
						index++;
					}
					msg->AddInt32("insertion_index", ++index);
				}
			}
			
			be_app->PostMessage(msg);
		}
		break;
	case kDragRowMessage: // handle drag-reordering of tracks
		if (fEditable) {
			TrackRow *row;
			//BurnerWindow *window;
			// XXX: need to look at msg->FindPointer("window", (void **)&window) to
			//      see if the message was dragged out of this window or another one
			if (msg->FindPointer("row", (void **)&row) == B_OK) {
//				BPoint dropPoint(ConvertFromScreen(msg->DropPoint()));
//				BRect bounds(Bounds());
//				dropPoint.y += bounds.top;
//				dropPoint.x += bounds.left;
				BRow *prevRow = RowAt(point);
				int32 origIndex = IndexOf(row);
				int32 destIndex;
				if (prevRow != NULL) {
					destIndex = IndexOf(prevRow) + 1;
					BRect rowRect;
					GetRowRect(prevRow, &rowRect);
					if (point.y < rowRect.top + (prevRow->Height() / 2)
						&& destIndex > 0)
					{
						destIndex--;
					}
				} else {
					// if prevRow == NULL, we must be inserting after all current tracks
					destIndex = CountRows();
				}
				
				if (origIndex != destIndex) {
					CDTrack *track = row->Track();
					RowForTrack(track)->SetDeleteTrack(false);
					RemoveTrack(track);
					if (origIndex > destIndex) {
						destIndex++;
					}
					AddTrack(track, destIndex);
					AddToSelection(RowForTrack(track));
					SelectionChanged();
				}
			}
		}
	default:
		break;
	}
}

void TrackListView::ItemInvoked()
{
	// send a message which will result in playing the currently
	// selected track, or pausing the playing track.
	BMessage playMsg(kTrackInvokedMessage);
	playMsg.AddPointer("tracklist", this);
	BWindow *win = Window();
	BView *view = win->FindView(CDPLAYERVIEWNAME);
	win->PostMessage(&playMsg, view);
}



void TrackListView::SelectionChanged()
{
	BurnerWindow *win = dynamic_cast<BurnerWindow *>(Window());
	if (win) {
		BMessage selMsg(BurnerWindow::TRACK_SELECTION_CHANGED);
		selMsg.AddPointer("tracklist", this);
		win->SendTrackMessage(&selMsg);
	}
	// call superclass method
	BColumnListView::SelectionChanged();
}

void TrackListView::KeyDown(const char *bytes, int32 numBytes)
{
	BMessage *msg = NULL;
	bool sendMsgToCDPlayer = false;

	switch (bytes[0]) {
	case B_DELETE:
		RemoveSelectedTracks();
		break;
//	case B_SPACE:
//		// change space into invoke, which is normally return
//		if (CurrentSelection() != NULL) {
//			ItemInvoked();
//		}
//		break;
//	case B_LEFT_ARROW:
//		// seek backward
//		msg = new BMessage(kSeekBackButtonMessage);
//		sendMsgToCDPlayer = true;
//		break;
//	case B_RIGHT_ARROW:
//		// seek forward
//		msg = new BMessage(kSeekForwardButtonMessage);
//		sendMsgToCDPlayer = true;	
//		break;
//	case B_ESCAPE:
//		// stop playback
//		msg = new BMessage(kStopButtonMessage);
//		sendMsgToCDPlayer = true;
//		break;
// XXX: need some way to reorder tracks from the keyboard?
//	case B_DOWN_ARROW:
//		{
//			if ((modifiers() & B_SHIFT_KEY) != 0) {
//				
//			}
//		}
//		break;
	default:
		BColumnListView::KeyDown(bytes, numBytes);
		break;
	}
	
	if (sendMsgToCDPlayer) {
		BWindow *win = Window();
		BView *cd = win->FindView(CDPLAYERVIEWNAME);
		win->PostMessage(msg, cd);
		delete msg;
	}
}

bool TrackListView::InitiateDrag(BPoint point, bool wasSelected)
{
	TrackRow *row = dynamic_cast<TrackRow *>(RowAt(point));
	if (row != NULL && fEditable) {
		if (!wasSelected) {
			AddToSelection(row);
			SelectionChanged();
		}
		BMessage dragMsg(kDragRowMessage);
		dragMsg.AddPointer("row", row);
		dragMsg.AddPointer("window", Window());
		// XXX: instead of rect, should create drag bitmap of track description
		//BBitmap *dragBitmap = new BBitmap(BRect(0, 0, 31, 31), B_RGB32, true);
		// draw into bitmap;
		//DragMessage(&dragMsg, dragBitmap, BPoint(15, 15));
		BRect dragSize(0, point.y, Bounds().Width(), point.y + row->Height());
		DragMessage(&dragMsg, dragSize, NULL);
		char *desc = row->Track()->DataSource()->Description();
		//printf("Dragging track '%s'\n", desc);
		free(desc);
	}
	return false;
}


// Links the tracks together and returns a pointer to the head of the list
CDTrack *TrackListView::GetTrackList()
{
	if (tracks.empty()) {
		return NULL;
	} else {
		map<CDTrack *, TrackRow *, ltTrack>::iterator tIter = tracks.begin();			
		return tIter->first;
	}
}

TrackRow *TrackListView::RowForTrack(CDTrack *track)
{
	return tracks[track];
}

void TrackListView::TrackUpdated(CDTrack *track)
{
	TrackRow *row = RowForTrack(track);
	if (row) {
		row->SyncWithTrack();
		UpdateRow(row);
	}
	fStatusView->Update();
}


void TrackListView::RemoveSelectedTracks()
{
	if (fEditable) {
		TrackRow *selectedRow = dynamic_cast<TrackRow *>(CurrentSelection());
		if (selectedRow != NULL) {
			int32 selectedIndex = IndexOf(selectedRow);
			while ((selectedRow = dynamic_cast<TrackRow *>(CurrentSelection())) != NULL) {
				RemoveTrack(selectedRow->Track());
			}
			int32 rows = CountRows();
			if (selectedIndex < rows) {
				AddToSelection(RowAt(selectedIndex));
			} else if (rows > 0) {
				AddToSelection(RowAt(rows - 1));
			}
		}
	}
}


void TrackListView::SetEditEnabled(bool enabled)
{
	if (fEditable != enabled) {
		fEditable = enabled;
		SetMouseTrackingEnabled(enabled);
		Invalidate();
	}
}

bool TrackListView::EditEnabled()
{
	return fEditable;
}


