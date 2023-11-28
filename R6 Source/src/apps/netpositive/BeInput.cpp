// ===========================================================================
//	BeInput.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "BeInput.h"
#include "HTMLView.h"
#include "BeDrawPort.h"
#include "ImageGlyph.h"
#include "URLView.h"
#include "Form.h"
#include "Builder.h"
#include "HTMLTags.h"
#include "HTMLDoc.h"

#include <Window.h>
#include <RadioButton.h>
#include <Button.h>
#include <CheckBox.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <ListView.h>
#include <malloc.h>

extern const char *kSubmitButtonTitle;
extern const char *kResetButtonTitle;
extern const char *kGenericButtonTitle;

HTMLView*
FindHTMLView(BView *view)
{
	if (view != NULL) {
		for (BView *parent = view->Parent(); parent != NULL; parent = parent->Parent()) {
			HTMLView *theHTMLView = dynamic_cast<HTMLView *>(parent);
			if (theHTMLView != NULL)
				return (theHTMLView);
		}
	}

	return (NULL);
}

// ===========================================================================

BeInputImage::BeInputImage(ConnectionManager *mgr, bool forceCache, BMessenger *listener, Document *htmlDoc) : 
	InputImage(mgr, forceCache, listener, htmlDoc), mView(0), mSentMessage(false)
{
}

void BeInputImage::Reset(DrawPort *)
{
	mSentMessage = false;
}

void BeInputImage::Layout(DrawPort *drawPort)
{
	mView = ((BeDrawPort *)drawPort)->GetView();	// Get ready to show it for the first time
	InputImage::Layout(drawPort);
}

bool BeInputImage::Clicked(float h, float v)
{
	if (mSentMessage)
		return false;
		
	bool clicked = mImage->Clicked(h,v);
	if (clicked && mView) {
		Click();
	}
	return clicked;
}

void BeInputImage::Click()
{
	BMessage msg(FORM_MSG + AV_SUBMIT);	// Should really just submit to the form...
	msg.AddPointer("InputGlyph",this);
	BWindow *w = mView->Window();
	if (w)
		w->PostMessage(&msg,mView);
	mSentMessage = true;
}

// ===========================================================================

MonoText::MonoText(
	BRect		rect, 
	char 		*name, 
	BRect 		textRect, 
	BeInputText	*inputText,
	bool		renderAsBullets) 
		: BTextView(rect, name, textRect, B_FOLLOW_LEFT | B_FOLLOW_TOP,
					B_WILL_DRAW | B_NAVIGABLE), mInputText(inputText), mRenderAsBullets(renderAsBullets)
{
//	SetWordWrap(false);
//	MakeResizable(true);
	SetDoesUndo(true); 
	mFrameTop = mFrameLeft = -1;
}

MonoText::~MonoText()
{
	if (mInputText)
		mInputText->ForgetTextView();
//	if (mInputText)
//		mInputText++;
}

void MonoText::AttachedToWindow()
{
	BTextView::AttachedToWindow();
	
	Style	theStyle = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
	FindHTMLView(this)->SetSubviewFont(theStyle, this);
}

void MonoText::DetachedFromWindow()
{
	// There's a bug in BScrollbar that if you delete the scroll bar
	// after the view has been deleted, it will try to call UnsetScroller
	// on the no-longer-existent target view.  Let's proactively kill
	// the scroll bars before that can happen.
	BScrollBar *scroller;
	scroller = ScrollBar(B_HORIZONTAL);
	if (scroller) {
		scroller->RemoveSelf();
		delete scroller;
	}
	scroller = ScrollBar(B_VERTICAL);
	if (scroller) {
		scroller->RemoveSelf();
		delete scroller;
	}
	BTextView::DetachedFromWindow();
}

void 
MonoText::MessageReceived(BMessage *msg)
{
	switch(msg->what){
		case CLEAR:
			Clear();
			break;
		default:
			BTextView::MessageReceived(msg);
	}
}

void MonoText::KeyDown(const char *bytes, int32 numBytes)
{
	switch (*bytes) {
		case B_TAB:
			// one-liners don't accept tabs (a la BTextControl)
			if (!DoesWordWrap()) {
				BView::KeyDown(bytes, numBytes);
				return;
			}
			break;

		case B_ENTER:
			if (((BeInputText*)mInputText)->SpecialKey(*bytes) == FALSE)
				return;	// Handle return if this is the only text field
	}
	BTextView::KeyDown(bytes,numBytes);
}

void MonoText::InsertText(const char *inText, int32 inLength, int32 inOffset, const text_run_array *inRuns)
{
	if (!mRenderAsBullets)
		BTextView::InsertText(inText, inLength, inOffset, inRuns);
	else {
		BString before;
		before.SetTo(mData, MIN(inOffset, (int32)mData.Length()));
		BString after;
		BString inTextString;
		inTextString.SetTo(inText, inLength);
		
		if (inOffset < (int32)mData.Length())
			after = mData.String() + inOffset;
			
		mData = before;
		mData += inTextString;
		mData += after;
				
		BString newText;
		for (int i = 0; i < inLength; i++)
			newText += "*";

		BTextView::InsertText(newText.String(), inLength, inOffset, inRuns);
	}
#ifdef JAVASCRIPT
	mInputText->ValueChanged();
#endif
}

void MonoText::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);
	if (mRenderAsBullets) {
		BString before;
		before.SetTo(mData, MIN(fromOffset, (int32)mData.Length()));
		BString after;
		if (toOffset < (int32)mData.Length())
			after = mData.String() + toOffset;

		mData = before;
		mData += after;
	}
#ifdef JAVASCRIPT
	mInputText->ValueChanged();
#endif
}

void MonoText::Draw(BRect updateRect)
{
	BTextView::Draw(updateRect);
}

void MonoText::Position()
{
	// Since calling MoveTo on a BView is an extremely expensive
	// operation, we want to avoid calling it unnecessarily at all costs.
	// Logically, we would like to move this view when its parent InputGlyph
	// subclass gets a SetTop or SetLeft, but this happens too many times,
	// so we will just set special members in this view and not call the MoveTo
	// until this function is called just prior to drawing.
	BRect parentBounds = Parent()->Bounds();
	float parentLeft = parentBounds.left;
	float parentTop = parentBounds.top;

	if (mFrameLeft - parentLeft != 0 || mFrameTop - parentTop != 0) {
		mFrameLeft = mViewLeft - parentLeft;
		mFrameTop = mViewTop - parentTop;
		if (mViewLeft >= 0 && mViewTop >= 0)
			MoveTo(mFrameLeft, mFrameTop);
	}
}


void
MonoText::MakeFocus(
	bool	focus)
{
	BTextView::MakeFocus(focus);
	HTMLView *view = dynamic_cast<HTMLView *>(Parent());
	if (view && focus) {
		view->MakeVisible(mViewLeft, mViewTop);
//		view->ScrollToH(mViewLeft);
//		view->ScrollToV(mViewTop);
	}
}

void
MonoText::Paste(
	BClipboard	*clipboard)
{
	BTextView::Paste(clipboard);

	Invalidate();
}

const char *MonoText::ActualText()
{
	if (mRenderAsBullets)
		return mData.String();
	else
		return Text();
}

// ===========================================================================

#define kMarginTop 2
#define kMarginBottom 4
#define kMarginLeft 4
#define kMarginRight 4

