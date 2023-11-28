#include <Autolock.h>
#include <stdlib.h>
#include <string.h>
#include <Debug.h>
#include <ctype.h>
#include <FindDirectory.h>
#include <unistd.h>
#include <sys/stat.h>
#include <parsedate.h>
#include <time.h>
#include <StopWatch.h>
#include "URL.h"
#include "StringBuffer.h"
#include "CookieManager.h"
#include "WagnerDebug.h"
#include "util.h"
#include "parameters.h"

class Cookie : public QueueEntry {
public:
	Cookie(const char *name, const char *value, const char *domain, const char *path, time_t);
	~Cookie();
	Cookie& SetTo(const char *name, const char *value, const char *domain, const char *path, time_t);
	inline const char *GetDomain() const;
	inline const char *GetName() const;
	inline const char *GetValue() const;
	inline const char *GetPath() const;
	inline time_t GetExpires() const;
	inline const char *GetExpireString() const;
	bool IsHostWithinCookieDomain(const char *hostName) const;
	inline bool MatchesPath(const char *path);
	inline bool SessionOnly() const;
	inline bool IsSameCookie(const char *name, const char *path, const char *domain) const;

private:
	Cookie *fDomainHashNext;
	Cookie *fNameHashNext;
	bool fSessionOnly;
	time_t fExpires;
	const char *fDomain;
	const char *fPath;
	char *fName;			// fName is the start of the allocated block.
	const char *fValue;
	Cookie **fNameHashPrev;
	Cookie **fDomainHashPrev;

	friend class CookieManager;
};

CookieManager cookieManager;

Cookie::Cookie(const char *name, const char *value, const char *domain, const char
	*path, time_t expires)
	:	fSessionOnly(false),
		fExpires(-1),
		fDomain(0),
		fPath(0),
		fName(0),
		fValue(0)
{
	SetTo(name, value, domain, path, expires);
}

Cookie::~Cookie()
{
	free(fName);
}

Cookie& Cookie::SetTo(const char *name, const char *value, const char *domain,
	const char *path, time_t expires)
{
	int sizeNeeded = strlen(name) + 1
		+ strlen(value) + 1
		+ strlen(domain) + 1
		+ strlen(path) + 1;

	// We allocate one buffer to store all of the strings
	fExpires = expires;
	fSessionOnly = (expires == -1);
	fName = (char*) realloc(fName, sizeNeeded);
	char *c = fName;
	c = append(c, name);
	*c++ = '\0';
	fValue = c;
	c = append(c, value);
	*c++ = '\0';
	fDomain = c;
	if (*domain == '.')
		domain++;	// never store leading dot in a domain

	c = append(c, domain);
	*c++ = '\0';
	fPath = c;
	c = append(c, path);
	*c++ = '\0';
	return *this;
}

inline const char* Cookie::GetName() const
{
	return fName;
}

inline const char* Cookie::GetValue() const
{
	return fValue;
}

inline const char* Cookie::GetDomain() const
{
	return fDomain;
}

inline const char* Cookie::GetPath() const
{
	return fPath;
}

inline time_t Cookie::GetExpires() const
{
	return fExpires;
}

inline const char *Cookie::GetExpireString() const
{
	return ctime(&fExpires);
}

/*
	Cookie::IsHostWithinCookieDomain(const char *hostname)

	Examples:
	
	hostname          Cookie's domain         IsHostWithinCookieDomain()?
	---------------------------------------------------------------------
	"foo.be.com"      "be.com"                true
	"Foo.Be.com"      "be.com"                true
	"foo.be.com"      "Be.com"                true
	"xbe.com"         "be.com"                false
	"zyxwbe.com"      "be.com"                false
	"be.com"          "foo.be.com"            false
	"be.com"          "foo.xbe.com"           false
	""                "foo.be.com"            false
	"be.com"          ""                      false
	""                ""                      true
*/

