#ifndef _INFO_WINDOW_H
#define _INFO_WINDOW_H

#include <View.h>
#include <Window.h>
#include <String.h>
#include "MediaFile.h"

class InfoView;
class MediaController;

const float kInfoWindowWidth = 290;

class InfoWindow : public BWindow {
public:

	InfoWindow(BPoint position, const char *title, const BString &text,
		entry_ref *ref, MediaController *controller);
	void SetTo(const char *title, const char *description, entry_ref *ref,
		MediaController *controller);
	virtual bool QuitRequested();

	InfoView *fInfoView;
};

class InfoView : public BView {
public:

	InfoView(BRect rect, const char *title, const BString &text,
		entry_ref *ref, MediaController *controller);
	void SetTo(const char *title, const char *description, entry_ref *ref,
		MediaController *controller);

private:
	virtual void Draw(BRect);
	virtual void Pulse();
	virtual void MouseDown(BPoint);
	void EraseBackground(BRect rect);
	void BeginDraw();
	void DrawStringAttr(const char *attr, const char *value, int length);
	void DrawStringAttr(const char *attr, const char *value);
	void DrawURLAttr(const char *attr, const char *value);
	void DrawFloatAttr(const char *attr, double value);
	void DrawPercentAttr(const char *attr, double value);
	void DrawInt32Attr(const char *attr, int32 value);
	void DrawInt64Attr(const char *attr, int64 value);
	void DrawGroupHeader(const char *name);
	void DrawRateAttr(const char *attr, double rate);
	void DrawSpace();

	BBitmap *fFileIcon;
	BString fTitle;
	BString fText;

	float fLine;
	float fWidth;
	float fStatTop;
	font_height fFontMetrics;
	BFont fFont;
	MediaController *fController;
	bool fWantsResize;
	bool fShowStats;
	BRect fToggleRect;
	
	// For shoutcast home URL
	BRect fURLRect;
	BString fURL;
};



#endif
