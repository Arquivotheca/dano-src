#include "TimedAlert.h"
#include <MessageRunner.h>
#include <Button.h>
#include <Font.h>
#include <TextView.h>
#include <stdio.h>

#define TICK_ALERT 'taTT'

TTimedAlert::TTimedAlert(bigtime_t interval, int32 button, const char *title, const char *text, const char *button1, const char *button2, const char *button3, button_width width, alert_type type)
	: BAlert(title, text, button1 ? button1 : "bogus", button2, button3, width, type),
	mInterval(interval), mTimeOutButton(button)
{
	BButton *b;
	if(! button1 && (b = ButtonAt(0)) != 0)
	{
		float h = b->Bounds().Height();
		b->Hide();
		b->ResizeTo(0, 0);
		ResizeBy(0, -h);
	}
	txt = new char [strlen(text) + 1];
	strcpy(txt, text);
	TextView()->SetFontAndColor(be_bold_font);
}


TTimedAlert::TTimedAlert(bigtime_t interval, int32 button, const char *title, const char *text, const char *button1, const char *button2, const char *button3, button_width width, button_spacing spacing, alert_type type)
	: BAlert(title, text, button1 ? button1 : "bogus", button2, button3, width, spacing, type),
	mInterval(interval), mTimeOutButton(button)
{
	BButton *b;
	if(! button1 && (b = ButtonAt(0)) != 0)
	{
		float h = b->Bounds().Height();
		b->Hide();
		b->ResizeTo(0, 0);
		ResizeBy(0, -h);
	}
	txt = new char [strlen(text) + 1];
	strcpy(txt, text);
	TextView()->SetFontAndColor(be_bold_font);
}


TTimedAlert::~TTimedAlert()
{
	delete [] txt;
}

void TTimedAlert::Show()
{
	char	tmp[1024];
	curr = mInterval / 1000000LL;
	sprintf(tmp, txt, curr);
	TextView()->SetText(tmp);
	TextView()->SetFont(be_bold_font);
	BAlert::Show();
	new BMessageRunner(this, new BMessage(TICK_ALERT), 1000000LL);
}

void TTimedAlert::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case TICK_ALERT :
			if(--curr > 0)
			{
				char	tmp[1024];
				sprintf(tmp, txt, curr);
				TextView()->SetText(tmp);
			}
			else
			{
				BButton *b = ButtonAt(mTimeOutButton);
				if(b)
					b->Invoke();
			}
			break;

		default :
			BAlert::MessageReceived(msg);
			break;
	}
}

void TTimedAlert::Stop(int32 stopbutton)
{
	BButton *b = ButtonAt(stopbutton);
	if(b)
		b->Invoke();
}
