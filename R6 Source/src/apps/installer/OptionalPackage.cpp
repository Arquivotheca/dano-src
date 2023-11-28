// ------------------------------------------------------------------
// OptionalPackage.cpp
//
//   See OptionalPackage.h
//
//   by Nathan Schrenk (nschrenk@be.com)
// ------------------------------------------------------------------

#include "OptionalPackage.h"
#include <ByteOrder.h>
#include <String.h>
#include <stdio.h>
#include <Message.h>

// constants for attribute names
static const char	*kNameAttr = "INSTALLER PACKAGE: NAME";
static const char 	*kDescAttr = "INSTALLER PACKAGE: DESCRIPTION";
static const char	*kGroupAttr = "INSTALLER PACKAGE: GROUP";
static const char	*kSizeAttr = "INSTALLER PACKAGE: SIZE";
static const char	*kOnAttr = "INSTALLER PACKAGE: ON_BY_DEFAULT";
static const char	*kAlwaysOnAttr = "INSTALLER PACKAGE: ALWAYS_ON";
static const char 	*kIconAttr = "INSTALLER PACKAGE: ICON";

OptionalPackage::OptionalPackage(const char *path, const char *name, const char *desc,
	const char *group, bool on, bool alwayson, off_t size, BBitmap *icon)
{
	this->path = strdup(path);
	this->name = strdup(name);
	this->desc = strdup(desc);
	this->group = strdup(group);
	this->on = on;
	this->alwayson = alwayson;
	this->size = size;
	this->icon = icon;
}


OptionalPackage::~OptionalPackage()
{
	free(path);
	free(name);
	free(desc);
	free(group);
	delete icon;
}

// static function to instantiate an OptionalPackage from the attributes on a BDirectory
OptionalPackage *OptionalPackage::CreateFromEntry(BEntry const &entry)
{
	char buffer[8192];
	BPath path;
	BNode dir;
		
	entry.GetPath(&path);
		
	dir.SetTo(&entry);

	BBitmap *icon(NULL);
	BString pkg_name, pkg_group, pkg_desc; 
	off_t size(0);
	bool on(false), alwayson(false);

	ssize_t rcode;
	bool good = true;
	// read name
	rcode = dir.ReadAttr(kNameAttr, B_STRING_TYPE, 0, buffer, 1024);
	if (rcode > 0)
		pkg_name.Append(buffer);
	else
		good = false;
	// read description
	rcode = dir.ReadAttr(kDescAttr, B_STRING_TYPE, 0, buffer, 1024);
	if (rcode > 0)
		pkg_desc.Append(buffer);
	else
		good = false;
	// read group name
	rcode = dir.ReadAttr(kGroupAttr, B_STRING_TYPE, 0, buffer, 1024);
	if (rcode > 0)
		pkg_group.Append(buffer);
	else
		good = false;
	// read size
	rcode = dir.ReadAttr(kSizeAttr, B_OFF_T_TYPE, 0, &size, sizeof(size));
	size = B_LENDIAN_TO_HOST_INT64(size);
	// read on by default
	rcode = dir.ReadAttr(kOnAttr, B_BOOL_TYPE, 0, &on, sizeof(on));
	// read always on
	rcode = dir.ReadAttr(kAlwaysOnAttr, B_BOOL_TYPE, 0, &alwayson, sizeof(alwayson));
	// read icon - a flattened BMessage containing an archived BBitmap
	rcode = dir.ReadAttr(kIconAttr, B_MESSAGE_TYPE, 0, buffer, 8192);
	if (rcode > 0) {
		BMessage msg;
		msg.Unflatten(buffer);
		icon = (BBitmap *)BBitmap::Instantiate(&msg);
	}

	if (good)
		return new OptionalPackage(path.Path(), pkg_name.String(), pkg_desc.String(), pkg_group.String(), on, alwayson, size, icon);
	else
		return NULL;
}

// writes this OptionalPackage to attributes on the given directory
status_t OptionalPackage::WriteToDirectory(BDirectory *dir)
{
	dir->WriteAttr(kNameAttr, B_STRING_TYPE, 0, name, strlen(name) + 1);
	dir->WriteAttr(kDescAttr, B_STRING_TYPE, 0, desc, strlen(desc) + 1);
	dir->WriteAttr(kGroupAttr, B_STRING_TYPE, 0, group, strlen(group) + 1);
	size = B_HOST_TO_LENDIAN_INT64(size);
	dir->WriteAttr(kSizeAttr, B_OFF_T_TYPE, 0, &size, sizeof(size));
	size = B_LENDIAN_TO_HOST_INT64(size);
	dir->WriteAttr(kOnAttr, B_BOOL_TYPE, 0, &on, sizeof(on));
	dir->WriteAttr(kAlwaysOnAttr, B_BOOL_TYPE, 0, &alwayson, sizeof(alwayson));
	if (icon != NULL) {
		BMessage bmap_msg;
		icon->Archive(&bmap_msg);
		size_t bufsize = bmap_msg.FlattenedSize();
		char *buffer = (char *)malloc(bufsize);
		bmap_msg.Flatten(buffer, bufsize);
		dir->WriteAttr(kIconAttr, B_MESSAGE_TYPE, 0, buffer, bufsize);
	}
	return B_OK;
}

