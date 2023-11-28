#include <storage2/Entry.h>
#include <storage2/Path.h>
#include <support2/Debug.h>
#include <support2/Message.h>
#include <support2/StringBuffer.h>
#include <support2/URL.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace B::Support2;

const type_code kURLType = (type_code) 'turl';

enum field_offset {
	kUser,
	kPassword,
	kHostname,
	kPath,
	kQuery,
	kFragment,
	kUploadQuery
};

enum Scheme {
	kNoScheme, kHTTP, kFile, kFTP, kHTTPS, kGopher, kNews, kNNTP,
	kTelnet, kRLogin, kTN3270, kWAIS, kRTSP, kPNM, kMailTo, kJavaScript,
	kAbout, kBeOS, kMerlin
};

const char *kSchemes[] = {
	"",		// Invalid Scheme
	"http",
	"file",
	"ftp",
	"https",
	"gopher",
	"news",
	"nntp",
	"telnet",
	"rlogin",
	"tn3270",
	"wais",
	"rtsp",
	"pnm",
	"mailto",
	"javascript",
	"about",
	"beos",
	"merlin",
	NULL
};

//
//	The format of fStrings is:
//	[user] '\0' [password] '\0' [hostname] '\0' [path] '\0' [query/javascript command] '\0' [fragment] '\0' [upload query command] '\0'
//

inline uint32 HashString(const char *string, uint32 hash = 0)
{
	while (*string)
		hash = (hash << 7) ^ (hash >> 24) ^ *string++;

	return hash;
}

#warning "I may need to take these out"
// Case insensitive version of HashString.
inline uint32 HashStringI(const char *string, uint32 hash = 0)
{
	while (*string) {
		char c = *string++;
		if (isascii(c))
			c = tolower(c);

		hash = (hash << 7) ^ (hash >> 24) ^ c;
	}
	
	return hash;
}

inline void bindump(char *data, int size)
{
	int lineoffs = 0;
	while (size > 0) {
		printf("\n%04x  ", lineoffs);
		for (int offs = 0; offs < 16; offs++) {
			if (offs < size)
				printf("%02x ", (uchar)data[offs]);
			else
				printf("   ");
		}
			
		printf("     ");
		for (int offs = 0; offs < MIN(size, 16); offs++)
			printf("%c", (data[offs] > 31 && (uchar) data[offs] < 128) ? data[offs] : '.');

		data += 16;
		size -= 16;
		lineoffs += 16;
	}

	printf("\n");
}

inline char *append(char *start, const char *string)
{
	char *end = start;
	while (*string)
		*end++ = *string++;

	return end;
}

inline char *get_nth_string(char *buf, int index)
{
	char *c = buf;
	while (index-- > 0)
		while (*c++ != '\0')
			;
	
	return c;
}

inline char *append_decimal(char *start, int dec)
{
	char buf[11];
	buf[10] = '\0';
	char *c = &buf[10];
	while (dec > 0) {
		*--c = (dec % 10) + '0';
		dec /= 10;
	}

	return append(start, c);
}

// end what i may need to take out

struct B::Support2::URLData {
	ushort fPort;
	Scheme fScheme;
	QueryMethod fQueryMethod;
	char fStrings[0];
};

inline bool IsPathScheme(Scheme scheme)
{
	switch (scheme) {
	case kNoScheme: case kGopher: case kNews: case kNNTP: case kTelnet: case kRLogin:
	case kTN3270: case kWAIS: case kMailTo: case kJavaScript: case kAbout:
		return false;
	default:
		return true;
	}
}

inline int HexToInt(const char *str)
{
    const int kHexDigits[] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
        0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };
    
	return (kHexDigits[static_cast<unsigned char>(str[0])] << 4) |
		kHexDigits[static_cast<unsigned char>(str[1])];
}

// "&" is valid in a query string, and should not be escaped
// there (RFC 1738, sec 2.2).  However, jeff seemed to think
// that "&" needed escaping (implicitly in the path part of a URL),
// and I'm wary of changing that.  RFC 1738 also says that it's
// OK to escape a valid character as long as you _don't_ want
// it to have special meaning (note that "&" does have special
// meaning in query strings, so we must leave it alone there), so
// letting "&"s be escaped in paths should not hurt anything.
// The upshot is separate validMaps and escapeMaps for paths
// and queries.  -- Laz, 2001-05-17.
static const unsigned int kValidInPath[] = {
	0x00000000, 0x4dfffff5, 0xffffffe1, 0x7fffffe2,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};
static const unsigned int kEscapeInPath[] = {
	0xffffffff, 0xbe30000a, 0x0000001e, 0x8000001d,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
};

static const unsigned int kValidInQuery[] = {
	0x00000000, 0x4ffffff5, 0xffffffe1, 0x7fffffe2,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};
static const unsigned int kEscapeInQuery[] = {
	0xffffffff, 0xbc30000a, 0x0000001e, 0x8000001d,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
};

inline bool IsValidURLChar(char c, const unsigned int *validMap)
{
	return (validMap[static_cast<unsigned char>(c) / 32] &
		(0x80000000 >> (static_cast<unsigned char>(c) % 32))) != 0;
}

inline bool NeedsEscape(char c, const unsigned int *escapeMap)
{
	return (escapeMap[static_cast<unsigned char>(c) / 32] &
		(0x80000000 >> (static_cast<unsigned char>(c) % 32))) != 0;
}


inline void UnescapeString(char *out, const char *in, size_t len)
{
	if (len > 0) {
		--len;
		while (*in && len-- > 0) {
			switch (*in) {
				case '%': {
					in++;
					char c = HexToInt(in);
					if (c == '\0')
						c = ' ';
					
					*out++ = c;
					in += 2;
					break;
				}
					
				case '+':
					*out++ = ' ';
					in++;
					break;
					
				default:
					*out++ = *in++;
			}
		}
		*out = '\0';
	}
}

/* static */
int BUrl::GetEscapedLength(const char *c)
{
	int len = 0;
	while (*c) {
		// Note: this sometimes overestimates the length by a few
		// characters to be on the safe size.
		if (NeedsEscape(*c, kEscapeInPath))
			len += 3;
		else
			len++;

		c++;
	}

	return len;
}


