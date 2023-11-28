/*****************************************************************************
 *	Filename: StatusProperty.h
 *  Copyright 2001 by Be Incorporated.
 *  
 *  Description: 
 *****************************************************************************/

#ifndef _STATUSPROPERTY_H_INCLUDED_
#define _STATUSPROPERTY_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <String.h>

#include "DeviceStatusProtocol.h"

typedef struct _SP_Search
{
	bool found;
	char *name;
	int32 index;
} SP_Search;

int SP_SortOnName(const void *itemA, const void *itemB);
bool SP_FindItem(void *item, void *ss);

class StatusProperty
{
	friend int SP_SortOnName(const void *, const void *);
	friend bool SP_FindItem(void *item, void *ss); 
	friend class StatusNode;
	public:
		StatusProperty();
		StatusProperty(const char *name, const char *device, int32 type, int32 flags, int32 index);
		virtual ~StatusProperty();

		bool IsInitialized();
		status_t GetData(int32 *value);
		status_t GetData(char *value, size_t *len);
		status_t SetData(int32 value);
		status_t SetData(const char *value);
		status_t SetTo(const char *name, const char *device, int32 type, int32 flags, int32 index);
	private:
		bool mInitialized;
		
		char *mName;
		char *mDevice;
		int32 mIndex;
		int32 mFlags;
		int32 mType;

		bool SetupControlData(const char *name, const char *device, int32 type, int32 flags, int32 index);
};

#endif _STATUSPROPERTY_H_INCLUDED_
