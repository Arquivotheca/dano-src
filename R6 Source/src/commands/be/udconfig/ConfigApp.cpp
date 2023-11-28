/*
	Copyright 2000, Be Incorporated.  All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <Alert.h>
#include <Entry.h>

#include "ConfigWindow.h"
#include "ConfigApp.h"

const char *STR_APP_SIG = "application/x-vnd.Be.UpdateConfig";
const char *STR_APP_NAME = "SettingsConfig";

#define RECOVERY_PATH	"/Recovery/beos/system/boot/recovery.data"
#define UPDATE_PATH		"/BeIA/beos/etc/update/cookie"


ConfigApp::ConfigApp(const char *fname):
	BApplication(STR_APP_SIG),
	fWindow(NULL),
	fFileName(NULL)
{
	if(fname) {
		fFileName = strdup(fname);
	}
}

ConfigApp::~ConfigApp()
{
	free(fFileName);
}


void
ConfigApp::AboutRequested()
{
}


void
ConfigApp::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
	default:
		BApplication::MessageReceived(msg);
		break;
	}
}


bool
ConfigApp::QuitRequested()
{
	return true;
}


void
ConfigApp::ReadyToRun()
{
	if(fFileName == NULL) {
		BAlert *a = new BAlert("Select File", "Would you like to configure Update or Recovery?", "Cancel", "Update", "Recovery",
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_IDEA_ALERT);
		a->SetShortcut(0, B_ESCAPE);
		int32 index = a->Go();
		if(index == 0) {
			PostMessage(B_QUIT_REQUESTED, this);
			return;
		}
		else if(index == 1) {
			fFileName = strdup(UPDATE_PATH);
		}
		else {
			fFileName = strdup(RECOVERY_PATH);
		}
	}

	struct stat st;
	int err;
	err = stat(fFileName, &st);
	if(err < 0) {
		BAlert *a;

		if(fFileName[0] != '/') {
			char *buf = (char*)malloc(strlen(fFileName) + 128);
			sprintf(buf, "The file \"%s\" could not be found.  "
					"Try specifying the file as an absolute path (a path that starts with a '/').", fFileName);
			a = new BAlert("File Not Found", buf, "Stop", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
			free(buf);
			a->Go();
			PostMessage(B_QUIT_REQUESTED, this);
			return;
		}
		else {
			char *c;
			char volume[B_FILE_NAME_LENGTH];
			int vlen;

			c = strchr(fFileName+1, '/');
			if(c) {
				vlen = c - fFileName;
			}
			else {
				vlen = strlen(fFileName);
			}
			memcpy(volume, fFileName, vlen);
			volume[vlen] = '\0';
			err = stat(volume, &st);
			if(err >= 0) {
				char *buf = (char*)malloc(strlen(fFileName) + vlen + 128);
				sprintf(buf, "The file \"%s\" could not be found on the mounted volume \"%s\".", fFileName, volume);
				a = new BAlert("File Not Found", buf, "Stop", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
				free(buf);
				a->Go();
				PostMessage(B_QUIT_REQUESTED, this);
				return;
			}
		}
	}

	fWindow = new ConfigWindow(BRect(100,100,500,225), STR_APP_NAME, fFileName);
	fWindow->Show();
}
