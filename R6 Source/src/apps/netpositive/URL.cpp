// ===========================================================================
//	URL.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "URL.h"
#include "Utils.h"
#include "Strings.h"
#include "Protocols.h"
#include "NPApp.h"
#include "MessageWindow.h"
#include "NetPositive.h"

#include <malloc.h>
#include <Entry.h>
#include <Path.h>
#include <Roster.h>
#include <ctype.h>
#include <stdio.h>

//=======================================================================
//===========================================================================
// Add http:// or www.xxx.com to typed address if required...

#ifdef R4_COMPATIBLE
const char		*B_URL_HTTP 				= "application/x-vnd.Be.URL.http";
const char		*B_URL_HTTPS 				= "application/x-vnd.Be.URL.https";
const char		*B_URL_FTP					= "application/x-vnd.Be.URL.ftp";
const char		*B_URL_GOPHER 				= "application/x-vnd.Be.URL.gopher";
const char		*B_URL_MAILTO 				= "application/x-vnd.Be.URL.mailto";
const char		*B_URL_NEWS					= "application/x-vnd.Be.URL.news";
const char		*B_URL_NNTP					= "application/x-vnd.Be.URL.nntp";
const char		*B_URL_TELNET 				= "application/x-vnd.Be.URL.telnet";
const char		*B_URL_RLOGIN 				= "application/x-vnd.Be.URL.rlogin";
const char		*B_URL_TN3270 				= "application/x-vnd.Be.URL.tn3270";
const char		*B_URL_WAIS					= "application/x-vnd.Be.URL.wais";
const char		*B_URL_FILE					= "application/x-vnd.Be.URL.file";
#endif

bool ValidateURL(BString& url)
{
	if (url.Length() == 0)
		return false;
		
	BString schemeStr;
	URLScheme scheme = URLParser::ParseScheme(url.String(), schemeStr);
	if (strstr(url.String(), "://") == NULL && (scheme == kNOSCHEME || scheme == kUNKNOWNSCHEME)) {
		BString guessStr;
		if (strncmp(url.String(),"ftp.",4) == 0) {
			guessStr = "ftp://";	// add ftp if it is missing....
			guessStr += url;
			scheme = kFTP;
		} else {
			scheme = kHTTP;
			guessStr = "http://";	// add http if it is missing....
			CropLeadingSpace(url);
			CropTrailingSpace(url);
			if (url.Length() == 0)
				return false;
			if (strchr(url.String(),'.') == NULL &&
				strchr(url.String(),'/') == NULL &&
				(!gPreferences.FindBool("LookupLocalHostnames") || DNSLooper::GetHostByName(url.String()) == 0)) {
				int32 colonPos = url.FindFirst(':');
				BString urlExtra;
				if (colonPos > 0) {
					urlExtra = &url[colonPos];
					url.Truncate(colonPos);
				}
				guessStr += "www.";
				guessStr += url;
				guessStr += ".com";
				guessStr += urlExtra;
			} else
				guessStr += url;
		}
		url = guessStr;
	} else if (strncmp(url.String(), "http:", 5) == 0 && strncmp(url.String(), "http://", 7) != 0) {
		BString temp = url.String() + 5;
		url = "http://";
		url += temp;
	}
	
	if (scheme == kFTP) {
		// Follow symlinks inside of FTP URL's.
		const char *urlString = url.String();
		const char *linkLocation = strstr(urlString, " -> ");
		if (linkLocation != 0) {
			BString partialURL = (linkLocation + 4);
			BString baseURL = url;
			baseURL.Truncate(linkLocation - urlString);
			URLParser parser;
			parser.SetURL(baseURL.String());
			URLParser convertedParser;
			convertedParser.BuildURL(partialURL, parser);
			convertedParser.WriteURL(url);
		}
	}
	
	if (scheme == kHTTP) {
		// If the address is in the form http://www.blah.com, then add a trailing slash
		URLParser parser;
		parser.SetURL(url.String());
		if (strlen(parser.Path()) == 0)
			url += "/";
	}
	
	return true;
}

//===========================================================================
//	RFC 1808

//	URL			=	(absoluteURL | relativeURL) ["#" fragment]
//	absoluteURL	=	generic-RL | (scheme ":" *(uchar | reserved))
//	generic-RL	=	scheme ":" relativeURL
//	relativeURL	=	net_path | abs_path | rel_path