/* static */
char *BUrl::EscapePathString(bool escape_all, char *outString, const char *inString, size_t inLen)
{
	return EscapeString(escape_all, outString, inString, inLen, kValidInPath, kEscapeInPath);
}

/* static */
char *BUrl::EscapeQueryString(bool escape_all, char *outString, const char *inString, size_t inLen)
{
	return EscapeString(escape_all, outString, inString, inLen, kValidInQuery, kEscapeInQuery);
}

/* static */
char* BUrl::EscapeString(bool escape_all, char *outString, const char *inString, size_t inLen,
                        const unsigned int *validMap, const unsigned int *escapeMap)
{
	// Avertissement!
	// At one time, this code translated spaces to the '+' character.
	// This is actually wrong, as some servers depend on %20 (like www.terraserver.com).
	// Also, if inString is not escaped yet, escape '+' and '%'.
	const char *kHexDigits = "0123456789abcdef";

	if (escape_all) {
		// Replace invalid URL characters with escapes *and* escape out any valid
		// characters that are delimiters or escape sequences so they won't be confused
		// with the real ones.
		for (const char *c = inString; *c && inLen > 0; c++) {
			if (NeedsEscape(*c, escapeMap)) {
				*outString++ = '%';
				*outString++ = kHexDigits[(static_cast<unsigned char>(*c) >> 4) & 0xf];
				*outString++ = kHexDigits[static_cast<unsigned char>(*c) & 0xf];
			} else
				*outString++ = *c;
	
			inLen--;
		}
	} else {
		// Replace invalid URL characters with escapes.  
		for (const char *c = inString; *c && inLen > 0; c++) {
			if (IsValidURLChar(*c, validMap))
				*outString++ = *c;
			else {
				*outString++ = '%';
				*outString++ = kHexDigits[(static_cast<unsigned char>(*c) >> 4) & 0xf];
				*outString++ = kHexDigits[static_cast<unsigned char>(*c) & 0xf];
			}
	
			inLen--;
		}
	}

	*outString++ = '\0';
	return outString;
}

BUrl::BUrl()
	:	fData(0)
{
}

BUrl::BUrl(const BUrl &url)
	:	fData(0)
{
	SetTo(url);
}

BUrl::BUrl(const char *urlString, bool escape_all)
	:	fData(0)
{
	SetTo(urlString, escape_all);
}

BUrl::BUrl(const char *scheme, const char *hostname, const char *path, int port,
	const char *fragment, const char *user, const char *password, const char *query,
	QueryMethod method, const char *uploadQuery, bool escape_all)
	:	fData(0)
{
	SetTo(scheme, hostname, path, port, fragment, user, password, query, method, uploadQuery, escape_all);
}

BUrl::BUrl(const entry_ref &ref)
	:	fData(0)
{
	SetTo(ref);
}

BUrl::BUrl(const BUrl &baseURL, const char *relativePath, bool escape_all)
	:	fData(0)
{
	SetTo(baseURL, relativePath, escape_all);
}

BUrl::BUrl(const char *name, const BMessage *message)
	:	fData(0)
{
	ExtractFromMessage(name,message);
}

BUrl::BUrl(const BValue &value)
	:	fData(0)
{
	if (value.IsDefined() && value.Type() == kURLType && value.Length() > 0) {
		ssize_t len = value.Length();

		fData = (URLData*) realloc(fData, len);
		memcpy(fData, value.Data(), len);
	}
}

BUrl::~BUrl()
{
	Reset();
}

void BUrl::Reset()
{
	free(fData);
	fData = 0;
}

bool BUrl::IsValid() const
{
	return fData != 0;
}

BUrl& BUrl::operator=(const BUrl &url)
{
	return SetTo(url);
}

BUrl& BUrl::operator=(const char *urlString)
{
	return SetTo(urlString, false);
}

BUrl& BUrl::operator=(const entry_ref &ref)
{
	return SetTo(ref);
}

BUrl& BUrl::SetTo(const char *scheme, const char *hostname, const char *path, int port,
	const char *fragment, const char *username, const char *password, const char *query,
	QueryMethod method, const char *uploadQuery, bool escape_all)
{
	int sizeNeeded = strlen(username) + 1
		+ strlen(password) + 1
		+ strlen(hostname) + 3		// may have to add extra '/'s
		+ GetEscapedLength(path) + 1
		+ GetEscapedLength(query) + 1
		+ strlen(fragment) + 1
		+ GetEscapedLength(uploadQuery) + 1
		+ sizeof(URLData);

	fData = (URLData*) realloc(fData, sizeNeeded);
	fData->fScheme = (Scheme) LookupScheme(scheme);
	if (fData->fScheme == kNoScheme) {
		PRINT(("Bad scheme \"%s\"\n", scheme));
		Reset();
		return *this;
	}

	fData->fPort = port;
	fData->fQueryMethod = method;

	char *c = fData->fStrings;
	c = append(c, username);
	*c++ = '\0';
	c = append(c, password);
	*c++ = '\0';
	c = append(c, hostname);
	*c++ = '\0';

	if (IsPathScheme(fData->fScheme)) {
		if (path[0] != '/')
			*c++ = '/';
	
		c = EscapePathString(escape_all, c, path);
		c = NormalizePath();
		c = EscapeQueryString(escape_all, c, query);
		c = append(c, fragment);
		*c++ = '\0';
	} else if (fData->fScheme == kJavaScript || fData->fScheme == kAbout || fData->fScheme == kMailTo) {
		*c++ = '\0';
		c = append(c, query);
		*c++ = '\0';
		*c++ = '\0';
	} else {
		 *c++ = '\0';
		 *c++ = '\0';
		 *c++ = '\0';
	}

	c = EscapeQueryString(escape_all, c, uploadQuery);
	
	return *this;
}

