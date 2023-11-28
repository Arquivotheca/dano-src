//--------------------------------------------------------------------
//	
//	script - obtuse psychotic defribulator
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _MESSENGER_H
#include <Messenger.h>
#endif

class TScriptApp : public BApplication {

public:
					TScriptApp();
virtual		void	ArgvReceived(int32 argc, char** argv);
virtual		void	ReadyToRun();

			void	Do();

			int		fArgc;
			char	*fArgv[3];
};

//====================================================================

int main()
{	
	BApplication* myApp = new TScriptApp();
	myApp->Run();

	delete myApp;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TScriptApp::TScriptApp()
		  :BApplication("application/x-vnd.Be-cmd-SCRT")
{
}

//--------------------------------------------------------------------

void TScriptApp::ArgvReceived(int32 argc, char** argv)
{
	fArgc = argc;
	if (argc != 3)
		fprintf (stderr, "wrong number of arguments\n");
	else {
		fArgv[0] = NULL;
		fArgv[1] = (char *) malloc(strlen(argv[1]) + 1);
		strcpy(fArgv[1], argv[1]);
		fArgv[2] = (char *) malloc(strlen(argv[2]) + 1);
		strcpy(fArgv[2], argv[2]);
	}
}

//--------------------------------------------------------------------

void TScriptApp::Do()
{
	BMessenger*	app;
	BMessage*	msg;
	char		fileName[100];
	FILE*		cmdFile;
	char		command[100];
	char		textCommand[4];
	long		commandVal;
	char*		temp;
	char* 		arg;
	ulong		sig;

	if (fArgc == 3) {
		arg = fArgv[1];
		if (strlen(arg) != 4) {
			printf("script error: illegal app signature '%s'\n", fArgv[1]);
			return;
		}
		memcpy(&sig, arg, sizeof(sig));
		app = new BMessenger(sig);
		if (app->IsValid()) {
			printf("script error: can't find application to script\n");
			delete(app);
			return;
		}

		// see if there is a script command file for this app
		sprintf(fileName, "/boot/system/script/%s", fArgv[1]);
		if (cmdFile = fopen(fileName, "r")) {

			// loop until we find requested command
			while (temp = fgets(command, 100, cmdFile))
				if (strncmp(command, fArgv[2], strlen(fArgv[2])) == 0)
					break;

			if (temp == NULL) {
				printf("script : unknown script command for %s\n", fArgv[1]);
				delete(app);
				fclose(cmdFile);
				return;
			}

			// find first quote character in string
			temp = (char*)strchr(command, '\'');
			if (temp == NULL) {
				printf("script : script command values must be quoted\n");
				delete(app);
				fclose(cmdFile);
				return;
			}

			// skip quote character
			temp++;

			// convert command to integer value
			if (isdigit(*temp))
				commandVal = atol(temp);
			else {
				strncpy(textCommand, temp, 4);
				commandVal = *(long*)textCommand;
			}

			msg = new BMessage(commandVal);
			app->SendMessage(msg);
			fclose(cmdFile);
		}
		delete(app);
	}
}

//--------------------------------------------------------------------

void TScriptApp::ReadyToRun()
{
	Do();
	PostMessage(B_QUIT_REQUESTED);
}
