/*
	Preferences.h
	
	Copyright 1997, Hekkelman Programmatuur
	
	Part of Sum-It for the BeBox version 1.1.

*/

#ifndef HPREFERENCES_H
#define HPREFERENCES_H

#include "HError.h"

#include <Locker.h>
#include <GraphicsDefs.h>

#include <map>

#if !__BEOS__
#include <UDebugging.h>
#endif

//using namespace std;

struct HPrefName {
	char name[32];
	
	HPrefName(const HPrefName& n)	{ strcpy(name, n.name); };
	HPrefName(const char *n)		{ if (strlen(n) > 28) throw HErr("Prefname too long"); strcpy(name, n); };
	HPrefName()						{ name[0] = 0; };
	
	bool operator< (const HPrefName& pref) const
	{
		return strcmp(name, pref.name) < 0;
	};
};

typedef std::map<HPrefName, char*> PrefMap;

class HPreferences {
public:
	HPreferences(const char *preffilename);
	virtual ~HPreferences();

	const char *GetPrefString(const char *name, const char *def = NULL);
	const char *GetIxPrefString(const char *name, int ix);
	
	int GetPrefInt(const char *name, int def = 0);
	double GetPrefDouble(const char *name, double def = 0.0);
	template <class C> C GetPrefColor(const char *name, C def)
	{
		PrefMap::iterator pi = fPrefs.find(name);
			
		if (pi != fPrefs.end())
		{
			rgb_color c;
			char s[4], *p;
			const char *v = (*pi).second;
			
			s[2] = 0;
			
			strncpy(s, v + 1, 2);
			c.red = strtoul(s, &p, 16);
			
			strncpy(s, v + 3, 2);
			c.green = strtoul(s, &p, 16);
			
			strncpy(s, v + 5, 2);
			c.blue = strtoul(s, &p, 16);
			
			return c;
		}
		else
		{
			SetPrefColor(name, def);
			return def;
		}
	} /* HPreferences::GetPrefColor */
	template <class R> R GetPrefRect(const char *name, R def)
	{
		PrefMap::iterator pi = fPrefs.find(name);
		
		if (pi != fPrefs.end())
		{
			R r;
			r.left = r.top = r.right = r.bottom = 0;
	
			const char *v = (*pi).second;
			char *p;
			
			r.left = strtod(v, &p);
			p++;
			r.top = strtod(p, &p);
			p++;
			r.right = strtod(p, &p);
			p++;
			r.bottom = strtod(p, &p);
			
			return r.right > r.left && r.bottom > r.top ? r : def;
		}
		else
		{
			SetPrefRect(name, def);
			return def;
		}
	} /* HPreferences::GetPrefRect */

	void SetPrefString(const char *name, const char *value);
	void SetIxPrefString(const char *name, int ix, const char *value);
	
	void SetPrefInt(const char *name, int value);
	void SetPrefDouble(const char *name, double value);
	template <class C> void SetPrefColor(const char *name, C Value)
	{
		rgb_color value = Value;
		
		char c[10];
		sprintf(c, "#%2.2x%2.2x%2.2x", value.red, value.green, value.blue);
		SetPrefString(name, c);
	} /* HPreferences::SetPrefColor */
	template <class R> void SetPrefRect(const char *name, R value)
	{
		char s[64];
		sprintf(s, "%g,%g,%g,%g", (float)value.left, (float)value.top,
			(float)value.right, (float)value.bottom);
		SetPrefString(name, s);
	} /* HPreferences::SetPrefRect */
	
	void RemovePref(const char *name);
	
	void ReadPrefFile();
	void WritePrefFile();
	void ResetAll();

#if !__BEOS__
	const FSSpec& Spec() const
		{ return fFile; }
#endif	

protected:
	void Clear();

	PrefMap fPrefs;
	
#if __BEOS__
	char *fFile;
	BLocker fLock;
#else
	FSSpec fFile;
#endif
};

extern HPreferences *gPrefs;


#endif