BeInputText::BeInputText(bool isPassword, Document *htmlDoc) : InputGlyph(htmlDoc), mFinal(false), mHScrollBar(0), mVScrollBar(0)
{
	if (isPassword)
		mType = AV_PASSWORD;
	else
		mType = AV_TEXT;

	BRect r(0,0,100,20);
	BRect tr = r;
	r.OffsetBy(32000,0);

	mBTextView = new MonoText(r,"BeInputText",tr,this,IsPassword());		// Not visible yet
	mFinal = false;
}

BeInputText::~BeInputText()
{
	if (mBTextView)
		mBTextView->ForgetInputText();
}

#ifdef JAVASCRIPT
void BeInputText::ValueChanged()
{
	if (mOnChangeScript.Length())
		mHTMLDoc->ExecuteHandler(mContainer, mOnChange);

	InputGlyph::ValueChanged();
}
#endif

void BeInputText::Blur()
{
	if (mBTextView)
		mBTextView->MakeFocus(false);
}

void BeInputText::Focus()
{
	if (mBTextView)
		mBTextView->MakeFocus(true);
}

void BeInputText::Select()
{
	if (mBTextView)
		mBTextView->SelectAll();
}

void BeInputText::Click()
{
	if (mBTextView)
		mBTextView->MakeFocus(true);
}

//	Handle tabs and returns

bool BeInputText::SpecialKey(ulong aChar)
{
	switch (aChar) {
		case B_TAB: {
			InputGlyph* textField = GetForm()->NextTextField(this);	// Tab to next field
			if (textField) {
				SetTarget(NULL,false);
				textField->SetTarget(NULL,true);
				textField->SelectAll(NULL);
			}
			return false;
		}
			
//		Enter will work as a submit if there is one submit and one text field.

		case B_ENTER: {
			if (mRows > 1)	// Multiline text entry field
				return true;
				
			InputGlyph* submit = GetForm()->SubmitTextField(this);

			if (submit) {
				mBTextView->SelectAll();
				BMessage msg(FORM_MSG + AV_SUBMIT);
				msg.AddPointer("InputGlyph",submit);
				BWindow* w = mBTextView->Window();
				w->PostMessage(&msg,FindHTMLView(mBTextView));
			}
			return false;	// Don't pass it on
		}
	}
	return true;
}
//	Reset button to default value

void BeInputText::Reset(DrawPort *drawPort)
{
	InputGlyph::Reset(drawPort);
	if (mBTextView) {
		if (mValue.Length() > 0)
			mBTextView->SetText(mValue.String());
		else
			mBTextView->SetText("");
	}
}

const char *BeInputText::GetValue()
{
	if (mBTextView)
		return mBTextView->Text();
	else
		return mValue.String();
}

//	Size the view but position it offscreen

void BeInputText::Layout(DrawPort *drawPort)
{
	if (!mFinal) {
		BView *view = ((BeDrawPort *)drawPort)->GetView();		// Show it for the first time

		font_height fh;
		mBTextView->GetFontHeight(&fh);
		float width = mCols*mBTextView->StringWidth("m",1) + 2;		// Monospaced, remember?
		//Ord height = mRows*(fh.ascent + fh.descent) + 2;			// Add 1 pixel border around text inside view (TextRect)
		float height = mRows * mBTextView->LineHeight() + 4.0;
		mBTextView->ResizeTo(width-1,height-1);
		mBTextView->SetTextRect(BRect(1, 1, 
								(mRows > 1) ? width - 2 : 
											  mBTextView->StringWidth(mBTextView->Text()) + 10,
								height - 1));
		mBTextView->SetWordWrap((mRows > 1) ? true : false);	
		mBTextView->MakeResizable((mRows > 1) ? false : true);
		
		if (mRows > 1) {
			BRect r(0,0,width, B_H_SCROLL_BAR_HEIGHT);
			r.OffsetBy(32000,0);
			mHScrollBar = new BScrollBar(r, NULL, mBTextView, 0, mCols, B_HORIZONTAL);
			mHScrollBar->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);
			view->AddChild(mHScrollBar);
			
			r.Set(0,0,B_V_SCROLL_BAR_WIDTH, height);
			r.OffsetBy(32000,0);
			mVScrollBar = new BScrollBar(r, NULL, mBTextView, 0, mRows, B_VERTICAL);
			mVScrollBar->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);
			view->AddChild(mVScrollBar);
			
			width += B_V_SCROLL_BAR_WIDTH;
			height += B_H_SCROLL_BAR_HEIGHT;
		}

		if (mMaxLength)
			mBTextView->SetMaxBytes(mMaxLength);

		view->AddChild(mBTextView);

		SetWidth(kMarginLeft + 1 + width + 1 + kMarginRight);
		SetHeight(kMarginTop + 1 + height + 1 + kMarginBottom);
		mFinal = true;
	}
}

float BeInputText::GetMinUsedWidth(DrawPort *drawPort)
{
	Layout(drawPort);
	return InputGlyph::GetMinUsedWidth(drawPort);
}

float BeInputText::GetMaxUsedWidth(DrawPort *drawPort)
{
	Layout(drawPort);
	return InputGlyph::GetMaxUsedWidth(drawPort);
}

void BeInputText::Draw(DrawPort *drawPort)
{
	if (mBTextView == NULL)
		return;
		
	BRect r;
	GetBounds(&r);

	r.left += kMarginRight;
	r.right -= kMarginLeft;
	r.top += kMarginTop;
	r.bottom -= kMarginBottom;

//	mBTextView->Position();
	if (mHScrollBar && mVScrollBar) {
		BRect f = mBTextView->Frame();
		mHScrollBar->MoveTo(f.left - 1, f.bottom + 1);
		mVScrollBar->MoveTo(f.right + 1, f.top - 1);
		mHScrollBar->ResizeTo(f.Width() + 2, B_H_SCROLL_BAR_HEIGHT);
		mVScrollBar->ResizeTo(B_V_SCROLL_BAR_WIDTH, f.Height() + 2);
		r.right -= B_V_SCROLL_BAR_WIDTH + 1;
		r.bottom -= B_H_SCROLL_BAR_HEIGHT + 1;

		drawPort->SetGray(176);
		drawPort->MoveTo(r.left,r.bottom);
		drawPort->LineTo(r.left,r.top);
		drawPort->LineTo(r.right,r.top);
	} else {
		drawPort->DrawBevel(&r);
	}	
}

//	Send the Name/Value pair for the contents of the edit field

void BeInputText::Submit(uint32 encoding, BString& submission)
{
	submission = "";
	if (mBTextView == NULL)
		return;
		
	const char	*theText;
	if (IsPassword())
		theText = mBTextView->ActualText();
	else
		theText = mBTextView->Text();
	BString		text = "";

	if ((mWrapType != AV_HARD) && (mWrapType != AV_PHYSICAL))
		text = theText;
	else {
		int32	numLines = mBTextView->CountLines();
		int32	contentLen = 0;
		char	*content = (char *)malloc(mBTextView->TextLength() + numLines);	// most we'll ever need

		for (int32 i = 0; i < numLines; i++) {
			int32 startOffset = mBTextView->OffsetAt(i);
			int32 endOffset = mBTextView->OffsetAt(i + 1);
			int32 lineLen = endOffset - startOffset;
	
			memcpy(content + contentLen, theText + startOffset, lineLen);
			contentLen += lineLen;
	
			// add a newline to every line except for the ones
			// that already end in newlines, and the last line 
			if ((theText[endOffset - 1] != '\n') && (i < (numLines - 1)))
				content[contentLen++] = '\n';
		}
		content[contentLen] = '\0';
		text = content;

		free(content);
	}

	BString encText = "";
	Encode(encoding, text, encText);

	if (mName.Length()) {
		Encode(encoding, mName,submission);
		submission += "=";
		submission += encText;
	} else
		submission = encText;
}