BUrl& BUrl::SetTo(const char *urlString, bool escape_all)
{
	fData = (URLData*) realloc(fData, sizeof(URLData) + GetEscapedLength(urlString) + 9);
	fData->fQueryMethod = GET_QUERY;
	fData->fPort = 0;

	// Remove leading spaces
	const char *src = urlString;
	while (isspace(*src))
		src++;

	// Parse scheme
	char *schemeEnd = strchr(src, ':');
	if (schemeEnd == 0 || schemeEnd == src) {
		Reset();
		return *this;
	}

	fData->fScheme = (Scheme) LookupScheme(urlString, schemeEnd - urlString);
	if (fData->fScheme == kNoScheme) {
		Reset();
		return *this;
	}

	switch (fData->fScheme) {
	case kMailTo:
		ParseMailToURL(schemeEnd + 1);
		break;
	case kJavaScript:
	case kAbout:
		ParseJavaScriptURL(schemeEnd + 1);
		break;
	default:
		// Treat as a path type URL (http, https, ftp, beos, rtsp, etc...
		ParsePathURL(schemeEnd + 1, escape_all);		
		break;
	}
	return *this;
}

BUrl& BUrl::SetTo(const BUrl &url)
{
	if (&url == this)
		return *this;
		
	if (!url.IsValid()) {
		Reset();
		return *this;
	}

	SetTo(url.GetScheme(), url.GetHostName(), url.GetPath(), url.GetPort(),
		url.GetFragment(), url.GetUserName(), url.GetPassword(), url.GetQuery(),
		url.GetQueryMethod(), url.GetUploadQuery(), false);

#if DEBUG
	if (!Equals(url)) {
		url.PrintToStream();
		PrintToStream();
		debugger("BUrl::SetTo: goofed\n");
	}
#endif

	return *this;
}

BUrl& BUrl::SetTo(const BUrl &baseURL, const char *relativePath, bool escape_all)
{
	// First, scan the beginning of the relative path and determine the basic form:
	//	scheme://  		-- Absolute path.  Ignore base url.
	//	scheme:path		-- relative path
	//	//machine/path 	-- It only inherits the scheme
	const char *s = relativePath;
	enum {
		kScanScheme,
		kScanFirstSlash,
		kScanSecondSlash,
		kScanLeadingSlash,
		kScanDone,
	} state = kScanScheme; 
	while (state != kScanDone && *s != '\0') {
		switch (state) {
		case kScanScheme:
			if (*s == ':')
				state = kScanFirstSlash;
			else if (*s == '/')
				state = kScanLeadingSlash;
			else if (!isalpha(*s))
				state = kScanDone;
			
			break;			

		case kScanFirstSlash:
			if (*s == '/')
				state = kScanSecondSlash;
			else if (s > relativePath + 1) {
				int scheme = LookupScheme(relativePath, s - relativePath - 2);
				if (IsPathScheme((Scheme) scheme))
					return SetToInternal(baseURL, s, escape_all);	// This is of the form scheme:path.
				else
					return SetTo(relativePath, escape_all);	// Something like mailto.  Treat as absolute
			} else {
				Reset();
				return *this;
			}
			
			break;
		
		case kScanSecondSlash:
			if (*s == '/')
				return SetTo(relativePath, escape_all);
					// The "relative" path is probably a fully qualified URL.
					// SetTo that.
			
			// This looks like http:/relative path. Strip off scheme
			return SetToInternal(baseURL, s - 1, escape_all);
		
		case kScanLeadingSlash:
			if (*s == '/') {
				// Special case.  The url is of the form //machine/stuff
				// The scheme is missing, so use the scheme from the base URL
				BStringBuffer tmp;
				tmp << baseURL.GetScheme() << ":" << relativePath;
				printf("%s\n", tmp.String());
				return SetTo(tmp.String(), escape_all);
			}

			state = kScanDone;
			break;
		
		default:
			;
		}

		s++;
	}

	return SetToInternal(baseURL, relativePath, escape_all);
}

// The internal form doesn't check for the different forms, it just
// dumbly appends the relative path
BUrl& BUrl::SetToInternal(const BUrl &baseURL, const char *relativePath, bool escape_all)
{
	int sizeNeeded = strlen(baseURL.GetHostName()) + 1
		+ strlen(baseURL.GetPath()) + 1
		+ GetEscapedLength(relativePath) + 2	// add a slash too
		+ strlen(baseURL.GetUserName()) + 1
		+ strlen(baseURL.GetPassword()) + 1
		+ 2		// query/fragment (not used)
		+ strlen(baseURL.GetUploadQuery()) + 1
		+ sizeof(URLData);

	fData = (URLData*) realloc(fData, sizeNeeded);

	if(baseURL.fData) {
		fData->fScheme = baseURL.fData->fScheme;
		fData->fPort = baseURL.fData->fPort;
	} else {
		fData->fScheme = kNoScheme;
		fData->fPort = 0;
	}

	fData->fQueryMethod = GET_QUERY;	// Default to GET.  This used to use
										// the base URLs method, but that breaks
										// where a page that comes from a POST
										// query has relative links.

	char *c = fData->fStrings;
	c = append(c, baseURL.GetUserName());
	*c++ = '\0';
	c = append(c, baseURL.GetPassword());
	*c++ = '\0';
	c = append(c, baseURL.GetHostName());
	*c++ = '\0';

	// Figure out the unescaped length of the relative path
	int relPathLength = 0;
	for (const char *tmp = relativePath; *tmp != '\0' &&  *tmp != '?' && *tmp != '#'; tmp++)
		relPathLength++;
	
	if (relPathLength > 0) {
		// Append the relative path
		char *pathStart = c;
		if (relativePath[0] == '/') {
			// If the relative path begins with '/', treat it as beginning from root.
			// Just append it onto hostname.
			EscapePathString(escape_all, c, relativePath, relPathLength);
		} else {
			// Relative path.  Append to path of base URL.
			c = append(c, baseURL.GetPath());
			*c = '\0';	// make sure this is terminated before we begin scanning.
	
			// Strip back to last path component
			while (c > pathStart && *c != '/')
				c--;
	
			*c++ = '/';		// looks redundant, but handles case where base path has no '/'
			EscapePathString(escape_all, c, relativePath, relPathLength);
		}

		c = NormalizePath();
	} else {
		c = append(c, baseURL.GetPath());
		*c++ = '\0';
	}

	// Copy the query.
	// xxxjeff Should take base query if there is no relative one.
	const char *src = relativePath + relPathLength;
	
	c = ExtractQueryString(c, &src, escape_all);

	if (*src == '#')
		src++;

	// Copy the fragment.
	// xxxjeff Should take base fragment if there is no relative one.
	while (*src)
		*c++ = *src++;

	*c++ = '\0';
	
	c = append(c, baseURL.GetUploadQuery());
	*c++ = '\0';
		
	return *this;
}

