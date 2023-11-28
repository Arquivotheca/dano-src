#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Font.h>
#include <Window.h>

#include "color_defs.h"
#include "dt_utils.h"
#include "time_utils.h"
#include "Calendar.h"

const char *abbrevdaylist[] = {
	"S","M","T","W","T","F","S"
};

const char *shortdaylist[] = {
	"Sun","Mon","Tues","Wed","Thurs","Fri","Sat"
};

const char *fulldaylist[] = {
	"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};

rgb_color kDayColor;

TDay::TDay(BView* target, BView* display)
	: BArchivable(),
		fIsActive(false),
		fSelected(false),
		fFocus(false),
		fDate(0),
		fDay(0),
		fFrame(0,0,1,1),
		fBorder(B_PLAIN_BORDER)
{
	kDayColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT);
	fTarget = target;
	fDisplay = display;
}

TDay::TDay(BRect frame, bool isactive, short date, short dayofweek,
	bool selected, alignment placement)
	: BArchivable(),
		fIsActive(isactive),
		fSelected(selected),
		fFocus(false),
		fDate(date),
		fDay(dayofweek),
		fFrame(frame),
		fPlacement(placement),
		fBorder(B_PLAIN_BORDER)
{
}

TDay::~TDay()
{
}

void
TDay::Draw()
{
	BRect frame(fFrame);

	if (!fIsActive) {
		fTarget->SetHighColor(kDayColor);
		fTarget->SetLowColor(fTarget->ViewColor());
		fTarget->FillRect(frame);
	} else {
		if (fBorder == B_PLAIN_BORDER) {
			fTarget->SetHighColor(kDayColor);
			fTarget->SetLowColor(fTarget->ViewColor());
			fTarget->FillRect(frame);
		} else if (fBorder == B_FANCY_BORDER) {
			fTarget->BeginLineArray(4);
			
			fTarget->AddLine( BPoint(frame.left, frame.top),
				BPoint(frame.right-1, frame.top), kMediumGray);
			fTarget->AddLine( BPoint(frame.left, frame.top),
				BPoint(frame.left, frame.bottom), kMediumGray);
			
			fTarget->AddLine( BPoint(frame.right-1, frame.top+1),
				BPoint(frame.right-1, frame.bottom), kWhite);
			fTarget->AddLine( BPoint(frame.left+1, frame.bottom),
				BPoint(frame.right-1, frame.bottom), kWhite);
		
			fTarget->EndLineArray();
		}

		float f_height = FontHeight(fTarget, false);
		
		char tempStr[3];
		sprintf(tempStr,"%u",fDate);
		
		float x = 0;
		float y = 0;
		if (fPlacement == B_ALIGN_CENTER) {
			x = fFrame.left + (fFrame.Width()+1)/2
				- (fTarget->StringWidth(tempStr)/2) + 1;
			y = fFrame.top + f_height;
		} else if (fPlacement == B_ALIGN_LEFT) {
			x = fFrame.right + 2;
			y = fFrame.top + f_height;
		}

		fTarget->MovePenTo(x,y);
		
		if (Selected()) {
//			fTarget->SetHighColor(200, 10, 10, 0);
//			fTarget->SetLowColor(kDayColor);

			//	draw the interior
			frame.InsetBy(1, 1);
			fTarget->SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT));
			fTarget->FillRect(frame);
			
			//	set the text color
			fTarget->SetHighColor(kWhite);
			fTarget->SetLowColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT));
		} else {
			fTarget->SetHighColor(kBlack);
			fTarget->SetLowColor(kDayColor);
		}
		fTarget->DrawString(tempStr);	

		if (fDisplay->Window()->IsActive()
			&& fDisplay->IsFocus() && IsFocus()) {
			fTarget->SetHighColor(keyboard_navigation_color());
			fTarget->SetLowColor(fTarget->ViewColor());			
			fTarget->StrokeRect(fFrame);
		} else {
			//	draw the border	
			if (Selected()) {
				fTarget->SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
				fTarget->StrokeLine(fFrame.LeftBottom(), fFrame.RightBottom());
				fTarget->StrokeLine(fFrame.RightBottom(), fFrame.RightTop());
				fTarget->SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_4_TINT));
				fTarget->StrokeLine(fFrame.LeftBottom(), fFrame.LeftTop());
				fTarget->StrokeLine(fFrame.LeftTop(), fFrame.RightTop());
			} else {
				fTarget->SetHighColor(kDayColor);
				fTarget->SetLowColor(fTarget->ViewColor());			
				fTarget->StrokeRect(fFrame);
			}
		}		
	}	
}