bool Cookie::IsHostWithinCookieDomain(const char *hostname) const
{
	const char *cookieDomain = GetDomain();

	// Work backwards from the ends of this Cookie's domain and the test hostname:
	int c_index = strlen(cookieDomain) - 1;
	int h_index = strlen(hostname) - 1;

	// Do this until we run off the beginning of the cookieDomain string:	
	while (c_index >= 0) {

		if (h_index < 0)
			return false; // We've run out of hostname string, but we still have
			              // cookieDomain string left.  No match.
		
		// Domain names are not case sensitive:
		if (tolower(cookieDomain[c_index]) != tolower(hostname[h_index]))
			return false;
		
		--c_index;
		--h_index;
	}

	// h_index < 0 would mean that hostname and cookieDomain are the same
	//  length and that we got through the above while() loop without finding
	//  a mismatch, so we should return true.
	// Otherwise, check to see if, after the final decrement above,
	//  hostname[h_index] is a '.'.  If it is, then we ran out of cookieDomain
	//  string, and hostname matched cookieDomain for as far as cookieDomain
	//  went, so this is a match.  If hostname[h_index] is not a '.', then the
	//  last part of hostname matched cookieDomain, but hostname still has
	//  other letters, so it doesn't match; e.g., hostname = "foo.be.com"
	//  matches cookieDomain = "be.com", but hostname = "foo.xbe.com" doesn't.

	if (h_index >= 0 && hostname[h_index] != '.')
		return false;

#if 0
	// If the host prefix contains dots, reject.
	// Ex. request host 'y.x.co.uk', and the match domain is 'co.uk', it
	// should be rejected.  However, this breaks yahoo mail.
	// Netscape actually disables this check for .com, .net, etc.
	// This eventually needs to do that, but it will require some testing.
	while (h_index > 0) {
		--h_index;
		if (hostname[h_index] == '.')
			return false;
	}
#endif

	return true;	
}


// MatchesPath():
//
// Note that <http://www.netscape.com/newsref/std/cookie_spec.html>
// states that a cookie with path "/foo" should be sent to a requesting
// URL with path "/foobar".  This strncmp() allows that.

inline bool Cookie::MatchesPath(const char *matchPath)
{
	return strncmp(GetPath(), matchPath, strlen(GetPath())) == 0;
}

inline bool Cookie::SessionOnly() const
{
	return fSessionOnly;
}

inline bool Cookie::IsSameCookie(const char *name, const char *path, const char *domain) const
{
	return (strcmp(fName, name) == 0
	        && strcmp(fPath, path) == 0
	        && strcasecmp(fDomain, domain) == 0);
}

/* static */
status_t CookieManager::ReloadCookiesHook(void *userData, uint32, void *)
{
	CookieManager *manager = (CookieManager *)userData;

	return manager->ReloadCookies();
}

status_t CookieManager::ReloadCookies()
{
	ClearCookies();
	
	Unlisten();
	fCookieNode = fUserNode ["~"] ["cookies"];
	ReadCookies();

	return B_OK;
}

CookieManager::CookieManager()
	:	fLock("CookieManager BLocker"),
		fNumCookies(0),
		fListening(false),
		fCookiesRead(false)
{
	ClearCookies();

	fUserNode = BinderNode::Root() ["user"];
	fCookieNode = fUserNode ["~"] ["cookies"];
}

CookieManager::~CookieManager()
{
	Unlisten();
}

void CookieManager::Listen()
{
	if (!fListening) {
		fUserObserverToken = fUserNode->AddObserverCallback(this, ReloadCookiesHook, B_PROPERTY_CHANGED, "~");
		fCookieObserverToken = fCookieNode->AddObserverCallback(this, ReloadCookiesHook, B_SOMETHING_CHANGED, NULL);
		fListening = true;
	}
}

void CookieManager::Unlisten()
{
	if (fListening) {
		fUserNode->RemoveObserverCallback(fUserObserverToken);
		fCookieNode->RemoveObserverCallback(fCookieObserverToken);
		fListening = false;
	}
}

void CookieManager::ClearCookies()
{
	BAutolock _lock(&fLock);
	// Clear old list
	memset(fNameHash, 0, kCookieHashSize * sizeof(fNameHash[0]));
	memset(fDomainHash, 0, kCookieHashSize * sizeof(fDomainHash[0]));
	for (QueueEntry *entry = fLRU.Dequeue(); entry; entry = fLRU.Dequeue())
		delete entry;
	fNumCookies = 0;
}

