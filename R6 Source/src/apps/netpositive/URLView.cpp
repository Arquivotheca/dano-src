// ===========================================================================
//	URLView.cpp
//  Copyright 1998 by Be Incorporated.
// 	Coypright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "URLView.h"
#include "MessageWindow.h"
#include "HTMLDoc.h"
#include "NPApp.h"
#include "Translation.h"
#include "NetPositive.h"

#include <Clipboard.h>
#include <MessageFilter.h>
#include <Window.h>
#include <Bitmap.h>
#include <Autolock.h>
#include <Button.h>
#include <stdio.h>

extern const char *kURLViewLabel;
extern const char *kApplicationSig;
extern bool ReadLine(BPositionIO &io, off_t &position, BString& string);
extern bool GetNextToken(BString &line, BString& token);
const float kURLInputLeftEdge = 26.0;

class GIFButton : public BButton {
public:
						GIFButton(BRect rect, Animation *up, Animation *down, Animation *disabled, BMessage *msg);
	virtual				~GIFButton();
	
	virtual void		Draw(BRect updateRect);
	virtual void		Pulse();
	virtual void		SetValue(int32 value);
	virtual void		SetEnabled(bool on);
	bool				Animated();
	void				SetAnimated(bool animate) {mAnimate = animate;}
	
	Animation *mUp;
	Animation *mDown;
	Animation *mDisabled;
	bool mAnimate;
	uint32 mFrame;
	bigtime_t mLastTime;
};

GIFButton::GIFButton(BRect rect, Animation *up, Animation *down, Animation *disabled, BMessage *msg)
	: BButton(rect, "", "", msg, B_FOLLOW_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE)
{
	ResizeTo(rect.Width(), rect.Height());
	mUp = up;
	mDown = down;
	mDisabled = disabled;
	mFrame = 0;
	mLastTime = 0;
	mAnimate = true; //animate when unless SetAnimate(true) is called
}


GIFButton::~GIFButton()
{
	delete mUp;
	delete mDown;
	delete mDisabled;
}

bool GIFButton::Animated()
{
	return (mAnimate && (mUp->NumFrames() > 1 || mDown->NumFrames() > 1 || mDisabled->NumFrames() > 1));
}

void GIFButton::Pulse()
{
	Animation *animation;
	if (!IsEnabled())
		animation = mDisabled;
	else if (Value())
		animation = mDown;
	else
		animation = mUp;

	//set animation to frame 0 and return if we aren't set to animate after all
	if(mAnimate == false){
		mFrame = 0;
		return;
	}
	
	uint32 numFrames = animation->NumFrames();
	bigtime_t now = system_time();
	if (numFrames > 1 && now > (mLastTime + animation->GetDelay(mFrame == 0 ? numFrames - 1 : mFrame - 1))) {
		mFrame++;
		if (mFrame + 1 > numFrames)
			mFrame = 0;
		mLastTime = now;
		Invalidate();
	}
}

void GIFButton::Draw(BRect updateRect)
{
	BBitmap *bitmap;
	if (!IsEnabled())
		bitmap = mDisabled->GetFrame(mFrame);
	else if (Value())
		bitmap = mDown->GetFrame(mFrame);
	else
		bitmap = mUp->GetFrame(mFrame);
		
	if (bitmap)
		DrawBitmap(bitmap);
		
	if (IsFocus() && Window()->IsActive()) {
		BRect rect = Bounds();
		rect.InsetBy(2,2);
		SetHighColor(keyboard_navigation_color());
		StrokeRect(rect);
		SetHighColor(0,0,0);
	}
}

void GIFButton::SetValue(int32 value)
{
	mFrame = 0;
	mLastTime = system_time();
	BButton::SetValue(value);
}

void GIFButton::SetEnabled(bool on)
{
	mFrame = 0;
	mLastTime = system_time();
	BButton::SetEnabled(on);
}

static int32 Animate(void *args)
{
	BView *view = (BView *)args;
	if (!view)
		return 0;
	
	while (true) {
		{
			BAutolock lock(view->Window());
			for (int i = 0; i < view->CountChildren(); i++) {
				view->ChildAt(i)->Pulse();
			}
		}
		snooze(100000);
	}
	
	return 0;
}