void
TDay::UpdateDay(bool isactive, short date, short dayofweek, bool selected)
{
	fIsActive = isactive;
	fDate = date;
	fDay = dayofweek;
	fSelected = selected;
//	fFocus = false;
}

void
TDay::Relocate(int32 l, int32 t, int32 r, int32 b)
{
	fFrame.Set(l, t, r, b);
}

bool
TDay::Selected() const
{
	return fSelected;
}

void
TDay::SetSelected(bool s)
{
	fSelected = s;
}

bool
TDay::IsFocus() const
{
	return fFocus;
}

void
TDay::SetFocus(bool f)
{
	fFocus = f;
}

alignment
TDay::Alignment() const
{
	return fPlacement;
}

void
TDay::SetAlignment(alignment a)
{
	fPlacement = a;
}

BRect
TDay::DayFrame() const
{
	return fFrame;
}

bool
TDay::IsActive() const
{
	return fIsActive;
}

short
TDay::Date() const
{
	return fDate;
}

short
TDay::Day() const
{
	return fDay;
}

// *********************************************************************** //

TCalendar::TCalendar(BPoint loc, int32 cell_width, int32 cell_height,
	int32 gap_size, bool show_header,
	time_t start_of_month, short days_in_month, short start_day_of_week,
	alignment placement, BMessage* msg)
	: BControl(BRect(loc.x, loc.y, loc.x+1, loc.y+1), "calendar", "label", msg,
		B_FOLLOW_NONE,
		B_FRAME_EVENTS | B_WILL_DRAW | B_NAVIGABLE),
		fShowHeader(show_header),
		fCellWidth(cell_width),
		fCellHeight(cell_height),
		fGapSize(gap_size),
		fPlacement(placement),
		fMonthInSeconds(start_of_month),
		fDayCount(days_in_month),
		fStartDay(start_day_of_week),
		fFocusCell(-1)
{
	for (int32 i=0 ; i<42 ; i++)
		fdaylist[i] = NULL;
		
	fInitd=false;
	fOSView = NULL;
	fOSBits = NULL;
	
	ResizeToPreferred();
	
	fKeyBuffer[0] = fKeyBuffer[1] = fKeyBuffer[2] = 0;
	fLastTime = 0;
}

TCalendar::~TCalendar()
{
	return;
	TDay* day=NULL;
	for (int32 i=0 ; i<42 ; i++) {
		day = fdaylist[i];
		delete day;
	}
}

void
TCalendar::AttachedToWindow()
{
	BControl::AttachedToWindow();
		
	if (!fInitd) {
		SetViewColor(kViewGray);
		if (fOSBits->Lock()) {
			fOSView->SetViewColor(kViewGray);
			fOSBits->Unlock();
		}
				
		char str[3];
		time_t t = time(NULL);
		
		short today = _Date(t, str);
	
		SetDays( fDayCount, fStartDay, today);
		fInitd = true;
	}
}

void 
TCalendar::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case msg_set_month:
			break;
		case msg_set_day:
			break;
			
		default:
			BControl::MessageReceived(msg);
	}
}

bool
TCalendar::FindCell(BPoint where, int32* index)
{
	bool found=false;
	int32 i;
	for (i=0 ; i<kMaxDayCount ; i++) {
		if (fdaylist[i]->DayFrame().Contains(where)) {
			found = fdaylist[i]->IsActive();
			break;
		}
	}
	
	if (found)
		*index = i;
	else
		*index = -1;
		
	return found;
}

void 
TCalendar::MouseDown(BPoint where)
{
	int32 index;
	bool found = FindCell(where, &index);
	if (found)
		SetValue(index);
			
	BPoint	newLoc,currPosition=where;
	ulong buttons;
	do {
		BPoint where;
		GetMouse(&where, &buttons);
		if (where.x != currPosition.x) {
			newLoc = where;

			if (FindCell(where, &index))
				SetValue(index);
		}
		snooze(15000);
		currPosition = where;
	} while (buttons);

	if (found) {
//		if (!IsFocus()) {			// 	should it really get the focus when you click in it?
//			MakeFocus(true);		//	the deselect code for the config view will also need	
//			ChangeFocus(Value());	//		to be updated
//		}
		Invoke(Message());
	} else
		BControl::MouseDown(where);
}

