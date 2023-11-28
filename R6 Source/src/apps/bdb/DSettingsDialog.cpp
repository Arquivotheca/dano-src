/*	$Id: DSettingsDialog.cpp,v 1.3 1999/05/03 13:09:52 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 02/04/99 13:36:13
*/

#include "bdb.h"
#include <Slider.h>
#include "DSettingsDialog.h"

#include <MenuField.h>
#include <MenuItem.h>

DSettingsDialog* DSettingsDialog::sInstance = NULL;

void DSettingsDialog::Display()
{
	if (sInstance)
		sInstance->Activate();
	else
		sInstance = DialogCreator<DSettingsDialog>::CreateDialog(NULL);
} // DSettingsDialog::Display

DSettingsDialog::DSettingsDialog(BRect frame, const char *name, window_type type, int flags,
			BWindow *owner, BPositionIO& data)
	: HDialog(frame, name, type, flags, owner, data)
{
	BSlider *slider = dynamic_cast<BSlider*>(FindView("cache block count"));
	slider->SetLimitLabels("min", "max");

	slider = dynamic_cast<BSlider*>(FindView("malloc debug"));
	slider->SetLimitLabels("0", "10");

	font_family ff;
	font_style fs;
	be_fixed_font->GetFamilyAndStyle(&ff, &fs);

	strcpy(fFontFamily, gPrefs->GetPrefString("font family", ff));
	strcpy(fFontStyle, gPrefs->GetPrefString("font style", fs));
	fFontSize = gPrefs->GetPrefDouble("font size", be_fixed_font->Size());

	BMenuField *mf = dynamic_cast<BMenuField*>(FindView("font family"));
	FailNil(mf);
	
	fFont = mf->Menu();
	FailNil(fFont);

	for (int i = 0; i < count_font_families(); i++)
	{
		get_font_family(i, &ff);
		BMenu *fontItem = new BMenu(ff);
		FailNil(fontItem);
		fFont->AddItem(new BMenuItem(fontItem, new BMessage(msg_FieldChanged)));
		fontItem->SetFont(be_plain_font);
		
		for (int j = 0; j < count_font_styles(ff); j++)
		{
			get_font_style(ff, j, &fs);
			
			BMessage *msg = new BMessage(msg_FieldChanged);
			msg->AddString("family", ff);
			msg->AddString("style", fs);
			fontItem->AddItem(new BMenuItem(fs, msg));
		}
	}
	
	fFont->SetRadioMode(true);
	
	CancelClicked();
	UpdateFields();
	
	Show();
} // DSettingsDialog::DSettingsDialog

DSettingsDialog::~DSettingsDialog()
{
	sInstance = NULL;
} // DSettingsDialog::~DSettingsDialog

bool DSettingsDialog::OKClicked()
{
	BMenuItem *item = fFont->FindMarked();
	if (item)
	{
		strcpy(fFontFamily, item->Label());
		item = item->Submenu()->FindMarked();
		if (item) strcpy(fFontStyle, item->Label());
	}
	
	fFontSize = GetValue("font size");

	gPrefs->SetPrefInt("use cache", IsOn("use cache"));
	gPrefs->SetPrefInt("cache block count", GetValue("cache block count"));

	gPrefs->SetPrefInt("malloc debug", GetValue("malloc debug"));
	gPrefs->SetPrefInt("leak check", IsOn("leak check"));
	
	gPrefs->SetPrefInt("stop at main", IsOn("stop at main"));
	gPrefs->SetPrefInt("don't stop for unwinding", IsOn("don't stop for unwinding"));
	gPrefs->SetPrefInt("output to window", IsOn("output to window"));
	
	gPrefs->SetPrefString("font family", fFontFamily);
	gPrefs->SetPrefString("font style", fFontStyle);
	gPrefs->SetPrefDouble("font size", fFontSize);

	gPrefs->SetPrefDouble("font size", fFontSize);
	
	gPrefs->SetPrefInt("array upper bound", GetValue("array upper bound"));
	
	gPrefs->SetPrefInt("at&t style disasm", IsOn("at&t style disasm"));

	return false;
} // DSettingsDialog::OKClicked

bool DSettingsDialog::CancelClicked()
{
	BMenuItem *item = fFont->FindMarked();
	if (item)
	{
		item = item->Submenu()->FindMarked();
		if (item) item->SetMarked(false);
		fFont->FindMarked()->SetMarked(false);
	}
	
	item = fFont->FindItem(fFontFamily);
	if (item)
	{
		item->SetMarked(true);
		item = item->Submenu()->FindItem(fFontStyle);
		if (item) item->SetMarked(true);
	}
	
	SetValue("font size", (int)fFontSize);

	SetOn("use cache", gPrefs->GetPrefInt("use cache", 1));
	SetValue("cache block count", gPrefs->GetPrefInt("cache block count", 20));

	SetOn("stop at main", gPrefs->GetPrefInt("stop at main", 1));
	SetOn("don't stop for unwinding", gPrefs->GetPrefInt("don't stop for unwinding", 1));
	SetOn("output to window", gPrefs->GetPrefInt("output to window", 0));
	
	SetValue("malloc debug", gPrefs->GetPrefInt("malloc debug", 10));
	SetOn("leak check", gPrefs->GetPrefInt("leak check", 0));

	SetValue("array upper bound", gPrefs->GetPrefInt("array upper bound", 100));

	SetOn("at&t style disasm", gPrefs->GetPrefInt("at&t style disasm", 1));
	SetOn("intel style disasm", !IsOn("at&t style disasm"));
	
	return false;
} // DSettingsDialog::CancelClicked

void DSettingsDialog::UpdateFields()
{
	const char *ff, *fs;
	if (CurrentMessage() &&
		CurrentMessage()->FindString("family", &ff) == B_NO_ERROR &&
		CurrentMessage()->FindString("style", &fs) == B_NO_ERROR)
	{
		BMenuItem *item;

		item = fFont->FindMarked();
		if (item)
		{
			item = item->Submenu()->FindMarked();
			if (item) item->SetMarked(false);
			fFont->FindMarked()->SetMarked(false);
		}
		
		item = fFont->FindItem(ff);
		if (item)
		{
			item->SetMarked(true);
			item = item->Submenu()->FindItem(fs);
			if (item) item->SetMarked(true);
		}
	}

	SetEnabled("cache block count", IsOn("use cache"));
} // DSettingsDialog::UpdateFields
