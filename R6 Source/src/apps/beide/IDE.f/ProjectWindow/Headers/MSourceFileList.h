// ---------------------------------------------------------------------------
/*
	MSourceFileList.h
	
	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Be Revisions:	John R. Dance
					1 January 1999

	MSourceFileList
		A templated list class that holds MSourceFile* in sort order.
		
	MSourceFileSet
		A templated list class that holds MSourceFile* in sort order.
		(then why is it called a Set?  Because it allows "multiple"
		MSourceFile's with the same leaf name.)

*/
// ---------------------------------------------------------------------------

#ifndef _MSOURCEFILELIST_H
#define _MSOURCEFILELIST_H

#include "MList.h"

class MSourceFile;

// ---------------------------------------------------------------------------
// class MSourceFileList
// ---------------------------------------------------------------------------

class MSourceFileList : public MList<MSourceFile*> 
{
public:
				MSourceFileList(int32 itemsPerBlock = 50);

	virtual		~MSourceFileList() {}

	bool		AddItem(MSourceFile* inSourceFile);

	bool		AddItem(MSourceFile* inSourceFile, int32 inAtIndex);

	bool		FindItem(const char* inFilename,
						 bool inSystemTree,
						 int32& outIndex) const;
};

// ---------------------------------------------------------------------------
// class MSourceFileSet
// ---------------------------------------------------------------------------

class MSourceFileSet : public MList<MSourceFile*> 
{
public:

			MSourceFileSet(int32 itemsPerBlock = 50);

	bool	AddItem(MSourceFile* inSourceFile);

	bool	AddItem(MSourceFile* inSourceFile, int32 inAtIndex);

	bool	FindItem(MSourceFile* inFile, int32& outIndex) const;
};

#endif
