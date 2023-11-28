
#include <OS.h>
#include <stdlib.h>
#include <stdio.h>
#include <Binder.h>
#include <Entry.h>
#include <File.h>
#include <Directory.h>
#include <Path.h>
#include "UserNode.h"

#define USER_RECORD_SKEL	"/boot/home/config/settings/binder/user-record.skel"
#define USER_RECORD			"user-record"

UserNode::UserNode(const BString &name, const BDirectory &dir) : m_name(name)
{
	BFile file;
	BEntry entry;
	BPath path;

	if (!dir.FindEntry(USER_RECORD,&entry) && entry.Exists())
		file.SetTo(&entry,O_RDONLY);
	else 
		file.SetTo(USER_RECORD_SKEL,O_RDONLY);

	if (!file.InitCheck()) Load(file);
	
	file.Unset();
	
	if (dir.GetEntry(&entry) || entry.GetPath(&path)) return;
	path.Append(USER_RECORD);
	SetPathname(path.Path());
}

void 
UserNode::SetName(const char *name)
{
	m_name = name;
	BPath path = Pathname();
	path.GetParent(&path);
	path.GetParent(&path);
	path.Append(name);
	path.Append(USER_RECORD);
	SetPathname(path.Path(),false,true);
	NotifyListeners(B_PROPERTY_CHANGED,"name");
}

put_status_t 
UserNode::WriteProperty(const char *name, const property &prop)
{
	if (!strcmp(name,"name")) return EPERM;
	return XMLBinderNode::WriteProperty(name,prop);
}

get_status_t 
UserNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (!strcmp(name,"name")) {
		prop = m_name;
		return B_OK;
	}
	return XMLBinderNode::ReadProperty(name,prop,args);
}
