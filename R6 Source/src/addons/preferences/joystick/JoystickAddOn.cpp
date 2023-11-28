#include <File.h>
#include <Resources.h>
#include <Entry.h>
#include <Slider.h>
#include <TextView.h>
#include <TextControl.h>
#include <Button.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Joystick.h>
#include <String.h>
#include <StringView.h>
#include <CheckBox.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "JoystickTweaker.h"

#include "PrefsAppExport.h"
#include "JoystickAddOn.h"

PPAddOn* get_addon_object(image_id i,PPWindow*w) {
	return (PPAddOn*)new JoystickAddOn(i,w);
}

////////////////////////////////////////////////////////////////

JoystickView* theview;

////////////////////////////////////////////////////////////////

JoystickAddOn::JoystickAddOn(image_id i,PPWindow*w):PPAddOn(i,w) {
}

bool JoystickAddOn::UseAddOn() {
	BJoystick stick;
	return (stick.CountDevices()>0);
}

BBitmap* JoystickAddOn::Icon() {
	return (BBitmap*)InstantiateFromResource(Resources,"icon");
}

BBitmap* JoystickAddOn::SmallIcon() {
	return (BBitmap*)InstantiateFromResource(Resources,"smallicon");
}

char* JoystickAddOn::Name() {
	size_t foo_size;
	return (char*)Resources->LoadResource(B_STRING_TYPE,"add-on name",&foo_size);
}

char* JoystickAddOn::InternalName() {
	return "joystick";
}

BView* JoystickAddOn::MakeView() {
	theview=(JoystickView*)InstantiateFromResource(Resources,"view");
	return theview;
}

///////////////////////////////////////////////////////////////////

JoystickView::JoystickView(BMessage*m):BView(m) {
	settings_list=new BList;
	detected_list=new BList;
	detected_file_list=new BList;
	currentport=0;
}

BArchivable* JoystickView::Instantiate(BMessage*m) {
	if (!validate_instantiation(m,"JoystickView")) {
		return NULL;
	}
	return new JoystickView(m);
}

void JoystickView::AllAttached() {
// set the button targets
	((BButton*)FindView("calibrate"))->SetTarget(this);
	((BButton*)FindView("probe"))->SetTarget(this);

// add the devices in the popup
	BJoystick stick;
	int32 ndev=stick.CountDevices();
	char name[B_OS_NAME_LENGTH];
	for (int32 i=0;i<ndev;i++) {
		stick.GetDeviceName(i,name);
		BMenuItem*mi=new BMenuItem(name,new BMessage('port'));
		mi->SetTarget(this);
		((BMenuField*)FindView("port"))->Menu()->AddItem(mi);
		if (i==0) {
			mi->SetMarked(true);
			((BMenuField*)FindView("port"))->MenuItem()->SetLabel(mi->Label());
		}
		char*name=new char[257]; // what's that constant's name already?
		name[0]='\0';
		detected_list->AddItem(name);
		snooze(0x55aa);
		name=new char[257];
		name[0]='\0';
		detected_file_list->AddItem(name);
	}

// get the list of supported joysticks
	BPath path;
	if (find_directory(B_BEOS_ETC_DIRECTORY,&path)==B_OK) {
		path.Append("joysticks");
		AddJoystickList(&path,settings_list);
	}
	if (find_directory(B_COMMON_ETC_DIRECTORY, &path)==B_OK) {
		path.Append("joysticks");
		AddJoystickList(&path,settings_list);		
	}
	for (int i=0;i<settings_list->CountItems();i++) {
		printf("found %s\n",(char*)settings_list->ItemAt(i));
	}
}

void JoystickView::MessageReceived(BMessage*m) {
	switch(m->what) {
		case 'dflt' : {
			printf("default\n");
			break;
		}
		case 'rvrt' : {
			printf("revert\n");
			break;
		}
		case 'cali' : {
			Calibrate(currentport);
			break;
		}
		case 'prob' : {
			Probe(currentport);
			break;
		}
		case 'port' : {
			printf("port changed\n");
			BMenuItem*menuitem;
			m->FindPointer("source",(void**)&menuitem);
			((BMenuField*)FindView("port"))->MenuItem()->SetLabel(menuitem->Label());
			currentport=Window()->CurrentMessage()->FindInt32("index");
			((BStringView*)FindView("currentname"))->SetText((char*)detected_list->ItemAt(currentport));
			((BButton*)FindView("calibrate"))->SetEnabled((*(char*)detected_list->ItemAt(currentport)!='\0'));
			break;
		}
		default : {
			BView::MessageReceived(m);
			break;
		}
	}
}

