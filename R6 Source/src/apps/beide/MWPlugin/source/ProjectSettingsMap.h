// ---------------------------------------------------------------------------
/*
	ProjectSettingsMap.h
	
	Provides an implementation of a map between a project and a group
	of settings associated with that project.
	
	The settings are implemented as a template class.
	The only requirements are a constructor and destructor.
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			6 July 1999

*/
// ---------------------------------------------------------------------------

#ifndef _PROJECTSETTINGSMAP_
#define _PROJECTSETTINGSMAP_

class MProject;

#include "MList.h"
#include <Locker.h>
#include <Autolock.h>

template <class T>
class ProjectSettingsMap
{
public:
				ProjectSettingsMap();
				~ProjectSettingsMap();
								
	T*			GetSettings(MProject& inProject);
	T*			AddSettings(MProject& inProject);
	void		RemoveSettings(MProject& inProject);
	
private:
	MList<MProject*>	fProjectList;
	MList<T*>			fSettingsList;
	
	BLocker				fLock;
};

// ---------------------------------------------------------------------------
//	ProjectMap member functions
// ---------------------------------------------------------------------------

template <class T>
ProjectSettingsMap<T>::ProjectSettingsMap()
{
}

// ---------------------------------------------------------------------------

template <class T>
ProjectSettingsMap<T>::~ProjectSettingsMap()
{
	BAutolock lock(fLock);
	int32 count = fSettingsList.CountItems();
	for (int32 i = 0; i < count; i++) {
		T* setting = fSettingsList.ItemAt(i);
		delete setting;
	}
}

// ---------------------------------------------------------------------------
								
template <class T>
T*
ProjectSettingsMap<T>::GetSettings(MProject& inProject)
{
	BAutolock lock(fLock);

	T* cache = NULL;	
	int32 index = fProjectList.IndexOf(&inProject);
	if (index >= 0) {
		cache = fSettingsList.ItemAt(index);
	}
	
	return cache;
}

// ---------------------------------------------------------------------------

template <class T>
T*
ProjectSettingsMap<T>::AddSettings(MProject& inProject)
{
	// Add this cache to our list of project caches...
	// Since we will probably use these first, add them to the front
	BAutolock lock(fLock);
	
	T* newCache = new T;
	fProjectList.AddItem(&inProject, 0);
	fSettingsList.AddItem(newCache, 0);
	return newCache;
}

// ---------------------------------------------------------------------------

template <class T>
void
ProjectSettingsMap<T>::RemoveSettings(MProject& inProject)
{
	// Find the project and settings and delete them from the lists
	BAutolock lock(fLock);
	
	int32 index = fProjectList.IndexOf(&inProject);
	if (index >= 0) {
		fProjectList.RemoveItemAt(index);
		T* cache = fSettingsList.RemoveItemAt(index);
		delete cache;
	}
}

#endif
