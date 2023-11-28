/*****************************************************************************
 *	Filename: StatusProperty.cpp
 *  Copyright 2001 by Be Incorporated.
 *  
 *  Description: 
 *****************************************************************************/

#include "StatusProperty.h"

StatusProperty::StatusProperty()
{
	mInitialized = false;
	mName = NULL;
	mDevice = NULL;
}

StatusProperty::StatusProperty(const char *name, const char *device, int32 type, int32 flags, int32 index)
{
	mInitialized = false;
	mName = NULL;
	mDevice = NULL;
	
	if(SetupControlData(name, device, type, flags, index))
		mInitialized = true;	
	else
		mInitialized = false;
}

StatusProperty::~StatusProperty()
{
	if(mName != NULL) delete mName;
	if(mDevice != NULL) delete mDevice;	
}

bool StatusProperty::IsInitialized()
{
	return mInitialized;
}

status_t StatusProperty::GetData(int32 *value)
{
	int32 fd = open(mDevice, O_RDWR);
	if ((fd >= 0) && (mType == typeIsNumber))
	{
		status_name_value nv;
		nv.info_size = sizeof(status_name_value);
		nv.name_index = mIndex;
		if(ioctl(fd, STATUS_GET_NAME_VALUE, &nv, sizeof(status_get_info)) == B_NO_ERROR)
		{
			*value = nv.io_value;	
			close(fd);
			return B_OK;
		}
	}
	
	if(fd >= 0) close(fd);
	
	return B_ERROR;
}

status_t StatusProperty::GetData(char *value, size_t *len)
{
	int32 fd = open(mDevice, O_RDWR);
	if ((fd >= 0) && (mType == typeIsText))
	{
		status_name_value nv;
		nv.info_size = sizeof(status_name_value);
		nv.name_index = mIndex;
		nv.io_value = *len;
		nv.storage = value;
		if(ioctl(fd, STATUS_GET_NAME_VALUE, &nv, sizeof(status_get_info)) == B_NO_ERROR)
		{	
			close(fd);
			return B_OK;			
		}
		else
			*len = nv.io_value;
	}
	
	if(fd >= 0) close(fd);
		
	return B_ERROR;
}

status_t StatusProperty::SetData(int32 value)
{
	int32 fd = open(mDevice, O_RDWR);
	if ((fd >= 0) && (mType == typeIsNumber))
	{
		status_name_value nv;
		nv.info_size = sizeof(status_name_value);
		nv.name_index = mIndex;
		nv.io_value = value;
		if(ioctl(fd, STATUS_SET_NAME_VALUE, &nv, sizeof(status_get_info)) == B_NO_ERROR)
		{
			close(fd);
			return B_OK;
		}
	}
	
	if(fd >= 0) close(fd);
	
	return B_ERROR;
}

status_t StatusProperty::SetData(const char *value)
{
	int32 fd = open(mDevice, O_RDWR);
	if ((fd >= 0) && (mType == typeIsNumber))
	{
		status_name_value nv;
		nv.info_size = sizeof(status_name_value);
		nv.name_index = mIndex;
		nv.io_value = strlen(value) + 1;
		nv.storage = (void *)value;
		if(ioctl(fd, STATUS_SET_NAME_VALUE, &nv, sizeof(status_get_info)) == B_NO_ERROR)
		{
			close(fd);
			return B_OK;
		}
	}
	
	if(fd >= 0) close(fd);
		
	return B_ERROR;
}

status_t StatusProperty::SetTo(const char *name, const char *device, int32 type, int32 flags, int32 index)
{
	status_t rtnvalue;
		
	if(SetupControlData(name, device, type, flags, index))
	{
		rtnvalue = B_OK;
		mInitialized = true;	
	}
	else
	{
		rtnvalue = B_ERROR;
		mInitialized = false;	
		
	}
	
	return rtnvalue;
}

bool StatusProperty::SetupControlData(const char *name, const char *device, int32 type, int32 flags, int32 index)
{
	bool success = true;
	
	mInitialized = false;
	mName = NULL;
	mDevice = NULL;
		
	if(name != NULL)
	{
		int32 len = strlen(name);
		mName = new char[len + 1];
		memset(mName, 0, (len + 1));
		strncpy(mName, name, len);
	}
	else success = false;
	
	if(success && (device != NULL))
	{
		int32 len = strlen(device);
		mDevice = new char[len + 1];
		memset(mDevice, 0, (len + 1));
		strncpy(mDevice, device, len);
		//TODO: Add code to open and check device
	}
	else success = false;
	
	mType = type;
	mFlags = flags;
	mIndex = index;
	
	return success;
}

int SP_SortOnName(const void *itemA, const void *itemB)
{
	StatusProperty **sp_a = (StatusProperty **)itemA;
	StatusProperty **sp_b = (StatusProperty **)itemB;
	
	return strcmp((*sp_a)->mName, (*sp_b)->mName);
}

bool SP_FindItem(void *item, void *ss)
{
	SP_Search* search = (SP_Search *)ss;	
	StatusProperty *sp = (StatusProperty *) item;
	
	if(strcmp(sp->mName, search->name) == 0)
	{	
		search->found = true;
		return true;
	}
	else
		search->index++;
	
	return false;
}
