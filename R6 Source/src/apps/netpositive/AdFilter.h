// ===========================================================================
//	AdFilter.h
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifndef __ADFILTER__
#define __ADFILTER__

#ifdef ADFILTER

#include "Utils.h"
#include "HTMLTags.h"
#include <Directory.h>

class Tag;
class FolderWatcher;

class AdFilterList;

class AdFilter : public CLinkable {
public:
				AdFilter(const char *site, const char *filter, const char *tag, const char *attr, const char *action);
				AdFilter();
				~AdFilter();
				
		void	ReadMessageAttributes(BMessage *msg);
				
static void		ShowAdFilters();
static void		Init();
static void		AddNewFilter(const char *site, const char *filter, const char *tag, const char *attr, const char *action);
static AdFilterList *BuildFilterList(const char *hostname);
static bool		ShouldFilterTag(AdFilterList *filterList, short tagID, Tag **tag);
static AdFilter* FindEntry(const char *filename);
static void		FolderWatcherMsg(BMessage *msg);

protected:
static void		PopulateEmptyFilterList();
static bool		MatchHostName(const char *hostname, const char *filterSite);

		BString	mSite;
		BString mFilter;
		BString mFilename;
		BString	mTag;
		BString	mAttr;
		BString	mAction;

static CLinkedList sMasterFilterList;
static FolderWatcher* sFilterFolder;
};

class AdFilterList {
public:
		CLinkedList	mFilterList;
		bool		mTagList[kTagsCount];
};

#endif
#endif