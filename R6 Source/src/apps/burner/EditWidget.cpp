//
// EditWidget.cpp
//

#include <stdio.h>
#include <SupportDefs.h>	// for min_c()
#include <Bitmap.h>
#include "AudioWrapperDataSource.h"
#include "BurnerWindow.h"
#include "EditWidget.h"
#include "GfxUtils.h"
#include "TrackEditView.h"
#include "TrackListView.h"

const float kTopBarHeight		= 12.0f;
const float kBottomBarHeight	= 30.0f;
const float kArcRadius			=  6.0f;
const float kUpperLineLength	=  7.0f;
const float kLowerLineLength	=  7.0f;
const float kPointerDescent		= 14.0f;

const float kBracketHeight		= 36.0f;
const float kBracketWidth		=  7.0f;
const float kTopBracketHeight	= 14.0f;
const float kTopBracketWidth	=  4.0f;

const rgb_color kLineColor		= {0, 0, 0, 255};
const rgb_color kBarColor		= {140, 255, 154, 255};
const rgb_color kSelectedColor	= {63, 184, 77, 255};
const rgb_color kTrackTextColor	= {0, 0, 0, 255};
const rgb_color kControlColor	= {248, 0, 0, 255};
const rgb_color kEmptyColor		= {112, 112, 112, 255};
const rgb_color kUnusedColor	= {74, 74, 74, 255};


EditWidget::EditWidget(BRect frame, uint32 resizingMode)
	: BView(frame, "EditWidget", resizingMode, B_WILL_DRAW |
		B_NAVIGABLE | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
	fHeadTrack = NULL;
	fSelectedTrack = NULL;
	fEnabled = true;
	fEditEnabled = false;
	fWindow = NULL;
	fMovingControl = NONE;
	fFocusedControl = NONE;
	fMaxFrames = 74 * 60 * 75; // default to 74 minutes
	fTrackListView = NULL;
	fTrackEditView = NULL;
	
	frame.OffsetTo(0, 0);
	fBounds = frame;
	CalculateRects(fBounds);
	SetDoubleBuffering(B_UPDATE_RESIZED | B_UPDATE_INVALIDATED);
}


EditWidget::~EditWidget()
{
}

void EditWidget::SetEnabled(bool enabled)
{
	if (enabled != fEnabled) {
		fEnabled = enabled;
		fEditEnabled = enabled && (fSelectedTrack != NULL);
		InvalidateBuffer();
	}
}

bool EditWidget::IsEnabled()
{
	return fEnabled;
}


void EditWidget::SetTrackList(CDTrack *track)
{
	fHeadTrack = track;
	CDTrack *aTrack = fHeadTrack;
	bool foundSelected = false;
	while (aTrack != NULL) {
		if (aTrack == fSelectedTrack) {
			foundSelected = true;
			break;
		}
		aTrack = aTrack->Next();
	}
	if (!foundSelected) {
		fSelectedTrack = NULL;
	}
	InvalidateBuffer();
}

CDTrack *EditWidget::TrackList()
{
	return fHeadTrack;
}


void EditWidget::SetSelectedTrack(CDTrack *track, bool selectInList)
{
	if (track != fSelectedTrack) {
		fSelectedTrack = track;
		if (selectInList && fTrackListView != NULL) {
			fTrackListView->DeselectAll();
			if (fSelectedTrack != NULL) {
				TrackRow *row = fTrackListView->RowForTrack(fSelectedTrack);
				fTrackListView->AddToSelection(row);
			}
			BMessage selMsg(BurnerWindow::TRACK_SELECTION_CHANGED);
			selMsg.AddPointer("tracklist", fTrackListView);
			fWindow->SendTrackMessage(&selMsg);
		}
		if (track != NULL) {
			// only accept AudioWrapperDataSource tracks
			AudioWrapperDataSource *src = dynamic_cast<AudioWrapperDataSource *>(track->DataSource());
			if (src == NULL) {
				fSelectedTrack = NULL;
			}
		} 
		fEditEnabled = fEnabled && (fSelectedTrack != NULL);
		InvalidateBuffer();
	}
}


void EditWidget::FrameResized(float width, float height)
{
	fBounds.Set(0, 0, width, height);
	CalculateRects(fBounds);
}

void EditWidget::AttachedToWindow()
{
	SetBackgroundColor();
	fWindow = dynamic_cast<BurnerWindow *>(Window());
	if (fWindow != NULL) {
		fWindow->AddTrackListener(this);
		fTrackListView = dynamic_cast<TrackListView *>(fWindow->FindView("TrackListView"));
		if (fTrackListView != NULL) {
			TrackRow *row = dynamic_cast<TrackRow *>(fTrackListView->CurrentSelection());
			if (row != NULL) {
				SetSelectedTrack(row->Track(), false);
			}
		}
	}
}

void EditWidget::DetachedFromWindow()
{
	fWindow->RemoveTrackListener(this);
	fWindow = NULL;
	fTrackEditView = NULL;
	fTrackListView = NULL;
}