void BUrl::AppendToInternal(BStringBuffer &str, bool minimal) const
{
	if (!fData)
		return;

	str << kSchemes[fData->fScheme] << ':';

	// Javascript
	if (fData->fScheme == kJavaScript || fData->fScheme == kAbout) {
		str << get_nth_string(fData->fStrings, kQuery);
		return;
	}

	bool isPath = IsPathScheme(fData->fScheme);
	if (isPath)
		str << "//";

	const char *in = fData->fStrings;
	if (!minimal && *in) {
		// Append the user
		while (*in)
			str << *in++;

		in++;
		if (*in) {
			str << ':';
			while (*in)
				str << *in++;
		}
		
		in++;
		str << '@';
	} else {
		for (int i = 0; i < 2; i++) {
			while (*in)
				in++;
				
			in++;
		}
	}

	// Hostname
	while (*in)
		str << *in++;
		
	in++;		

	if (fData->fPort != 0)
		str << ':' << (unsigned long) fData->fPort;

	if (isPath) {
		while (*in)
			str << *in++;
	} else {
		while (*in)
			in++;
	}
			
	in++;

	// The query string is hidden for POST queries.
	if (*in) {
		if (GetQueryMethod() == GET_QUERY) {
			str << '?';
			while (*in)
				str << *in++;
		} else {
			while (*in)
				in++;
		}
	}
	
	in++;
	
	if (!minimal && *in) {
		str << '#';
		while (*in)
			str << *in++;
	}
	// upload query strings are not shown here
}

BUrl& BUrl::SetTo(const entry_ref &ref)
{
	BEntry entry(&ref);
	if (entry.InitCheck() != B_OK) {
		Reset();
		return *this;
	}

	BPath path;
	if (entry.GetPath(&path) != B_OK) {
		Reset();
		return *this;
	}

	SetTo("file", "", path.Path(), 0, "", "", "", "", GET_QUERY, "", true);
	return *this;
}

void BUrl::GetString(char *buffer, size_t size) const
{
	BStringBuffer tmp;
	AppendTo(tmp);
	if (tmp.Length() < size)
		memcpy(buffer, tmp.String(), tmp.Length() + 1);
	else {
		memcpy(buffer, tmp.String(), size - 1);
		buffer[size - 1] = '\0';
	}
}

void BUrl::AppendTo(BStringBuffer &str) const
{
	AppendToInternal(str, false);
}

QueryMethod BUrl::GetQueryMethod() const
{
	if (fData)
		return fData->fQueryMethod;
		
	return GET_QUERY;
}

void BUrl::GetUnescapedFileName(char *out, int size) const
{
	const char *fname = 0;
	for (const char *path = GetPath(); *path; path++) {
		if (*path == '/')
			fname = path + 1;
	}

	UnescapeString(out, fname, size);
}

void BUrl::SetFragment(const char *fragment)
{
	if (fData && fData->fScheme == kJavaScript) return;

	// Note: This little dance with a temporary URL is necessary.
	//  We can't just do SetTo(GetScheme(), ...), because that could
	//  clobber the argument strings' memory as we fill *fData with
	//  their data.  -- Hard Experience, 2001-05-18
	BUrl tmp(GetScheme(), GetHostName(), GetPath(), GetPort(),
	        fragment, GetUserName(), GetPassword(),
	        GetQuery(), GetQueryMethod(), GetUploadQuery());
	SetTo(tmp);
}

void BUrl::SetQuery(const char *query, bool escape_all)
{
	if (fData && fData->fScheme == kJavaScript) return;
	
	int query_length = GetEscapedLength(query) + 1;
	BString esc_query;
	char *esc_query_buf = esc_query.LockBuffer(query_length);
	EscapeQueryString(escape_all, esc_query_buf, query);
	BUrl tmp(GetScheme(), GetHostName(), GetPath(), GetPort(),
	        GetFragment(), GetUserName(), GetPassword(),
	        esc_query_buf, GetQueryMethod(), GetUploadQuery());
	esc_query.UnlockBuffer();
	SetTo(tmp);
}

void BUrl::SetUploadQuery(const char *uploadQuery, bool escape_all)
{
	if (fData && fData->fScheme == kJavaScript) return;

	int upload_query_length = GetEscapedLength(uploadQuery) + 1;
	BString esc_upload_query;
	char *esc_upload_query_buf = esc_upload_query.LockBuffer(upload_query_length);
	EscapeQueryString(escape_all, esc_upload_query_buf, uploadQuery);
	BUrl tmp(GetScheme(), GetHostName(), GetPath(), GetPort(),
	        GetFragment(), GetUserName(), GetPassword(),
	        GetQuery(), GetQueryMethod(), esc_upload_query_buf);
	esc_upload_query.UnlockBuffer();
	SetTo(tmp);
}

void BUrl::SetQueryMethod(QueryMethod method, bool compressIfPost)
{
	if (fData == 0)
		fData = (URLData*) calloc(sizeof(URLData),1);

	if (((fData->fScheme == kHTTP) || (fData->fScheme == kHTTPS)) &&
		compressIfPost && (method == POST_QUERY) && GetQuery()[0]) {
		BUrl url(*this);
		char *buffer = (char*)malloc(strlen(url.GetPath()) + strlen(GetQuery()) + 2);
		sprintf(buffer, "%s?%s", url.GetPath(), url.GetQuery());
		SetTo(url.GetScheme(), url.GetHostName(), buffer, url.GetPort(),
			url.GetFragment(), url.GetUserName(), url.GetPassword(), "",
			POST_QUERY, url.GetUploadQuery(), false);
		free(buffer);
	} else
		fData->fQueryMethod = method;
}

const char* BUrl::GetScheme() const
{
	if (fData == 0)
		return "";
		
	return kSchemes[fData->fScheme];
}

