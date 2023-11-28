#ifndef SLIDER_MENU_ITEM_H
#define SLIDER_MENU_ITEM_H

#include <Bitmap.h>
#include <MenuItem.h>
#include <Window.h>

#include <MediaRoster.h>
#include <ParameterWeb.h>

class TSliderMenuItem : public BMenuItem {
public:
						//	min is minimum value
						//	max is maximum value
						//	sliderWidth is physical slider width to be drawn
						//	the rest are standard BMenuItem params
						TSliderMenuItem(int32 min, int32 max,
							BMessage *message, BMessage *modificationMessage = NULL, 
							char shortcut = 0, uint32 modifiers = 0);
virtual					~TSliderMenuItem();
						
virtual	void			Draw();
virtual	void			DrawContent();
virtual	void			Highlight(bool on);			// mouse tracking enabler/disabler

virtual	void			GetContentSize(float *width, float *height);

virtual	bool			IsEnabled() { return true; }
		
virtual	status_t		Invoke(BMessage *msg = NULL);
virtual	status_t		SendModificationMessage();

		void 			DrawBar(BView*);
		void			DrawHashMarks(BView*, BRect frame);
		void			DrawText(BView*);
		void 			DrawThumb(BView*);
		
		status_t		MouseWatcher();
		
		int32			Value() const;
virtual	void 			SetValue(int32);			//	value between min and max

		void			SetInitialValue(int32);		//	use in conjunction with menu to
													//	init the control on each showing

		float			Position() const;
virtual	void			SetPosition(float p);		//	position between 0 and 1
		
		int32 			MinValue() const { return fMinValue; }
		int32			MaxValue() const { return fMaxValue; }
		
		void			ShowPink(bool p) { fShowPink = p; }

		float			GetVolume();
		void			InitMediaServices();
private:		
		void			SetVolume(bool);
		
		void			InitObject();
		void			KillObject();
		
		void			InitBarFrames();
		
		int32			StartThread();
		void			StopThread();
		
		int32			ValueForPoint(BPoint p) const;
		
		thread_id		fThreadID;			//	mouse tracking thread
		
		BPoint			fLocation;			// 	current location of thumb
		float			fXOffset;			//	inset from border for ends and bar	
		float 			fYOffset;		
		
		BRect			fLeftBarFrame;		//	cached frames
		BRect			fMidBarFrame;
		BRect			fRightBarFrame;
		BRect			fBarFrame;
		BRect			fThumbFrame;
		
		BBitmap*		fThumbBits;			
		BBitmap*		fLeftCapBits;
		BBitmap*		fRightCapBits;
			
		bool			fInitd;
		BBitmap*		fOffScreenBits;
		BView*			fOffScreenView;
		
		bool			fFirstTime;			//	offscreen buffers
		int32			fInitialValue;
		
		int32			fValue;				//	current value
		int32			fMinValue;			//	min and max for value
		int32			fMaxValue;
		
		BMessage*		fModificationMessage;
		
		bool			fShowPink;

		BMediaRoster*	fMediaRoster;
		BParameter*		fMasterGain;
		float			fFloor;
		float			fCeiling;
		
		bool			fLooping;
};

class TVolumeMenu : public BPopUpMenu {
public:
						TVolumeMenu(const char* title, TSliderMenuItem* submenu);
virtual					~TVolumeMenu();
		void			AttachedToWindow();

private:
		TSliderMenuItem* fSubmenu;
};

const char* const kDeskbarItemName = "MediaReplicant";

class _EXPORT TDeskbarView;
class TDeskbarView : public BView
{
public:
							TDeskbarView();
							~TDeskbarView();
							
							TDeskbarView( BMessage * message);
		status_t 			Archive(BMessage * into, bool deep) const;
static	BArchivable* 		Instantiate(BMessage * from);

		void 				AttachedToWindow();
		void				DetachedFromWindow();
		
		void 				Draw(BRect area);
		void 				MouseDown(BPoint where);
		
		void				ShowVolumeMenu(BPoint);
		void				ShowContextMenu(BPoint where);
private:
		
		BBitmap* 			fSmallIcon;
};

void show_deskbar_icon(bool showIcon);

#endif