void EditWidget::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case BurnerWindow::TRACK_ADDED:		// fall through
	case BurnerWindow::TRACK_DELETED:	// fall through
	case BurnerWindow::TRACK_MOVED:
		if (msg->FindPointer("tracklist", (void **)&fTrackListView) == B_OK) {
			SetTrackList(fTrackListView->GetTrackList());
		}
		break;
	case BurnerWindow::TRACK_SELECTION_CHANGED:
		{
			if (msg->FindPointer("tracklist", (void **)&fTrackListView) == B_OK) {
				TrackRow *row = dynamic_cast<TrackRow *>(fTrackListView->CurrentSelection());
				CDTrack *track(NULL);
				if (row) {
					track = row->Track();
				}
				SetSelectedTrack(track, false);
			}
		}
		break;
	default:
		BView::MessageReceived(msg);
	}
}

void EditWidget::MouseDown(BPoint where)
{
	bool inval = false;
	
	fClickPoint = where;
	fLastPoint = where;
	fValueChanged = false;
	SetMouseEventMask(B_POINTER_EVENTS, 0);
		
	if (fSelectedTrack != NULL) {
		// see if the click hit a control
		if (fEditEnabled && LeftFaderRect().Contains(where)) {
			fMovingControl = LEFT_FADER;
			fClickOffset = where - LeftFaderRect().LeftTop();
		} else if (fEditEnabled && RightFaderRect().Contains(where)) {
			fMovingControl = RIGHT_FADER;
			fClickOffset = where - RightFaderRect().LeftTop();
		} else if (fEditEnabled && LeftBracketRect().Contains(where)) {
			fMovingControl = LEFT_BRACKET;
			fClickOffset = where - LeftBracketRect().LeftTop();
		} else if (fEditEnabled && RightBracketRect().Contains(where)) {
			fMovingControl = RIGHT_BRACKET;	
			fClickOffset = where - RightBracketRect().LeftTop();
		} else if (fEditEnabled && fPregapPointerRect.Contains(where)) {
			fMovingControl = PREGAP_POINTER;
			fClickOffset = where - fPregapPointerRect.LeftTop();
		} else if (fTopBarRect.Contains(where)) {
			fMovingControl = SELECTED_TRACK;
		} else {
			fMovingControl = NONE;
		}
	} else {
		if (fTopBarRect.Contains(where)) {
			fMovingControl = SELECTED_TRACK;
		} else {
			fMovingControl = NONE;
		}
	}
	// give this view focus if it wants it and doesn't have it
//	if (fMovingControl != NONE && !IsFocus()) {
//		MakeFocus(true);
//	}
	
	// set the focus marker properly
//	if (fFocusedControl != fMovingControl) {
//		fFocusedControl = fMovingControl;
//		inval = true;
//	}
	
	if (fMovingControl == SELECTED_TRACK) {
		// set the selected track to be the track at the click point.
		CDTrack *track = FindTrackAtPoint(where);
		if (track != fSelectedTrack) {
			SetSelectedTrack(track, true);
		}
	}
	
	if (inval) {
		if (fTrackEditView == NULL) {
			fTrackEditView = dynamic_cast<TrackEditView *>(fWindow->FindView("TrackEditView"));
		}
		if (fTrackEditView != NULL) {
			fTrackEditView->Populate(fSelectedTrack);
		}
		InvalidateBuffer();
	}
}