const char* BUrl::GetUserName() const
{
	if (fData == 0)
		return "";

	return get_nth_string(fData->fStrings, kUser);
}

const char* BUrl::GetPassword() const
{
	if (fData == 0)
		return "";

	return get_nth_string(fData->fStrings, kPassword);
}

const char* BUrl::GetHostName() const
{
	if (fData == 0)
		return "";

	return get_nth_string(fData->fStrings, kHostname);
}

ushort BUrl::GetPort() const
{
	return fData ? fData->fPort : 0;
}

const char* BUrl::GetPath() const
{
	if (strcasecmp(GetScheme(), "mailto") == 0 || fData == 0)
		return "";

	return get_nth_string(fData->fStrings, kPath);
}

const char* BUrl::GetExtension() const
{
	const char *extension = 0;
	for (const char *path = GetPath(); *path; path++) {
		switch (*path) {
		case '/':
			extension = 0;
			break;
		case '.':
			extension = path + 1;
			break;
		}
	}

	return extension ? extension : "";
}

void BUrl::GetUnescapedPath(char *out, int size) const
{
	UnescapeString(out, GetPath(), size);
}

void BUrl::GetUnescapedQuery(char *out, int size) const
{
	UnescapeString(out, GetQuery(), size);
}

const char* BUrl::GetQuery() const
{
	if (fData == 0)
		return "";

	return get_nth_string(fData->fStrings, kQuery);
}

const char* BUrl::GetUploadQuery() const
{
	if (fData == 0)
		return "";

	return get_nth_string(fData->fStrings, kUploadQuery);
}

const char* BUrl::GetFragment() const
{
	if (fData == 0)
		return "";

	return get_nth_string(fData->fStrings, kFragment);
}

void BUrl::AddQueryParameter(const char *name, const char *value, bool escape_all)
{
	if (!IsValid())
		return;

	// Get the length of the current query portion -- if there
	// is already something here, then we need to also add an '&'
	// to separate our new parameter.
	int origQueryLen = strlen(GetQuery());
	
	// Text for this parameter is the length of the name, its value
	// the '=' divider, and the '&' separator if not the first parameter.
	int paramLength = GetEscapedLength(name) + GetEscapedLength(value)
					+ (origQueryLen > 0 ? 2 : 1);
	
	int newLen =
		strlen(GetUserName()) + 1
		+ strlen(GetPassword()) + 1
		+ strlen(GetHostName()) + 1
		+ strlen(GetPath()) + 1
		+ origQueryLen + 1
		+ strlen(GetFragment()) + 1
		+ strlen(GetUploadQuery()) + 1
		+ paramLength
		+ sizeof(URLData);
	
	fData = (URLData*) realloc(fData, newLen);
	
	char *queryEnd = (char*) GetFragment() - 1;

	// Move the fragment & name portion over to insert new parameter inside.
	memmove(queryEnd + paramLength + 1, queryEnd + 1, strlen(GetFragment()) + 1 + strlen(GetUploadQuery()) + 1);
	
	if (origQueryLen > 0)
		*queryEnd++ = '&';	// Not first parameter, add separator.
		
	queryEnd = EscapeQueryString(escape_all, queryEnd, name) - 1; // -1 because EscapeQueryString() terminates
	*queryEnd++ = '=';
	EscapeQueryString(escape_all, queryEnd, value);
}

void BUrl::AddUploadQueryParameter(const char *name, const char *value, bool escape_all)
{
	if (!IsValid())
		return;

	// Get the length of the current query portion -- if there
	// is already something here, then we need to also add an '&'
	// to separate our new parameter.
	int origQueryLen = strlen(GetUploadQuery());
	
	// Text for this parameter is the length of the name, its value
	// the '=' divider, and the '&' separator if not the first parameter.
	int paramLength = GetEscapedLength(name) + GetEscapedLength(value)
					+ (origQueryLen > 0 ? 2 : 1);
	
	int newLen =
		strlen(GetUserName()) + 1
		+ strlen(GetPassword()) + 1
		+ strlen(GetHostName()) + 1
		+ strlen(GetPath()) + 1
		+ strlen(GetQuery()) + 1
		+ strlen(GetFragment()) + 1
		+ origQueryLen + 1
		+ paramLength + 1
		+ sizeof(URLData);
	
	fData = (URLData*) realloc(fData, newLen);
	
	char *queryEnd = (char*)GetUploadQuery() + origQueryLen;

	// Move the fragment & name portion over to insert new parameter inside.
//	memmove(queryEnd + paramLength + 1, queryEnd + 1, strlen(GetFragment()) + 1 + strlen(GetName()) + 1);
	
	if (origQueryLen > 0)
		*queryEnd++ = '&';	// Not first parameter, add separator.
		
	queryEnd = EscapeQueryString(escape_all, queryEnd, name) - 1; // -1 because EscapeQueryString() terminates
	*queryEnd++ = '=';
	queryEnd = EscapeQueryString(escape_all, queryEnd, value);
	*queryEnd++ = '\0';
}

status_t BUrl::GetQueryParameterInternal(const char *query, const char *name, BString *out_value) const
{
	if( query ) {
		BString buffer;
		while( *query ) {
			const char* pname = query;
			while( *query && *query != '=' && *query != '&' ) query++;
			if( query > pname ) {
				const char* pnameend = query;
				const char* pval = query;
				if( *query == '=' ) {
					pval++;
					query++;
					while( *query && *query != '&' ) query++;
				}
				
				// Pull out this parameter name, and remove any escapes.
				buffer.SetTo(pname, (size_t)(pnameend-pname));
				char* b = buffer.LockBuffer(buffer.Length() + 1);
				UnescapeString(b, b, buffer.Length() + 1);
				buffer.UnlockBuffer();
				// Are query parameters case insensitive? I think so...
				if (buffer.ICompare(name) == 0) {
					// Found it!  Pull out value and return.
					if( query > pval ) {
						out_value->SetTo(pval, (size_t)(query-pval));
						char* b = out_value->LockBuffer(out_value->Length() + 1);
						UnescapeString(b, b, out_value->Length() + 1);
						out_value->UnlockBuffer();
					} else {
						*out_value = "";
					}
					return B_OK;
				}
				
			}
			if( *query ) query++;
		}
	}
	
	*out_value = "";
	return B_NAME_NOT_FOUND;
}

