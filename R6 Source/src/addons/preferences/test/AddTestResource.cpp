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
#include "AddResourceHelpers.h"
#include <View.h>

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

	BMessage msg,msg1,msg2,msg3,mmsg;

	msg.AddString("class","TestView");
	msg.AddString("_name","testview");
	msg.AddRect("_frame",BRect(0,0,199,199));
	msg.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg.AddInt32("_color",0xff);
	msg.AddInt32("_color",0xffffffff);
	msg.AddInt32("_color",0x0000ffff);

	msg1.MakeEmpty();
	msg1.AddString("class","BView");
	msg1.AddString("_name","");
	msg1.AddRect("_frame",BRect(50,50,149,149));
	msg1.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg1.AddInt32("_color",0xff);
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0xff00ffff);

	msg.AddMessage("_views",&msg1);

	ResMessageAbort(&r,&msg,0,"view");

	ResStringAbort(&r,"A better test",1000,"add-on name");

	BBitmap* b=new BBitmap(BRect(0,0,31,31),B_CMAP8);
	memset(b->Bits(),231,1024);
	ResArchivableAbort(&r,b,2000,"icon");

	PostMessage(B_QUIT_REQUESTED);
}