void EditWidget::MouseMoved(BPoint where, uint32 /*transit*/, const BMessage */*dragMessage*/)
{
	bool inval = false;
	AudioWrapperDataSource *src;
//	BPoint origPoint(where);
	
	switch (fMovingControl)
	{
	case NONE:
	
		break;
	case LEFT_BRACKET:
		if (where.x != fLastPoint.x) { // cursor moved since we saw it last
			// XXX: must constrain start point so that start + fade in cannot
			//      go past end - fade out
			if (where.x - fClickOffset.x < fBottomBarRect.left - 2) {
				where.x = fBottomBarRect.left - 2;
			}
			if (where.x > fBottomBarRect.right) {
				where.x = fBottomBarRect.right;
			}
			src = dynamic_cast<AudioWrapperDataSource *>(fSelectedTrack->DataSource());
			if (src != NULL) {
				int64 maxLength = src->Source()->Length();
				int64 perPixel =  maxLength / (fBottomBarRect.IntegerWidth() - 2);
				int64 delta = perPixel * (int32)floor(where.x - fLastPoint.x);
				int64 maxStart = src->GetEnd() - 1;
				int64 start = (int64)src->GetStart();
				if (start + delta < 0) {
					delta = -start;
				} else if (start + delta > maxStart) {
					delta = maxStart - start - 1;
				}
				if (delta != 0) {
					src->SetStart(start + delta);
					fSelectedTrack->SyncWithDataSource();
					fLastPoint = where;
					fValueChanged = true;
					inval = true;
				}
			} else {
				// something is seriously wrong if src == NULL
				fMovingControl = NONE;
			}	
		}
		break;
	case RIGHT_BRACKET:
		if (where.x != fLastPoint.x) { // cursor moved since we saw it last
			// XXX: must constrain end point so that end - fade out cannot
			//      go past start + fade in
			if (where.x < fBottomBarRect.left + 1) {
				where.x = fBottomBarRect.left + 1;
			}
			if (where.x > fBottomBarRect.right - 1) {
				where.x = fBottomBarRect.right - 1;
			}
			src = dynamic_cast<AudioWrapperDataSource *>(fSelectedTrack->DataSource());
			if (src != NULL) {
				int64 maxLength = src->Source()->Length();
				int64 perPixel =  maxLength / (fBottomBarRect.IntegerWidth() - 2);
				int64 delta = perPixel * (int32)floor(where.x - fLastPoint.x);
				int64 minEnd = src->GetStart() + 1;
				int64 end = (int64)src->GetEnd();
				if (end + delta > maxLength - 1) {
					delta = maxLength - end - 1;
				} else if (end + delta < minEnd) {
					delta = minEnd - end;
				}
				if (delta != 0) {
					src->SetEnd(end + delta);
					fSelectedTrack->SyncWithDataSource();
					fLastPoint = where;
					fValueChanged = true;
					inval = true;
				}
			} else {
				// something is seriously wrong if src == NULL
				fMovingControl = NONE;
			}	
		}
		break;
	case LEFT_FADER:
		if (where.x != fLastPoint.x) { // cursor moved since we saw it last
			BRect leftBracket(LeftBracketRect());
			BRect rightFader(RightFaderRect());
			
			if (where.x - fClickOffset.x < leftBracket.left + 2) {
				where.x = leftBracket.left + 2 + fClickOffset.x;
//				fClickOffset.x = 0;
			}
			if (where.x - fClickOffset.x > rightFader.left - 5) {
				where.x = rightFader.left - 5 + fClickOffset.x;
//				fClickOffset.x = 5;
			}
			src = dynamic_cast<AudioWrapperDataSource *>(fSelectedTrack->DataSource());
			if (src != NULL) {
				int64 maxLength = src->Source()->Length();
				int64 perPixel =  maxLength / (fBottomBarRect.IntegerWidth() - 2);
				int64 delta = perPixel * (int32)floor(where.x - fLastPoint.x);
				int64 start = (int64)src->GetStart();
				int64 currFadeIn = (int64)src->GetFadeIn();
				int64 maxFadeIn = src->GetEnd() - src->GetFadeOut() - start - 1;
				if (currFadeIn + delta < 0) {
					delta = -currFadeIn;
				} else if (currFadeIn + delta > maxFadeIn) {
					delta = maxFadeIn - currFadeIn;
				}
				if (delta != 0) {
					src->SetFadeIn(currFadeIn + delta);
					fLastPoint = where;
					fValueChanged = true;
					inval = true;
				}
			} else {
				// something is seriously wrong if src == NULL
				fMovingControl = NONE;
			}	
		}
		break;
	case RIGHT_FADER:
		if (where.x != fLastPoint.x) { // cursor moved since we saw it last
			BRect rightBracket(RightBracketRect());
			BRect leftFader(LeftFaderRect());
			
			if (where.x - fClickOffset.x > rightBracket.right - 5) {
				where.x = rightBracket.right - 5 + fClickOffset.x;
//				fClickOffset.x = 0;
			}
			if (where.x - fClickOffset.x < leftFader.right) {
				where.x = leftFader.right + 1 + fClickOffset.x;
//				fClickOffset.x = 5;
			}
			src = dynamic_cast<AudioWrapperDataSource *>(fSelectedTrack->DataSource());
			if (src != NULL) {
				int64 maxLength = src->Source()->Length();
				int64 perPixel =  maxLength / (fBottomBarRect.IntegerWidth() - 2);
				int64 delta = perPixel * (int32)floor(fLastPoint.x - where.x);
				int64 end = (int64)src->GetEnd();
				int64 currFadeOut = (int64)src->GetFadeOut();
				int64 maxFadeOut = end - src->GetStart() - src->GetFadeIn() - 1;
				if (currFadeOut + delta < 0) {
					delta = -currFadeOut;
				} else if (currFadeOut + delta > maxFadeOut) {
					delta = maxFadeOut - currFadeOut;
				}
				if (delta != 0) {
					src->SetFadeOut(currFadeOut + delta);
					fLastPoint = where;
					fValueChanged = true;
					inval = true;
				}
			} else {
				// something is seriously wrong if src == NULL
				fMovingControl = NONE;
			}
		}	
		break;
	case PREGAP_POINTER:
	
		break;
	case SELECTED_TRACK:
		{
			// set the selected track to be the track at the click point.
			CDTrack *track = FindTrackAtPoint(where);
			if (track != fSelectedTrack) {
				SetSelectedTrack(track, true);
//				inval = true;
			}
	
		}
		break;
	};
	
	if (inval) {
		// update list view
		if (fSelectedTrack != NULL) {
			if (fTrackListView == NULL) {
				fTrackListView = dynamic_cast<TrackListView *>(fWindow->FindView("TrackListView"));
			}
			if (fTrackListView != NULL) {
				fTrackListView->TrackUpdated(fSelectedTrack);
			}
		}
		// update edit view
		if (fTrackEditView == NULL) {
			fTrackEditView = dynamic_cast<TrackEditView *>(fWindow->FindView("TrackEditView"));
		}
		if (fTrackEditView != NULL) {
			fTrackEditView->Populate(fSelectedTrack);
		}
		InvalidateBuffer();
	}
}


void EditWidget::MouseUp(BPoint /*where*/)
{
	
//	switch (fMovingControl) {
//	case NONE:
//		// do nothing
//		break;
//	case LEFT_BRACKET:
//		if (fValueChanged) {
//			// XXX: do something
//		}
//		break;
//	case RIGHT_BRACKET:
//	
//		break;
//	case LEFT_FADER:
//	
//		break;
//	case RIGHT_FADER:
//	
//		break;
//	case PREGAP_POINTER:
//	
//		break;
//	case SELECTED_TRACK:
//	
//		break;
//	}

	// end tracking
	fMovingControl = NONE;
}

