#include <Bitmap.h>
#include <Roster.h>
#include <Mime.h>
#include <Button.h>
#include <NodeInfo.h>
#include <stdio.h>
#include <math.h>
#include "InfoWindow.h"
#include "Bitmaps.h"
#include "MediaController.h"

const float kDivider = 65.0;
const float kExternalPadding = 12;
const float kInternalPadding = 4;
const float kIconHeight = 32;
const float kIconWidth = 32;

const rgb_color kDarkBackground = {171, 221, 161, 255};
const rgb_color kLightBackground = {255, 255, 255, 255};
const rgb_color kHighlightText = {195, 0, 0, 255};
const rgb_color kNormalText = {0, 0, 0, 255};
const rgb_color kHeaderText = {0, 0, 220, 255};

const window_look kWindowLook = B_TITLED_WINDOW_LOOK;
const window_feel kWindowFeel = B_NORMAL_WINDOW_FEEL;

InfoWindow::InfoWindow(BPoint pos, const char *title, const BString &text,
	entry_ref *ref, MediaController *controller)
	:	BWindow(BRect(pos.x, pos.y, pos.x + kInfoWindowWidth, pos.y + 1),
			"", kWindowLook, kWindowFeel, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fInfoView = new InfoView(Bounds(), title, text, ref, controller);
	BString windowTitle;
	windowTitle << title << " info";
	SetTitle(windowTitle.String());
	AddChild(fInfoView);
	SetPulseRate(1000000);
}

void InfoWindow::SetTo(const char *title, const char *description, entry_ref *ref,
	MediaController *controller)
{
	fInfoView->SetTo(title, description, ref, controller);
	BString windowTitle;
	windowTitle << title << " info";
	SetTitle(windowTitle.String());
}

bool InfoWindow::QuitRequested()
{
	Hide();
	return false;
}

InfoView::InfoView(BRect rect, const char *title, const BString &text, entry_ref *ref,
	MediaController *controller)
	:	BView(rect, "info_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED),
			fController(controller),
		fShowStats(false)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	fFileIcon = new BBitmap(BRect(0, 0, 31, 31), B_COLOR_8_BIT);
	SetTo(title, text.String(), ref, controller);

	GetFont(&fFont);
	fFont.GetHeight(&fFontMetrics);
	fWantsResize = true;
}

