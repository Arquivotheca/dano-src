/*
	HDialog.cpp
	
	Copyright 1997, Hekkelman Programmatuur
	
	First build at september 5, 1997
	
	
*/

#include "HDialog.h"
#include "HTabSheet.h"
#include "HColorControl.h"
#include "HDlogView.h"
#include "lib.h"

#include <Alert.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Font.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <Region.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Slider.h>
#include <StringView.h>
#include <TextControl.h>
#include <Debug.h>

#include <ctype.h>
#include <map>
#include <string>
#include <typeinfo>
using std::map;
using std::max;

const unsigned long
	msg_AddDialog		= 'ADlg',
	msg_RemoveDialog	= 'RDlg';

typedef map<int, FieldCreator> field_map;

static field_map *sFieldMap = NULL;

float gFactor = 0.0;

BRect dRect::ToBe()
{
	if (gFactor == 0.0)
		gFactor = be_plain_font->Size() / 10.0;

	BRect r;
	r.left = left * gFactor;
	r.top = top * gFactor;
	r.right = right * gFactor;
	r.bottom = bottom * gFactor;
	return r;
} /* BRect::operator */

//#pragma mark -
//#pragma mark --- HDialog ---

HDialog::HDialog(BRect frame, const char *name, window_type type,
	int flags, BWindow *owner, BPositionIO& data)
	: BWindow(frame, name, type, flags)
{
	fOwner = owner;
	
	frame.OffsetTo(0, 0);

	AddChild(fMainView = new HDlogView(frame, "main"));
	
	BuildIt(data);
	
	if (fOwner)
	{
		BMessage m(msg_AddDialog);
		m.AddPointer("dialog", this);
		fOwner->PostMessage(&m);
	}
} /* HDialog::HDialog */

HDialog::~HDialog()
{
	if (fOwner)
	{
		BMessage m(msg_RemoveDialog);
		m.AddPointer("dialog", this);
		fOwner->PostMessage(&m);
	}
} /* HDialog::~HDialog */

bool HDialog::IsOn(const char *name) const
{
	BControl *ctrl;
	ctrl = dynamic_cast<BControl *>(FindView(name));
	if (ctrl)
		return ctrl->Value() == B_CONTROL_ON;
	else
		return false;
} /* HDialog::IsOn */

void HDialog::SetOn(const char *name, bool on)
{
	BControl *ctrl;
	ctrl = dynamic_cast<BControl *>(FindView(name));
	if (ctrl)
		ctrl->SetValue(on ? B_CONTROL_ON : B_CONTROL_OFF);
} /* HDialog::SetOn */
	
void HDialog::MessageReceived(BMessage *inMessage)
{
	switch (inMessage->what)
	{
		case msg_OK:
			if (OKClicked())
				Close();
			break;
		
		case msg_FieldChanged:
			UpdateFields();
			break;
			
		case msg_Cancel:
			if (CancelClicked())
				Close();
			break;
			
		default:
			BWindow::MessageReceived(inMessage);
	}
} /* HDialog::MessageReceived */

bool HDialog::OKClicked()
{
	return true;
} /* HDialog::OKClicked */

bool HDialog::CancelClicked()
{
	return true;
} /* HDialog::CancelClicked */

void HDialog::UpdateFields()
{
//	beep();
} /* HDialog::UpdateFields */

void HDialog::SetText(const char *name, const char *text)
{
	BView *c = FindView(name);
	if (!c)
	{
		ASSERT(false);
		return;
	}

	BTextControl *tc = dynamic_cast<BTextControl*>(c);

	if (tc)
	{
		tc->SetText(text);
		if (tc->IsFocus())
			tc->TextView()->SelectAll();
	}
	else if (typeid(*c) == typeid(BStringView))
		static_cast<BStringView *>(c)->SetText(text);

} /* HDialog::SetText */

const char* HDialog::GetText(const char *name) const
{
	BTextControl *t = dynamic_cast<BTextControl *>(FindView(name));
	if (t)
		return t->Text();
	else
		return NULL;
} /* HDialog::SetText */

