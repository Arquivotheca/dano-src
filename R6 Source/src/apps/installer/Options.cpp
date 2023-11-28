// ------------------------------------------------------------------
// Options.cpp
//
//   See Options.h
//
//   by Nathan Schrenk (nschrenk@be.com)
// ------------------------------------------------------------------

#include "flag_data.h"
#include "Options.h"
#include <ScrollBar.h>
#include <Font.h>
#include <Window.h>


//--------------------------------------------------------------------------
// Implementation of GroupHeaderView follows
//

static const float LEFT_MARGIN = 3.0;
static const float RIGHT_MARGIN = 3.0;
static const float TOP_MARGIN = 2.0;
static const float BOTTOM_MARGIN = 2.0;

GroupHeaderView::GroupHeaderView(char *name)
	: BView(BRect(0, 0, 1, 1), NULL, B_FOLLOW_ALL, B_WILL_DRAW)
{
	this->name = strdup(name);
	// set bold font
	BFont font;
	GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(12.0);
	SetFont(&font);
	float height, width;
	GetPreferredSize(&height, &width);
	ResizeTo(height, width);
}

GroupHeaderView::~GroupHeaderView()
{
	free(name);
}


void GroupHeaderView::Draw(BRect )
{
	font_height fh;
	GetFontHeight(&fh);
	BRect bounds = Bounds();
	BPoint string_pos(bounds.left + LEFT_MARGIN,
							bounds.bottom - fh.descent - BOTTOM_MARGIN);
	DrawString(name, string_pos);
	// draw other nifty stuff here, like lines
	//float str_width = StringWidth(name);
	//StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
	//StrokeLine(bounds.LeftTop(), bounds.RightTop());
}

// Calculates preferred size based upon the margins, font, & name string
void GroupHeaderView::GetPreferredSize(float *width, float *height)
{
	font_height fh;
	GetFontHeight(&fh);
	float strwid = StringWidth(name);
	*width = strwid + LEFT_MARGIN + RIGHT_MARGIN;
	*height = fh.ascent + fh.descent + fh.leading + TOP_MARGIN + BOTTOM_MARGIN;
}



//--------------------------------------------------------------------------
// OptionView implementation follows
//

OptionView::OptionView(BRect frame)
	: BView(frame, "optionView", B_FOLLOW_ALL_SIDES, B_NAVIGABLE_JUMP | B_WILL_DRAW)// | B_NAVIGABLE)
{
	pref_height = 0;
	arranged = false;
	compensated_for_scrollview_brokenness = true;
	scroll = NULL;
}

// delete all the children and the groups
OptionView::~OptionView()
{
	Clear();
}

// Adds an option to the list.
// Do not use AddChild(...) to add a child to this view, use AddOption instead.
// When you are done adding options, call ArrangeOptions();
void OptionView::AddOption(OptionCheckBox *option)
{
	OptionalPackage *pkg = option->GetPackage();
	OptionGroup *grp = groups[pkg->group];
	if (grp == NULL) {
		grp = new OptionGroup(pkg->group);
		AddChild(grp->header_view);
		groups[pkg->group] = grp;
	}
	grp->AddItem(option);
	AddChild(option);
	arranged = false;
}

// Used to sort a BList containing OptionCheckBox objects so that the items
// are presented on the sceen in alphabetical order.
int option_checkbox_comparator(const void *b1, const void *b2)
{
	return strcmp((*(OptionCheckBox **)b1)->GetPackage()->name,
				  (*((OptionCheckBox **)b2))->GetPackage()->name);
}

// Arrange all of the child views in the list so that they are as wide as this view
// and are stacked on top of each other.
void OptionView::ArrangeOptions()
{
	if (!arranged) {
		BRect bounds = Bounds();
		float top(bounds.top), left(bounds.left), width, height;
		BView *view;

		// walk through group list
		OptionGroup *grp;
		map<const char*, OptionGroup*,  ltstr>::iterator grpiter = groups.begin();
		while (grpiter != groups.end()) {
			grp = grpiter->second;
			++grpiter;
			// position group heading
			view = grp->header_view;
			view->MoveTo(left, top);
			view->GetPreferredSize(&width, &height);
			view->ResizeTo(bounds.Width(), height);
			top += height;
			// sort the group
			grp->SortItems(option_checkbox_comparator);			
			// walk through options in this group
			int32 numitems = grp->CountItems();
			OptionCheckBox **option_array = (OptionCheckBox **)grp->Items();
			for (int32 i = 0; i < numitems; i++) {
				view = (OptionCheckBox *)option_array[i];
				// position this option
				view->MoveTo(left + 8, top);
				view->GetPreferredSize(&width, &height);
				view->ResizeTo(bounds.Width() - 8, height);
				top += height;
			}
		}
		//ResizeTo(bounds.Width(), top);
		pref_height = top;
		arranged = true;
		if (scroll != NULL) {
			ScrollTo(0.0, 0.0);
			AdjustScrollBars();
		}
	}
}


// Does necessary stuff when added to a scroll view
void OptionView::TargetedByScrollView(BScrollView *scroller)
{
	scroll = scroller;
	if (scroll != NULL) {
		arranged = false;
		compensated_for_scrollview_brokenness = false;
		ArrangeOptions();
	}
}

void OptionView::GetPreferredSize(float *width, float *height)
{
	if (!arranged) {
		ArrangeOptions();
	}
	BRect bounds = Bounds();
	*width = bounds.Width();
	*height = pref_height;
}

void OptionView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);
	if (!compensated_for_scrollview_brokenness) {
		AdjustScrollBars();
		compensated_for_scrollview_brokenness = true;
	}