void EditWidget::KeyDown(const char *bytes, int32 numBytes)
{
	bool inval = false;

	switch(bytes[0]) {
	case B_TAB:
		// XXX: handle tabbing backwards when the SHIFT key is down!
		//      get the modifiers to check for shift, then set a flag
		if (fFocusedControl == NONE) {
			// I don't think that this will ever be true, because this
			// view shouldn't get key events if its not focused, and if
			// it is focused, fFocusedControl should be set, but just in case...
			fFocusedControl = LEFT_BRACKET;
			inval = true;
		} else if (fFocusedControl == LEFT_BRACKET) {
			fFocusedControl = LEFT_FADER;
			inval = true;
		} else if (fFocusedControl == LEFT_FADER) {
			fFocusedControl = RIGHT_FADER;
			inval = true;
		} else if (fFocusedControl == RIGHT_FADER) {
			fFocusedControl = RIGHT_BRACKET;
			inval = true;
		} else if (fFocusedControl == RIGHT_BRACKET) {
			fFocusedControl = PREGAP_POINTER;		
			inval = true;
		} else if (fFocusedControl == PREGAP_POINTER) {
			BView::KeyDown(bytes, numBytes);
		}
		break;
	case B_LEFT_ARROW: // fall through
	case B_DOWN_ARROW:
		printf("EditWidget::KeyDown() - left/right arrows not handled!\n");
		// XXX: check which control is focused, then manipulate it
		break;
	case B_RIGHT_ARROW: // fall through
	case B_UP_ARROW:
		printf("EditWidget::KeyDown() - up/down arrows not handled!\n");
		// XXX: check which control is focused, then manipulate it	
		break;
	case B_HOME:
		printf("EditWidget::KeyDown() - home key not handled!\n");
		// XXX: check which control is focused, then manipulate it
		break;	
	case B_END:
		printf("EditWidget::KeyDown() - end key not handled!\n");
		// XXX: check which control is focused, then manipulate it	
		break;
	default:
		BView::KeyDown(bytes, numBytes);
		break;
	}

	if (inval) {
		InvalidateBuffer();
	}
}

void EditWidget::MakeFocus(bool focused)
{
	if (focused == IsFocus()) {
		return;
	}
	
	if (focused) {
		fFocusedControl = LEFT_BRACKET;
	} else {
		fFocusedControl = NONE;
	}
	BView::MakeFocus(focused);
	InvalidateBuffer();
}

void EditWidget::Draw(BRect update)
{
	DrawBuffer();
}

void EditWidget::DrawBuffer()
{
		DrawTopBar();
		DrawBottomBar();
		DrawBrackets();
		DrawFaders();
		// DrawPregapPointer(BRect bottomBar)
}

// fOffscreen must be locked before calling this function
void EditWidget::DrawTopBar()
{
	PushState();
	SetHighColor(kLineColor);
	// draw border around bar
	BRect rect(fTopBarRect);
	StrokeRect(rect, B_SOLID_HIGH);
	
	// set font to correct size
	BFont font;
	GetFont(&font);
	font.SetSize(10);
	SetFont(&font, B_FONT_ALL);
	font_height fh;
	font.GetHeight(&fh);
	
	// draw a rectangle for each track into the bar
	int32 total = 0;
	CDTrack *track = fHeadTrack;
	rect.InsetBy(0, 1);
	rect.right = rect.left;	
	while (track != NULL) {
		int32 len = track->Length();
		rect.left = rect.right + 1;
		rect.right = rect.left + ceil(((float)len / (float)fMaxFrames) * (fTopBarRect.Width() - 2));
		if (rect.right > fTopBarRect.right - 1) { // don't draw past the end of the box
			rect.right = fTopBarRect.right - 1;
		}
		rect.right -= 1;
		if (track == fSelectedTrack) {
			SetHighColor(kSelectedColor);
			fSelectedTrackRect = rect;
		} else {
			SetHighColor(kBarColor);		
		}
		FillRect(rect, B_SOLID_HIGH);

		char buf[3];
		sprintf(buf, "%d", track->Index());
		float width = StringWidth(buf);
		if (width <= (rect.Width() - 4)) {
			SetLowColor(HighColor());
			SetHighColor(kTrackTextColor);
			DrawString(buf, BPoint(floor(rect.left + (rect.Width() - width) / 2.0) + 1,
				floor(rect.top + fh.ascent + fh.leading)));
		}
		
		rect.right += 1;
		SetHighColor(kLineColor);
		StrokeLine(rect.RightTop(), rect.RightBottom(), B_SOLID_HIGH);
		total += len;
		
		
		track = track->Next();
	}

	// fill rest of bar with empty color
	rect.left = rect.right + 1;
	rect.right = fTopBarRect.right - 1;
	SetHighColor(kEmptyColor);
	FillRect(rect, B_SOLID_HIGH);

	PopState();
}

