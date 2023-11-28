#ifndef _SLIDER_TEST_H
#define _SLIDER_TEST_H

#include <BeBuild.h>
#include <Control.h>
#include <Message.h>

/*------------------------------------------------------------*/

const int32 kMinBarHeight = 18;
const int32 kMinBarWidth = 32;

const int32 kTriThumbWidth = 15;

const int32	kYGap = 4;

const rgb_color kWhiteColor = {255,255,255,255};
const rgb_color kWhiteGrayColor = {235,235,235,255};
const rgb_color kBGGrayColor = {216,216,216,255};
const rgb_color kLtGrayColor = {184,184,184,255};
const rgb_color kMedGrayColor = {144,144,144,255};
const rgb_color kDarkGrayColor = { 80, 80, 80, 255 };
const rgb_color kBlackColor = {0,0,0,255};

const rgb_color kUnusedColor = {153,153,153,255};

// snooze amounts in mousedown loop
const int32 kMinSnoozeAmount = 5000;
const int32 kMaxSnoozeAmount = 1000000;
const int32 kDefaultSnoozeAmount = 20000;

rgb_color DisabledColor(rgb_color c);
color_space BitsPerPixel();
float FontHeight(const BView* v, bool full);

/*------------------------------------------------------------*/

class TMultiThumbSlider;
class TThumb {
public:
							TThumb(TMultiThumbSlider* owner,
								BMessage* mouseDownMsg,
								BMessage* stdMsg,
								BMessage* modMsg);
virtual						~TThumb();

virtual	void				Draw(BView* target, BRect barRect);
		
virtual	int32				Value() const;
virtual	void				SetValue(int32);

		BPoint				InitialLocation() const;
		void				SetInitialLocation(BPoint);
virtual	BPoint				Location() const;
		void				SetLocation(BPoint);
		
virtual	BRect				Frame() const;
		
virtual	bool				HitTest(BPoint) const;

		BMessage*			MouseDownMessage() const;
		void				SetMouseDownMessage(BMessage*);
		BMessage*			Message() const;
		void				SetMessage(BMessage*);
		BMessage*			ModificationMessage() const;
		void				SetModificationMessage(BMessage*);
				
		TMultiThumbSlider*	Owner() { return fOwner; }

private:
		TMultiThumbSlider*	fOwner;
		int32				fValue;
		BPoint 				fInitialLocation;
		BPoint				fLocation;
		BMessage*			fMouseDownMsg;
		BMessage*			fStandardMsg;
		BMessage*			fModificationMsg;
};

const int32 kMaxThumbCount = 16;
const int32 kThumbHeight = 7;
const int32 kThumbWidth = 12;

class TMultiThumbSlider : public BControl {
public:

		enum {
			kMainThumb
		};


							TMultiThumbSlider(BRect frame,
									const char *name,
									const char *label,
									int32 minValue,
									int32 maxValue,
									uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW);
virtual						~TMultiThumbSlider();

virtual	void 				AttachedToWindow();
virtual	void				DetachedFromWindow();

virtual	void				MessageReceived(BMessage *msg);
virtual void 				FrameMoved(BPoint new_position);
virtual void				FrameResized(float w,float h);
virtual void 				KeyDown(const char * bytes, int32 n);
virtual void 				MouseDown(BPoint);
virtual void 				MouseUp(BPoint pt);
virtual void 				MouseMoved(BPoint pt, uint32 c, const BMessage *m);

virtual	void 				SetValue(int32);
virtual	void 				SetValueFor(int32, int32, bool postUpdates = true, bool lazy = true);
virtual int32				ValueFor(int32) const;
virtual int32				ValueForPoint(BPoint) const;
virtual int32				ValueForPoint(BPoint, int32) const;
virtual BPoint				PointForValue(int32) const;
virtual void				SetPosition(float);
virtual void				SetPosition(float, int32);
		float				Position() const;
		float				Position(int32) const;

virtual void				SetEnabled(bool on); 
	
virtual	void 				Draw(BRect update);
virtual void 				DrawSlider(BRect update);
virtual void 				DrawBar(BView *osv, BRect barFrame);
virtual void				DrawHashMarks(BView *osv, BRect barFrame, BRect updateRect);
virtual void 				DrawThumb(BView *osv, BRect barFrame, BRect updateRect);
		
virtual BRect				BarFrame() const;
virtual BRect				ThumbFrame() const;

virtual void 				GetPreferredSize( float *width, float *height);
virtual void 				ResizeToPreferred();
	
virtual status_t 			Invoke(BMessage *msg=NULL);

virtual void				SetSnoozeAmount(int32);
		int32				SnoozeAmount() const;

virtual	void 				SetKeyIncrementValue(int32 value);
		int32 				KeyIncrementValue()	const;
					
virtual	void 				SetBarColor(rgb_color);
		rgb_color 			BarColor() const;
virtual	void 				UseFillColor(bool, const rgb_color* c=NULL);
		bool				FillColor(rgb_color*) const;
		
		BView*				OffscreenView() const;
		BBitmap*			OffscreenBitmap() const;

		status_t 			AddThumb(int32 value,  BMessage* mdMsg=NULL,
								BMessage* stdMsg=NULL, BMessage* modMsg=NULL);
		status_t 			AddThumb(TThumb* t, int32 value);
		TThumb*				RemoveThumb(int32 index);
		int32 				PointInThumb(BPoint pt) const;
		int32				ThumbCount() const;
		TThumb*				ThumbAt(int32) const;
		
		BMessage*			MouseDownMessageFor(int32 index);
		void				SetMouseDownMessageFor(BMessage *message, int32 i);
		BMessage*			MessageFor(int32 index);
		void				SetMessageFor(BMessage *message, int32 i);
		BMessage*			ModificationMessageFor(int32 index);
		void				SetModificationMessageFor(BMessage *message, int32 i);
		
		bool				TrackingThumb(int32 which);
		int32				CurrentThumb() { return fCurrentThumb; }
		
		void				SetMaxValue(int32);
		void				SetMinValue(int32);
		int32				MinValue() const;
		int32				MaxValue() const;

private:

		void				_SetValue(int32 v, int32 i);
		int32				_Value(int32 i) const;

		BPoint				_Location() const;
		BPoint				_Location(int32 index) const;
		void				_SetLocation(BPoint p);
		void				_SetLocation(BPoint p, int32 index);
		
		BPoint				_InitialLocation(int32 index) const;
		void				_SetInitialLocation(BPoint p, int32 index);

		float				_MinPosition() const;
		float				_MaxPosition() const;

		void 				_InitObject(int32 minValue, int32 maxValue);
									
		int32				fSnoozeAmount;

		rgb_color 			fBarColor;
		rgb_color 			fFillColor;
		bool				fUseFillColor;
		
		int32 				fMinValue;
		int32 				fMaxValue;
		int32 				fKeyIncrementValue;
		
		BBitmap*			fOffScreenBits;
		BView*				fOffScreenView;
				
		BList*				fThumbList;
		int32				fCurrentThumb;

		float				fTrackingOffset;
};

inline void 
TMultiThumbSlider::SetMaxValue(int32 max)
{
	 fMaxValue = max;
}

inline void 
TMultiThumbSlider::SetMinValue(int32 min)
{
	 fMinValue = min;
}

inline int32 
TMultiThumbSlider::MinValue() const
{
	return fMinValue;
}

inline int32 
TMultiThumbSlider::MaxValue() const
{
	return fMaxValue;
}


#endif
