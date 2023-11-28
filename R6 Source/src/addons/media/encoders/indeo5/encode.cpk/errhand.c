/************************************************************************
*																		*
*				INTEL CORPORATION PROPRIETARY INFORMATION				*
*																		*
*	 This listing is supplied under the terms of a license agreement	*
*	   with INTEL Corporation and may not be copied nor disclosed		*
*		 except in accordance with the terms of that agreement.			*
*																		*
*************************************************************************
*																		*
*				Copyright (C) 1994-1997 Intel Corp.                       *
*						  All Rights Reserved.							*
*																		*
************************************************************************/

#include "datatype.h"
#include "errhand.h"

#ifdef DEBUG
/* This list of files matches the #defines in errhand.h */
/* This array is not modified, but if it is const then it
 * can't be passed to HivePrintString due to a level 1 warning */
char gac8Files[NUMFILES][NAMELENGTH] = {	
			"  bidism\0", 
			"   bsdbg\0",
			"  bsutil\0",
			"    enbs\0",
			"enmesrch\0",
			"  enntry\0",
			"enntryns\0",
			"   enwav\0",
			"hufftbls\0",
			"  matrix\0",
			"      mc\0",
			"   tksys\0",
			"  encpia\0",
			" entrans\0",
			"   enseg\0",
			"    cdec\0",
			"dentryns\0",
			"   dewav\0",
			"  dexfrm\0",
			" parsebs\0"};
#endif
