/*
	HColorPicker2.cpp
	
	Copyright Hekkelman Programmatuur
	
	Created: 10/10/97 11:37:52
*/

#include "HColorPicker2.h"
#include "HColorSquare.h"
#include "HColorSlider.h"
#include "HColorDemo.h"
#include "lib.h"

HColorPicker2::HColorPicker2(BRect frame, const char *name, window_type type, int flags,
			BWindow *owner, BPositionIO& data)
	: HDialog(frame, name, type, flags, owner, data)
{
	fSlider = static_cast<HColorSlider*>(FindView("v"));
	fSquare = static_cast<HColorSquare*>(FindView("hs"));
	fDemo = static_cast<HColorDemo*>(FindView("demo"));
	
	fSlider->SetValue(1.0);
	
	rgb_color c = { 255, 200, 200, 0 };
	fDemo->SetOldColor(c);
	fDemo->SetNewColor(c);
	fSquare->SetColor(c);

//	Run();
} /* HColorPicker2::HColorPicker2 */

void HColorPicker2::CreateField(int kind, BPositionIO& data, BView*& inside)
{
	dRect r;
	char name[256];
	BView *v;
	
	switch (kind)
	{
		case 'csqr':
			data >> r >> name;
			inside->AddChild(v = new HColorSquare(r.ToBe(), name));
			break;
		case 'csld':
			data >> r >> name;
			inside->AddChild(v = new HColorSlider(r.ToBe(), name, kWhite));
			break;
		case 'cdmo':
			data >> r >> name;
			inside->AddChild(v = new HColorDemo(r.ToBe(), name));
			break;
	}
} /* HColorPicker2::CreateField */

void HColorPicker2::MessageReceived(BMessage *msg)
{
	roSColor *c;
	long l;
	
	if (msg->WasDropped() && msg->FindData("roColour", 'roCr', (const void**)&c, &l) == B_NO_ERROR)
	{
		rgb_color rgb = ro2rgb(*c);

		fSquare->SetColor(rgb);
		fDemo->SetNewColor(rgb);
		
		float r, g, b, h, s, v;

		rgb2hsv(c->m_Red, c->m_Green, c->m_Blue, h, s, v);
		hsv2rgb(h, s, 1.0, r, g, b);
		
		fSlider->SetMax(f2rgb(r, g, b));
		
		return;
	}

	switch (msg->what)
	{
		case msg_SliderChanged:
			fSquare->SetValue(fSlider->Value());
			fDemo->SetNewColor(fSquare->Color());
			break;
		
		case msg_ColorSquareChanged:
		{
			rgb_color c;
			fDemo->SetNewColor(c = fSquare->Color());
			
			float r, g, b, a, h, s, v;
			rgb2f(c, r, g, b, a);
			rgb2hsv(r, g, b, h, s, v);
			hsv2rgb(h, s, 1.0, r, g, b);
			
			fSlider->SetMax(f2rgb(r, g, b));
			break;
		}
		
		default:
			HDialog::MessageReceived(msg);
	}
} /* HColorPicker::MessageReceived */

void HColorPicker2::Connect(BMessage& msg, BHandler *target)
{
	fMessage = msg;
	fTarget = BMessenger(target);
	
	const void *p;
	ssize_t s;
	
	if (msg.FindData("color", (type_code)B_RGB_COLOR_TYPE, (const void**)&p, &s) == B_OK)
	{
		rgb_color c = *(rgb_color *)p;
		
		float r, g, b, h, s, v, a;
		rgb2f(c, r, g, b, a);
		rgb2hsv(r, g, b, h, s, v);
		
		fDemo->SetOldColor(c);
		fDemo->SetNewColor(c);
		fSquare->SetColor(c);
		fSlider->SetValue(v);
		fSlider->SetMax(fSquare->Color());
		fSquare->SetValue(v);
	}
	
	Show();
} /* HColorPicker2::Connect */

bool HColorPicker2::OKClicked()
{
	rgb_color c = fSquare->Color();
	fMessage.ReplaceData("color", B_RGB_COLOR_TYPE, &c, sizeof(rgb_color));
	fTarget.SendMessage(&fMessage);
	
	return true;
} /* HColorPicker2::OKClicked */

void HColorPicker2::RegisterFields()
{
	RegisterFieldCreator('csld', CreateField);
	RegisterFieldCreator('cdmo', CreateField);
	RegisterFieldCreator('csqr', CreateField);
} /* HColorPicker2::RegisterFields */
