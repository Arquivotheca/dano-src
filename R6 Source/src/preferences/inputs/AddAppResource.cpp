/*
01234567890123456789012345678901234567890123456789012345678901234567890123456789
*/

#include <Message.h>
#include <Application.h>
#include <Window.h>
#include <Roster.h>
#include <View.h>
#include <Directory.h>
#include <File.h>
#include <Resources.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class AApplication:public BApplication {
public:
	AApplication();
private:
	void ReadyToRun();
	void ArgvReceived(int32 argc,char** argv);
	char* target;
};

int main() {
	delete new AApplication;
}

AApplication::AApplication():BApplication("application/x-vnd.Be-SetAppResource-Input") {
	target=NULL;
	Run();
}

void AApplication::ArgvReceived(int32 argc,char** argv) {
	if (argc>1) {
		target=new char[strlen(argv[1])+1];
		strcpy(target,argv[1]);
	}
	if (argc>2) printf("ignoring extra parameters\n");
}

void AApplication::ReadyToRun() {
// prepare the BResources object
	if (target==NULL) {
		printf("need a parameter\n");
		exit(-1);
	}
	BFile f(target,B_READ_WRITE);
	if (f.InitCheck()!=B_OK) {
		printf("could not init file\n");
		exit(-1);
		return;
	}
	BResources r;
	if (r.SetTo(&f,true)!=B_OK) {
		printf("error setting resource for file\n");
		exit(-1);
		return;
	}

	BMessage msg,msg1,msg2,msg3,mmsg;
//	rgb_color col;

// create the BWindow
	msg.AddString("class","PPWindow");
	msg.AddString("_name","Inputs");
	msg.AddRect("_frame",BRect(100,100,199,499));
	msg.AddString("_title","Inputs");
	msg.AddInt32("_wlook",B_TITLED_WINDOW_LOOK);
	msg.AddInt32("_wfeel",B_NORMAL_WINDOW_FEEL);
	msg.AddInt32("_flags",B_NOT_ZOOMABLE|B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS|B_PULSE_NEEDED);

// top-level view
	msg1.AddString("class","BView");
	msg1.AddString("_name","backview");
	msg1.AddRect("_frame",BRect(0,0,599,399));
	msg1.AddInt32("_resize_mode",B_FOLLOW_ALL_SIDES);
	msg1.AddInt32("_color",0xff);
	msg1.AddInt32("_color",0xffffffff);
	msg1.AddInt32("_color",0xd8d8d8ff);

// add-on frame

	msg2.MakeEmpty();
	msg2.AddString("class","BView");
	msg2.AddString("_name","addonview");
	msg2.AddRect("_frame",BRect(120,50,120,50));
	msg2.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg2.AddInt32("_flags",B_WILL_DRAW);
	msg2.AddInt32("_color",0xff);
	msg2.AddInt32("_color",0xffffffff);
	msg2.AddInt32("_color",0xd8d8d8ff);

	msg3.MakeEmpty();
	msg3.AddString("class","BStringView");
	msg3.AddString("_name","addontitle");
	msg3.AddRect("_frame",BRect(120,27,299,48));
	msg3.AddInt32("_resize_mode",B_FOLLOW_NONE);
	msg3.AddInt32("_flags",B_WILL_DRAW);

	msg1.AddMessage("_views",&msg3);

	msg1.AddMessage("_views",&msg2);

	msg.AddMessage("_views",&msg1);

	char* buffer=new char[msg.FlattenedSize()];
	if (msg.Flatten(buffer,msg.FlattenedSize())!=B_OK) {
		printf("problem flattening. %d\n",__LINE__);
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	if (r.AddResource(B_MESSAGE_TYPE,0,buffer,msg.FlattenedSize(),"main window")!=B_OK) {
		printf("problem addind. %d\n",__LINE__);
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	delete buffer;

// add the string constants

	char* str;

// globals

// 100 - add-on path
	str="preferences/inputs";
	if (r.AddResource(B_STRING_TYPE,100,str,strlen(str)+1,"add-on path")!=B_OK) exit(-1);

// 101 - settings file. *MUST NOT BE TRANSLATED*
	str="Inputs";
	if (r.AddResource(B_STRING_TYPE,101,str,strlen(str)+1,"settings file")!=B_OK) exit(-1);

// alerts

// 1000-1002 - could not get resources (!) or app_info
	str="No resources/app info";
	if (r.AddResource(B_STRING_TYPE,1000,str,strlen(str)+1,"nores title")!=B_OK) exit(-1);
	str="I could not find the resources or the application info.";
	if (r.AddResource(B_STRING_TYPE,1001,str,strlen(str)+1,"nores text")!=B_OK) exit(-1);
	str="Too bad!";
	if (r.AddResource(B_STRING_TYPE,1002,str,strlen(str)+1,"nores button")!=B_OK) exit(-1);

// 1010-1012 - could not find archived window
	str="No archived window";
	if (r.AddResource(B_STRING_TYPE,1010,str,strlen(str)+1,"noarc title")!=B_OK) exit(-1);
	str="I could not find the archived window in the resources.";
	if (r.AddResource(B_STRING_TYPE,1011,str,strlen(str)+1,"noarc text")!=B_OK) exit(-1);
	str="Too bad!";
	if (r.AddResource(B_STRING_TYPE,1012,str,strlen(str)+1,"noarc button")!=B_OK) exit(-1);

// 1020-1022 - No add-ons
	str="No add-ons";
	if (r.AddResource(B_STRING_TYPE,1020,str,strlen(str)+1,"noadd title")!=B_OK) exit(-1);
	str="I could not find any add-on to use with the preference panel";
	if (r.AddResource(B_STRING_TYPE,1021,str,strlen(str)+1,"noadd text")!=B_OK) exit(-1);
	str="Too bad!";
	if (r.AddResource(B_STRING_TYPE,1022,str,strlen(str)+1,"noadd button")!=B_OK) exit(-1);

// 1030-1032 - Could not initialize window
	str="No window init";
	if (r.AddResource(B_STRING_TYPE,1030,str,strlen(str)+1,"noini title")!=B_OK) exit(-1);
	str="I could not initialize the window";
	if (r.AddResource(B_STRING_TYPE,1031,str,strlen(str)+1,"noini text")!=B_OK) exit(-1);
	str="Too bad!";
	if (r.AddResource(B_STRING_TYPE,1032,str,strlen(str)+1,"noini button")!=B_OK) exit(-1);

	PostMessage(B_QUIT_REQUESTED);
}
