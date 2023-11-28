/*
01234567890123456789012345678901234567890123456789012345678901234567890123456789
*/

#include <Message.h>
#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <Resources.h>
#include <Bitmap.h>
#include <stdio.h>
#include <string.h>
#include <Slider.h>
#include <TranslationUtils.h>
#include <Menu.h>
#include "AddResourceHelpers.h"
#include "PrefsAppExport.h"

class MAApplication:public AApplication {
public:
	MAApplication():AApplication(){}
	void ReadyToRun();
};

int main() {
	(new MAApplication)->Run();
}

void MAApplication::ReadyToRun() {
	BFile f(target,B_READ_WRITE);
	if (f.InitCheck()!=B_OK) {
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	BResources r;
	if (r.SetTo(&f,true)!=B_OK) {
		PostMessage(B_QUIT_REQUESTED);
		return;
	}

	BMessage msg,msg1,msg2,msg3,msg4,msg5;
	BMessage* mmsg;

	msg.MakeEmpty();
	msg.AddString("class","MouseView");
	msg.AddString("_name","mouseview");
	msg.AddRect("_frame",BRect(0,0,429,279));
	msg.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg.AddInt32("_color",0xff);
	msg.AddInt32("_color",0xffffffff);
	msg.AddInt32("_color",0xd8d8d8ff);

	msg1.MakeEmpty();
	msg1.AddString("class","BSlider");
	msg1.AddString("_name","dblclkslider");
	msg1.AddRect("_frame",BRect(0,6,179,63));
	msg1.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg1.AddInt32("_flags",B_FRAME_EVENTS|B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddInt32("_color",0xff);
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0xd8d8d8ff);
	msg1.AddString("_label","Double-click speed");
	msg1.AddInt32("_val",250000);
	msg1.AddInt32("_min",0);
	msg1.AddInt32("_max",400000);
	msg1.AddString("_minlbl","Slow");
	msg1.AddString("_maxlbl","Fast");
	msg1.AddInt32("_incrementvalue",50000);
	msg1.AddInt32("_hashcount",5);
	msg1.AddInt16("_hashloc",B_HASH_MARKS_BOTTOM);
	msg1.AddInt16("_sstyle",B_BLOCK_THUMB);

	mmsg=new BMessage('dclk');

	msg1.AddMessage("_msg",mmsg);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BSlider");
	msg1.AddString("_name","speedslider");
	msg1.AddRect("_frame",BRect(0,64,179,99));
	msg1.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg1.AddInt32("_flags",B_FRAME_EVENTS|B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddInt32("_color",0xff);
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0xd8d8d8ff);
	msg1.AddString("_label","Mouse speed");
	msg1.AddInt32("_val",65536*3);
	msg1.AddInt32("_min",0);
	msg1.AddInt32("_max",65536*6);
	msg1.AddString("_minlbl","Slow");
	msg1.AddString("_maxlbl","Fast");
	msg1.AddInt32("_incrementvalue",32768);
	msg1.AddInt32("_hashcount",5);
	msg1.AddInt16("_hashloc",B_HASH_MARKS_BOTTOM);
	msg1.AddInt16("_sstyle",B_BLOCK_THUMB);

	mmsg=new BMessage('mspd');

	msg1.AddMessage("_msg",mmsg);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BSlider");
	msg1.AddString("_name","accelslider");
	msg1.AddRect("_frame",BRect(0,122,179,149));
	msg1.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg1.AddInt32("_flags",B_FRAME_EVENTS|B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddInt32("_color",0xff);
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0xd8d8d8ff);
	msg1.AddString("_label","Mouse acceleration");
	msg1.AddInt32("_val",256);
	msg1.AddInt32("_min",0);
	msg1.AddInt32("_max",256*3);
	msg1.AddString("_minlbl","Slow");
	msg1.AddString("_maxlbl","Fast");
	msg1.AddInt32("_incrementvalue",32*3);
	msg1.AddInt32("_hashcount",5);
	msg1.AddInt16("_hashloc",B_HASH_MARKS_BOTTOM);
	msg1.AddInt16("_sstyle",B_BLOCK_THUMB);

	mmsg=new BMessage('macc');

	msg1.AddMessage("_msg",mmsg);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BMenuField");
	msg1.AddString("_name","ffm_mf");
	msg1.AddRect("_frame",BRect(0,190,179,209));
	msg1.AddFloat("_divide",110);
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddString("_label","Focus follows mouse :");

	msg2.MakeEmpty();
	msg2.AddString("class","_BMCMenuBar_");
	msg2.AddString("_name","_mc_mb_");
	msg2.AddRect("_frame",BRect(110,3,179,29));
	msg2.AddInt32("_flags",B_WILL_DRAW);
	msg2.AddFloat("_maxwidth",69);
	msg2.AddBool("_rsize_to_fit",true);
	msg2.AddInt32("_border",1);
	msg2.AddString("_fname","Swis721 BT");
	msg2.AddString("_fname","Roman");
	msg2.AddFloat("_fflt",10);
	msg2.AddFloat("_fflt",-1);
	msg2.AddFloat("_fflt",-1);

	msg3.MakeEmpty();
	msg3.AddString("class","_BMCItem_");
	msg3.AddString("_label","menuname");

	msg4.MakeEmpty();
	msg4.AddString("class","BPopUpMenu");
	msg4.AddInt32("_flags",B_WILL_DRAW);
	msg4.AddString("_name","");
	msg4.AddBool("_rsize_to_fit",true);
	msg4.AddInt32("_layout",B_ITEMS_IN_COLUMN);
	msg4.AddBool("_radio",true);
	msg4.AddString("_fname","Swis721 BT");
	msg4.AddString("_fname","Roman");
	msg4.AddFloat("_fflt",10);
	msg4.AddFloat("_fflt",-1);
	msg4.AddFloat("_fflt",-1);

	msg5.MakeEmpty();
	msg5.AddString("class","BMenuItem");
	msg5.AddString("_label","Disabled");

	mmsg=new BMessage('ffm');
	mmsg->AddInt32("ffm",B_NORMAL_MOUSE);

	msg5.AddMessage("_msg",mmsg);

	msg4.AddMessage("_items",&msg5);

	msg5.MakeEmpty();
	msg5.AddString("class","BMenuItem");
	msg5.AddString("_label","Enabled");

	mmsg=new BMessage('ffm');
	mmsg->AddInt32("ffm",B_FOCUS_FOLLOWS_MOUSE);

	msg5.AddMessage("_msg",mmsg);

	msg4.AddMessage("_items",&msg5);

	msg5.MakeEmpty();
	msg5.AddString("class","BMenuItem");
	msg5.AddString("_label","Warping");

	mmsg=new BMessage('ffm');
	mmsg->AddInt32("ffm",B_WARP_MOUSE);

	msg5.AddMessage("_msg",mmsg);

	msg4.AddMessage("_items",&msg5);

	msg5.MakeEmpty();
	msg5.AddString("class","BMenuItem");
	msg5.AddString("_label","Instant Warping");

	mmsg=new BMessage('ffm');
	mmsg->AddInt32("ffm",B_INSTANT_WARP_MOUSE);

	msg5.AddMessage("_msg",mmsg);

	msg4.AddMessage("_items",&msg5);

	msg3.AddMessage("_submenu",&msg4);

	msg2.AddMessage("_items",&msg3);

	msg1.AddMessage("_views",&msg2);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BMenuField");
	msg1.AddString("_name","side_mf");
	msg1.AddRect("_frame",BRect(220,6,399,49));
	msg1.AddFloat("_divide",100);
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddString("_label","Cursor orientation :");

	msg2.MakeEmpty();
	msg2.AddString("class","_BMCMenuBar_");
	msg2.AddString("_name","_mc_mb_");
	msg2.AddRect("_frame",BRect(97,3,179,24));
	msg2.AddInt32("_flags",B_WILL_DRAW);
	msg2.AddFloat("_maxwidth",82);
	msg2.AddInt32("_border",1);
	msg2.AddBool("_rsize_to_fit",true);
	msg2.AddString("_fname","Swis721 BT");
	msg2.AddString("_fname","Roman");
	msg2.AddFloat("_fflt",10);
	msg2.AddFloat("_fflt",-1);
	msg2.AddFloat("_fflt",-1);

	msg3.MakeEmpty();
	msg3.AddString("class","_BMCItem_");
	msg3.AddString("_label","menuname");

	msg4.MakeEmpty();
	msg4.AddString("class","BPopUpMenu");
	msg4.AddString("_name","");
	msg4.AddBool("_rsize_to_fit",true);
	msg4.AddInt32("_flags",B_WILL_DRAW);
	msg4.AddInt32("_layout",B_ITEMS_IN_COLUMN);
	msg4.AddBool("_radio",true);
	msg4.AddString("_fname","Swis721 BT");
	msg4.AddString("_fname","Roman");
	msg4.AddFloat("_fflt",10);
	msg4.AddFloat("_fflt",-1);
	msg4.AddFloat("_fflt",-1);

	msg5.MakeEmpty();
	msg5.AddString("class","BMenuItem");
	msg5.AddString("_label","Right-Handed");

	mmsg=new BMessage('csro');

	msg5.AddMessage("_msg",mmsg);

	msg4.AddMessage("_items",&msg5);

	msg5.MakeEmpty();
	msg5.AddString("class","BMenuItem");
	msg5.AddString("_label","Left-Handed");

	mmsg=new BMessage('csro');

	msg5.AddMessage("_msg",mmsg);

	msg4.AddMessage("_items",&msg5);

	msg3.AddMessage("_submenu",&msg4);

	msg2.AddMessage("_items",&msg3);

	msg1.AddMessage("_views",&msg2);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","MousePointerView");
	msg1.AddString("_name","mpointer");
	msg1.AddRect("_frame",BRect(410,9,425,24));
	msg1.AddInt32("_flags",B_WILL_DRAW);
	msg1.AddInt32("_dmod",B_OP_OVER);
	msg1.AddInt32("_color",0xff);
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0xd8d8d8ff);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BMenuField");
	msg1.AddString("_name","nbutton_mf");
	msg1.AddRect("_frame",BRect(247,35,399,99));
	msg1.AddFloat("_divide",70);
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddString("_label","Mouse Type :");

	msg2.MakeEmpty();
	msg2.AddString("class","_BMCMenuBar_");
	msg2.AddString("_name","_mc_mb_");
	msg2.AddRect("_frame",BRect(70,3,152,29));
	msg2.AddInt32("_flags",B_WILL_DRAW);
	msg2.AddFloat("_maxwidth",82);
	msg2.AddInt32("_border",1);
	msg2.AddBool("_rsize_to_fit",true);
	msg2.AddString("_fname","Swis721 BT");
	msg2.AddString("_fname","Roman");
	msg2.AddFloat("_fflt",10);
	msg2.AddFloat("_fflt",-1);
	msg2.AddFloat("_fflt",-1);

	msg3.MakeEmpty();
	msg3.AddString("class","_BMCItem_");
	msg3.AddString("_label","menuname");

	msg4.MakeEmpty();
	msg4.AddString("class","BPopUpMenu");
	msg4.AddString("_name","");
	msg4.AddBool("_rsize_to_fit",true);
	msg4.AddInt32("_layout",B_ITEMS_IN_COLUMN);
	msg4.AddInt32("_flags",B_WILL_DRAW);
	msg4.AddBool("_radio",true);
	msg4.AddString("_fname","Swis721 BT");
	msg4.AddString("_fname","Roman");
	msg4.AddFloat("_fflt",10);
	msg4.AddFloat("_fflt",-1);
	msg4.AddFloat("_fflt",-1);

	msg5.MakeEmpty();
	msg5.AddString("class","BMenuItem");
	msg5.AddString("_label","1 Button");

	mmsg=new BMessage('nbut');
	mmsg->AddInt32("nbut",1);

	msg5.AddMessage("_msg",mmsg);

	msg4.AddMessage("_items",&msg5);

	msg5.MakeEmpty();
	msg5.AddString("class","BMenuItem");
	msg5.AddString("_label","2 Buttons");

	mmsg=new BMessage('nbut');
	mmsg->AddInt32("nbut",2);

	msg5.AddMessage("_msg",mmsg);

	msg4.AddMessage("_items",&msg5);

	msg5.MakeEmpty();
	msg5.AddString("class","BMenuItem");
	msg5.AddString("_label","3 Buttons");

	mmsg=new BMessage('nbut');
	mmsg->AddInt32("nbut",3);

	msg5.AddMessage("_msg",mmsg);

	msg4.AddMessage("_items",&msg5);

	msg3.AddMessage("_submenu",&msg4);

	msg2.AddMessage("_items",&msg3);

	msg1.AddMessage("_views",&msg2);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","MouseButtonsView");
	msg1.AddString("_name","buttons");
	msg1.AddRect("_frame",BRect(280,84,336,165));
	msg1.AddInt32("_flags",B_WILL_DRAW);
	msg1.AddInt32("_evmsk",B_POINTER_EVENTS);

	msg2.MakeEmpty();
	ArchiveBitmapAbort(&msg2,"mouse_shape.tga");
	msg1.AddMessage("bitmap",&msg2);

	msg1.AddInt32("xmin",2);
	msg1.AddInt32("xmax",50);
	msg1.AddInt32("ymin",10);
	msg1.AddInt32("ymax",25);

	msg2.MakeEmpty();
	msg2.AddString("class","BPopUpMenu");
	msg2.AddString("_name","popup1");
	msg2.AddRect("_frame",BRect(0,0,1,1));
	msg2.AddInt32("_flags",B_WILL_DRAW);
	msg2.AddInt32("_layout",B_ITEMS_IN_COLUMN);
	msg2.AddBool("_rsize_to_fit",true);
	msg2.AddBool("_radio",true);
	msg2.AddBool("_dyn_label",true);

	msg3.MakeEmpty();
	msg3.AddString("class","BMenuItem");
	msg3.AddString("_label","1");

	mmsg=new BMessage('bmap');
	mmsg->AddInt32("bnum",1);
	mmsg->AddInt32("bmap",B_PRIMARY_MOUSE_BUTTON);

	msg3.AddMessage("_msg",mmsg);

	msg2.AddMessage("_items",&msg3);

	msg3.MakeEmpty();
	msg3.AddString("class","BMenuItem");
	msg3.AddString("_label","2");

	mmsg=new BMessage('bmap');
	mmsg->AddInt32("bnum",1);
	mmsg->AddInt32("bmap",B_SECONDARY_MOUSE_BUTTON);

	msg3.AddMessage("_msg",mmsg);

	msg2.AddMessage("_items",&msg3);

	msg3.MakeEmpty();
	msg3.AddString("class","BMenuItem");
	msg3.AddString("_label","3");

	mmsg=new BMessage('bmap');
	mmsg->AddInt32("bnum",1);
	mmsg->AddInt32("bmap",B_TERTIARY_MOUSE_BUTTON);

	msg3.AddMessage("_msg",mmsg);

	msg2.AddMessage("_items",&msg3);

	msg1.AddMessage("menus",&msg2);

	msg2.MakeEmpty();
	msg2.AddString("class","BPopUpMenu");
	msg2.AddString("_name","popup2");
	msg2.AddRect("_frame",BRect(0,0,1,1));
	msg2.AddInt32("_flags",B_WILL_DRAW);
	msg2.AddInt32("_layout",B_ITEMS_IN_COLUMN);
	msg2.AddBool("_rsize_to_fit",true);
	msg2.AddBool("_radio",true);

	msg3.MakeEmpty();
	msg3.AddString("class","BMenuItem");
	msg3.AddString("_label","1");

	mmsg=new BMessage('bmap');
	mmsg->AddInt32("bnum",2);
	mmsg->AddInt32("bmap",B_PRIMARY_MOUSE_BUTTON);

	msg3.AddMessage("_msg",mmsg);

	msg2.AddMessage("_items",&msg3);

	msg3.MakeEmpty();
	msg3.AddString("class","BMenuItem");
	msg3.AddString("_label","2");

	mmsg=new BMessage('bmap');
	mmsg->AddInt32("bnum",2);
	mmsg->AddInt32("bmap",B_SECONDARY_MOUSE_BUTTON);

	msg3.AddMessage("_msg",mmsg);

	msg2.AddMessage("_items",&msg3);

	msg3.MakeEmpty();
	msg3.AddString("class","BMenuItem");
	msg3.AddString("_label","3");

	mmsg=new BMessage('bmap');
	mmsg->AddInt32("bnum",2);
	mmsg->AddInt32("bmap",B_TERTIARY_MOUSE_BUTTON);

	msg3.AddMessage("_msg",mmsg);

	msg2.AddMessage("_items",&msg3);

	msg1.AddMessage("menus",&msg2);

	msg2.MakeEmpty();
	msg2.AddString("class","BPopUpMenu");
	msg2.AddString("_name","popup3");
	msg2.AddRect("_frame",BRect(0,0,1,1));
	msg2.AddInt32("_flags",B_WILL_DRAW);
	msg2.AddInt32("_layout",B_ITEMS_IN_COLUMN);
	msg2.AddBool("_rsize_to_fit",true);
	msg2.AddBool("_radio",true);

	msg3.MakeEmpty();
	msg3.AddString("class","BMenuItem");
	msg3.AddString("_label","1");

	mmsg=new BMessage('bmap');
	mmsg->AddInt32("bnum",3);
	mmsg->AddInt32("bmap",B_PRIMARY_MOUSE_BUTTON);

	msg3.AddMessage("_msg",mmsg);

	msg2.AddMessage("_items",&msg3);

	msg3.MakeEmpty();
	msg3.AddString("class","BMenuItem");
	msg3.AddString("_label","2");

	mmsg=new BMessage('bmap');
	mmsg->AddInt32("bnum",3);
	mmsg->AddInt32("bmap",B_SECONDARY_MOUSE_BUTTON);

	msg3.AddMessage("_msg",mmsg);

	msg2.AddMessage("_items",&msg3);

	msg3.MakeEmpty();
	msg3.AddString("class","BMenuItem");
	msg3.AddString("_label","3");

	mmsg=new BMessage('bmap');
	mmsg->AddInt32("bnum",3);
	mmsg->AddInt32("bmap",B_TERTIARY_MOUSE_BUTTON);

	msg3.AddMessage("_msg",mmsg);

	msg2.AddMessage("_items",&msg3);

	msg1.AddMessage("menus",&msg2);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BView");
	msg1.AddString("name","");
	msg1.AddRect("_frame",BRect(0,244,429,245));
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0x8e8e8eff);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BButton");
	msg1.AddString("_name","default");
	msg1.AddRect("_frame",BRect(8,255,82,279));
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddString("_label","Defaults");

	mmsg=new BMessage('dflt');

	msg1.AddMessage("_msg",mmsg);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BButton");
	msg1.AddString("_name","revert");
	msg1.AddRect("_frame",BRect(93,255,167,279));
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddString("_label","Revert");
	msg1.AddBool("_disable",true);

	mmsg=new BMessage('rvrt');

	msg1.AddMessage("_msg",mmsg);

	msg.AddMessage("_views",&msg1);

	ResMessageAbort(&r,&msg,0,"view");

	ResStringAbort(&r,"Mouse",1000,"add-on name");

#if 0
	BBitmap* b;
	b=BTranslationUtils::GetBitmapFile("mouse_icon.tga");
	ResArchivableAbort(&r,b,2000,"icon");
	b=BTranslationUtils::GetBitmapFile("mouse_small_icon.tga");
	ResArchivableAbort(&r,b,2001,"smallicon");
#endif

	PostMessage(B_QUIT_REQUESTED);
}