//	net_path	=	"//" net_loc [abs_path]
//	abs_path	=	"/" rel_path
//	rel_path	=	[path] [";" params] ["?" query]

//	path		= 	fsegment *("/" segment)
//	fsegment	=	1*pchar
//	segment		=	*pchar

URLParser::URLParser() : mScheme(kNOSCHEME), mBasePathKnown(false), mHostPort(-1)
{
}

void URLParser::Reset()
{
	mScheme = kNOSCHEME;
	mSchemeStr = "";
	mNetLocation = "";
	mPath = "";
	mParams = "";
	mQuery = "";
	mFragment = "";
	
	mUser = "";
	mPassword = "";
	
	mBasePath = "";
	mBasePathKnown = false;
	
	mHostName = "";
	mHostPort = -1;
}

//	Used when guessing a host from a incomplete host name

void URLParser::SetHostName(const char* host)
{
	mHostName = host;
	mNetLocation = mHostName;	// Port?
}

//	Lookup scheme

URLScheme URLParser::ParseScheme(const char* url, BString& schemeStr)
{
	URLScheme scheme = kNOSCHEME;
	schemeStr = "";
	
//	NP_ASSERT(url);
	const char *s = strchr(url,':');		// Host starts after scheme
	if (s != NULL) {
		s++;
		schemeStr.SetTo(url,s - url);
		schemeStr.ToLower();
		const char* sch = schemeStr.String();
		
		if ((strcmp(sch,"http:") == 0) || (strcmp(sch, "https:") == 0))
		 	if (*s == '/' && *(s + 1) == '/')
				scheme = (*(s - 2) == 's') ? kHTTPS: kHTTP;
			else {
				// Some sites use broken http:/dir1/dir2/foo.html links.  Treat these as if they have no scheme so
				// that a proper base path will get used for them.
				scheme = kNOSCHEME;
				schemeStr.SetTo(url, s - url + 1);
			}
		else if (strcmp(sch,"file:") == 0)
			scheme = kFILE;
		else if (strcmp(sch,"ftp:") == 0)
			scheme = kFTP;
		else if (strcmp(sch,"news:") == 0)
			scheme = kNEWS;
		else if (strcmp(sch,"nntp:") == 0)
			scheme = kNNTP;
		else if (strcmp(sch,"mailto:") == 0)
			scheme = kMAILTO;
		else if (strcmp(sch,"telnet:") == 0)
			scheme = kTELNET;
		else if (strcmp(sch,"gopher:") == 0)
			scheme = kGOPHER;
		else if (strcmp(sch,"javascript:") == 0)
			scheme = kJAVASCRIPT;
		else if (strcmp(sch,"rlogin:") == 0)
			scheme = kRLOGIN;
		else if (strcmp(sch,"tn3270:") == 0)
			scheme = kTN3270;
		else if (strcmp(sch,"wais:") == 0)
			scheme = kWAIS;
			
		else if (strcmp(sch,kInternalURLPrefix) == 0)
			scheme = kNETPOSITIVE;

		else {
			
//			URL has a : but does not have a recognizable scheme....
//			Check to see if the 'scheme' is a reasonable length
//			vaild schemes can have alpha, digits, '+' '-' '@' ':' and '.'

			if (s - url > 16) {
				schemeStr = "";
				s = NULL;
			} else {
				const char* u;
				for (u = url; u < s-1; u++) {
					if (!(isalnum(u[0]) || strchr("+-.@:",u[0]))) {
						s = url;
						break;
					}
				}
			
//				Looks like this is a scheme we don't know about

				if (s != url) {
					pprint("URLParser: Unknown Scheme - %s",schemeStr.String());
					scheme = kUNKNOWNSCHEME;
				} else
					schemeStr = "";
			}
		}
	}	
	return scheme;
}

//	Interpret a net location into hostnames, passwords, ports etc

void URLParser::ParseNetLocation()
{
	if (mNetLocation.Length() == 0)
		return;
	const char* net = mNetLocation.String();
		
	switch (mScheme) {
		case kHTTP:	mHostPort = 80;	break;
		case kHTTPS: mHostPort = 443; break;
		case kFTP:	mHostPort = 21;	break;
		case kFILE:	mHostPort = 21;	break;	// file becomes ftp at some point
		//case kNNTP;
		//case kMAILTO;
		//case kTELNET;
		//case kGOPHER;
		default: mHostPort = 0; break;
	}
	
//	Check for password

	char *s = strchr(net,'@');
	if (s) {
		mUser.SetTo(net,s - net);	// <user>:<password>@<host>:<port>
		net = s + 1;
		if (mUser.Length() > 0) {
			s = strchr(mUser.String(),':');
			if (s) {
				s[0] = 0;
				mPassword = s + 1;
			}
		}
	}
	
//	Check for port
	
	s = strchr(net,':');
	if (s) {	
		mHostName.SetTo(net,s - net);	// Find a custom port, if any
		sscanf(s+1,"%d",&mHostPort);
	} else
		mHostName = net;
	
}