void BeInputText::SetTop(float top)
{
	InputGlyph::SetTop(top);
	mBTextView->SetTop(kMarginTop + GetTop() + 1);
	mBTextView->Position();
}


void BeInputText::SetLeft(float left)
{
	InputGlyph::SetLeft(left);
	mBTextView->SetLeft(kMarginLeft + GetLeft() + 1);
	mBTextView->Position();
}

void BeInputText::OffsetBy(const BPoint& offset)
{
	InputGlyph::OffsetBy(offset);
	mBTextView->SetTop(kMarginTop + GetTop() + 1);
	mBTextView->SetLeft(kMarginLeft + GetLeft() + 1);
	mBTextView->Position();
}


void
BeInputText::SetAttributeStr(
	long		attributeID,
	const char*	value)
{
	switch (attributeID) {
		case A_VALUE:
			SetValue(value, false);
			break;
		case A_ONCHANGE:
			mOnChangeScript = value;
#ifdef JAVASCRIPT
			if (!mHTMLDoc->LockDocAndWindow())
				return;
			mHTMLDoc->InitJavaScript();
			mHTMLDoc->UnlockDocAndWindow();
#endif
			break;
		default:
			InputGlyph::SetAttributeStr(attributeID, value);
			break;
	}
}

#ifdef JAVASCRIPT
void BeInputText::SetContainer(jseVariable container)
{
	if (!mContainer) {
		if (!mHTMLDoc->LockDocAndWindow())
			return;
		mContainer = container;
		if (mOnChangeScript.Length())
			mOnChange = mHTMLDoc->AddHandler(container, "onChange", mOnChangeScript.String());
		else
			mOnChange = 0;
		mHTMLDoc->UnlockDocAndWindow();
	}
	InputGlyph::SetContainer(container);
}
#endif


void BeInputText::SetValue(const char *value, bool force)
{
	if (mRows > 1) {
		// this is a textarea, not input type = "text"
		BString theValue;
		ParseValue(value,theValue);

		theValue.ReplaceAll("\r\n","\n");
		theValue.ReplaceAll("\r", "\n");
		
		mOrigValue = theValue;
		if (!mUserValuesSet || force)
			mValue = theValue;

		if (mBTextView)
			if (mValue.Length() > 0)
				mBTextView->SetText(mValue.String());
			else
				mBTextView->SetText("");
	} else if (mBTextView) {
		mOrigValue = value;
		if (!mUserValuesSet || force)
			mValue = value;

		if (mValue.Length() > 0)
			mBTextView->SetText(mValue.String());
		else
			mBTextView->SetText("");
	} else
		InputGlyph::SetAttributeStr(A_VALUE, value);
}

InputValueItem*
BeInputText::UserValues()
{
	CLinkedList *list = NULL;

	if (mOption.First() != NULL) {
		list = new CLinkedList();
		for (Option *o = (Option *)mOption.First(); o != NULL; o = (Option *)o->Next()) 
			list->Add(new Option(o));
	}

	return (new InputValueItem(mBTextView->ActualText(), mChecked, list));
}

//	Make me the target

void BeInputText::SetTarget(DrawPort*, bool target)
{
	mBTextView->MakeFocus(target);
}

bool	BeInputText::IsTarget()
{
	if (mBTextView == NULL)
		return false;
	return mBTextView->IsFocus();
}

void BeInputText::SelectAll(DrawPort *)
{
	if (mBTextView == NULL)
		return;
	mBTextView->SelectAll();
}

void
BeInputText::Init()
{
	BWindow *window = mBTextView->Window();
	if (window && !window->Lock())
		return;

	InputGlyph::Init();

	if (mBTextView)
		if (mValue.Length() > 0)
			mBTextView->SetText(mValue.String());
		else
			mBTextView->SetText("");
	if (window)
		window->Unlock();
}

// ===========================================================================
//	BeControl

class DumbRadioButton : public BRadioButton {
public:
				DumbRadioButton(InputGlyph* radioGlyph,
								BRect frame,
								const char *name,
								const char *label,
								BMessage *message,
								ulong resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
								ulong flags = B_WILL_DRAW | B_NAVIGABLE); 
virtual	void	SetValue(long value);
virtual void	MakeFocus(bool focus);
virtual	void	Draw(BRect updateRect);
		void	SetTop(float top) {mViewTop = top;}
		void	SetLeft(float left) {mViewLeft = left;}
		void	Position();
virtual void	AttachedToWindow();

protected:
		float	mViewTop,mViewLeft;
		float	mFrameTop, mFrameLeft;
	InputGlyph	*mRadioGlyph;
};

DumbRadioButton::DumbRadioButton(InputGlyph* radioGlyph,
								BRect frame,
								const char *name,
								const char *label,
								BMessage *message,
								ulong resizeMask,
								ulong flags) : BRadioButton(frame,name,label,message,resizeMask,flags) , mRadioGlyph(radioGlyph)
{
	mFrameLeft = mFrameTop = -1;
	mViewLeft = mViewTop = -1;
}

void DumbRadioButton::AttachedToWindow()
{
	BRadioButton::AttachedToWindow();
//	SetViewColor(B_TRANSPARENT_32_BIT);
//	SetLowColor(B_TRANSPARENT_32_BIT);
}

void DumbRadioButton::SetValue(long value)
{
	if (value) {
		const char* s = mRadioGlyph->GetName();
		Form* f = mRadioGlyph->GetForm();
		
		InputGlyph *g;
		for (int i = 0; (bool)(g = f->GetInput(i)); i++) {
			if (g->IsRadio() && ((s == NULL && g->GetName() == NULL) || (s && g->GetName() && !strcmp(s,g->GetName()))))
				if (g != mRadioGlyph) {
					g->SetCheck(0,NULL);
#ifdef JAVASCRIPT
					g->ValueChanged();
#endif
				}
		}
		BControl::SetValue(1);
	} else
		BRadioButton::SetValue(value);
#ifdef JAVASCRIPT
	mRadioGlyph->ValueChanged();
#endif
}

void DumbRadioButton::MakeFocus(bool focus)
{
	BRadioButton::MakeFocus(focus);
	HTMLView *view = dynamic_cast<HTMLView *>(Parent());
	if (view && focus) {
		view->MakeVisible(mViewLeft, mViewTop);
//		view->ScrollToH(mViewLeft);
//		view->ScrollToV(mViewTop);
	}
}

void DumbRadioButton::Draw(BRect updateRect)
{
	if (updateRect.right == Bounds().right)
		updateRect.right -= 2;
	BRadioButton::Draw(updateRect);

	if (IsFocus())
		SetHighColor(keyboard_navigation_color());
	else
		SetHighColor(LowColor());
		
	StrokeRect(Bounds());
}

