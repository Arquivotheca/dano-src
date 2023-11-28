// PackageDB.cpp

// can be on multiple volumes

// make package list

// add package

// check for existing package

// remove package

// uses packages

// get package -- package item


// name string
// version string
// developer string

#include "PackageDB.h"
#include "AttrIO.h"
#include "SResIO.h"

#include "AutoPtr.h"
#include "PackageItem.h"
#include "Util.h"
#include <ctype.h>
#include <NodeInfo.h>

#include "MyDebug.h"

#if (!TRANSCEIVER_BUILD)
#include "InstallerType.h"
#endif

long CreatePathAt(const BDirectory *dir,const char *path,BEntry *final);

const	char PackageDB::kDirLoc[] = "home/config/settings/packages";


PackageDB::PackageDB()
{
	Rewind();
};


PackageDB::~PackageDB()
{

};

// get entry for the database dir on a specified volume
long PackageDB::DBDir(BEntry *dirEntry, dev_t dev)
{	
	BVolume	vol(dev);
	//BPath	thePath;
	
	// find dir on volume, create it if needed
	//find_directory(B_COMMON_SETTINGS_DIRECTORY,&thePath,true,&vol);

	BDirectory	dir;	
	vol.GetRootDirectory(&dir);
	if (dir.FindEntry(kDirLoc,	dirEntry) == B_NO_ERROR &&
		dir.SetTo(dirEntry) == B_NO_ERROR) {
		PRINT(("Got %s\n",kDirLoc));
		return B_NO_ERROR;
	}
	
	PRINT(("Did not find %s\n",kDirLoc));
	return B_ERROR;
}

// does the specified volume have a package database
bool PackageDB::HasDB(dev_t devID)
{
	BEntry	dirEnt;
	long err = DBDir(&dirEnt, devID);

	return (err == B_NO_ERROR);
}

// creates package registry on specified volume
// returns error status
long PackageDB::CreateDB(dev_t volID, BEntry *result)
{
	PRINT(("PackageDB::CreateDB\n"));
	long err = B_NO_ERROR;
	
	BVolume	vol(volID);
	
	BDirectory	dir;
	vol.GetRootDirectory(&dir);
	return CreatePathAt(&dir,kDirLoc,result);
}


void PackageDB::Rewind()
{
	status_t err = B_NO_ERROR;
	
#if DEBUG
	PRINT(("Iterating through volumes ---\n"));

	volRost.Rewind();
	BVolume		curVol;
	do {
		err = volRost.GetNextVolume(&curVol);
		if (err < B_NO_ERROR)
			break;
		if (!curVol.IsPersistent()) 
			continue;
		BDirectory rootDir;
		curVol.GetRootDirectory(&rootDir);
		BEntry	e;
		rootDir.GetEntry(&e);
		char path[128];
		path[0] = '/';
		e.GetName(path+1);
		PRINT(("%s\n",path));
	} while (true);
	PRINT(("Finished Iterating through volumes ---\n"));

#endif

	volRost.Rewind();

	singleRewind = TRUE;
	
	
	// set the iter directory to an invalid dir
	entry_ref	ent;
	ent.directory = -1;
	ent.device = -1;
	iterDir.SetTo(&ent);
}

long PackageDB::CountPackages()
{
	// !!
	PRINT(("PackageDB::CountPackages NOT IMPLEMENTED\n"));
	return 0;
}