//	Parse the url into its components

void URLParser::SetURL(const char* constURL)
{
//	Clip leading spaces
//	Clip trailing spaces

	if (constURL == NULL)
		return;	

	while (isspace(constURL[0]))
		constURL++;
	BString scratchURL(constURL);
	CropTrailingSpace(scratchURL);
	
	const char *url = scratchURL.String();
	char *p;

	if (scratchURL.Length() == 0)
		return;
	
	Reset();
	
//	2.4.1	Parsing the fragment indentifier

	p = strchr(url,'#');			// Strip off fragments
	if (p) {
		p[0] = 0;
		mFragment = p+1;
	}
	
//	2.4.2	Parsing the scheme

	mScheme = ParseScheme(url,mSchemeStr);
	
//	2.4.3	Parsing the Network Location/Login

	const char *s = url + mSchemeStr.Length();
	if (s[0] == '/' && s[1] == '/') {		// Network/Location Login
		s += 2;
		const char* net_loc = s;
		while (s[0] && s[0] != '/')			// Up to next slash is the net_loc
			s++;
		mNetLocation.SetTo(net_loc,s - net_loc);
		ParseNetLocation();
	}
	
//	2.4.4	Parsing the Query information

	p = strchr(url,'?');
	if (p) {
		p[0] = '\0';
		mQuery = p+1;
	}
	
//	2.4.5	Parsing the Paramaters

	p = strchr(url,';');
	if (p) {
		p[0] = '\0';
		mParams = p+1;
	}

//	2.4.6	Parsing the path

	mPath = s;
}

//	BuildURL from base and embedded url

void URLParser::BuildURL(BString& URL, URLParser& base)
{   
//	If embedded URL has a scheme, its absolute

	if (Scheme() != kNOSCHEME) {
		WriteURL(URL);
		return;
	}
	
//	Embedded URL inherites scheme

	mScheme = base.Scheme();
	mSchemeStr = base.mSchemeStr;
	
//	Step 3: If embedded host is empty, embedded inherits host from base

	if (NetLocation() == NULL || !(*NetLocation())) {
		mNetLocation = base.NetLocation();
		
//	Step 4:	If embedded path is preceeded by a slash, path is not relative, proceed to 7

		const char* path = Path();
		if (path == NULL || path[0] != '/') {
		
//	Step 5:	If embedded path is empty, embedded inherits base path and

			if (path == NULL || path[0] == '\0') {
				mPath = base.Path();
				
//				(a) If embedded params are empty, inherit base params and

				if (Params() == NULL || !(*Params())) {
					mParams = base.Params();
					
//				(b) If embedded query is empty, inherit base query

					if (Query() == NULL || !(*Query()))
						mQuery = base.Query();
				}
			} else {

//	Step 6:	Last segment of base path is removed and embedded path is appended in its place

				AppendBasePath(base.BasePath());
			}
		}
	}
	
//	Step 7:	Recombine elements to form embedded url

	WriteURL(URL);
}

//	Calculate the base path once

const char* URLParser::BasePath()
{
	if (mBasePathKnown == false) {
		mBasePath = Path();
		int i = mBasePath.Length();
		const char *p = mBasePath.String();
		if (p) {
			while (i) {
				if (p[i-1] == '/') {
//					p[i] = '\0';
					mBasePath.Truncate(i);
					p = mBasePath.String();
					break;
				}
				--i;
			}
			if (i == 0) {
//				p[0] = 0;	// Null path for base
				mBasePath.Truncate(0);
				p = mBasePath.String();
			}
		}
		mBasePathKnown = true;
	}
	return mBasePath.String();
}

//	Reconstitute a URL from the components
//	Don't always want to include host