void JoystickView::Probe(int32 portnb) {
	printf("probe\n");
	char devname[B_OS_NAME_LENGTH];
	sprintf(devname,"/dev/joystick/%s",((BMenuField*)FindView("port"))->MenuItem()->Label());
	printf("current port is %s\n",devname);
	BJoystick stick;

	BString subname;
	int i;
	for (i=settings_list->CountItems()-1;i>=0;i--) {
		if (stick.Open(devname,false)<0) {
			printf("error while opening joystick port\n");
			return;
		}
		entry_ref ref;
		status_t ref_err = get_ref_for_path((char*)settings_list->ItemAt(i), &ref);
		if (!ref_err && stick.EnterEnhancedMode(&ref)) {
			stick.GetControllerName(&subname);
			if (CheckStickMatch((char*)settings_list->ItemAt(i),subname.String())) {
				printf("found a %s\n",subname.String());
				break;
			}
		}
		stick.Close();
	}
	if (i<0) {
		*(char*)(detected_list->ItemAt(portnb))='\0';
		*(char*)(detected_file_list->ItemAt(portnb))='\0';
		((BStringView*)FindView("currentname"))->SetText("");
		((BButton*)FindView("calibrate"))->SetEnabled(false);
	} else {
		strcpy((char*)(detected_list->ItemAt(portnb)),subname.String());
		strcpy((char*)(detected_file_list->ItemAt(portnb)),(char*)(settings_list->ItemAt(i)));
		((BStringView*)FindView("currentname"))->SetText(subname.String());
		((BButton*)FindView("calibrate"))->SetEnabled(true);
		stick.Close();
	}
}

bool JoystickView::CheckStickMatch(const char*path,const char*stickname) {
	char line[300];
	char arg[300];
	FILE * f = fopen(path, "r");
	if (f==NULL) return false;
	while (true) {
		char * ptr;
		line[0] = 0;
		fgets(line, 299, f);
		if (!line[0]) {
			fclose(f);
			return false;
		}
		ptr = line;
		while (*ptr && isspace(*ptr)) ptr++;
		if (!*ptr || *ptr=='#') continue;
		if (1 == sscanf(ptr, "gadget = \"%299[^\"\n]\"", arg)) {
			if (!strcmp(arg,stickname)) {
				fclose(f);
				return true;
			}
		}
	}
}

void JoystickView::Calibrate(int32 port) {
	printf("calibrate\n");
	printf("will attempt to calibrate joystick from port %ld\n",port);
	char devname[B_OS_NAME_LENGTH];
	sprintf(devname,"/dev/joystick/%s",((BMenuField*)FindView("port"))->MenuItem()->Label());
	printf("port path : %s\n",devname);
	printf("joystick name : %s\n",(char*)detected_list->ItemAt(port));
	printf(" file name : %s\n",(char*)detected_file_list->ItemAt(port));

	if ((*(char*)detected_list->ItemAt(port)=='\0')||(*(char*)detected_file_list->ItemAt(port)=='\0')) return; // quick sanity check

	BJoystick*calibjoystick=new BJoystick;
	if (calibjoystick->Open(devname)<0) {
		printf("could not open the joystick port\n");
		delete calibjoystick;
		return;
	}
	entry_ref ref;
	status_t ref_err = get_ref_for_path((char*)detected_file_list->ItemAt(port), &ref);
	if ((ref_err!=B_OK)||(!calibjoystick->EnterEnhancedMode(&ref))) {
		printf("could not switch the joystick to enhanced mode\n");
		calibjoystick->Close();
		delete calibjoystick;
		return;
	}
	BString subname;
	calibjoystick->GetControllerName(&subname);
	if (strcmp(subname.String(),(char*)detected_list->ItemAt(port))) {
		printf("did not detect the right joystick\n");
		calibjoystick->Close();
		delete calibjoystick;
		return;
	}
	calibjoystick->Update();
	new JoystickWindow(calibjoystick);
}

void JoystickView::AddJoystickList(BPath*path,BList*list) {
	BDirectory dir;
	if (dir.SetTo(path->Path())) {
		return;
	}
	BEntry ent;
	while (dir.GetNextEntry(&ent)==B_OK) {
		BPath thispath;
		if (ent.GetPath(&thispath)==B_OK) {
			list->AddItem(strdup(thispath.Path()));
		}
	}
}