//=======================================================================
//	URL edit text at top of window

class URLTextControl : public BTextControl {
public:
						URLTextControl(BRect rect, const char *name);

virtual status_t		Invoke(BMessage *msg);
virtual	void			MessageReceived(BMessage *msg);
virtual	void			KeyDown(const char *bytes, int32 numBytes);
virtual void			AttachedToWindow();
virtual void			Draw(BRect updateRect);

private:
	void				InitData();
	bool				mUserModified;
};


class TEnterFilter : public BMessageFilter {
public:
						TEnterFilter(BTextControl *tc);
virtual	filter_result	Filter(BMessage *message, BHandler **target);
private:
	BTextControl*	fTextControl;
};

TEnterFilter::TEnterFilter(BTextControl	*tc) : BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
{
	fTextControl = tc;
}


filter_result TEnterFilter::Filter(BMessage	*message, BHandler	**)
{
	switch (message->what) {
		case B_KEY_DOWN: {
			int8 theByte = '\0';
			if (message->FindInt8("byte", &theByte) == B_NO_ERROR) {
				if (theByte == '\n') {
					fTextControl->Invoke();
					return (B_SKIP_MESSAGE);
				}
				
				if ( theByte == B_ESCAPE || theByte == B_FUNCTION_KEY || theByte == B_INSERT)
					return (B_SKIP_MESSAGE);
			}
			break;
		}
		case B_PASTE: { //sneak into clipboard and remove \n and \r from paste
			const char *text = NULL;
			int32 textLen = 0;
			BMessage *clip = NULL;
			if(be_clipboard->Lock()){
				if((clip = be_clipboard->Data()))
					clip->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &textLen);
				if(textLen > 0 && text){
					BString changedText;
					changedText.Append(text, textLen);
					changedText.RemoveSet("\r\n");
					be_clipboard->Clear();
					clip = be_clipboard->Data();
					clip->AddData("text/plain", B_MIME_TYPE, changedText.String(), changedText.Length());
					be_clipboard->Commit();
				}
				be_clipboard->Unlock();
			}
			return (B_DISPATCH_MESSAGE);
		}			
		default:
			break;
	}

	return (B_DISPATCH_MESSAGE);
}


//=======================================================================
//	URL edit text at top of window

URLTextControl::URLTextControl(
	BRect		rect,
	const char	*name)
		: BTextControl(rect, name, "", "", NULL, B_FOLLOW_ALL)
{
	InitData();
}

status_t
URLTextControl::Invoke(
	BMessage	*msg)
{
	BWindow		*window = Window();
	BMessage	*curMessage = window->CurrentMessage();
	if (curMessage == NULL)
		return (B_NO_ERROR);

	int8 theByte = '\0';
	if (curMessage->FindInt8("byte", &theByte) != B_NO_ERROR)
		return (B_NO_ERROR);

	if (theByte == '\n') {
		TextView()->SelectAll();
		BMessage message(HTML_MSG + HM_ANCHOR);
		message.AddString("url", Text());
		if (!window->Lock())
			return B_ERROR;
		window->PostMessage(&message, window->FindView("NPBaseView"));
		window->Unlock();
	}

	return BTextControl::Invoke(msg);
}

void 
URLTextControl::MessageReceived(
	BMessage	*msg)
{
	switch (msg->what) {
		case HTML_MSG:
			if (!mUserModified) {
				const char *newStr = msg->FindString("url");
				bool select = msg->FindBool("select");
				SetText(newStr);
				if (select) {
	//				MakeFocus();
					TextView()->SelectAll();
				}
			} else
				mUserModified = false;
			break;
	
		default:
			BTextControl::MessageReceived(msg);
			break;
	}
}

void URLTextControl::KeyDown(const char *bytes, int32 numBytes)
{
	mUserModified = true;
	BTextControl::KeyDown(bytes, numBytes);
}

void
URLTextControl::InitData()
{
	BTextView *tv = TextView();

	tv->SetMaxBytes(1024);
	tv->DisallowChar('\n');
	tv->AddFilter(new TEnterFilter(this));
	
	// Label will probably be B_EMPTY_STRING, which sets
	// this divider to 0. At least the support is here to
	// give the control a name...
	SetDivider(StringWidth(Label()));
	mUserModified = false;
}

