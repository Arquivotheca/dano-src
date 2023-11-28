// ---------------------------------------------------------------------------
/*
	DocumentationLookup.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			15 January 1999

	Utilities for looking up keywords in documentation
*/
// ---------------------------------------------------------------------------

#ifndef _DOCUMENTATIONLOOKUP_H
#define _DOCUMENTATIONLOOKUP_H

#include <Query.h>
#include <List.h>

// ---------------------------------------------------------------------------
// class DocumentationLookupEntry
// results of a query into the documentation
// This is a pretty big structure.  The idea is that you get the information
// you need out of here and throw it away.
// ---------------------------------------------------------------------------

class DocumentationLookupEntry
{
public:
	enum		{ kStringLength = 256};

	char		fTitle[kStringLength];
	char		fClass[kStringLength];
	char		fDescription[kStringLength];
	char		fURL[kStringLength];
};

// ---------------------------------------------------------------------------
// class DocumentationLookup
// Handles lookups for the title attribute
// ---------------------------------------------------------------------------

class DocumentationLookup
{
public:
					DocumentationLookup(const char* lookupKey);
					~DocumentationLookup();

	bool 			GetNextExactMatch(DocumentationLookupEntry& outEntry);
	bool 			GetNextGeneralMatch(DocumentationLookupEntry& outEntry);
	
	// Utility functions
	static void 	Open(const char* url);
	
private:
	bool			GetNextMatch(DocumentationLookupEntry& outEntry, BQuery& theQuery);

	void			SaveMatch(const entry_ref& ref);
	bool			IsDuplicateMatch(const entry_ref& ref);
	
	BQuery			fExactQuery;
	BQuery			fGeneralQuery;
	
	BList			fMatchList;
};

#endif
