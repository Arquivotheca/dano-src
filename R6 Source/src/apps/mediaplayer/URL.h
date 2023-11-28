#ifndef _URL_H
#define _URL_H

#include <String.h>

struct entry_ref;

class URL {
public:
	URL();
	URL(const URL&);
	URL(const char *urlString);
	URL(const char *scheme, const char *hostname, const char *path, int port);
	URL(const entry_ref&);
	
	URL& operator=(const URL& url);
	URL& operator=(const char*);
	URL& operator=(const entry_ref&);

	URL& SetTo(const char *scheme, const char *hostname, const char *path, int port,
			const char *username = "", const char *password = "");
	URL& SetTo(const char *urlString);
	URL& SetTo(const URL& url);
	URL& SetTo(const entry_ref&);

	bool IsValid() const;
	
	int GetPort() const;
	const char* GetPath() const;
	const char* GetEscapedPath() const;
	const char* GetFileName() const;
	const char* GetHostName() const;
	const char* GetScheme() const;
	const char* GetURLString() const;
	const char* GetUserName() const;
	const char* GetPassword() const;
	status_t GetRef(entry_ref&) const;

	void PrintToStream() const;

private:

	int HexToInt(const char *c);
	bool IsValidURLChar(char c);
	void EscapeString(BString *outEscaped, const char *in);
	void UnescapeString(BString *outUnescaped, const char *in);
	void BuildURLString();

	bool fIsValid;	
	BString fScheme;
	BString fHostName;
	BString fPath;
	BString fEscapedPath;
	int fPort;
	BString fURLString;		
	BString fUserName;
	BString fPassword;
};

#endif
