// ---------------------------------------------------------------------------
/*
	PlugInUtil.h
	
	Utility functions for general plugin use.
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 September 1998

*/
// ---------------------------------------------------------------------------

#ifndef _PLUGINUTIL_H
#define _PLUGINUTIL_H

#include <SupportDefs.h>

class BTextView;
class BView;
class MProject;
class BDirectory;
class BList;
class BMessage;
class CommandLineText;

class PlugInUtil 
{
public:
	static void DisallowInvalidChars(BTextView& inText);
	static void SetViewGray(BView* inView);
	static void GetProjectDirectory(MProject* theProject, 
									BDirectory& projectDirectory);
	static void AddMultipleOptions(BList& outList, const char* optionString);
};

// ---------------------------------------------------------------------------


template <class T>
void
UpdateSetting(T& newSettings, const T& defaultSettings, int32 newVersion, const T& oldSettings, int32 oldVersion)
{
	// default is to just copy default over to the new settings since
	// we don't know anything about the versions
	// For specific classes and versions we explicitly instantiate this function
	newSettings = defaultSettings;
}


// ---------------------------------------------------------------------------

template <class T>
bool
ValidateSetting(BMessage& inOutMessage, 
				const char* messageName, 
				type_code messageType, 
				T& defaultSetting)
{
	bool changed = false;
	
	T* message;
	long messageLength;
	if (inOutMessage.HasData(messageName, messageType)) {
		inOutMessage.FindData(messageName, messageType, (const void**) &message, &messageLength);
		message->SwapLittleToHost();
		if (messageLength != sizeof(T) || message->fVersion != T::kCurrentVersion) {
			T newSetting;
			UpdateSetting(newSetting, defaultSetting, T::kCurrentVersion, *message, message->fVersion);
			newSetting.SwapHostToLittle();
			inOutMessage.ReplaceData(messageName, messageType, &newSetting, sizeof(T));
			// (newSetting.SwapLittleToHost() not needed because newSetting is just temporary)
 			changed = true; 
 		}
	}
	else {
		defaultSetting.SwapHostToLittle();
		inOutMessage.AddData(messageName, messageType, &defaultSetting, sizeof(T));
		defaultSetting.SwapLittleToHost();
		changed = true; 
	}
	
	return changed;
}

#endif