void CookieManager::ParseCookieString(const Wagner::URL &src, const char *string, time_t serverdate)
{
	// Values we want
	StringBuffer cookieName(4096);
	StringBuffer cookieValue(8192);
	StringBuffer cookiePath(4096);
	StringBuffer cookieDomain(255);
	time_t cookieExpiration = -1;

	// Scan the string
	enum {
		kScanWhitespace,
		kScanName,
		kScanValue,
		kScanDone
	} state = kScanWhitespace;

	bool first = true;;
	StringBuffer nameBuffer;
	StringBuffer valueBuffer;

	const char *c = string;
	while (state != kScanDone) {
		switch (state) {
			case kScanWhitespace:
				if (*c == '\0')
					state = kScanDone;
				else if (!isspace(*c)) {
					state = kScanName;
					nameBuffer.Clear();
				} else
					c++;
	
				break;
				
			case kScanName:
				if (*c == '\0')
					state = kScanDone;
				else if (*c == '=') {
					c++;
					state = kScanValue;
					valueBuffer.Clear();
				} else if (!isspace(*c))
					nameBuffer.Append(*c++);
				else
					c++;	// skip character
	
				break;
				
			case kScanValue:
				// note: only expires field can have spaces in it.  Space delimits
				// other fields.
				if (*c == ';' || *c == '\0'
					|| (isspace(*c) && strcasecmp(nameBuffer.String(), "expires") != 0)) {

					if (*c == '\0')
						state = kScanDone;
					else
						c++;

					if (first) {
						cookieName = nameBuffer.String();
						cookieValue = valueBuffer.String();
					} else if (strcasecmp(nameBuffer.String(), "expires") == 0) {

						// If we have a server given date, fix this date to use
						// our local date and time. This fixes the problem with
						// wrong client or server time settings.

						cookieExpiration = parsedate(valueBuffer.String(), serverdate);

						if(serverdate != -1)
							cookieExpiration = time(NULL) + (cookieExpiration - serverdate);

					}
					else if (strcasecmp(nameBuffer.String(), "domain") == 0)
						cookieDomain = valueBuffer.String();
					else if (strcasecmp(nameBuffer.String(), "path") == 0)
						cookiePath = valueBuffer.String();
					else if (strcasecmp(nameBuffer.String(), "max-age") == 0)
						cookieExpiration = time(NULL) + atoi(valueBuffer.String());
					else
						PRINT(("unknown cookie parameter \"%s\"\n", nameBuffer.String()));
					
					first = false;
					state = kScanWhitespace;
				} else
					valueBuffer.Append(*c++);
				
				break;
			
			case kScanDone:
				break;
		}
	}
	SetCookie(src, cookieName.String(), cookieValue.String(), cookieDomain.String(), cookiePath.String(), cookieExpiration);
}

void  CookieManager::SetCookie(const Wagner::URL &fromPage, const char *name, const char *value,
	const char *domain, const char *path, time_t expires)
{
	if (strlen(domain) == 0) {
		domain = fromPage.GetHostName();
	}
	
	if (domain[0] == '.')
		domain++;	// Strip leading dots in a domain.
	
	if (strlen(path) == 0) {
		path = "/";
	}

	BAutolock _lock(&fLock);
	if (!fCookiesRead) {
		ReadCookies();
	}

	// See if this cookie already exists.  The test against
	// fromPage.GetHostName() prevents you from modifying an existing
	// cookie for one domain from some other domain.
	Cookie **nameBucket = &fNameHash[HashString(name) % kCookieHashSize];
	Cookie *cookie;
	for (cookie = *nameBucket; cookie; cookie = cookie->fNameHashNext)
		if (cookie->IsSameCookie(name, path, domain)
			&& cookie->IsHostWithinCookieDomain(fromPage.GetHostName()))
			break;

	if (cookie == 0) {
		cookie = new Cookie(name, value, domain, path, expires);
		if (!cookie->IsHostWithinCookieDomain(fromPage.GetHostName())) {
			PRINT(("Bad cookie: domain %s doesn't match host domain %s\n",
				cookie->GetDomain(), fromPage.GetHostName()));
			delete cookie;
			return;		
		}

#if 0
		// This criteria, while seeming correct by RFC standards, rejects
		// cookies that IE and NetPositive accept.  I've disabled the check
		// for now, as it breaks these sites.
		if (!cookie->MatchesPath(fromPage.GetPath())) {
			PRINT(("Bad cookie: path %s doesn't match cookie path %s\n",
				fromPage.GetPath(), cookie->GetPath()));
			delete cookie;
			return;
		}
#endif

		if (AddCookie(cookie) >= B_OK) {
			WriteCookieToBinder(cookie);
		}
		else {
			delete cookie;
			cookie = NULL;
		}
		
#if ENABLE_LOG
		{
			// XXX FIX: Use ctime_r() when available.
			char expires_str[MAX_TIMESTR];
			if (expires >= 0)
				strcpy(expires_str, ctime(&expires));
			else
				strcpy(expires_str, "(session only)\n");
	
			WLOG("Accept cookie: name=%s; value=%s; domain=%s; path=%s; expires=%s",
					name, value, domain, path, expires_str);
		}
#endif		

	} else {
		// Already have this cookie, update its value and move to the
		// back of the LRU.
		if (expires != cookie->GetExpires() ||
		    strcmp(value, cookie->GetValue()) != 0)
		{
			cookie->SetTo(name, value, domain, path, expires);
			WriteCookieToBinder(cookie);
		}
		
		fLRU.RemoveEntry(cookie);
		fLRU.Enqueue(cookie);
	}
}

