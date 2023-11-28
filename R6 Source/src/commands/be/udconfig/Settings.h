/*
 * Copyright 2000 Be, Incorporated.  All Rights Reserved.
 */

#ifndef _UDCONFIG_H_
#define _UDCONFIG_H_

#include <List.h>
#include <Locker.h>

struct Setting {
	char *name;
	char *value;
	int32 cookie;
	int dirty;
};


class SettingsFile {
public:

	SettingsFile(const char *path);
	virtual ~SettingsFile();

	status_t InitCheck() const;
	const char *GetName() const;

	enum {
		S_NONE = 0,
		S_FORCE = 1,
		S_REWRITE = 2
	};

	virtual status_t Load(int32 flags = S_NONE) = 0;
	virtual status_t Save(int32 flags = S_NONE) = 0;

	BList *AcquireSettingsList();
	void ReleaseSettingsList();
	void ClearSettingsList();


private:

	BList fSettingsList;
	BLocker fSettingsLock;
	status_t fErr;
	char *fName;
};


#endif	/* _UDCONFIG_H_ */
