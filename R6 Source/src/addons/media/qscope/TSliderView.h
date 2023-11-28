/*
 *  TSliderView.h - Horizontal slider view
 *
 *  Created by Robert Polic, modified by Christian Bauer
 */

#ifndef __T_SLIDER_VIEW__
#define __T_SLIDER_VIEW__


// Callback function
typedef void (*slider_func)(float, void *);


class TSliderView : public BView {
public:
	TSliderView(BRect frame, char *name, float val, slider_func func, void *arg);
	~TSliderView();

	virtual	void Draw(BRect);
	virtual void MouseDown(BPoint);

	void DrawSlider();
	void SetValue(float);
	float Value();

private:
	float fValue, back_width;
	BBitmap* fSlider;
	BBitmap* fKnob;
	BView* fOffView;
	slider_func callback;
	void *callback_arg;
};

#endif
