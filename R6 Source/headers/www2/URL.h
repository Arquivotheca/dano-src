#ifndef _URL_H
#define _URL_H

#include <kernel/OS.h>
#include <storage2/Entry.h>
#include <support2/String.h>
#include <support2/Value.h>
#include <www2/BArray.h>

using namespace B::Support2;
using namespace B::Storage2;

class StringBuffer;


struct URLData;

namespace Wagner {

enum QueryMethod {
	GET_QUERY,
	POST_QUERY
};

class URL {
public:
	URL();
	URL(const URL&);
	/*! \param escape_all Set to \c true if you want to escape any "%"s and "+"s in the
	    \c path, \c query, and \c uploadQuery strings.  Set to \c false if those strings
	    might already contain %-escapes that you don't want escaped a second time.  Any
	    other characters that are unsafe in URLs are %-escaped in either case.
	*/
	URL(const char *scheme, const char *hostname, const char *path, int port,
		const char *fragment, const char *username = "", const char *password = "",
		const char *query = "", QueryMethod method = GET_QUERY, const char *uploadQuery = "",
		bool escape_all = false);
	URL(const char *urlString, bool escape_all = false);
	URL(const URL& baseURL, const char *relativePath, bool escape_all = false);
	URL(const char *name, const BMessage *message);
	URL(const BValue &value);
	URL(const entry_ref&);
	~URL();
	
	void Reset();
	bool IsValid() const;

	BValue AsValue() const;
	inline operator BValue() const { return AsValue(); }
	
	/*! \note Equals() does *not* consider the fragment, username, password, or query method.
	    The resource cache depends on this so it doesn't store multiple
	    copies of the same resource.
	*/
	bool Equals(const char*) const;
	bool Equals(const URL& url) const;
	bool operator==(const URL& url) const;
	bool operator==(const char *urlString) const;
	bool operator!=(const URL& url) const;

	//! This assignment operator is equivalent to URL::SetTo(url_string, false).
	URL& operator=(const char *url_string);
	URL& operator=(const URL& url);
	URL& operator=(const entry_ref&);

	URL& SetTo(const URL& url);
	URL& SetTo(const char *scheme, const char *hostname, const char *path, int port,
		const char *fragment, const char *username = "", const char *password = "",
		const char *query = "", QueryMethod method = GET_QUERY, const char *uploadQuery = "",
		bool escape_all = false);
	URL& SetTo(const char *urlString, bool escape_all = false);
	URL& SetTo(const URL& baseURL, const char *relativePath, bool escape_all = false);
	URL& SetTo(const entry_ref&);

	const char* GetScheme() const;
	const char* GetUserName() const;
	const char* GetPassword() const;
	const char* GetHostName() const;
	ushort GetPort() const;
	const char* GetPath() const;
	const char* GetExtension() const;
	const char* GetQuery() const;
	const char* GetUploadQuery() const;
	const char* GetFragment() const;
	QueryMethod GetQueryMethod() const;

	/*! GetUnescapedFileName(), GetUnescapedPath(), GetUnescapedQuery():
	    Extract the filename, path, or query, converting any %-escapes
	    and "+"s appropriately.
	    
	    \param out A buffer that you have allocated for the
	    null-terminated result.
	    
	    \param size The size of the buffer that you allocated. Note that
	    this is the full size of the buffer, including the room for the
	    null terminator.
	*/
	void GetUnescapedFileName(char *out, int size) const;
	void GetUnescapedPath(char *out, int size) const;
	void GetUnescapedQuery(char *out, int size) const;
	
	void SetFragment(const char *fragment);

	//! Set this URL's query to \c query.
	void SetQuery(const char *query, bool escape_all = false);
	void SetQueryMethod(QueryMethod method, bool compressIfPost = false);
	void AddQueryParameter(const char *name, const char *value, bool escape_all = false);
	status_t GetQueryParameter(const char* name, BString* out_value) const;
	status_t RemoveQueryParameter(const char* name);
	status_t ReplaceQueryParameter(const char* name, const char* value,
	                               bool addIfNotPresent = true, bool escape_all = false);

