#include "SettingsManager.h"
#include "AttrIO.h"
#include "SResIO.h"
#include <fs_attr.h>
#include <time.h>
#include "Util.h"
#include "MyDebug.h"
#include <Path.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <stdlib.h>
#include <string.h>

long CreatePathAt(const BDirectory *dir,const char *path,BEntry *final);


SettingsManager::SettingsManager(bool readOnly)
	:	fReadOnly(readOnly)
{
	long err;
	
	SetupDefaults();
	
	// return;

	bool newSettings = false;
	
	BPath tPath;
	find_directory(B_COMMON_SETTINGS_DIRECTORY,&tPath,true);
	
	const char foldNam[] = "SoftwareValet";
	const char fileNam[] = "SoftwareValet_settings";
	BDirectory sDir(tPath.Path());

	BEntry newDir;
	
	err = CreatePathAt(&sDir,foldNam,&newDir);
	if (err == B_NO_ERROR) err = sDir.SetTo(&newDir);
	if (err < B_NO_ERROR) {
#if (!TRANSCEIVER_BUILD)
		doError("Could not find \"%s\" folder.",foldNam);
#endif
		return;
	}
	
	if (!sDir.Contains(fileNam)) {
		PRINT(("creating pref file\n"));
		BFile	f;
		err = sDir.CreateFile(fileNam,&f);
		newSettings = true;
		if (err < B_NO_ERROR) {
#if (!TRANSCEIVER_BUILD)
			doError("Error creating settings file. Using defaults.");
#endif
			return;
		}
	}
	
	err = sDir.FindEntry(fileNam,&sEntry);
	if (err < B_NO_ERROR || !sEntry.IsFile()) {
#if (!TRANSCEIVER_BUILD)
		doError("Error finding settings file.");
#endif
		return;
	}
	if (!newSettings)
		ReadSettings();
	else {
		bool readOnly = fReadOnly;
		fReadOnly = false;
		SaveSettings();
		fReadOnly = readOnly;
	}
}

SettingsManager::~SettingsManager()
{
	SaveSettings();
}

// types that are of fixed size
#define is_fixed_size(x) (x == B_BOOL_TYPE || x == B_INT8_TYPE || \
x == B_INT16_TYPE || x == B_INT32_TYPE || x == B_INT64_TYPE || \
x == B_FLOAT_TYPE || x == B_DOUBLE_TYPE || x == B_POINT_TYPE || \
x == B_RECT_TYPE || x == B_POINTER_TYPE )
// not sure about messenger


status_t MergeMessage(BMessage &dst, BMessage &src)
{
	// merge all items from src into dst
	// replacing any that already exist
	const char 	*name;
	type_code	type;
	int32		count;
	status_t	err = B_OK;
	
	// iterate through all names in the source
	for ( int32 i = 0; 
		src.GetInfo(B_ANY_TYPE, i, &name, &type, &count) >= B_OK ; i++ )
	{
		bool isFixed = is_fixed_size(type);
		// remove the existing name (if any)
		if (dst.RemoveData(name) != B_ERROR) {
			const void *data;
			ssize_t	numBytes;
			// add all the indexed fields under that name
			int fieldidx = 0;
			while(fieldidx < count) {
				if ((err = src.FindData(name, B_ANY_TYPE, fieldidx, &data, &numBytes)) < B_OK) {
					PRINT(("find data %s, index %d failed\n",name,i));
					return err;
				}
				if ((err = dst.AddData(name, type, data, numBytes,isFixed)) < B_OK) {
					PRINT(("add data failed\n"));
					return err;
				}
				fieldidx++;
			}
		}
	}
	return err;
}



