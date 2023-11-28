/*
	TPreferences.cpp
	Implementation.
*/

#include <FindDirectory.h>
#include "TPreferences.h"

//
// TPreferences::TPreferences
//
// Open the settings file and read the data in.
//
TPreferences::TPreferences(const char *filename) : BMessage('pref')
{
	BFile file;

	fStatus = find_directory(B_COMMON_SETTINGS_DIRECTORY, &fPath);
	if (fStatus != B_OK)
	{
		return;
	}

	fPath.Append(filename);
	fStatus = file.SetTo(fPath.Path(), B_READ_ONLY);
	if (fStatus == B_OK)
	{
		fStatus = Unflatten(&file);
	}
}

//
// TPreferences::~TPreferences
//
// Write the preferences to disk.
//
TPreferences::~TPreferences()
{
	BFile file;

	if (file.SetTo(fPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) == B_OK)
	{
		Flatten(&file);
	}
}

status_t TPreferences::InitCheck(void)
{
	BAutolock	autolocker(fLock);
	return fStatus;
}

status_t TPreferences::SetBool(const char *name, bool b)
{
	BAutolock	autolocker(fLock);
	if (HasBool(name))
	{
		return ReplaceBool(name, 0, b);
	}
	return AddBool(name, b);
}

status_t TPreferences::SetInt8(const char *name, int8 i)
{
	BAutolock	autolocker(fLock);
	if (HasInt8(name))
	{
		return ReplaceInt8(name, 0, i);
	}
	return AddInt8(name, i);
}

status_t TPreferences::SetInt16(const char *name, int16 i)
{
	BAutolock	autolocker(fLock);
	if (HasInt16(name))
	{
		return ReplaceInt16(name, 0, i);
	}
	return AddInt16(name, i);
}

status_t TPreferences::SetInt32(const char *name, int32 i)
{
	BAutolock	autolocker(fLock);
	if (HasInt32(name))
	{
		return ReplaceInt32(name, 0, i);
	}
	return AddInt32(name, i);
}

status_t TPreferences::SetInt64(const char *name, int64 i)
{
	BAutolock	autolocker(fLock);
	if (HasInt64(name))
	{
		return ReplaceInt64(name, 0, i);
	}
	return AddInt64(name, i);
}

status_t TPreferences::SetFloat(const char *name, float f)
{
	BAutolock	autolocker(fLock);
	if (HasFloat(name))
	{
		return ReplaceFloat(name, 0, f);
	}
	return AddFloat(name, f);
}

status_t TPreferences::SetDouble(const char *name, double f)
{
	BAutolock	autolocker(fLock);
	if (HasDouble(name))
	{
		return ReplaceDouble(name, 0, f);
	}
	return AddDouble(name, f);
}

status_t TPreferences::SetString(const char *name, const char *s)
{
	BAutolock	autolocker(fLock);
	if (HasString(name))
	{
		return ReplaceString(name, 0, s);
	}
	return AddString(name, s);
}

status_t TPreferences::SetPoint(const char *name, BPoint p)
{
	BAutolock	autolocker(fLock);
	if (HasPoint(name))
	{
		return ReplacePoint(name, 0, p);
	}
	return AddPoint(name, p);
}

status_t TPreferences::SetRect(const char *name, BRect r)
{
	BAutolock	autolocker(fLock);
	if (HasRect(name))
	{
		return ReplaceRect(name, 0, r);
	}
	return AddRect(name, r);
}

status_t TPreferences::GetBool(const char *name, bool *b)
{
	BAutolock	autolocker(fLock);
	return FindBool(name, b);
}

status_t TPreferences::GetInt8(const char *name, int8 *i)
{
	BAutolock	autolocker(fLock);
	return FindInt8(name, i);
}

status_t TPreferences::GetInt16(const char *name, int16 *i)
{
	BAutolock	autolocker(fLock);
	return FindInt16(name, i);
}

status_t TPreferences::GetInt32(const char *name, int32 *i)
{
	BAutolock	autolocker(fLock);
	return FindInt32(name, i);
}

status_t TPreferences::GetInt64(const char *name, int64 *i)
{
	BAutolock	autolocker(fLock);
	return FindInt64(name, i);
}

status_t TPreferences::GetFloat(const char *name, float *f)
{
	BAutolock	autolocker(fLock);
	return FindFloat(name, f);
}

status_t TPreferences::GetDouble(const char *name, double *f)
{
	BAutolock	autolocker(fLock);
	return FindDouble(name, f);
}

status_t TPreferences::GetString(const char *name, const char **s)
{
	BAutolock	autolocker(fLock);
	return FindString(name, s);
}

status_t TPreferences::GetPoint(const char *name, BPoint *p)
{
	BAutolock	autolocker(fLock);
	return FindPoint(name, p);
}

status_t TPreferences::GetRect(const char *name, BRect *r)
{
	BAutolock	autolocker(fLock);
	return FindRect(name, r);
}
