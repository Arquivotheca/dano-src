// ===========================================================================
//	AdFilter.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifdef ADFILTER

#include "AdFilter.h"
#include "NPApp.h"
#include "Builder.h"
#include "HTMLTags.h"
#include "FolderWatcher.h"
#include "Strings.h"

#include <Path.h>
#include <FindDirectory.h>
//#include <File.h>
//#include <Node.h>
//#include <fs_attr.h>
//#include <NodeMonitor.h>
//#include <NodeInfo.h>
#include <malloc.h>
#include <stdio.h>

extern const char *kAdFilterFolderName;
extern const char *kTrackerSig;
extern const char *kDefaultFilterFileName;
extern const char *kAdFilterSiteAttr;
extern const char *kAdFilterFilterAttr;
extern const char *kAdFilterMimeType;
extern const char *kApplicationSig;
extern const char **GetDefaultAdList();

FolderWatcher* AdFilter::sFilterFolder;

CLinkedList AdFilter::sMasterFilterList;


const BPath *GetFilterFolder() {
	static BPath *settingsPath = NULL;
	if (!settingsPath) {
		settingsPath = new BPath;
		find_directory(B_USER_SETTINGS_DIRECTORY, settingsPath);
		settingsPath->Append(kAdFilterFolderName);
	}
	return settingsPath;
}


AdFilter::AdFilter()
{
}

AdFilter::AdFilter(const char *site, const char *filter, const char *tag, const char *attr, const char *action) :
	mSite(site), mFilter(filter), mTag(tag), mAttr(attr), mAction(action)
{
	mTag.ToUpper();
	mAttr.ToUpper();
	mAction.ToUpper();
}

AdFilter::~AdFilter()
{
}


void AdFilter::ShowAdFilters()
{
	BEntry entry(GetFilterFolder()->Path());

	entry_ref ref;
	entry.GetRef(&ref);

	BMessage msg(B_REFS_RECEIVED);
	msg.AddRef("refs", &ref);	

	BMessenger messenger(kTrackerSig);
	messenger.SendMessage(&msg);
}

void AdFilter::Init()
{
	BEntry entry(GetFilterFolder()->Path());
	sFilterFolder = new FolderWatcher(entry, kAdFilterMimeType, kApplicationSig, be_app, false, true);

	bool createDir;
	sFilterFolder->Init(&createDir);
	if (createDir)
		PopulateEmptyFilterList();
}

void AdFilter::FolderWatcherMsg(BMessage *msg)
{
	FolderWatcher *folderWatcher;
	if (msg->FindPointer("FolderWatcher", (void**)&folderWatcher) == B_OK && folderWatcher == sFilterFolder) {
		const char *filename = NULL;
		msg->FindString("Filename", &filename);
		if (!filename)
			return;
		switch(msg->what) {
			case FolderWatcher::kFileAdded: {
				AdFilter *af = new AdFilter;
				BMessage attrs;
				if (msg->FindMessage("AttrData", &attrs) == B_OK)
					af->ReadMessageAttributes(&attrs);
				af->mFilename = filename;
				sMasterFilterList.Add(af);
				break;
			}
			case FolderWatcher::kFileRemoved: {
				AdFilter *entry = FindEntry(filename);
				if (entry) {
					sMasterFilterList.Delete(entry);
					delete entry;
				}
				break;
			}
			case FolderWatcher::kFileChanged: {
				AdFilter *entry = FindEntry(filename);
				BMessage attrs;
				if (entry && msg->FindMessage("AttrData", &attrs) == B_OK)
					entry->ReadMessageAttributes(&attrs);
				break;
			}
		}
	}
}

void AdFilter::PopulateEmptyFilterList()
{
/*
	const char **filterList = GetDefaultAdList();
	for (int i = 0; filterList[i]; i++) {
//		printf("Adding filter %s\n", filterList[i]);
		AddNewFilter("*", filterList[i]);
	}
*/
}

extern int SearchToken(const char** tokens, int count, const BString key);
extern const char* gTags[];


AdFilterList *AdFilter::BuildFilterList(const char *hostname)
{		
	AdFilterList *filterList = new AdFilterList;

	for (int i = 0; i < kTagsCount; i++)
		filterList->mTagList[i] = false;

	AdFilter *flt = (AdFilter *)sMasterFilterList.First();
	while (flt) {
		if (MatchHostName(hostname, flt->mSite.String())) {
//printf("*****Checking %s against filter %s.  Matched.\n", hostname, flt->mSite);
			filterList->mFilterList.Add(new AdFilter(flt->mSite.String(), flt->mFilter.String(), flt->mTag.String(),
										 flt->mAttr.String(), flt->mAction.String()));
			// Matches all tags.
			if (flt->mTag.Length() == 0 || flt->mTag == "*") {
				for (int i = 0; i < kTagsCount; i++)
					filterList->mTagList[i] = true;
			} else
				filterList->mTagList[SearchToken(gTags, kTagsCount, flt->mTag.String())] = true;
		} else {
//printf("*****Checking %s against filter %s.  No match.\n", hostname, flt->mSite);
		}
		flt = (AdFilter *)flt->Next();
	}
	
	
	return filterList;
}

