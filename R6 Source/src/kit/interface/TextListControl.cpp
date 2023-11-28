//
// TextListControl.cpp
//


/*
	TODO:
		- The current data structure used for storing choice strings is horribly
		  inefficient at searching by key.  A better data structure should be used.
		
		- AutoPrune method is currently inefficient, as it copies everything into
		  a separate list.
		
		- Fix popup behavior when the widget is near the bottom of the screen.  The
		  choice list should be able to go above the text input area.

		- The choice window should size itself in a smart manner so that it
		  is smaller than requested if there are few items

		- Add support for other navigation keys, like page up, page down, home, end.

		- Change auto-complete behavior to be non-greedy, or perhaps add some
		  way to cycle through the matching choices?
*/

#include <Debug.h>
#include <Glyph.h>
#include <Menu.h>			// for menu_info
#include <ObjectList.h>		// from inc/support_p
#include <Picture.h>
#include <Region.h>
#include <ScrollBar.h>
#include <Shape.h>
#include <String.h>
#include <Window.h>
#include <TextListControl.h>

#ifndef _MENU_PRIVATE_H
#include "MenuPrivate.h"
#endif

//#include <TextControlPrivate.h>
#include "TextControlPrivate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//static const uint32 kTextControlInvokeMessage	= 'tCIM';
const uint32 kTextInputModifyMessage		= 'tIMM';
const uint32 kPopupButtonInvokeMessage		= 'pBIM';
const uint32 kPopupWindowHideMessage		= 'pUWH';
const uint32 kWindowMovedMessage			= 'wMOV';

// control messages that get sent to the popup window:

const uint32 kInvalidateListMessage		= 'cbIL';
	// causes popup to invalidate list and adjust scrollbar
const uint32 kSelectItemMessage			= 'cbSI';
	// selects the item specified in the int32 "index" member, old item in "old_index"
const uint32 kSetDisplayedCountMessage	= 'cbCD';
	// specifies the number of choices to display in the "count" member
const uint32 kSetDisplayMatchesOnlyMessage	= 'cbDM';
	// specifies the list display state in the "matches_only" member
const uint32 kTextChangedMessage		= 'cbTC';
	// specifies that the box text changed
	
const float kTextInputMargin			=  3.0f;
const float kLabelRightMargin		=  6.0f;
const float kButtonRightMargin		=  3.0f;

namespace BPrivate {

// -------------------------------------------------------------------------------

// -------------------------------------------------------------------------------
class KeyedString : public BString {
public:
					KeyedString(int32 key, const char *string)
						: BString(string), key(key) {}

	virtual			~KeyedString() {}
	
	int32	key;
};

class KeyedStringObjectList : public BObjectList<KeyedString> {};

// -------------------------------------------------------------------------------

class TextListTextInput: public _BTextInput_ {
public:
						TextListTextInput(BRect rect, BRect trect, ulong rMask, ulong flags);
						TextListTextInput(BMessage *data);
	virtual				~TextListTextInput();
	static	BArchivable	*Instantiate(BMessage *data);
	virtual	status_t	Archive(BMessage *data, bool deep = true) const;
		
	virtual void		KeyDown(const char *bytes, int32 numBytes);
	virtual void		MakeFocus(bool state);

protected:
	virtual void		InsertText(const char				*inText, 
								   int32					inLength, 
								   int32					inOffset,
								   const text_run_array		*inRuns);
	virtual void		DeleteText(int32 fromOffset, int32 toOffset);
};

// -------------------------------------------------------------------------------

class TextListWindow : public BWindow
{
public:
						TextListWindow(BTextListControl *box, int32 count, bool displayMatchesOnly);
	virtual				~TextListWindow();
	virtual void		WindowActivated(bool active);
	virtual void		FrameResized(float width, float height);
	virtual void		MessageReceived(BMessage *message);
	virtual void		Quit();
	
	TextListView		*ListView();
	BScrollBar			*ScrollBar();

	BRect				fHole;
	
private:

	void				SetChoiceCount(int32 count);
	void				DoPosition();

	BScrollBar			*fScrollBar;
	TextListView		*fListView;
	BTextListControl	*fParent;
	int32				fChoiceCount;
};

// -------------------------------------------------------------------------------

// TextListView is similar to a BListView, but it's implementation is tied to
// BTextListControl.  BListView is not used because it requires that a BListItem be
// created for each choice, which is unacceptable for representing a BTextList
// for a number of reasons.  TextListView just pulls the choice strings directly
// from the BTextList and draws them.

class TextListView : public BView
{
public:
						TextListView(	BRect frame, BTextListControl *parent);
	virtual				~TextListView();
		
	virtual void		Draw(BRect update);
	virtual void		MouseDown(BPoint where);
	virtual void		MouseUp(BPoint where);
	virtual void		MouseMoved(BPoint where, uint32 transit, const BMessage *dragMessage);
	virtual void		AttachedToWindow();
	virtual void		DetachedFromWindow();
	virtual void		SetFont(const BFont *font, uint32 properties = B_FONT_ALL);

//	virtual void		WindowActivated(bool active)
//		{
//			printf("TextListView::WindowActivated(%s)\n", active ? "true" : "false");
//		}
	
	void				ScrollToSelection();
	void				InvalidateItem(int32 index, bool force = false);
	BRect				ItemFrame(int32 index);
	void				AdjustScrollBar();
	float				LineHeight();

	void				SetMatchesOnly(bool matchesOnly);

private:
	
	BPoint				fClickLoc;
	font_height			fFontHeight;
	bigtime_t			fClickTime;

	rgb_color			fSelLowCol;
	rgb_color			fSelHighCol;
	rgb_color			fLowCol;
	rgb_color			fHighCol;

	int32				fSelIndex;
	BTextListControl 			*fParent;
	bool				fTrackingMouseDown;
	bool				fClickedInWindow;
	bool				fMatchesOnly;
};

// ----------------------------------------------------------------------------

} // namespace BPrivate

using namespace BPrivate;

// -------------------------------------------------------------------------------

BTextListControl::BTextListControl(BRect frame, const char *name, const char *label,
					const char *initial_text, BMessage *message,
					BChoiceList *choiceList, bool ownChoiceList,
					uint32 resizeMask, uint32 flags)
	: BTextControl(frame, name, label, initial_text, message,
					new TextListTextInput(BRect(0, 0, 10, 10), BRect(0, 0, 10, 10),
						B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS),
					resizeMask, flags),
	fChoiceList(choiceList),
	fOwnsChoiceList(ownChoiceList)
{
	if ((flags & B_NAVIGABLE) != 0) {
		BView::SetFlags(Flags() | B_NAVIGABLE);
	}
	InitData(NULL);
}

BTextListControl::BTextListControl(BMessage *data)
	: BTextControl(data)
{
	InitData(data);
}