void InfoView::Draw(BRect)
{
	BRect bounds = Bounds();
	fWidth = bounds.Width();
	
	fURLRect.left = -1;
	fURLRect.right = -1;
	fURLRect.top = -1;
	fURLRect.bottom = -1;

	// Compute the location of the bottom of the header
	fFont = *be_plain_font;
	fFont.SetSize(20.0);
	SetFont(&fFont);
	font_height fh;
	fFont.GetHeight(&fh);
	fLine = kExternalPadding + kIconHeight / 2 - (fh.ascent + fh.descent) / 2
		+ fh.ascent;

	// Fill in background rects & divider line
	EraseBackground(BRect(0, 0, 1000000, kExternalPadding + 32));

	// Draw icon bitmap
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fFileIcon, BPoint(kDivider / 2 - kIconWidth / 2, kExternalPadding));

	// Draw title
	SetHighColor(kNormalText);
	SetLowColor(kLightBackground);
	MovePenTo(kDivider + kInternalPadding, fLine);
	const char *inStr = fTitle.String();
	char truncated[1024];
	char *outStr = truncated;
	fFont.GetTruncatedStrings(&inStr, 1, B_TRUNCATE_END, fWidth - kExternalPadding
		- kDivider, &outStr);
	DrawString(truncated);

	// Draw information
	fFont.SetSize(9.0);
	fFont.GetHeight(&fFontMetrics);
	SetFont(&fFont);
	float space = fh.descent + fh.leading + fFontMetrics.ascent + kExternalPadding;
	EraseBackground(BRect(0, kExternalPadding + 32, 1000000, fLine + space));
	fLine += space;
	DrawGroupHeader("File Info");
	char *str = (char*) fText.String();
	while (*str) {
		BString name, value;
		char *eol = strchr(str, '\n');
		char *colon = strchr(str, ':');
		if (colon != 0 && (eol != 0 && colon < eol)) {
			// Draw a label to the left of the divider line
			name.SetTo(str, (int)(colon - str));
			str = colon + 1;
			while (*str == ' ')
				str++;
		}

		if (eol == 0) {
			value.SetTo(str);
			break;
		} else
			value.SetTo(str, (int)(eol - str));

		// This is a bit of a hack, but a nice enough feature
		// to warrant it.  If there is a 'Home' attribute, this
		// file is the URL for a shoutcast station.  Store
		// the position of the data and allow the user to click on it.
		if (name == "Home") {
			fURLRect.left = kDivider + kInternalPadding;
			fURLRect.right = fURLRect.left + fFont.StringWidth(value.String()) + 10;
			fURLRect.top = fLine - fFontMetrics.ascent;
			fURLRect.bottom = fLine + fFontMetrics.descent;
			
			fURL = value.String();
			DrawURLAttr(name.String(), value.String());		
		} else
			DrawStringAttr(name.String(), value.String());
	
		str = eol + 1;
	}

	// Statistics.
	if (fShowStats) {
		fStatTop = fLine - 5;
		EraseBackground(BRect(0, fLine, 1000000, fLine + 10));
		fLine += 10;
		player_stats stats;
		if (!fController->Lock()) {
			Window()->Quit();
			return;
		}
		
		fController->GetStats(&stats);
		fController->Unlock();
	
		if (stats.has_video) {
			DrawGroupHeader("Frames");
			DrawInt64Attr("Displayed", stats.frames_played);
			BString drop;
			drop << stats.frames_dropped << " (" << (float) stats.frames_dropped /
				(float) stats.frames_played  * 100 << "%)";
			DrawStringAttr("Dropped", drop.String());
	
			DrawSpace();
			DrawGroupHeader("Display");
			BString drawingMode;
			if (stats.using_overlay)
				drawingMode << "Overlay ";
			else
				drawingMode << "DrawBitmap ";
	
			if (stats.num_buffers > 1)
				drawingMode << "(Double Buffered)";
			else
				drawingMode << "(Single Buffered)";
	
			DrawStringAttr("Mode", drawingMode.String());
	
		}
		
		DrawSpace();
		DrawGroupHeader("Processing");
		if (stats.has_video) {
			DrawPercentAttr("Decode Video", (double) stats.video_decode_time * 100);
			DrawPercentAttr("Draw", (double) stats.video_display_time / stats.total_play_time
				* 100); 
		}
		
		DrawPercentAttr("Decode Audio", (double) stats.audio_decode_time * 100);
	
		DrawSpace();
		DrawGroupHeader("I/O");
		DrawRateAttr("Raw stream", stats.raw_data_rate);
		if (stats.is_network_stream)
			DrawRateAttr("Connection", stats.connection_rate);
	
	}

	EraseBackground(BRect(0, fLine, 1000000, bounds.bottom));
	fLine += 5;

	BFont italic;
	italic = *be_plain_font;
	italic.SetFace(B_ITALIC_FACE);
	SetFont(&italic);
	MovePenTo(bounds.right - fFont.StringWidth("MMMMMM"), fLine + kExternalPadding / 2);
	SetHighColor(190, 0, 0);
	if (fShowStats)
		DrawString("Less"B_UTF8_ELLIPSIS);
	else
		DrawString("More"B_UTF8_ELLIPSIS);

	fToggleRect.left = bounds.right - fFont.StringWidth("MMMMMM");
	fToggleRect.top = fLine - 10;
	fToggleRect.bottom = fLine + 10;
	fToggleRect.right = bounds.right;

	if (fWantsResize) {
		Window()->ResizeTo(Bounds().Width(), fLine + kExternalPadding);
		fWantsResize = false;
	}
}

void InfoView::SetTo(const char *title, const char *description, entry_ref *ref,
	MediaController *controller)
{
	if (ref) {
		if (BNodeInfo::GetTrackerIcon(ref, fFileIcon, B_LARGE_ICON) != B_OK)
			fFileIcon->SetBits((const void*) kGenericMediaFileBitmap, 32 * 32, 0,
				B_COLOR_8_BIT);
	} else {
		fFileIcon->SetBits((const void *) kStreamFileBitmap, 32 * 32, 0,
			B_COLOR_8_BIT);
	}

	fTitle = title;
	fText = description;
	fController = controller;
	fWantsResize = true;
}

void InfoView::Pulse()
{
	if (fShowStats)
		Invalidate(BRect(kDivider, fStatTop, 1000000, 1000000));
}

void InfoView::BeginDraw()
{
	fLine = kIconHeight;
}

