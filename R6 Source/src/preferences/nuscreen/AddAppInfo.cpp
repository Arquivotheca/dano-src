/*
01234567890123456789012345678901234567890123456789012345678901234567890123456789
*/

#include <Message.h>
#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <Resources.h>
#include <stdio.h>
#include <AppFileInfo.h>
#include <stdlib.h>

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

AApplication::AApplication():BApplication("application/x-vnd.Be-SetAppInfo-Desktop") {
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
	BAppFileInfo a;
	if (a.SetTo(&f)!=B_OK) {
		printf("error setting app_info for file\n");
		exit(-1);
		return;
	}
	a.SetInfoLocation(B_USE_BOTH_LOCATIONS);
	if (a.SetAppFlags(B_SINGLE_LAUNCH)!=B_OK) {
		printf("error setting app_flags\n");
		exit(-1);
	}
	if (a.SetSignature("application/x-vnd.Be.preferences.desktop")!=B_OK) {
		printf("error setting signature\n");
		exit(-1);
	}
	PostMessage(B_QUIT_REQUESTED);
}