long PackageDB::GetNextPackage(PackageItem *item,  int32 data, dev_t volID)
{
	PRINT(("PackageDB::GetNextPackage\n"));
	long err;
	bool notFound = TRUE;
	bool singleVol = FALSE;

	// the rewind was called and we are with a single volume
	if (volID >= 0) {
		PRINT(("searching in single volume mode\n"));
		singleVol = TRUE;
		if (singleRewind) {
			PRINT(("rewind peformed, setting iter dir\n"));
			singleRewind = FALSE;
			BEntry iterEntry;
			err = DBDir(&iterEntry, volID);
			iterDir.SetTo(&iterEntry);
			if (err < B_NO_ERROR)
				return B_ERROR;
			PRINT(("iter dir successfully set\n"));
		}
	}	
	while(notFound) {
		BEntry	recEntry;
		err = iterDir.GetNextEntry(&recEntry);
		if (err >= B_NO_ERROR) {
			char fn[B_FILE_NAME_LENGTH];
			recEntry.GetName(fn);
			PRINT(("got entry %s\n",fn));
			
			// inefficient
			err = ReadPackage(item,&recEntry,data);
			if (err == B_NO_ERROR) {
				// we got it!
				PRINT(("read package returned no error\n"));
				notFound = FALSE;
			}
			// keep looping through this directory
		}
		else if (!singleVol) {
			// switch to next dir
			BVolume		curVol;
			bool		isDBVol = false;
			// get the next volume with a database dir
			// set the iter directory
			do {
				err = volRost.GetNextVolume(&curVol);
				isDBVol = (curVol.IsPersistent() && HasDB(curVol.Device()));
			} while(err == B_NO_ERROR && !isDBVol);
			// if we didn't run out of volumes
			if (err >= B_NO_ERROR && isDBVol) {
				BEntry iterEntry;
				err = DBDir(&iterEntry, curVol.Device());
				if (err < B_NO_ERROR) {
					PRINT(("no more entries\n"));
					notFound = FALSE;
					break;
				}
				err = iterDir.SetTo(&iterEntry);
				if (err < B_NO_ERROR) {
					PRINT(("no more entries\n"));
					notFound = FALSE;
				}
			}
			else {
				notFound = FALSE;
			}
		}
		else {
			notFound = FALSE;
		}
	}
	// no package found or not found
	return err;
}