void DumbRadioButton::Position()
{
	// Since calling MoveTo on a BView is an extremely expensive
	// operation, we want to avoid calling it unnecessarily at all costs.
	// Logically, we would like to move this view when its parent InputGlyph
	// subclass gets a SetTop or SetLeft, but this happens too many times,
	// so we will just set special members in this view and not call the MoveTo
	// until this function is called just prior to drawing.
	BRect parentBounds = Parent()->Bounds();
	float parentLeft = parentBounds.left;
	float parentTop = parentBounds.top;

	if (mFrameLeft - parentLeft != 0 || mFrameTop - parentTop != 0) {
		mFrameLeft = mViewLeft - parentLeft;
		mFrameTop = mViewTop - parentTop;
		if (mViewLeft >= 0 && mViewTop >= 0)
			MoveTo(mFrameLeft, mFrameTop);
	}
}


class CheckBox : public BCheckBox
{
public:
				CheckBox(BRect frame, const char *name, const char *label, BMessage *message, InputGlyph *inputGlyph);
virtual void	MakeFocus(bool focus);
virtual	void	Draw(BRect updateRect);
		void	SetTop(float top) {mViewTop = top;}
		void	SetLeft(float left) {mViewLeft = left;}
		void	Position();
virtual void	AttachedToWindow();
virtual void	SetValue(long value);

protected:
		float	mViewTop,mViewLeft;
		float	mFrameLeft, mFrameTop;
		InputGlyph	*mInputGlyph;
};

CheckBox::CheckBox(BRect frame, const char *name, const char *label, BMessage *message, InputGlyph *inputGlyph) : BCheckBox(frame, name, label, message)
{
	mFrameLeft = mFrameTop = -1;
	mViewLeft = mViewTop = -1;
	mInputGlyph = inputGlyph;
}

void CheckBox::SetValue(long value)
{
	BCheckBox::SetValue(value);
#ifdef JAVASCRIPT
	mInputGlyph->ValueChanged();
#endif
}

void CheckBox::AttachedToWindow()
{
	BCheckBox::AttachedToWindow();
	//SetViewColor(B_TRANSPARENT_32_BIT);
	//SetLowColor(B_TRANSPARENT_32_BIT);
}

void CheckBox::MakeFocus(bool focus)
{
	BCheckBox::MakeFocus(focus);
	HTMLView *view = dynamic_cast<HTMLView *>(Parent());
	if (view && focus) {
		view->MakeVisible(mViewLeft, mViewTop);
//		view->ScrollToH(mViewLeft);
//		view->ScrollToV(mViewTop);
	}
}

void CheckBox::Draw(BRect updateRect)
{
	if (updateRect.right == Bounds().right)
		updateRect.right -= 2;
	BCheckBox::Draw(updateRect);

	if (IsFocus())
		SetHighColor(keyboard_navigation_color());
	else
		SetHighColor(LowColor());
		
	StrokeRect(Bounds());
}

void CheckBox::Position()
{
	// Since calling MoveTo on a BView is an extremely expensive
	// operation, we want to avoid calling it unnecessarily at all costs.
	// Logically, we would like to move this view when its parent InputGlyph
	// subclass gets a SetTop or SetLeft, but this happens too many times,
	// so we will just set special members in this view and not call the MoveTo
	// until this function is called just prior to drawing.
	BRect parentBounds = Parent()->Bounds();
	float parentLeft = parentBounds.left;
	float parentTop = parentBounds.top;

	if (mFrameLeft - parentLeft != 0 || mFrameTop - parentTop != 0) {
		mFrameLeft = mViewLeft - parentLeft;
		mFrameTop = mViewTop - parentTop;
		if (mViewLeft >= 0 && mViewTop >= 0)
			MoveTo(mFrameLeft, mFrameTop);
	}
}


class Button : public BButton
{
public:
				Button(BRect frame, const char *name, const char *label, BMessage *message);
virtual void	MakeFocus(bool focus);
		void	SetTop(float top) {mViewTop = top;}
		void	SetLeft(float left) {mViewLeft = left;}
		void	Position();

protected:
		float	mViewTop,mViewLeft;
		float	mFrameLeft, mFrameTop;
};

Button::Button(BRect frame, const char *name, const char *label, BMessage *message) : BButton(frame, name, label, message)
{
	mFrameLeft = mFrameTop = -1;
	mViewLeft = mViewTop = -1;
}

void Button::MakeFocus(bool focus)
{
	BButton::MakeFocus(focus);
	HTMLView *view = dynamic_cast<HTMLView *>(Parent());
	if (view && focus) {
		view->MakeVisible(mViewLeft, mViewTop);
//		view->ScrollToH(mViewLeft);
//		view->ScrollToV(mViewTop);
	}
}

void Button::Position()
{
	// Since calling MoveTo on a BView is an extremely expensive
	// operation, we want to avoid calling it unnecessarily at all costs.
	// Logically, we would like to move this view when its parent InputGlyph
	// subclass gets a SetTop or SetLeft, but this happens too many times,
	// so we will just set special members in this view and not call the MoveTo
	// until this function is called just prior to drawing.
	BRect parentBounds = Parent()->Bounds();
	float parentLeft = parentBounds.left;
	float parentTop = parentBounds.top;

	if (mFrameLeft - parentLeft != 0 || mFrameTop - parentTop != 0) {
		mFrameLeft = mViewLeft - parentLeft;
		mFrameTop = mViewTop - parentTop;
		if (mViewLeft >= 0 && mViewTop >= 0)
			MoveTo(mFrameLeft, mFrameTop);
	}
}



BeControl::BeControl(Document* htmlDoc) :
	InputGlyph(htmlDoc)
{
	mSizeFinal = false;
	mBControl = NULL;				// Not visible yet
}

BeControl::~BeControl()
{
}

void BeControl::Blur()
{
	if (mBControl)
		mBControl->MakeFocus(false);
}

void BeControl::Focus()
{
	if (mBControl)
		mBControl->MakeFocus(true);
}

void BeControl::Click()
{
	if (mBControl) {
		mBControl->Window()->PostMessage(B_MOUSE_DOWN, mBControl);
		mBControl->Window()->PostMessage(B_MOUSE_UP, mBControl);
	}
}

const char* BeControl::GetTitle()
{
	if (mValue.Length())
		return mValue.String();
	if (IsSubmit())
		return kSubmitButtonTitle;
	if (IsReset())
		return kResetButtonTitle;
	return kGenericButtonTitle;
}

//	Create the correct type of control
//	Target the HTMLView.....

void BeControl::Create(BView *view)
{
	BRect r(0,0,13,13);
	r.OffsetBy(32000,0);
	const char *title = GetTitle();
	
	BMessage* msg;
	switch (mType) {
		case AV_SUBMIT:
			msg = new BMessage(FORM_MSG + AV_SUBMIT);
			msg->AddPointer("InputGlyph",this);
			mBControl = new Button(r,NULL,title,msg);
			break;
		case AV_RESET:
			msg = new BMessage(FORM_MSG + AV_RESET);
			msg->AddPointer("InputGlyph",this);
			mBControl = new Button(r,NULL,title,msg);
			break;
		case AV_BUTTON:
			msg = new BMessage(FORM_MSG + AV_BUTTON);
			msg->AddPointer("InputGlyph",this);
			mBControl = new Button(r,NULL,title,msg);
			break;
		case AV_CHECKBOX:
			mBControl = new CheckBox(r,"BCheckBox",NULL,new BMessage(FORM_MSG + AV_CHECKBOX), this);
			break;
		
		case AV_RADIO:
			mBControl = new DumbRadioButton(this,r,"BRadioButton",NULL,new BMessage(FORM_MSG + AV_RADIO));
			break;
	}

	view->AddChild(mBControl);
	mBControl->SetTarget(FindHTMLView(mBControl));

	switch (mType) {
		case AV_CHECKBOX:
		case AV_RADIO:
			mBControl->SetValue(mChecked);
			break;
	}

	BWindow *window = mBControl->Window();
	if (window && !window->Lock())
		return;
	if (dynamic_cast<BButton *>(mBControl) != NULL) {
		Style	theStyle = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
		FindHTMLView(mBControl)->SetSubviewFont(theStyle, mBControl);
	}

	mBControl->ResizeToPreferred();

	BRect frame = mBControl->Frame();
	if (window)
		window->Unlock();
	SetWidth(kMarginLeft + frame.Width() + kMarginRight);
	SetHeight(kMarginTop + frame.Height() + kMarginBottom);
}

