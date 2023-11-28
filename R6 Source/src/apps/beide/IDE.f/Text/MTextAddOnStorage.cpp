// ---------------------------------------------------------------------------
/*
	MTextAddOnStorage.cpp
	
	Copyright (c) 2001 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 May 2001

	Simple interface to handle storing persistent data on behalf of the addon
	Specify the contents of the data as a BMessage
	Name the data with your addon name as a prefix (ie: Comment_xxx)

	Just a wrapper for the MPreferences BMessage handers...
*/
// ---------------------------------------------------------------------------

#include "MPreferences.h"
#include "MTextAddOn.h"

MTextAddOnStorage::MTextAddOnStorage()
{
}

// ---------------------------------------------------------------------------

MTextAddOnStorage::~MTextAddOnStorage()
{
}

// ---------------------------------------------------------------------------

bool
MTextAddOnStorage::DataExists(const char* inName, size_t* outSize)
{
	return MPreferences::MessagePreferenceExists(inName, outSize);
}

// ---------------------------------------------------------------------------

status_t
MTextAddOnStorage::SaveData(const char* inName, const BMessage& inMessage)
{
	return MPreferences::SetMessagePreference(inName, inMessage);
}

// ---------------------------------------------------------------------------

status_t
MTextAddOnStorage::GetData(const char* inName, BMessage& outMessage)
{
	return MPreferences::GetMessagePreference(inName, outMessage);
}

// ---------------------------------------------------------------------------

status_t
MTextAddOnStorage::RemoveData(const char* inName)
{
	return MPreferences::RemoveMessagePreference(inName);
}