void BTextListControl::InitData(BMessage *data)
{
	BTextView *textView = TextView();
	
	if (data != NULL) {
		// XXX: de-archive from data message
			
	} else {
		fUserText.SetTo(B_EMPTY_STRING);
		fTextEnd = 0;
		fSelected = -1;
		fSelectedIndex = -1;
		fCompletionIndex = -1;
		fAutoComplete = false;
		fMaxVisible = 10;
		fAutoPrune = false;
		fMatchList = NULL;
	}
	
//	const char *text = textView->Text();
//	fTextEnd = strlen(text) - 1;

	fButtonRect = textView->Frame();
	fButtonRect.InsetBy(-kTextInputMargin, -kTextInputMargin);
	fButtonRect.right = fButtonRect.left - (kButtonRightMargin + 1);
	fButtonRect.left = fButtonRect.right - (BPopUpMenuGlyph::kButtonWidth - 1);
	fButtonRect.InsetBy(0, 1);
	fButtonRect.bottom -= 1;
	
	fChoiceWindow = NULL;
	fButtonDepressed = false;
	fDepressedWhenClicked = false;
	fTrackingButtonDown = false;
}

BTextListControl::~BTextListControl()
{
	if (fChoiceWindow != NULL) {
		HideChoiceWindow(false);
	}
	if (fMatchList != NULL) {
		delete fMatchList;
	}
	if (fOwnsChoiceList && fChoiceList != NULL) {
		delete fChoiceList;
	}
}

BArchivable *BTextListControl::Instantiate(BMessage */*data*/)
{
	// XXX: not yet implemented
	TRESPASS();
	return NULL;
}

status_t BTextListControl::Archive(BMessage */*data*/, bool /*deep*/) const
{
	// XXX: not yet implemented
	TRESPASS();
	return B_ERROR;
}


void BTextListControl::SetChoiceList(BChoiceList *list, bool ownership)
{
	if (fChoiceList != NULL && fOwnsChoiceList) {
		if (fChoiceList->Lock()) {
			fChoiceList->SetOwner(NULL);
			fChoiceList->Unlock();
		}
		delete fChoiceList;		
	}
	fChoiceList = list;
	fOwnsChoiceList = ownership;
	if (fOwnsChoiceList && list->Lock()) {
		list->SetOwner(this);
		list->Unlock();
	}
	fSelected = -1;
	fSelectedIndex = -1;
	ChoiceListChanged();
}

BChoiceList *BTextListControl::ChoiceList() const
{
	return fChoiceList;
}

void BTextListControl::ChoiceListChanged()
{
	if (fChoiceList == NULL) {
		return;
	}
	
	if (fSelected >= 0) {
		if (fAutoPrune) {
			BuildMatchList();
		} else {
			fChoiceList->Lock();
			fChoiceList->GetChoiceByKey(fSelected, &fSelectedIndex);
			fChoiceList->Unlock();
		}
	}

	if (fChoiceWindow != NULL) {
		BMessage msg(kInvalidateListMessage);
		fChoiceWindow->PostMessage(&msg, fChoiceWindow);
	}
	
}

void BTextListControl::SelectByKey(int32 key, bool changeTextSelection)
{
	if (fChoiceList == NULL) {
		return;
	}

	int32 index = -1;

	if (fAutoPrune) {
		index = MatchListIndexForKey(key);
	} else {
		const char *choice;
		fChoiceList->Lock();
		choice = fChoiceList->GetChoiceByKey(key, &index);
		fChoiceList->Unlock();
	}

	if (index >= 0) {
		SelectByIndex(index, changeTextSelection);
	}
}


void BTextListControl::SelectByIndex(int32 index, bool changeTextSelection)
{
	if (fChoiceList == NULL) {
		return;
	}
	
	fChoiceList->Lock();

	if (fChoiceList->CountChoices() <= index) {
		fChoiceList->Unlock();
		return;
	}
	
	int32 oldIndex = fSelectedIndex;
	int32 count = (fAutoPrune) ? fMatchList->CountItems() : fChoiceList->CountChoices();
	if (index >= count) {
		index = count - 1;
	}

	if (index >= 0) {
		const char *choice;
		int32 newKey;
		// fill choice from the appropriate list
		if (fAutoPrune) {
			KeyedString *str = fMatchList->ItemAt(index);
			choice = str->String();
			newKey = str->key;
		} else {
			choice = fChoiceList->GetChoiceAt(index, &newKey);
		}

		fSelectedIndex = index;
		fSelected = newKey;
		
		if (changeTextSelection) {

			BTextView *text = TextView();
			text->SetText(choice);
			text->SelectAll();
		}
	} else { // deselect
		fSelectedIndex = -1;
		fSelected = -1;
		if (changeTextSelection) {
			// deselect text and set insertion point after last character
			BTextView *text = TextView();
			int32 insertPoint = text->TextLength();
			text->Select(insertPoint, insertPoint);
		}
	}

	fChoiceList->Unlock();
	
	if (fChoiceWindow != NULL) {
		BMessage msg(kSelectItemMessage);
		msg.AddInt32("index", index);
		msg.AddInt32("old_index", oldIndex);
		fChoiceWindow->PostMessage(&msg, fChoiceWindow);
	}
}

int32 BTextListControl::CurrentSelection() const
{
	return fSelected;
}

void BTextListControl::SetAutoCompletion(bool enabled)
{
	fAutoComplete = enabled;
}

void BTextListControl::SetMaxVisibleItems(int32 count)
{
	fMaxVisible = count;
	if (fChoiceWindow != NULL) {
		BMessage msg(kSetDisplayedCountMessage);
		msg.AddInt32("count", count);
		fChoiceWindow->PostMessage(&msg, fChoiceWindow);
	}
}

int32 BTextListControl::MaxVisibleItems() const
{
	return fMaxVisible;
}

void BTextListControl::SetAutoPrune(bool enabled)
{
	if (fAutoPrune != enabled) {
		fAutoPrune = enabled;
		if (enabled) {
			fMatchList = new KeyedStringObjectList();	
			BuildMatchList();
		} else {
			// empty the list
			KeyedString *str;
			bool translateSelection = ((fChoiceList != NULL) && fChoiceList->Lock());
			while ((str = fMatchList->RemoveItemAt(0)) != NULL) {
				if (translateSelection && (fSelected == str->key)) {
					// translate the selection index
					fChoiceList->GetChoiceByKey(fSelected, &fSelectedIndex);
				}
				delete str;
			}
			if (translateSelection) {
				fChoiceList->Unlock();
			}
			delete fMatchList;
			fMatchList = NULL;
		}
		BMessage msg(kSetDisplayMatchesOnlyMessage);
		msg.AddBool("matches_only", enabled);
		fChoiceWindow->PostMessage(&msg, fChoiceWindow);
	}
		
}

bool BTextListControl::IsAutoPruneEnabled() const
{
	return fAutoPrune;
}


bool BTextListControl::IsAutoCompletionEnabled() const
{
	return fAutoComplete;
}

void BTextListControl::SetEnabled(bool enabled)
{
	if (enabled != IsEnabled()) {
		if (!enabled) {
			HideChoiceWindow(false); // dismiss window if its up
		}
		BTextControl::SetEnabled(enabled);
		Invalidate();
	}
}

