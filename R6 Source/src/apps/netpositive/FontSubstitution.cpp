// ===========================================================================
//	FontSubstitution.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#include "FontSubstitution.h"
#include "FolderWatcher.h"
#include "Strings.h"
#include "NPApp.h"

#include <Application.h>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Autolock.h>

static FontSubFolder *sFontFolder = NULL;


int32 FontSubFolder::InitThreadEntry(void *data)
{
	BAutolock autolock(sFontFolder->mItemListLocker);
	FontSubFolder *fontFolder = (FontSubFolder *)data;
	bool createDir;
	fontFolder->Init(&createDir);
	if (createDir) {
		const FontSubstitutionItem *fontList = GetFontSubstitutionList();
		for (int i = 0; ; i++) {
			const FontSubstitutionItem *fontItem = &fontList[i];
			if (!fontItem->mFont)
				return 0;
			BMessage msg;
			msg.AddString(kFontFontAttr, fontItem->mFont);
			msg.AddString(kFontMapsToAttr, fontItem->mMapsTo);
			fontFolder->AddFile(NULL, msg);
		}
	}
	NetPositive::RemoveThread(find_thread(NULL));
	return 0;
}


FolderWatcher* FontSubFolder::CreateFolderWatcher()
{
	BPath settingsPath;
	find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath);
	settingsPath.Append(kFontFolderName);
	BEntry entry(settingsPath.Path());
	sFontFolder = new FontSubFolder(entry, kFontMimeType, kApplicationSig, be_app, false, true);

//	sFontFolder->mItemListLocker.Lock();
	thread_id tid = spawn_thread(InitThreadEntry, "FontSubstitution init", B_NORMAL_PRIORITY - 1, sFontFolder);
	resume_thread(tid);
	NetPositive::AddThread(tid);

	return sFontFolder;
}


FontSubFolder::FontSubFolder(const BEntry& path, const char *fileType, const char *appSig, BLooper *looper, bool recursive, bool createDir)
	: FolderWatcher(path, fileType, appSig, looper, recursive, createDir, true)
{
}

FolderItem* FontSubFolder::CreateFolderItem() const
{
	return new FontSubItem;
}


const char *FontSubFolder::MapFont(const char *origFont, int& matchType)
{
	if (!origFont)
		return NULL;
		
	BAutolock autolock(sFontFolder->mItemListLocker);
	for (int i = 0; i < count_font_families(); i++) {
		font_family familyName;
		get_font_family(i, &familyName);
		if (strcasecmp(origFont, familyName) == 0) {
			matchType = kMatched;
			return origFont;
		}
	}
	
	for (int i = 0; i < sFontFolder->mItemList.CountItems(); i++) {
		FontSubItem *item = (FontSubItem *)sFontFolder->mItemList.ItemAt(i);
		if (item->mFont.Length() && strcasecmp(item->mFont.String(), origFont) == 0) {
			matchType = kMapped;
			return item->mMapsTo.String();
		}
	}
	matchType = kNoMatch;
	return origFont;
}


void FontSubItem::ReadMessageAttributes(BMessage *msg)
{
	mFont = msg->FindString(kFontFontAttr);
	mMapsTo = msg->FindString(kFontMapsToAttr);
}
