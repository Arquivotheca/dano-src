//  NetworkPanelApp.cpp
//
//	russ 1/21/99
// 
//	(c) 1997-99 Be, Inc. All rights reserved

#include <FindDirectory.h>
#include <File.h>
#include <string.h>
#include <stdlib.h>

#include "FileAndPrintApp.h"
#include "FileAndPrintPanel.h"

main()
{
	FileAndPrintApp app;
	app.Run();

	return B_NO_ERROR;
}

FileAndPrintApp::FileAndPrintApp()
	: BApplication("application/x-vnd.Be.CIFSSETUP")
{
	char path[PATH_MAX];
	BFile* file;
	
	// Check if Network file exists, if it doesn't
	// create it.
	find_directory(B_COMMON_SETTINGS_DIRECTORY, -1, false, path, PATH_MAX);
	strcat(path, "/network");
	
	file = new BFile();
	file->SetTo(path, B_CREATE_FILE | B_FAIL_IF_EXISTS);
	file->Unset();

	new FileAndPrintPanel(); 
}

FileAndPrintApp::~FileAndPrintApp()
{
}





















