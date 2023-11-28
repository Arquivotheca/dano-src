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
#include <TextView.h>
#include <Menu.h>
#include <TranslationUtils.h>

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

	BMessage msg,msg1,msg2,msg3,msg4,msg5,*mmsg;

	msg.AddString("class","JoystickView");
	msg.AddString("_name","joyview");
	msg.AddRect("_frame",BRect(0,0,429,279));
	msg.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg.AddInt32("_color",0xff);
	msg.AddInt32("_color",0xffffffff);
	msg.AddInt32("_color",0xd8d8d8ff);

	msg1.MakeEmpty();
	msg1.AddString("class","BMenuField");
	msg1.AddString("_name","port");
	msg1.AddRect("_frame",BRect(20,50,139,79));
	msg1.AddFloat("_divide",40);
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddString("_label","port :");

	msg2.MakeEmpty();
	msg2.AddString("class","_BMCMenuBar_");
	msg2.AddString("_name","_mc_mb_");
	msg2.AddRect("_frame",BRect(40,0,119,19));
	msg2.AddInt32("_flags",B_WILL_DRAW);
	msg2.AddFloat("_maxwidth",119);
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
	msg4.AddInt32("_flags",B_WILL_DRAW);
	msg4.AddString("_name","<current>");
	msg4.AddBool("_rsize_to_fit",true);
	msg4.AddInt32("_layout",B_ITEMS_IN_COLUMN);
	msg4.AddBool("_radio",true);
	msg4.AddString("_fname","Swis721 BT");
	msg4.AddString("_fname","Roman");
	msg4.AddFloat("_fflt",10);
	msg4.AddFloat("_fflt",-1);
	msg4.AddFloat("_fflt",-1);

	msg3.AddMessage("_submenu",&msg4);

	msg2.AddMessage("_items",&msg3);

	msg1.AddMessage("_views",&msg2);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BButton");
	msg1.AddString("_name","probe");
	msg1.AddRect("_frame",BRect(150,80,224,103));
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	mmsg=new BMessage('prob');
	msg1.AddMessage("_msg",mmsg);
	msg1.AddString("_label","Probe");

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BButton");
	msg1.AddString("_name","calibrate");
	msg1.AddRect("_frame",BRect(150,50,224,73));
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	mmsg=new BMessage('cali');
	msg1.AddMessage("_msg",mmsg);
	msg1.AddString("_label","Calibrate");
	msg1.AddBool("_disable",true);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BStringView");
	msg1.AddString("_name","currentname");
	msg1.AddRect("_frame",BRect(150,110,299,129));
	msg1.AddInt32("_flags",B_WILL_DRAW);

	msg.AddMessage("_views",&msg1);

#if 0
	msg1.MakeEmpty();
	msg1.AddString("class","BView");
	msg1.AddRect("_frame",BRect(20,180,299,181));
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0x808080ff);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BButton");
	msg1.AddString("_name","default");
	msg1.AddRect("_frame",BRect(20,190,94,213));
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddString("_label","Defaults");

	mmsg=new BMessage('dflt');

	msg1.AddMessage("_msg",mmsg);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BButton");
	msg1.AddString("_name","revert");
	msg1.AddRect("_frame",BRect(120,190,194,213));
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddString("_label","Revert");
	msg1.AddBool("_disable",true);

	mmsg=new BMessage('rvrt');
#endif

	msg1.AddMessage("_msg",mmsg);

	msg.AddMessage("_views",&msg1);

	ResMessageAbort(&r,&msg,0,"view");

	ResStringAbort(&r,"Joystick",1000,"add-on name");

#if 0
	BBitmap*b=BTranslationUtils::GetBitmapFile("joystick_icon.tga");
	ResArchivableAbort(&r,b,2000,"icon");
	b=BTranslationUtils::GetBitmapFile("joystick_small_icon.tga");
	ResArchivableAbort(&r,b,2001,"smallicon");
#endif

	PostMessage(B_QUIT_REQUESTED);
}