void CookieManager::GetCookieValue(const Wagner::URL &requestingURL, const char *name,
	char *outValue, size_t valueSize)
{
	BAutolock _lock(&fLock);
	if (!fCookiesRead) {
		ReadCookies();
	}

	for (Cookie *cookie = fNameHash[HashString(name) % kCookieHashSize]; cookie;
		cookie = cookie->fNameHashNext) {
		if (strcmp(name, cookie->GetName()) == 0
			&& cookie->MatchesPath(requestingURL.GetPath())
			&& cookie->IsHostWithinCookieDomain(requestingURL.GetHostName())) {
			strncpy(outValue, cookie->GetValue(), valueSize);
			outValue[valueSize - 1] = '\0';	// terminate of strncpy didn't (if truncated)
			return;
		}
	}

	// Not registered	
	if (valueSize > 0)
		strcpy(outValue, "");
}

status_t CookieManager::AddCookie(Cookie *cookie)
{
	Cookie **nameBucket = &fNameHash[HashString(cookie->GetName()) % kCookieHashSize];

	// Guard against duplicate cookies:
	for (Cookie *old_cookie = *nameBucket; old_cookie; old_cookie = old_cookie->fNameHashNext)
	{
		if (old_cookie->IsSameCookie(cookie->GetName(), cookie->GetPath(), cookie->GetDomain()))
		{
			// Use the one whose expiration is later:
			if (old_cookie->GetExpires() >= cookie->GetExpires()) {
				return B_ERROR;
			}
			else {
				DeleteCookie(old_cookie);
				break;
			}
		}
	}
	
	cookie->fNameHashPrev = nameBucket;
	cookie->fNameHashNext = *nameBucket;
	*nameBucket = cookie;
	if (cookie->fNameHashNext != 0)
		cookie->fNameHashNext->fNameHashPrev = &cookie->fNameHashNext;

	Cookie **domainBucket = &fDomainHash[HashDomain(cookie->GetDomain()) % kCookieHashSize];
	cookie->fDomainHashPrev = domainBucket;
	cookie->fDomainHashNext = *domainBucket;
	*domainBucket = cookie;
	if (cookie->fDomainHashNext != 0)
		cookie->fDomainHashNext->fDomainHashPrev = &cookie->fDomainHashNext;

	fLRU.Enqueue(cookie);
	
	++fNumCookies;
	if (fNumCookies > kMaxCookies) {
		Cookie *head_cookie = dynamic_cast<Cookie*>(fLRU.Head());
		if (!head_cookie) {
			PRINT(("Corrupt Cookie list!  Aborting CookieManager::AddCookie()."));
			return B_ERROR;
		}
		BString domain = head_cookie->GetDomain();
		PRINT(("Too many cookies: deleting cookies for domain %s\n", domain.String()));
		for (Cookie *del = fDomainHash[HashDomain(domain.String()) % kCookieHashSize]; del;) {
			PRINT(("whack cookie %s=%s\n", del->GetName(), del->GetValue()));
			Cookie *tmp = del;
			del = del->fDomainHashNext;
			if (tmp->IsHostWithinCookieDomain(domain.String()))
				DeleteCookie(tmp);
		}
	}
	
	return B_OK;
}

