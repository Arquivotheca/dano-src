// ------------------------------------------------------------------
// OptionCheckBox.cpp
//
//   See OptionCheckBox.h.
//
//   by Nathan Schrenk (nschrenk@be.com)
// ------------------------------------------------------------------

#include "OptionCheckBox.h"
#include <stdio.h>

static const float FIELD_SPACING = 3.0;  // min space between label, size, & icon
static const float TOP_MARGIN = 3.0;  // from CheckBox.cpp
static const float BOTTOM_MARGIN = 3.0;  // from CheckBox.cpp
static const char *MAX_SIZE_STR = "MMMMMMMM"; // string with width as large as max size str. 

OptionCheckBox::OptionCheckBox(OptionalPackage *pkg, BMessage *msg)
		: BCheckBox(BRect(0, 0, 10, 10), pkg->name, pkg->name, msg,
					B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP)
{
	option = pkg;
	size_str = NULL;
	if (pkg->on || pkg->alwayson)
		SetValue(B_CONTROL_ON);
	if (pkg->alwayson)
		SetEnabled(false);
}

OptionCheckBox::~OptionCheckBox()
{
	delete option;
	free(size_str);
}

OptionalPackage *OptionCheckBox::GetPackage()
{
	return option;
}

// pass in a pointer to a char[] that is at least 7 chars long
void convert_size_to_string(off_t size, char *buf)
{
	if (size >= 1024LL * 1024 * 1024 * 1024)
		sprintf(buf, "%.1f TB", size / (1024.*1024.*1024.*1024.));
	else if (size >= 1024LL * 1024 * 1024)
		sprintf(buf, "%.1f GB", size / (1024.*1024.*1024.));
	else if (size >= 1024LL * 1024)
		sprintf(buf, "%.1f MB", size / (1024.*1024.));
	else
		sprintf(buf, "%.1f KB", size / (1024.));
}

// Has BCheckBox draw the box & label, then draws the size & icon on the right 
void OptionCheckBox::Draw(BRect update)
{
	BCheckBox::Draw(update);		// draw checkbox & label
	
	BBitmap *icon = option->icon;
	BRect icon_size = (icon == NULL) ? BRect(0, 0, 0, 0) : icon->Bounds();
	BRect bounds = Bounds();
	
	// build the size string if it hasn't yet been built
	if (size_str == NULL) {
		char buf[32];
		convert_size_to_string(option->size, buf);
		size_str = strdup(buf);
	}
	// draw size string
	font_height fh;
	GetFontHeight(&fh);
	float size_width = StringWidth(size_str);
	// XXX: change color if IsEnabled() != true?
	DrawString(size_str, BPoint(bounds.right - size_width - 1.0,
			   bounds.top + TOP_MARGIN + fh.ascent));

	// draw icon
	size_width = StringWidth(MAX_SIZE_STR);
	if (option->icon != NULL) {
		drawing_mode mode = DrawingMode();
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(icon, BPoint(Bounds().Width() - icon_size.Width() - size_width - 1.0, 2.0));
		SetDrawingMode(mode);
	}                                                                  
}

// include space needed for size and icon in the preferred size
void OptionCheckBox::GetPreferredSize(float *width, float *height)
{
	BCheckBox::GetPreferredSize(width, height); // get the parental opinion
	float icon_width(0.0), icon_height(0.0);
	if (option->icon != NULL) {
		BRect icon_size = option->icon->Bounds();
		icon_width = icon_size.Width();
		icon_height = icon_size.Height() + 2.0;
	}
	*width += (FIELD_SPACING * 2) + icon_width + StringWidth(MAX_SIZE_STR);
	*height = (*height > icon_height) ? *height + 2.0: icon_height + 2.0;
}

void OptionCheckBox::MouseMoved(BPoint , uint32 transit, const BMessage *)
{
	BLooper *looper;
	BMessage *msg;
	switch (transit) {
		case B_ENTERED_VIEW:
			looper = Looper();
			if (looper != NULL) {
				msg = new BMessage(OPTION_MOUSED_OVER);
				msg->AddPointer("message", option->desc);
				looper->PostMessage(msg, looper);
			}
			break;	
		case B_EXITED_VIEW:
			looper = Looper();
			if (looper != NULL) {
				msg = new BMessage(OPTION_MOUSED_OVER);
				// no 'message' field added here, indicating mouse exit from this view
				looper->PostMessage(msg, looper);
			}
			break;	
		default:
			break; // do nothing	
	}
}

