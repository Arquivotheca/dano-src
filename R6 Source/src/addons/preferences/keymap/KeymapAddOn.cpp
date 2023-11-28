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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "PrefsAppExport.h"
#include "KeymapAddOn.h"

/////////////////////////////////////////////
//


extern "C" {

PPAddOn* get_addon_object(image_id i,PPWindow*w) {
	return (PPAddOn*)new KeymapAddOn(i,w);
}

} // extern "C"

KeymapAddOn::KeymapAddOn(image_id i,PPWindow*w):PPAddOn(i,w) {
}

bool KeymapAddOn::UseAddOn() {
	return true;
}

BBitmap* KeymapAddOn::Icon() {
	return new BBitmap(BRect(0,0,31,31),B_RGB32);
//	return (BBitmap*)InstantiateFromResource(Resources,"icon");
}

char* KeymapAddOn::Name() {
//	size_t foo_size;
//	return (char*)Resources->LoadResource(B_STRING_TYPE,"add-on name",&foo_size);
	return "Keymap";
}

char* KeymapAddOn::InternalName() {
	return "keymap";
}

BView* KeymapAddOn::MakeView() {
// really temporary code
#if 0
	BMessage model(0UL);
	BTextControl s(BRect(0,0,99,99),"name","label","text",&model);
	BMessage m;
	s.Archive(&m);
	PrintMessageToStream(&m);
#endif
// some other really temporary code
#if 0
	BMessage* model=new BMessage('mdel');
	BPopUpMenu* menu=new BPopUpMenu("menuname");
	menu->AddItem(new BMenuItem("menuitemname",model));	
	BMenuField* menufield=new BMenuField(BRect(0,0,100,100),"menufieldname","menufieldlabel",menu);
	BMessage m;
	menufield->Archive(&m);
	PrintMessageToStream(&m);
#endif
#if 0
	BView* v=(BView*)InstantiateFromResource(Resources,"view");
	return v;
#endif
	KeymapView* v=new KeymapView(BRect(0,0,399,399));

	KeyView*k=new KeyView(BRect(16,16,383,127));
	v->AddChild(k);

	BTabView*tv=new BTabView(BRect(0,144,399,399),"tabview");

	BTab* tab;
	BView*iv;

	iv=new BView(BRect(0,0,399,255-tv->TabHeight()),"keyedit",0,B_WILL_DRAW);
	UnicodeView* u=new UnicodeView(BRect(0,0,191,191));
	iv->AddChild(u);
	BSlider*s=new BSlider(BRect(0,191,191,231),"unislider","Character page selector",new BMessage('page'),0,255);
	s->SetTarget(u);
	iv->AddChild(s);

	KeyEditView*ke=new KeyEditView(BRect(200,0,399,199));
	iv->AddChild(ke);

	tab=new BTab;
	tv->AddTab(iv,tab);
	tab->SetLabel("Regular Keys");

	iv=new BView(BRect(0,0,399,255-tv->TabHeight()),"mods",0,B_WILL_DRAW);
	tab=new BTab;
	tv->AddTab(iv,tab);
	tab->SetLabel("Modifiers");

	iv=new BView(BRect(0,0,399,255-tv->TabHeight()),"deadkeys",0,B_WILL_DRAW);
	tab=new BTab;
	tv->AddTab(iv,tab);
	tab->SetLabel("Dead keys");

	v->AddChild(tv);

	return v;
}

//////////////////////////////////////////////////////////

KeymapView::KeymapView(BRect r):BView(r,"keymap",0,0) {
	SetViewColor(224,224,224);
}

void KeymapView::AllAttached() {
	((BSlider*)FindView("unislider"))->SetTarget(FindView("unicode"));
}

//////////////////////////////////////////////////////////

UnicodeView::UnicodeView(BRect r):BView(r,"unicode",0,B_WILL_DRAW) {
// font height (for glyph centering)
	font_height fh;
	be_plain_font->GetHeight(&fh);
	voffset=fh.ascent/(fh.ascent+fh.descent);

// list of known glyphs
	char* list=new char[65536*3];
	char* current=list;
	for (int i=1;i<65536;i++) {
		StringFor(i,current);
		current+=strlen(current);
	}
	be_plain_font->GetHasGlyphs(list,65535,has_glyph+1);
	has_glyph[0]=false;
	delete[] list;
	page=0;
}

void UnicodeView::Draw(BRect) {
	char s[4];
	for (int y=0;y<16;y++) {
		for (int x=0;x<16;x++) {
			int c=page*256+x+y*16;
			StringFor(c,s);
#if 0
			if (has_glyph[c]) {
#else
			bool b;
			be_plain_font->GetHasGlyphs(s,1,&b);
			if (b) {
#endif
				SetHighColor(0,0,0);
				DrawString(s,BPoint(x*(Bounds().Width()+1)/16,(y+voffset)*(Bounds().Height()+1)/16));
			} else {
				SetHighColor(255,128,128);
				DrawString("?",BPoint(x*(Bounds().Width()+1)/16,(y+voffset)*(Bounds().Height()+1)/16));
			}
		}
	}
}

void UnicodeView::MouseDown(BPoint p) {
	int x=int(16*p.x/(Bounds().Width()+1));
	int y=int(16*p.y/(Bounds().Height()+1));
	if ((x<0)||(x>15)||(y<0)||(y>15)) {
		printf("UnicodeView::MouseDown : out of range\n");
		return;
	}
	char s[4];
	StringFor(page*256+y*16+x,s);
	BMessage m('glph');
	m.AddString("glyph",s);
	DragMessage(&m,BRect(p.x-4,p.y-4,p.x+4,p.y+4));
}

void UnicodeView::MessageReceived(BMessage*m) {
	switch (m->what) {
		case 'page' : {
			page=m->FindInt32("be:value");
			Invalidate();
			break;
		}
		default : {
			BView::MessageReceived(m);
			break;
		}
	}
}

void UnicodeView::StringFor(int c,char* s) {
	memset(s,0,4);
	if (c<128) {
		s[0]=c;
	} else if (c<2048) {
		s[0]=0xc0+(c>>6);
		s[1]=0x80+(c&0x3f);
	} else {
		s[0]=0xe0+(c>>12);
		s[1]=0x80+((c>>6)&0x3f);
		s[2]=0x80+(c&0x3f);
	}
}

///////////////////////////

KeyView::KeyView(BRect r):BView(r,"keys",0,B_WILL_DRAW) {
	GenerateLayout();
}

void KeyView::Draw(BRect) {
	for (int y=0;y<7;y++) {
		for (int x=0;x<linelength[y];x++) {
			StrokeRect(keyframe[x][y]);
		}
	}
}

void KeyView::MouseDown(BPoint) {
	printf("KeyView::MouseDown\n");
}

/////////////////////////////

KeyEditView::KeyEditView(BRect r):BView(r,"keyedit",0,B_WILL_DRAW) {
}

void KeyEditView::Draw(BRect) {
	DrawString("Normal",BPoint(0,10));
	DrawString("Shift",BPoint(0,30));
	DrawString("CapsLock",BPoint(0,50));
	DrawString("Shift CapsLock",BPoint(0,70));
	DrawString("Option",BPoint(0,90));
	DrawString("Shift Option",BPoint(0,110));
	DrawString("Option CapsLock",BPoint(0,130));
	DrawString("Shift Option CapsLock",BPoint(0,150));
	DrawString("Control",BPoint(0,170));
}