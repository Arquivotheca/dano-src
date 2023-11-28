// ---------------------------------------------------------------------------
/*
	DocumentationLookup.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			15 January 1999

	Utilities for looking up keywords in documentation
*/
// ---------------------------------------------------------------------------

#include "DocumentationLookup.h"
#include "MAlert.h"

#include <Entry.h>
#include <Node.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Query.h>
#include <Roster.h>
#include <String.h>
// currently FSUtils.h isn't public
#include <FSUtils.h>

#include <stdio.h>

// ---------------------------------------------------------------------------
// static strings
// ---------------------------------------------------------------------------

const char* kIndexFileType = "application/x-vnd.Be-doc_bookmark";
const char* kTypeAttr = "BEOS:TYPE";
const char* kNameAttrName = "name";
const char* kClassAttrName = "be:class_name";
const char* kKitAttrName = "be:kit_name";
const char* kPathAttrName = "META:url";
const char* kDescriptionAttrName = "be:short_description";

const char* kDocumentType = "text/html";

// ---------------------------------------------------------------------------
// DocumentationLookup member functions
// ---------------------------------------------------------------------------

DocumentationLookup::DocumentationLookup(const char* lookupKey)
{
	// Do two queries here...
	// An exact match and then a generalized query...
	// Exact:
	//		name == lookupKey || name == lookupKey()
	// General
	//		name contains lookupKey (case insensitive)
		
	BVolume vol;
	BVolumeRoster vr;
	vr.GetBootVolume(&vol);
	fExactQuery.SetVolume(&vol);
	fGeneralQuery.SetVolume(&vol);
	
	// Do exact query
	// try lookup key
	fExactQuery.PushAttr(kNameAttrName);
	fExactQuery.PushString(lookupKey);
	fExactQuery.PushOp(B_EQ);
	// or lookup key with parens
	fExactQuery.PushAttr(kNameAttrName);
	BString keyAsMethod(lookupKey);
	keyAsMethod += "()";
	fExactQuery.PushString(keyAsMethod.String());
	fExactQuery.PushOp(B_EQ);
	fExactQuery.PushOp(B_OR);
	// file type
	fExactQuery.PushAttr(kTypeAttr);
	fExactQuery.PushString(kIndexFileType);
	fExactQuery.PushOp(B_EQ);
	fExactQuery.PushOp(B_AND);
	fExactQuery.Fetch();

	// Do general query
	// lookup key
	fGeneralQuery.PushAttr(kNameAttrName);
	fGeneralQuery.PushString(lookupKey, true);				// case insensitive
	fGeneralQuery.PushOp(B_CONTAINS);
	// file type
	fGeneralQuery.PushAttr(kTypeAttr);
	fGeneralQuery.PushString(kIndexFileType);
	fGeneralQuery.PushOp(B_EQ);
	fGeneralQuery.PushOp(B_AND);
	fGeneralQuery.Fetch();
}

// ---------------------------------------------------------------------------

DocumentationLookup::~DocumentationLookup()
{
	// delete all the entries in our match list
	// notice that I don't remove them so it is ok to iterate forwards

	int32 count = fMatchList.CountItems();
	for (int32 i = 0; i < count; i++) {
		entry_ref* ref = (entry_ref*) fMatchList.ItemAtFast(i);
		delete ref;
	}
	fMatchList.MakeEmpty();		
}

// ---------------------------------------------------------------------------

bool
DocumentationLookup::GetNextExactMatch(DocumentationLookupEntry& outEntry)
{	
	return this->GetNextMatch(outEntry, fExactQuery);
}

// ---------------------------------------------------------------------------

bool
DocumentationLookup::GetNextGeneralMatch(DocumentationLookupEntry& outEntry)
{
	return this->GetNextMatch(outEntry, fGeneralQuery);
}

// ---------------------------------------------------------------------------

bool
DocumentationLookup::GetNextMatch(DocumentationLookupEntry& outEntry, BQuery& theQuery)
{	
	// clean out the DocumentationLookupEntry before we get started
	memset(&outEntry, 0, sizeof(DocumentationLookupEntry));

	// Get the next ref - if we get it, fill in our entry
	// otherwise return false
	// We have to watch out for getting entries in the trash, so
	// we loop until either we don't get an entry or we get
	// a good one
	// Get the next ref and if we don't have any, return false
	
	entry_ref ref;
	bool matchFound = false;
	while (true) {
		status_t err = theQuery.GetNextRef(&ref);
		if (err != B_OK) {
			// game's over, we didn't get a match
			matchFound = false;
			break;
		}
	
		// got a match, but make sure it isn't in the trash or
		// a duplicate before getting too excited...
		
		// (ignoreThisRef is true if it is in the trash of if it is a duplicate)
		bool ignoreThisRef = false;

		BEntry entry(&ref);
		BDirectory trashForVolume;
		if (FSGetTrashDir(&trashForVolume, ref.device) == B_OK && trashForVolume.Contains(&entry)) {
			ignoreThisRef = true;
		}
		else if (this->IsDuplicateMatch(ref)) {
			ignoreThisRef = true;
		}

		if (ignoreThisRef == false) {
			matchFound = true;
			break;
		}
	}
	
	// once we get a match, fill in the outEntry will all the attributes
	if (matchFound) {
		BNode matchNode(&ref);
		// the title is currently the name of the file itself
		strncpy(outEntry.fTitle, ref.name, DocumentationLookupEntry::kStringLength);
		matchNode.ReadAttr(kClassAttrName, B_STRING_TYPE, 0, 
							outEntry.fClass, DocumentationLookupEntry::kStringLength);
		matchNode.ReadAttr(kDescriptionAttrName, B_STRING_TYPE, 0, 
							outEntry.fDescription, DocumentationLookupEntry::kStringLength);
		matchNode.ReadAttr(kPathAttrName, B_STRING_TYPE, 0, 
							outEntry.fURL, DocumentationLookupEntry::kStringLength);

		// also save this ref for further duplicate checking
		this->SaveMatch(ref);
	}
	
	return matchFound;
}

// ---------------------------------------------------------------------------

void
DocumentationLookup::SaveMatch(const entry_ref& ref)
{
	fMatchList.AddItem(new entry_ref(ref));
}

// ---------------------------------------------------------------------------

bool 
DocumentationLookup::IsDuplicateMatch(const entry_ref& ref)
{
	// iterate our saved list and see we already have this ref
	
	bool isDuplicate = false;
	int32 count = fMatchList.CountItems();
	for (int32 i = 0; i < count; i++) {
		entry_ref* refFromList = (entry_ref*) fMatchList.ItemAtFast(i);
		if (refFromList && *refFromList == ref) {
			isDuplicate = true;
			break;
		}
	}
	
	return isDuplicate;
}

// ---------------------------------------------------------------------------
void
DocumentationLookup::Open(const char* url)
{
	// Remember that the url isn't necessarily a file name.  It is a file
	// with a #marker on the end, so we can't just launch it like a file.
	// Use a document type to launch the browser.  In that way, the user can
	// set up any browser they want for documentation.
	
	const char* argv[1];
	argv[0] = url;	
	status_t result = be_roster->Launch(kDocumentType, 1, (char**) argv);

	// Tell the user if there were any errors
	if (result != B_OK && result != B_ALREADY_RUNNING) {
		BString text = "Could not launch browser for ";
		text += kDocumentType;
		text += " with url of ";
		text += url;
		MStopAlert alert(text.String());
		alert.Go();	
	}
}