void BTextListControl::SetDivider(float divide)
{
	float diff = Divider() - divide;
	fButtonRect.OffsetBy(-diff, 0);
	BTextControl::SetDivider(divide + BPopUpMenuGlyph::kButtonWidth + kButtonRightMargin); // BTextControl will Invalidate()
}

void BTextListControl::FrameMoved(BPoint new_position)
{
	BTextControl::FrameMoved(new_position);
}

void BTextListControl::FrameResized(float new_width, float new_height)
{
	BTextControl::FrameResized(new_width, new_height);
}

void BTextListControl::WindowActivated(bool active)
{
	// causes a redraw so focus marks will update
	if (TextView()->IsFocus() || IsFocus()) {
		Draw(Bounds());
	}
	// hide the choice window if this window got deactivated
	if (!active) {
		HideChoiceWindow(false);
	}
}

void BTextListControl::Draw(BRect update)
{
	// have superclass draw
	BTextControl::Draw(update);

	if (fChoiceList == NULL) {
		return;
	}
	
	BPopUpMenuGlyph glyph;
	
	glyph.SetFrame(fButtonRect);
	glyph.SetPressed(fButtonDepressed);
	glyph.SetActive(false);
	glyph.SetFocused(TextView()->IsFocus() && Window()->IsActive());
	glyph.SetEnabled(IsEnabled());
	glyph.SetBackgroundColor(LowColor());
	if (glyph.IsFocused()) {
		int32 selStart, selEnd;
		TextView()->GetSelection(&selStart, &selEnd);
		if (selStart != selEnd)
			glyph.SetBorderColor(TextView()->NextFocusColor());
		else
			glyph.SetBorderColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	} else {
		glyph.SetBorderColor(HighColor());
	}
	glyph.SetFillColor(ui_color(B_MENU_SELECTED_BACKGROUND_COLOR));
	glyph.SetLabelColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	
	glyph.Draw(this);
}

void BTextListControl::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kTextInputModifyMessage:
		TryAutoComplete();
		if (fAutoPrune) {
			BuildMatchList();
			BMessage msg(kTextChangedMessage);
			fChoiceWindow->PostMessage(&msg, fChoiceWindow);
		}
		break;
	case kPopupButtonInvokeMessage:
		if (fChoiceWindow == NULL) {
			ShowChoiceWindow();
		} else {
			HideChoiceWindow(false);
		}
		break;
	case kWindowMovedMessage:
		HideChoiceWindow(false); // does nothing if the window is not showing
		break;
	default:
		BTextControl::MessageReceived(msg);
	}
}

void BTextListControl::KeyDown(const char *bytes, int32 numBytes)
{
	switch(bytes[0]) {
	case B_SPACE:
		{
			BMessage msg(kPopupButtonInvokeMessage);
			Window()->PostMessage(&msg, this);
			TextView()->MakeFocus(true);
		}
		break;
	default:
		BTextControl::KeyDown(bytes, numBytes);
		break;
	}
}


void BTextListControl::MouseDown(BPoint where)
{
	if (!IsEnabled() || fChoiceList == NULL) {
		return;
	}
	
	BRect sloppyButtonRect(fButtonRect);
	sloppyButtonRect.InsetBy(-2, -2);
	bool inval = false;
	
//	if (fChoiceWindow != NULL && fChoiceWindow->Lock()) {
//		if (!fChoiceWindow->IsHidden()) {
//			SetPopupWindowShowing(false);
//			fButtonDepressed = false;
//			inval = true;
//		}
//		fChoiceWindow->Unlock();
//	}
	if (sloppyButtonRect.Contains(where) && (fChoiceWindow == NULL)) {
		// clicked in button area, and popup is not showing
		fDepressedWhenClicked = fButtonDepressed;
		fButtonDepressed = !fButtonDepressed;
		fTrackingButtonDown = true;
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
//		TextView()->MakeFocus(true);
		inval = true;
		
	} else {
		// default BTextControl behaviour
		BTextControl::MouseDown(where);
		return;
	}

	if (inval) {
		Invalidate(sloppyButtonRect);
	}
}

void BTextListControl::MouseUp(BPoint where)
{
	if (fTrackingButtonDown) {
		// send a kPopupButtonInvokeMessage when the button changes state
		if (fButtonDepressed != fDepressedWhenClicked) {
			BMessage msg(kPopupButtonInvokeMessage);
			Window()->PostMessage(&msg, this);
		}
		fTrackingButtonDown = false;
	} else {
		// default BTextControl behaviour
		BTextControl::MouseUp(where);
		return;
	}
}

void BTextListControl::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fTrackingButtonDown) {
		BRect sloppyRect(fButtonRect);
		sloppyRect.InsetBy(-3, -3);
		bool oldState = fButtonDepressed;
		fButtonDepressed = (sloppyRect.Contains(where)) ? !fDepressedWhenClicked : fDepressedWhenClicked;
		if (oldState != fButtonDepressed) {
			BRect inval(fButtonRect);
			inval.InsetBy(-2, -2);
			Invalidate(inval);
		}
	} else {
		// default BTextControl behaviour
		BTextControl::MouseMoved(where, transit, dragMessage);
		return;
	}
}

void BTextListControl::AttachedToWindow()
{
	fButtonRect = TextView()->Frame();
	fButtonRect.InsetBy(-kTextInputMargin, -kTextInputMargin);
	fButtonRect.right = fButtonRect.left - (kButtonRightMargin + 1);
	fButtonRect.left = fButtonRect.right - (BPopUpMenuGlyph::kButtonWidth - 1);
	fButtonRect.InsetBy(0, 1);
	fButtonRect.bottom -= 1;

	BTextControl::AttachedToWindow();
}

void BTextListControl::DetachedFromWindow()
{
	BTextControl::DetachedFromWindow();
}

void BTextListControl::SetFlags(uint32 flags)
{
	BTextControl::SetFlags(flags);
	// BTextControl removes the B_NAVIGABLE flags from the flags,
	// so we have to put it back in here if it is set.
	if ((flags & B_NAVIGABLE) != 0) {
		BControl::SetFlags(flags);
	}
}

void BTextListControl::SetFont(const BFont *font, uint32 properties)
{
	// XXX: should set font of popup window, and change it if it's visible?
	BTextControl::SetFont(font, properties);
}

void BTextListControl::GetPreferredSize(float *width, float *height)
{
	BTextControl::GetPreferredSize(width, height);
	*width += BPopUpMenuGlyph::kButtonWidth + 4;
}


void BTextListControl::MakeFocus(bool focusState)
{
	if (focusState == IsFocus()) {
		return;
	}

	if (fChoiceList == NULL) {
		BTextControl::MakeFocus(focusState);	
	} else {
		// BTextControl focuses the text view in MakeFocus(), so we have
		// to override it because we want focus for the button
		if (focusState && !IsEnabled() && IsNavigating()) {
			// if this control is disabled and the user is trying to
			// tab-navigate into it, don't take focus.
			return;
		}
		BControl::MakeFocus(focusState);
		Invalidate(fButtonRect);
	}
}


BPrivate::TextListWindow *BTextListControl::CreateChoiceWindow()
{
	TextListWindow *win = new TextListWindow(this, fMaxVisible, fAutoPrune);
	return win;
}

