// ===========================================================================
//	Cookie.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "Cookie.h"
#include "Store.h"
#include "Cache.h"
#include "URL.h"
#include "NPApp.h"
#include "MessageWindow.h"

#include <malloc.h>
#include <Alert.h>
#include <stdio.h>
#include <parsedate.h>

extern const char *kCookieFileName;
extern const char *kCookieAskMessage;
extern const char *kCookieAccept;
extern const char *kCookieReject;

//====================================================================

bool		Cookie::mOpen = false;
CLinkedList Cookie::mCookies;
TLocker		Cookie::sLocker("Cookie Lock");

//====================================================================
// Create a new cookie, add it to list


Cookie::Cookie(const char* name, long expires, const char* path, const char* domain, bool secure, bool accept) 
	: mName(name), mDomain(domain), mPath(path), mExpires(expires), mSecure(secure), mAccept(accept)
{
	sLocker.Lock();
	mCookies.Add(this);
	sLocker.Unlock();
}

Cookie::~Cookie()
{
}

char* NextField(char *str)
{
	str = strchr(str,' ');
	if (str) {
		str[0] = 0;
		str++;
	}
	return str;
}

long NextStr(char *d, long count, char *dst);

//	Read all cookies from disk

int32 Cookie::Open(void *)
{
	char* str = (char*)malloc(8192);	// Cookies can be quite large
//	NP_ASSERT(str);
	
	Store *store = UResourceCache::ReadCacheData(kCookieFileName);
	if (store == NULL)
		return 0;

	sLocker.Lock();

	long count = store->GetLength();
	char *d = (char *)store->GetData(0,count);
	while (count) {
		int i = NextStr(d,count,str);

		char *domain,*path,*expires,*name;
		char *s = str;
		bool secure = false;
		bool accept = true;
		
		if (s[0] != '#') {
			domain = s;
			if ((bool)(s = NextField(s))) {		// Load a cookie
				path = s;
				if ((bool)(s = NextField(s))) {
					expires = s;
					if ((bool)(s = NextField(s))) {
						name = s;
						if ((bool)(s = NextField(s))) {
							long e;
							if (strncmp(s, "TRUE", 4) == 0)
								secure = true;
							if ((bool)(s = NextField(s))) {
								if (strcmp(s, "REJECT\n") == 0)
									accept = false;
							}
							sscanf(expires,"%ld",&e);
							new Cookie(name,e,path,domain,secure, accept);
						}
					}
				}
			}
		}
		
		d += i;
		count -= i;
		if (i == 0) break;
	}
	
	free(str);
	delete store;
	mOpen = true;
	
	sLocker.Unlock();
	return 0;
}

//	Log all Cookies to disk

void Cookie::Close()
{

	BucketStore* store = new BucketStore;
	store->Write("#Cookies\n",strlen("#Cookies\n"));

	Cookie* c;
	sLocker.Lock();
	for (c = (Cookie *)mCookies.First(); c; c = (Cookie *)c->Next()) {
		if (c->Expires() != 0) {
			BString str;
			
			str = c->Domain();
			str += " ";
			str += c->Path();
			
			char exp[128];
			sprintf(exp," %ld ",c->Expires());
			str += exp;
			
			if (c->Accept())
				str += c->Name();
			else
				str += ".";
			str += c->Secure() ? " TRUE" : " FALSE";
			str += c->Accept() ? " ACCEPT\n" : " REJECT\n";
			
			store->Write(str.String(),str.Length());			// Cache log entry holds all http fields
		}
	}
	sLocker.Unlock();

	store->Write("#Cookies\n",strlen("#Cookies\n"));
	
	UResourceCache::WriteCacheData(kCookieFileName,"text/plain",store);
	delete store;
	mOpen = false;
	
}

//	Does this cookie have the same name?

bool Cookie::SameName(const char* name)
{
	int nameSize = strchr(name,'=') - name;
	int nameSize2 = mName.FindFirst('=');
	if (nameSize != nameSize2)
		return false;
	return strncmp(name,mName.String(),nameSize) == 0;
}