status_t BUrl::GetUploadQueryParameter(const char* name, BString* out_value) const
{
	return GetQueryParameterInternal(GetUploadQuery(), name, out_value);
}

status_t BUrl::GetQueryParameter(const char* name, BString* out_value) const
{
	return GetQueryParameterInternal(GetQuery(), name, out_value);
}

status_t BUrl::RemoveQueryParameter(const char *name)
{
	char *query = (char*) GetQuery();
//	char *query_start = query;
	if(query) {
		BString buffer;
		for(;*query;) {
			char *param_start = query;

			// Search for end of parameter name
			for(; *query && *query!='&' && *query!='='; query++ ) {}

			if(query > param_start) {

				char *param_name_end = query;
				char *param_end = query;
				if(*query == '=') {

					// Search for end of parameter block
					for(query++;*query && *query!='&'; query++) {}

					param_end = query;

					// Include '&' into the parameter block
					if(*query == '&') {
						param_end++;
					}
				}
			
				// Unescape the parameter name for comparison
				char *b = buffer.LockBuffer(param_name_end-param_start+1);
				UnescapeString(b,param_start,param_name_end-param_start+1);
				buffer.UnlockBuffer();
		
				if(buffer == name) {

					// If this is the last parameter
					// also remove the previous '&'
					// Exception: When the URL already contains
					// an '&' at the end it will be kept.
					if(*query==0 && *(param_start-1)=='&') { // param_start-1 should always be safe. 
						param_start --;
					}

					// remove the parameter
					memmove(param_start,param_end,strlen(param_end) + 1 + strlen(GetFragment()) + 1 + strlen(GetUploadQuery()) + 1);

					return B_OK;
				}
			}
			if(*query) query++;
		}
	}
	return B_NAME_NOT_FOUND;
}

status_t BUrl::RemoveUploadQueryParameter(const char *name)
{
	char *query = (char*) GetUploadQuery();
	if(query) {
		BString buffer;
		for(;*query;) {
			char *param_start = query;

			// Search for end of parameter name
			for(; *query && *query!='&' && *query!='='; query++ ) {}

			if(query > param_start) {

				char *param_name_end = query;
				char *param_end = query;
				if(*query == '=') {

					// Search for end of parameter block
					for(query++;*query && *query!='&'; query++) {}

					param_end = query;

					// Include '&' into the parameter block
					if(*query == '&') {
						param_end++;
					}
				}
			
				// Unescape the parameter name for comparison
				char *b = buffer.LockBuffer(param_name_end-param_start+1);
				UnescapeString(b,param_start,param_name_end-param_start+1);
				buffer.UnlockBuffer();
		
				if(buffer == name) {

					// If this is the last parameter
					// also remove the previous '&'
					// Exception: When the URL already contains
					// an '&' at the end it will be kept.
					if(*query==0 && *(param_start-1)=='&') { // param_start-1 should always be safe. 
						param_start --;
					}

					// remove the parameter
					memmove(param_start,param_end,strlen(param_end) + 1);

					return B_OK;
				}
			}
			if(*query) query++;
		}
	}
	return B_NAME_NOT_FOUND;
}

status_t BUrl::ReplaceUploadQueryParameter(const char *name, const char *value, bool addIfNotPresent, bool escape_all)
{
	char *query = (char*) GetUploadQuery();
	if(query) {
		BString buffer;
		for(;*query;) {
			char *param_start = query;

			// Search for end of parameter name
			for(; *query && *query != '&' && *query != '='; query++) {}

			if(query > param_start) {

				char *param_name_end = query;
				char *param_end = query;
				if(*query == '=') {

					// Search for end of parameter block
					for(query++;*query && *query != '&'; query++) {}

					param_end = query;
				}
			
				// Unescape the parameter name for comparison
				char *b = buffer.LockBuffer(param_name_end-param_start+1);
				UnescapeString(b,param_start,param_name_end-param_start+1);
				buffer.UnlockBuffer();

				if(buffer == name) {

					// Prepare and escape replacement parameter block
					int newParameterBlockLen = GetEscapedLength(name) + 1 + GetEscapedLength(value);
					int oldParameterBlockLen = param_end-param_start;

					// Resize fData buffer and move data
					int diffLen = (newParameterBlockLen - oldParameterBlockLen);

					int newLen =
						strlen(GetUserName()) + 1
						+ strlen(GetPassword()) + 1
						+ strlen(GetHostName()) + 1
						+ strlen(GetPath()) + 1
						+ strlen(GetQuery()) + 1
						+ strlen(GetFragment()) + 1
						+ strlen(GetUploadQuery()) + 1
						+ sizeof(URLData)
						+ diffLen;

					if(diffLen > 0)	{
						URLData *oldData = fData;
						fData = (URLData*) realloc(fData, newLen);
						if(fData != oldData) {
							// mess with the param_start, param_end, param_name_end, & query vars.
							// they now point to the old buffer
							int ptrShift = (int)fData - (int)oldData;
							param_start += ptrShift;
							param_end += ptrShift;
						}
						memmove(param_end+diffLen,param_end,strlen(param_end) + 1);
					}
					else if(diffLen < 0)
						memmove(param_end+diffLen,param_end,strlen(param_end) + 1);

					// Write escaped parameter block into gap

					// Save last character since EscapeQueryString() will overwrite it with 0
					char endChar = *(param_end + diffLen);
					char *escapeEnd = EscapeQueryString(escape_all, param_start, name) - 1;
					*escapeEnd++ = '=';
					// Restore last character after escaping
					*(EscapeQueryString(escape_all, escapeEnd, value) - 1) = endChar;

					return B_OK;
				}
			}
			if(*query) query++;
		}
	}

	if(addIfNotPresent) {
		AddQueryParameter(name,value);
		return B_OK;
	}

	return B_NAME_NOT_FOUND;
}

