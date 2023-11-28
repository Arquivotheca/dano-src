/*
	Preferences.c
	
	Copyright 1997, Hekkelman Programmatuur
	
	Part of Sum-It for the BeBox version 1.1.

*/

#include "HPreferences.h"
#include "lib.h"

#include <stdio.h>
#include <string.h>

#include <Debug.h>

#if __BEOS__
#	include <Autolock.h>
#	include <Path.h>
#	include <FindDirectory.h>
#	define BLOCK		BAutolock lock(fLock); if (! lock.IsLocked()) return
#else
#	include <FSp_fopen.h>
#	include <UDebugging.h>
#	include <stdio.h>
#	include "Utils.h"
#	include "BeCompat.h"

#	define BLOCK

#endif

HPreferences *gPrefs = NULL;

HPreferences::HPreferences(const char *preffilename)
{
#if __BEOS__
	BPath settings;
	
	FailOSErr(find_directory(B_USER_SETTINGS_DIRECTORY, &settings, true));

	char p[PATH_MAX];
	strcpy(p, settings.Path());
	strcat(p, "/");
	strcat(p, preffilename);
	fFile = strdup(p);
	FailNil(fFile);
#else
	short ref;
	long dir;
	Str255 name;
	
//	c2pstrcpy(name, preffilename);
	name[0] = strlen(preffilename);
	memcpy(name+1, preffilename, name[0]);

	OSErr err = FindFolder(0, kPreferencesFolderType, true, &ref, &dir);
	
	if (err == noErr)
		err = FSMakeFSSpec(ref, dir, name, &fFile);

//	if (err != fnfErr)
//		throw err;
#endif
} /* HPreferences::HPreferences */

HPreferences::~HPreferences()
{
	PrefMap::iterator pi;
	
	for (pi = fPrefs.begin(); pi != fPrefs.end(); pi++)
		free((*pi).second);
	
	fPrefs.clear();

#if __BEOS__
	free(fFile);
#endif
} /* HPreferences::~HPreferences */

const char *HPreferences::GetPrefString(const char *name, const char *def)
{
	PrefMap::iterator pi = fPrefs.find(name);
	
	if (pi != fPrefs.end())
		return (*pi).second;
	else
	{
		fPrefs[name] = strdup(def);
		return def;
	}
} /* HPreferences::GetPrefString */

const char *HPreferences::GetIxPrefString(const char *name, int ix)
{
	char n[32];
	ASSERT(strlen(name) < 28);
	
	sprintf(n, "%s%d", name, ix);
	
	PrefMap::iterator pi = fPrefs.find(n);
	
	if (pi != fPrefs.end())
		return (*pi).second;
	else
		return NULL;
} /* HPreferences::GetIxPrefString */

int HPreferences::GetPrefInt(const char *name, int def)
{
	PrefMap::iterator pi = fPrefs.find(name);
	
	if (pi != fPrefs.end())
		return atoi((*pi).second);
	else
	{
		SetPrefInt(name, def);
		return def;
	}
} /* HPreferences::GetPrefInt */

double HPreferences::GetPrefDouble(const char *name, double def)
{
	PrefMap::iterator pi = fPrefs.find(name);
	
	if (pi != fPrefs.end())
		return atof((*pi).second);
	else
	{
		SetPrefDouble(name, def);
		return def;
	}
} /* HPreferences::GetPrefDouble */

void HPreferences::SetPrefString(const char *name, const char *value)
{
	BLOCK;

	if (name && value)
	{
		PrefMap::iterator pi = fPrefs.find(name);
		
		if (pi != fPrefs.end())
		{
			free((*pi).second);
			(*pi).second = strdup(value);
		}
		else
			fPrefs[name] = strdup(value);
	}
} /* HPreferences::SetPrefString */

