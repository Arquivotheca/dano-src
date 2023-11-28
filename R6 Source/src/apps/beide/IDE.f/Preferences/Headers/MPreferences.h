//========================================================================
//	MPreferences.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Object for handling preferences

#ifndef _MPREFERENCES_H
#define _MPREFERENCES_H

#include "IDEConstants.h"
#include <SupportDefs.h>

typedef uint32 PrefType;

class String;
class BMessage;
class BLocker;
struct entry_ref;

class MPreferences
{
public:

	static status_t		OpenPreferences(const char* name);
	static status_t		ClosePreferences();

//	If defaultDataLen is > 0, and the preference is not found,
//	the value already in outData is stored for that preference
//	with the length given in defaultDataLen.
	static status_t		GetPreference(const char* inName,
									  PrefType inType,
									  void* outData,
									  size_t& inOutSize);
	static status_t		GetMessagePreference(const char* inName, 
											 BMessage& outMessage);
										  
	static status_t		SetPreference(const char* inName,
									  PrefType inType,
									  const void* inData,
									  size_t inSize);

	static status_t		SetMessagePreference(const char* inName, 
											 const BMessage& inMessage);

	static status_t		RemovePreference(const char* inName,
										 PrefType inType);

	static status_t		RemoveMessagePreference(const char* inName);
									
	static bool			PreferenceExists(const char* inName,
										 PrefType inType,
										 size_t* outSize = NULL);

	static bool			MessagePreferenceExists(const char* inName,
												size_t* outSize = NULL);
									
	static void			Reset(int32 prefVersion);

	static void			GetRef(entry_ref& outRef);

private:

	static	BLocker		fPrefLock;
	static	entry_ref	fRef;

};

#endif
