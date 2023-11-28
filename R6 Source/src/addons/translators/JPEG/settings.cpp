/* settings.cpp */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <Application.h>
#include <Window.h>
#include <Alert.h>

#include <TranslatorAddOn.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <View.h>
#include <Rect.h>
#include <StringView.h>
#include <Slider.h>
#include <FindDirectory.h>

#include "settings.h"

static settings_pair jpeg_settings[MAX_SETTINGS];
static bool settings_initialized = false;

/* this isn't great, but it should work */
static void
load_settings(void)
{
	char buffer[B_PATH_NAME_LENGTH];
	char *value;
	FILE *fp;
	int i = 0;
		
	memset(jpeg_settings, 0, sizeof(jpeg_settings));
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, true,
						buffer, sizeof(buffer)) != B_OK)
		strcpy(buffer, "/boot/home/config/settings");

	strcat(buffer, "/JPEGTranslatorSettings");

	fp = fopen(buffer, "r");
	if (fp == NULL)
		return;
		
	while (fgets(buffer, sizeof(buffer), fp)) {
		buffer[strlen(buffer) - 1] = '\0';
		if (i >= MAX_SETTINGS)
			break;
		value = buffer;
		while (*value && !isspace(*value))
			value++;
		*value++ = '\0';
		while (*value && isspace(*value))
			value++;		
		strncpy(jpeg_settings[i].key, buffer, SETTINGS_KEY_LEN);
		strncpy(jpeg_settings[i].value, value, SETTINGS_VALUE_LEN);
		i++;
	}

	fclose(fp);

	settings_initialized = true;
}

static void
save_settings(void)
{
	char buffer[B_PATH_NAME_LENGTH];
	FILE *fp;
	int i;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, true,
						buffer, sizeof(buffer)) != B_OK)
		strcpy(buffer, "/boot/home/config/settings");

	strcat(buffer, "/JPEGTranslatorSettings");

	fp = fopen(buffer, "w");
	if (fp == NULL)
		/* XXXficus shouldn't be silently failing */
		return;
	
	for (i = 0; i < MAX_SETTINGS; i++) {
		if (jpeg_settings[i].key[0] == 0)
			break;
		fprintf(fp, "%s %s\n", jpeg_settings[i].key, jpeg_settings[i].value);
	}

	fclose(fp);
}

const char *
find_setting(const char *key)
{
	int i;
	
	if (!settings_initialized) {
		load_settings();
		settings_initialized = true;
	}
	
	for (i = 0; i < MAX_SETTINGS; i++) {
		if (!strcasecmp(key, jpeg_settings[i].key))
			return jpeg_settings[i].value;
	}

	return NULL;
}

/* what a stupid name */
status_t
set_setting(const char *key, const char *value)
{
	int i;
	
	if (!settings_initialized) {
		load_settings();
		settings_initialized = true;
	}
	
	for (i = 0; i < MAX_SETTINGS; i++) {
		if (!strcasecmp(key, jpeg_settings[i].key)
			|| jpeg_settings[i].key[0] == 0)
			break;
	}
	
	if (i == MAX_SETTINGS)
		return B_ERROR;
	
	strncpy(jpeg_settings[i].key, key, SETTINGS_KEY_LEN);
	strncpy(jpeg_settings[i].value, value, SETTINGS_KEY_LEN);

	return B_OK;
}

/* --------------------------------------------------------------- */