void URLTextControl::AttachedToWindow()
{
	BTextControl::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_32_BIT);
	int32 bgColor = gPreferences.FindInt32("ToolbarBGColor");
	SetLowColor((bgColor & 0xff0000) >> 16, (bgColor & 0xff00) >> 8, bgColor & 0xff);
	if (gPreferences.FindBool("ShowTitleInToolbar")) {
		TextView()->MakeEditable(false);
		TextView()->MakeSelectable(false);
	}
}

void URLTextControl::Draw(BRect updateRect)
{
	FillRect(Bounds(), B_SOLID_LOW);
	BTextControl::Draw(updateRect);
}

//	Only paste one line

/*
void URLTextView::Paste(BClipboard *clip)
{
	clip->Lock();
	BMessage* m = clip->Data();

	void* data;
	int32 numBytes;
	if (m->FindData("text/plain",B_MIME_TYPE,&data,&numBytes) == B_NO_ERROR) {
		if (data) {
			int i;
			const char *text = (const char *)data;
			for (i = 0; i < numBytes; i++)
				if (text[i] == 0xD || text[i] == 0xA)				// Check for line feeds
					break;
			if (i != numBytes) {
				CString str;
				str.Set(text,i);
				m->RemoveData("text/plain");
				m->AddData("text/plain",B_MIME_TYPE,(const char*)str,i);	//	Clip text at line feed
			}
		}
	}
	clip->Unlock();

	BTextControl::Paste(clip);
}
*/

//=======================================================================
//	View to draw at the top of the window

URLView::URLView(
	BRect		frame, 
	BHandler	*target,
	int			resizeFlags) 
		: BView(frame, "URLView", resizeFlags, 
				B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP),
		  mTarget(target)
{
	mAnimationThread = 0;
	mPageIcon = 0;
	mBGBitmap = 0;
}

void URLView::AttachedToWindow()
{
	BView::AttachedToWindow();
	InitData();
	int32 bgColor = gPreferences.FindInt32("ToolbarBGColor");
	SetLowColor((bgColor & 0xff0000) >> 16, (bgColor & 0xff00) >> 8, bgColor & 0xff);
	mPageIcon = TranslateGIF("netpositive:HTML.gif");
	mBGBitmap = TranslateGIF("netpositive:URLBG.gif");
	for (int i = 0; i < CountChildren(); i++) {
		BButton *button = dynamic_cast<BButton *>(ChildAt(i));
		if (button) {
			button->SetTarget(mTarget);
		}
	}
}

void URLView::DetachedFromWindow()
{
	if (mAnimationThread) {
		kill_thread(mAnimationThread);
		NetPositive::RemoveThread(mAnimationThread);
		mAnimationThread = 0;
	}
}

void
URLView::SetDownloading(bool downloading)
{
	for(int i=0; i < CountChildren(); ++i){
		GIFButton *gifButton = dynamic_cast<GIFButton*>(ChildAt(i));
		if(gifButton){ //this assumes that the download button is the first button in the list
			gifButton->SetAnimated(downloading);
			break;
		}
	}
}

void
URLView::Draw(
	BRect)
{
	const rgb_color	darkG = {190, 190, 190, 0};
	const rgb_color	white = {255, 255, 255, 0};	
	BRect bounds = Bounds();

	bounds.InsetBy(1.0, 1.0);
	FillRect(bounds, B_SOLID_LOW);
	if(mBGBitmap){
		DrawBitmap(mBGBitmap, Bounds());
	}

	if (mPageIcon)
	{
		SetDrawingMode(B_OP_OVER);
		DrawBitmapAsync(mPageIcon, BPoint(7,6));
	}
}

void URLView::MouseDown(BPoint where)
{
	if (BRect(5,5,21,21).Contains(where) && mPageIcon)
		NetPositive::DragLink(mURLText->Text(), BRect(5,5,21,21), this, mPageIcon, where - BPoint(5,5));
}