bool AdFilter::MatchHostName(const char *hostname, const char *filterSite)
{
	if (strcmp(filterSite, "*") == 0)
		return true;
		
	int hostnameLength = strlen(hostname);
	int filterLength = strlen(filterSite);
	const char *hostnamePos = hostname;
	const char *filterPos = filterSite;
	
	do {
		if (*filterPos == '*') {
//			printf("Found a wildcard.\n");
			// Found a wildcard.  Starting at the character after the
			// wildcard, and up to the character before the next wildcard,
			// look for a match in the hostname.
			filterPos++;
			char *nextWildPos = strstr(filterPos, "*");
			int substrlen = 0;
			if (nextWildPos)
				substrlen = nextWildPos - filterPos;
			else
				substrlen = strlen(filterPos);
			if (substrlen > 0) {
				char *tmp = (char *)malloc(substrlen + 1);
				strncpy(tmp, filterPos, substrlen);
				tmp[substrlen] = 0;
//				printf("Trying to match %s.\n", tmp);
				hostnamePos = strstr(hostnamePos, tmp);
				if (!hostnamePos)
					return false;
				else {
					hostnamePos += substrlen;
					filterPos += substrlen;
//			printf("Matched.  Rest of hostname is %s.\n", hostnamePos);
				}
				free(tmp);
			}
		} else {
//			printf("Didn't find a wildcard.\n");
			const char *nextWildPos = strstr(filterPos, "*");
			if (nextWildPos != 0) {
//				printf("Trying to match %s to %s, length %d.\n", filterPos, hostnamePos, nextWildPos - filterPos);
				if (strncmp(hostnamePos, filterPos, nextWildPos - filterPos) == 0)
					hostnamePos += nextWildPos - filterPos;
				else
					return false;
				filterPos = nextWildPos;
			} else {
//				printf("No more wildcards.  Matching exactly %s to %s.\n", hostnamePos, filterPos);
				// No more wildcards to process.  The rest of the hostname
				// must match exactly.
				return (strcmp(hostnamePos, filterPos) == 0);
			}
		}
	} while (hostnamePos < hostname + hostnameLength && filterPos < filterSite + filterLength);
	return true;
}


void AdFilter::AddNewFilter(const char *site, const char *filter, const char *tag, const char *attr, const char *action)
{
	BMessage msg;
	msg.AddString(kAdFilterSiteAttr, site);
	msg.AddString(kAdFilterFilterAttr, filter);
	msg.AddString(kAdFilterAttrAttr, attr);
	msg.AddString(kAdFilterTagAttr, tag);
	msg.AddString(kAdFilterActionAttr, action);
	sFilterFolder->AddFile(NULL, msg);
}

extern const char *gAttributes[];

bool AdFilter::ShouldFilterTag(AdFilterList *filterList, short tagID, Tag **origTag)
{
	if (gPreferences.FindBool("FilterEnabled") && filterList->mTagList[tagID]) {
		Tag *tag = origTag ? (*origTag ? (Tag *)(*origTag)->First() : 0) : 0;
		do {
			AdFilter *flt = (AdFilter *)filterList->mFilterList.First();
			bool stripTag = false;
			while (flt) {
				if ((flt->mTag.Length() == 0 || flt->mTag == "*" || flt->mTag.ICompare(gTags[tagID]) == 0) &&
					(flt->mAttr.Length() == 0 || flt->mAttr == "*" || (tag && flt->mAttr.ICompare(gAttributes[tag->mAttributeID]) == 0)) &&
					(flt->mFilter.Length() == 0 || flt->mFilter == "*" || (tag && tag->mValue && !tag->mNumericValue && strstr((const char *)tag->mValue, flt->mFilter.String()) != 0)))
					if (flt->mAction.ICompare(kAdFilterActionDeleteTag) == 0)
						return true;
					else if (flt->mAction.ICompare(kAdFilterActionDeleteAttr) == 0) {
						stripTag = true;
						break;
					}
				flt = (AdFilter *)flt->Next();
			}
			if (stripTag && tag) {
				Tag *tmp = (Tag *)tag->Next();
				if (tag == *origTag)
				*origTag = (Tag *)(*origTag)->Next();
				delete tag;
				tag = tmp;
			} else
				tag = tag ? (Tag *)tag->Next() : 0;
		} while (tag);
	}
	return false;
}

AdFilter *AdFilter::FindEntry(const char *filename)
{
	AdFilter *entry = (AdFilter *)sMasterFilterList.First();
	while (entry) {
		if (entry->mFilename == filename)
			return entry;
		entry = (AdFilter *)entry->Next();
	}
	return NULL;
}

void AdFilter::ReadMessageAttributes(BMessage *msg)
{
	mSite = msg->FindString(kAdFilterSiteAttr);
	mFilter = msg->FindString(kAdFilterFilterAttr);
	mTag = msg->FindString(kAdFilterTagAttr);
	mAttr = msg->FindString(kAdFilterAttrAttr);
	mAction = msg->FindString(kAdFilterActionAttr);
}

#endif