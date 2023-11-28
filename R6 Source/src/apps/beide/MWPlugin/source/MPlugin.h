//========================================================================
//	MPlugin.h
//	Copyright 1998 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPLUGIN_H
#define _MPLUGIN_H


#if defined(__POWERPC__) || defined(__ARMEB__)	/* FIXME: This should probably use <endian.h> for the right define */
#define MW_FOUR_CHAR_CODE(x) (x)
#elif defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */

#define MW_FOUR_CHAR_CODE(x) (((ulong) ((x) & 0x000000FF)) << 24) \
							| (((ulong) ((x) & 0x0000FF00)) << 8) \
							| (((ulong) ((x) & 0x00FF0000)) >> 8) \
							| (((ulong) ((x) & 0xFF000000)) >> 24)
#endif

const ulong kMWCCPlugType = MW_FOUR_CHAR_CODE('mwcc');
const ulong kMWLDPlugType = MW_FOUR_CHAR_CODE('mwld');

// ---------------------------------------------------------------------------
// Constants and defines that used to be in PlugHeaders.pch++
// ---------------------------------------------------------------------------

typedef char char64[64];
#define NIL '\0'

#define BUILDINGPPC 1

#if BUILDINGPPC
#define kLangPrefsType kLanguagePrefs
#define kWarningPrefsType kWarningPrefs
#define kGlobalOptsType kGlobalOptsPrefs
#else
#define kLangPrefsType kLanguagePrefsx86
#define kWarningPrefsType kWarningsPrefsx86
#define kGlobalOptsType kGlobalOptsx86
#endif

#endif

