#ifndef TMTM_SLIDER
#define TMTM_SLIDER

#include <InterfaceDefs.h>
#include <Bitmap.h>
#include "multithumbslider.h"

const rgb_color kDarkGreen = {144, 186, 136, 255};
const rgb_color kFillGreen = {171, 221, 161, 255};
const rgb_color kBorderGray = {128, 128, 128, 255 };

class TimeThumb : public TThumb {
public:
	TimeThumb(TMultiThumbSlider *, BMessage * = 0, BMessage * = 0, BMessage * = 0);
	virtual ~TimeThumb();

	bigtime_t TimeValue() const;
	void SetTimeValue(bigtime_t);
		
protected:
	BBitmap *fBits;

private:
	bigtime_t timeValue;
};

class TLeftThumb : public TimeThumb {
public:
	TLeftThumb(TMultiThumbSlider* owner, BMessage * = 0, BMessage * = 0,
		BMessage * = 0);

	virtual void Draw(BView *target, BRect barFrame);
	virtual	BRect Frame() const;
	virtual bool HitTest(BPoint) const;
};

class TCenterThumb : public TimeThumb {
public:
	TCenterThumb(TMultiThumbSlider* owner, BMessage * = 0, BMessage * = 0,
		BMessage * = 0);
	
	virtual void Draw(BView *target, BRect barFrame);
	virtual	BRect Frame() const;
	virtual bool HitTest(BPoint) const;
};

class TRightThumb : public TimeThumb {
public:
	TRightThumb(TMultiThumbSlider* owner, BMessage * = 0, BMessage * = 0,
		BMessage * = 0);

	virtual void Draw(BView *target, BRect barFrame);
	virtual	BRect Frame() const;
	virtual bool HitTest(BPoint) const;
};

class TransportView;
		
enum label_placement {
	kLabelNone,
	kLabelTop,
	kLabelCenter,
	kLabelCenterWhileTracking,
	kLabelBottom
};
		
enum {
	kMainThumb = TMultiThumbSlider::kMainThumb,
	kLeftThumb,
	kRightThumb
};

class TMultiThumbMediaSlider : public TMultiThumbSlider {
public:
							TMultiThumbMediaSlider(TransportView* parent, BRect frame,
									const char *name,
									const char *label,
									int32 minValue,
									int32 maxValue,
									uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS);
virtual						~TMultiThumbMediaSlider();

		void				AttachedToWindow();
		void				GetPreferredSize(float* w, float* h);
		void				ResizeToPreferred();
virtual void 				MouseUp(BPoint pt);
		
virtual void				SetValueFor(int32 value, int32 thumb, bool postUpdates = true, bool lazy = true);

		void				AddThumbs(int32 v1, int32 v2, int32 v3,
								BMessage* mainMdMsg=NULL, BMessage* mainMsg=NULL, BMessage* mainModMsg=NULL,
								BMessage* leftMdMsg=NULL, BMessage* leftMsg=NULL, BMessage* leftModMsg=NULL,
								BMessage* rightMdMsg=NULL, BMessage* rightMsg=NULL, BMessage* rightModMsg=NULL);

		BRect 				LeftUsedRect(BRect barFrame) const;
		BRect 				RightUsedRect(BRect barFrame) const;
		BRect 				FillRect(BRect barFrame) const;
		
virtual void 				DrawBar(BView* osv, BRect barFrame);
virtual BRect				BarFrame() const;

virtual void				DrawHashMarks(BView* osv, BRect barFrame, BRect updateRect);

		void				DrawThumb(BView *osv, BRect barFrame, BRect updateRect);
		
		void				SetLabelPlacement(label_placement);
		label_placement		LabelPlacement() const;
		
		float				LabelHeight() const;
		
		void				DrawLabel(BView* osv, BRect barFrame);
		void				DrawLabelBox(BView* osv, BRect frame, rgb_color fill,
								rgb_color top, rgb_color bottom);								
		void				DrawText(BView* osv, BRect, bool, bigtime_t, rgb_color);
		float				DrawDigit(BView* osv, const BBitmap *map, int32 digit,
								BPoint destLoc);
								
		void				UpdatePosition(float pos, int32 thumb);
		void				UpdateTime(bigtime_t time, int32 thumb);
		void				UpdateMainTime(bigtime_t time);
		
		void				SetThumbCollisionDetection(bool);
		bool				ThumbCollisionDetection() const;
		void				SetEnabledPortion(float);


private:
		BRect				TrackingLabelRect(int32 index, BRect barFrame, BPoint pos) const;
		
		TransportView*		fParent;		
		BBitmap*			fDigitBits;
		BBitmap*			fInvertedDigitBits;
		BBitmap*			fLeftCap;
		BBitmap*			fRightCap;
		BBitmap*			fDropHereBits;
		label_placement		fLabelPlacement;
		bool				fThumbCollisionDetection;
		float				fEnabledPortion;
};

#endif
