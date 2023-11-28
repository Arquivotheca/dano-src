#include <File.h>
#include <Resources.h>
#include <Entry.h>
#include <Slider.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Button.h>
#include <View.h>
#include <InterfaceDefs.h>
#include <Region.h>
#include <Path.h>
#include <FindDirectory.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "PrefsAppExport.h"
#include "MouseAddOn.h"

////////////////////////////////////////////////////
// format of the mouse settings file...

typedef enum {
	MOUSE_1_BUTTON = 1,
	MOUSE_2_BUTTON,
	MOUSE_3_BUTTON
} mouse_type;

typedef struct {
	int32       left;
	int32       right;
	int32       middle;
} map_mouse;

typedef struct {
	bool    enabled;
	int32   accel_factor;
	int32   speed;
} mouse_accel;

typedef struct {
	mouse_type  type;
	map_mouse   map;
	mouse_accel accel;
	bigtime_t   click_speed;
} mouse_settings;

#define mouse_settings_file "Mouse_settings"

//////////////////////////////////////////
// default settings (should be in resources)

#define default_dbl_clk 500000
#define default_speed 65536
#define default_accel 65536
#define default_mmode 0
#define default_external_mmode B_NORMAL_MOUSE
#define default_nbuttons 3
#define default_orientation 0

////////////////////////////////////////////////////

MouseView::MouseView(BMessage*m):BView(m) {
// code that was used to track a bug ( BMenuField / B_POINTER_EVENTS )
#if 0
	BPopUpMenu*m=new BPopUpMenu("Mname");
	m->AddItem(new BMenuItem("MIlabel1",new BMessage('mmsg')));
	m->AddItem(new BMenuItem("MIlabel2",new BMessage('mmsh')));
	m->AddItem(new BMenuItem("MIlabel3",new BMessage('mmsi')));
	BMenuField* mf=new BMenuField(BRect(200,0,400,29),"MFname","MFlabel",m);
	BMessage msg;
	mf->Archive(&msg);
	PrintMessageToStream(&msg);
	mf=new BMenuField(&msg);
	AddChild(mf);
#endif
}

BArchivable* MouseView::Instantiate(BMessage*m) {
	if (!validate_instantiation(m,"MouseView")) {
		return NULL;
	}
	return new MouseView(m);
}

void MouseView::AllAttached() {
	get_mouse_map(&mmap);
	get_mouse_type(&nbuttons);

	bigtime_t dcs;
	get_click_speed(&dcs);
	printf("double click speed : %Ld\n",dcs);
	((BSlider*)FindView("dblclkslider"))->SetTarget(this);
	((BSlider*)FindView("dblclkslider"))->SetValue(500000-dcs);

	int32 spd;
	get_mouse_speed(&spd);
	printf("mouse speed : %ld\n",spd);
	((BSlider*)FindView("speedslider"))->SetTarget(this);
	((BSlider*)FindView("speedslider"))->SetValue(65536*(3+log(spd/65536.)/log(2.)));

	int32 acc;
	get_mouse_acceleration(&acc);
	printf("mouse acceleration : %ld\n",acc);
	((BSlider*)FindView("accelslider"))->SetTarget(this);
	((BSlider*)FindView("accelslider"))->SetValue((int32)sqrt(acc));

	((BButton*)FindView("default"))->SetTarget(this);
	((BButton*)FindView("revert"))->SetTarget(this);
	for (int i=0;i<4;i++) {
		((BMenuField*)FindView("ffm_mf"))->Menu()->ItemAt(i)->SetTarget(this);
	}
	int mmode=mouse_mode();
	mmode=(mmode&1)+((mmode>>1)&1)+((mmode>>2)&1);
	printf("mmode : %d\n",mmode);
	((BMenuField*)FindView("ffm_mf"))->MenuItem()->SetLabel(((BMenuField*)FindView("ffm_mf"))->Menu()->ItemAt(mmode)->Label());
	((BMenuField*)FindView("ffm_mf"))->Menu()->ItemAt(mmode)->SetMarked(true);
	for (int i=0;i<2;i++) {
		((BMenuField*)FindView("side_mf"))->Menu()->ItemAt(i)->SetTarget(this);
	}
	((BMenuField*)FindView("side_mf"))->MenuItem()->SetLabel(((BMenuField*)FindView("side_mf"))->Menu()->ItemAt(0)->Label());
	((BMenuField*)FindView("side_mf"))->Menu()->ItemAt(0)->SetMarked(true);
	((MousePointerView*)FindView("mpointer"))->current_bitmap=0;
	for (int i=0;i<3;i++) {
		((BMenuField*)FindView("nbutton_mf"))->Menu()->ItemAt(i)->SetTarget(this);
	}
	int32 nbuttons;
	get_mouse_type(&nbuttons);
	printf("nbuttons : %ld\n",nbuttons);
	((BMenuField*)FindView("nbutton_mf"))->MenuItem()->SetLabel(((BMenuField*)FindView("nbutton_mf"))->Menu()->ItemAt(nbuttons-1)->Label());
	((BMenuField*)FindView("nbutton_mf"))->Menu()->ItemAt(nbuttons-1)->SetMarked(true);

	for (int i=0;i<3;i++) {
		uint32 thismap=0;
		switch (i) {
			case 0 : {
				thismap=mmap.left;
				break;
			}
			case 1 : {
				thismap=mmap.right;
				break;
			}
			case 2 : {
				thismap=mmap.middle;
				break;
			}
		}
		for (int j=0;j<3;j++) {
			uint32 but[]={B_PRIMARY_MOUSE_BUTTON,B_SECONDARY_MOUSE_BUTTON,B_TERTIARY_MOUSE_BUTTON};
			((MouseButtonsView*)FindView("buttons"))->menu[i]->ItemAt(j)->SetTarget(this);
			((MouseButtonsView*)FindView("buttons"))->menu[i]->ItemAt(j)->SetMarked((thismap==but[j]));
		}
	}
}

