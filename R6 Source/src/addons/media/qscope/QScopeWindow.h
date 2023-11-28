#ifndef QSCOPEWINDOW_H
#define QSCOPEWINDOW_H

#include <Window.h>

class QScopeSubscriber;
class BitmapView;
class BBitmap;
class DrawLooper;
class TSliderView;

// Window object
class QScopeWindow : public BWindow 
{
public:
				QScopeWindow();
	virtual bool QuitRequested(void);
	virtual void MessageReceived(BMessage *msg);

	QScopeSubscriber *	Subscriber() { return the_subscriber;};
	
private:
	static void trigger_level_callback(float value, void *arg);
	static void hold_off_callback(float value, void *arg);

	BitmapView *main_view;
	BBitmap *the_bitmap;

	DrawLooper *the_looper;

	BDACStream *dac_stream;
	BADCStream *adc_stream;
	QScopeSubscriber *the_subscriber;

	friend class TSliderView;

	bool illumination;		// Backlight
};

#endif
