#include "UpgradeList.h"

#define DEBUG 0
#include <Debug.h>
#include <string.h>
#include <stdlib.h>

UpgradeItem::UpgradeItem(BMessage *newdata)
{
	data = *newdata;
	
//	const char *str;
//	str = data.FindString("language");
//	ASSERT(str);
//	languages.AddItem(strdup(str));
	
//	str = data.FindString("platform");
//	ASSERT(str);
//	platforms.AddItem(strdup(str));
}

void UpgradeItem::PrintToStream()
{
	data.PrintToStream();
}

UpgradeItem::~UpgradeItem()
{
}


////////////////////////

UpgradeItemList::UpgradeItemList()
	:	RList<UpgradeItem *>()
{
}

UpgradeItemList::UpgradeItemList(const BMessage *archive)
	:	RList<UpgradeItem *>()
{
	SetTo(archive);
}

status_t UpgradeItemList::SetTo(const BMessage *archive)
{
	if (!archive)
		return B_ERROR;
		
	MakeEmpty();
	
	type_code 	type;
	int32		count;
	archive->GetInfo("upgradeitem",&type,&count);
	for (int i = 0; i < count; i++) {
		BMessage	msg;
		status_t err = archive->FindMessage("upgradeitem",i,&msg);
		if (err >= B_OK) AddItem(new UpgradeItem(&msg));
	}
	return B_OK;
}

UpgradeItemList::~UpgradeItemList()
{
	for (int i = CountItems()-1; i >= 0 ; i--)
		delete ItemAt(i);
}

status_t
UpgradeItemList::Archive(BMessage *archive, bool deep)
{
	deep;

	int32 count = CountItems();
	//archive->AddString("class","UpgradeItemList");
	for (int i = 0; i < count; i++) {
		archive->AddMessage("upgradeitem",&ItemAt(i)->data);
	}
	
	return B_NO_ERROR;
}

void UpgradeItemList::MakeEmpty()
{
	for (int i = CountItems()-1; i >= 0 ; i--)
		delete ItemAt(i);
	RList<UpgradeItem *>::MakeEmpty();
}

#define COMPILER_BUG 1
bool UpgradeItemList::AddUpgrade(UpgradeItem *add, int32 atindex)
{
	int i = 0;
	int64 newVersID;
	
#if COMPILER_BUG
	add->data.FindInt64("versionid",&newVersID);
#else
	newVersID = add->VersionID();
#endif
	for (i = CountItems()-1; i >= 0; i--) {
		UpgradeItem *it = (UpgradeItem *)ItemAt(i);

		int64	oVersID;
#if COMPILER_BUG
		it->data.FindInt64("versionid",&oVersID);
#else
		oVersID = it->VersionID();
#endif		
		if (oVersID == newVersID)
		{
			// need to merge in language and platform information
			// also we take the lowest price, favor free
			
			int i;

			const char *newlang = add->data.FindString("language");
			ASSERT(newlang);			
			PRINT(("checking language %s\n",newlang));

			type_code	type;
			int32		count;			
			it->data.GetInfo("language",&type,&count);
			for (i = count-1; i >= 0; i--) {
				if (strcasecmp(it->data.FindString("language",i),newlang) == 0)
					break;
			}
			if (i < 0) {
				// not found
				it->data.AddString("language",newlang);
				PRINT(("adding language %s\n",newlang));
			}

			// now platforms
			
			const char *newplat = add->data.FindString("platform");
			ASSERT(newplat);
			PRINT(("checking platform %s\n",newplat));

			it->data.GetInfo("platform",&type,&count);
			for (i = count-1; i >= 0; i--) {
				if (strcasecmp(it->data.FindString("platform",i),newplat) == 0)
					break;
			}
			if (i < 0) {
				// not found
				it->data.AddString("platform",newplat);
				PRINT(("adding language %s\n",newplat));
			}
			
			// favor lowest price
			//if (add->HasString("upgradeprice")) {
			//	const char *str = add->FindString("upgradeprice");
			//	add->AddFloat("fprice",atof(str));
			//}
			
			if (it->data.FindFloat("fprice") > add->data.FindFloat("fprice")) {
				it->data.RemoveName("fprice");
				it->data.AddFloat("fprice",add->data.FindFloat("fprice"));
			}
			break;
		}
	}

	if (i < 0) {
		if (atindex >= 0)
			return AddItem(add,atindex);
		else
			return AddItem(add);
	}
	else {
		// we merged in the data so
		delete add;
	}
	return false;
}

bool UpgradeItemList::AddUpgrade(BMessage *add)
{
	PRINT(("UpgradeItemList::AddUpgrade\n"));
	// convert versionid
	int64 vid;
	const char *str = add->FindString("versionid");
	if (!str) {
		// required info
		return false;
	}
	sscanf(str,"%Ld",&vid);
	add->RemoveName("versionid");
	add->AddInt64("versionid",vid);
	
	// convert newprice
	float price;
	str = add->FindString("newprice");
	if (str) {
		price = atof(str);
		add->RemoveName("newprice");
		add->AddFloat("newprice",price);
	}
	// convert upgradeprice
	str = add->FindString("upgradeprice");
	if (!str) {
		// required info
		return false;
	}
	price = atof(str);
	add->RemoveName("upgradeprice");
	add->AddFloat("fprice",price);

	// convert mediatype
	int32 media;
	str = add->FindString("mediatype");
	if (str) {
		media = atol(str);
		add->RemoveName("mediatype");
		add->AddInt32("mediatype",media);
	}
	// convert competitive
	str = add->FindString("competitive");
	if (str) {
		media = atol(str);
		add->RemoveName("competitive");
		add->AddBool("competitive",media);
	}
	
	// strip non-needed data
	add->RemoveName("status");
	add->RemoveName("message");
	add->RemoveName("sid");
	add->RemoveName("pid");
	add->RemoveName("upservice");
	add->RemoveName("regservice");

	return AddUpgrade(new UpgradeItem(add));
}
