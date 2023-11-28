// a settings file class.
// Copyright (c) 1999-2000, Be Incoporated. All Rights Reserved.

#ifndef _APPPREFS_H_
#define _APPPREFS_H_

#include <Message.h>
#include <FindDirectory.h>
#include <Path.h>

struct attr_info;
class BFile;

class BAppPrefs : public BMessage {
public :
	BAppPrefs(char const*const leafname=NULL,char const*const basename=NULL,const directory_which dir=B_USER_SETTINGS_DIRECTORY);
	BAppPrefs(BMessage const&,char const*const leafname=NULL,char const*const basename=NULL,const directory_which dir=B_USER_SETTINGS_DIRECTORY);

	BAppPrefs(const BAppPrefs&);
	BAppPrefs&operator=(const BAppPrefs&);

	virtual ~BAppPrefs();

	status_t InitCheck() const;

	status_t SetTo(char const*const leafname=NULL,char const*const basename=NULL,const directory_which dir=B_USER_SETTINGS_DIRECTORY);
	const char* Path() const;

	status_t Load(const uint32 flags=0);
	status_t Save(const uint32 flags=0) const;

private:
	virtual void _ReservedAppPrefs1();
	virtual void _ReservedAppPrefs2();
	virtual void _ReservedAppPrefs3();
	virtual void _ReservedAppPrefs4();
	virtual void _ReservedAppPrefs5();
	virtual void _ReservedAppPrefs6();
	virtual void _ReservedAppPrefs7();
	virtual void _ReservedAppPrefs8();
	virtual void _ReservedAppPrefs9();
	virtual void _ReservedAppPrefs10();

	virtual status_t Perform(const perform_code d,void *const arg);

	void _CtorInit(char const*const leafname=NULL,char const*const basename=NULL,const directory_which dir=B_USER_SETTINGS_DIRECTORY);

	static status_t _StoreAttributes(BMessage const*const m,BFile*const f,char const*const basename="");
	static status_t _ExtractAttribute(BMessage *const m,BFile const*const f,char const*const full_name,char const*const partial_name,attr_info const*const ai);
	static bool _SupportsType(const type_code);

	struct _PrivateAppPrefs*_p_AppPrefs;
};

#endif

