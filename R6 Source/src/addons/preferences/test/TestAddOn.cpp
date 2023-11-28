#include <File.h>
#include <Resources.h>
#include <Entry.h>

#include <stdio.h>
#include <string.h>

#include "PrefsAppExport.h"
#include "TestAddOn.h"

PPAddOn* get_addon_object(image_id i,PPWindow*w) {
	return (PPAddOn*)new TestAddOn(i,w);
}

TestAddOn::TestAddOn(image_id i,PPWindow*w):PPAddOn(i,w) {
}

bool TestAddOn::UseAddOn() {
	printf("TestAddOn::UseAddOn\n");
	return true;
}

BBitmap* TestAddOn::Icon() {
	printf("TestAddOn::Icon\n");
	return (BBitmap*)InstantiateFromResource(Resources,"icon");
}

char* TestAddOn::Name() {
	printf("TestAddOn::Name\n");
	char* name;
	size_t foo_size;
	name=(char*)Resources->LoadResource(B_STRING_TYPE,"add-on name",&foo_size);
	if (name!=NULL) {
		return name;
	} else {
		return "A test add-on";
	}
}

char* TestAddOn::InternalName() {
	printf("TestAddOn::InternalName\n");
	return "logaddon";
}

void TestAddOn::LoadPrefs(BMessage*m) {
	printf("TestAddOn::LoadPrefs\n");
	int8 i8;
	int16 i16;
	int32 i32;
	int64 i64;
	float f;
	double d;
	bool b;
	const char* s;
	if (m->FindInt8("vint8",&i8)==B_OK) {
		printf("vint8 : %d\n",i8);
	}
	if (m->FindInt8("$hint8",&i8)==B_OK) {
		printf("hint8 : %d\n",i8);
	}
	if (m->FindInt16("vint16",&i16)==B_OK) {
		printf("vint16 : %d\n",i16);
	}
	if (m->FindInt16("$hint16",&i16)==B_OK) {
		printf("hint16 : %d\n",i16);
	}
	if (m->FindInt32("vint32",&i32)==B_OK) {
		printf("vint32 : %ld\n",i32);
	}
	if (m->FindInt32("$hint32",&i32)==B_OK) {
		printf("hint32 : %ld\n",i32);
	}
	if (m->FindInt64("vint64",&i64)==B_OK) {
		printf("vint64 : %Ld\n",i64);
	}
	if (m->FindInt64("$hint64",&i64)==B_OK) {
		printf("hint64 : %Ld\n",i64);
	}
	if (m->FindFloat("vfloat",&f)==B_OK) {
		printf("vfloat : %f\n",f);
	}
	if (m->FindFloat("$hfloat",&f)==B_OK) {
		printf("hfloat : %f\n",f);
	}
	if (m->FindDouble("vdouble",&d)==B_OK) {
		printf("vdouble : %f\n",d);
	}
	if (m->FindDouble("$hdouble",&d)==B_OK) {
		printf("hdouble : %f\n",d);
	}
	if (m->FindBool("vbool",&b)==B_OK) {
		printf("vbool : %s\n",b?"true":"false");
	}
	if (m->FindBool("$hbool",&b)==B_OK) {
		printf("hbool : %s\n",b?"true":"false");
	}
	if (m->FindString("vstring",&s)==B_OK) {
		printf("vstring : %s\n",s);
	}
	if (m->FindString("$hstring",&s)==B_OK) {
		printf("hstring : %s\n",s);
	}
}

void TestAddOn::SavePrefs(BMessage*m) {
	printf("TestAddOn::SavePrefs\n");
	m->AddString("vstring","visible");
	m->AddString("$hstring","hidden");
	m->AddInt8("vint8",81);
	m->AddInt8("$hint8",82);
	m->AddInt16("vint16",161);
	m->AddInt16("$hint16",162);
	m->AddInt32("vint32",321);
	m->AddInt32("$hint32",322);
	m->AddInt64("vint64",641);
	m->AddInt64("$hint64",642);
	m->AddBool("vbool",true);
	m->AddBool("$hbool",false);
	m->AddFloat("vfloat",32.1);
	m->AddFloat("$hfloat",32.2);
	m->AddDouble("vdouble",64.1);
	m->AddDouble("$hdouble",64.2);
}

bool TestAddOn::QuitRequested() {
	printf("TestAddOn::QuitRequested\n");
	return true;
}

void TestAddOn::PanelActivated(bool s) {
	printf("TestAddOn::PanelActivated(%s)\n",s?"true":"false");
}

BView* TestAddOn::MakeView() {
	printf("TestAddOn::MakeView\n");
	return (BView*)InstantiateFromResource(Resources,"view");
}

//////////////////////////////

TestView::TestView(BMessage*m):BView(m) {
}

BArchivable* TestView::Instantiate(BMessage*m) {
	if (!validate_instantiation(m,"TestView")) {
		return NULL;
	}
	return new TestView(m);
}