status_t BUrl::ReplaceQueryParameter(const char *name, const char *value, bool addIfNotPresent, bool escape_all)
{
	char *query = (char*) GetQuery();
//	char *query_start = query;
	if(query) {
		BString buffer;
		for(;*query;) {
			char *param_start = query;

			// Search for end of parameter name
			for(; *query && *query!='&' && *query!='='; query++ ) {}

			if(query > param_start) {

				char *param_name_end = query;
				char *param_end = query;
				if(*query == '=') {

					// Search for end of parameter block
					for(query++;*query && *query!='&'; query++) {}

					param_end = query;
				}
			
				// Unescape the parameter name for comparison
				char *b = buffer.LockBuffer(param_name_end-param_start+1);
				UnescapeString(b,param_start,param_name_end-param_start+1);
				buffer.UnlockBuffer();

				if(buffer == name) {

					// Prepare and escape replacement parameter block
					int newParameterBlockLen = GetEscapedLength(name) + 1 + GetEscapedLength(value);
					int oldParameterBlockLen = param_end-param_start;

					// Resize fData buffer and move data
					int diffLen = (newParameterBlockLen - oldParameterBlockLen);

					int newLen =
						strlen(GetUserName()) + 1
						+ strlen(GetPassword()) + 1
						+ strlen(GetHostName()) + 1
						+ strlen(GetPath()) + 1
						+ strlen(GetQuery()) + 1
						+ strlen(GetFragment()) + 1
						+ strlen(GetUploadQuery()) + 1
						+ sizeof(URLData)
						+ diffLen;

					if(diffLen > 0)	{
						URLData *oldData = fData;
						fData = (URLData*) realloc(fData, newLen);
						if(fData != oldData) {
							// mess with the param_start, param_end, param_name_end, & query vars.
							// they now point to the old buffer
							int ptrShift = (int)fData - (int)oldData;
							param_start += ptrShift;
							param_end += ptrShift;
						}
						memmove(param_end+diffLen,param_end,strlen(param_end) + 1 + strlen(GetFragment()) + 1 + strlen(GetUploadQuery()) + 1);
					}
					else if(diffLen < 0)
						memmove(param_end+diffLen,param_end,strlen(param_end) + 1 + strlen(GetFragment()) + 1 + strlen(GetUploadQuery()) + 1);

					// Write escaped parameter block into gap

					// Save last character since EscapeQueryString() will overwrite it with 0
					char endChar = *(param_end+diffLen);
					char *escapeEnd = EscapeQueryString(escape_all, param_start, name) - 1;
					*escapeEnd++ = '=';
					// Restore last character after escaping
					*(EscapeQueryString(escape_all, escapeEnd, value) - 1) = endChar;

					return B_OK;
				}
			}
			if(*query) query++;
		}
	}

	if(addIfNotPresent) {
		AddQueryParameter(name,value);
		return B_OK;
	}

	return B_NAME_NOT_FOUND;
}


int BUrl::LookupScheme(const char *scheme)
{
	for (int i = 0; kSchemes[i] != 0; i++)
		if (strcasecmp(kSchemes[i], scheme) == 0)
			return i;

	return kNoScheme;
}

int BUrl::LookupScheme(const char *scheme, int len)
{
	for (int i = 0; kSchemes[i] != 0; i++)
		if (strncasecmp(kSchemes[i], scheme, len) == 0)
			return i;

	return kNoScheme;
}

status_t BUrl::GetRef(entry_ref &ref) const
{
	char pathBuf[B_PATH_NAME_LENGTH];
	UnescapeString(pathBuf, GetPath(), B_PATH_NAME_LENGTH);
	return get_ref_for_path(pathBuf, &ref);
}

BValue BUrl::AsValue() const
{
	BValue value;
	if (fData == 0) {
		// This is an invalid URL, add a zero length member to indicate this.
		value.Assign(kURLType, 0, 0); 
	} else {
		// Figure out how big the data is
		int len = sizeof(URLData);
		int stringIndex = 7;
		char *c = fData->fStrings;
		while (stringIndex > 0) {
			if (*c++ == '\0')
				stringIndex--;
	
			len++;
		}
	
		value.Assign(kURLType, fData, len); 
	}
	
	return value;
}

void BUrl::AddToMessage(const char *name, BMessage *message) const
{
	message->Data().Overlay(BValue::String(name), *this);
}

void BUrl::ExtractFromMessage(const char *name, const BMessage *message)
{
	BValue value = message->Data().ValueFor(BValue::String(name));

	if (value.IsDefined() && value.Length() > 0) {
		ssize_t len = value.Length();

		fData = (URLData*) realloc(fData, len);
		memcpy(fData, value.Data(), len);
	} else {
		// This was a bad URL
		Reset();
	}	
}

void BUrl::PrintToStream() const
{
	BStringBuffer tmp;
	tmp << *this;
	printf("BUrl \"%s\"\n", tmp.String());
	printf("  scheme: \"%s\"\n", GetScheme());
	printf("  host: \"%s\"\n", GetHostName());
	printf("  path: \"%s\"\n", GetPath());
	printf("  query: \"%s\"\n", GetQuery());
	printf("  user: \"%s\"\n", GetUserName());
	printf("  password: \"%s\"\n", GetPassword());
	printf("  port: %d\n", GetPort());
	printf("  fragment: %s\n", GetFragment());
	printf("  upload query: \"%s\"\n", GetUploadQuery());
	printf("\n");
}

//	Important: Do not consider fragment, user, or password
//	in hash.  The do not change the resource pointed to, and
//	returning different hash values can result in multiple
//	cached copies of the same resource.  The scheme and port are not
//	the considered because they are generally always the same (http:80).
uint BUrl::GenerateHash() const
{
	uint32 hash = 0;	

	// Hash hostname
	// Most of the URLs are going to be www.XXX.com.  Strip off the beginning
	// and end to generate a more unique hash.  Note also the tolower().  As the
	// host is case insensitive, hash the lowercase version of the letters to
	// guarantee different case version generate the same hash.  
	const char *in = GetHostName();
	while (*in)
		if (*in++ == '.')
			break;
		
	while (*in && !in != '.') {
		char c = *in++;
		if (isascii(c))
			c = tolower(c);
			
		hash = (hash << 7) ^ (hash >> 24) ^ c;
	}
	
	// HashString is case sensitive, because the path and query are.
	hash = HashString(GetPath(), hash);
	hash = HashString(GetQuery(), hash);
	return hash;
}