void BTextListControl::TryAutoComplete()
{
	if (fChoiceList == NULL) {
		return;
	}
	BTextView *text = TextView();
	const char *textPtr = text->Text();
	int32 from, to;
	text->GetSelection(&from, &to);

	if (from == to) {
		fUserText.SetTo(textPtr);
	}

	if (fAutoComplete && (from == to)) {
		bool autoCompleted = false;
		if ((to > fTextEnd) && (from == text->TextLength())) {
			BString completion;
			int32 key;
			// find the first matching choice and do auto-completion with it
			if (fChoiceList->Lock() && fChoiceList->FindMatch(textPtr, 0, &fCompletionIndex, &completion, &key) == B_OK) {
				text->Insert(completion.String());
				text->Select(to, to + completion.Length());
				SelectByKey(key);
				autoCompleted = true;
				fChoiceList->Unlock();
			} else {
				fCompletionIndex = -1;
			}
		}
		fTextEnd = to;

		if (!autoCompleted) {
			int32 sel = CurrentSelection();
			if (sel >= 0) {
				const char *selText(NULL);
				if (fChoiceList->Lock()) {
					selText = fChoiceList->GetChoiceByKey(sel, NULL);
					fChoiceList->Unlock();
				}
				if (selText != NULL && !strncmp(textPtr, selText, strlen(textPtr))) {
					// don't deselect if the text input matches the selection
					return;
				}
			}
			fCompletionIndex = -1;
			SelectByIndex(fCompletionIndex); // deselect
		}
	}
}

void BTextListControl::BuildMatchList()
{
	if (fMatchList == NULL || fChoiceList == NULL) {
		return;
	}
	
	// empty the list
	BString *str;
	while ((str = fMatchList->RemoveItemAt(0)) != NULL) {
		delete str;
	}

	// build prefix
//	BTextView *textView = TextView();
//	BString prefix(textView->Text());
//	int32 startSel, endSel;
//	textView->GetSelection(&startSel, &endSel);
//	if (startSel != endSel) {
//		prefix.Remove(startSel, endSel - startSel);
//	}
	BString prefix(fUserText.String());

	int32 matchIndex;
	int32 startIndex = 0;
	int32 index = 0;
	int32 key;
	BString choice;
	
	if (fChoiceList->Lock()) {
		// fill the list with matches
		while (fChoiceList->FindMatch(prefix.String(), startIndex, &matchIndex, &choice, &key) == B_OK) {
			choice.Prepend(prefix);
			fMatchList->AddItem(new KeyedString(key, choice.String()));
			if (key == fSelected) { // translate the selection index
				fSelectedIndex = index;
			}
			index++;
			startIndex = matchIndex + 1;
		}
		fChoiceList->Unlock();
	}
}

int32 BTextListControl::MatchListIndexForKey(int32 key)
{
	int32 index = -1;
	if (fMatchList != NULL) {
		int32 count = fMatchList->CountItems();
		KeyedString *str;
		for (int32 i = 0; i < count; i++) {
			str = fMatchList->ItemAt(i);
			if (key == str->key) {
				index = i;
				break;
			}
		}
	}
	return index;
}


bool BTextListControl::ShowChoiceWindow()
{
	if ((fChoiceWindow != NULL) || (fChoiceList == NULL)) {
		return false;
	}
		// build prefix
	BTextView *textView = TextView();
//	fUserText.SetTo(textView->Text());
//	int32 startSel, endSel;
//	textView->GetSelection(&startSel, &endSel);
//	if (startSel != endSel) {
//		fUserText.Remove(startSel, endSel - startSel);
//	}
	
	textView->MakeFocus(true);
	fChoiceWindow = CreateChoiceWindow();
	fChoiceWindow->Show();

	fButtonDepressed = true;
	BRect inval(fButtonRect);
	inval.InsetBy(-2, -2);
	Invalidate(inval);
	return true;
}

bool BTextListControl::HideChoiceWindow(bool invoked)
{
	if (fChoiceWindow == NULL) {
		return false;
	}

	fButtonDepressed = false;
	BRect inval(fButtonRect);
	inval.InsetBy(-2, -2);
	Invalidate(inval);

	fChoiceWindow->PostMessage(B_QUIT_REQUESTED);
	fChoiceWindow = NULL;
	
	BTextView *textView = TextView();
	if (!invoked) {
		textView->SetText(fUserText.String());
		int32 len = textView->TextLength();
		textView->Select(len, len);
	} else {
		fUserText.SetTo(textView->Text());
	}
	return true;
}

// currently, ResolveSpecifier is a trivial implementation that just calls
// BTextControl::ResolveSpecifier()
BHandler *BTextListControl::ResolveSpecifier(	BMessage *msg,
												int32 index,
												BMessage *specifier,
												int32 form,
												const char *property)
{
	return BTextControl::ResolveSpecifier(msg, index, specifier, form, property);
}

// currently, GetSupportedSuites is a trivial implementation that just calls
// BTextControl::GetSupportedSuites()
status_t BTextListControl::GetSupportedSuites(BMessage *data)
{
	return BTextControl::GetSupportedSuites(data);
}


status_t BTextListControl::Perform(perform_code /*d*/, void */*arg*/)
{
	return B_ERROR;
}


void BTextListControl::_ReservedTextListControl1() { }
void BTextListControl::_ReservedTextListControl2() { }
void BTextListControl::_ReservedTextListControl3() { }
void BTextListControl::_ReservedTextListControl4() { }
void BTextListControl::_ReservedTextListControl5() { }
void BTextListControl::_ReservedTextListControl6() { }
void BTextListControl::_ReservedTextListControl7() { }
void BTextListControl::_ReservedTextListControl8() { }
void BTextListControl::_ReservedTextListControl9() { }
void BTextListControl::_ReservedTextListControl10() { }
void BTextListControl::_ReservedTextListControl11() { }
void BTextListControl::_ReservedTextListControl12() { }

// -------------------------------------------------------------------------------

TextListTextInput::TextListTextInput(BRect rect, BRect trect, ulong rMask, ulong flags)
	: _BTextInput_(rect, trect, rMask, flags)
{
	// do nothing
}


TextListTextInput::TextListTextInput(BMessage *data)
	: _BTextInput_(data)
{
	// do nothing
}


TextListTextInput::~TextListTextInput()
{
	// do nothing
}

BArchivable *TextListTextInput::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "TextListTextInput"))
		return NULL;
	return new TextListTextInput(data);
}

status_t TextListTextInput::Archive(BMessage *data, bool deep) const
{
	return _BTextInput_::Archive(data, deep);
}

