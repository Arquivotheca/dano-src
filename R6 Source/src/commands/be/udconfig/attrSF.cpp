/*
 * Copyright 2000 Be, Incorporated.  All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fs_attr.h>

#include <Node.h>
#include <TypeConstants.h>

#include "attrSF.h"

attrSF::attrSF(const char *name) :
	SettingsFile(name)
{
}

attrSF::~attrSF()
{
}

status_t 
attrSF::Load(int32 flags)
{
	Setting *setting = NULL;
	BList *list;
	status_t err;
	attr_info ai;
	char *attr_value = NULL;
	size_t value_size = 0;
	ssize_t ret;
	int32 next_id = 0;
	int i;
	int n;

	ClearSettingsList();

	list = AcquireSettingsList();
	if(list == NULL) {
		return B_ERROR;
	}

	BNode node(GetName());
	err = node.InitCheck();
	if(err != B_OK) {
		goto exit;
	}

	char attr_name[B_ATTR_NAME_LENGTH];

	while(node.GetNextAttrName(attr_name) == B_OK) {
		err = node.GetAttrInfo(attr_name, &ai);
		if(err != B_OK || ai.type != B_STRING_TYPE) {
			err = B_OK;
			continue;
		}

		if((size_t)ai.size+1 > value_size) {
			char *nbuf = (char*)realloc(attr_value, ai.size+1);
			if(nbuf == NULL) {
				err = B_NO_MEMORY;
				break;
			}
			attr_value = nbuf;
			value_size = (size_t)ai.size+1;
		}

		ret = node.ReadAttr(attr_name, B_STRING_TYPE, 0LL, attr_value, ai.size);
		if(ret <= 0) {
			continue;
		}
		attr_value[ret] = '\0';

		setting = (Setting*)malloc(sizeof(Setting));
		if(setting == NULL) {
			err = B_NO_MEMORY;
			break;
		}
		memset(setting, 0, sizeof(Setting));

		setting->cookie = next_id++;
		setting->name = strdup(attr_name);
		setting->value = strdup(attr_value);
		if(setting->name == NULL || setting->value == NULL) {
			err = B_NO_MEMORY;
			break;
		}

		n = list->CountItems();
		for(i = 0; i < n; i++) {
			if(strcmp(((Setting*)list->ItemAt(i))->name, setting->name) > 0) {
				break;
			}
		}
		if(!list->AddItem((void*)setting, i)) {
			err = B_NO_MEMORY;
			break;
		}
		setting = NULL;
	}

	if(setting) {
		free(setting->name);
		free(setting->value);
		free(setting);
		setting = NULL;
	}

exit:
	ReleaseSettingsList();

	return err;
}

status_t 
attrSF::Save(int32 flags)
{
	Setting *setting = NULL;
	BList *list;
	status_t err;
	int nitems;
	int i;

	list = AcquireSettingsList();
	if(list == NULL) {
		return B_ERROR;
	}

	BNode node(GetName());
	err = node.InitCheck();
	if(err != B_OK) {
		goto exit;
	}

	nitems = list->CountItems();
	for(i = 0; i < nitems; i++) {
		setting = (Setting*)list->ItemAt(i);
		if(setting == NULL) {
			continue;
		}
		node.WriteAttr(setting->name, B_STRING_TYPE, 0LL, setting->value, strlen(setting->value)+1);
	}

	err = B_OK;
exit:
	ReleaseSettingsList();

	return err;
}
