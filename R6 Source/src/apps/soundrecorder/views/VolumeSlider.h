#include <Slider.h>

class BBitmap;

class TVolumeSlider : public BSlider {
public:
							TVolumeSlider(	BRect frame,
											BMessage *message,
											int32 minValue,
											int32 maxValue,
											BBitmap *optionalWidget,
											uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
											uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS);
virtual						~TVolumeSlider();

virtual void				GetPreferredSize(float*, float*);
		
		void 				DrawBar();
		void				DrawHashMarks();
		void 				DrawThumb();
		void				DrawFocusMark() {}
		
		BRect				BarFrame() const;
		BRect				ThumbFrame() const;
		BPoint				ThumbPosition() const;

static	BBitmap*			NewVolumeWidget();
		// temporary call that hardcodes a speaker graphic

		void				ValueUpdated(float);
							// called when volume of the mixer changes - does not
							// fight back during tracking
protected:
virtual void				AttachedToWindow();

private:
		int32				fMin;
		int32				fMax;
		BBitmap*			fThumbBits;
		BBitmap*			fLeftCapBits;
		BBitmap*			fRightCapBits;
		BBitmap*			fOptionalWidget;
};
