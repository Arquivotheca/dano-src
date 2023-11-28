//========================================================================
//	MDefaultTargets.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MDefaultTargets.h"
#include "MPrefsStruct.h"

// ---------------------------------------------------------------------------
//		• SwapBigToHost
// ---------------------------------------------------------------------------

void
SwapTargetsBigToHost(
	TargetPrefs&		inPrefs)
{
	ASSERT(sizeof(TargetRec) == 140);
	if (B_HOST_IS_LENDIAN)
	{
		int32		count = inPrefs.pCount;
		TargetRec*	recs = inPrefs.pTargetArray;
		for (int32 i = 0; i < count; i++)
		{
			recs[i].Flags = B_BENDIAN_TO_HOST_INT16(recs[i].Flags);
		}
	}
}

// ---------------------------------------------------------------------------
//		• SwapHostToBig
// ---------------------------------------------------------------------------

void
SwapTargetsHostToBig(
	TargetPrefs&		inPrefs)
{
	if (B_HOST_IS_LENDIAN)
	{
		int32		count = inPrefs.pCount;
		TargetRec*	recs = inPrefs.pTargetArray;
		for (int32 i = 0; i < count; i++)
		{
			recs[i].Flags = B_HOST_TO_BENDIAN_INT16(recs[i].Flags);
		}
	}
}