void EditWidget::DrawBottomBar()
{
	PushState();
	rgb_color lineColor = kLineColor;
	rgb_color unusedColor = kUnusedColor;
	rgb_color selectedColor = kSelectedColor;
	if (!fEditEnabled) {
		lineColor = tint_color(lineColor, B_LIGHTEN_1_TINT);
		unusedColor = tint_color(unusedColor, B_LIGHTEN_1_TINT);
		selectedColor = tint_color(gray_color(selectedColor), B_LIGHTEN_1_TINT);
	}
	SetHighColor(lineColor);
	// draw border around bar
	BRect rect(fBottomBarRect);
	StrokeRect(rect, B_SOLID_HIGH);
	rect.InsetBy(1, 1);
	
	if (fSelectedTrack != NULL) {
		AudioWrapperDataSource *src = dynamic_cast<AudioWrapperDataSource *>(fSelectedTrack->DataSource());
		if (src != NULL) {
			// color unused space at start
			float realLength = (float)src->Source()->Length(); 
			float width = floor((src->GetStart() / realLength) * rect.Width());
			rect.right = rect.left + width;
			if (width > 0) {
				SetHighColor(unusedColor);
				FillRect(rect);
			}
			// color used space in middle
			width = floor(((src->Source()->Length() - src->GetEnd()) / realLength) * (fBottomBarRect.Width() - 2));
			rect.left = rect.right;
			rect.right = fBottomBarRect.right - width - 1;
			fUsedRect = rect;
			SetHighColor(selectedColor);
			FillRect(rect);
			
			// do alpha-blended drawing for fade in/fade out here
			SetDrawingMode(B_OP_ALPHA);
			BRect blendRect(rect);
			BRect faderRect(LeftFaderRect());
			blendRect.right = faderRect.left + 3;
			rgb_color col = kUnusedColor;
			if (blendRect.left < blendRect.right) {
				int32 fadeWidth = blendRect.IntegerWidth();
				BeginLineArray(fadeWidth);
				for (float x = 0; x < fadeWidth; x++) {
					col.alpha = 255 - (uchar)floor((x / (float)fadeWidth) * 255);
					AddLine(BPoint(blendRect.left + x, blendRect.top),
										BPoint(blendRect.left + x, blendRect.bottom), col);
				}
				EndLineArray();
			}
			
			blendRect = rect;
			faderRect = RightFaderRect();
			blendRect.left = faderRect.right - 3;
			if (blendRect.right > blendRect.left) {
				int32 fadeWidth = blendRect.IntegerWidth();
				BeginLineArray(fadeWidth);
				for (float x = 0; x < fadeWidth; x++) {
					col.alpha = (uchar)floor((x / (float)fadeWidth) * 255);
					AddLine(BPoint(blendRect.left + x, blendRect.top),
										BPoint(blendRect.left + x, blendRect.bottom), col);
				}
				EndLineArray();
			}
			

			SetDrawingMode(B_OP_COPY);			
			// color unused space at end
			rect.left = rect.right;
			rect.right = fBottomBarRect.right - 1;
			if (rect.IsValid() && rect.Width() > 0) {
				SetHighColor(unusedColor);
				FillRect(rect);			
			}
			
		}

	} else {
		SetHighColor(unusedColor);
		FillRect(rect);
		BFont font;
		GetFont(&font);
		font.SetFace(B_BOLD_FACE);
		font.SetSize(12);
		font_height fh;
		font.GetHeight(&fh);
		SetFont(&font);
		SetHighColor(LowColor());
		SetLowColor(unusedColor);
		const char *text = "No track selected";
		DrawString(text,
			BPoint(rect.left + (rect.Width() - font.StringWidth(text)) / 2,
					rect.top + (rect.Height() - (fh.ascent + fh.descent)) / 2 + fh.ascent));
	}

	PopState();
}