/////////////////////////////////////////////////////////////////

JoystickWindow::JoystickWindow(BJoystick*j):BWindow(BRect(200,200,499,599),"Joystick Calibration",B_MODAL_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,B_NOT_ZOOMABLE|B_NOT_RESIZABLE|B_NOT_MINIMIZABLE) {
	calibjoystick=j;
	topview=new BView(Bounds(),"topview",B_FOLLOW_ALL_SIDES,0);
	topview->SetViewColor(216,216,216);
	AddChild(topview);
	topview->AddChild(new BStringView(BRect(0,0,299,15),"calibmessage",""));
	((BStringView*)FindView("calibmessage"))->SetAlignment(B_ALIGN_CENTER);

	Run();
	resume_thread(spawn_thread(_CalibThread,"Joystick Calibration",B_DISPLAY_PRIORITY,this));
}

int32 JoystickWindow::CalibThread() {
	printf("Calibration thread starting\n");

	int32 numaxes;
	int32 numbuttons;
	int32 numhats;
	AxisView** axes;
	ButtonView** buttons;
	HatView** hats;
	BStringView** saxes;
	BStringView** sbuttons;
	BStringView** shats;
	BCheckBox** blatch;
	BCheckBox** bauto;
	BString eltname;

	int16* rest;

	int32 calibphase=0;
	int32 previousphase=0;

	BJoystick::_joystick_info * j_info=_BJoystickTweaker(*calibjoystick).get_info();

	Lock();

	numaxes=calibjoystick->CountAxes();
	numbuttons=calibjoystick->CountButtons();
	numhats=calibjoystick->CountHats();

	rest=new int16[numaxes];

	axes=new AxisView*[numaxes];
	buttons=new ButtonView*[numbuttons];
	hats=new HatView*[numhats];
	saxes=new BStringView*[numaxes];
	sbuttons=new BStringView*[numbuttons];
	shats=new BStringView*[numhats];
	blatch=new BCheckBox*[numbuttons];
	bauto=new BCheckBox*[numbuttons];

	for (int32 i=0;i<numaxes;i++) {
		axes[i]=new AxisView(BRect(100,27+20*i,249,27+9+20*i));
		topview->AddChild(axes[i]);
		calibjoystick->GetAxisNameAt(i,&eltname);
		saxes[i]=new BStringView(BRect(0,20+20*i,99,39+20*i),"",eltname.String());
		saxes[i]->SetAlignment(B_ALIGN_RIGHT);
		topview->AddChild(saxes[i]);
	}
	for (int32 i=0;i<numbuttons;i++) {
		buttons[i]=new ButtonView(BRect(100,27+20*i+20*numaxes,109,27+9+20*i+20*numaxes));
		topview->AddChild(buttons[i]);
		calibjoystick->GetButtonNameAt(i,&eltname);
		sbuttons[i]=new BStringView(BRect(0,20+20*i+20*numaxes,99,39+20*i+20*numaxes),"",eltname.String());
		sbuttons[i]->SetAlignment(B_ALIGN_RIGHT);
		blatch[i]=new BCheckBox(BRect(120,22+20*i+20*numaxes,169,41+20*i+20*numaxes),"latch","Latch",NULL);
		bauto[i]=new BCheckBox(BRect(180,22+20*i+20*numaxes,229,41+20*i+20*numaxes),"auto","Auto",NULL);
		topview->AddChild(sbuttons[i]);
		topview->AddChild(blatch[i]);
		topview->AddChild(bauto[i]);
	}
	for (int32 i=0;i<numhats;i++) {
		hats[i]=new HatView(BRect(100,27+30*i+20*numbuttons+20*numaxes,119,27+19+30*i+20*numbuttons+20*numaxes));
		topview->AddChild(hats[i]);
		calibjoystick->GetHatNameAt(i,&eltname);
		shats[i]=new BStringView(BRect(0,25+30*i+20*numbuttons+20*numaxes,99,44+30*i+20*numbuttons+20*numaxes),"",eltname.String());
		shats[i]->SetAlignment(B_ALIGN_RIGHT);
		topview->AddChild(shats[i]);
	}

	((BStringView*)FindView("calibmessage"))->SetText("center the joystick and press the trigger");
	ResizeTo(Bounds().Width(),27+30*numhats+20*numbuttons+20*numaxes);
	Show();
	Unlock();

	printf("calib thread about to enter loop\n");

	while (calibphase<4) {
		calibjoystick->Update();

		switch (calibphase) {
			case 0 : { // waiting for a button
				if (calibjoystick->ButtonValues()&1) {
					calibphase++;
				}
				break;
			}
			case 1 : { // waiting for a button release
				if (calibjoystick->ButtonValues()==0) {
					calibphase++;
					for (int32 i=0;i<numaxes;i++) {
						axes[i]->EnableMinMax(true);
					}
				}
				break;
			}
			case 2 : { // waiting for a button
				if (calibjoystick->ButtonValues()&1) {
					calibphase++;
				}
				break;
			}
			case 3 : { // waiting for a button release
				if (calibjoystick->ButtonValues()==0) {
					calibphase++;
				}
				break;
			}
			default : {
				printf("unknown calibration phase\n");
				break;
			}
		}

		Lock();
		int16* axesvalues=new int16[numaxes];
		uint8* hatsvalues=new uint8[numhats];
		calibjoystick->GetAxisValues(axesvalues);
		for (int32 i=0;i<numaxes;i++) {
			axes[i]->SetValue(axesvalues[i]/32768.);
		}
		int32 cbut=calibjoystick->ButtonValues();
		for (int32 i=0;i<numbuttons;i++) {
			buttons[i]->SetValue((cbut>>i)&1);
		}
		calibjoystick->GetHatValues(hatsvalues);
		for (int32 i=0;i<numhats;i++) {
			hats[i]->SetValue(hatsvalues[i]);

		}

		if (previousphase!=calibphase) {
			switch(calibphase) {
				case 2 : {
					for (int32 i=0;i<numaxes;i++) {
						rest[i]=axesvalues[i];
					}
					((BStringView*)FindView("calibmessage"))->SetText("move the joystick in all directions, then press the trigger");
					break;
				}
				case 4 : {
					((BStringView*)FindView("calibmessage"))->SetText("");
					break;
				}
			}
		}
		previousphase=calibphase;

		Sync();
		Unlock();
		delete[] axesvalues;
		delete[] hatsvalues;
		snooze(30000);
	}

	for (int i=0;i<numaxes;i++) {
		printf("axis %d : center %hd, min %hd, max %hd\n",i,rest[i],(short)(axes[i]->minvalue*32767+.5),(short)(axes[i]->maxvalue*32767+.5));
		printf("dead min %hd, dead max %hd\n",(short)(0.9*rest[i]+3276.7*axes[i]->minvalue),(short)(0.9*rest[i]+3276.7*axes[i]->maxvalue));
		j_info->axis_calib[i].bottom=(short)(axes[i]->minvalue*32767+.5);
		j_info->axis_calib[i].top=(short)(axes[i]->maxvalue*32767+.5);
		j_info->axis_calib[i].start_dead=(short)(0.9*rest[i]+3276.7*axes[i]->minvalue);
		j_info->axis_calib[i].end_dead=(short)(0.9*rest[i]+3276.7*axes[i]->maxvalue);
		j_info->axis_calib[i].bottom_mul=(int)ceil(128.0*32767/(j_info->axis_calib[i].start_dead-j_info->axis_calib[i].bottom));
		j_info->axis_calib[i].top_mul=(int)ceil(128.0*32767/(j_info->axis_calib[i].top-j_info->axis_calib[i].end_dead));
	}
	printf("TODO : save button settings\n");
	for (int i=0;i<numbuttons;i++) {
		printf("button %d :",i);
		if (blatch[i]->Value()!=0) {
			printf("latch ");
		}
		if (bauto[i]->Value()!=0) {
			printf("auto ");
		}
		printf("\n");
	}

	_BJoystickTweaker(*calibjoystick).save_config(NULL);

	delete[] axes;
	delete[] buttons;
	delete[] hats;
	delete[] saxes;
	delete[] sbuttons;
	delete[] shats;
	delete[] blatch;
	delete[] bauto;

	delete[] rest;

	calibjoystick->Close();
	delete calibjoystick;
	printf("Calibration thread stopping\n");
	Lock();
	Quit();
	return B_OK;
}