void MouseView::AllDetached() {
	BPath path;
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		long ref;
		path.Append (mouse_settings_file);
		if ((ref = creat(path.Path(), 0644)) >= 0) {
			mouse_settings settings;
			get_mouse_type((int32*)&settings.type);
			get_mouse_map((mouse_map*)&settings.map);
			settings.accel.enabled=true;
			get_mouse_speed(&settings.accel.speed);
			get_mouse_acceleration(&settings.accel.accel_factor);
			get_click_speed(&settings.click_speed);
			write (ref, &settings, sizeof (mouse_settings));
			close(ref);
		}
	}
}

void MouseView::MessageReceived(BMessage*m) {
	switch(m->what) {
		case 'dclk' : {
			DoubleClickSpeed(m);
			break;
		}
		case 'mspd' : {
			MouseSpeed(m);
			break;
		}
		case 'macc' : {
			MouseAcceleration(m);
			break;
		}
		case 'ffm' : {
			FFM(m);
			break;
		}
		case 'csro' : {
			Orientation(m);
			break;
		}
		case 'nbut' : {
			NumberButtons(m);
			break;
		}
		case 'bmap' : {
			ButtonMapping(m);
			break;
		}
		case 'dflt' : {
			Default(m);
			break;
		}
		case 'rvrt' : {
			Revert(m);
			break;
		}
		default : {
			BView::MessageReceived(m);
			break;
		}
	}
}

void MouseView::DoubleClickSpeed(BMessage*m) {
	set_click_speed(500000-m->FindInt32("be:value"));
	EnableRevert();
}

void MouseView::MouseSpeed(BMessage*m) {
	set_mouse_speed(int32(65536*(exp((m->FindInt32("be:value")/65536.-3)*log(2.)))));
	EnableRevert();
}

void MouseView::MouseAcceleration(BMessage*m) {
	int32 acc=m->FindInt32("be:value");
	set_mouse_acceleration(acc*acc);
	EnableRevert();
}

void MouseView::FFM(BMessage*m) {
	BMenuItem*menuitem;
	m->FindPointer("source",(void**)&menuitem);
	((BMenuField*)FindView("ffm_mf"))->MenuItem()->SetLabel(menuitem->Label());
	set_mouse_mode((mode_mouse)m->FindInt32("ffm"));
	EnableRevert();
}

void MouseView::Orientation(BMessage*m) {
	BMenuItem*menuitem;
	m->FindPointer("source",(void**)&menuitem);
	((BMenuField*)FindView("side_mf"))->MenuItem()->SetLabel(menuitem->Label());
	((MousePointerView*)FindView("mpointer"))->current_bitmap=m->FindInt32("index");
	FindView("mpointer")->Invalidate();
	EnableRevert();
}