void EditWidget::DrawBrackets()
{
	// only draw brackets if there is a track selected
	if (fSelectedTrack != NULL) {
		PushState();
		BRect leftTopRect, rightTopRect, leftBottomRect, rightBottomRect;
		
		leftBottomRect = LeftBracketRect();
		rightBottomRect = RightBracketRect();

		leftTopRect = fSelectedTrackRect;
		leftTopRect.top -= 2;
		leftTopRect.bottom += 2;
		leftTopRect.right = leftTopRect.left + 1;
		leftTopRect.left -= 2;

		rightTopRect = fSelectedTrackRect;
		rightTopRect.top -= 2;
		rightTopRect.bottom += 2;
		rightTopRect.left = rightTopRect.right - 1;
		rightTopRect.right += 2;

		// set up initial colors
		rgb_color lineColor = kLineColor;
		rgb_color controlColor = kControlColor;
		
		// draw top left bracket
		SetHighColor(controlColor);
		BRect rect = leftTopRect;
		rect.InsetBy(1, 1);
		FillRect(rect, B_SOLID_HIGH);
		
		BeginLineArray(6);
		AddLine(leftTopRect.LeftTop(), leftTopRect.RightTop(), lineColor);
		AddLine(leftTopRect.RightTop(), leftTopRect.RightTop() + BPoint(0, 2), lineColor);
		AddLine(leftTopRect.RightTop() + BPoint(-1, 2), leftTopRect.RightBottom() - BPoint(1, 2), lineColor);
		AddLine(leftTopRect.RightBottom(), leftTopRect.RightBottom() - BPoint(0, 2), lineColor);
		AddLine(leftTopRect.LeftBottom(), leftTopRect.RightBottom(), lineColor);
		AddLine(leftTopRect.LeftTop(), leftTopRect.LeftBottom(), lineColor);
		EndLineArray();

		// draw top right bracket
		rect = rightTopRect;
		rect.InsetBy(1, 1);
		FillRect(rect, B_SOLID_HIGH);

		BeginLineArray(6);
		AddLine(rightTopRect.LeftTop(), rightTopRect.RightTop(), lineColor);
		AddLine(rightTopRect.LeftTop(), rightTopRect.LeftTop() + BPoint(0, 2), lineColor);
		AddLine(rightTopRect.LeftTop() + BPoint(1, 2), rightTopRect.LeftBottom() - BPoint(-1, 2), lineColor);
		AddLine(rightTopRect.LeftBottom(), rightTopRect.LeftBottom() - BPoint(0, 2), lineColor);
		AddLine(rightTopRect.LeftBottom(), rightTopRect.RightBottom(), lineColor);
		AddLine(rightTopRect.RightTop(), rightTopRect.RightBottom(), lineColor);
		EndLineArray();
		
		// change to disabled colors if editing is not enabled
		if (!fEditEnabled) {
			lineColor = tint_color(lineColor, B_LIGHTEN_1_TINT);
			controlColor = gray_color(controlColor);
		}
		
		// draw left bottom bracket
		SetHighColor((fFocusedControl == LEFT_BRACKET) ?
								 ui_color(B_KEYBOARD_NAVIGATION_COLOR) : controlColor);
		rect = leftBottomRect;
		rect.InsetBy(1, 1);
		rect.right = rect.left + 1;
		FillRect(rect, B_SOLID_HIGH);
		rect = leftBottomRect;
		rect.InsetBy(1, 1);
		rect.bottom = rect.top + 1;
		FillRect(rect, B_SOLID_HIGH);
		rect = leftBottomRect;
		rect.InsetBy(1, 1);
		rect.top = rect.bottom - 1;
		FillRect(rect, B_SOLID_HIGH);
				
		BeginLineArray(8);
		AddLine(leftBottomRect.LeftTop() + BPoint(1, 0), leftBottomRect.RightTop(), lineColor);
		AddLine(leftBottomRect.RightTop(), leftBottomRect.RightTop() + BPoint(0, 3), lineColor);
		AddLine(leftBottomRect.RightTop() + BPoint(-3, 3), leftBottomRect.RightTop() + BPoint(0, 3), lineColor);
		AddLine(leftBottomRect.RightTop() + BPoint(-3, 3), leftBottomRect.RightBottom() + BPoint(-3, -3), lineColor);
		AddLine(leftBottomRect.RightBottom() - BPoint(0, 3), leftBottomRect.RightBottom() + BPoint(-3, -3), lineColor);
		AddLine(leftBottomRect.RightBottom() - BPoint(0, 3), leftBottomRect.RightBottom(), lineColor);
		AddLine(leftBottomRect.LeftBottom() + BPoint(1, 0), leftBottomRect.RightBottom(), lineColor);
		AddLine(leftBottomRect.LeftTop() + BPoint(0, 1), leftBottomRect.LeftBottom() - BPoint(0, 1), lineColor);		
		EndLineArray();

		// draw right bottom bracket
		SetHighColor((fFocusedControl == RIGHT_BRACKET) ?
								 ui_color(B_KEYBOARD_NAVIGATION_COLOR) : controlColor);
		rect = rightBottomRect;
		rect.InsetBy(1, 1);
		rect.left = rect.right - 1;
		FillRect(rect, B_SOLID_HIGH);
		rect = rightBottomRect;
		rect.InsetBy(1, 1);
		rect.bottom = rect.top + 1;
		FillRect(rect, B_SOLID_HIGH);
		rect = rightBottomRect;
		rect.InsetBy(1, 1);
		rect.top = rect.bottom - 1;
		FillRect(rect, B_SOLID_HIGH);
				
		BeginLineArray(8);
		AddLine(rightBottomRect.RightTop() - BPoint(1, 0), rightBottomRect.LeftTop(), lineColor);
		AddLine(rightBottomRect.LeftTop(), rightBottomRect.LeftTop() + BPoint(0, 3), lineColor);
		AddLine(rightBottomRect.LeftTop() + BPoint(3, 3), rightBottomRect.LeftTop() + BPoint(0, 3), lineColor);
		AddLine(rightBottomRect.LeftTop() + BPoint(3, 3), rightBottomRect.LeftBottom() + BPoint(3, -3), lineColor);
		AddLine(rightBottomRect.LeftBottom() - BPoint(0, 3), rightBottomRect.LeftBottom() + BPoint(3, -3), lineColor);
		AddLine(rightBottomRect.LeftBottom() - BPoint(0, 3), rightBottomRect.LeftBottom(), lineColor);
		AddLine(rightBottomRect.RightBottom() - BPoint(1, 0), rightBottomRect.LeftBottom(), lineColor);
		AddLine(rightBottomRect.RightTop() + BPoint(0, 1), rightBottomRect.RightBottom() - BPoint(0, 1), lineColor);		
		EndLineArray();
		
		SetHighColor(lineColor);
		float x1, x2, x3, x4, y, xRadius, yAdjust;
		x1 = leftTopRect.left + floor(leftTopRect.Width() / 2);
		x2 = leftBottomRect.left + floor(leftBottomRect.Width() / 2);
		x3 = rightTopRect.left + ceil(rightTopRect.Width() / 2);
		x4 = rightBottomRect.left + ceil(rightBottomRect.Width() / 2);

		// yAdjust corrects for the cases where the lines would collide with each other.
		// Bump the left line up if the right line travels to the left of the top left bracket.
		if (x4 <= x1) {
			yAdjust = 3;
		} else {
			yAdjust = 0;
		}
	
		BRect ellipseRect;
		// draw connector line from left top to left bottom bracket
		StrokeLine(BPoint(x1, leftTopRect.bottom + 1),
			BPoint(x1, leftTopRect.bottom + kUpperLineLength - yAdjust), B_SOLID_HIGH);
		StrokeLine(BPoint(x2, leftBottomRect.top),
			BPoint(x2, leftBottomRect.top - kLowerLineLength - yAdjust), B_SOLID_HIGH);
		xRadius = floor((x1 - x2) / 2);
		xRadius = (abs((int)xRadius) < kArcRadius) ? abs((int)xRadius) : kArcRadius;
		y = leftBottomRect.top - kLowerLineLength - kArcRadius - yAdjust;
		if (x1 - x2 >= 0) {
			ellipseRect.Set(x1 - (xRadius * 2), y - (kArcRadius * 2), x1, y);
			StrokeArc(ellipseRect, 270, 90, B_SOLID_HIGH);
			ellipseRect.Set(x2, y, x2 + (xRadius * 2), y + (kArcRadius * 2));
			StrokeArc(ellipseRect, 90, 90, B_SOLID_HIGH);
			StrokeLine(BPoint(x2 + xRadius, y), BPoint(x1 - xRadius, y), B_SOLID_HIGH);
		} else {
			ellipseRect.Set(x1, y - (kArcRadius * 2), x1 + (xRadius * 2), y);
			StrokeArc(ellipseRect, 180, 90, B_SOLID_HIGH);
			ellipseRect.Set(x2 - (xRadius * 2), y, x2, y + (kArcRadius * 2));
			StrokeArc(ellipseRect, 0, 90, B_SOLID_HIGH);
			StrokeLine(BPoint(x2 - xRadius, y), BPoint(x1 + xRadius/* - 2*/, y), B_SOLID_HIGH);
		}
		
		// Bump the right line up if the left line travels to the right of the top right bracket.
		if (x2 >= x3) {
			yAdjust = 3;
		} else {
			yAdjust = 0;
		}
		// draw connector line from right top to right bottom bracket
		StrokeLine(BPoint(x3, rightTopRect.bottom + 1),
			BPoint(x3, rightTopRect.bottom + kUpperLineLength - yAdjust), B_SOLID_HIGH);
		StrokeLine(BPoint(x4, rightBottomRect.top),
			BPoint(x4, rightBottomRect.top - kLowerLineLength - yAdjust), B_SOLID_HIGH);
		xRadius = floor((x3 - x4) / 2);
		xRadius = (abs((int)xRadius) < kArcRadius) ? abs((int)xRadius) : kArcRadius;
		y = rightBottomRect.top - kLowerLineLength - kArcRadius - yAdjust;			
		if (x3 - x4 >= 0) {
			ellipseRect.Set(x3 - (xRadius * 2), y - (kArcRadius * 2), x3, y);
			StrokeArc(ellipseRect, 270, 90, B_SOLID_HIGH);
			ellipseRect.Set(x4, y, x4 + (xRadius * 2), y + (kArcRadius * 2));
			StrokeArc(ellipseRect, 90, 90, B_SOLID_HIGH);

			StrokeLine(BPoint(x4 + xRadius, y), BPoint(x3 - xRadius, y), B_SOLID_HIGH);
		} else {
			ellipseRect.Set(x3, y - (kArcRadius * 2), x3 + (xRadius * 2), y);
			StrokeArc(ellipseRect, 180, 90, B_SOLID_HIGH);
			ellipseRect.Set(x4 - (xRadius * 2), y, x4, y + (kArcRadius * 2));
			StrokeArc(ellipseRect, 0, 90, B_SOLID_HIGH);
			
			StrokeLine(BPoint(x4 - xRadius, y), BPoint(x3 + xRadius, y), B_SOLID_HIGH);
		}
		
		PopState();
	}	
}

