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

#include <stdio.h>
#include <string.h>

#include "PrefsAppExport.h"
#include "MouseAddOn.h"

MouseView*the_view;

///////////////////////////////////////////////////

PPAddOn* get_addon_object(image_id i,PPWindow*w) {
	return (PPAddOn*)new MouseAddOn(i,w);
}

///////////////////////////////////////////////////

MouseAddOn::MouseAddOn(image_id i,PPWindow*w):PPAddOn(i,w) {
}

bool MouseAddOn::UseAddOn() {
	return true;
}

BBitmap* MouseAddOn::Icon() {
	return (BBitmap*)InstantiateFromResource(Resources,"icon");
}

BBitmap* MouseAddOn::SmallIcon() {
	return (BBitmap*)InstantiateFromResource(Resources,"smallicon");
}

char* MouseAddOn::Name() {
	size_t foo_size;
	return (char*)Resources->LoadResource(B_STRING_TYPE,"add-on name",&foo_size);
}

char* MouseAddOn::InternalName() {
	return "mouse";
}

BView* MouseAddOn::MakeView() {
	printf("MouseAddOn::MakeView Resources is %p\n",Resources);
	the_view=(MouseView*)InstantiateFromResource(Resources,"view");
	printf("MouseAddOn::MakeView will return %p\n",the_view);
	return the_view;
}

void MouseAddOn::PanelActivated(bool c) {
	if (c) {
		the_view->PanelActivated();
	}
}

/////////////////////////////////////////////////////////////////////

MousePointerView::MousePointerView(BMessage*m):BView(m) {
	bmp[0]=new BBitmap(BRect(0,0,15,15),B_CMAP8);
	bmp[1]=new BBitmap(BRect(0,0,15,15),B_CMAP8);
	uchar*bits[2]={(uchar*)bmp[0]->Bits(),(uchar*)bmp[1]->Bits()};
	for (int j=0;j<16;j++) {
		for (int i=0;i<16;i++) {
			uchar val=B_HAND_CURSOR[4+(i>>3)+j*2]&(0x80>>(i&7))?0:B_TRANSPARENT_8_BIT;
			bits[0][i+j*16]=val;
			bits[1][15-i+j*16]=val;
		}
	}
}

MousePointerView::~MousePointerView() {
	delete bmp[0];
	delete bmp[1];
}

void MousePointerView::Draw(BRect) {
	DrawBitmap(bmp[current_bitmap],BPoint(0,0));
}

BArchivable* MousePointerView::Instantiate(BMessage*m) {
	if (!validate_instantiation(m,"MousePointerView")) {
		return NULL;
	}
	return new MousePointerView(m);
}

////////////////////////////////////////////////

MouseButtonsView::MouseButtonsView(BMessage*m):BView(m) {
	buttons=0;
	BMessage msg;
	m->FindMessage("menus",0,&msg);
	menu[0]=new BPopUpMenu(&msg);
	m->FindMessage("menus",1,&msg);
	menu[1]=new BPopUpMenu(&msg);
	m->FindMessage("menus",2,&msg);
	menu[2]=new BPopUpMenu(&msg);

	m->FindMessage("bitmap",&msg);
	bmp=new BBitmap(&msg);
	xmin=m->FindInt32("xmin");
	xmax=m->FindInt32("xmax");
	ymin=m->FindInt32("ymin");
	ymax=m->FindInt32("ymax");
}

MouseButtonsView::~MouseButtonsView() {
}

void MouseButtonsView::AttachedToWindow() {
	SetViewBitmap(bmp);
}

void MouseButtonsView::MouseDown(BPoint p) {
	buttons=Window()->CurrentMessage()->FindInt32("buttons");
	Invalidate();

	BRegion r;
	GetClippingRegion(&r);
	if (r.Contains(p)) {
		int32 nbuttons=((MouseView*)Parent())->nbuttons;
		for (int i=0;i<nbuttons;i++) {
			BRect rec(xmin+i*(xmax-xmin)/nbuttons+1,ymin,xmin+(i+1)*(xmax-xmin)/nbuttons-1,ymax);
			if (rec.Contains(p)) {
				int cnv=i;
				if ((nbuttons==3)&&(i>0)) {
					cnv=3-i;
				}
				menu[cnv]->Go(ConvertToScreen(p),true,false,true);
			}
		}
	}
}

void MouseButtonsView::MouseUp(BPoint) {
	buttons=0;
	Invalidate();
}

void MouseButtonsView::MouseMoved(BPoint,uint32,const BMessage*) {
	int32 newbuttons=Window()->CurrentMessage()->FindInt32("buttons");
	if (newbuttons!=buttons) {
		buttons=newbuttons;
		Invalidate();
	}
}

void MouseButtonsView::Draw(BRect) {
	int32 nbuttons=((MouseView*)Parent())->nbuttons;
	mouse_map mmap=((MouseView*)Parent())->mmap;

	SetHighColor(0,0,0);
	SetDrawingMode(B_OP_COPY);
	for (int i=1;i<nbuttons;i++) {
		StrokeLine(BPoint(xmin+i*(xmax-xmin)/nbuttons,ymin),BPoint(xmin+i*(xmax-xmin)/nbuttons,ymax));
	}
	for (int i=0;i<nbuttons;i++) {
		int cnv=i;
		if ((nbuttons==3)&&(i>0)) {
			cnv=3-i;
		}
		if (buttons&((&mmap.left)[cnv])) {
			SetHighColor(128,128,128);
			SetDrawingMode(B_OP_BLEND);
			FillRect(BRect(xmin+i*(xmax-xmin)/nbuttons+1,ymin,xmin+(i+1)*(xmax-xmin)/nbuttons-1,ymax));
		}
		SetHighColor(0,0,0);
		SetDrawingMode(B_OP_OVER);
		int o=(&mmap.left)[cnv];
		o-=o>>2;
		DrawChar('0'+o,BPoint(xmin+(i+.5)*(xmax-xmin)/nbuttons-2,(ymin+ymax)/2+5));
	}
#if 0
	for (int i=0;i<nbuttons;i++) {
		int cnv=i;
		if ((nbuttons==3)&&(i>0)) {
			cnv=3-i;
		}
		if (buttons&((&mmap.left)[cnv])) {
			SetHighColor(255,0,0);
			FillRect(BRect(48/nbuttons*i,0,48/nbuttons*(i+1)-1,15));
		}
		SetHighColor(0,0,0);
		StrokeRect(BRect(48/nbuttons*i,0,48/nbuttons*(i+1)-1,15));
	}
#endif
}

BArchivable*MouseButtonsView::Instantiate(BMessage*m) {
	if (!validate_instantiation(m,"MouseButtonsView")) {
		return NULL;
	}
	return new MouseButtonsView(m);
}