void MouseView::NumberButtons(BMessage*m) {
	BMenuItem*menuitem;
	m->FindPointer("source",(void**)&menuitem);
	((BMenuField*)FindView("nbutton_mf"))->MenuItem()->SetLabel(menuitem->Label());
	nbuttons=m->FindInt32("nbut");
	set_mouse_type(nbuttons);
	FindView("buttons")->Invalidate();
	EnableRevert();
}

void MouseView::ButtonMapping(BMessage*m) {
	uint32* but=NULL;
	switch(m->FindInt32("bnum")) {
		case 1: {
			but=&mmap.left;
			break;
		}
		case 2: {
			but=&mmap.right;
			break;
		}
		case 3: {
			but=&mmap.middle;
			break;
		}
	}
	*but=m->FindInt32("bmap");
	set_mouse_map(&mmap);
	EnableRevert();
	FindView("buttons")->Invalidate();
}

void MouseView::Default(BMessage*) {
// double click
	((BSlider*)FindView("dblclkslider"))->SetValue(500000-default_dbl_clk);
	set_click_speed(default_dbl_clk);
// mouse speed
	((BSlider*)FindView("speedslider"))->SetValue(65536*(3+log(default_speed/65536.)/log(2.)));
	set_mouse_speed(default_speed);
// mouse acceleration
	((BSlider*)FindView("accelslider"))->SetValue((int32)sqrt(default_accel));
	set_mouse_acceleration(default_accel);
// ffm
	((BMenuField*)FindView("ffm_mf"))->MenuItem()->SetLabel(((BMenuField*)FindView("ffm_mf"))->Menu()->ItemAt(default_mmode)->Label());
	((BMenuField*)FindView("ffm_mf"))->Menu()->FindMarked()->SetMarked(false);
	((BMenuField*)FindView("ffm_mf"))->Menu()->ItemAt(default_mmode)->SetMarked(true);
	set_mouse_mode(default_external_mmode);
// mouse type
	((BMenuField*)FindView("nbutton_mf"))->MenuItem()->SetLabel(((BMenuField*)FindView("nbutton_mf"))->Menu()->ItemAt(default_nbuttons-1)->Label());
	((BMenuField*)FindView("nbutton_mf"))->Menu()->FindMarked()->SetMarked(false);
	((BMenuField*)FindView("nbutton_mf"))->Menu()->ItemAt(default_nbuttons-1)->SetMarked(true);
	set_mouse_type(default_nbuttons);
	nbuttons=default_nbuttons;
	FindView("buttons")->Invalidate();
// pointer orientation
	((BMenuField*)FindView("side_mf"))->MenuItem()->SetLabel(((BMenuField*)FindView("side_mf"))->Menu()->ItemAt(default_orientation)->Label());
	((BMenuField*)FindView("side_mf"))->Menu()->FindMarked()->SetMarked(false);
	((BMenuField*)FindView("side_mf"))->Menu()->ItemAt(default_orientation)->SetMarked(true);
	((MousePointerView*)FindView("mpointer"))->current_bitmap=default_orientation;
	FindView("mpointer")->Invalidate();
// button mapping
	for (int i=0;i<3;i++) {
		((MouseButtonsView*)FindView("buttons"))->menu[i]->FindMarked()->SetMarked(false);
		((MouseButtonsView*)FindView("buttons"))->menu[i]->ItemAt(i)->SetMarked(true);
	}
	mmap.left=B_PRIMARY_MOUSE_BUTTON;
	mmap.middle=B_TERTIARY_MOUSE_BUTTON;
	mmap.right=B_SECONDARY_MOUSE_BUTTON;
	set_mouse_map(&mmap);
}

