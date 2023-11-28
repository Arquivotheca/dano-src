#ifndef _COOKIE_MANAGER_H
#define _COOKIE_MANAGER_H

#include <stdio.h>
#include <Locker.h>
#include <String.h>
#include <time.h>
#include <posix/limits.h>
#include "Timer.h"
#include "Queue.h"
#include "URL.h"
#include "util.h"
#include "Binder.h"

const uint kCookieHashSize = 389;

class Cookie;

class CookieManager {
public:
	CookieManager();
	~CookieManager();
	
	/*! Parse an HTTP response header's "Set-Cookie:" string, and store the
	    resulting cookie for future use.
	    
	    \param fromURL The URL of the page that sent you this "Set-Cookie:".
	    Used to make sure that a page at evil.org doesn't try to create or
	    change cookies for innocent.com .
	    
	    \param cookieString The "Set-Cookie:" string, without the "Set-Cookie:".
	    For example, "session-id=102-6304643-4649769; expires=Wed, 09-May-2001 23:59:59 GMT".
	    
	    \param serverdate The time according to the server, if you know it.  Used
	    to get expiration times right when the server and the local machine disagree
	    about what time it is.
	*/
	void ParseCookieString(const Wagner::URL &fromURL, const char *cookieString, time_t serverdate = -1);

	/*! Build a semicolon-space delimited string of "name=value" pairs
	    appropriate for use in an HTTP request header.
	    	    
	    \param forURL The URL to build a cookie string for.  \c BuildCookieString()
	    pays attention only to the URL's hostname and path.
	    
	    \param outString A StringBuffer to put the resulting cookie string into.
	    An example of an output string from \c BuildCookieString() would be
	    "registered=no; mesp_dw=h%3DY; UID=7D1A8791E6402624".
	*/
	void BuildCookieString(const Wagner::URL &forURL, StringBuffer &outString);
	void BuildCookieString(const Wagner::URL &forURL, char *outString, size_t size);

	void SetCookie(const Wagner::URL &src, const char *name, const char *value,
		const char *domain = "", const char *path = "", time_t expiration = -1);
	void GetCookieValue(const Wagner::URL &requestingResource, const char *name,
		char *outValue, size_t valueSize);
		
	void DumpCookies();
	
#if ENABLE_LOG
	void GetUserInfo(BString* into);
#endif
	
private:
	status_t AddCookie(Cookie *cookie);
	void DeleteCookie(Cookie *cookie);
	void ReadCookies();
	void WriteCookieToBinder(const Cookie *cookie);
	void ClearCookies();
	void Listen();
	void Unlisten();
	static unsigned HashDomain(const char *host);
	static void MakeCookiePropertyName(const Cookie *cookie, BString *name);
	void ReadCookieFile();
	status_t ReloadCookies();
	
	BLocker fLock;
	BinderNode::observer_token fUserObserverToken;
	BinderNode::property       fUserNode;
	BinderNode::observer_token fCookieObserverToken;
	BinderNode::property       fCookieNode;
	Queue fLRU;
	int32 fNumCookies;
	Cookie *fDomainHash[kCookieHashSize];
	Cookie *fNameHash[kCookieHashSize];
	bool fListening;
	bool fCookiesRead;
	
	static status_t ReloadCookiesHook(void *userData, uint32 observed, void *extraData);
};

extern CookieManager cookieManager;

#endif
