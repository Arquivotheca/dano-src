#ifndef TIME_WINDOW_H
#define TIME_WINDOW_H

#include <Box.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <StringView.h>
#include <TabView.h> 

#include "time_utils.h"
#include "AnalogClock.h"
#include "Calendar.h"
#include "ListSelector.h"
#include "TimeZone.h"

#if __INTEL__
# define SHOW_GMT 1
#else
# define SHOW_GMT 0
#endif

void write_rtc_setting(bool is_local);
bool get_rtc_setting(void);

// ************************************************************************** //

class TDateSelector : public TListSelector {
public:
							TDateSelector(BRect);
							~TDateSelector();
						
		void				KeyDown(const char *key, int32 numBytes);
		void				MouseDown(BPoint);
		
		void				SetTargetList(short);
		void				NextTargetList();
		void				PreviousTargetList();

		void				SetDate(const char* date);
private:
		char				fKeyBuffer[10];
		bigtime_t			fLastTime;
};

class TTimeSelector : public TListSelector {
public:
							TTimeSelector(BRect);
							~TTimeSelector();
						
		void				KeyDown(const char *key, int32 numBytes);
		void				MouseDown(BPoint);
		
		void				SetTargetList(short);
		void				NextTargetList();
		void				PreviousTargetList();
		
		void				SetTime(const char* time);
private:
		char				fKeyBuffer[3];
		bigtime_t			fLastTime;
};

class TDTView : public BView {
public:
						TDTView(BRect frame, const char*);
						~TDTView() {}
				
		void			AttachedToWindow();
		void			DetachedFromWindow();
		void			Draw(BRect);
		void			MessageReceived(BMessage*);
		void			MouseDown(BPoint);
		void			Pulse();
		
		void			GetPreferredSize(float*, float*);
		void			ResizeToPreferred();
		
		void			UpdateCalendar(bool changeDays);
		void			UpdateControls(bool forceUpdate=false);
		
		void			GetCurrentDate(int32* month, int32* day, int32* year);
		void			GetCurrentTime(int32* hour, int32* minute, int32* second,
							bool* interval);
							
		void			UpdateSystemDate(int32 month, int32 day, int32 year);
		void			UpdateSystemTime(int32 hour, int32 minute, int32 second,
							bool interval);
	
private:
		bool			fFirstTime;
		TDateSelector* 	fDateSelector;
		TCalendar*		fCalendar;
		
		TTimeSelector*	fTimeSelector;
		TAnalogClock*	fAnalogClock;
		
#if SHOW_GMT
		BStringView*	fGMTLabel;
		BRadioButton*	fGMTBtn;
		BRadioButton*	fLocalTimeBtn;
#endif
	
		BStringView*	fTimeZoneString;
		TTimeZone* 		fTimeZone;
		BButton*		fSetTimeZoneBtn;	
};

class TConfigView : public BTabView {
public:
						TConfigView(BRect frame);
						~TConfigView();

		void			AttachedToWindow();
		void 			MessageReceived(BMessage *msg);
		void			DrawBox(BRect selFrame);
	
private:
		TDTView*		fDTView;
		TTimeZone*		fTimeZone;
};

// ************************************************************************** //

class TTimeWindow : public BWindow {
public:
						TTimeWindow();
						~TTimeWindow();

		void			FrameResized(float w, float h);
		void 			MessageReceived(BMessage *msg);
		bool 			QuitRequested();

		void			AddParts();

private:
		TConfigView*	fConfigView;
};

#endif /*  TIME_WINDOW_H  */