/////////////////////////////////////////////////////////////////

AxisView::AxisView(BRect r):BView(r,"",0,B_WILL_DRAW) {
	SetViewColor(B_TRANSPARENT_32_BIT);
	currentvalue=0;
	minvalue=0;
	maxvalue=0;
	minmaxenabled=0;
}

void AxisView::Draw(BRect) {
	BRect b=Bounds();
	float xmin=Bounds().Width()*(minvalue+1)/2;
	float xmax=Bounds().Width()*(maxvalue+1)/2;
	float x=Bounds().Width()*(currentvalue+1)/2;

	SetHighColor(0,64,0);
	if (minmaxenabled) {
		if (b.left<xmin) {
			FillRect(BRect(b.left,b.top,xmin-1,b.bottom));
		}
		if (b.right>xmax) {
			FillRect(BRect(xmax+1,b.top,b.right,b.bottom));
		}
		SetHighColor(0,128,0);
		if (xmin<x) {
			FillRect(BRect(xmin,b.top,x-1,b.bottom));
		}
		if (xmax>x) {
			FillRect(BRect(x,b.top,xmax,b.bottom));
		}
	} else {
		if (b.left<x) {
			FillRect(BRect(b.left,b.top,x-1,b.bottom));
		}
		if (b.right>x) {
			FillRect(BRect(x+1,b.top,b.right,b.bottom));
		}
	}
	SetHighColor(0,255,0);
	StrokeLine(BPoint(x,0),BPoint(x,Bounds().bottom));
}