//	NAME=VALUE; expires=DATE; path=PATH; domain=DOMAIN_NAME; secure
//	Set a cookie, add to list

void Cookie::Set(URLParser& URL, const char* str)
{
	BString name,exp,path,domain;
	bool secure;
	Parse(str,name,exp,path,domain,&secure);
	
	if (domain.Length() == 0)
		domain = URL.HostName();
	if (path.Length() == 0)
		path = URL.Path();
	if (path.Length() == 0)
		path = "/";
		
	long expires = 0x7fffffff;
	if (exp.Length() > 0)
		expires = parsedate(exp.String(), -1);	// Make a real date here....еееееееееееееееее
		
//	See if cookie is already there

	sLocker.Lock();
	Cookie* c;
	for (c = (Cookie *)mCookies.First(); c; c = (Cookie *)c->Next()) {
		if (c->Match(domain.String(),path.String())) {
			if (c->SameName(name.String())) {
				c->mName = name;
				c->mExpires = expires;
				pprint("Cookie replaced: %s,%s,%s, %ld",domain.String(),path.String(),name.String(), expires);
				sLocker.Unlock();
				return;
			}
		}
	}
	
	int32 cookieOption = gPreferences.FindInt32("CookieOption");
	bool accept = true;
	if (cookieOption == 1) {
		accept = false;
	} else if (cookieOption == 2) {
		char *cookieMessage = (char *)malloc(strlen(kCookieAskMessage) + domain.Length() + path.Length() + 1);
		sprintf(cookieMessage, kCookieAskMessage, domain.String(), path.String());
		BAlert *alert = new BAlert("",cookieMessage ? cookieMessage : kCookieAskMessage,kCookieAccept, kCookieReject);
		if (alert->Go() == 1)
			accept = false;
		if (cookieMessage)
			free(cookieMessage);
	}
	
	if (!accept)
		name = ".";
	
	sLocker.Unlock();

//	Add a new cookie
		
	pprint("New Cookie: %s; %s; %s; %s",name.String(),exp.String(),path.String(),domain.String());
	new Cookie(name.String(),expires,path.String(),domain.String(),secure,accept);
}

//	Handy string utils

char*  SkipChars(const char* str, const char* skip)
{
	while (str[0] && strchr(skip, str[0]))
		str++;
	return (char *)str;
}

char*  SkipStr(const char* str, const char* skip, bool caseSensitive)
{
	int i = strlen(skip);
	if (caseSensitive) {
		if (strncmp(skip,str,i) == 0)
			return (char *)(str + i);
	} else {
		if (strncasecmp(skip,str,i) == 0)
			return (char *)(str + i);
	}
	return NULL;
}


//	Break down response string into its components

void Cookie::Parse(const char* inStr, BString& name, BString& expires, BString& path, BString& domain, bool *secure)
{
	char *copiedStr = (char *)malloc(strlen(inStr) + 1);
	char *str = copiedStr;
	strcpy(str, inStr);
	
	expires = "";
	path = "";
	domain = "";

	*secure = false;
	
	name = str;				// NAME=VALUE
	int32 semicolonPos = name.FindFirst(';');
	if (semicolonPos >= 0)
		name.Truncate(semicolonPos);
	str = strchr(str,';');
	if (str == NULL) {
		free(copiedStr);
		return;
	}
	str[0] = 0;					// Crop name

	while (str[1]) {
		str = SkipChars(str + 1," ");
		
		const char *p;
		if ((bool)(p = SkipStr(str,"expires", false))) {
			expires = SkipChars(p,"=");
			semicolonPos = expires.FindFirst(';');
			if (semicolonPos >= 0)
				expires.Truncate(semicolonPos);
		}
		if ((bool)(p = SkipStr(str,"path", false))) {
			char *tmppath = SkipChars(p,"=");
			path = tmppath;
			// Sigh.  Some servers forget the semicolon after the path, and predictably,
			// other browsers don't care.  Since a space is illegal in a path, we'll treat
			// it as a delimiter here.
			int32 spacePos = path.FindFirst(' ');
			semicolonPos = path.FindFirst(';');
			if (spacePos > 0 && (spacePos < semicolonPos || semicolonPos < 0)) {
				semicolonPos = spacePos;
				tmppath[spacePos] = ';';
			}
			if (semicolonPos >= 0)
				path.Truncate(semicolonPos);
		}
		if ((bool)(p = SkipStr(str,"domain", false))) {
			domain = SkipChars(p,"=");
			semicolonPos = domain.FindFirst(';');
			if (semicolonPos >= 0)
				domain.Truncate(semicolonPos);
		}
		if ((bool)(p = SkipStr(str,"secure", false)))
			*secure = true;
			
		str = strchr(str,';');
		if (str == NULL) {
			free(copiedStr);
			return;
		}
		str[0] = 0;
	}
	free(copiedStr);

}

