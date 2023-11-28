//========================================================================
//	MAreaFileList.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MAREAFILELIST_H
#define _MAREAFILELIST_H

#include "MList.h"

struct AreaFileRec;

typedef int32 (*compare_strings)(const char *, const char *);


class MAreaFileList : public MList<AreaFileRec*> 
{
public:

							MAreaFileList(
								int32	itemsPerBlock = 50);

virtual						~MAreaFileList() {}

		bool				AddItem(
								AreaFileRec* inFileRec);

		bool				AddItem(
								AreaFileRec* inFileRec, 
								int32 inAtIndex);

		bool				FindItem(
								const char *	inFilename,
								bool			inSystemTree,
								int32&			outIndex) const;
};

#endif