void AxisView::SetValue(float f) {
	if (f<-1) {
		f=-1;
	}
	if (f>1) {
		f=1;
	}
	currentvalue=f;
	if (minmaxenabled) {
		if (f<minvalue) {
			minvalue=f;
		}
		if (f>maxvalue) {
			maxvalue=f;
		}
	}
	Draw(BRect());
}

void AxisView::EnableMinMax(bool e) {
	if (e) {
		minvalue=maxvalue=currentvalue;
	}
	minmaxenabled=e;
}

/////////////////////////////////////////////////////////////

ButtonView::ButtonView(BRect r):BView(r,"",0,B_WILL_DRAW) {
	SetViewColor(B_TRANSPARENT_32_BIT);
	buttonpressed=false;
}

void ButtonView::Draw(BRect) {
	if (buttonpressed) {
		SetHighColor(255,0,0);
	} else {
		SetHighColor(128,0,0);
	}
	FillRect(Bounds());
}

void ButtonView::SetValue(bool b) {
	buttonpressed=b;
	Draw(BRect());
}

/////////////////////////////////////////////////////////////

HatView::HatView(BRect r):BView(r,"",0,B_WILL_DRAW) {
	SetViewColor(B_TRANSPARENT_32_BIT);
	currentvalue=0;
}

void HatView::Draw(BRect) {
	BRect b=Bounds();
	BPoint p;
	switch (currentvalue) {
		case 0 : {
			p=BPoint((Bounds().left+Bounds().right)/2,(Bounds().top+Bounds().bottom)/2);
			break;
		}
		case 1 : {
			p=BPoint((Bounds().left+Bounds().right)/2,Bounds().top+2);
			break;
		}
		case 2 : {
			p=BPoint(Bounds().right-2,Bounds().top+2);
			break;
		}
		case 3 : {
			p=BPoint(Bounds().right-2,(Bounds().top+Bounds().bottom)/2);
			break;
		}
		case 4 : {
			p=BPoint(Bounds().right-2,Bounds().bottom-2);
			break;
		}
		case 5 : {
			p=BPoint((Bounds().left+Bounds().right)/2,Bounds().bottom-2);
			break;
		}
		case 6 : {
			p=BPoint(Bounds().left+2,Bounds().bottom-2);
			break;
		}
		case 7 : {
			p=BPoint(Bounds().left+2,(Bounds().top+Bounds().bottom)/2);
			break;
		}
		case 8 : {
			p=BPoint(Bounds().left+2,Bounds().top+2);
			break;
		}
	}
	SetHighColor(0,0,128);
	FillRect(BRect(b.left,b.top,b.right,p.y-2));
	FillRect(BRect(b.left,p.y-1,p.x-2,p.y+1));
	FillRect(BRect(p.x+2,p.y-1,b.right,p.y+1));
	FillRect(BRect(b.left,p.y+2,b.right,b.bottom));
	SetHighColor(128,128,255);
	StrokeRect(BRect(p.x-1,p.y-1,p.x+1,p.y+1));
	SetHighColor(192,192,255);
	StrokeLine(p,p);
}

void HatView::SetValue(uint8 v) {
	currentvalue=v;
	Draw(BRect());
}
