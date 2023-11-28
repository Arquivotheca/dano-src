#include <Entry.h>
#include <Path.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <Debug.h>
#include "URL.h"

URL::URL()
	:	fIsValid(false),
		fPort(0)
{
}

URL::URL(const URL &url)
{
	SetTo(url);
}

URL::URL(const char *urlString)
{
	SetTo(urlString);
}

URL::URL(const char *scheme, const char *hostname, const char *path, int port)
{
	SetTo(scheme, hostname, path, port);
}

URL::URL(const entry_ref &ref)
{
	SetTo(ref);
}

URL& URL::operator=(const URL &url)
{
	return SetTo(url);
}

URL& URL::operator=(const char *urlString)
{
	return SetTo(urlString);
}

URL& URL::operator=(const entry_ref &ref)
{
	return SetTo(ref);
}

URL& URL::SetTo(const char *scheme, const char *hostname, const char *path, int port,
	const char *username, const char *password)
{
	fScheme = scheme;
	fHostName = hostname;
	fPath = path;
	fPort = port;
	fUserName = username;
	fPassword = password;
	fIsValid = true;
	BuildURLString();	
	return *this;
}

URL& URL::SetTo(const char *urlString)
{
	fURLString = "";
	fIsValid = false;
	fScheme = "";
	fHostName = "";
	fPath = "";
	fPort = 0;
	fPassword = "";
	fUserName = "";
	
	// Parse scheme
	const char *c = urlString;
	char *schemeEnd = strstr(c, "://");
	if (schemeEnd == 0)
		fScheme = "http";	// default to HTTP
	else {
		fScheme.SetTo(urlString, schemeEnd - urlString);
		c = schemeEnd + 3;
	}
	
	if (*c == 0)
		return *this;		

	// Parse user/password
	char *passwordEnd = strchr(c, '@');
	if (passwordEnd != 0 && fScheme != "file") {
		// This has a user name.
		char *usernameEnd = strchr(c, ':');
		if (usernameEnd != 0 && usernameEnd < passwordEnd) {
			// This also has a password
			fPassword.SetTo(usernameEnd + 1, passwordEnd - (usernameEnd + 1));
			fUserName.SetTo(c, usernameEnd - c);
		} else {
			fPassword = "";
			fUserName.SetTo(c, passwordEnd - c);
		}

		c = passwordEnd + 1;
	}

	if (*c == 0)
		return *this;

	// Parse hostname
	char *hostNameEnd = strchr(c, ':');
	char *hostSlash = strchr(c, '/');
	if (hostNameEnd == 0 || (hostSlash != 0 && hostSlash < hostNameEnd))
		hostNameEnd = hostSlash;
		
	if (hostNameEnd == 0) {
		fIsValid = true;
		fPath = "/";
		fHostName.SetTo(c);
		BuildURLString();
		return *this;
	}
		
	fHostName.SetTo(c, hostNameEnd - c);
	c = hostNameEnd;

	// Parse port id	
	if (*c == ':') {
		c++;
		fPort = 0;
		while (isdigit(*c)) {
			fPort = fPort * 10 + *c - '0';
			c++;
		}
	}

	// Parse filepath, unescaping
	fEscapedPath = c;
	UnescapeString(&fPath, c);

	fIsValid = true;
	BuildURLString();
	return *this;
}

URL& URL::SetTo(const URL &url)
{
	fIsValid = url.fIsValid;
	fScheme = url.fScheme;
	fHostName = url.fHostName;
	fEscapedPath = url.fEscapedPath;
	fPath = url.fPath;
	fPort = url.fPort;
	fURLString = url.fURLString;
	fUserName = url.fUserName;
	fPassword = url.fPassword;
	return *this;
}

URL& URL::SetTo(const entry_ref &ref)
{
	fScheme = "file";
	fHostName = "";
	fPort = 0;
	fUserName = "";
	fPassword = "";
	fIsValid = false;
	
	BEntry entry(&ref);
	if (entry.InitCheck() != B_OK)
		return *this;

	BPath path;
	if (entry.GetPath(&path) != B_OK)
		return *this;
		
	fPath = path.Path();
	EscapeString(&fEscapedPath, fPath.String());
	fIsValid = true;

	BuildURLString();

	return *this;
}

void URL::BuildURLString()
{
	fURLString << fScheme << "://";
	if (fUserName != "") {
		fURLString << fUserName;
		if (fPassword != "")
			fURLString << ":" << fPassword;
	
		fURLString << "@";
	}
	
	fURLString << fHostName;
	if (fPort != 0)
		fURLString << ":" << (long) fPort;		

	EscapeString(&fEscapedPath, fPath.String());
	fURLString << fEscapedPath;
	
}

int URL::GetPort() const
{
	return fPort;
}

const char* URL::GetPath() const
{
	return fPath.String();
}

const char* URL::GetEscapedPath() const
{
	return fEscapedPath.String();
}

const char* URL::GetFileName() const
{
	const char *filename = "";
	if (fPath.Length() > 2) {
		for (filename = fPath.String() + fPath.Length() - 2; filename > fPath.String();
			filename--) {
			if (*filename == '/') {
				filename++;
				break;
			}
		}
	}
			
	return filename;
}

