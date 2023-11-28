// ===========================================================================
//	URL.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __URL__
#define __URL__

#include <SupportDefs.h>
#include <String.h>
class BEntry;

//=======================================================================
//	Extends a url from human input to a useful url
//
//	cnn
//	cnn.com
//	www.cnn.com
//

bool ValidateURL(BString& url);

//===========================================================================
//	Known URL schemes

enum URLScheme {
	kNOSCHEME,
	kUNKNOWNSCHEME,
	kNETPOSITIVE,
	
	kFILE,
	kFTP,
	kHTTP,
	kHTTPS,
	kNEWS,
	kNNTP,
	
	kMAILTO,
	kTELNET,
	kGOPHER,
	kRLOGIN,
	kTN3270,
	kWAIS,
	
	kJAVASCRIPT
};

//===========================================================================
//	Object to parse a URL into its useful pieces
//	
//	URL's have the form
//	<scheme>://<net_loc>/<path>;<params>?<query>#<fragment>
//

class URLParser /*: public NPObject*/ {
public:
					URLParser();
					
		void		SetURL(const char* url);
static	URLScheme	ParseScheme(const char* url, BString& schemeStr);
		void		ParseNetLocation();
				
		void		BuildURL(BString& URL,URLParser& base);
		void		WriteURL(BString& URL, bool includeHost = true);
					
		URLScheme	Scheme();		// http,ftp,telnet etc
		URLScheme	GuessScheme();	// guess if not sure
		const char*	NetLocation();	// Entire host name including passwords, ports etc
		const char*	Path();			// /foo/bar/page.html
		const char*	Params();		// ;Params=x
		const char*	Query();		// Everything after the '?'
		const char*	Fragment();		// Everything after the '#'
		
		const char*	BasePath();		// Base of url
	
		void		SetHostName(const char* host);
		const char*	HostName();		// Host Name
		int			HostPort();		// Host Port
		
		const char*	User();
		const char*	Password();
		
static	void		Test();
	

protected:
	void		Reset();
	void		AppendBasePath(const char* basePath);
	
	URLScheme	mScheme;
	BString		mSchemeStr;
	BString		mNetLocation;		// net_loc
	BString		mPath;
	BString		mParams;
	BString		mQuery;
	BString		mFragment;
	
	BString		mBasePath;
	bool		mBasePathKnown;
	
	BString		mHostName;
	int			mHostPort;
	
	BString		mUser;
	BString		mPassword;
};

void CleanName(const char *src, char *dst);
void FileRefToURL(BEntry& entry, BString& url);
void EscapeURL(BString& url);
bool LaunchExternalURLHandler(const BString url);

//=======================================================================

#endif
