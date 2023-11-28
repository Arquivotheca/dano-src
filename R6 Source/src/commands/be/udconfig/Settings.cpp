/*
 * Copyright 2000 Be, Incorporated.  All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Settings.h"


SettingsFile::SettingsFile(const char *path) :
	fSettingsLock("SettingsFile list lock"),
	fErr(B_NO_INIT),
	fName(NULL)
{
	fName = strdup(path);
	if(fName == NULL) {
		fErr = B_NO_MEMORY;
		return;
	}

	fErr = B_OK;
}

SettingsFile::~SettingsFile()
{
#warning clear out the list
	free(fName);
}

status_t 
SettingsFile::InitCheck() const
{
	return fErr;
}

const char *
SettingsFile::GetName() const
{
	return fName;
}

BList *
SettingsFile::AcquireSettingsList()
{
	if(!fSettingsLock.Lock()) {
		return NULL;
	}
	return &fSettingsList;
}

void 
SettingsFile::ReleaseSettingsList()
{
	fSettingsLock.Unlock();
}

void
SettingsFile::ClearSettingsList()
{
	BList *list = AcquireSettingsList();
	if(list != NULL) {
		Setting *s;
		int nitems;
		int i;

		nitems = list->CountItems();
		for(i = 0; i < nitems; i++) {
			s = (Setting*)list->ItemAt(i);
			if(s != NULL) {
				free(s->name);
				free(s->value);
				free(s);
			}
		}

		list->MakeEmpty();

		ReleaseSettingsList();
	}
}