void TextListTextInput::KeyDown(const char *bytes, int32 numBytes)
{
	BTextListControl		*cb(NULL);
	
	switch (bytes[0]) {
		case B_RETURN:
			cb = dynamic_cast<BTextListControl *>(Parent());

			if (!cb->IsEnabled())
				break;

			ASSERT(fInitialText);
			if (strcmp(fInitialText, Text()) != 0) {
				cb->Invoke();
			}
			free(fInitialText);
			fInitialText = strdup(Text());
			SelectAll();

			// hide popup window if it's showing when the user presses the enter key
			if (cb->fChoiceWindow != NULL) {
				cb->HideChoiceWindow(true);
			}
			break;
//		case B_TAB: // cycle to the next autocomplete... this code is very outdated
//			cb = dynamic_cast<BTextListControl *>(Parent());
//			if (cb->fAutoComplete && cb->fCompletionIndex >= 0) {	
//				int32 from, to;
//				GetSelection(&from, &to);
//				if (from == to) {
//					// HACK: this should never happen.  The rest of the class should be
//					// fixed so that fCompletionIndex is set to -1 if the text is modified
//					printf("BTextListControl::TextInput::KeyDown() -- HACK! this shouldn't happen!");
//					cb->fCompletionIndex = -1;
//				}
//				
//				const char *text = Text();
//				BString prefix;
//				prefix.Append(text, from);
//				
//				int32 match;
//				BString completion;
//				if (cb->fChoiceList->GetMatch(	prefix.String(),
//											  	cb->fCompletionIndex + 1,
//												&match,
//												&completion) == B_OK)
//				{
//					Delete(); 			// delete the selection
//					Insert(completion.String());
//					Select(from, from + completion.CountChars());
//					cb->fCompletionIndex = match;
//					cb->Select(cb->fCompletionIndex);
//				} else {
//					//system_beep();
//				}	
//			} else {
//				BView::KeyDown(bytes, numBytes);
//			}
//			break;
		case B_UP_ARROW:		// fall through
		case B_DOWN_ARROW:
			{
				// show popup window (does nothing if already showing)
				cb = dynamic_cast<BTextListControl *>(Parent());
				if (cb->fChoiceList == NULL) {
					break;
				}

				bool shown = cb->ShowChoiceWindow();

				int32 oldIndex = cb->fSelectedIndex;
				int32 index = oldIndex;
				int32 choices = 0;
				bool changeText = false;

				if (cb->fChoiceList->Lock()) {
					choices = (cb->fAutoPrune) ? cb->fMatchList->CountItems()
											   : cb->fChoiceList->CountChoices();
					cb->fChoiceList->Unlock();
				}
				
				if ((choices > 0) && !shown) {
					if (index < 0) {
						// no current selection, so select first or last item
						index = (bytes[0] == B_UP_ARROW) ? choices - 1 : 0;
					} else {
						// select the previous or the next item, if possible,
						// depending on whether this is an up or down arrow
						if ((bytes[0] == B_UP_ARROW) && ((index - 1) >= 0)) {
							index--;
							changeText = true;
						} else if ((bytes[0] == B_DOWN_ARROW) && (index + 1 < choices)) {
							index++;
							changeText = true;
						}
					}
				}
				
				if (index != oldIndex) {
					cb->SelectByIndex(index, changeText);
				}
			}
			break;
		case B_ESCAPE:
			{
				cb = dynamic_cast<BTextListControl *>(Parent());
				if (cb != NULL) {
					cb->HideChoiceWindow(false);
				}				
			}
			break;
		default:
			_BTextInput_::KeyDown(bytes, numBytes);
			break;
	}
}

void TextListTextInput::MakeFocus(bool state)
{
	bool focus = IsFocus();
	_BTextInput_::MakeFocus(state);
	if ((state != focus) && !state) {
		// hide popup window if it's showing when the text input loses focus
		BTextListControl *parent = dynamic_cast<BTextListControl *>(Parent());
		if (parent != NULL) {
			parent->HideChoiceWindow(false);
		}
	}
}

void TextListTextInput::InsertText(const char *inText, int32 inLength, int32 inOffset, const text_run_array *inRuns)
{
	_BTextInput_::InsertText(inText, inLength, inOffset, inRuns);
	BMessage msg(kTextInputModifyMessage);
	Window()->PostMessage(&msg, Parent());
}

void TextListTextInput::DeleteText(int32 fromOffset, int32 toOffset)
{
	_BTextInput_::DeleteText(fromOffset, toOffset);
	BMessage msg(kTextInputModifyMessage);
	Window()->PostMessage(&msg, Parent());
}

// -------------------------------------------------------------------------------

const float kWinLeftOffset		= 7.0f;
const float kWinTopOffset		= 7.0f;
const float kWinRightOffset		= 5.0f;
const float kWinBorderThickness = 5.0f;

namespace BPrivate {

class CBWinDecorView : public BView
{
public:
				CBWinDecorView(BRect frame, BRect hole, rgb_color background, rgb_color view);
	virtual 	~CBWinDecorView();

protected:

	virtual void	Draw(BRect rect);

	BRect 		fHole;
	rgb_color	fBackgroundCol; // the color of the text background
	rgb_color	fViewCol;
};

CBWinDecorView::CBWinDecorView(BRect frame, BRect hole, rgb_color background, rgb_color view)
	: BView(frame, "_popup_win_decor_", B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	fHole = hole;
	fBackgroundCol = background;
	fViewCol = view;
	SetViewColor(fViewCol);
	SetLowColor(fViewCol);
}


CBWinDecorView::~CBWinDecorView()
{
}

void CBWinDecorView::Draw(BRect /*rect*/)
{
	BRect bounds(Bounds());
	rgb_color lineCol = {0, 0, 0, 255};
	rgb_color whiteCol = { 255, 255, 255, 255};
	rgb_color shade1Col = tint_color(fBackgroundCol, B_DARKEN_1_TINT);
	rgb_color shade2Col = tint_color(fBackgroundCol, B_DARKEN_2_TINT);

	// draw outer border
	SetHighColor(lineCol);
	StrokeRoundRect(bounds, 3, 3);

	// draw inner list border.
	BButtonBorderGlyph glyph;
	
	glyph.SetFrame(BRect(kWinBorderThickness-1, kWinBorderThickness-1,
						 bounds.right - B_V_SCROLL_BAR_WIDTH-kWinBorderThickness+2,
						 bounds.bottom - (kWinBorderThickness-1)));
	glyph.SetBackgroundColor(fViewCol);
	glyph.SetBorderColor(lineCol);
	glyph.SetFillColor(whiteCol);
	glyph.SetLabelColor(lineCol);
	glyph.SetPressed(true);
	glyph.SetActive(false);
	glyph.SetFocused(true);
	glyph.SetEnabled(true);
	
	glyph.Draw(this);
	
	BeginLineArray(5);
	// draw white highlights on left and top outer borders
	rgb_color shineCol = fViewCol.blend_copy(whiteCol, 182);
	AddLine(BPoint(1, 2), BPoint(1, bounds.bottom - 2), shineCol);
	AddLine(BPoint(2, 1), BPoint(bounds.right - 2, 1), shineCol);
	
	// draw lines dividing choice list from text input
	AddLine(BPoint(fHole.left, fHole.bottom + 1),
			BPoint(fHole.right, fHole.bottom + 1), fBackgroundCol);
	AddLine(BPoint(fHole.left, fHole.bottom + 2),
			BPoint(fHole.right, fHole.bottom + 2), fBackgroundCol);
	AddLine(BPoint(fHole.left + 1, fHole.bottom + 1),
			BPoint(fHole.right - 1, fHole.bottom + 1), lineCol);
	EndLineArray();
}

} // namespace BPrivate