	void SetUploadQuery(const char *uploadQuery, bool escape_all = false);
	void AddUploadQueryParameter(const char *name, const char *value, bool escape_all = false);
	status_t GetUploadQueryParameter(const char* name, BString* out_value) const;
	status_t RemoveUploadQueryParameter(const char* name);
	status_t ReplaceUploadQueryParameter(const char* name, const char* value,
	                                     bool addIfNotPresent=true, bool escape_all = false);

	void GetString(char *buffer, size_t maxSize) const;
	void AppendTo(StringBuffer&) const;
	status_t GetRef(entry_ref &outRef) const;

	void AddToMessage(const char *name, BMessage *message) const;
	void ExtractFromMessage(const char *name, const BMessage *message);

	uint GenerateHash() const;

	void GetMinimalString(StringBuffer&) const;

	void PrintToStream() const;

	/*! EscapePathString() and EscapeQueryString():
	    
	    Convert inString to a legal URL string by %-escaping any characters that
	    need it, and put the null-terminated result in outString.
	    
	    EscapePathString() will escape "&"s.  EscapeQueryString() will not.
	    The RFC implies that escaping "&"s in paths should be harmless,
	    though of dubious utility, but Jeff thought that doing so was important
	    enough to merit a checkin comment, so that's what we do. In query
	    strings, however, "&"s must not be escaped, as they have a special
	    reserved meaning there. (Ref. for all of this: RFC 1738, sec. 2.2.)

	    \param escape_all Set to \c true if you want to escape any "%"s and "+"s
	    in \c inString.  Set to \c false if that string might already contain
		%-escapes that you don't want escaped a second time.  Any other
		characters that are unsafe in URLs are %-escaped in either case.
	    
	    \param outString Pointer to a buffer where you want the resulting
	    escaped string. You must have previously allocated this buffer to
	    be at least GetEscapedLength() + 1 bytes long.
	    
	    \param inString The string that needs escaping.

	    \param inLen The length of inString that you want to escape.  Useful
	    if you want to escape a substring of inString, rather than everything
	    up to the null terminator.

	    \returns A pointer to one past the null terminator of the resulting
	    escaped string in outString.
	*/

	static char *EscapePathString(bool escape_all, char *outString, const char *inString,
	                              size_t inLen = 0x7fffffff);
	static char *EscapeQueryString(bool escape_all, char *outString, const char *inString,
	                               size_t inLen = 0x7fffffff);

	/*! Tells how long the string in \c unescaped_string will be after it
	    has been %-escaped.
	*/
	int GetEscapedLength(const char *unescaped_string);

private:
	URL& SetToInternal(const URL &base, const char *relativePath, bool escape_all = false);
	void AppendToInternal(StringBuffer&, bool minimal) const;
	int LookupScheme(const char *scheme);
	int LookupScheme(const char *scheme, int len);
	char* NormalizePath();
	void ParsePathURL(const char *url, bool escape_all);
	void ParseMailToURL(const char *url);
	void ParseJavaScriptURL(const char *url);
	status_t GetQueryParameterInternal(const char *query, const char *name, BString *out_value) const;
	static char* EscapeString(bool escaped, char *outString, const char *inString, size_t inLen,
	                          const unsigned int *validMap, const unsigned int *escapeMap);

	/*! Extract our internal representation of a query string (escaped, stripped
	    of "?" and "#", and null-terminated) from src into dest.  Typical usage
	    is: dest = ExtractQueryString(dest, &src);
	    
	    \param dest Pointer to the buffer where you want the escaped, null-terminated
	    query string.
	    
	    \param src *src is a pointer to the source query string. It must be
	    unescaped. It may be followed by a "#" (fragment). If it does not
	    begin with "?", then ExtractQueryString() assumes there is no query,
	    and merely puts a null terminator into dest. *src is incremented to
	    one past the end of the source query (so it points to either the "#"
	    or the null terminator).
	    
	    \returns A pointer to one past the null terminator of the query string
	    in dest.
	*/
	static char *ExtractQueryString(char *dest, const char **src, bool escape_all);
	
	URLData *fData;
};

}

#endif