//	See if this cookie is for this domain (.netscape.com -> www.netscape.com)

bool Cookie::Match(const char* domain, const char* path)
{
	// If the cookie has expired, set its expires value to 0 so that it won't get written out
	// to disk when we quit.  Expired cookies never match.
	if (mExpires > 0 && mExpires < time(NULL)) {
		pprint("Cookie %s/%s expired, deleting\n", mName.String(), mDomain.String());
		mExpires = 0;
		return false;
	}

	// Cookie is expired, no match.
	if (mExpires == 0)
		return false;
		
	//pprint("COOKIEMATCH: %s%s [%s%s]",domain,path,(char*)mDomain,(char*)mPath);
	if (mDomain.Length() > (int32)strlen(domain))
		return false;
		
	domain += strlen(domain) - mDomain.Length();
	if (mDomain == domain) {
		if (strncmp(mPath.String(),path,mPath.Length()) == 0) {
			//pprint("COOKIEMATCH: %s%s [%s]",domain,path,(char*)mName);
			return true;
		}
	}
	return false;
}

//	Add all relevant cookies to HTTP request

void Cookie::Add(URLParser& URL, BString& request)
{
	const char *path = URL.Path();
	if (!path || !(*path))
		path = "/";
	const char *domain = URL.HostName();
	if (!domain || !(*domain))
		return;
	
	bool foundOne = false;
	
	Cookie* c;
	sLocker.Lock();
	for (c = (Cookie *)mCookies.First(); c; c = (Cookie *)c->Next()) {
		if (c->Match(domain,path) && c->Accept()) {
			if (!foundOne) {
				request += "Cookie: ";
				foundOne = true;
				request += c->Name();
			} else {
				request += "; ";
				request += c->Name();
			}
		}
	}
	if (foundOne)
		request += "\r\n";
		
	sLocker.Unlock();
}

const char* Cookie::Name()
{
	return mName.String();
}

const char* Cookie::Domain()
{
	return mDomain.String();
}

bool Cookie::Accept()
{
	return mAccept;
}

const char* Cookie::Path()
{
	const char *path = mPath.String();
	if (path == NULL)
		return "/";
	return path;
}

long Cookie::Expires()
{
	return mExpires;
}

bool Cookie::Secure()
{
	return mSecure;
}


//	Need to see if all this guff is working

#if 0
void Cookie::Test()
{
	pprint("Cookie::Test");
	
	URLParser URL;
	URL.SetURL("http://www.be.com/index.html");
	
	Cookie::Set(URL,BString("type=no_path_or_domain; expires=Wednesday, 09-Nov-99 23:12:40 GMT"));
	Cookie::Set(URL,BString("type=no_domain; path=/foo/; expires=Wednesday, 09-Nov-99 23:12:40 GMT"));
	Cookie::Set(URL,BString("type=path/foo/_and_domain; expires=Wednesday, 09-Nov-99 23:12:40 GMT; path=/foo/; domain=be.com"));
	Cookie::Set(URL,BString("type=domain_and_path/foo/; expires=Wednesday, 09-Nov-99 23:12:40 GMT; domain=be.com; path=/foo/; secure"));
	
	Cookie::Set(URL,BString("applename=applevalue; path=/pub/; domain=apple.com"));
}
#endif