void URLParser::WriteURL(BString& URL, bool includeHost)
{
	if (includeHost) {
		URL = mSchemeStr;
		if (NetLocation() && *NetLocation()) {
			URL += "//";
			URL += NetLocation();
		} else {
			if (Scheme() == kFILE)		// File urls can have a null host
				URL += "//";
		}
	} else
		URL = "";
	
	const char* p = Path();
	if (p && *p) {
		if (p[0] != '/' && NetLocation() && *NetLocation())
			URL += "/";
		URL += p;
	}
	if (Params() && *Params()) {
		URL += ";";
		URL += Params();
	}
	if (Query() && *Query()) {
		URL += "?";
		URL += Query();
	}
	if (includeHost) {					// Don't write fragments if no host. (HTTP GET)
		if (Fragment() && *Fragment()) {
			URL += "#";
			URL += Fragment();
		}
	}
}

//	Append path to base path

void URLParser::AppendBasePath(const char* basePath)
{
//	BString newPath(basePath);
	char *newPath;
	if (basePath) {
		newPath = (char *)malloc(strlen(basePath) + strlen(Path()) + 1);
		strcpy(newPath, basePath);
	} else {
		newPath = (char *)malloc(strlen(Path()) + 1);
		*newPath = 0;
	}
	const char *path = Path();
	
//	Clip leading spaces

	while (isspace(path[0]))
		path++;
		
//	Clip leading '../' from path (old: if basePath is NULL)
//	I am not sure this is the correct behavior, but is required for several popular sites

//	while (basePath == NULL && strncmp(path,"../",3) == 0) {
	bool dotdot = false;
	if ( (basePath == NULL || *basePath == 0) || 
		 ((basePath != NULL) && (basePath[0] == '/') && (basePath[1] == '\0')) ) {
		while (strncmp(path,"../",3) == 0) {
			dotdot = true;
			path += 2;		
			if (strncmp(path,"/..",3) == 0)
				path++;
		}
		if (dotdot) {
			// this prevents base: '/' path: '../foo.gif' turining into '//foo.gif'
			if ((basePath != NULL) && (basePath[0] == '/'))
				path++;
		}
	}
	
	//pprint("Path: '%s'",p);
	//pprint("basePath: '%s'",basePath);

//	newPath += path;
	strcat(newPath, path);
	char *p = newPath;
	
//	All occurences of "./" are removed;

	while (p[0] == '.' && p[1] == '/') {
		strcpy(p,p+2);
	}
		
	char *s;
	while ((bool)(s = strstr(p,"/./"))) {		// Remove /./ case
		strcpy(s,s+2);
	}

//	If the path ends in a "." the "." is removed
	
	int i = strlen(p);
	if (p[i-1] == '.' && p[i-2] == '/')
		p[i-1] = '\0';
		
//	All occurences of "<segment>/../" where <segment> != ".." are removed
	
	s = p;
	while ((bool)(s = strstr(s,"/../"))) {
		char* ss = s + 3;
		while (s > p) {
			s--;
			if (s[0] == '/')
				break;
		}
		if (s[0] == '/') {
			if (s[1] == '.' && s[2] == '.')	// Segment is a "..", skip it
				s = ss;
			else {
				strcpy(s,ss);
			}
		} else
			s = ss;
	}
	
//	If path ends with "<segment>/.." where <segment> != ".." are removed

	s = p + strlen(p) - 3;
	if (strcmp(s ,"/..") == 0) {
		while (s > p) {
			--s;
			if (s[0] == '/')
				break;
		}
		if (s[0] == '/')
			s[1] = '\0';
	}

	mPath = newPath;
	free(newPath);
}

//	Guess a scheme if one is not known (usually guess http)

URLScheme	URLParser::GuessScheme()
{
	if (mScheme != kNOSCHEME)
		return mScheme;
		
	if (NetLocation() && strncmp(NetLocation(),"ftp.",4) == 0)
		return kFTP;
		
	return kHTTP;
}

//	Return the pieces of the url

URLScheme	URLParser::Scheme()
{
	return mScheme;
}

const char*	URLParser::NetLocation()
{
	return mNetLocation.String();
}

const char*	URLParser::Path()
{
	return mPath.String();
}

const char*	URLParser::Query()
{
	return mQuery.String();
}

const char*	URLParser::Params()
{
	return mParams.String();
}

const char*	URLParser::Fragment()
{
	return mFragment.String();
}

const char*	URLParser::HostName()
{
	return mHostName.String();
}

int URLParser::HostPort()
{
	return mHostPort;
}

const char*	URLParser::User()
{
	return mUser.String();
}

const char*	URLParser::Password()
{
	return mPassword.String();
}