const char* URL::GetHostName() const
{
	return fHostName.String();
}

const char* URL::GetScheme() const
{
	return fScheme.String();
}

const char* URL::GetURLString() const
{
	return fURLString.String();
}

const char* URL::GetUserName() const
{
	return fUserName.String();
}

const char* URL::GetPassword() const
{
	return fPassword.String();
}

status_t URL::GetRef(entry_ref &ref) const
{
	return get_ref_for_path(GetPath(), &ref);
}

int URL::HexToInt(const char *str)
{
	int val = 0;
	for (int i = 0; i < 2; i++) {
		unsigned char c = str[i];
		if (c >= '0' && c <= '9')
			val = (val << 4) | (c - '0');
		else if (c >= 'a' && c <= 'f')
			val = (val << 4) | (c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			val = (val << 4) | (c - 'A' + 10);
		else
			break;
	}

	return val;
}

bool URL::IsValidURLChar(char c)
{
	const char *kValidCharacters = ":@&=+$-_.!*'(),/?~";
	return isalpha(c) || isdigit(c) || strchr(kValidCharacters, c) != 0;
}

void URL::EscapeString(BString *outEscaped, const char *in)
{
	// Find expanded length
	size_t escapedLength = 0;
	for (char *c = (char*) in; *c; c++) {
		if (!IsValidURLChar(*c))
			escapedLength += 3;
		else
			escapedLength++;
	}

	ASSERT(escapedLength >= strlen(in));

	// Copy.
	char *buf = outEscaped->LockBuffer(escapedLength + 1);
	for (char *c = (char*) in; *c; c++) {
		if (!IsValidURLChar(*c)) {
			sprintf(buf, "%%%02x", *(uchar*) c);
			buf += 3;
		} else
			*buf++ = *c;
	}

	*buf = '\0';
	outEscaped->UnlockBuffer();
	ASSERT(escapedLength == (size_t) outEscaped->Length());
}

void URL::UnescapeString(BString *outUnescaped, const char *in)
{
	char *out = outUnescaped->LockBuffer(strlen(in) + 1);
	while (*in) {	
		if (*in == '%') {
			in++;
			char c = HexToInt(in);
			if (c == '\0')
				c = ' ';
			
			*out++ = c;
			in += 2;
		} else
			*out++ = *in++;
	}
	
	*out = '\0';
	outUnescaped->UnlockBuffer();
}

bool URL::IsValid() const
{
	return fIsValid;
}

void URL::PrintToStream() const
{
	printf("url: \"%s\"\t(%s)\n", fURLString.String(), fIsValid ? "url is valid"
		: "url is not valid");
	printf("\tscheme: \"%s\"\n", fScheme.String());
	printf("\thost: \"%s\"\n", fHostName.String());
	printf("\tpath: \"%s\"\n", fPath.String());
	printf("\tfilename: \"%s\"\n", GetFileName());
	if (GetUserName() != "")
		printf("\tuser: \"%s\"\n", GetUserName());
		
	if (GetPassword() != "")
		printf("\tpassword: \"%s\"\n", GetPassword());
	
	if (GetPort() != 0)
		printf("\tport: %d\n", GetPort());
}

#ifdef _TEST_

int main()
{
	URL url;
	url.PrintToStream();
		
	URL url2("http://www.be.com");
	url2.PrintToStream();
	
	URL url3("www.askjeeves.com");
	url3.PrintToStream();
	
	URL url4("ftp.microsoft.com/ftp%20archives/windows%20source/WINSRC1.ZIP");
	url4.PrintToStream();
	
	URL url5("rtsp://www.nude_squirrels.com:6000");
	url5.PrintToStream();
	
	URL url6("http://www.shoutcast.com:7000/somecgi-script");
	url6.PrintToStream();
	
	URL url8("http", "www.irs.gov", "/fun stuff/taxforms", 666);
	url8.PrintToStream();

	URL url9 = url8;
	url9.PrintToStream();
	
	URL url10("red.be.com:24/public/movies/avi/BABENEW.AVI");
	url10.PrintToStream();
	
	URL url11("www.be.com:999");
	url11.PrintToStream();
	
	URL url12("ftp://jeff:secret@ftp.be.com/somefile.tgz");
	url12.PrintToStream();
	
	URL url13("ftp://jeff@ftp.be.com/somefile.tgz");
	url13.PrintToStream();

	URL url14("ftp://jeff:secret@ftp.be.com:2000/somefile.tgz");
	url14.PrintToStream();
	
	URL url15("ftp://jeff@ftp.be.com:2000/somefile.tgz");
	url15.PrintToStream();

	URL url16("file:///Source/media/quicktime/beavisandbutthead/girl3.mov");
	url16.PrintToStream();
	
	entry_ref ref;
	get_ref_for_path("/Source/media/quicktime/beavisandbutthead/girl3.mov", &ref);
	URL url17(&ref);
	url17.PrintToStream();
}


#endif