void CookieManager::BuildCookieString(const Wagner::URL &url, char *out, size_t len)
{
	StringBuffer str;
	BuildCookieString(url, str);
	strncpy(out, str.String(), len);
	out[len - 1] = '\0';
}

unsigned CookieManager::HashDomain(const char *hostname)
{
	// Count back two dots, but strip leading dot
	const char *c = hostname + strlen(hostname) - 1;
	int dots = 2;
	for (;c != hostname; c--)
		if (*c == '.')
			if (--dots == 0)
				break;
	
	if (*c == '.')
		c++;
	
	// Domain names are not case sensitive, so hash tolower()ed name:
	StringBuffer lchostname;
	while (*c) {
		lchostname.Append((char)tolower(*c));
		++c;
	}
	
	return HashString(lchostname.String());
}

void CookieManager::BuildCookieString(const Wagner::URL &url, StringBuffer &str)
{
	// Walk through the cookies in this chain and try to match path/domain
	time_t now = time(NULL);
	BAutolock _lock(&fLock);

	if (!fCookiesRead) {
		ReadCookies();
	}

	// XXX Fix: We should send the cookies back in order of the specificity of their
	//          paths, according to <http://www.netscape.com/newsref/std/cookie_spec.html>.
	bool needSeparator = false;
	for (Cookie *cookie = fDomainHash[HashDomain(url.GetHostName()) % kCookieHashSize];
         cookie;
	     /* Would be "cookie = cookie->fDomainHashNext", but sometimes we delete cookie. */)
	{
		if (cookie->SessionOnly() || now < cookie->GetExpires()) {
			if (cookie->IsHostWithinCookieDomain(url.GetHostName()) && cookie->MatchesPath(url.GetPath())) {
				if (needSeparator)
					str << "; ";
				else
					needSeparator = true;

				str << cookie->GetName() << '=' << cookie->GetValue();

				// Move the cookie to the end of the LRU queue, since we just
				// used it:
				fLRU.RemoveEntry(cookie);
				fLRU.Enqueue(cookie);
			}
			cookie = cookie->fDomainHashNext;
		} else {
			// Oops, this cookie is expired!
			Cookie *tmp = cookie;
#if ENABLE_LOG
			// XXX FIX: Use ctime_r() when available.
			char expires_str[MAX_TIMESTR];
			time_t expires = tmp->GetExpires();
			if (expires >= 0)
				strcpy(expires_str, ctime(&expires));
			else
				strcpy(expires_str, "(session only)\n");

			WLOG("Expiring cookie: name=%s; value=%s; domain=%s; path=%s; expires=%s",
					tmp->GetName(), tmp->GetValue(), tmp->GetDomain(), tmp->GetPath(), expires_str);
#endif
			cookie = cookie->fDomainHashNext;
			DeleteCookie(tmp);
		}
	}
}

void CookieManager::DeleteCookie(Cookie *cookie)
{
	// Remove from the name hash table
	*cookie->fNameHashPrev = cookie->fNameHashNext;
	if (cookie->fNameHashNext)
		cookie->fNameHashNext->fNameHashPrev = cookie->fNameHashPrev;

	// Remove from the domain hash table
	*cookie->fDomainHashPrev = cookie->fDomainHashNext;
	if (cookie->fDomainHashNext)
		cookie->fDomainHashNext->fDomainHashPrev = cookie->fDomainHashPrev;

	// Remove from the LRU
	fLRU.RemoveEntry(cookie);
	fNumCookies--;

	BString cookie_property_name;
	MakeCookiePropertyName(cookie, &cookie_property_name);
	
	// Remove the callback so we don't get told about our own actions:
	Unlisten();
		fCookieNode[cookie_property_name.String()] = BinderNode::property::undefined;
	Listen();
	
	delete cookie;
}