void HPreferences::SetIxPrefString(const char *name, int ix, const char *value)
{
	ASSERT(strlen(name) < 28);
	char n[32];
	sprintf(n, "%s%d", name, ix);
	
	SetPrefString(n, value);
} /* HPreferences::SetIxPrefString */

void HPreferences::SetPrefInt(const char *name, int value)
{
	char s[10];
	sprintf(s, "%d", value);
	SetPrefString(name, s);
} /* HPreferences::SetPrefInt */

void HPreferences::SetPrefDouble(const char *name, double value)
{
	char s[20];
	sprintf(s, "%g", value);
	SetPrefString(name, s);
} /* HPreferences::SetPrefDouble */

void HPreferences::RemovePref(const char *name)
{
	PrefMap::iterator pi = fPrefs.find(name);
	
	if (pi != fPrefs.end())
	{
		free((*pi).second);
		fPrefs.erase(pi);
	}
	
	if (strlen(name) < 28)
	{
		int ix = 0;
		char n[32];
		
		do
		{
			sprintf(n, "%s%d", name, ix++);
			
			pi = fPrefs.find(n);
	
			if (pi != fPrefs.end())
			{
				free((*pi).second);
				fPrefs.erase(pi);
			}
		}
		while (pi != fPrefs.end());
	}
} /* HPreferences::RemovePref */

void HPreferences::ReadPrefFile()
{
	BLOCK;

	FILE *f;
	
#if __BEOS__
	f = fopen(fFile, "r");
#else
	f = FSp_fopen(&fFile, "r");
#endif
	
	if (!f)
		return;
	
	char s[2048];
	
	while (fgets(s, 2047, f))
	{
		char *n, *v;
		
		n = s;
		v = strchr(s, '=');
		if (!v)
			continue;
		
		*v++ = 0;
		v[strlen(v) - 1] = 0;
		
		char *p = v;
		
		while ((p = strchr(p, 0x1b)) != 0)
		{
			if (p[1] == 'n')
			{
				memmove(p + 1, p + 2, strlen(p + 2) + 1);
				*p = '\n';
			}
			else if (p[1] == 0x1b)
				p++;
			p++;
		}
		
		fPrefs[n] = strdup(v);
	}
	
	fclose(f);
} /* HPreferences::ReadPrefFile */

void HPreferences::WritePrefFile()
{
	BLOCK;

	FILE *f;
	
#if __BEOS__
	f = fopen(fFile, "w");
#else
	f = FSp_fopen(&fFile, "w");
#endif
	
	if (!f)
		throw HErr("Could not create settings file");
	
	PrefMap::iterator pi;
	
	for (pi = fPrefs.begin(); pi != fPrefs.end(); pi++)
	{
		if (!(*pi).second)
			continue;
		else if (strchr((*pi).second, '\n') == NULL)
			fprintf(f, "%s=%s\n", (*pi).first.name, (*pi).second);
		else
		{
			char *buf = (char *)malloc(2 * strlen((*pi).second));
			FailNil(buf);
			
			char *a, *b;
			a = buf, b = (*pi).second;
			
			while (*b)
			{
				if (*b == '\n')
				{
					*a++ = 0x1b;
					*a++ = 'n';
					b++;
				}
				else if (*b == 0x1b)
				{
					*a++ = 0x1b;
					*a++ = 0x1b;
					b++;
				}
				else
					*a++ = *b++;
			}

			*a = 0;

			fprintf(f, "%s=%s\n", (*pi).first.name, buf);
			free(buf);
		}
	}
	
	fclose(f);

#if !__BEOS__
	FInfo info;

	FSpGetFInfo(&fFile, &info);
	info.fdType = 'TEXT';
	info.fdCreator = 'HkIt';
	FSpSetFInfo(&fFile, &info);
#endif
} /* HPreferences::WritePrefFile */

void HPreferences::ResetAll()
{
	BLOCK;

	fPrefs.erase(fPrefs.begin(), fPrefs.end());
} // HPreferences::ResetAll