void HDialog::SetEnabled(const char *name, bool enabled)
{
	BControl *ctl;
	ctl = dynamic_cast<BControl*>(FindView(name));
	if (ctl)
		ctl->SetEnabled(enabled);
} /* HDialog::SetEnabled */

bool HDialog::GetDouble(const char *name, double& d) const
{
	d = atof(GetText(name));
	
	if (isnan(d))
	{
		(new BAlert("", "Invalid number entered", "OK"))->Go();
		FindView(name)->MakeFocus(true);
		return false;
	}

	return true;
} /* HDialog::GetDouble */

void HDialog::CreateField(int kind, BPositionIO& data, BView*& inside)
{
	dRect r;
	char name[256];
	char label[256];
	ulong cmd;
	BView *v;
	
	switch (kind)
	{
		case 'btn ':
			data >> r >> name >> label >> cmd;
			inside->AddChild(v = new BButton(r.ToBe(), name, label, new BMessage(cmd)));
			if (cmd == msg_OK || strcmp(name, "ok") == 0)
				SetDefaultButton(static_cast<BButton*>(v));
			break;
		case 'radb':
			data >> r >> name >> label;
			inside->AddChild(new BRadioButton(r.ToBe(), name, label, new BMessage(msg_FieldChanged)));
			break;
		case 'chkb':
			data >> r >> name >> label;
			inside->AddChild(new BCheckBox(r.ToBe(), name, label, new BMessage(msg_FieldChanged)));
			break;
		case 'edit':
		{
			char val[256], allowed[256];
			short max, width;
			data >> r >> name >> label >> val >> allowed >> max >> width;
			
			BRect b = r.ToBe();
			
			inside->AddChild(v = new BTextControl(b, name, *label ? label : NULL,
				val, new BMessage(msg_FieldChanged)));
					
			BTextView *tv = static_cast<BTextControl*>(v)->TextView();
			if (*allowed)
			{
				for (int i = 0; i < 256; i++)
					if (isprint(i))
					{
						if (strchr(allowed, i)) tv->AllowChar(i);
						else tv->DisallowChar(i);
					}
			}

			if (max) tv->SetMaxBytes(max);
			if (width) static_cast<BTextControl*>(v)->SetDivider(width * gFactor);
			
			if (v->Bounds().Height() < b.Height())
			{
				float d = v->Bounds().Height() - tv->Bounds().Height();
				v->ResizeTo(b.Width(), b.Height());
				tv->ResizeTo(tv->Bounds().Width(), b.Height() - d);
			}
			break;
		}
		case 'capt':
			data >> r >> name >> label;
			inside->AddChild(v = new BStringView(r.ToBe(), name, label));
			v->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
			v->SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
			break;
		case 'popu':
		{
			short id, div;
			data >> r >> name >> label >> id >> div;
			inside->AddChild(v = new BMenuField(r.ToBe(), name, *label ? label : NULL,
				HResources::GetMenu(id, true)));
			if (div) static_cast<BMenuField*>(v)->SetDivider(div * gFactor);
			break;
		}
		case 'tabb':
			data >> r >> name;
			inside->AddChild(v = new HTabSheet(r.ToBe(), name));
			inside = v;
			break;
		case 'tabe':
			inside = inside->Parent();
			break;
		case 'shet':
			data >> name >> label;
			inside = dynamic_cast<HTabSheet*>(inside)->AddSheet(name, label);
			break;
		case 'shte':
			inside = inside->Parent();
			break;
		case 'box ':
			data >> r >> name;
			inside->AddChild(v = new BBox(r.ToBe(), name));
			if (*name) dynamic_cast<BBox*>(v)->SetLabel(name);
			v->SetFont(be_plain_font);
			inside = v;
			break;
		case 'boxe':
			inside = inside->Parent();
			break;
		case 'list':
		case 'olst':
		{
			data >> r >> name;
			
			BRect lr = r.ToBe();
			lr.right -= B_V_SCROLL_BAR_WIDTH;
			
			BListView *lv;
			if (kind == 'list')
				lv = new BListView(lr, name);
			else
				lv = new BOutlineListView(lr, name);
			strcat(name, "_scr");
			inside->AddChild(new BScrollView(name, lv, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));
			break;
		}
		case 'clct':
		{
			data >> r >> name >> label;
			rgb_color c = { 255, 100, 100, 0 };
			inside->AddChild(v = new HColorControl(r.ToBe(), name, label, c));
			v->SetViewColor(inside->ViewColor());
			break;
		}
		case 'line':
		{
			HDlogView *dv = dynamic_cast<HDlogView*>(inside);
			
			data >> r;
			
			if (dv)
				dv->AddMyLine(r.ToBe());
			break;
		}
		case 'sldr':
		{
			int32 msg, vMin, vMax, thumb;
			data >> r >> name >> label >> msg >> vMin >> vMax >> thumb;
			inside->AddChild(new BSlider(r.ToBe(), name, label, new BMessage(msg), vMin, vMax, (thumb_style)thumb));
			break;
		}
		default:
		{
			if (sFieldMap && sFieldMap->find(kind) != sFieldMap->end())
				(*sFieldMap)[kind](kind, data, inside);
			else
				throw HErr("Unknown type for dialog item (%4.4s)", &kind);
		}
	}
} /* HDialog::CreateField */