void CookieManager::ReadCookies()
{
	BAutolock _lock(&fLock);
		
	Listen();
	
	time_t now = time(NULL);

	BinderNode::iterator cookie_iter = fCookieNode->Properties();
	for (BString property_name = cookie_iter.Next();
	     property_name != "";
	     property_name = cookie_iter.Next())
	{
		BinderNode::property binder_cookie;
		if (fCookieNode->GetProperty(property_name.String(), binder_cookie) == B_OK)
		{
			if (binder_cookie.IsObject() && binder_cookie->IsValid())
			{
				BString name = binder_cookie["cookie_name"].String();
				BString value = binder_cookie["value"].String();
				BString domain = binder_cookie["domain"].String();
				BString path = binder_cookie["path"].String();
				BString expires_str = binder_cookie["expires"].String();
				time_t expires = parsedate(expires_str.String(), -1);

				if (path.Length() == 0) {
					path.SetTo("/");
				}

				if (name.Length() == 0 || value.Length() == 0 || domain.Length() == 0) {
					PRINT(("Bad cookie entry: name = \"%s\", value = \"%s\", domain = \"%s\".\n",
					       name.String(), value.String(), domain.String()));
					continue;
				}
				
				if (expires == -1 || now < expires) {
					Cookie *cookie = new Cookie(name.String(), value.String(), domain.String(),
					                            path.String(), expires);
					if (AddCookie(cookie) <= B_ERROR) {
						delete cookie;
						cookie = NULL;
					}
				}
#if DEBUG
				else {
					// XXX FIX: Use ctime_r() when available.
					PRINT(("Skipped loading of expired cookie %s %s %s %d: %s",
					       name.String(), value.String(), domain.String(), expires, ctime(&expires)));
				}
#endif // DEBUG
			}
		}
	}
	
	ReadCookieFile();
	
	fCookiesRead = true;
}

void CookieManager::ReadCookieFile()
{
	// Get current user
	BString current_user = BinderNode::Root() ["user"] ["~"] ["name"];

	// Get the path to the cookie file.
	char cookie_file_path[PATH_MAX];
	if (find_directory(B_USER_SETTINGS_DIRECTORY, 0, true, cookie_file_path, 1024) != B_OK) {
		PRINT(("find_directory() for user config directory failed.\n"));
		strcpy(cookie_file_path, "/boot/home/find_directory_failed_so_cookie_file_is_here.txt");
		return;
	}
	if (strlen(cookie_file_path) + 18 + current_user.Length() > PATH_MAX) {
		return;
	}

	strcat(cookie_file_path, "/web/");
	strcat(cookie_file_path, current_user.String());
	strcat(cookie_file_path, "_cookies.txt");

	BAutolock _lock(&fLock);
		
	FILE *cookie_file = fopen(cookie_file_path, "r");
	if (cookie_file == 0) {
		perror("Couldn't open cookie file for reading.");
		return;
	}

	char string[1024];
	char *token[5];
	time_t now = time(NULL);
	while (fgets(string, 1024, cookie_file) != 0) {
		enum {
			kScanWhitespace,
			kScanText
		} st = kScanWhitespace;
		int tindex = 0;
		for (char *c = string; *c && tindex < 5; c++) {
			switch (st) {
			case kScanWhitespace:
				if (!isspace(*c)) {
					token[tindex] = c;	// mark beginning of string
					++tindex;
					st = kScanText;
					continue;
				}
				
				break;

			case kScanText:
				if (isspace(*c)) {
					*c = '\0';				// delimit the string
					st = kScanWhitespace;
				}

				break;
			}
		}

		if (tindex == 0)
			break;		// EOF
			
		if (tindex != 4) {
			PRINT(("Bad cookie file entry \"%s\".\n", string));
			continue;
		}
	
		time_t expiration = atoi(token[3]);
		if (expiration == -1 || now < expiration) {
			char *equal = strchr(token[0], '=');
			if (!equal) {
				PRINT(("Bad cookie file entry (no '=')\n"));
				continue;			
			}
			
			*equal = '\0';
			
			Cookie *cookie = new Cookie(token[0], equal + 1, token[1], token[2], expiration);
			if (AddCookie(cookie) >= B_OK) {
				WriteCookieToBinder(cookie);
			}
			else {
				delete cookie;
				cookie = NULL;
			}
		} else
			// XXX FIX: Use ctime_r() when available.
			PRINT(("Skipped loading of expired cookie %s %s %s %d: %s",
				token[0], token[1], token[2], expiration, ctime(&expiration)));
	}
	
	fclose(cookie_file);
	remove(cookie_file_path); // Cookie file is obsolete.
}

