// ------------------------------------------------------------------
// OptionalPackage.h
//
//   Represents an optionally installed package.
//
//   by Nathan Schrenk (nschrenk@be.com)
// ------------------------------------------------------------------

#ifndef _OPTIONAL_PACKAGE_H_
#define _OPTIONAL_PACKAGE_H_

#include <Bitmap.h>
#include <stdlib.h>
#include <string.h>
#include <SupportDefs.h> // for int32 def
#include <StorageKit.h>

class OptionalPackage
{
public:
	OptionalPackage(const char *path, const char *name, const char *desc, const char *group, bool on,
		bool alwayson, off_t size, BBitmap *icon = NULL);
	~OptionalPackage();

	static OptionalPackage *CreateFromEntry(BEntry const &entry);
	status_t WriteToDirectory(BDirectory *dir);
	
	char *path;		// path to package directory
	char *name;		// package name
	char *desc;		// package description
	char *group;	// this package's group
	bool on;		// whether or not the package is on by default
	bool alwayson;	// whether the package can be deselected
	off_t size;		// approximate size in bytes
	BBitmap *icon;	// icon to display (optional)
	
private:
	OptionalPackage(const OptionalPackage &) {} // unusable copy constructor
};

#endif // _OPTIONAL_PACKAGE_H_