void EditWidget::DrawFaders()
{
	// don't draw faders if no track is selected
	if (fSelectedTrack == NULL) {
		return;
	}
	
	BRect leftFaderRect = LeftFaderRect();
	BRect rightFaderRect = RightFaderRect();
	
	rgb_color lineColor = kLineColor;
	rgb_color controlColor = kControlColor;
		
	if (!fEditEnabled) {
		lineColor = tint_color(lineColor, B_LIGHTEN_1_TINT);
		controlColor = gray_color(controlColor);
	}
	
	// draw left fader
	SetHighColor(lineColor);
	StrokeLine(leftFaderRect.RightTop(), leftFaderRect.RightBottom());
	StrokeLine(leftFaderRect.RightTop() - BPoint(1, 0),
						   leftFaderRect.LeftTop() + BPoint(0, 3));
	StrokeLine(leftFaderRect.RightBottom() - BPoint(1, 0),
						   leftFaderRect.LeftTop() + BPoint(0, 3));
	SetHighColor((fFocusedControl == LEFT_FADER) ?
							 ui_color(B_KEYBOARD_NAVIGATION_COLOR) : controlColor);
	FillTriangle(leftFaderRect.RightTop() + BPoint(-1, 1),
							 leftFaderRect.RightBottom() - BPoint(1, 1),
							 leftFaderRect.LeftTop() + BPoint(1, 3));

	// draw right fader
	SetHighColor(lineColor);
	StrokeLine(rightFaderRect.LeftTop(), rightFaderRect.LeftBottom());
	StrokeLine(rightFaderRect.LeftTop() + BPoint(1, 0),
						   rightFaderRect.RightTop() + BPoint(0, 3));
	StrokeLine(rightFaderRect.LeftBottom() + BPoint(1, 0),
						   rightFaderRect.RightTop() + BPoint(0, 3));
	SetHighColor((fFocusedControl == RIGHT_FADER) ?
							 ui_color(B_KEYBOARD_NAVIGATION_COLOR) : controlColor);
	FillTriangle(rightFaderRect.LeftTop() + BPoint(1, 1),
							 rightFaderRect.LeftBottom() - BPoint(-1, 1),
							 rightFaderRect.RightTop() + BPoint(-1, 3));

	// draw lines connecting faders to brackets
	float y = leftFaderRect.top + 3;
	BRect bracketRect(LeftBracketRect());
	SetHighColor(lineColor);
	if (leftFaderRect.left > bracketRect.right - 3) {
		StrokeLine(BPoint(leftFaderRect.left - 1, y), BPoint(bracketRect.right - 2, y));
	}
	bracketRect = RightBracketRect();
	if (rightFaderRect.right < bracketRect.left + 3) {
		StrokeLine(BPoint(rightFaderRect.right + 1, y), BPoint(bracketRect.left + 2, y));
	}
}