/* static */
void CookieManager::MakeCookiePropertyName(const Cookie *cookie, BString *name)
{
	*name  = cookie->GetDomain();
	*name += ',';
	*name += cookie->GetName();
	*name += ',';
	*name += cookie->GetPath();
	name->ReplaceAll('/', '%');
}

void CookieManager::WriteCookieToBinder(const Cookie *cookie)
{
	BAutolock _lock(&fLock);
		
	time_t now = time(NULL);
	time_t expires = cookie->GetExpires();

	if (!cookie->SessionOnly() && now < expires) {
		BString cookie_property_name;
		MakeCookiePropertyName(cookie, &cookie_property_name);
		
		// Add cookie:
		BinderNode::property cookie_prop;
		fCookieNode->GetProperty("+cookie", cookie_prop);

		// Remove the callback so we don't get told about our own actions:
		Unlisten();
			if (cookie_prop.IsObject())
			{
				fCookieNode[cookie_property_name.String()] = cookie_prop;
	
				cookie_prop["cookie_name"] = cookie->GetName();
				cookie_prop["value"] = cookie->GetValue();
				cookie_prop["domain"] = cookie->GetDomain();
				cookie_prop["path"] = cookie->GetPath();
				{
					char exp_str[64];
					struct tm exp_tm;
		
					gmtime_r(&expires, &exp_tm); 
					strftime(exp_str, sizeof(exp_str) - 1, "%Y-%m-%d %H:%M:%S UTC", &exp_tm);
		 			exp_str[sizeof(exp_str) - 1] = '\0';
					cookie_prop["expires"] = exp_str;
		 		}
			}
		Listen();
	}
#if DEBUG
	else {
		if (cookie->SessionOnly())
			PRINT(("Skipping writing session only cookie %s %s %s %s\n",
				cookie->GetName(), cookie->GetValue(), cookie->GetDomain(),
				cookie->GetPath()));	
		else {
			const char *expires_str=cookie->GetExpireString();
			PRINT(("Skipping writing expired cookie %s %s %s %s %d: %s",
				cookie->GetName(), cookie->GetValue(), cookie->GetDomain(),
				cookie->GetPath(), cookie->GetExpires(), expires_str));
		}
	}
#endif
}

void CookieManager::DumpCookies()
{
	BAutolock _lock(&fLock);
	printf("Cookies:\n");
	for (uint i = 0; i < kCookieHashSize; i++)
		for (Cookie *cookie = fNameHash[i]; cookie; cookie = cookie->fNameHashNext) {
			printf("  %s=%s; domain=%s; path=%s ", cookie->GetName(), cookie->GetValue(),
				cookie->GetDomain(), cookie->GetPath());
			if (cookie->SessionOnly()) {
				printf("(session only)\n");
			}
			else {
				printf("expires %s", cookie->GetExpireString());
			}
		}
}

#if ENABLE_LOG

void CookieManager::GetUserInfo(BString* into)
{
	BAutolock _lock(&fLock);
	*into << "<TABLE BORDER=1>\n"
			 "<TR><TH>Name <TH>Value <TH>Domain <TH>Path <TH>Expiration</TR>\n";
	BString name, value, domain, path, expire;
	for (uint i = 0; i < kCookieHashSize; i++)
		for (Cookie *cookie = fNameHash[i]; cookie; cookie = cookie->fNameHashNext) {
			if (cookie->SessionOnly()) {
				expire = "(session only)\n";
			}
			else {
				char expires_str[MAX_TIMESTR];
				cookie->GetExpireString(expires_str);
				escape_for_html(&expire, expires_str, false);
			}
			*into << "<TR>\n"
				  << "<TD>" << escape_for_html(&name, cookie->GetName(), false)
				  << " <TD>" << escape_for_html(&value, cookie->GetValue(), false)
				  << " <TD>" << escape_for_html(&domain, cookie->GetDomain(), false)
				  << " <TD>" << escape_for_html(&path, cookie->GetPath(), false)
				  << " <TD>" << expire)
				  << "</TR>\n";
		}
	
	*into << "</TABLE>\n";
}

#endif