void 
TCalendar::KeyDown(const char *bytes, int32 numBytes)
{
	//	do a simple type ahead
	if (isdigit(bytes[0])) {
		//	make the keyrate multiplier 2 * max keys in buffer
		if (fLastTime > system_time()) {
			fKeyBuffer[0] = fKeyBuffer[1];			
			fKeyBuffer[1] = bytes[0];
		} else {
			//add the first key
			fKeyBuffer[0] = '0';
			fKeyBuffer[1] = bytes[0];
		}
		fKeyBuffer[2] = 0;
		SetDay(atoi(fKeyBuffer));
		fLastTime = system_time() + 500000;
		return;
	}
	
	switch (bytes[0]) {
		case B_LEFT_ARROW:
			SetFocusCell(FocusCell()-1);
			break;
		case B_RIGHT_ARROW:
			SetFocusCell(FocusCell()+1);
			break;
			
		case B_UP_ARROW:
			SetFocusCell(FocusCell()-7);
			break;
		case B_DOWN_ARROW:
			SetFocusCell(FocusCell()+7);
			break;
				
		case B_SPACE:
			SetValue(FocusCell());
			Invoke(Message());
			break;
			
		default:
			BControl::KeyDown(bytes, numBytes);
			break;
	}
}

void
TCalendar::MakeFocus(bool state)
{
	BControl::MakeFocus(state);
	
	//	if there isn't a cell focused
	//	set the focus cell to the selected cell
	if (FocusCell() < 0)
		SetFocusCell(Value());

	Draw(Bounds());
}

void TCalendar::Draw(BRect)
{
	PushState();
	if (fOSBits->Lock()) {
		fOSView->SetLowColor(fOSView->ViewColor());
		fOSView->SetHighColor(fOSView->ViewColor());
		fOSView->FillRect(Bounds());
					
		DrawHeader();
		for (int32 i=0 ; i<42 ; i++)
			fdaylist[i]->Draw();

		fOSView->Sync();
		fOSBits->Unlock();
		DrawBitmap(fOSBits);
	}
	PopState();
}

void
TCalendar::DrawHeader()
{	
	fOSView->SetHighColor(kBlack);
	fOSView->SetLowColor(fOSView->ViewColor());
	
	float left=0;
	const char*	day;
	for (short indx=0 ; indx<7 ; indx++) {
		if (StringWidth("Wednesday")+5 <= fCellWidth)
			day = fulldaylist[indx];
		else if (StringWidth("Thurs")+5 <= fCellWidth)
			day = shortdaylist[indx];			
		else
			day = abbrevdaylist[indx];
			 
		left = (fCellWidth + fGapSize) * indx;
		left += ((fCellWidth+1)/2) - (StringWidth(day)/2) + 1;

		fOSView->DrawString(day, BPoint(left, FontHeight(this,true)));		
	}
}

void TCalendar::FrameResized(float w,float h)
{
	BControl::FrameResized(w, h);
}

// 	traverse the list of day objects (42)
//	find the startday and set its index and day of week
void
TCalendar::SetDays(short days, short startday, short selected_day)
{
	bool	isactive=false;
	short	indx,date,dayofweek,dateindx=1;
	
	if (fOSBits->Lock()) {
		for (indx=0 ; indx<kMaxDayCount ; indx++) {
			if ((indx >= startday) && (indx < (startday+days))) {
				isactive = true;
				date = dateindx++;
				dayofweek = (indx>6) ? (indx % 7) : indx;
			} else {
				isactive = false;
				date = 0;
				dayofweek = 0;
			}
			
			fdaylist[indx]->UpdateDay(isactive, date, dayofweek, (date==selected_day));
			if (date == selected_day) {
				SetValue(indx);
			}
			
			fdaylist[indx]->Draw();
		}
		fOSView->Sync();
		fOSBits->Unlock();
		DrawBitmap(fOSBits);
	}
}

void
TCalendar::UpdateCalendar( time_t secs, short days, short startday, short selected_day)
{
	fMonthInSeconds = secs;
	fDayCount = days;
	fStartDay = startday;
	
	SetDays(days, startday, selected_day);
}

