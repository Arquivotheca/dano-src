/*
	CameraView.cpp
	Implementation.
*/

#include <stdio.h>
#include <string.h>
#include <Box.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Button.h>
#include <StringView.h>
#include <ScrollView.h>
#include <SerialPort.h>
#include <InterfaceDefs.h>
#include "appconfig.h"
#include "CameraView.h"

CameraView::CameraView(BRect view_rect) :
	BView(view_rect, NULL, B_FOLLOW_ALL, 0)
{
	int32		i;
	BRect		r;
	BBox		*box;
	BButton		*but;
	BPopUpMenu	*menu;
	BMenuField	*mf;
	PictureList	*list;
	BScrollView	*scroller;
	BSerialPort	serial;
	char		devName[B_OS_NAME_LENGTH];

	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);

	box = new BBox(BRect(7, 7, 7 + 136, 7 + 144));
	box->SetLabel(STR_CONNECTION);
	menu = new BPopUpMenu("PortMenu");
	for (i = 0; i < serial.CountDevices(); i++)
	{
		serial.GetDeviceName(i, devName);
		menu->AddItem(new BMenuItem(devName, new BMessage(MSG_SERIALPORT)));
	}
	{
		BEntry path("/dev/bus/usb");
		if(path.Exists()) {
			menu->AddItem(new BMenuItem("USB", new BMessage(MSG_USB)));
		}
	}	
	
	if (gPrefs->GetInt32("port", &i) != B_NO_ERROR)
		i = 0;
	menu->ItemAt(i)->SetMarked(true);

	mf = new BMenuField(BRect(8, 24, 8 + 110, 24 + 22),
				"SerialPort", STR_PORT, menu);
	mf->SetDivider(50.0f);
	box->AddChild(mf);
	menu = new BPopUpMenu("SpeedMenu");
	menu->AddItem(new BMenuItem("9600", new BMessage(MSG_9600)));
	menu->AddItem(new BMenuItem("19200", new BMessage(MSG_19200)));
	menu->AddItem(new BMenuItem("38400", new BMessage(MSG_38400)));
	menu->AddItem(new BMenuItem("57600", new BMessage(MSG_57600)));
	menu->AddItem(new BMenuItem("115200", new BMessage(MSG_115200)));
	if (gPrefs->GetInt32("speed", &i) != B_NO_ERROR)
		i = 1;
	menu->ItemAt(i)->SetMarked(true);
	mf = new BMenuField(BRect(8, 48, 8 + 110, 48 + 22),
				"SerialSpeed", STR_SPEED, menu);
	mf->SetDivider(50.0f);
	box->AddChild(mf);
	box->AddChild(new BButton(BRect(24, 80, 24 + 87, 80 + 24),
				"Probe", STR_PROBE, new BMessage(MSG_PROBE)));
	box->AddChild(new BButton(BRect(24, 112, 24 + 87, 112 + 24),
				"Connect", STR_CONNECT, new BMessage(MSG_CONNECT)));
	AddChild(box);

	box = new BBox(BRect(7, 159, 7 + 136, 159 + 88));
	box->SetLabel(STR_CAMERA);
	r.Set(8, 16, 8 + 123, 16 + 14);
	box->AddChild(new BStringView(r, "Make", STR_UNKNOWN_MAKE));
	r.OffsetBy(0, 17);
	box->AddChild(new BStringView(r, "Model", STR_UNKNOWN_MODEL));
	r.OffsetBy(0, 17);
	box->AddChild(new BStringView(r, "Version", STR_UNKNOWN_VERSION));
	r.OffsetBy(0, 17);
	box->AddChild(new BStringView(r, "NumPictures", STR_UNKNOWN_NUMPICS));
	AddChild(box);

	r = Bounds();
	box = new BBox(BRect(151, 7, r.right - 7, r.bottom - 7), NULL, B_FOLLOW_ALL);
	box->SetLabel(STR_PICTURES);
	r = box->Bounds();
	list = new PictureList(BRect(8, 16, r.right - 88 - B_V_SCROLL_BAR_WIDTH, r.bottom - 8), "Pictures");
	scroller = new BScrollView("ScrollPictures", list, B_FOLLOW_ALL, 0, false, true);
	box->AddChild(scroller);
	r = scroller->Bounds();
	r.Set(r.right + 12, 14, r.right + 12 + 72, 14 + 24);
	but = new BButton(r, "SelAll", STR_SELECTALL, new BMessage(MSG_SELALL), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	but->SetEnabled(false);
	box->AddChild(but);
	r.OffsetBy(0, 32);
	but = new BButton(r, "Save", STR_SAVE, new BMessage(MSG_SAVE), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	but->SetEnabled(false);
	box->AddChild(but);
	r.OffsetBy(0, 32);
	but = new BButton(r, "Delete", STR_DELETE, new BMessage(MSG_DELETE), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	but->SetEnabled(false);
	box->AddChild(but);
	AddChild(box);

	// disable scrolling to start off
	BScrollBar	*sb = scroller->ScrollBar(B_VERTICAL);
	if (sb != NULL)
	{
		sb->SetSteps(8, 64);
		sb->SetRange(0, 0);
		sb->SetValue(0);
		sb->SetProportion(1.0f);
	}

	// If the USB port is selected default, disable the speed menu
	{
		mf = (BMenuField *)FindView("SerialPort");
		if (mf != NULL)
		{
			BMenu *menu = mf->Menu();
			int n = menu->CountItems();
			for (i = 0; i < n; i++)
			{
				if (menu->ItemAt(i)->IsMarked())
				{
					if(strcmp(menu->ItemAt(i)->Label(), "USB") == 0) {
						BMenuField *mf1 = (BMenuField *)FindView("SerialSpeed");
						if (mf1 != NULL) {
							BMenu *menu1 = mf1->Menu();
							menu1->SetEnabled(false);
						}
					}
				}
			}
		}
		
	}
}