//=======================================================================
//=======================================================================

void BuildAURL(BString& URL, const char* partialURL, const char* baseURL)
{
	if (partialURL == NULL || partialURL[0] == '\0') {
		URL = baseURL;
		return;
	}
	
	if (baseURL == NULL || baseURL[0] == '\0') {
		URL = partialURL;
		return;
	}

	URLParser parser;
	URLParser base;
	parser.SetURL(partialURL);
	base.SetURL(baseURL);
	
	parser.BuildURL(URL,base);
}

void
URLParser::Test()
{

}

//	Escape non-legal chars

void EscapeURL(BString& url)
{
	const char* s = url.String();
	const char hexchar[] = "0123456789ABCDEF";
	
//	Count how many chars need to be escaped

	int count = 0;
	for (const char *c = s; *c; c++) {
		if (*c <= 0x20 || *c == '%' || *c > 0x7E)
			count++;
	}
	if (count == 0)
		return;
		
	char *u = (char*)malloc(strlen(s) + count*2 + 1);
	int j = 0;
	for (int i = 0; s[i]; i++) {
		unsigned int c = (unsigned int)s[i];
		if (c <= 0x20 || c == '%' || c > 0x7E) {	// Other illegal chars?
			u[j++] = '%';
			u[j++] = hexchar[(c >> 4) & 0x0F];
			u[j++] = hexchar[c & 0x0F];
		} else
			u[j++] = c;
	}
	u[j++] = 0;
	url = u;
	free(u);
}

void FileRefToURL(BEntry& entry, BString& url)
{
	url = "file://";
	BPath path;
	entry.GetPath(&path);
	url += path.Path();
	EscapeURL(url);
	pprint("FileRefToURL: '%s'",url.String());
}

//	Convert the HTML %20 nonsense

short Nybble(char c)
{
	return (c >= '0' && c <= '9') ? c - '0' : c - 'A' + 10;
}

void CleanName(const char *src, char *dst)
{
	char c;
	do {
		c = *src++;
		if (c == '%') {
			*dst++ = (Nybble(src[0]) << 4) | Nybble(src[1]);
			src += 2;
		} else
			*dst++ = c;
	} while (c);
}

bool LaunchExternalURLHandler(const BString url)
{
	URLParser parser;
	parser.SetURL(url.String());

	const char *handler = NULL;
	
	switch(parser.Scheme()) {
		case kFTP:
			handler = B_URL_FTP;
			break;
		case kNNTP:
			handler = B_URL_NNTP;
			break;
		case kNEWS:
			handler = B_URL_NEWS;
			break;
		case kGOPHER:
			handler = B_URL_GOPHER;
			break;
		case kMAILTO:
			handler = B_URL_MAILTO;
			break;
		case kTELNET:
			handler = B_URL_TELNET;
			break;
		case kRLOGIN:
			handler = B_URL_RLOGIN;
			break;
		case kTN3270:
			handler = B_URL_TN3270;
			break;
		case kWAIS:
			handler = B_URL_WAIS;
			break;
		case kNOSCHEME:
		case kUNKNOWNSCHEME:
		case kNETPOSITIVE:
		case kFILE:
		case kHTTP:
		case kHTTPS:
		case kJAVASCRIPT:
		default:
			// Try to handle these internally.
			break;
	}
	if (!handler)
		return false;
	
	// If we can't find out about what app this MIME type is mapped to, or if it's
	// mapped to us, then return false to handle internally.
	entry_ref appRef;
	char sig[128];
	status_t status = B_ERROR;

	BMessage msg(B_ARGV_RECEIVED);
	msg.AddString("argv", "app");	// Bogus.  argv[0] is supposed to be your app name.
	msg.AddString("argv", url.String());
	msg.AddInt32("argc",2);

	if (be_roster->FindApp(handler, &appRef) == B_OK &&
		BNode(&appRef).ReadAttr("BEOS:APP_SIG", B_MIME_TYPE, 0, sig, 127) != 0 &&
		strcasecmp(sig, kApplicationSig) != 0) {
	
		status_t status = be_roster->Launch(handler, &msg);
		if (status == B_OK)
			return true;
	}
	if (status != B_OK && parser.Scheme() == kMAILTO) {
		// Maintain compatiblity with previous behaviour.
		status_t status = be_roster->Launch("text/x-email", &msg);
		return (status == B_OK || status == B_ALREADY_RUNNING);
	}
	return false;		
}
