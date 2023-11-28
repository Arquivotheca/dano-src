
#include "NumControl.h"

#include <stdio.h>

NumControl::NumControl(BRect rect, const char *name, BMessage *msg, int32 startNum, int32 maxNum, int32 minNum) :
	BControl(rect, name, NULL, msg, B_FOLLOW_NONE, B_WILL_DRAW),
	fMax(maxNum),
	fMin(minNum),
	fPageNum(NULL)
{
	SetValue(startNum);
	char buf[16];
	sprintf(buf, "%ld", Value());
	fPageNum = new BTextControl(BRect(17, 2, 43, 15), "PageNum", NULL, buf, new BMessage('nval'));
	fPageNum->SetDivider(0);
	AddChild(fPageNum);
}


NumControl::~NumControl()
{
}

void 
NumControl::AttachedToWindow()
{
	fPageNum->SetTarget(this);
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	SetLowColor(ViewColor());
}

void 
NumControl::SetValue(int32 value)
{
	if (value < fMin) value = fMin;
	else if (value > fMax) value = fMax;
	
	char buf[16];
	sprintf(buf, "%ld", value);
	if (fPageNum) fPageNum->SetText(buf);

	if (Value() == value) return;
	
	BControl::SetValue(value);
	
	BMessage msg(*Message());
	msg.AddInt32("value", Value());
	Invoke(&msg);
}

void 
NumControl::SetLimits(int32 minimum, int32 maximum)
{
	fMin = minimum; fMax = maximum;
}

void 
NumControl::Draw(BRect updateRect)
{
	FillRect(Bounds(), B_SOLID_LOW);
	
	char buf[16];
	if (fMin != 1) {
		sprintf(buf, "%ld", fMin);
		DrawString(buf, BPoint(5, 14));
	}	
	BPoint pt1(10, 10);
	BPoint pt2(15, 5);
	BPoint pt3(15, 15);
	
	FillTriangle(pt1, pt2, pt3);
	
	pt1.Set(50, 10);
	pt2.Set(45, 5);
	pt3.Set(45, 15);
	
	FillTriangle(pt1, pt2, pt3);

	sprintf(buf, "%ld", fMax);
	DrawString(buf, BPoint(55, 14));
}

void 
NumControl::MouseDown(BPoint where)
{
	BRect rect(9, 4, 16, 16);
	if (rect.Contains(where)) {
		int32 val = Value();
		SetValue(--val);
	}

	rect.Set(44, 4, 51, 16);
	if (rect.Contains(where)) {
		int32 val = Value();
		SetValue(++val);	
	}
	
//	rect.Set(20, 4, 40, 16);
//	if (rect.Contains(where)) {
//		char buf[16];
//		sprintf(buf, "%ld", Value());
//		BTextControl * ctl = new BTextControl(rect, "control");
//	}

}

void 
NumControl::KeyDown(const char *bytes, int32 numBytes)
{
}

void 
NumControl::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case 'nval': {
			// get the text from the page
			int32 val;
			sscanf(fPageNum->Text(), "%ld", &val);
			SetValue(val);
			break;
		}
		default :
			BControl::MessageReceived(msg);
	
	}
}


