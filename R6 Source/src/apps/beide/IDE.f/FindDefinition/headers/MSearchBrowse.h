//========================================================================
//	MSearchBrowse.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MSEARCHBROWSE_H
#define _MSEARCHBROWSE_H

#include <StorageDefs.h>


bool SearchBrowseDataForIdentifier(void*	 browseData, 
			const char* name, bool fSearchStatics, 
			int32* offset, bool* fIsCode);		

#endif