TextListWindow::TextListWindow(BTextListControl *box, int32 count, bool displayMatchesOnly)
	: BWindow(BRect(0, 0, 50, 50), NULL, B_NO_BORDER_WINDOW_LOOK,
		B_FLOATING_SUBSET_WINDOW_FEEL, B_NOT_MOVABLE | B_NOT_RESIZABLE
			| B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AVOID_FOCUS
			| B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS)
{
	fChoiceCount = count;
	fParent = box;
	fListView = new TextListView(BRect(0, 0, 50, 50), fParent); // must have list view before calling DoPosition()
	fListView->SetMatchesOnly(displayMatchesOnly);
	DoPosition();

	BRect bounds(Bounds());
	BRect rect(bounds);

	BWindow *parentWin = fParent->Window();
	if (parentWin) {
		AddToSubset(parentWin);
	}
	rgb_color backCol = fParent->ViewColor();
	if (backCol == B_TRANSPARENT_COLOR) {
		backCol = fParent->LowColor();
	}
	CBWinDecorView *decor = new CBWinDecorView(bounds, fHole, fParent->TextView()->LowColor(), backCol);
	AddChild(decor);

	rect.right -= (B_V_SCROLL_BAR_WIDTH + kWinBorderThickness - 0);
	rect.left += kWinLeftOffset;
	rect.top = fHole.bottom + 3;
	rect.bottom -= (kWinBorderThickness+1);
	fListView->MoveTo(rect.LeftTop());
	fListView->ResizeTo(rect.Width(), rect.Height());
	decor->AddChild(fListView);
	
	rect.top = bounds.top + kWinBorderThickness - 1;
	rect.left = rect.right + 2;
	rect.right += B_V_SCROLL_BAR_WIDTH + 2;
	rect.bottom += 2;
	fScrollBar = new BScrollBar(rect, "_popup_scroll_bar_", fListView, 0, 1000, B_VERTICAL);
	decor->AddChild(fScrollBar);
	
	fListView->ScrollToSelection();
	fListView->AdjustScrollBar();
}


TextListWindow::~TextListWindow()
{
}

void TextListWindow::WindowActivated(bool /*active*/)
{
//	if (active) {
//		fListView->AdjustScrollBar();
//	}
}

void TextListWindow::FrameResized(float /*width*/, float /*height*/)
{
	fListView->AdjustScrollBar();
}

void TextListWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case kSelectItemMessage:
		{
			int32 currSel = message->FindInt32("old_index");
			int32 newSel = message->FindInt32("index");
			
			if (newSel >= 0) {
				// new item was selected, so scroll to it and invalidate it
				fListView->ScrollToSelection();
				fListView->InvalidateItem(newSel);
			}
			if (currSel >= 0) {
				// an item was previously selected, so invalidate it
				fListView->InvalidateItem(currSel);
			}
		}
		break;
	case kSetDisplayedCountMessage:
		{
			int32 count = message->FindInt32("count");
			SetChoiceCount(count);
		}
		break;
	case kSetDisplayMatchesOnlyMessage:
		{
			bool matchesOnly = message->FindBool("matches_only");
			fListView->SetMatchesOnly(matchesOnly);
		}
		break;
	case kInvalidateListMessage: // fall through
	case kTextChangedMessage:
		{
			fListView->Invalidate();
			fListView->AdjustScrollBar();
		}
		break;
	default:
		BWindow::MessageReceived(message);
		break;
	}
}
void TextListWindow::Quit()
{
	BWindow::Quit();
}
	
void TextListWindow::SetChoiceCount(int32 count)
{
	if (fChoiceCount != count) {
		fChoiceCount = count;
		Hide();
		DoPosition();
		Show();
	}
}

void TextListWindow::DoPosition()
{
	fHole = fParent->TextView()->Frame();
	BRect winRect(fHole);
	winRect = fParent->ConvertToScreen(winRect);
	winRect.left -= kWinLeftOffset;
	winRect.top -= kWinTopOffset;
	winRect.right += B_V_SCROLL_BAR_WIDTH + kWinBorderThickness + 1;
	winRect.bottom = winRect.top + (fListView->LineHeight() * fChoiceCount) + (kWinBorderThickness * 2) + fHole.Height() + 4;
	MoveTo(winRect.LeftTop());
	ResizeTo(winRect.IntegerWidth(), winRect.IntegerHeight());
	
	fHole.OffsetTo(kWinLeftOffset, kWinTopOffset);

	BRect winBounds(Bounds());
	winBounds.OffsetTo(0, 0);
	BView *clipView = new BView(winBounds, NULL, B_FOLLOW_NONE, 0);
	AddChild(clipView);
	
	BPicture winClip;
	clipView->BeginPicture(&winClip);
	BRegion region(winBounds);
	region.Exclude(fHole);
	clipView->ConstrainClippingRegion(&region);
	clipView->FillRoundRect(winBounds, 3, 3);
	clipView->EndPicture();
	ClipWindowToPicture(&winClip, B_ORIGIN, 0);

	clipView->RemoveSelf();
	delete clipView;
}

TextListView *TextListWindow::ListView()
{
	return fListView;
}

BScrollBar *TextListWindow::ScrollBar()
{
	return fScrollBar;
}

// ----------------------------------------------------------------------------

