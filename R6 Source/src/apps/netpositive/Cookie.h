// ===========================================================================
//	Cookies.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __COOKIE__
#define __COOKIE__

#include "Utils.h"
#include <SupportDefs.h>
#include <String.h>

class URLParser;
class BLocker;

//====================================================================

class Cookie : public CLinkable {
public:
					Cookie(const char* name,long expires, const char* path, const char* domain, bool secure, bool accept);
					~Cookie();
					
static	int32		Open(void *args);			// All the cookies from disk
static	void		Close();		// All the cookies to disk
		
static	void		Set(URLParser& URL, const char* str);
static	void		Add(URLParser& URL, BString& request);	// Add cookies to http request

static	void		Test();
		
protected:
		bool	SameName(const char* name);
static	void	Parse(const char* str, BString& name, BString& expires, BString& path, BString& domain, bool* secure);
		bool	Match(const char* domain, const char* path);
		
const char*		Name();
const char*		Domain();
const char*		Path();
long			Expires();
bool			Secure();
bool			Accept();

static	CLinkedList	mCookies;
static	bool		mOpen;
static 	TLocker		sLocker;

	BString 	mName;
	BString		mDomain;
	BString		mPath;

	long		mExpires;
	unsigned	mSecure : 1;
	unsigned	mAccept : 1;
};

#endif