void EditWidget::InvalidateBuffer()
{
	Invalidate();
}

void EditWidget::SetBackgroundColor()
{
	SetColorsFromParent();
}

void EditWidget::CalculateRects(BRect bounds)
{
	fTopBarRect = bounds;
	fTopBarRect.InsetBy(4, 0);
	fTopBarRect.top += 1;
	fTopBarRect.bottom = fTopBarRect.top + kTopBarHeight;
	
	fBottomBarRect = bounds;
	fBottomBarRect.InsetBy(4, 0);
	fBottomBarRect.bottom -= kPointerDescent;
	fBottomBarRect.top = fTopBarRect.bottom + kUpperLineLength + (kArcRadius * 2) + kLowerLineLength + 3;
	
//	BRect			fPregapPointerRect;
}

BRect EditWidget::LeftBracketRect()
{
	BRect rect(fUsedRect);

	rect.left -= 3;
	rect.right = rect.left + 6;
	rect.top -= 3;
	rect.bottom += 3;

	return rect;
}

BRect EditWidget::RightBracketRect()
{
	BRect rect(fUsedRect);

	rect.right += 3;
	rect.left = rect.right - 6;
	rect.top -= 3;
	rect.bottom += 3;

	return rect;
}

BRect EditWidget::LeftFaderRect()
{
	BRect rect(LeftBracketRect());

	AudioWrapperDataSource *src = dynamic_cast<AudioWrapperDataSource *>(fSelectedTrack->DataSource());
	int64 maxLength = src->Source()->Length();
	int64 perPixel =  maxLength / (fBottomBarRect.IntegerWidth() - 2);
	float fadeLength = (((float)src->GetFadeIn()) / (float)perPixel);

	rect.OffsetBy(fadeLength, 0);
	rect.top = floor(rect.top + rect.Height() / 2.0) - 3;
	rect.bottom = rect.top + 6;
	rect.right -= 1;
	rect.left = rect.right - 4;
	
	return rect;
}

BRect EditWidget::RightFaderRect()
{
	BRect rect = RightBracketRect();

	AudioWrapperDataSource *src = dynamic_cast<AudioWrapperDataSource *>(fSelectedTrack->DataSource());
	int64 maxLength = src->Source()->Length();
	int64 perPixel =  maxLength / (fBottomBarRect.IntegerWidth() - 2);
	float fadeLength = (((float)src->GetFadeOut()) / (float)perPixel);

	rect.OffsetBy(-fadeLength, 0);
	rect.top = floor(rect.top + rect.Height() / 2.0) - 3;
	rect.bottom = rect.top + 6;
	rect.left += 1;
	rect.right = rect.left + 4;
	
	return rect;
}


void EditWidget::SetMaxFrames(int32 maxFrames)
{
	fMaxFrames = maxFrames;
}

int32 EditWidget::MaxFrames()
{
	return fMaxFrames;
}

CDTrack *EditWidget::FindTrackAtPoint(BPoint point) const
{
	CDTrack *found = NULL;
	CDTrack *track = fHeadTrack;
	BRect rect(fTopBarRect);
	rect.right = rect.left;	
	while (track != NULL) {
		int32 len = track->Length();
		rect.left = rect.right + 1;
		rect.right = rect.left + ceil(((float)len / (float)fMaxFrames) * (fTopBarRect.Width() - 2));
		if (rect.right > fTopBarRect.right - 1) { // don't go past the end of the box
			rect.right = fTopBarRect.right - 1;
		}
		if (rect.Contains(point)) {
			found = track;
			break;
		}
		track = track->Next();
	}

	return found;
}