TextListView::TextListView(BRect frame, BTextListControl *parent)
	: BView(frame, "_text_list_view_", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	  fClickLoc(-100, -100)
{
	fParent = parent;

	BFont font;
	parent->GetFont(&font);
	SetFont(&font);
	font.GetHeight(&fFontHeight);

	fHighCol = ui_color(B_UI_CONTROL_TEXT_COLOR);
	fSelLowCol = ui_color(B_UI_CONTROL_HIGHLIGHT_COLOR);
	fSelHighCol = ui_color(B_UI_CONTROL_TEXT_COLOR);
	fLowCol = ui_color(B_UI_CONTROL_BACKGROUND_COLOR);
	
	SetViewColor(B_TRANSPARENT_COLOR);
	SetHighColor(fHighCol);
	fTrackingMouseDown = false;
	fSelIndex = parent->fSelectedIndex;
	fClickTime = 0;
	fMatchesOnly = false;
}

TextListView::~TextListView()
{
}

void TextListView::Draw(BRect update)
{
	float h = LineHeight();
	BRect rect(Bounds());
	rect.top = rect.bottom = -1;	// to ensure that the whole view gets drawn
									// when there are no visible items
	int32 index;
	int32 selected = (fTrackingMouseDown) ? fSelIndex : fParent->fSelectedIndex;
	int32 choices = 0;
	const char *choice;
	
	if (fParent->fChoiceList->Lock()) {
		choices = (fMatchesOnly) ? fParent->fMatchList->CountItems()
								: fParent->fChoiceList->CountChoices();
	
		// draw each visible item
		for (index = (int32)floor(update.top / h); index < choices; index++) {
			rect.top = (index * h)/* + 2*/;
			rect.bottom = rect.top + h - 1;
			BRegion clipRegion(rect);
			ConstrainClippingRegion(&clipRegion);
	
			if (index == selected) {
				SetHighColor(fSelHighCol);
				SetLowColor(fSelLowCol);		
				PushState();
	//			FillRect(rect, B_SOLID_HIGH);
				FillRect(rect, B_SOLID_LOW);
	//			FillRoundRect(rect, 2, 2, B_SOLID_LOW);
				PopState();
			} else {
				SetHighColor(fHighCol);
				SetLowColor(fLowCol);		
				FillRect(rect, B_SOLID_LOW);
			}
			if (fMatchesOnly) {
				choice = fParent->fMatchList->ItemAt(index)->String();
			} else {
				choice = fParent->fChoiceList->GetChoiceAt(index, NULL);
			}
			DrawString(choice, BPoint(rect.left + 1, rect.bottom - fFontHeight.descent - 1));
			ConstrainClippingRegion(NULL);
		}

		fParent->fChoiceList->Unlock();
	}

	// draw empty area on bottom
	if (rect.bottom < update.bottom) {
		update.top = rect.bottom;
		SetLowColor(fLowCol);
		FillRect(update, B_SOLID_LOW);
	}
}

void TextListView::MouseDown(BPoint where)
{
	BRect rect(Window()->Frame());
	ConvertFromScreen(&rect);
	if (!rect.Contains(where)) {
		// hide the popup window when the user clicks outside of it
		if (fParent->Window()->Lock()) {
			fParent->HideChoiceWindow(false);
			fParent->Window()->Unlock();
		}		
		return;
	}

	fClickedInWindow = true;
	
	rect = Bounds();
	if (!rect.Contains(where)) {
		return;
	}

/* checking for double click is obsolete, since control now Invoke()s on single click */
//	// check for double click
//	bigtime_t now = system_time();
//	bigtime_t clickSpeed;
//	get_click_speed(&clickSpeed);
//	if ((now - fClickTime < clickSpeed)
//		&& ((abs((int)(fClickLoc.x - where.x)) < 3) && (abs((int)(fClickLoc.y - where.y)) < 3)))
//	{
//		// this is a double click
////		printf("TextListView::MouseDown() -- double click occurred\n");
//
//		if (fParent->Window()->Lock()) {
//			// cause an BTextListControl::Invoke() to happen
//			fParent->Invoke();
//			// hide popup window
//			fParent->HideChoiceWindow(true);
//			fParent->Window()->Unlock();
//		}
//		return;
//	}
//	fClickTime = now;

	fTrackingMouseDown = true;
	fClickLoc = where;

	float h = LineHeight();
	int32 oldIndex = fSelIndex;
	fSelIndex = (int32)floor(where.y / h);
	int32 choices = 0;

	if (fParent->fChoiceList->Lock()) {
		choices = (fMatchesOnly) ? fParent->fMatchList->CountItems()
								 : fParent->fChoiceList->CountChoices();
		fParent->fChoiceList->Unlock();
	}
	
	if (fSelIndex < 0 || fSelIndex >= choices) {
		fSelIndex = -1;
	}

	if (oldIndex != fSelIndex) {
		InvalidateItem(oldIndex);
		InvalidateItem(fSelIndex);
	}
}

void TextListView::MouseUp(BPoint /*where*/)
{
	if (fTrackingMouseDown) {
		fTrackingMouseDown = false;
		bool winLocked = false;
		BWindow *parentWin = fParent->Window();
		if (parentWin != NULL) {
			winLocked = parentWin->Lock();
		}
		
		if (fSelIndex >= 0) {
			fParent->SelectByIndex(fSelIndex, true);
			fParent->Invoke();
			fParent->HideChoiceWindow(true);
		} else {
			fParent->SelectByIndex(-1);
		}
		
		if (winLocked) {
			parentWin->Unlock();
		}
	}
	if (fClickedInWindow) {
		fClickedInWindow = false;
		fParent->Window()->Activate(true);
	}
//	fClickLoc = where;
}

void TextListView::MouseMoved(BPoint where, uint32 /*transit*/,
								const BMessage* /*dragMessage*/)
{
	// we only care about the mouse position when we're tracking
	// the mouse, and we have a valid choice list 
	if (fTrackingMouseDown && (fParent->fChoiceList != NULL)) {
		BRect bounds(Bounds());
		if (bounds.Contains(where)) {
			float h = LineHeight();
			int32 oldIndex = fSelIndex;
			fSelIndex = (int32)floor(where.y / h);
			int32 choices = 0;
			
			if (fParent->fChoiceList->Lock()) {
				choices = (fMatchesOnly) ? fParent->fMatchList->CountItems()
										 : fParent->fChoiceList->CountChoices();
				fParent->fChoiceList->Unlock();
			}
	
			if (fSelIndex < 0 || fSelIndex >= choices) {
				fSelIndex = -1;
			}
			if (oldIndex != fSelIndex) {
				InvalidateItem(oldIndex);
				InvalidateItem(fSelIndex);
			}
		}
	}
}

void TextListView::AttachedToWindow()
{
	SetEventMask(B_POINTER_EVENTS, 0);
}

void TextListView::DetachedFromWindow()
{
	SetEventMask(0, 0);
}


void TextListView::SetFont(const BFont *font, uint32 properties)
{
	BView::SetFont(font, properties);
	GetFontHeight(&fFontHeight);
	Invalidate();
}

void TextListView::ScrollToSelection()
{
	int32 selected = fParent->CurrentSelection();
	if (selected >= 0) {
		BRect frame(ItemFrame(selected));
		BRect bounds(Bounds());
		float newY(0.0f);
        bool doScroll = false;

		if (frame.bottom > bounds.bottom) {
			newY = frame.bottom - bounds.Height() - 1;
			doScroll = true;
		} else if (frame.top < bounds.top) {
			newY = frame.top;
			doScroll = true;
		}
		if (doScroll) {
			ScrollTo(bounds.left, newY);
		}
	}
}

// InvalidateItem() only does a real invalidate if the index is valid or the
// force flag is turned on
void TextListView::InvalidateItem(int32 index, bool force)
{
	int32 choices = 0;
	if (fParent->fChoiceList->Lock()) {
		choices = (fMatchesOnly) ? fParent->fMatchList->CountItems()
								 : fParent->fChoiceList->CountChoices();
		fParent->fChoiceList->Unlock();
	}
	if ((index >= 0 && index < choices) || force) {
		Invalidate(ItemFrame(index));
	}
}

// This method doesn't check the index to see if it is valid, it just returns the
// BRect that an item and the index would have if it existed.
BRect TextListView::ItemFrame(int32 index)
{
	BRect rect(Bounds());
	float h = LineHeight();
	rect.top = index * h;
	rect.bottom = rect.top + h;
	return rect;
}

// The window must be locked before this method is called
void TextListView::AdjustScrollBar()
{
	BScrollBar *sb = ScrollBar(B_VERTICAL);
	if (sb) {
		int32 choices = 0;
		if (fParent->fChoiceList->Lock()) {
			choices = (fMatchesOnly) ? fParent->fMatchList->CountItems()
									 : fParent->fChoiceList->CountChoices();
			fParent->fChoiceList->Unlock();
		}
		float h = LineHeight();
		float max = h * choices;
		BRect frame(Frame());
		float diff = max - frame.Height();
		float prop = frame.Height() / max;
		if (diff < 0) {
			diff = 0.0;
			prop = 1.0;
		}
		sb->SetSteps(h, h * (frame.IntegerHeight() / h));
		sb->SetRange(0.0, diff);
		sb->SetProportion(prop);
	}	
}

float TextListView::LineHeight()
{
	return ceil(fFontHeight.ascent + fFontHeight.descent + fFontHeight.leading + 2);
}

void TextListView::SetMatchesOnly(bool matchesOnly)
{
	if (fMatchesOnly != matchesOnly) {
		fMatchesOnly = matchesOnly;
		Invalidate();
	}
}
	
// ----------------------------------------------------------------------------


BChoiceList::BChoiceList()
	: fOwner(NULL)
{
}

BChoiceList::BChoiceList(BMessage */*archive*/)
	: fOwner(NULL)
{
}

BChoiceList::~BChoiceList()
{
}

status_t BChoiceList::Archive(BMessage *archive, bool /*deep*/) const
{
	archive->AddString(B_CLASS_NAME_ENTRY, "BChoiceList");
	return B_OK;
}

void BChoiceList::SetOwner(BTextListControl *owner)
{
	fOwner = owner;
}

BTextListControl *BChoiceList::Owner()
{
	return fOwner;
}

bool BChoiceList::Lock()
{
	return fLock.Lock();
}

void BChoiceList::Unlock()
{
	fLock.Unlock();
}


void BChoiceList::_ReservedChoiceList1() { }
void BChoiceList::_ReservedChoiceList2() { }
void BChoiceList::_ReservedChoiceList3() { }
void BChoiceList::_ReservedChoiceList4() { }
void BChoiceList::_ReservedChoiceList5() { }
void BChoiceList::_ReservedChoiceList6() { }
void BChoiceList::_ReservedChoiceList7() { }
void BChoiceList::_ReservedChoiceList8() { }


// ----------------------------------------------------------------------------

BStringChoiceList::BStringChoiceList()
{
	fList = new KeyedStringObjectList();
	fAutoKey = 0;
}


BStringChoiceList::BStringChoiceList(BMessage */*archive*/)
{
	// XXX: not yet implemented
	TRESPASS();
}

status_t BStringChoiceList::Archive(BMessage */*archive*/, bool /*deep*/) const
{
	// XXX: not yet implemented
	TRESPASS();
	return B_ERROR;
}

BArchivable *BStringChoiceList::Instantiate(BMessage */*data*/)
{
	// XXX: not yet implemented
	TRESPASS();
	return NULL;
}

BStringChoiceList::~BStringChoiceList()
{
	KeyedString *str;
	while ((str = fList->RemoveItemAt(0)) != NULL) {
		delete str;
	}
	delete fList;
}

const char* BStringChoiceList::GetChoiceAt(int32 index, int32 *key) const
{
	KeyedString *str = fList->ItemAt(index);
	if (str != NULL) {
		if (key != NULL) {
			*key = str->key;
		}
		return str->String();
	}
	return NULL;
}

const char* BStringChoiceList::GetChoiceByKey(int32 key, int32 *index) const
{
	KeyedString *str;
	int32 choices = fList->CountItems();
	for (int32 i = 0; i < choices; i++) {
		str = fList->ItemAt(i);
		if (str->key == key) {
			if (index != NULL) {
				*index = i;
			}
			return str->String();
		}
	}
	return NULL;
}


status_t BStringChoiceList::FindMatch(	const char *prefix,
										int32 startIndex,
										int32 *matchIndex,
										BString *completionText,
										int32 *matchKey) const
{
	KeyedString *str;
	int32 len = strlen(prefix);
	int32 choices = fList->CountItems();
	for (int32 i = startIndex; i < choices; i++) {
		str = fList->ItemAt(i);
		if (!str->Compare(prefix, len)) {
			// prefix matches
			*matchIndex = i;
			completionText->SetTo(str->String() + len);
			if (matchKey != NULL) {
				*matchKey = str->key;
			}
			return B_OK;
		}
	}
	*matchIndex = -1;
	completionText->SetTo(B_EMPTY_STRING);
	return B_ERROR;
}


int32 BStringChoiceList::CountChoices() const
{
	return fList->CountItems();
}

status_t BStringChoiceList::AddChoice(const char *string, int32 key)
{
	if (key < 0) {
		key = fAutoKey++;
	}
	KeyedString *str = new KeyedString(key, string);
	bool r = fList->AddItem(str);
	if (Owner()) {
		Owner()->ChoiceListChanged();
	}
	return (r) ? B_OK : B_ERROR;
}

status_t BStringChoiceList::AddChoice(const char *string, int32 key, int32 index)
{
	if (key < 0) {
		key = fAutoKey++;
	}
	KeyedString *str = new KeyedString(key, string);
	bool r = fList->AddItem(str, index);
	if (Owner()) {
		Owner()->ChoiceListChanged();
	}
	return (r) ? B_OK : B_ERROR;
}

//int BStringCompareFunction(const BString *s1, const BString *s2)
//{
//	return s1->Compare(s2);
//}

status_t BStringChoiceList::RemoveChoice(const char *string)
{
	BString *str;
	int32 choices = fList->CountItems();
	for (int32 i = 0; i < choices; i++) {
		str = fList->ItemAt(i);
		if (!str->Compare(string)) {
			fList->RemoveItemAt(i);
			if (Owner() != NULL) {
				Owner()->ChoiceListChanged();
			}
			return B_OK;
		}		
	}
	return B_ERROR;		
}

status_t BStringChoiceList::RemoveChoiceAt(int32 index)
{
	KeyedString *str = fList->RemoveItemAt(index);
	if (str) {
		delete str;
		if (Owner() != NULL) {
			Owner()->ChoiceListChanged();
		}
		return B_OK;
	} else {
		return B_ERROR;
	}
}

status_t BStringChoiceList::RemoveChoiceByKey(int32 key)
{
	KeyedString *str;
	int32 choices = fList->CountItems();
	for (int32 i = 0; i < choices; i++) {
		str = fList->ItemAt(i);
		if (key == str->key) {
			fList->RemoveItemAt(i);
			if (Owner() != NULL) {
				Owner()->ChoiceListChanged();
			}
			return B_OK;
		}		
	}
	return B_ERROR;		
}

void BStringChoiceList::_ReservedStringChoiceList1() { }
void BStringChoiceList::_ReservedStringChoiceList2() { }
void BStringChoiceList::_ReservedStringChoiceList3() { }
void BStringChoiceList::_ReservedStringChoiceList4() { }
void BStringChoiceList::_ReservedStringChoiceList5() { }
void BStringChoiceList::_ReservedStringChoiceList6() { }


// ----------------------------------------------------------------------------