void HDialog::BuildIt(BPositionIO& data)
{
	int kind, cnt;
	BView *contains = fMainView;
	
	if (gFactor == 0.0)
		gFactor = be_plain_font->Size() / 10.0;

	data >> cnt;
	while (cnt--)
	{
		data >> kind;
		CreateField(kind, data, contains);
	}
} /* HDialog::BuildIt */

void HDialog::RegisterFieldCreator(int kind, FieldCreator fieldCreator)
{
	if (!sFieldMap) sFieldMap = new field_map;
	(*sFieldMap)[kind] = fieldCreator;
} /* HDialog::RegisterFieldCreator */

void HDialog::RegisterFields()
{
} /* HDialog::RegisterFields */

BRect GetPosition(dRect dr, BWindow *owner)
{
	BRect frame = dr.ToBe();
	
	if (owner)
	{
		BRect r = owner->Frame();
		frame.OffsetTo(r.left + (r.Width() - frame.Width()) / 2,
			r.top + (r.Height() - frame.Height()) / 3);
	}
	else
	{
		BScreen s;
		frame.OffsetBy((s.Frame().Width() - frame.Width()) / 2,
			(s.Frame().Height() - frame.Height()) / 3);
	}
	
	return frame;
} /* GetPosition */

int HDialog::GetValue(const char *id) const
{
	BView *v = FindView(id);
	if (v == NULL) THROW(("View '%s' not found", id));
	
		// according to stroustrup I shouldn't do this:

	if (typeid(*v) == typeid(BMenuField))
	{
		BMenu *menu = static_cast<BMenuField*>(v)->Menu();
		return max(menu->IndexOf(menu->FindMarked()) + 1, 1L);
	}
	else if (typeid(*v) == typeid(BTextControl))
		return atoi(GetText(id));
	else if (typeid(*v) == typeid(BSlider))
		return static_cast<BSlider*>(v)->Value();
	
	THROW(("view '%s' not of valid type", id));
} // HDialog::GetValue

void HDialog::SetValue(const char *id, int value)
{
	BView *v = FindView(id);
	if (v == NULL) THROW(("View '%s' not found", id));
	
	if (typeid(*v) == typeid(BMenuField))
	{
		BMenuField *mf = static_cast<BMenuField*>(v);
		BMenuItem *item = mf->Menu()->ItemAt(value - 1);
		if (item)
			item->SetMarked(true);
		return;
	}
	else if (typeid(*v) == typeid(BTextControl))
	{
		char b[32];
		sprintf(b, "%d", value);
		SetText(id, b);
		return;
	}
	else if (typeid(*v) == typeid(BSlider))
	{
		static_cast<BSlider*>(v)->SetValue(value);
		return;
	}
	
	THROW(("view '%s' not found", id));
} // HDialog::SetValue

void HDialog::SetLabel(const char *id, const char *label)
{
	BControl *control = dynamic_cast<BControl*>(FindView(id));
	if (control)
		control->SetLabel(label);
	else
		THROW(("Control '%s' not found", id));
} // HDialog::SetLabel