//	Create and size the control

void BeControl::Layout(DrawPort *drawPort)
{
	if (mSizeFinal == false) {
		BView *view = ((BeDrawPort *)drawPort)->GetView();	// Show it for the first time
		Create(view);
		mSizeFinal = true;
	}
}

float BeControl::GetMinUsedWidth(DrawPort *drawPort)
{
	Layout(drawPort);
	return InputGlyph::GetMinUsedWidth(drawPort);
}

float BeControl::GetMaxUsedWidth(DrawPort *drawPort)
{
	Layout(drawPort);
	return InputGlyph::GetMaxUsedWidth(drawPort);
}

void BeControl::SetTop(float top)
{
	InputGlyph::SetTop(top);
	Button *btn = dynamic_cast<Button *>(mBControl);
	if (btn) {
		btn->SetTop(kMarginLeft + top);
		btn->Position();
	}
	CheckBox *cbx = dynamic_cast<CheckBox *>(mBControl);
	if (cbx) {
		cbx->SetTop(kMarginLeft + top);
		cbx->Position();
	}
	DumbRadioButton *drb = dynamic_cast<DumbRadioButton *>(mBControl);
	if (drb) {
		drb->SetTop(kMarginLeft + top);
		drb->Position();
	}
}


void BeControl::SetLeft(float left)
{
	InputGlyph::SetLeft(left);
	Button *btn = dynamic_cast<Button *>(mBControl);
	if (btn) {
		btn->SetLeft(kMarginLeft + left);
		btn->Position();
	}
	CheckBox *cbx = dynamic_cast<CheckBox *>(mBControl);
	if (cbx) {
		cbx->SetLeft(kMarginLeft + left);
		cbx->Position();
	}
	DumbRadioButton *drb = dynamic_cast<DumbRadioButton *>(mBControl);
	if (drb) {
		drb->SetLeft(kMarginLeft + left);
		drb->Position();
	}
}

void BeControl::OffsetBy(const BPoint& offset)
{
	InputGlyph::OffsetBy(offset);
	Button *btn = dynamic_cast<Button *>(mBControl);
	if (btn) {
		btn->SetTop(kMarginLeft + mTop);
		btn->SetLeft(kMarginLeft + mLeft);
		btn->Position();
	}
	CheckBox *cbx = dynamic_cast<CheckBox *>(mBControl);
	if (cbx) {
		cbx->SetTop(kMarginLeft + mTop);
		cbx->SetLeft(kMarginLeft + mLeft);
		cbx->Position();
	}
	DumbRadioButton *drb = dynamic_cast<DumbRadioButton *>(mBControl);
	if (drb) {
		drb->SetTop(kMarginLeft + mTop);
		drb->SetLeft(kMarginLeft + mLeft);
		drb->Position();
	}
}


/*
void BeControl::Draw(DrawPort *)
{
	Button *btn = dynamic_cast<Button *>(mBControl);
	if (btn) btn->Position();
	CheckBox *cbx = dynamic_cast<CheckBox *>(mBControl);
	if (cbx) cbx->Position();
	DumbRadioButton *drb = dynamic_cast<DumbRadioButton *>(mBControl);
	if (drb) drb->Position();
}
*/

bool BeControl::GetCheck()
{
	if (mBControl)
		return mBControl->Value();
	else
		return mChecked;
}

void BeControl::SetCheck(bool on, DrawPort *)
{
	if (mBControl)
		mBControl->SetValue(on);
	else
		mChecked = on;
}

void BeControl::Reset(DrawPort *drawPort)
{
	InputGlyph::Reset(drawPort);

	if (mBControl)
		mBControl->SetValue(mChecked);
}

void BeControl::Submit(uint32 encoding, BString& submission)
{
	switch (mType) {
		case AV_CHECKBOX:
		case AV_RADIO:
			if (mValue.Length() == 0)
				mValue = "on";
			if (!GetCheck())
				return;
			break;
	}
	InputGlyph::Submit(encoding, submission);
}

void BeControl::SetValue(const char *value, bool force)
{
	switch (mType) {
		case AV_SUBMIT:
		case AV_RESET:
		case AV_BUTTON:
			if (mBControl)
				mBControl->SetLabel(value);
			break;
		default:
			InputGlyph::SetValue(value, force);
	}
}


InputValueItem*
BeControl::UserValues()
{
	CLinkedList *list = NULL;

	if (mOption.First() != NULL) {
		list = new CLinkedList();
		for (Option *o = (Option *)mOption.First(); o != NULL; o = (Option *)o->Next()) 
			list->Add(new Option(o));
	}

	return (new InputValueItem(mValue.String(), (mBControl != NULL) ? mBControl->Value() : false, list));
}

void
BeControl::Init()
{
	InputGlyph::Init();

	if (mBControl)
		mBControl->SetValue(mChecked);
}

// ===========================================================================
//	Return selected items

SelectView::SelectView(
	bool	multiple)
{
	mMultiple = multiple;
	mOptions = NULL;
}

SelectView::~SelectView()
{
	// keep this for virtual-ness...
}

int32 SelectView::GetNumOptions()
{
	if (!mOptions)
		return 0;
		
	int32 i = 0;
	Option *o = (Option*)mOptions->First();
	while (o) {
		o = (Option*)o->Next();
		i++;
	}
	
	return i;
}

int32 SelectView::GetSelectedIndex()
{
	if (!mOptions)
		return 0;
		
	int i = 0;
	Option *o = (Option*)mOptions->First();
	while (o) {
		if (OptionIsMarked(i))
			return i;
		o = (Option*)o->Next();
		i++;
	}
	return -1;
}

void SelectView::SetSelectedIndex(int32)
{
}

void SelectView::Submit(uint32 encoding, BString& name, BString& submission, InputGlyph *g)
{
	submission = "";
	if (mOptions == NULL)
		return;
		
	int	i = 0;
	Option *o;
	for (o = (Option *)mOptions->First(); o; o = (Option *)o->Next()) {
		if (OptionIsMarked(i)) {
			const char *v = o->mHasValue ? o->mValue.String() : o->mOption.String();
//			const char *v = o->mHasValue ? o->mValue.String() : "";
			
			if (name.Length() != 0) {
				BString value;
				if (v && *v) {
					BString vstr(v);
					g->Encode(encoding, vstr,value);
				}
				if (submission.Length() != 0)
					submission += "&";
				submission += name;
				submission += "=";
				if (v && *v)
					submission += value;
			}
		}
		i++;
	}
}