short
TCalendar::FocusCell() const
{
	return fFocusCell;
}

void
TCalendar::SetFocusCell(short index)
{
	if (index < 0 || index >= kMaxDayCount)
		return;
	
	if (!fdaylist[index]->IsActive())
		return;

	if (index == fFocusCell)
		return;
		
	if (fOSBits->Lock()) {
		//	if there is already a cell focused, de-focus it
		if (FocusCell() > -1) {
			fdaylist[FocusCell()]->SetFocus(false);
			fdaylist[FocusCell()]->Draw();
		}
		//	set the focus on the new cell
		fdaylist[index]->SetFocus(true);
		fdaylist[index]->Draw();
		fFocusCell = index;
		
		fOSView->Sync();
		fOSBits->Unlock();
		DrawBitmap(fOSBits);
	}
}

void
TCalendar::SetValue(int32 index)
{
	if (index < 0 || index > kMaxDayCount || index == Value())
		return;
		
	if (fdaylist[index]->IsActive()) {
		if (fOSBits->Lock()) {
			if (Value() >= 0) { 
				fdaylist[Value()]->SetSelected(false);
				fdaylist[Value()]->Draw();
			}
			
			fdaylist[index]->SetSelected(true);
			fdaylist[index]->Draw();
			fOSBits->Unlock();
		}
		
		BControl::SetValue(index);
	}
}

status_t
TCalendar::Invoke(BMessage *msg)
{
	if (!msg)
		msg = Message();
	if (!msg)
		return B_BAD_VALUE;

	BMessage clone(*msg);

	clone.AddInt64("when", system_time());
	clone.AddPointer("source", this);
	clone.AddInt32("be:value",Value());
	clone.AddInt32("date", fdaylist[Value()]->Date());
	status_t err = BInvoker::Invoke(&clone);
	return err;
}

void
TCalendar::SetFont(const BFont *font, uint32 mask)
{
	BControl::SetFont(font, mask);
	ResizeToPreferred();
}

void
TCalendar::GetPreferredSize(float *width, float *height)
{
	*width = (7 * fCellWidth) + (6 * fGapSize);
	*height = (6 * fCellHeight) + (5 * fGapSize);
	if (fShowHeader)
		*height += FontHeight(this, true) + 2;			// 2 pixel gap to top of day
}

void
TCalendar::ResizeToPreferred()
{
	float week_width, calendar_height;
	
	GetPreferredSize(&week_width, &calendar_height);
	ResizeTo(week_width, calendar_height);
	
	//
	if (fOSView) {
		fOSBits->RemoveChild(fOSView);
		fOSView->ResizeTo(week_width, calendar_height);
	} else	
		fOSView = new BView(Bounds(), "offscreen", B_FOLLOW_NONE, B_WILL_DRAW);

	fOSView->SetViewColor(kViewGray);
	BFont f;
	GetFont(&f);
	fOSView->SetFont(&f);
	
	if (fOSBits)
		delete fOSBits;
	fOSBits = new BBitmap(Bounds(), B_RGB32, true);
	fOSBits->AddChild(fOSView);

	//
	float x = 0.0, y = 0.0;
	if (fShowHeader)
		y = FontHeight(this, true) + 2;
	
	TDay* day=NULL;
	for (int32 row=0 ; row<6 ; row++) {
		for (int32 col=0 ; col<7 ; col++) {
			x = (col * fCellWidth) + (col * fGapSize);			
			
			if (fdaylist[col + (7*row)] == NULL)
				day = new TDay(fOSView, this);
			else
				day = fdaylist[col + (7*row)];
				
			day->Relocate(x, y, x + (fCellWidth-1), y + (fCellHeight - 1));
			day->SetAlignment(fPlacement);
			fdaylist[col + (7*row)] = day;
		}
		y += fCellHeight + fGapSize;
	}
}

void
TCalendar::SetDay(short day)
{
	if (Day() == day)
		return;
		
	if (day < 1)
		SetValue(fStartDay);
	else if (day > fDayCount)
		SetValue(fDayCount-1+fStartDay);
	else
		SetValue(day-1+fStartDay);
}

short
TCalendar::Day() const
{
	return Value() - fStartDay + 1;
}