void SettingsManager::SetupDefaults()
{
	BPath	tPath;

// download
	find_directory(B_COMMON_DIRECTORY,&tPath,false);
	tPath.Append("Downloads");
	
	data.AddString("download/path",tPath.Path());
	data.AddInt32("download/flags",DL_AUTORESUME);

// communication
	srandom((ulong)time(NULL));
	
	data.AddInt32("comm/checkfreq",EVERY_WEEK);
	data.AddInt16("comm/checkday",random() % 7);
	data.AddInt16("comm/checkhr",random() % 8);
	data.AddBool("comm/listenerauto",false);
#if DEBUG
	data.AddString("comm/servername","208.223.202.131");
#else
	data.AddString("comm/servername","valet.bedepot.com");
#endif

	data.AddString("comm/proxy","");
	data.AddInt32("comm/proxyport",8080);

// install
	find_directory(B_COMMON_DIRECTORY,&tPath,false);
	data.AddBool("install/log",true);
	data.AddString("install/logpath",tPath.Path());
	data.AddBool("install/preview",true);
	data.AddBool("install/usepath",false);
	
	find_directory(B_APPS_DIRECTORY,&tPath,false);
	data.AddString("install/path",tPath.Path());

// register
	data.AddInt32("register/mode",DO_REGISTER);

	BMessage	regMsg('RegD');
	data.AddMessage("register/data",&regMsg);

// update
	data.AddInt32("update/mode",UPDT_AUTODOWNLOAD);

// general
	BPoint pos(-1,-1);
	data.AddPoint("general/setwindow",pos);
	data.AddPoint("general/manwindow",pos);
	data.AddInt16("general/prefpanel",0);	
	
// logging
	data.AddInt32("logging/flags",0xFFFFFFFF);
	
// notification
	data.AddInt32("notification/freq",EMAIL_OFF);

// listener
//	lastScheduledConnection = 0;
}

void SettingsManager::SaveSettings()
{
	if (fReadOnly)
		return;
		
	// byte ordering!!!
	long err;
	
	PRINT(("SettingsManager::SaveSettings\n"));
	BFile	sFile(&sEntry,O_RDWR);
	if (sFile.InitCheck() < B_NO_ERROR) {
#if (!TRANSCEIVER_BUILD)	
		doError("Could not save settings.");
#endif
		return;
	}
	SResIO	resIO;
	resIO.SetTo(&sFile, 'Sett', 0);
	ssize_t	size;
	
	if ((err = data.Flatten(&resIO, &size)) < B_OK ||
		(err = resIO.SetTo(&sFile, 'Sett', 1)) < B_OK ||
		(err = reg.Flatten(&resIO, &size)) < B_OK) {
#if (!TRANSCEIVER_BUILD) 
		doError("Error saving settings: %s.",strerror(err));
#endif
	}
}

