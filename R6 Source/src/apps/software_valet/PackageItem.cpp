#include "PackageItem.h"
#include "SerialNum.h"
#include "UpgradeList.h"
#include <ctype.h>

// will be read in
PackageItem::PackageItem()
	:	fFilled(false),
		fRemapped(false)
{
	data.AddString("package",B_EMPTY_STRING);
	data.AddString("version",B_EMPTY_STRING);
	data.AddString("developer",B_EMPTY_STRING);
	data.AddString("description",B_EMPTY_STRING);
	data.AddString("platforms",B_EMPTY_STRING);
	data.AddString("languages",B_EMPTY_STRING);
	data.AddInt32("mediatype",0);
	data.AddInt32("softtype", SOFT_TYPE_COMMERCIAL);
	
	data.AddInt32("releasedate",0);
	data.AddInt32("purchasedate",0);
	data.AddInt32("installdate",0);
	data.AddInt64("installsize",0);
	data.AddInt32("registered",REGISTERED_NO);
	
	data.AddString("sid",B_EMPTY_STRING);	// serialID
	data.AddString("pid",B_EMPTY_STRING);	// prefixID
	data.AddString("vid",B_EMPTY_STRING);	// versionID
	data.AddString("depotsn",B_EMPTY_STRING); // BeDepotSN
	
	data.AddBool("regservice",true);		// registration service
	data.AddBool("upservice",true);		// update service
	data.AddBool("anonsid",true);		// support anon serials??
	
	// depot serial number -- not checked, not valid
	supported = 0;
}

static bool IsValidSerial(const char *can);
static bool IsValidSerial(const char *can)
{
	bool valid = false;
	
	if (can && *can) {
		const int kSnLen = 16;
		char serial[kSnLen+1];
		
		// copy only valid characters (digits)
		// break if invalid character found
		int len = strlen(can);
		int j = 0;
		for (int i = 0; j < kSnLen && i < len; i++) {
			if (isdigit(can[i]))
				serial[j++] = can[i];
			else if (can[i] != '-') {
				break;
			}
		}
		if (j == kSnLen) {
			serial[kSnLen] = 0;		// null terminate
			valid = SNcheck(serial);
		}
	}
	return valid;
}

bool PackageItem::ValetSupported()
{
	// cached
	if (supported & 0x1) return supported & 0x2;
	
	// validate bedepot serial number
	bool valid = true;
	const char *can = data.FindString("depotsn");
	supported = 0x1;
	if (IsValidSerial(can))
		supported = supported | 0x2;
		
	return supported & 0x2;
}

UpgradeItemList	*PackageItem::Updates()
{
	if (!fFilled) {
		fUpdates.SetTo(&updates);
		fFilled = true;
	}
	return &fUpdates;
}