JPEGSettingsView::JPEGSettingsView(BRect rect)
				: BView(rect, "JPEGTranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW)
{

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	font_height fh;
	be_bold_font->GetHeight(&fh);
	BRect r(10, 15, 10 + be_bold_font->StringWidth(translatorName), 15 + fh.ascent + fh.descent);

	BStringView *str = new BStringView(r, "title", translatorName);
	str->SetFont(be_bold_font);
	AddChild(str);

	char versStr[100];
	sprintf(versStr, "v%d.%d.%d %s", (translatorVersion>>8), (translatorVersion>>4)&0xf,
		(translatorVersion)&0xf, __DATE__);
	r.top = r.bottom + 20;
	be_plain_font->GetHeight(&fh);
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(versStr);
	
	str = new BStringView(r, "info", versStr);
	str->SetFont(be_plain_font);
	AddChild(str);
	
	const char *copyright_string = "© 1998-1999 Be Incorporated";
	r.top = r.bottom + 10;
	r.bottom = r.top + fh.ascent + fh.descent;
	r.right = r.left + be_plain_font->StringWidth(copyright_string);
	
	str = new SwapStringView(r, "author", copyright_string, "by Ficus Kirkpatrick");
	str->SetFont(be_plain_font);
	AddChild(str);
	
	BMessage *m = new BMessage(OUTPUT_QUALITY_MSG);
	r.top = r.bottom + 10;
	r.bottom = r.top + 30;
	r.right = rect.right - 10;
	
	_slider = new BSlider(r, "quality", "Output Quality", m, 1, 100);
	_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	_slider->SetHashMarkCount(3);
	_slider->SetLimitLabels("Low", "High");
	AddChild(_slider);
}

void 
JPEGSettingsView::AllAttached(void)
{
	const char *v;
	
	BView::AllAttached();
	load_settings();
	v = find_setting(OUTPUT_QUALITY_SETTING);
	if (v != NULL)
		_output_quality = atoi(v);

	if (_output_quality < 1 || _output_quality > 100)
		_output_quality = OUTPUT_QUALITY_DEFAULT;
	
	_slider->SetValue(_output_quality);
	_slider->SetTarget(this);
}

void 
JPEGSettingsView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
	case OUTPUT_QUALITY_MSG:
		_output_quality = _slider->Value();
		break;
		
	default:
		BView::MessageReceived(msg);
	}
}

void 
JPEGSettingsView::DetachedFromWindow(void)
{
	char str[64];
	
	sprintf(str, "%d", _output_quality);
	set_setting(OUTPUT_QUALITY_SETTING, str);
	save_settings();
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

SwapStringView::SwapStringView(BRect r, const char *view_name,
								const char *a, const char *b)
				: BStringView(r, view_name, a)
{
	_label_a = a;
	_label_b = b;
	_is_a = true;
}

void
SwapStringView::MouseDown(BPoint p)
{
	_is_a = !_is_a;
	if (_is_a)
		SetText(_label_a);
	else
		SetText(_label_b);
}

status_t
MakeConfig(BMessage *msg, BView **view, BRect *bounds_buf)
{
//	BStringView *str;
//	BSlider *slider;
	
	//*view = new JPEGSettingsView(BRect(0, 0, 230, 150));
	//bounds_buf->Set(0, 0, 230, 150);
	bounds_buf->Set(0,0,239,239);
	*view = new JPEGSettingsView(*bounds_buf);
	(*view)->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	return B_OK;							
}

status_t
GetConfigMessage(BMessage *msg)
{
	return B_OK;
}

// Make it an app

class TranslatorWindow : public BWindow {
	public:
		TranslatorWindow(BRect rect, const char *name, window_type kind, uint32 flags);
		bool QuitRequested();
};

TranslatorWindow::TranslatorWindow(BRect rect, const char *name, window_type kind, uint32 flags) :
	BWindow(rect, name, kind, flags) {

}

bool TranslatorWindow::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

const char *translator_file_name = "JPEGTranslator";
const char *translator_window_title = "JPEG Settings";

int main() {
	char app_signature[255];
	sprintf(app_signature, "application/x-vnd.Be-%s", translator_file_name);
	BApplication app(app_signature);
	
	BRect extent(0, 0, 239, 239);
	BRect window_rect(extent);
	window_rect.OffsetTo(100, 100);
	TranslatorWindow *window = new TranslatorWindow(window_rect, translator_window_title,
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE);

	BView *config = NULL;
	status_t err = MakeConfig(NULL, &config, &extent);
	if ((err < B_OK) || (config == NULL)) {
		char error_message[255];
		sprintf(error_message, "%s does not currently allow user configuration.", translator_file_name);
		BAlert *alert = new BAlert("No Config", error_message, "Quit");
		alert->Go();
		exit(1);
	}
	
	window->ResizeTo(extent.Width(), extent.Height());
	window->AddChild(config);
	window->Show();
	app.Run();
	return 0;
}
