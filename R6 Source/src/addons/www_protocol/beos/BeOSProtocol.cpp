#include <Entry.h>
#include <File.h>
#include <stdio.h>
#include <sys/stat.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <String.h>
#include <image.h>
#include "URL.h"
#include "Protocol.h"
#include "CookieManager.h"
#include "ContentManager.h"
#include "PasswordManager.h"
#include "WagnerDebug.h"
#include "util.h"

using namespace Wagner;

class BeOSProtocol : public Protocol {
public:
	BeOSProtocol(void* handle);
	virtual status_t Open(const URL &url, const URL&, BMessage* outErrorParams, uint32 flags);
	virtual ssize_t GetContentLength();
	virtual void GetContentType(char *type, int size);
	virtual CachePolicy GetCachePolicy();
	virtual ssize_t Read(void *buffer, size_t size);
	virtual bool GetRedirectURL(URL&, bigtime_t*);
private:
	URL fRedirect;
	BString fBuffer;
	size_t fBufferPos;
	BString fContentType;
};

BeOSProtocol::BeOSProtocol(void* handle)
	: 	Protocol(handle),
		fBufferPos(0)
{
}

status_t BeOSProtocol::Open(const URL &url, const URL&, BMessage* outErrorParams, uint32)
{
	const char *path = url.GetPath();
	if (strcmp(path, "/") == 0 || strcmp(path, "/component") == 0) {
		BString signature;
		if( url.GetQueryParameter("signature", &signature) != B_OK ) {
			outErrorParams->AddString(S_ERROR_TEMPLATE, "Errors/beos.html");
			outErrorParams->AddString("message", "no \"signature\" parameter was supplied");
			return B_NAME_NOT_FOUND;
		}

		fContentType = signature;
	} else if (strcmp(path, "/login") == 0) {
		BString username;
		BString password;
		BString challenge;
		BString requestURL;
		url.GetQueryParameter("username", &username);
		url.GetQueryParameter("password", &password);
		url.GetQueryParameter(S_CHALLENGE_STRING, &challenge);
		url.GetQueryParameter("requested_url", &requestURL);
	
		URL tmp(requestURL.String(), true);
		passwordManager.SetPassword(tmp.GetHostName(), challenge.String(), username.String(),
			password.String());
		fRedirect.SetTo(tmp);
	}
#if ENABLE_LOG
	else if (strcmp(path, "/log") == 0) {
		BString buffer;
		wdebug.ReadLog(&buffer);
		fBuffer = "<HTML><HEAD><TITLE>Log</TITLE></HEAD><BODY>\n<PRE>";
		fBuffer << escape_for_html(&buffer) << "</PRE>\n</BODY></HTML>\n";
	} else if (strcmp(path, "/cookies") == 0) {
		fBuffer = "<HTML>\n"
				  "<HEAD>\n"
				  "<TITLE>Active Cookies</TITLE>\n"
				  "</HEAD><BODY>\n"
				  "<H1>Active Cookies</H1>\n";
		cookieManager.GetUserInfo(&fBuffer);
		fBuffer << "</BODY>\n</HTML>\n";
	} else if (strcmp(path, "/add-ons") == 0) {
		fBuffer = "<HTML>\n"
				  "<HEAD>\n"
				  "<TITLE>Add Ons</TITLE>\n"
				  "</HEAD><BODY>\n"
				  "<H1>Add Ons</H1>\n";
		ContentManager::Default().GetUserInfo(&fBuffer);
		fBuffer << "</BODY>\n</HTML>\n";
	}
#endif

	if (fBuffer.Length() > 0) {
		fBufferPos = 0;
		fContentType = "text/html";
	}
	
	return B_OK;
}

ssize_t BeOSProtocol::GetContentLength()
{
	return fBuffer.Length();
}

void BeOSProtocol::GetContentType(char *type, int size)
{
	strncpy(type, fContentType.String(), size);
}

CachePolicy BeOSProtocol::GetCachePolicy()
{
	return CC_NO_CACHE;
}

ssize_t BeOSProtocol::Read(void *buffer, size_t size)
{
	if (size > fBuffer.Length()-fBufferPos)
		size = fBuffer.Length()-fBufferPos;

	if (size > 0)
		memcpy(buffer, fBuffer.String()+fBufferPos, size);

	fBufferPos += size;
	return size;
}

bool BeOSProtocol::GetRedirectURL(URL &url, bigtime_t *outDelay)
{
	if (fRedirect.IsValid()) {
		url = fRedirect;
		*outDelay = 0;
		return true;
	}
	
	return false;
}

// ----------------------- BeOSProtocolFactory -----------------------

class BeOSProtocolFactory : public ProtocolFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_PROTOCOL_SCHEMES, "beos");
	}
	
	virtual Protocol* CreateProtocol(void* handle, const char* scheme)
	{
		(void)scheme;
		return new BeOSProtocol(handle);
	}
	
	virtual bool KeepLoaded() const
	{
		return false;
	}
};

extern "C" _EXPORT ProtocolFactory* make_nth_protocol(int32 n, image_id /* you */, uint32 /* flags */, ...)
{
	if( n == 0 ) return new BeOSProtocolFactory;
	return 0;
}