void MouseView::Revert(BMessage*) {
// double click
	((BSlider*)FindView("dblclkslider"))->SetValue(initial_clkspeed);
	set_click_speed(500000-initial_clkspeed);
// mouse speed
	((BSlider*)FindView("speedslider"))->SetValue(initial_speed);
//	set_mouse_speed(initial_speed);
	set_mouse_speed(int32(65536*(exp((initial_speed/65536.-3)*log(2.)))));
// mouse acceleration
	((BSlider*)FindView("accelslider"))->SetValue(initial_accel);
	set_mouse_acceleration(initial_accel*initial_accel);
// ffm
	int mm=(initial_mmode&1)+((initial_mmode>>1)&1)+((initial_mmode>>2)&1);
	((BMenuField*)FindView("ffm_mf"))->MenuItem()->SetLabel(((BMenuField*)FindView("ffm_mf"))->Menu()->ItemAt(mm)->Label());
	((BMenuField*)FindView("ffm_mf"))->Menu()->FindMarked()->SetMarked(false);
	((BMenuField*)FindView("ffm_mf"))->Menu()->ItemAt(mm)->SetMarked(true);
	set_mouse_mode((mode_mouse)initial_mmode);
// mouse type
	((BMenuField*)FindView("nbutton_mf"))->MenuItem()->SetLabel(((BMenuField*)FindView("nbutton_mf"))->Menu()->ItemAt(initial_nbuttons-1)->Label());
	((BMenuField*)FindView("nbutton_mf"))->Menu()->FindMarked()->SetMarked(false);
	((BMenuField*)FindView("nbutton_mf"))->Menu()->ItemAt(initial_nbuttons-1)->SetMarked(true);
	set_mouse_type(initial_nbuttons);
	nbuttons=initial_nbuttons;
	FindView("buttons")->Invalidate();
// pointer orientation
	printf("initial orientation %ld\n",initial_orientation);
	((BMenuField*)FindView("side_mf"))->MenuItem()->SetLabel(((BMenuField*)FindView("side_mf"))->Menu()->ItemAt(initial_orientation)->Label());
	((BMenuField*)FindView("side_mf"))->Menu()->FindMarked()->SetMarked(false);
	((BMenuField*)FindView("side_mf"))->Menu()->ItemAt(initial_orientation)->SetMarked(true);
	((MousePointerView*)FindView("mpointer"))->current_bitmap=initial_orientation;
	FindView("mpointer")->Invalidate();
// button mapping
	for (int i=0;i<3;i++) {
		uint32 thismap=0;
		switch (i) {
			case 0 : {
				thismap=initial_mmap.left;
				break;
			}
			case 1 : {
				thismap=initial_mmap.right;
				break;
			}
			case 2 : {
				thismap=initial_mmap.middle;
				break;
			}
		}
		for (int j=0;j<3;j++) {
			uint32 but[]={B_PRIMARY_MOUSE_BUTTON,B_SECONDARY_MOUSE_BUTTON,B_TERTIARY_MOUSE_BUTTON};
			((MouseButtonsView*)FindView("buttons"))->menu[i]->ItemAt(j)->SetMarked((thismap==but[j]));
		}
	}
	mmap=initial_mmap;
	set_mouse_map(&mmap);
	((BButton*)FindView("revert"))->SetEnabled(false);
	printf("revert settings\n");
}

void MouseView::PanelActivated() {
	((BButton*)FindView("revert"))->SetEnabled(false);
	initial_speed=((BSlider*)FindView("speedslider"))->Value();
	initial_accel=((BSlider*)FindView("accelslider"))->Value();
	initial_clkspeed=((BSlider*)FindView("dblclkslider"))->Value();
	initial_mmap=mmap;
	initial_nbuttons=((BMenuField*)FindView("nbutton_mf"))->Menu()->FindMarked()->Message()->FindInt32("nbut");
	initial_mmode=((BMenuField*)FindView("ffm_mf"))->Menu()->FindMarked()->Message()->FindInt32("ffm");
	initial_orientation=((MousePointerView*)FindView("mpointer"))->current_bitmap;
	printf("MouseView::PanelActivated\n");
}

void MouseView::EnableRevert() {
	((BButton*)FindView("revert"))->SetEnabled(
				initial_speed!=((BSlider*)FindView("speedslider"))->Value()
				||initial_accel!=((BSlider*)FindView("accelslider"))->Value()
				||initial_clkspeed!=((BSlider*)FindView("dblclkslider"))->Value()
				||initial_mmap.left!=mmap.left
				||initial_mmap.middle!=mmap.middle
				||initial_mmap.right!=mmap.right
				||initial_mmode!=((BMenuField*)FindView("ffm_mf"))->Menu()->FindMarked()->Message()->FindInt32("ffm")
				||initial_nbuttons!=nbuttons
				||initial_orientation!=((MousePointerView*)FindView("mpointer"))->current_bitmap
	);
}

/////////////////////////////////////////////////////////////////////
