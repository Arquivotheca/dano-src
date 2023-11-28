/*****************************************************************************
 *	Filename: StatusNode.h
 *  Copyright 2001 by Be Incorporated.
 *  
 *  Author: Myron W. Walker  >> others... see change log at end of file...
 *
 *  Description: 
 *****************************************************************************/
 
#include "StatusNode.h"

StatusNode::StatusNode():BinderNode(), mContainer(0)
{
	mContainer = new BinderContainer();
	
	BString dsp("/dev/misc/status");
	
	//Finding Devices and Properties
	FindDevices(dsp);
	
	//Sorting Properties
	mProperties.SortItems(SP_SortOnName);
}

StatusNode::~StatusNode()
{

}

status_t StatusNode::OpenProperties(void **cookie, void *copyCookie)
{
	store_cookie *c = new store_cookie;
	if (copyCookie) {
		*c = *((store_cookie*)copyCookie);
	} else {
		c->index = 0;
	}
	*cookie = c;
	return B_OK;
}

status_t StatusNode::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	status_t err = ENOENT;
	store_cookie *c = (store_cookie*)cookie;
	
	int numProperties = mProperties.CountItems();

	if (c->index < numProperties)
	{	
		StatusProperty * titem = (StatusProperty *)mProperties.ItemAt(c->index);	
		strncpy(nameBuf, (const char *)titem->mName, *len);
		*len = strlen((const char *)titem->mName);
		c->index++;
		err = B_OK;
	}
	
	return err;
}

status_t StatusNode::CloseProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	delete c;
	return B_OK;
}

put_status_t StatusNode::WriteProperty(const char *name, const property &prop)
{
	status_t rtnvalue = B_ERROR;
	
	if(name == NULL) return B_ERROR;

	size_t len = strlen(name) + 1;
	
	SP_Search ss;
	ss.found = false;
	ss.name = new char[len];
	ss.index = 0;
	strncpy(ss.name, name, len);

	mProperties.DoForEach(SP_FindItem, &ss);
	
	if(ss.found)
	{
		StatusProperty *item = (StatusProperty *)mProperties.ItemAt(ss.index);
		if(item == NULL) return B_ERROR;		

		switch(item->mType)
		{
			case typeIsNumber:
				item->SetData(prop.Number());
				rtnvalue = B_OK;
				break;
			case typeIsText:
				BString str = prop.String();
				item->SetData(str.String());
				rtnvalue = B_OK;
				break;
		}
	}
	else rtnvalue = ENOENT;
		
	return rtnvalue;
}

get_status_t StatusNode::ReadProperty(const char *name, property &prop, const property_list &)
{
	status_t rtnvalue = B_ERROR;
	
	if(name == NULL) return B_ERROR;

	prop.Undefine();
		
	size_t len = strlen(name) + 1;
	
	SP_Search ss;
	ss.found = false;
	ss.name = new char[len];
	ss.index = 0;
	strncpy(ss.name, name, len);

	mProperties.DoForEach(SP_FindItem, &ss);
	
	if(ss.found)
	{
		StatusProperty *item = (StatusProperty *)mProperties.ItemAt(ss.index);
		 if(item == NULL) return B_ERROR;		

		switch(item->mType)
		{
			case typeIsNumber:
				{
					int32 value;
					item->GetData(&value);
					prop = property((int)value);
					rtnvalue = B_OK;
				}
				break;
			case typeIsText:
				{
					char value[100];
					size_t len = 100;
					item->GetData(value, &len);
					prop = property(value);	
					rtnvalue = B_OK;
				}
				break;
		}
	}
	else rtnvalue = ENOENT;

	return rtnvalue;
}

void StatusNode::FindDevices(BString curpath)
{
	DIR * d = opendir(curpath.String());
	struct dirent * dent;
	struct stat st;

	if (!d) {
		fprintf(stderr, "error: opendir(%s):\n", curpath.String());
		return;
	}
	
	while ((dent = readdir(d)) != NULL)
	{
		if (dent->d_name[0] == '.') {
			continue;
		}
		
		BString newpath(curpath);
		newpath << "/" << dent->d_name;
		
		if (lstat(newpath.String(), &st) < 0) {
			fprintf(stderr, "error: stat(%s): \n", newpath.String());
			continue;
		}
		if (S_ISDIR(st.st_mode)) {
			FindDevices(newpath);
		}
		else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode))
		{
			RegisterDevice(newpath);		
		}
		else
		{
			fprintf(stderr, "question: what is %s? (%o)\n", newpath.String(), st.st_mode);
		}
	}
	
	closedir(d);
}

void StatusNode::RegisterDevice(BString devpath)
{
	int32 fd;
		
	fd = open(devpath.String(), O_RDWR);
	if (fd >= 0)
	{
		status_get_info gis;
		gis.info_size = sizeof(status_get_info);
		if(ioctl(fd, STATUS_GET_INFO, &gis, sizeof(status_get_info)) == B_NO_ERROR)
		{
			for(int i = 0; i < gis.name_count; i++)
			{ 
				status_get_name_info ni;
				ni.info_size = sizeof(status_get_name_info);
				ni.name_index = i;
				if(ioctl(fd, STATUS_GET_NAME_INFO, &ni, sizeof(status_get_info)) == B_NO_ERROR)
				{
					StatusProperty *np = new StatusProperty(ni.name, devpath.String(), ni.value_type, ni.flags, i);
					mProperties.AddItem(np);
				}
			}
		}
	}
	
	close(fd);
}