//	if (IsFocus()) { // draw highlight around view
//		rgb_color navcolor = keyboard_navigation_color();
//		rgb_color oldhigh = HighColor();
//		SetHighColor(navcolor);
//		BRect rect(Bounds());
//		StrokeRect(rect);
//		SetHighColor(oldhigh);
//	}
}


//void OptionView::MakeFocus(bool focused)
//{
//	BView::MakeFocus(focused);
//	Draw(Bounds());
//}
//
//void OptionView::KeyDown(const char *bytes, int32 numBytes)
//{
//	switch (bytes[0]) {
//	
//	default:
//		BView::KeyDown(bytes, numBytes);
//		break;
//	}
//}


// Adjusts the scroll bars appropriately for the current frame dimensions
// and list content
void OptionView::AdjustScrollBars()
{
	if (scroll != NULL) {
		BRect frame = Frame();
		float frameWidth = frame.Width();
		float frameHeight = frame.Height(); 
		float dataWidth, dataHeight, diff, prop;
		GetPreferredSize(&dataWidth, &dataHeight);

//		printf("Frame = (%f x %f), Data = (%f x %f)\n", frameWidth,
//			frameHeight, dataWidth, dataHeight);
		// adjust horizontal scrollbar
		BScrollBar *sb = scroll->ScrollBar(B_HORIZONTAL);
		if (sb != NULL) {
			diff = dataWidth - frameWidth;
			prop = frameWidth / dataWidth;
			if (diff <= 0.0) {
				diff = 0.0;
				prop = 1.0;
			}
			sb->SetRange(0.0, diff);
			sb->SetProportion(prop);
			sb->SetSteps(5.0, frameWidth);
		}
		// adjust vertical scrollbar
		sb = scroll->ScrollBar(B_VERTICAL);
		if (sb != NULL) {
			diff = dataHeight - frameHeight;
			prop = frameHeight / dataHeight;
			if (diff <= 0.0) {
				diff = 0.0;
				prop = 1.0;	
			}
			sb->SetRange(0.0, diff);
			sb->SetProportion(prop);
			sb->SetSteps(15.0, frameHeight);
		}
	}
}

// Arrange the children when this view is attached to a window, if they aren't already
// arranged.
void OptionView::AttachedToWindow()
{
	if (!arranged) {
		ArrangeOptions();
	}
}

// Returns a NULL terminated array of OptionalPackage objects corresponding to those
// packages that are selected.  The caller has resposibility for free()ing the array,
// but not for deleting the objects pointed to by the array.
//
// Yeah, I know, this is sort of an annoying API. -nschrenk
OptionalPackage **OptionView::GetSelectedOptions()
{
	BWindow *window = Window();
	if (window)
		window->Lock();
	int32 count = CountChildren();
	OptionalPackage **packages(NULL);
	if (count > 0) {
		packages = (OptionalPackage **)malloc(sizeof(OptionalPackage *) * count);
		int32 ipkg = 0;

		// walk through group list
		OptionGroup *grp;
		OptionCheckBox *view;
		map<const char*, OptionGroup*,  ltstr>::iterator grpiter = groups.begin();
		while (grpiter != groups.end()) {
			grp = grpiter->second;
			++grpiter;
			// walk through options in this group
			int32 numitems = grp->CountItems();
			OptionCheckBox **option_array = (OptionCheckBox **)grp->Items();
			for (int32 i = 0; i < numitems; i++) {
				view = (OptionCheckBox *)option_array[i];
				if (view->Value() == B_CONTROL_ON) {
					packages[ipkg] = view->GetPackage();
					ipkg++;
				}
			}
		}
		packages[ipkg] = NULL;
	}
	if (window)
		window->Unlock();
	return packages;
}


// Calculates the combined size of all selected packages.
off_t OptionView::CalculateSelectedSize()
{
	off_t total(0);
	OptionalPackage **selected = GetSelectedOptions();
	if (selected != NULL) {
		int32 ipkg(0);
		OptionalPackage *pkg;
		while ((pkg = selected[ipkg++]) != NULL) {
			total += pkg->size;
		}
		free(selected);
	}
	return total;
}

// Enables or disables all the checkboxes in the list
void OptionView::SetEnabled(bool enabled)
{
	// walk through group list
	OptionGroup *grp;
	OptionCheckBox *view;
	map<const char*, OptionGroup*,  ltstr>::iterator grpiter = groups.begin();
	while (grpiter != groups.end()) {
		grp = grpiter->second;
		++grpiter;
		// walk through options in this group
		int32 numitems = grp->CountItems();
		OptionCheckBox **option_array = (OptionCheckBox **)grp->Items();
		for (int32 i = 0; i < numitems; i++) {
			view = (OptionCheckBox *)option_array[i];
			// turn this control on or off
			if (enabled && !(view->GetPackage()->alwayson)) {
				view->SetEnabled(true);
			}
			else {
				view->SetEnabled(false);
			}
		}
	}
}

// Removes all children 
void OptionView::Clear()
{
	// delete all child views (OptionCheckBoxes and GroupHeaderViews)
	BView *child;
	BWindow *window = Window();
	if (window && window->Lock()) {
		child = ChildAt(0);
		while (child != NULL) {
			RemoveChild(child);
			delete child;
			child = ChildAt(0);
		}	
		window->Unlock();
	}
	
	// delete all OptionGroups and 'wipe them off the map'
	OptionGroup *grp;
	map<const char*, OptionGroup*,  ltstr>::iterator grpiter = groups.begin();
	while (grpiter != groups.end()) {
		grp = grpiter->second;
		delete grp;
		++grpiter;
	}
	groups.clear();
	arranged = false;
	ArrangeOptions();
}
