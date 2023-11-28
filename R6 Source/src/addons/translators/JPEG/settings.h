/* settings.h */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <StringView.h>
#include <Rect.h>
#include <Point.h>

class BSlider;

#define MAX_SETTINGS			16
#define SETTINGS_KEY_LEN		64
#define SETTINGS_VALUE_LEN		128
struct settings_pair
{
	char key[SETTINGS_KEY_LEN];
	char value[SETTINGS_VALUE_LEN];
};

const char *find_setting(const char *key);

#define OUTPUT_QUALITY_MSG			'tori'
#define OUTPUT_QUALITY_SETTING		"output_quality"
#define OUTPUT_QUALITY_DEFAULT		75

class JPEGSettingsView : public BView
{
public:
	JPEGSettingsView(BRect r);

	void AllAttached(void);
	void MessageReceived(BMessage *msg);
	void DetachedFromWindow(void);
	
private:
	BSlider *_slider;
	int32 _output_quality;
	
};

class SwapStringView : public BStringView
{
public:
	SwapStringView(BRect r, const char *view_name, const char *a, const char *b);

	void MouseDown(BPoint p);
	
private:
	const char *_label_a;
	const char *_label_b;
	bool _is_a;
};

#endif /* SETTINGS_H */