void InfoView::DrawStringAttr(const char *attr, const char *value, int)
{
	float lineHeight = fFontMetrics.ascent + fFontMetrics.descent + fFontMetrics.leading;

	// Fill in background rects & divider line
	EraseBackground(BRect(0, fLine, 1000000, fLine + lineHeight));

	SetHighColor(kHighlightText);
	SetLowColor(kLightBackground);
	MovePenTo(kDivider - (fFont.StringWidth(attr) + kInternalPadding), fLine);
	DrawString(attr);

	SetHighColor(kNormalText);
	SetLowColor(kLightBackground);
	MovePenTo(kDivider + kInternalPadding, fLine);
	char truncated[1024];
	char *outStr = truncated;
	fFont.GetTruncatedStrings(&value, 1, B_TRUNCATE_END, fWidth - kExternalPadding
		- kDivider, &outStr);
	DrawString(truncated);

	fLine += lineHeight;
}

void InfoView::DrawURLAttr(const char *attr, const char *value)
{
	BFont urlFont = *be_plain_font;
	urlFont.SetFace(B_UNDERSCORE_FACE);
	font_height height;
	urlFont.GetHeight(&height);
	float lineHeight = height.ascent + height.descent + height.leading;

	EraseBackground(BRect(0, fLine, 1000000, fLine + lineHeight));

	// Draw text
	SetHighColor(kHighlightText);
	SetLowColor(kLightBackground);
	MovePenTo(kDivider - (fFont.StringWidth(attr) + kInternalPadding), fLine);
	DrawString(attr);

	SetHighColor(kNormalText);
	SetLowColor(kLightBackground);
	MovePenTo(kDivider + kInternalPadding, fLine);
	char truncated[1024];
	char *outStr = truncated;

	SetFont(&urlFont);
	SetHighColor(0, 0, 255);
	urlFont.GetTruncatedStrings(&value, 1, B_TRUNCATE_END, fWidth - kExternalPadding
		- kDivider, &outStr);
	DrawString(truncated);
	SetFont(&fFont);

	fLine += lineHeight;
}

void InfoView::DrawGroupHeader(const char *name)
{
	float lineHeight = fFontMetrics.ascent + fFontMetrics.descent + fFontMetrics.leading;

	EraseBackground(BRect(0, fLine, 1000000, fLine + lineHeight));

	// Draw text
	SetHighColor(kHeaderText);
	SetLowColor(kLightBackground);
	MovePenTo(kDivider - (fFont.StringWidth(name) + kInternalPadding), fLine);
	DrawString(name);
	fLine += lineHeight;
}

void InfoView::DrawStringAttr(const char *attr, const char *value)
{
	DrawStringAttr(attr, value, strlen(value));
}

void InfoView::DrawFloatAttr(const char *attr, double value)
{
	char buffer[64];
	sprintf(buffer, "%.2g", value);
	DrawStringAttr(attr, buffer);
}

void InfoView::DrawPercentAttr(const char *attr, double value)
{
	char buffer[64];
	sprintf(buffer, "%.2g%%", value);
	DrawStringAttr(attr, buffer);
}


void InfoView::DrawInt32Attr(const char *attr, int32 value)
{
	char buffer[64];
	sprintf(buffer, "%li", value);
	DrawStringAttr(attr, buffer);
}

void InfoView::DrawInt64Attr(const char *attr, int64 value)
{
	char buffer[64];
	sprintf(buffer, "%Li", value);
	DrawStringAttr(attr, buffer);
}

void InfoView::DrawRateAttr(const char *attr, double size)
{
	char buffer[64];
	if (size < 1024)
		sprintf(buffer, "%.2f bytes/second", size);
	else if (size < (double) 0x100000) {
		size /= 1024;
		sprintf(buffer, "%.2fk/second", size);
	} else {
		size /= 0x100000;
		sprintf(buffer, "%.2fM/second", size);
	}
		
	DrawStringAttr(attr, buffer);
}


void InfoView::DrawSpace()
{
	EraseBackground(BRect(0, fLine, 1000000, fLine + 5));
	fLine += 5;
}

void InfoView::MouseDown(BPoint point)
{
	if (fToggleRect.Contains(point)) {
		fShowStats = !fShowStats;
		fWantsResize = true;
		Invalidate();
	} else if (fURLRect.Contains(point)) {
		InvertRect(fURLRect);
		Flush();
		const char *url_list[] = {fURL.String(), };
		be_roster->Launch(B_URL_HTTP, 1, (char**) url_list);
		InvertRect(fURLRect);
	}
}

void InfoView::EraseBackground(BRect rect)
{
	SetHighColor(kDarkBackground);
	FillRect(BRect(0, rect.top, kDivider, rect.bottom));
	SetHighColor(kLightBackground);
	FillRect(BRect(kDivider, rect.top, rect.right, rect.bottom));
	SetHighColor(kNormalText);
	StrokeLine(BPoint(kDivider, rect.top), BPoint(kDivider, rect.bottom));
}