void SelectView::GetOptimalSize(float *width, float *height)
{
	*width = 200;
	*height = 20;
}

void SelectView::SetOptions(CLinkedList *options)
{
	mOptions = options;
}

void SelectView::UpdateOption(Option *o)
{
}

Option* SelectView::OptionAt(int32 index)
{
	if (!mOptions)
		return NULL;
		
	int i = 0;
	for (CLinkable* item = mOptions->First(); item; item = item->Next()) {
		if (i++ == index)
			return (Option *)item;
	}
	return NULL;
}

void SelectView::Reset()
{
}

bool SelectView::OptionIsMarked(int)
{
	return false;
}

InputValueItem*
SelectView::UserValues(
	InputValueItem	*item)
{
	if (item->mOption != NULL) {
		int32 i = 0;
		for (Option *o = (Option *)item->mOption->First(); o != NULL; o = (Option *)o->Next()) {
			o->mSelected = OptionIsMarked(i);
			i++;
		}
	}

	return (item);
}

// ===========================================================================
//	Class for a popup menu

MenuView::MenuView(
	BRect	rect, 
	const char	*name,
	bool	multiple,
	BeSelect*	glyph) 
		: BView(rect, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE), 
		  SelectView(multiple), mGlyph(glyph)
{
	mBPopUpMenu = 0;
	mWidth = 0;
	mFrameLeft = mFrameTop = -1;
}

MenuView::~MenuView()
{
	delete mBPopUpMenu;
}

void MenuView::AttachedToWindow()
{
	BView::AttachedToWindow();

	Style	theStyle = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
	FindHTMLView(this)->SetSubviewFont(theStyle, this);
	
	SetViewColor(0xD0,0xD0,0xD0);
}

void MenuView::Draw(BRect)
{
	BRect r = Bounds();

	SetHighColor(96,96,96);
	StrokeRect(r);
	SetHighColor(0,0,0);

	r.InsetBy(1,1);

	const rgb_color c1 = {240, 240, 240, 255};
	const rgb_color c2 = {128, 128, 128, 255};

	BeginLineArray(4);
	AddLine(BPoint(r.left, r.bottom), BPoint(r.left, r.top), c1);
	AddLine(BPoint(r.left, r.top), BPoint(r.right, r.top), c1);
	AddLine(BPoint(r.right, r.top), BPoint(r.right, r.bottom), c2);
	AddLine(BPoint(r.right, r.bottom), BPoint(r.left, r.bottom), c2);
	EndLineArray();

//	Draw the currently selected thingy

	BMenuItem *item = GetMarked();
	SetLowColor(0xD0,0xD0,0xD0);
	if (item)
		DrawString(item->Label(),BPoint(r.left + 15,r.bottom - 4));
		
	if (IsFocus())
		SetHighColor(keyboard_navigation_color());
	else
		SetHighColor(0xd0,0xd0,0xd0);
		
	StrokeLine(BPoint(r.left + 15, r.bottom - 2), BPoint(r.right - 15, r.bottom - 2));

	SetLowColor(0,0,0);
}

void MenuView::MakeFocus(bool focus)
{
	BView::MakeFocus(focus);
	HTMLView *view = dynamic_cast<HTMLView *>(Parent());
	if (view && focus) {
		view->MakeVisible(mViewLeft, mViewTop);
//		view->ScrollToH(mViewLeft);
//		view->ScrollToV(mViewTop);
	}
	Draw(Bounds());
}

void MenuView::Position()
{
	// Since calling MoveTo on a BView is an extremely expensive
	// operation, we want to avoid calling it unnecessarily at all costs.
	// Logically, we would like to move this view when its parent InputGlyph
	// subclass gets a SetTop or SetLeft, but this happens too many times,
	// so we will just set special members in this view and not call the MoveTo
	// until this function is called just prior to drawing.
	BRect parentBounds = Parent()->Bounds();
	float parentLeft = parentBounds.left;
	float parentTop = parentBounds.top;

	if (mFrameLeft - parentLeft != 0 || mFrameTop - parentTop != 0) {
		mFrameLeft = mViewLeft - parentLeft;
		mFrameTop = mViewTop - parentTop;
		if (mViewLeft >= 0 && mViewTop >= 0)
			MoveTo(mFrameLeft, mFrameTop);
	}
}


void MenuView::MouseDown(BPoint)
{
//	DoMenu(mBPopUpMenu->IsStickyPrefOn());
	DoMenu(false);
}

void MenuView::KeyDown(const char *bytes, int32 numBytes)
{
	if (numBytes == 1 && *bytes == ' ')
		DoMenu(true);
	else
		BView::KeyDown(bytes, numBytes);
}

void MenuView::GetOptimalSize(float *width, float *height)
{
	font_height fh;
	GetFontHeight(&fh);
	*height = 3.0 + ceil(fh.ascent + fh.descent) + 4.0;
	*width = mWidth;
}

void MenuView::DoMenu(bool keepOpen)
{
	if (mBPopUpMenu == NULL)
		return;

	BPoint where = BPoint(Bounds().left+1,Bounds().top+1);
	ConvertToScreen(&where);

	BMenuItem *oldItem = GetMarked();

	BMenuItem *item = mBPopUpMenu->Go(where, false, keepOpen, Frame());

	bool changed = false;
	if (item) {
		if (item != oldItem)
			oldItem->SetMarked(false);
			
		if (!item->IsMarked())
			changed = true;
		item->SetMarked(!item->IsMarked());
		if (GetMarked() == NULL)
			GetItemAt(0)->SetMarked(true);
		Invalidate();
		Window()->UpdateIfNeeded();
	}
	if (changed)
		mGlyph->SelectViewChanged();
}

void MenuView::SetOptions(CLinkedList* options)
{
	mWidth = 0;
	mOptions = options;

	mBPopUpMenu = new BPopUpMenu("Popup");
	mBPopUpMenu->SetRadioMode(!mMultiple);

	Style	theStyle = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
	FindHTMLView(this)->SetSubviewFont(theStyle, mBPopUpMenu);

	BMenuItem *item;
	
	if (mOptions == NULL)
		return;
		
	Option *o;
	for (o = (Option *)mOptions->First(); o; o = (Option *)o->Next()) {
		
		const char *s;							// Can have an empty option
		if (o->mOption.Length() == 0)
			s = "";		
		else
			s = o->mOption.String();


		BMenu *menuToUse = mBPopUpMenu;
		
		if (o->mOptGroupLabel.Length() > 0) {
			BMenuItem *submenuItem = mBPopUpMenu->FindItem(o->mOptGroupLabel.String());
			if (!submenuItem) {
				menuToUse = new BMenu(o->mOptGroupLabel.String());
				mBPopUpMenu->AddItem(menuToUse);
			} else
				menuToUse = submenuItem->Submenu();
			
		}
		if (!menuToUse)
			menuToUse = mBPopUpMenu;

		if ((strcmp(s, "-") == 0) || (strcmp(s, "- ") == 0))
			item = new BSeparatorItem();
		else
			item = new BMenuItem(s, new BMessage('item'));

		menuToUse->AddItem(item);
		mWidth = MAX(mWidth,15 + StringWidth(s) + 24);
		if (o->mSelected)
			item->SetMarked(true);
	}
	
	if (GetMarked() == NULL) {
		item = GetItemAt(0);	// Zero based?
		if (item)
			item->SetMarked(true);
	}
}

