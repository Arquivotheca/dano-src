#include <Control.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <View.h>

#ifndef Calendar_H
#define Calendar_H

const int32 msg_set_month = 'mont';
const int32 msg_set_day = 'day ';

class TCalendar;
class TDay : public BArchivable {
public:
						TDay(BView* target, BView* display);
						TDay(BRect rect, bool isactive=false, short date=0,
							short dayofweek=0, bool selected=false,
							alignment=B_ALIGN_LEFT);
						~TDay();

		void			Draw();
					
		void			UpdateDay(bool isactive,short date,short dayofweek, bool s);
		void			Relocate( int32 left, int32 top, int32 right, int32 bottom);

		bool			Selected() const;
		void			SetSelected(bool s);
		
		bool			IsFocus() const;
		void			SetFocus(bool f);
		
		alignment		Alignment() const;
		void			SetAlignment(alignment a);
					
		BRect			DayFrame() const;
		bool			IsActive() const;
		short			Date() const;
		short			Day() const;
		
private:
		BView*			fTarget;
		BView*			fDisplay;
		bool			fIsActive;
		bool			fSelected;
		bool			fFocus;
		short 			fDate;
		short			fDay;
		BRect			fFrame;
		alignment		fPlacement;
		border_style 	fBorder;
};

const int32 kMaxDayCount = 42;

class TCalendar : public BControl {
public:
						TCalendar(BPoint loc, int32 cell_width, int32 cell_height,
							int32 gap_size, bool show_header, time_t start_of_month,
							short days_in_month, short start_day_of_week,
							alignment placement, BMessage* msg);
						~TCalendar();
		
		void 			AttachedToWindow();
		void			MessageReceived(BMessage *msg);
		void			MouseDown(BPoint where);
		void			KeyDown(const char *bytes, int32 numBytes);
		void			MakeFocus(bool focusState = true);

		bool			FindCell(BPoint, int32*);
		
		void			DrawHeader();
		void 			Draw(BRect updateRect);
		void 			FrameResized(float w,float h);
		
		void 			SetDays(short days, short startday, short selected_day);			
		void			UpdateCalendar(time_t secs, short days, short startday,
							short selected_day);

		short			FocusCell() const;
		void			SetFocusCell(short index);
		
		void			SetValue(int32 day);
		status_t		Invoke(BMessage*);	

		void            SetFont(const BFont *font, uint32 mask = B_FONT_ALL);

		void			GetPreferredSize(float *width, float *height);
		void			ResizeToPreferred();
		
		void			SetDay(short);
		short			Day() const;

private:
		bool			fShowHeader;
		int32			fCellWidth;
		int32			fCellHeight;
		int32			fGapSize;
		alignment		fPlacement;
		
		time_t			fMonthInSeconds;
		short			fDayCount;
		short 			fStartDay;
		
		short			fFocusCell;
				
		TDay			*fdaylist[kMaxDayCount];
		bool			fInitd;
		
		BView*			fOSView;
		BBitmap*		fOSBits;
		
		bigtime_t			fLastTime;
		char			fKeyBuffer[3];
};

#endif
