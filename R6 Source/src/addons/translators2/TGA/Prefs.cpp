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

const int32 default_bits_per_pixel = 32;
const int32 default_compressed = 0;
const int32 default_greyscale = 0;

Prefs::Prefs(const char *name) {
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append(name);
	BFile *file = new BFile();
	file->SetTo(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	node = file;
	
	bits_per_pixel = LoadInt32("BitsPerPixel", default_bits_per_pixel);
	if (bits_per_pixel != 32 && bits_per_pixel != 24 && bits_per_pixel != 16 &&
		bits_per_pixel != 8) bits_per_pixel = 32;
		
	compressed = LoadInt32("Compressed", default_compressed);
	if (compressed != 0 && compressed != 1) compressed = 0;
	
	greyscale = LoadInt32("Greyscale", default_greyscale);
	if (greyscale != 0 && greyscale != 1) greyscale = 0;
}

void Prefs::Save() {
	SaveInt32("BitsPerPixel", bits_per_pixel);
	SaveInt32("Compressed", compressed);
	SaveInt32("Greyscale", greyscale);
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
		else printf("LoadInt32(): %s\n", strerror(err));
	} else printf("LoadInt32(): %s\n", strerror(err));
	return 0;
}

bool Prefs::SaveInt32(const char *name, int32 value) {
	status_t err = node->WriteAttr(name, B_INT32_TYPE, 0, &value, 4);
	if (err >= 0) return true;
	else if (err == B_FILE_ERROR || err == B_NOT_ALLOWED) printf("SaveInt32(): File is read only\n");
	else printf("SaveInt32(): %s\n", strerror(err));
	return false;
}