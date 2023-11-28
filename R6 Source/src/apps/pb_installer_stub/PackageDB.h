#ifndef _PACKAGEDB_H_
#define _PACKAGEDB_H_

#define _EXPORTED_ 0
#include <Entry.h>
#include <Directory.h>
#include <VolumeRoster.h>

class PackageItem;

class PackageDB
{
public:
			PackageDB();
			~PackageDB();
	
	// get entry for the database dir on a specified volume
	// returns error if database directory doesn't exist
	long 	DBDir(BEntry *dirEntry, dev_t dev);
	// return true if specified volume has database directory false otherwise
	bool	HasDB(dev_t devID);
	// create database directory on specifed volume
	long	CreateDB(dev_t devID, BEntry *result);
	
	// not implemented
	long	CountPackages();
	// rewind to first volume database
	void	Rewind();
	
	enum {
		ALL_DATA = -1,
		BASIC_DATA = 0x00000001,
		UPDATE_DATA = 0x00000002
	};
	
	// get next package (from all volumes or on a specific volume)
	// PackageItem must be allocated
	long	GetNextPackage(PackageItem *item, int32 data = ALL_DATA, dev_t volID = -1);
	
	// find a first package (on any volume, later allow specific volume)
	long	FindPackage(PackageItem *item,
							const char *name,
							const char *version = NULL,
							const char *developer = NULL,
							dev_t device = -1);
	long 	FindSerialID(PackageItem *item,
							const char *serialID,
							const char *prefixID,
							dev_t device = -1);
	//long	FindPackage(PackageItem *item, BMessage *message);
	long	FindNextOlderVersion(PackageItem *finalItem,
									const char *name,
									const char *developer,
									time_t	date,
									dev_t	device = -1);				
	// add a package on the specified volume, boot as default
	// creates database directory if non-existent
	long	AddPackage(PackageItem *item, dev_t volID = -1);
	
	// write an existing package item back out to the database
	// item must be a valid package item that was read or added to the db
	long	WritePackage(PackageItem *item, int32 data = ALL_DATA);
	
	// not implemented
	long	RemovePackage(PackageItem *item, dev_t volID = -1);
	
	// not implemented
	long	DependantPackages(PackageItem *item,
								BList	*dependants,
								dev_t volID = -1);
								
	long	ReadPackage(PackageItem *,BEntry *, int32 data = ALL_DATA);
	
	
	static bool  MapOldPackageName(const char *name,
					char *newname,
					char *depotsn,
					int32 &media,
					int32 &softtype,
					bool  &serivce,
					char *vid);
private:
	BVolumeRoster	volRost;
	// current iteration directory
	BDirectory		iterDir;
	// if iterating on a single volume?
	bool			singleRewind;
	static const	char kDirLoc[];
};


#endif
