// ===========================================================================
//	MessageWindow.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifdef DEBUGMENU

#include <Message.h>
#include <Messenger.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextView.h>
#include <Window.h>

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define PPRINT_MSG	'pprt'

//#define PRINT_TO_STDOUT

// ===========================================================================

class MessageWindow : public BWindow {

public:
				MessageWindow(BRect frame);
virtual			~MessageWindow();
				
virtual	void	MessageReceived(BMessage *msg);
static	void	Print(const char *text);
		
		static	MessageWindow* mMessageWindow;

protected:
		bool			mNeedScroll;
		BScrollView*	mScrollView;
		BTextView*		mTextView;
		BMessenger*		mMessenger;
};

// ============================================================================

MessageWindow* MessageWindow::mMessageWindow = NULL;

void ShowMessages()
{
	if (MessageWindow::mMessageWindow == NULL) {
		BScreen screen(B_MAIN_SCREEN_ID);
		BRect rect = screen.Frame();
		rect.left = rect.right - (320);
		rect.InsetBy(0,32);
		MessageWindow::mMessageWindow = new MessageWindow(rect);
		MessageWindow::mMessageWindow->Show();
	} else
		MessageWindow::mMessageWindow->Activate();
}

void MessageWindow::Print(const char *text)
{
#ifdef PRINT_TO_STDOUT
	printf(text);
#endif
	
	if (mMessageWindow == NULL)
		return;
		
	BMessage msg(PPRINT_MSG);
	msg.AddString("text",text);
	mMessageWindow->PostMessage(&msg);
}

MessageWindow::MessageWindow(BRect frame) : BWindow(frame, "Message", B_DOCUMENT_WINDOW, 0), mNeedScroll(false)
{
	frame.OffsetTo(0,0);
	frame.right -= 14;
	frame.bottom -= 14;
	BRect textRect(frame);
	textRect.InsetBy(2,2);
	textRect.bottom = textRect.top;
	mTextView = new BTextView(frame,"Messages",textRect,B_FOLLOW_ALL,B_WILL_DRAW);
	mScrollView = new BScrollView("Message Scoller",mTextView,B_FOLLOW_ALL,0,TRUE,TRUE);
	AddChild(mScrollView);
	SetSizeLimits(256,2048,128,2048);
	mMessenger = new BMessenger(this);
	mMessageWindow = this;
}

MessageWindow::~MessageWindow()
{
	mMessageWindow = NULL;
	delete mMessenger;
}

void MessageWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case PPRINT_MSG: {
			const char *tmp = 0;
			msg->FindString("text", &tmp);
			mTextView->Insert(tmp);
			mTextView->ScrollToSelection();
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

//=======================================================================
//	Stub for sending a debug string

void pprintMessage(char* text)
{
	strcat(text,"\n");
	MessageWindow::Print(text);
}

//=======================================================================

//	Minor thing happened

void 	pprint(const char *format, ...)
{
#ifndef PRINT_TO_STDOUT
	if (MessageWindow::mMessageWindow == NULL)
		return;
#endif

	char *dst = (char *)malloc(4096);
	*dst = 0;
	va_list	v;

	va_start(v,format);
	vsprintf(dst,format,v);
	pprintMessage(dst);
	free(dst);
}

//	Major thing happened

void 	pprintBig(const char *format, ...)
{
	if (MessageWindow::mMessageWindow == NULL)
		return;

	char *dst = (char *)malloc(4096);
	*dst = 0;
	va_list	v;

	va_start(v,format);
	vsprintf(dst,format,v);
	pprintMessage(dst);
	free(dst);
}

void pprintHex(const void *data, long count)
{
	if (MessageWindow::mMessageWindow == NULL)
		return;

	unsigned short *d = (unsigned short *)data;
	long addr = 0;
	char s[80];
	short x;
	
	//pprintMono(true);
	while (count > 0) {
		char str[20];
		str[16] = 0;
		memcpy(str,d,16);
		for (x = 0; x < 16; x++)
			if (str[x] < 32)
				str[x] = '.';
		if (count < 16)
			str[count] = 0;

		sprintf(s,"%8lX..%4X.%4X.%4X.%4X..%4X.%4X.%4X.%4X %s",addr,d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],str);
		for (x = 0; x < 50; x++) {
			if (s[x] == ' ')
				s[x] = '0';
			else if (s[x] == '.')
				s[x] = ' ';
		}

//		Clip line if count < 16

		if (count < 16) {
			str[count] = 0;
			short clip = (count >> 1) * 5;
			clip += count < 8 ? 10 : 11;
			if (count & 1)
				clip -= 2;
			for (x = clip; x < 50; x++)
				s[x] = ' ';
		}
		
		pprint(s);
		d += 8;
		addr += 16;
		count -= 16;
	}
	//pprintMono(true);
}

#endif // #ifdef DEBUGMENU
// ============================================================================
