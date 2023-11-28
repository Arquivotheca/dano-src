//========================================================================
//	MPreferences.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//  Copyright 1998-1999 Be Incorporated, All Rights Reserved.
//========================================================================	
//	Object for handling preferences
//	MPreferences have both a name and a type.  The names are unique and serve to identify
//	each pref uniquely.  The types can be reused.

#include "MPreferences.h"
#include "MLocker.h"

#include <StorageKit.h>
#include <KernelKit.h>
#include <Message.h>
#include <Debug.h>

BLocker MPreferences::fPrefLock;
entry_ref MPreferences::fRef;

// ---------------------------------------------------------------------------
// MPreferences member functions
// ---------------------------------------------------------------------------

status_t
MPreferences::OpenPreferences(const char* fileName)
{
	MLocker<BLocker>	lock(fPrefLock);
	BPath				path;
	status_t			err = find_directory (B_USER_SETTINGS_DIRECTORY, &path, true);

	BDirectory			settingsDir(path.Path());
	BEntry				metrowerks(&settingsDir, "Metrowerks");
	BDirectory			metroDir(&metrowerks);

	if (!metrowerks.Exists())
	{
		err = settingsDir.CreateDirectory("Metrowerks", &metroDir);
	}

	BEntry				prefFileEntry;

	if (metrowerks.Exists() && B_NO_ERROR != metroDir.FindEntry(fileName, &prefFileEntry))
	{
		BFile				prefFile;
		err = metroDir.CreateFile(fileName, &prefFile);
	}

	if (B_NO_ERROR == metroDir.FindEntry(fileName, &prefFileEntry))
		err = prefFileEntry.GetRef(&fRef);	

	return err;
}

// ---------------------------------------------------------------------------

status_t
MPreferences::ClosePreferences()
{
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------

bool
MPreferences::PreferenceExists(
	const char * 	inName,
	PrefType		inType,
	size_t * 		outSize)	// can be nil
{
	MLocker<BLocker>	lock(fPrefLock);
	BNode				file(&fRef);
	status_t			err;
	attr_info			info;

	err = file.GetAttrInfo(inName, &info);
	if (err == B_NO_ERROR)
	{
		ASSERT(inType == info.type);
		if (inType != info.type)
			err = B_ERROR;
		if (outSize != nil)
			*outSize = info.size;
	}
	
	return err == B_NO_ERROR;
}

// ---------------------------------------------------------------------------

bool
MPreferences::MessagePreferenceExists(const char* inName, size_t* outSize)
{
	return MPreferences::PreferenceExists(inName, B_MESSAGE_TYPE, outSize);
}

// ---------------------------------------------------------------------------

status_t
MPreferences::GetPreference(
	const char * 	inName,
	PrefType		inType,
	void * 			outData,
	size_t&			inOutSize)
{
	ASSERT(outData != nil);

	MLocker<BLocker>	lock(fPrefLock);
	BNode				file(&fRef);
	status_t			err;
	attr_info			info;

	err = file.GetAttrInfo(inName, &info);
	if (err == B_NO_ERROR && inType != info.type)
		err = B_ERROR;
	if (err == B_NO_ERROR)
	{
		ssize_t	size = file.ReadAttr(inName, inType, 0, outData, inOutSize);
		err = size != inOutSize ? B_ERROR : err;
		inOutSize = size;
	}
	
	return err;
}

// ---------------------------------------------------------------------------

status_t
MPreferences::SetPreference(
	const char * 	inName,
	PrefType		inType,
	const void * 	inData,
	size_t 			inSize)
{
	MLocker<BLocker>	lock(fPrefLock);
	BNode				file(&fRef);
	status_t			err;
	attr_info			info;
	ssize_t				size;

	err = file.GetAttrInfo(inName, &info);

	if (err == B_NO_ERROR && info.size > inSize)
		err = file.RemoveAttr(inName);


	if (err == B_NO_ERROR || err == ENOENT)
	{
		// Attribute doesn't already exist
		size = file.WriteAttr(inName, inType, 0, inData, inSize);
		err = size == inSize ? B_NO_ERROR : B_ERROR;		
	}
	
	return err;
}

// ---------------------------------------------------------------------------

status_t
MPreferences::RemovePreference(
	const char * 	inName,
	PrefType		inType)
{
	MLocker<BLocker>	lock(fPrefLock);
	BNode				file(&fRef);
	status_t			err;
	attr_info			info;

	err = file.GetAttrInfo(inName, &info);
	if (err == B_NO_ERROR && inType != info.type)
		err = B_ERROR;
	if (err == B_NO_ERROR)
	{
		err = file.RemoveAttr(inName);
	}
	
	return err;
}

// ---------------------------------------------------------------------------

status_t
MPreferences::RemoveMessagePreference(const char* inName)
{
	return MPreferences::RemovePreference(inName, B_MESSAGE_TYPE);
}

// ---------------------------------------------------------------------------

void
MPreferences::Reset(
	int32 prefVersion)
{
	MLocker<BLocker>	lock(fPrefLock);
	BNode				file(&fRef);
	status_t			err;
	attr_name_t			name;
	
	while ((err = file.GetNextAttrName(name)) != B_NO_ERROR)
	{
		err = file.RemoveAttr(name);
	}

	(void) file.WriteAttr("version", 'pref', 0, &prefVersion, sizeof(prefVersion));
}

// ---------------------------------------------------------------------------

void
MPreferences::GetRef(
	entry_ref&	outRef)
{
	outRef = fRef;
}

// ---------------------------------------------------------------------------

status_t
MPreferences::SetMessagePreference(const char* inName, const BMessage& inMessage)
{
	// Flatten and save a BMessage under the preference name inName
	// (useful for variable length structures)
	// (AccessPaths could use this, but they are currently working with 
	// their old method)
	
	MLocker<BLocker> lock(fPrefLock);
	BNode file(&fRef);

	ssize_t size = inMessage.FlattenedSize();
	char *buffer = new char[size];
	status_t result = inMessage.Flatten(buffer, size);
	if (result == B_OK) {
		ssize_t bytesWritten = file.WriteAttr(inName, B_MESSAGE_TYPE, 0, buffer, size);
		if (bytesWritten != size) {
			result = B_ERROR;
		}
	}
	
	delete [] buffer;
	return result;
}

// ---------------------------------------------------------------------------

status_t
MPreferences::GetMessagePreference(const char* inName, BMessage& outMessage)
{
	// Get and unflatten a preference saved as a flattened BMessage
	// (see comments in SetMessagePreference)
	
	MLocker<BLocker> lock(fPrefLock);
	BNode file(&fRef);
	
	attr_info info;
	status_t result = B_ERROR;
	if (file.GetAttrInfo(inName, &info) == B_OK) {
		char* buffer = new char[info.size];
		if (file.ReadAttr(inName, B_MESSAGE_TYPE, 0, buffer, info.size) == info.size) {
			result = outMessage.Unflatten(buffer);
		}
		delete [] buffer;
	}
	
	return result;
}