void MenuView::UpdateOption(Option *inOption)
{
	if (!mOptions)
		return;
		
	int i = 0;
	Option *o = (Option *)mOptions->First();
	while (o && o != inOption) {
		o = (Option *)o->Next();
		i++;
	}
	if (!o)
		return;
	BMenuItem *item = GetItemAt(i);

	if (!item)
		return;
	item->SetMarked(o->mSelected);
	item->SetLabel(o->mOption.String());
}


BMenuItem* MenuView::GetItemAt(int i)
{
	int k = 0;
	BMenuItem *item;
	
	for (int j = 0; j < mBPopUpMenu->CountItems(); j++) {
		item = mBPopUpMenu->ItemAt(j);
		BMenu *menu = item->Submenu();
		if (menu) {
			for (int j = 0; j < menu->CountItems(); j++) {
				item = menu->ItemAt(j);
				if (k++ == i)
					return item;
			}
		} else
			if (k++ == i)
				return item;
	}
	return NULL;
}

BMenuItem* MenuView::GetMarked()
{
	BMenuItem *item;
	
	item = mBPopUpMenu->FindMarked();
	if (item)
		return item;
		
	for (int j = 0; j < mBPopUpMenu->CountItems(); j++) {
		item = mBPopUpMenu->ItemAt(j);
		BMenu *menu = item->Submenu();
		if (menu) {
			item = menu->FindMarked();
			if (item)
				return item;
		}
	}
	
	return NULL;
}

//	Reset to defaults 

void MenuView::Reset()
{
	Option *o;
	BMenuItem *item,*oldItem = GetMarked();
	
	if (mOptions == NULL)
		return;

	int i = 0;
	for (o = (Option *)mOptions->First(); o; o = (Option *)o->Next()) {
		item = GetItemAt(i);
		item->SetMarked(o->mSelected);
		i++;
	}
	if (GetMarked() == NULL) {
		item = GetItemAt(0);		// Zero based?
		item->SetMarked(true);
	}
	if (oldItem != GetMarked())
		Invalidate();
}

bool MenuView::OptionIsMarked(int i)
{
	BMenuItem *item = GetItemAt(i);
	if (item)
		return item->IsMarked();
	return false;
}

void MenuView::SetSelectedIndex(int32 index)
{
	for (int i = 0; i < mBPopUpMenu->CountItems(); i++) {
		BMenuItem *item = GetItemAt(i);
		if (item)
			item->SetMarked(index == i);
	}
}

// ===========================================================================


ListView::ListView(
	BListView	*target, 
	int			size, 
	char		*name,
	bool		multiple,
	BeSelect*	glyph) 
		: BScrollView(name, target, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_NAVIGABLE, FALSE, TRUE), 
		  SelectView(multiple), mGlyph(glyph)
{
	mSize = size;
	mListView = target;
	mFrameLeft = mFrameTop = -1;
}

ListView::~ListView()
{
	// keep this for virtual-ness...
}

void
ListView::AttachedToWindow()
{
	BScrollView::AttachedToWindow();

	Style	theStyle = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
	FindHTMLView(this)->SetSubviewFont(theStyle, mListView);
}

void ListView::MakeFocus(bool focus)
{
	BScrollView::MakeFocus(focus);
	HTMLView *view = dynamic_cast<HTMLView *>(Parent());
	if (view && focus) {
		view->MakeVisible(mViewLeft, mViewTop);
//		view->ScrollToH(mViewLeft);
//		view->ScrollToV(mViewTop);
	}
}

void ListView::Position()
{
	// Since calling MoveTo on a BView is an extremely expensive
	// operation, we want to avoid calling it unnecessarily at all costs.
	// Logically, we would like to move this view when its parent InputGlyph
	// subclass gets a SetTop or SetLeft, but this happens too many times,
	// so we will just set special members in this view and not call the MoveTo
	// until this function is called just prior to drawing.
	BRect parentBounds = Parent()->Bounds();
	float parentLeft = parentBounds.left;
	float parentTop = parentBounds.top;

	if (mFrameLeft - parentLeft != 0 || mFrameTop - parentTop != 0) {
		mFrameLeft = mViewLeft - parentLeft;
		mFrameTop = mViewTop - parentTop;
		if (mViewLeft >= 0 && mViewTop >= 0)
			MoveTo(mFrameLeft, mFrameTop);
	}
}

void ListView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case msg_ListViewChanged:
			mGlyph->SelectViewChanged();
			break;
		default:
			BScrollView::MessageReceived(msg);
	}
}


void ListView::GetOptimalSize(float *width, float *height)
{
	*width = 50.0;		// whatever
	*height = 50.0;		// whatever

	int32 count = mListView->CountItems();
	if (count > 0) {
		for (int32 i = 0; i < count; i++) {
			BListItem *item = mListView->ItemAt(i);
			*width = (*width >= item->Width()) ? *width : item->Width();
		}

		*width = ceil(*width) + 2.0 + B_V_SCROLL_BAR_WIDTH ;
		*height = (floor(mListView->ItemFrame(0).Height()) * mSize) + mSize + 3.0;
	}
}

void ListView::SetOptions(CLinkedList* options)
{
	mListView->MakeEmpty();	

	mOptions = options;
	if (mOptions == NULL)
		return;
		
	Option* o = (Option*)mOptions->First();
	for (o = (Option *)mOptions->First(); o; o = (Option *)o->Next()) {
		if (o->mOption.Length() > 0)
			mListView->AddItem(new BStringItem(o->mOption.String()));
		else
			mListView->AddItem(new BStringItem(""));
	}

	Reset();
}

void ListView::UpdateOption(Option *inOption)
{
	if (!mOptions)
		return;
		
	int i = 0;
	Option *o = (Option *)mOptions->First();
	while (o && o != inOption) {
		o = (Option *)o->Next();
		i++;
	}
	if (!o)
		return;
	BListItem *item = mListView->ItemAt(i);
	if (!item)
		return;
	if (o->mSelected)
		mListView->Select(i);
	else
		mListView->Deselect(i);
	BStringItem *stringItem = dynamic_cast<BStringItem *>(item);
	if (stringItem)
		stringItem->SetText(o->mOption.String());
}

//	Reset to defaults

void ListView::Reset()
{
	mListView->DeselectAll();

	if (mOptions == NULL)
		return;

	int		i = 0;
	Option 	*o;
	for (o = (Option *)mOptions->First(); o; o = (Option *)o->Next()) {
		if (o->mSelected != o->mOn)
			o->mOn = o->mSelected;
	
		if (o->mOn)
			mListView->Select(i, mMultiple);	

		i++;
	}

	mListView->ScrollToSelection();
}

bool ListView::OptionIsMarked(int i)
{
	if (mOptions == NULL)
		return false;

	Option *o = (Option *)mOptions->First();
	o = (Option *)o->At(i);
	if (o == NULL)
		return false;

	return mListView->IsItemSelected(i);
}

void ListView::SetSelectedIndex(int32 index)
{
	mListView->DeselectAll();
	mListView->Select(index);
}


// ===========================================================================

BeSelect::BeSelect(Document* htmlDoc) :
	InputGlyph(htmlDoc)
{
	mSelectView = NULL;		// Not visible yet
	mType = T_SELECT;
	mIsPopup = true;		// Default to a menu view
}

BeSelect::~BeSelect()
{
}