void URLView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case B_COPY_TARGET:
			NetPositive::HandleCopyTarget(msg);
			break;
		case CLEAR:{
			BTextView *textView = mURLText->TextView();
			if(textView)
				textView->Clear();
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
URLView::SetButtonEnabled(
	uint32	button_msg,
	bool	enable)
{
	BAutolock lock(Window());
		
	for (int i = 0; i < CountChildren(); i++) {
		BButton *button = dynamic_cast<BButton *>(ChildAt(i));
		if (button && button->Message()->what == button_msg)
			button->SetEnabled(enable);
	}
}


void
URLView::InitData()
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	BRect bounds = Bounds();
	
	const float hSpacing = 5.0;
	
	float right = bounds.right - hSpacing;
	
	ResourceIO res("netpositive:toolbar");
	off_t position = 0;
	BString line;
	while (ReadLine(res, position, line)) {
		BString command;
		BString	enabled;
		BString url;
		BString up;
		BString down;
		BString disabled;
		GetNextToken(line, command);
		GetNextToken(line, enabled);
		GetNextToken(line, url);
		GetNextToken(line, up);
		GetNextToken(line, down);
		GetNextToken(line, disabled);
		
		uint32 commandConstant;
		if (command.Length() < 4)
			commandConstant = 0;
		else {
			commandConstant = (command[0] << 24) + (command[1] << 16) + (command[2] << 8) + command[3];
		}

		const char *str = gPreferences.FindString("DefaultURL");
		if (commandConstant == NP_HOMEBUTTON && (!str || !*str))
			continue;
		str = gPreferences.FindString("SearchURL");
		if (commandConstant == NP_SEARCHBUTTON && (!str || !*str))
			continue;
	
		AddButton(commandConstant, url.String(), &right, hSpacing, enabled.ICompare("enabled") == 0, up.String(), down.String(), disabled.String());
	}
	
	BRect r = Bounds();
	r.left += kURLInputLeftEdge;
	r.top = 0.0;
	r.bottom = r.top;
	r.right = right;
	mURLText = new URLTextControl(r, "url");
	AddChild(mURLText);
	mURLText->MoveTo(r.left, ceil((bounds.Height() - mURLText->Frame().Height()) / 2.0));

	mLastWidth = Frame().Width();
}

void URLView::AddButton(uint32 msgConst, const char *url, float *right, float hSpacing, bool enabled, const char *upImg, const char *downImg, const char *disabledImg)
{
	ResourceIO upIO(upImg);
	Animation *upAnimation = new Animation(&upIO);
	
	ResourceIO downIO(downImg);
	Animation *downAnimation = new Animation(&downIO);
	
	ResourceIO disabledIO(disabledImg);
	Animation *disabledAnimation = new Animation(&disabledIO);
	
	
	BRect rect = Bounds();
	if (upAnimation->NumFrames()) {
		BBitmap *bmp = upAnimation->GetFrame(0);
		rect.top = (rect.Height() - bmp->Bounds().Height()) / 2;
		rect.bottom = rect.top + bmp->Bounds().Height() - 1;
		rect.right = *right;
		rect.left = *right - bmp->Bounds().Width() + 1;
	}
	*right -= rect.Width() + hSpacing;
	
	BMessage *msg = new BMessage(msgConst);
	if (url && *url)
		msg->AddString("be:url", url);
	if (msgConst == B_NETPOSITIVE_OPEN_URL)
		msg->AddString("Referrer", "__NetPositive_toolbar");

	GIFButton *button = new GIFButton(rect, upAnimation, downAnimation, disabledAnimation, msg);
	
	if (button->Animated() && mAnimationThread == 0) {
		// Don't rely on the window's pulse mechanism.  We could be a replicant.
		mAnimationThread = spawn_thread(Animate, "URLView animation", B_LOW_PRIORITY, this);
		NetPositive::AddThread(mAnimationThread);
		resume_thread(mAnimationThread);
	}
	
	button->SetEnabled(enabled);
	AddChild(button);
	button->SetViewColor(B_TRANSPARENT_32_BIT);
	if(msgConst == SHOW_DOWNLOADS)
		button->SetAnimated(false);
}

