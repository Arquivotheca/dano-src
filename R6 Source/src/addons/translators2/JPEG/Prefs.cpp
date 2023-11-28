//****************************************************************************************
//
//	File:		Prefs.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "Prefs.h"
#include <Path.h>
#include <FindDirectory.h>
#include <File.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const int32 default_quality = 75;
const int32 default_progressive = 0;
extern bool debug;

Prefs::Prefs(const char *name) {
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append(name);
	BFile *file = new BFile();
	file->SetTo(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	node = file;

	quality = LoadInt32("Quality", default_quality);
	progressive = LoadInt32("Progressive", default_progressive);
}

void Prefs::Save() {
	SaveInt32("Quality", quality);
	SaveInt32("Progressive", progressive);
}

Prefs::~Prefs() {
	delete node;
}

int32 Prefs::LoadInt32(const char *name, int32 default_value) {
	int32 value;
	status_t err = node->ReadAttr(name, B_INT32_TYPE, 0, &value, 4);
	if (err >= 0) return value;
	else if (err == B_ENTRY_NOT_FOUND) {
		err = node->WriteAttr(name, B_INT32_TYPE, 0, &default_value, 4);
		if (err >= 0) return default_value;
		else {
			if (debug) printf("LoadInt32(): %s\n", strerror(err));
		}
	} else {
		if (debug) printf("LoadInt32(): %s\n", strerror(err));
	}
	return 0;
}

bool Prefs::SaveInt32(const char *name, int32 value) {
	status_t err = node->WriteAttr(name, B_INT32_TYPE, 0, &value, 4);
	if (err >= 0) return true;
	else if (err == B_FILE_ERROR || err == B_NOT_ALLOWED) {
		if (debug) printf("SaveInt32(): File is read only\n");
	} else {
		if (debug) printf("SaveInt32(): %s\n", strerror(err));
	}
	return false;
}