void BeSelect::Blur()
{
	MenuView *mv = dynamic_cast<MenuView *>(mSelectView);
	if (mv) mv->MakeFocus(false);
	ListView *lv = dynamic_cast<ListView *>(mSelectView);
	if (lv) lv->MakeFocus(false);
}

void BeSelect::Focus()
{
	MenuView *mv = dynamic_cast<MenuView *>(mSelectView);
	if (mv) mv->MakeFocus(true);
	ListView *lv = dynamic_cast<ListView *>(mSelectView);
	if (lv) lv->MakeFocus(true);
}

//	Reset button to default value

void BeSelect::Reset(DrawPort *drawPort)
{
	InputGlyph::Reset(drawPort);

	if (mSelectView)
		mSelectView->Reset();
}

//

void BeSelect::Submit(uint32 encoding, BString& submission)
{
	submission = "";
	if (mSelectView) {
		BString name;
		Encode(encoding, mName,name);
		mSelectView->Submit(encoding, name,submission, this);
	}
}

int32 BeSelect::GetNumOptions()
{
	if (mSelectView)
		return mSelectView->GetNumOptions();
	else {
		int i = 0;
		for (CLinkable* item = mOption.First(); item; item = item->Next())
			i++;
		return i;
	}
}

Option* BeSelect::OptionAt(int32 index)
{
	if (mSelectView)
		return mSelectView->OptionAt(index);
	else {
		int i = 0;
		for (CLinkable* item = mOption.First(); item; item = item->Next()) {
			if (i++ == index)
				return (Option *)item;
		}
	}
	return NULL;
}

int32 BeSelect::GetSelectedIndex()
{
	if (mSelectView)
		return mSelectView->GetSelectedIndex();
	else
		return 0;
}

void BeSelect::SetSelectedIndex(int32 index)
{
	if (mSelectView)
		mSelectView->SetSelectedIndex(index);
}

void BeSelect::UpdateOption(Option *o)
{
	if (mSelectView)
		mSelectView->UpdateOption(o);
}

//	Size the view but position it offscreen

void BeSelect::Layout(DrawPort *drawPort)
{
	if (mSelectView == NULL) {
		mIsPopup = (mSize == 1);

		BView *view = ((BeDrawPort *)drawPort)->GetView();	// Get ready to show it for the first time

		if (mIsPopup) {
			BRect r(0,0,200,24);
			r.OffsetBy(32000,0);
			const char *name = mName.String();
			if (!name || !(*name))
				name = " ";
			MenuView *menuView = new MenuView(r, mName.String(), mMultiple, this);
			view->AddChild(menuView);
			mSelectView = menuView;
		} else {
			BRect r(0,0,200,mSize*14);
			r.OffsetBy(32000,0);

			BListView *blistView = new BListView(r, "", (mMultiple) ? 
												 B_MULTIPLE_SELECTION_LIST : 
												 B_SINGLE_SELECTION_LIST,
												 B_FOLLOW_ALL);
			ListView *listView = new ListView(blistView, mSize, "ListView", mMultiple, this); 
			view->AddChild(listView);
			mSelectView = listView;
			blistView->SetTarget(listView);
			blistView->SetSelectionMessage(new BMessage(msg_ListViewChanged));
		}
		
		float height;
		float width;
		mSelectView->SetOptions(&mOption);
		mSelectView->GetOptimalSize(&width,&height);

		BView *theView = dynamic_cast<BView *>(mSelectView);
		theView->ResizeTo(width,height);
		
		SetWidth(kMarginLeft + width + kMarginRight);
		SetHeight(kMarginTop + height + kMarginBottom);
	}
}

float BeSelect::GetMinUsedWidth(DrawPort *drawPort)
{
	Layout(drawPort);
	return InputGlyph::GetMinUsedWidth(drawPort);
}

float BeSelect::GetMaxUsedWidth(DrawPort *drawPort)
{
	Layout(drawPort);
	return InputGlyph::GetMaxUsedWidth(drawPort);
}

/*
void BeSelect::Draw(DrawPort *)
{
	MenuView *mv = dynamic_cast<MenuView *>(mSelectView);
	if (mv) mv->Position();
	ListView *lv = dynamic_cast<ListView *>(mSelectView);
	if (lv) lv->Position();
}
*/

void BeSelect::SetTop(float top)
{
	InputGlyph::SetTop(top);
	MenuView *mv = dynamic_cast<MenuView *>(mSelectView);
	if (mv) {
		mv->SetTop(kMarginTop + top);
		mv->Position();
	}
	ListView *lv = dynamic_cast<ListView *>(mSelectView);
	if (lv) {
		lv->SetTop(kMarginTop + top);
		lv->Position();
	}
}


void BeSelect::SetLeft(float left)
{
	InputGlyph::SetLeft(left);
	MenuView *mv = dynamic_cast<MenuView *>(mSelectView);
	if (mv) {
		mv->SetLeft(kMarginLeft + left);
		mv->Position();
	}
	ListView *lv = dynamic_cast<ListView *>(mSelectView);
	if (lv) {
		lv->SetLeft(kMarginLeft + left);
		lv->Position();
	}
}

void BeSelect::OffsetBy(const BPoint& offset)
{
	InputGlyph::OffsetBy(offset);
	MenuView *mv = dynamic_cast<MenuView *>(mSelectView);
	if (mv) {
		mv->SetTop(kMarginLeft + mTop);
		mv->SetLeft(kMarginLeft + mLeft);
		mv->Position();
	}
	ListView *lv = dynamic_cast<ListView *>(mSelectView);
	if (lv) {
		lv->SetTop(kMarginLeft + mTop);
		lv->SetLeft(kMarginLeft + mLeft);
		lv->Position();
	}
}


InputValueItem*
BeSelect::UserValues()
{
	InputValueItem *result = NULL;

	if (mSelectView != NULL)
		result = mSelectView->UserValues(InputGlyph::UserValues());
	else
		result = InputGlyph::UserValues();

	return (result);
}

void BeSelect::SetAttributeStr(long attributeID, const char* value)
{
	if (attributeID == A_ONCHANGE) {
		mOnChangeScript = value;
#ifdef JAVASCRIPT
		if (!mHTMLDoc->LockDocAndWindow())
			return;
		mHTMLDoc->InitJavaScript();
		mHTMLDoc->UnlockDocAndWindow();
#endif
	} else
		InputGlyph::SetAttributeStr(attributeID, value);
}

#ifdef JAVASCRIPT
void BeSelect::SetContainer(jseVariable container)
{
	if (!mContainer) {
		if (!mHTMLDoc->LockDocAndWindow())
			return;
		mContainer = container;
		if (mOnChangeScript.Length())
			mOnChange = mHTMLDoc->AddHandler(container, "onChange", mOnChangeScript.String());
		else
			mOnChange = 0;
		mHTMLDoc->UnlockDocAndWindow();
	}
	InputGlyph::SetContainer(container);
}
#endif

void BeSelect::SelectViewChanged()
{
#ifdef JAVASCRIPT
	if (mOnChangeScript.Length()) {
		if (!mHTMLDoc->LockDocAndWindow())
			return;
		mHTMLDoc->ExecuteHandler(mContainer, mOnChange);
		mHTMLDoc->UnlockDocAndWindow();
	}

	InputGlyph::ValueChanged();
#endif
}

void
BeSelect::Init()
{
	InputGlyph::Init();
	if (mSelectView != NULL)
		mSelectView->Reset();
}
