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
#include "png.h"
#include <Path.h>
#include <FindDirectory.h>
#include <File.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const int32 default_interlacing = PNG_INTERLACE_ADAM7;
const int32 default_res_x = 4000;
const int32 default_res_y = 4000;
const int32 default_res_units = PNG_RESOLUTION_METER;
const int32 default_offset_x = 0;
const int32 default_offset_y = 0;
const int32 default_offset_units = PNG_OFFSET_PIXEL;
extern bool debug;

Prefs::Prefs(const char *name) {
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append(name);
	BFile *file = new BFile();
	file->SetTo(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	node = file;

	interlacing = LoadInt32("Interlacing", default_interlacing);
	res_x = LoadInt32("ResX", default_res_x);
	res_y = LoadInt32("ResY", default_res_y);
	res_units = LoadInt32("ResUnits", default_res_units);
	offset_x = LoadInt32("OffsetX", default_offset_x);
	offset_y = LoadInt32("OffsetY", default_offset_y);
	offset_units = LoadInt32("OffsetUnits", default_offset_units);
}

void Prefs::Save() {
	SaveInt32("Interlacing", interlacing);
	SaveInt32("ResX", res_x);
	SaveInt32("ResY", res_y);
	SaveInt32("ResUnits", res_units);
	SaveInt32("OffsetX", offset_x);
	SaveInt32("OffsetY", offset_y);
	SaveInt32("OffsetUnits", offset_units);
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