void SettingsManager::ReadSettings()
{
	status_t err;
		
	PRINT(("SettingsManager::ReadSettings\n"));
	BFile	sFile(&sEntry,O_RDWR);
	err = sFile.InitCheck();
	if (err < B_NO_ERROR) {
#if (!TRANSCEIVER_BUILD)
		doError("Could not read settings file.");
#endif
		return;
	}
	SResIO	resIO;
	
	resIO.SetTo(&sFile, 'Sett',0);
	BMessage temp;
	err = temp.Unflatten(&resIO);
	if (err >= B_OK) {
		if ((err = MergeMessage(data,temp)) < B_OK) {
#if (!TRANSCEIVER_BUILD)
			doError("Could not read saved settings from disk: %s",strerror(err));
#endif
		}
		resIO.SetTo(&sFile, 'Sett',1);
		if ((err = temp.Unflatten(&resIO) < B_OK) ||
			(err = MergeMessage(reg,temp)) < B_OK)
		{
#if (!TRANSCEIVER_BUILD)
			doError("Could not read registration settings from disk: %s",strerror(err));
#endif
		}
	}
#if __INTEL__
	else {
	#if (!TRANSCEIVER_BUILD)
		doError("Could not read saved settings from disk: %s",strerror(err));
	#endif
	}
#else
	else {
		// read in settings from pre-1.5 version
		off_t sz;

		sFile.Lock();
		AttrIO	aio(&sFile);
		
		char	buf[512];
		int32	flags;
		int16	num;
		bool	sw;
		
		// download prefs
		if (aio.ReadStringAttr("valet/download_path",buf,512) >= B_OK)
			ReplaceString(&data,"download/path",buf);
		if (aio.ReadInt32Attr("valet/download_flags",&flags) >= B_OK)
			ReplaceInt32(&data,"download/flags",flags);
			
		// comm prefs
		if (aio.ReadInt16Attr("valet/check_freq",&num) >= B_OK)
			ReplaceInt32(&data,"comm/checkfreq",num);
		if (aio.ReadInt16Attr("valet/check_day",&num) >= B_OK)
			ReplaceInt16(&data,"comm/checkday",num);
		if (aio.ReadInt16Attr("valet/check_hour",&num) >= B_OK)
			ReplaceInt16(&data,"comm/checkhr",num);
		if (aio.ReadBoolAttr("valet/listener_auto",&sw) >= B_OK)
			ReplaceBool(&data,"comm/listenerauto",sw);

			
		// install prefs
		int32	installFlags = 0;
		aio.ReadInt32Attr("valet/inst_mode",&installFlags);
		sw = installFlags & INST_LOG;
		data.AddBool("install/log",sw);
		sw = installFlags & INST_PATH;
		data.AddBool("install/usepath",sw);
		sw = installFlags & INST_PREVIEW;
		ReplaceBool(&data,"install/preview",sw);
	
		
		if (aio.ReadStringAttr("valet/log_path",buf,512) >= B_OK)
			ReplaceString(&data,"install/logpath",buf);
		if (aio.ReadStringAttr("valet/install_path",buf,512) >= B_OK)
			ReplaceString(&data,"install/path",buf);
			
		// register prefs
		attr_info	aiBuf;
		err = sFile.GetAttrInfo("valet/reg_data",&aiBuf);
		if (err == B_NO_ERROR) {
			sz = aiBuf.size;
			
			char *mbuf = new char[sz];
			err = sFile.ReadAttr("valet/reg_data",B_MESSAGE_TYPE,0,mbuf,sz);
			
			BMessage newRegMsg;
			err = newRegMsg.Unflatten(mbuf);
			if (err >= B_NO_ERROR) {
				// successful
				reg = newRegMsg;
			}
			delete[] mbuf;
		}
		
		if (aio.ReadInt16Attr("valet/reg_mode",&num) >= B_OK)
			ReplaceInt32(&data,"register/mode",num);
			
		// update prefs
		if (aio.ReadInt32Attr("valet/updt_mode",&flags) >= B_OK)
			ReplaceInt32(&data,"update/mode",flags);
			
		// uinstall prefs
		// aio.ReadInt16Attr("valet/uninst_mode",&uinstMode);
		// aio.ReadInt32Attr("valet/uninst_flags",&uinstFlags);
		
		// logging flags
		if (aio.ReadInt32Attr("valet/logging_flags",&flags) >= B_OK)
			ReplaceInt32(&data,"logging/flags",flags);
		if (aio.ReadInt16Attr("valet/email_notification",&num) >= B_OK)
			ReplaceInt32(&data,"notification/freq",num);
	
		// application prefs
		if (aio.ReadInt16Attr("valet/curpref_panel",&num) >= B_OK)
			ReplaceInt16(&data,"general/prefpanel",num);
			
		/******
		BPoint p;
		long xy[2];
		sFile.ReadAttr("valet/setwind_pos",B_POINT_TYPE,0,(char *)xy,sizeof(xy));
		p.x = xy[0]; p.y = xy[1];
		data.AddPoint("general/setwindow",p);

		sFile.ReadAttr("valet/mainwind_pos",B_POINT_TYPE,0,(char *)xy,sizeof(xy));
		p.x = xy[0]; p.y = xy[1];
		data.AddPoint("general/manwindow",p);
		*****/
		
		// general
		// aio.ReadInt32Attr("valet/lastconnect",&lastScheduledConnection);

		sFile.Unlock();
	}
#endif
}

