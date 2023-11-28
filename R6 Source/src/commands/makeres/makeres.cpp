//******************************************************************************
//
//	File:			makeres.cpp
//
//	Description:	APPI (Application Info) resource creation tool
//
//	Written by:		Steve Horowitz
//
//	Copyright 1994-95, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#include <stdio.h>
#include <FS.h>
#include <Roster.h>
#include <File.h>
#include <Directory.h>
#include <Volume.h>

/*
	Sample Resource Text File (don't put ## comments in file)

-STARTFILE-
	SHRK								## app signature
	<FLAGS>								## optional flags
	2									## number of document types for app
	TEXT								## document 1 type
	RTFT								## document 2 type
-ENDFILE-

	<FLAGS> is any combination of "+" separated flag strings:	

	SINGLE_LAUNCH | EXCLUSIVE_LAUNCH | MULTIPLE_LAUNCH \
	+ BACKGROUND_APP + ARGV_ONLY
*/

#define SEPCHARS	" +\n"

main(long argc, char* argv[])
{
	FILE*		resText;
	BFile		file;
	BDirectory	root;
	BVolume*	volume;
	char		sigStr[10];
	char		numStr[10];
	char		docTypeStr[10];
	char		flagStr[300];
	char*		word;
	ulong		flags;
	long*		appiBuf;
	long		resSize;
	long		numDocs;
	long		idx;
	long		i;
	int			vol;
	int			dir;

	if (argc != 2) {
		printf("makeres: makeres <res_text_file>\n");
		exit(1);
	}

	if (resText = fopen(argv[1], "r")) {
		// read signature string
		fgets(sigStr, sizeof(sigStr), resText);

		// read flag string and parse
		fgets(flagStr, sizeof(flagStr), resText);
		flags = 0;
		if (strlen(flagStr) > 1) {
			word = (char*)strtok(flagStr, SEPCHARS);
			while (word != NULL) {
				if (strcmp(word, "EXCLUSIVE_LAUNCH") == 0)
					flags = flags | EXCLUSIVE_LAUNCH;
				if (strcmp(word, "MULTIPLE_LAUNCH") == 0)
					flags = flags | MULTIPLE_LAUNCH;
				if (strcmp(word, "BACKGROUND_APP") == 0)
					flags = flags | BACKGROUND_APP;
				if (strcmp(word, "ARGV_ONLY") == 0)
					flags = flags | ARGV_ONLY;

				word = (char*)strtok(NULL, SEPCHARS);
			}
		}

		// read number of documents
		fgets(numStr, sizeof(numStr), resText);
		numDocs = atol(numStr);

		// allocate buffer to hold (long signature, long flags, long numDocs)
		// + numDocs * (sizeof(DOCTYPE) + sizeof(ICONID))
		resSize = sizeof(long) + sizeof(long) + sizeof(long) + (numDocs * 4);
		idx = 0;
		appiBuf = (long*)malloc(resSize);
		appiBuf[idx++] = *(long*)sigStr;
		appiBuf[idx++] = flags;
		appiBuf[idx++] = numDocs;

		for (i = 0; i < numDocs; i++) {
			fgets(docTypeStr, 10, resText);
			appiBuf[idx++] = *(long*)docTypeStr;
		}

		kgetcurvoldir(&vol, &dir);
		volume = volume_for(vol);
		ASSERT(volume);
		
		volume->GetRootDirectory(&root);
		if (root.GetFile("app.resources", &file) != NO_ERROR)
			root.Create("app.resources", &file);
		ASSERT(file.Error() == NO_ERROR);

		file.OpenResources(TRUE);
		ASSERT(file.Error() == NO_ERROR);

		if (file.HasResource("app info", 'APPI'))
			file.ReplaceResource("app info", 'APPI', appiBuf, resSize);
		else
			file.AddResource("app info", 'APPI', appiBuf, resSize);
		ASSERT(file.Error() == NO_ERROR);

		file.CloseResources();
		ASSERT(file.Error() == NO_ERROR);

		free(appiBuf);
		fclose(resText);
	}

	exit(0);
}