void BUrl::GetMinimalString(BStringBuffer &buf) const
{
	return AppendToInternal(buf, true);
}

// As in GenerateHash, this ignores fragment, user, password, and query method
// The path and query are case sensitive.  The scheme and hostname are not.
bool BUrl::Equals(const BUrl &url) const
{
	if (!IsValid() || !url.IsValid())
		return false;

	return fData->fScheme == url.fData->fScheme
		&& fData->fPort == url.fData->fPort
		&& strcasecmp(GetHostName(), url.GetHostName()) == 0
		&& strcmp(GetPath(), url.GetPath()) == 0
		&& strcmp(GetQuery(), url.GetQuery()) == 0;
}

bool BUrl::Equals(const char *str) const
{
	BUrl tmp(str);
	return Equals(tmp);
}

bool BUrl::operator==(const char *urlString) const
{
	return Equals(urlString);	
}

bool BUrl::operator==(const BUrl &url) const
{
	return Equals(url);
}

bool BUrl::operator!=(const BUrl &url) const
{
	return !Equals(url);
}

void BUrl::ParsePathURL(const char *_src, bool escape_all)
{
	const char *src = _src;
	char *dest = fData->fStrings;
	if (src[0] != '/' || src[1] != '/') {
		Reset();
		return;
	}
	
	// Note that we scan for a slash.  The @ sign might be an unescaped part
	// of a path.
	src += 2;
	const char *atSign = strchr(src, '@');
	const char *firstSlash = strchr(src, '/');
	if (atSign != 0 && (firstSlash == 0 || firstSlash > atSign)) {
		// parse username
		while (*src != ':' && *src != '@')
			*dest++ = *src++;
		
		*dest++ = '\0';	// delimit username
		if (*src == ':') {
			// parse password
			src++;
			while (*src != '@')
				*dest++ = *src++;
		}

		*dest++ = '\0';	// delimit password or lack thereof
		src++;			// skip @
	} else {
		*dest++ = '\0';	// NULL for user name.
		*dest++ = '\0';	// NULL for password
	}
		
	// host name
	while (*src != ':' && *src != '/' && *src != '\0' && *src != '?')
		*dest++ = *src++;	

	*dest++ = '\0';		// delimit host
	
	// port
	if (*src == ':') {
		src++;
		fData->fPort = atoi(src);	
		while (*src != '/' && *src != '\0' && *src != '?')
			src++;
	} else
		fData->fPort = 0;

	// path
	if (*src != '/')
		*dest++ = '/';		// make sure there's at least one slash

	// Escape path
	{
		int path_length = 0;
		const char *src_path_end = src;
		while (*src_path_end != '#' && *src_path_end != '\0' && *src_path_end != '?') {
			src_path_end++;
			path_length++;
		}
		
		EscapePathString(escape_all, dest, src, path_length);
		src += path_length;
	}
	
	dest = NormalizePath();

	dest = ExtractQueryString(dest, &src, escape_all);

	//  fragment
	if (*src == '#') {
		src++;	// skip hash mark	
		while (*src != '\0')
			*dest++ = *src++;
	}
			
	*dest++ = '\0';

	// no upload queries from this string
	*dest++ = '\0';
}

void BUrl::ParseMailToURL(const char *src)
{
	char *dest = fData->fStrings;
	fData->fPort = 0;

	// Parse username
	while (*src && *src != '@')
		*dest++ = *src++;

	*dest++ = '\0';	// terminate username
	*dest++ = '\0';	// terminate password (there is none)

	// Parse hostname
	if (*src == '@') {
		src++;
		while ((*src != '\0') && (*src != '?'))
			*dest++ = *src++;
	}
	
	*dest++ = '\0';	// terminate hostname
	*dest++ = '\0'; // terminate path
	
	// Should the query part of a mailto: URL be escaped?  Other
	// queries are, but I don't know.  -- Laz, 2001-05-25
	if (*src == '?') {
		src++;
		while (*src != '\0')
			*dest++ = *src++;
	}
	*dest++ = '\0'; // terminate query
	*dest++ = '\0'; // terminate fragment
	*dest++ = '\0'; // terminate upload query
}

void BUrl::ParseJavaScriptURL(const char *url)
{
	fData->fPort = 0;
	char *dest = fData->fStrings;
	*dest++ = '\0';	// user
	*dest++ = '\0'; // password
	*dest++ = '\0'; // host
	*dest++ = '\0'; // path
	while (*url)
		*dest++ = *url++;
	
	*dest++ = '\0';	// query (function call)
	*dest++ = '\0'; // fragment
	*dest++ = '\0'; // upload query
}

//
//	Normalize the path, removing '.' and '..' entries.  Notice that this is done
//	in place, as the length of the path will not increase.
//
char* BUrl::NormalizePath()
{
	const char *pathStart = GetPath();	
	const char *in = pathStart;
	char *out = (char*) pathStart;

	while (*in) {
		if (in[0] == '/' && in[1] == '.' && in[2] == '/')
			in += 2;	// remove /./
		else if (in[0] == '/' && in[1] == '.' && in[2] == '.'
			&& (in[3] == '/' || in[3] == '\0')) {
			// remove /..
			if (out > pathStart) {
				// Walk back a path component
				out--;
				while (*out != '/' && out > pathStart)
					out--;
			}

			in += 3;
			if (*out == '.' && out[1] == '\0')
				out++;	// special case to preserve last slash in '/..'
		} else
			*out++ = *in++;
	}

	// If this was an empty path, set it to '/'
	if (out == pathStart)
		*out++ = '/';

	*out++ = '\0';
	return out;
}


/* static */
char *BUrl::ExtractQueryString(char *dest, const char **src, bool escape_all)
{
	int query_length = 0;
	if (**src == '?') {
		++(*src);
		const char *src_query_end = *src;
		while (*src_query_end != '#' && *src_query_end != '\0') {
			src_query_end++;
			query_length++;
		}
	}
	dest = EscapeQueryString(escape_all, dest, *src, query_length);
	*src += query_length;
	return dest;
}


