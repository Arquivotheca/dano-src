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
	MAApplication app;
	app.Run();
	return 0;
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

	msg.AddString("class","KeyboardView");
	msg.AddString("_name","kbview");
	msg.AddRect("_frame",BRect(0,0,429,279));
	msg.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg.AddInt32("_color",0xff);
	msg.AddInt32("_color",0xffffffff);
	msg.AddInt32("_color",0xd8d8d8ff);

	msg1.MakeEmpty();
	msg1.AddString("class","BSlider");
	msg1.AddString("_name","repeatslider");
	msg1.AddRect("_frame",BRect(0,6,179,49));
	msg1.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg1.AddInt32("_flags",B_FRAME_EVENTS|B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddInt32("_color",0xff);
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0xd8d8d8ff);
	msg1.AddString("_label","Key repeat rate");
	msg1.AddInt32("_val",20);
	msg1.AddInt32("_min",20);
	msg1.AddInt32("_max",300);
	msg1.AddString("_minlbl","Slow");
	msg1.AddString("_maxlbl","Fast");
	msg1.AddInt32("_incrementvalue",70);
	msg1.AddInt32("_hashcount",5);
	msg1.AddInt16("_hashloc",B_HASH_MARKS_BOTTOM);
	msg1.AddInt16("_sstyle",B_BLOCK_THUMB);

	mmsg=new BMessage('kbrr');

	msg1.AddMessage("_msg",mmsg);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BSlider");
	msg1.AddString("_name","delayslider");
	msg1.AddRect("_frame",BRect(0,64,179,109));
	msg1.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg1.AddInt32("_flags",B_FRAME_EVENTS|B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddInt32("_color",0xff);
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0xd8d8d8ff);
	msg1.AddString("_label","Delay until key repeat");
	msg1.AddInt32("_val",1);
	msg1.AddInt32("_min",1);
	msg1.AddInt32("_max",4);
	msg1.AddString("_minlbl","Short");
	msg1.AddString("_maxlbl","Long");
	msg1.AddInt32("_incrementvalue",1);
	msg1.AddInt32("_hashcount",4);
	msg1.AddInt16("_hashloc",B_HASH_MARKS_BOTTOM);
	msg1.AddInt16("_sstyle",B_BLOCK_THUMB);

	mmsg=new BMessage('kbde');

	msg1.AddMessage("_msg",mmsg);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BTextControl");
	msg1.AddString("_name","testarea");
	msg1.AddRect("_frame",BRect(8,128,171,147));
	msg1.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddString("_label","label");
	msg1.AddInt32("_a_label",B_ALIGN_CENTER);
	msg1.AddInt32("_a_text",B_ALIGN_CENTER);
	msg1.AddFloat("_divide",0);

	mmsg=new BMessage(0UL);

	msg1.AddMessage("_msg",mmsg);

	msg2.MakeEmpty();
	msg2.AddString("class","_BTextInput_");
	msg2.AddString("_name","_input_");
	msg2.AddRect("_frame",BRect(3,3,160,16));
	msg2.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg2.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE|B_FRAME_EVENTS|B_PULSE_NEEDED);
	msg2.AddRect("_trect",BRect(0,0,153,13));
	msg2.AddString("_text","Typing test area");

	msg1.AddMessage("_views",&msg2);

	msg.AddMessage("_views",&msg1);

	msg1.MakeEmpty();
	msg1.AddString("class","BView");
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

	msg1.MakeEmpty();
	msg1.AddString("class","BButton");
	msg1.AddString("_name","edit");
	msg1.AddRect("_frame",BRect(340,255,429,279));
	msg1.AddInt32("_flags",B_WILL_DRAW|B_NAVIGABLE);
	msg1.AddString("_label","Edit keymap…");

	mmsg=new BMessage('edit');

	msg1.AddMessage("_msg",mmsg);

	msg.AddMessage("_views",&msg1);

	ResMessageAbort(&r,&msg,0,"view");

	ResStringAbort(&r,"Keyboard",1000,"add-on name");

#if 0
	BBitmap*b;
	b=BTranslationUtils::GetBitmapFile("keyboard_icon.tga");
	ResArchivableAbort(&r,b,2000,"icon");
	b=BTranslationUtils::GetBitmapFile("keyboard_small_icon.tga");
	ResArchivableAbort(&r,b,2001,"smallicon");
#endif

	PostMessage(B_QUIT_REQUESTED);
}