// version and developer can be null
// if multiple with same name???
long PackageDB::FindPackage(PackageItem *item,
							const char *name,
							const char *version,
							const char *developer,
							dev_t dev)
{
	// once implemented, use a query!
	PackageItem	found;
	long err = B_ERROR;
	Rewind();
	while((err = GetNextPackage(&found,BASIC_DATA,dev)) >= B_NO_ERROR) {
		if ((strcasecmp(found.data.FindString("package"),name)) == 0 &&
				(version ? (strcmp(found.data.FindString("version"),version) == 0) : TRUE) &&
				(developer ? (strcasecmp(found.data.FindString("developer"),developer) == 0) : TRUE)) {
			*item = found;
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

long PackageDB::FindSerialID(PackageItem *item,
							const char *serialID,
							const char *prefixID,
							dev_t dev)
{
	if (serialID == NULL || prefixID == NULL)
		return B_ERROR;
	
	PackageItem	found;	
	status_t err;
	
	Rewind();
	while((err = GetNextPackage(&found,BASIC_DATA,dev)) >= B_NO_ERROR)
	{
		if ((strcasecmp(found.data.FindString("sid"),serialID)) == 0 &&
			(strcasecmp(found.data.FindString("pid"),prefixID)) == 0)
		{
			*item = found;
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

long PackageDB::FindNextOlderVersion(PackageItem *finalItem,
									const char *name,
									const char *developer,
									time_t	date,
									dev_t dev)
{
	char *vers = 0;
	time_t	smallestDiff = INT_MAX;

	long err = B_ERROR;
	Rewind();
	
	// temp item
	PackageItem		item;
	while(GetNextPackage(&item,BASIC_DATA,dev) >= B_NO_ERROR) {
		if ((strcasecmp(item.data.FindString("package"),name)) == 0 &&
			(developer ? (strcasecmp(item.data.FindString("developer"),developer) == 0) : TRUE)) {
			
			// we have a candidate
			time_t diff = date - item.data.FindInt32("releasedate");
			if (diff > 0 && diff < smallestDiff) {
				smallestDiff = diff;
				*finalItem = item;
				err = B_NO_ERROR;
			}
		}
	}
	return err;
}

bool PackageDB::MapOldPackageName(const char *name,
					char *newname,
					char *depotsn,
					int32 &media,
					int32 &softtype,
					bool &services,
					char *vid)
{
	// mediatype 0 esd, 1 physical
	// softtype 0 commerical, 1 trial, 2 free, 3 shareware, 4 beta, 255 other 
	struct pdesc{
		const char *name;
		int mediatype;
		int softtype;
		const char *newname;
		const char *depotsn;
		const char *vid;
	} oldpackages[] = {
		{"Adam", 0 , 0 ,"Adam","7050414010526337","0"} ,
		{"Adam ESD", 0 , 0 ,"Adam","7050414010526337","0"} ,
		{"Be Basics ESD", 0 , 0 ,"Be Basics","8050410020929326","0"} ,
		//{"Be Basics", 0 , 1 ,"Be Basics","8050410020929326"} , not migrated
		{"Be Basics CD", 0 , 0 ,"Be Basics","8050410020929326","0"} ,
		{"Be Basics Limited", 0 , 1 ,"Be Basics","8050410020929326", "1420423"} ,
		{"Be Basics Limited (Be CD)", 0 , 1 ,"Be Basics","8050410020929326", "1420423"} ,
		{"Be Studio ESD", 0 , 0 ,"Be Studio","8050410020929326","0"} ,
		//{"Be Studio", 0 , 1 ,"Be Studio","8050410020929326"} , not migrated
		{"Be Studio CD", 0 , 0 ,"Be Studio","8050410020929326","0"} ,
		{"Be Studio Limited", 0 , 1 ,"Be Studio","8050410020929326","1479440"} ,
		{"Be Studio Limited (Be CD)", 0 , 1 ,"Be Studio","8050410020929326","1479440"} ,
		{"Becasso ESD", 0 , 0 ,"Becasso","0050410010429403","0"} ,
		{"Becasso", 0 , 0 ,"Becasso","0050410010429403","0"} ,
		{"GraphicConverter ESD", 0 , 0 ,"GraphicConverter","4000712000124450","0"} ,
		{"GraphicConverter Limited", 0 , 1 ,"GraphicConverter","4000712000124450","1639586"} ,
		{"h.Scribe", 0 , 0 ,"h.Scribe","2090413000127451","0"} ,
		{"h.Scribe_DEMO", 0 , 1 ,"h.Scribe","2090413000127451","1492664"} ,
		{"Mail-It ESD", 0 , 0 ,"Mail-It","8050410020929326","0"} ,
		{"BeatWare Mail-It ESD", 0, 0, "Mail-It","8050410020929326","0"},
		{"Mail-It Limited", 0 , 1 ,"Mail-It","8050410020929326","1535917"} ,
		{"BeatWare Mail-It Limited", 0, 1, "Mail-It","8050410020929326","1535917"},
		{"ObjektSynth ESD", 0 , 0 ,"ObjektSynth","5020516070125385","0"} ,
		{"pe", 0 , 0 ,"Pe","0040615000526398","0"} ,
		{"pe demo", 0 , 1 ,"Pe","0040615000526398","1627554"} ,
		{"Pe ESD", 0 , 0 ,"Pe","0040615000526398","0"} ,
		{"Pe Limited", 0 , 1 ,"Pe","0040615000526398","1627554"} ,
		//{"roColour", 0 , 1 ,"roColour","5050317060120482",""} , not migrated
		{"roColour2Web", 0 , 0 ,"roColour","5050317060120482","0"} ,
		{"Spellswell Plus ESD", 0 , 0 ,"Spellswell Plus","9090715030128309","0"} ,
		{"PackageBuilder", 0 , 2 ,"PackageBuilder","3030819070327364","1742397"} ,
		{"SoftwareValet", 0 , 2 ,"SoftwareValet","3030819070327364","1344480"} 
	};
#if 1
	for (int i = 0; i < nel(oldpackages); i++) {
		if (strcasecmp(name,oldpackages[i].name) == 0) {
			if (oldpackages[i].newname)
				strcpy(newname, oldpackages[i].newname);
			else
				strcpy(newname, name);
			media = oldpackages[i].mediatype;
			softtype = oldpackages[i].softtype;
			strcpy(depotsn,oldpackages[i].depotsn);
			strcpy(vid,oldpackages[i].vid);
			services = true;
			return true;
		}
	}
#endif

#if (!__INTEL__)
	// look for Limited, ESD or DEMO suffixes
	const char *c = name;
	while (*c) c++;
	while (c > name && (!isspace(*c) || !*c)) {
		c--;
	}
	// either space or beginning of string
	const char *suf = c;
	if (suf > name && *suf)
		suf++;
	
	if (strcasecmp(suf,"Limited") == 0) {
		softtype = 1;	
		memcpy(newname,name,c-name);
		newname[c-name] = 0;
		return true;
	}
	else if (strcasecmp(suf,"ESD") == 0) {
		softtype = 0;
		memcpy(newname,name,c-name);
		newname[c-name] = 0;
		return true;
	}
	else if (strcasecmp(suf,"Demo") == 0) {
		softtype = 1;
		memcpy(newname,name,c-name);
		newname[c-name] = 0;
		return true;
	}
#endif
	
	return false;
}	

long PackageDB::ReadPackage(PackageItem *item, BEntry *recEntry, int32 data)
{
	item->fRemapped = false;
	BFile	pkgFile(recEntry,O_RDONLY);
	
	if (pkgFile.InitCheck() < B_NO_ERROR)
		return B_ERROR;

	BMessage	temp;
	SResIO		resIO;
	if (resIO.SetTo(&pkgFile,'info',1) >= B_OK) {
	
		if (data & BASIC_DATA)
		{
			PRINT(("reading basic data\n"));
			if (temp.Unflatten(&resIO) < B_OK) {
				PRINT(("unflatten error\n"));
				return B_ERROR;
			}
			if (MergeMessage(item->data,temp) < B_OK) {
				PRINT(("merge message error\n"));
				return B_ERROR;
			}
#if DEBUG
			item->data.PrintToStream();
#endif
		}
		// update info
		if ((data & UPDATE_DATA) && resIO.SetTo(&pkgFile,'updt',1) >= B_OK)
		{
			PRINT(("reading update data\n"));
			if (item->updates.Unflatten(&resIO) < B_OK) {
				PRINT(("unflatten error\n"));
				return B_ERROR;
			}
			item->fFilled = false;
		}
		// installation info
		//if (resIO.SetTo(&pkgFile,'inst',1) >= B_OK)
	}
	else {
		AttrIO	aio(&pkgFile);
		
		PRINT(("db file opened\n"));
		//pkgFile.Lock();
		PRINT(("db file locked\n"));
			
		//				
		status_t err;
	
		// frees strings
		const int bufsz = 512;
		char buf[bufsz];
		
		err = aio.ReadStringAttr("pkgname",buf,bufsz);
		
		// map old package info
		char newname[128];
		char depotsn[32];
		char vid[32];
		int32 media, soft;
		bool	service;
		
		// set defaults
		depotsn[0] = 0;	// no serial
		media = 0;		// esd
		soft = 255;		// unknowntype
		service = false;	// no services
		if (MapOldPackageName(buf,newname,depotsn,media,soft,service,vid)) {
			if (service) {
				item->fRemapped = true;
				ReplaceString(&item->data,"pid","10000");
				ReplaceString(&item->data,"vid",vid);
				if (soft == 0)
					ReplaceString(&item->data,"sid","old");
			}
			ReplaceString(&item->data,"package",newname);
		}
		else
			ReplaceString(&item->data,"package",buf);
			
		ReplaceInt32(&item->data,"mediatype",media);
		ReplaceInt32(&item->data,"softtype",soft);
		ReplaceString(&item->data,"depotsn",depotsn);
		ReplaceBool(&item->data,"regservice",service);
		ReplaceBool(&item->data,"upservice",service);
		
		err = aio.ReadStringAttr("pkgvers",buf,bufsz);
		ReplaceString(&item->data,"version",buf);
	
		err = aio.ReadStringAttr("pkgdeveloper",buf,bufsz);
		ReplaceString(&item->data,"developer",buf);
	
		err = aio.ReadStringAttr("pkgdescription",buf,bufsz);
		ReplaceString(&item->data,"description",buf);

		
		//err = aio.ReadStringAttr("pkgnewversion",buf,bufsz);
		
		//if (!item->newVersion) {
		//	item->newVersion = strdup(B_EMPTY_STRING);
		//}
		
		//err = aio.ReadStringAttr("pkgnewurl",&item->newURL); // may be null
		int32 data32;
		//int16 data16;
		
		err = aio.ReadInt32Attr("pkgreleasedate",&data32);
		ReplaceInt32(&item->data,"releasedate",data32);
		
		err = aio.ReadInt32Attr("pkginstalldate",&data32);
		ReplaceInt32(&item->data,"installdate",data32);
		
		int16 data16;
		err = aio.ReadInt16Attr("pkgregistered",&data16);
		ReplaceInt32(&item->data,"registered",data16);
		//err = aio.ReadStringAttr("pkgregsn",buf,bufsz);

		
		err = aio.ReadInt32Attr("pkginstallsize",&data32);
		ReplaceInt64(&item->data,"installsize",data32);
		
		err = aio.ReadInt32Attr("pkgnewreleasedate",&data32);
		ReplaceInt64(&item->data,"releasedate",data32);
		
		recEntry->GetRef(&item->fileRef);
		WritePackage(item,BASIC_DATA);
	//	err = aio.ReadInt32Attr("pkgnewsize",(int32 *)&item->newSize);
	//	err = aio.ReadInt32Attr("pkgdownloadeddate",(int32 *)&item->downloadedDate);
	//	err = aio.ReadInt32Attr("pkgupdtdlstate",&item->downloadState);
	}
	recEntry->GetRef(&item->fileRef);
	
	//if (!len) return B_ERROR;
	//pkgFile.Unlock();

	return B_NO_ERROR;
}


long PackageDB::WritePackage(PackageItem *item, int32 data)
{
	long err = B_NO_ERROR;
	BFile pkgFile(&item->fileRef,O_RDWR);
	
	if (pkgFile.InitCheck() < B_NO_ERROR) {
		PRINT(("couldn't open package file\n"));
		return B_ERROR;
	}
	
	SResIO	resIO;
	
	if ((data & BASIC_DATA) && resIO.SetTo(&pkgFile,'info',1) >= B_OK) {
		PRINT(("Writing basic data\n"));
		if (item->data.Flatten(&resIO) < B_OK) {
			PRINT(("error writing basic data\n"));
			return B_ERROR;
		}
	}
	if ((data & UPDATE_DATA) && resIO.SetTo(&pkgFile,'updt',1) >= B_OK) {
		PRINT(("Writing update data\n"));
		if (item->updates.Flatten(&resIO) < B_OK) {
			PRINT(("error writing update data\n"));
			return B_ERROR;
		}
	}
		
		// update info
		//if (resIO.SetTo(&pkgFile,'updt',1) >= B_OK) {
		//
		//}
		// installation info
		//if (resIO.SetTo(&pkgFile,'inst',1) >= B_OK)

	
#if 0

	AttrIO	aio(&pkgFile);


	
	PRINT(("db file opened\n"));
	pkgFile.Lock();
	PRINT(("db file locked\n"));



	aio.WriteStringAttr("pkgname",item->name);
	aio.WriteStringAttr("pkgvers",item->version);
	aio.WriteStringAttr("pkgdeveloper",item->developer);
	aio.WriteStringAttr("pkgdescription", item->description);
	aio.WriteStringAttr("pkgnewversion", item->newVersion);
	aio.WriteStringAttr("pkgnewurl",item->newURL);
	
	aio.WriteInt32Attr("pkgreleasedate",item->releaseDate);
	aio.WriteInt32Attr("pkginstalldate",item->installDate);
	aio.WriteInt16Attr("pkgregistered",item->registered);
	aio.WriteStringAttr("pkgregsn",item->regSerial);
	
	// int64, yes!
	aio.WriteInt32Attr("pkginstallsize",item->installedSize);
	aio.WriteInt32Attr("pkgnewreleasedate",item->newReleaseDate);
	aio.WriteInt32Attr("pkgdownloadeddate",item->downloadedDate);
	aio.WriteInt32Attr("pkgupdtdlstate",item->downloadState);

	// int64 ??
	aio.WriteInt32Attr("pkgnewsize",item->newSize);
	
	// type

	// checksumming
	
	long nameLen =	strlen(item->name);
	long versLen =	strlen(item->version);
	long devLen = strlen(item->developer);

	char *identBuf = (char *)malloc(nameLen + versLen + devLen + 1);
	char *ckBuf = identBuf;
		
	memcpy(identBuf, item->name, nameLen);
	identBuf += nameLen;
	memcpy(identBuf, item->version, versLen);
	identBuf += versLen;
	memcpy(identBuf, item->developer, devLen);
	identBuf += devLen;
	*identBuf = 0;

	PRINT(("identifier is %s\n",ckBuf));
	uchar cksum[16];
	MD5String(ckBuf,cksum);
	
	pkgFile.WriteAt(0,cksum,sizeof(cksum));



	pkgFile.Unlock();
	
#endif
	
	// set type
	// close the file
	pkgFile.Unset();
	
	BNode node(&item->fileRef);
	BNodeInfo	ninf(&node);
	ninf.SetType("application/x-scode-IPkg");
	
	return err;
}

// add a new package, returns error
// what if an old one exists
long PackageDB::AddPackage(PackageItem *item, dev_t devID)
{
	PRINT(("PackageDB::AddPackage\n"));
	long err = B_NO_ERROR;
	
#if DEBUG
	//char ename[B_FILE_NAME_LENGTH];
	//fDirEntry.GetName(ename);
	//PRINT(("set fDirEntry %s\n",ename));
#endif
	if (devID == -1) {
		// default to boot volume
		BVolume bootVol;
		BVolumeRoster rost;
		rost.GetBootVolume(&bootVol);
		devID = bootVol.Device();
	}
	
	BEntry	dirEnt;
	err = CreateDB(devID,&dirEnt);
	if (err < B_NO_ERROR) {
		PRINT(("couldn't get/create dbdir\n"));
		return err;
	}
	
	BDirectory	dbDir(&dirEnt);
	if (dbDir.InitCheck() < B_NO_ERROR) {
		PRINT(("dbentry is invalid\n"));
		return B_ERROR;
	}
	const char *namestr = item->data.FindString("package");
	const char *versstr = item->data.FindString("version");
	
	long nameLen =	strlen(namestr);
	long versLen =	strlen(versstr);
	AutoPtr<char>	fnBuf(nameLen + versLen + 3 + 1);
	char *fileName = fnBuf.Value();

	memcpy(fileName,namestr,nameLen);
	memcpy(fileName + nameLen,versstr,versLen);
	memcpy(fileName + nameLen + versLen,".db",4);

	BFile	pkgFile;	
	BEntry	pkgEntry;
	
	if (!dbDir.Contains(fileName)) {
		err = dbDir.CreateFile(fileName,&pkgFile);
		
		if (err < B_NO_ERROR) {
			PRINT(("couldn't create db file %s\n",fileName));
			return err;
		}
	}
	err = dbDir.FindEntry(fileName,&pkgEntry);

	err = pkgEntry.GetRef(&item->fileRef);
	if (err < B_NO_ERROR) {
		PRINT(("couldn't open package file\n"));
		return err;
	}
	err = WritePackage(item,ALL_DATA);

	return err;
}

// removes a package, on specified volume or on any volume
// returns error
long PackageDB::RemovePackage(PackageItem *item, dev_t volID)
{
	item; volID;
	PRINT(("PackageDB::RemovePackage NOT IMPLEMENTED\n"));
	return B_ERROR;
}

// get currently installed packages which use this package
long PackageDB::DependantPackages(PackageItem *item,
								BList	*dependants,
								dev_t volID)
{
	item;
	dependants;
	volID;
	PRINT(("PackageDB::DependantPackages NOT IMPLEMENTED\n"));
	return B_ERROR;
}

