#include <File.h>
#include <Resources.h>
#include <Entry.h>
#include <Slider.h>
#include <TextView.h>
#include <TextControl.h>
#include <Button.h>
#include <Menu.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Roster.h>
#include <TabView.h>
#include <Application.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "TestDyn.h"
#include "PrefsAppExport.h"

/////////////////////////////////////////////
//

PPAddOn* get_addon_object(image_id i,PPWindow*w) {
	return (PPAddOn*)new TestDynAddOn(i,w);
}

TestDynAddOn::TestDynAddOn(image_id i,PPWindow*w):PPAddOn(i,w) {
}

BBitmap* TestDynAddOn::Icon() {
	return new BBitmap(BRect(0,0,31,31),B_RGB32);
//	return (BBitmap*)InstantiateFromResource(Resources,"icon");
}

char* TestDynAddOn::Name() {
//	size_t foo_size;
//	return (char*)Resources->LoadResource(B_STRING_TYPE,"add-on name",&foo_size);
	return "Dynamic Panel";
}

char* TestDynAddOn::InternalName() {
	return "dyntest";
}

BView* TestDynAddOn::MakeView() {
	BView*v=new TestDynView(BRect(0,0,299,299),this);

	v->AddChild(new BButton(BRect(10,10,99,29),"newicon","Change icon",new BMessage('nicn')));
	v->AddChild(new BButton(BRect(10,40,99,59),"new","New Panel",new BMessage('npnl')));
	v->AddChild(new BButton(BRect(10,70,99,89),"mouse","Go to Mouse",new BMessage('mous')));
	BMessage*m=new BMessage('rmte');
	m->AddString("be:panel","dyntest");	
	v->AddChild(new BButton(BRect(10,100,99,119),"remote","Remote Message",m));

	return v;
}

//////////////////////////////////////////////////////////

TestDynView::TestDynView(BRect r,PPAddOn*a):BView(r,"testdyn",0,0) {
	AddOn=a;
	SetViewColor(96,96,96);
}

void TestDynView::AllAttached() {
	((BButton*)FindView("newicon"))->SetTarget(this);
	((BButton*)FindView("new"))->SetTarget(this);
	((BButton*)FindView("mouse"))->SetTarget(this);
	((BButton*)FindView("remote"))->SetTarget(be_app);
}

void TestDynView::MessageReceived(BMessage*m) {
	switch (m->what) {
		case 'nicn' : {
			printf("view : new icon\n");
			BBitmap*bmp=new BBitmap(BRect(0,0,31,31),B_RGB32);
			AddOn->ChangeIcon(bmp,new BBitmap(bmp));
			break;
		}
		case 'npnl' : {
			printf("view : new panel\n");
			AddOn->UseNewPanel(new TestDynAddOn2(AddOn->MyIid,AddOn->Window));
			break;
		}
		case 'mous' : {
			printf("view : mouse\n");
			AddOn->SwitchToPanel("mouse");
			break;
		}
		case 'rmte' : {
			printf("got a remote message\n");
			break;
		}
		default : {
			BView::MessageReceived(m);
			break;
		}
	}
}

//////////////////////////////////////////////////////////

TestDynAddOn2::TestDynAddOn2(image_id i,PPWindow*w):PPAddOn(i,w) {
}

BBitmap* TestDynAddOn2::Icon() {
	return new BBitmap(BRect(0,0,31,31),B_RGB32);
//	return (BBitmap*)InstantiateFromResource(Resources,"icon");
}

char* TestDynAddOn2::Name() {
//	size_t foo_size;
//	return (char*)Resources->LoadResource(B_STRING_TYPE,"add-on name",&foo_size);
	return "Dynamic SubPanel";
}

char* TestDynAddOn2::InternalName() {
	return "dyntest0";
}

BView* TestDynAddOn2::MakeView() {
	BView*v=new TestDynView2(BRect(0,0,299,299),this);
	v->AddChild(new BButton(BRect(10,10,99,29),"remove","remove self",new BMessage('rmov')));
	return v;
}

/////////////////////////////////////////////////////////////

TestDynView2::TestDynView2(BRect r,PPAddOn*a):BView(r,"testdyn",0,0) {
	AddOn=a;
	SetViewColor(153,102,51);
}

void TestDynView2::AllAttached() {
	((BButton*)FindView("remove"))->SetTarget(this);
}

void TestDynView2::MessageReceived(BMessage*m) {
	switch (m->what) {
		case 'rmov' : {
			printf("view : remove self\n");
			AddOn->RemovePanel(AddOn);
			break;
		}
		default : {
			BView::MessageReceived(m);
			break;
		}
	}
}

//////////////////////////////////////////////////////////
