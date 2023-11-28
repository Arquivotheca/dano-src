#include "HTConnection.h"
#include "HTEscape.h"

#include <stdlib.h>
#include <Message.h>


#define DEBUG 0
#include <Debug.h>

HTConnection::HTConnection()
	:	BTSBuffSocket(),
		fProxy(NULL),
		fUserAgent(NULL),
		fHost(NULL),
		fResource(NULL),
		followRedirects(true),
		fRedirected(false)
{
	fUserAgent = strdup("Mozilla/2.0 (compatible; NetPositive; BeOS)");
}


HTConnection::~HTConnection()
{
	free(fProxy);
	free(fUserAgent);
	free(fHost);
	free(fResource);
	
//	Close();
}

void
HTConnection::SetProxy(const char *p, int32 port)
{
	fProxyPort = port;
	free(fProxy);
	fProxy = strdup(p);
}

const char	*
HTConnection::GetProxy(int32 &port)
{
	port = fProxyPort;
	return fProxy;
}

void
HTConnection::SetUserAgent(const char *p)
{
	free(fUserAgent);
	fUserAgent = strdup(p);
}

const char	*
HTConnection::GetUserAgent()
{
	return fUserAgent;
}

int32
HTConnection::ContentLength()
{
	return headers.ulContentLength;
}

const char	*
HTConnection::StatusLine()
{
	return headers.statusLine;
}

status_t
HTConnection::SetURL(const char *fullURL)
{
	PRINT(("HTConnection::SetURL\n"));

	if (!fullURL)
		return B_ERROR;
		
	free(fHost);
	free(fResource);
	fHost = NULL;
	fResource = NULL;

	if (GetProxy(fHostPort)) {
		// using proxy
		PRINT(("HTConnection::SetURL -- Using proxy!\n"));

		fHost = strdup(GetProxy(fHostPort));
		
		if (strncmp(fullURL,"http://",7) != 0) {
			int urlen = strlen(fullURL);
			fResource = (char *)malloc(7 + urlen + 1);
			memcpy(fResource,"http://",7);
			memcpy(fResource+7,fullURL,urlen+1);
		}
		else {
			fResource = strdup(fullURL);
		}
	}
	else {
		PRINT(("HTConnection::SetURL -- Not using proxy, parsing url!\n"));

		// not using proxy
		char *c;
		char *url = strdup(fullURL);
			
		c = strchr(url,':');
		if (!c)
			c = url;
		else
			c++;
		
		// skip any leading slashes after the :
		while (*c && *c == '/')
			c++;
		
		char *rhost = c;
		
		// find the host
		c = strchr(rhost,'/');
		if (!c) {
			fHost = strdup(rhost);
			fResource = strdup("/");
		}
		else {
			*c = 0;
			fHost = strdup(rhost);
			*c = '/';
			fResource = strdup(c);
		}
		
		// find the port in the host
		c = strchr(fHost,':');
		if (c) {
			// the remaining characters should be a port
			*c = 0;
			c++;
			fHostPort = atol(c);
			PRINT(("url specified port %d\n",fHostPort));
			if (fHostPort <= 0) fHostPort = 80;
		}
		else {
			fHostPort = 80;
			PRINT(("defaulting to port %d\n",fHostPort));
		}
		
		free(url);
	}
	
	PRINT(("fHost is %s\n",fHost));
	PRINT(("fResource is %s\n",fResource));
	
	return B_NO_ERROR;
}


/*** can block ***/
status_t
HTConnection::Connect(const char *fullURL)
{
	PRINT(("HTConnection::Connect\n"));

	if (IsConnected())
		Close();
	
	if (fullURL) {
		SetURL(fullURL);	
	}
	
	if ( !(fHost && fResource)) {
		PRINT(("null host or resource\n"));
		return B_ERROR;
	}
	PRINT(("fHost is %s\n",fHost));
	PRINT(("fResource is %s\n",fResource));

	PRINT(("HTConnection::Connect -- Setting address!\n"));

	//////////////////
	status_t		res;
	BTSAddress		addr;

	/** hostname error, good, timeout?? **/
	if ((res = addr.SetTo(fHostPort,fHost)) < B_NO_ERROR)
	{
		PRINT(("hostname errror %s\n",strerror(res)));
		return res;
	}
	
	/** make connect to addr interruptible **/
	if ((res = ConnectToAddress(addr)) < B_NO_ERROR) {
		PRINT(("could not connect to %s  %s\n",fHost,strerror(res)));
		return res;
	}
	PRINT(("connection successful\n"));

	return B_NO_ERROR;	
}

/** can block **/
status_t
HTConnection::Get(const char *queryString, int redirectCount)
{
	if (redirectCount == 0)
		fRedirected = false;
	if (IsConnected()) {
		/** any write calls could block **/
		WriteString("GET ");
		WriteString(fResource);
		if (queryString) {
			WriteString("?");
			WriteString(queryString);
		}
		WriteLine(" HTTP/1.0");
		// headers here
		WriteString("User-Agent: ");
		WriteLine(GetUserAgent());
		// complete the get request
		WriteLine("");
		// error checking!
	
		/** these could block **/	
		status_t res = headers.RcvStatus(this);
		headers.RcvHeaders(this);
	
		if (followRedirects && redirectCount < 5)
		{
			if (res >= 300 && res <= 303) {
				// redirect
				if (headers.szLocation) {
					//Close();
					PRINT(("redirect requested to %s\n",headers.szLocation));
					SetResourceRelative(headers.szLocation);
					PRINT(("new resource is %s\n",fResource));
					Connect();
					fRedirected = true;
					return Get(queryString, redirectCount+1);
				}
			}
		}
	
		return res;
	}
	// must be connected
	return B_ERROR;
}

status_t
HTConnection::Post(const char *data)
{
	data;
	
	// not implemented
	return B_ERROR;
}

status_t
HTConnection::Post(const BMessage *msg, int redirectCount)
{
	if (!IsConnected())
		return B_ERROR;
	
	if (redirectCount == 0)
		fRedirected = false;
	// post all the strings in a BMessage
	
	// first compute content length
	int clen = 0;
	const char  *name; 
	uint32  type;
	int32   count; 
	
	for ( int32 i = 0; msg->GetInfo(B_STRING_TYPE, i, &name, &type, &count) == B_NO_ERROR; 
         i++ )
	{
		clen += HTEscapedLen(name);
		clen += HTEscapedLen(msg->FindString(name));
		// for = and &
		clen += 2;	
	}
	char *buff = (char *)malloc(clen);
	
	char *dst = buff;
	for ( int32 i = 0; msg->GetInfo(B_STRING_TYPE, i, &name, &type, &count) == B_NO_ERROR; 
         i++ )
	{
		if (i > 0)
			*dst++ = '&';
			
		const char *c = name;
		while (*c) {
			if (IsURLEscaped(*c)) {
				URLEscapeChar(dst,*c++);
				dst += 3;
			}
			else
				*dst++ = *c++;
		}
		*dst++ = '=';
		
		c = msg->FindString(name);
		while (*c) {
			if (IsURLEscaped(*c)) {
				URLEscapeChar(dst,*c++);
				dst += 3;
			}
			else
				*dst++ = *c++;
		}	
		*dst = 0;
	}
	
	// now we have the string
	// perform the posting

	WriteString("POST ");
	WriteString(fResource);
	WriteLine(" HTTP/1.0");
	// headers here
	WriteString("User-Agent: ");
	WriteLine(GetUserAgent());

	char line[80];
	sprintf(line,"Content-Length: %d",strlen(buff));
	WriteLine(line);
	WriteLine("Content-Type: application/x-www-form-urlencoded");
	PRINT((line));
	PRINT(("\n"));
	PRINT((buff));
	PRINT(("\n"));
	
	// complete the post message header
	WriteLine("");
	// 
	WriteLine(buff);
	
	// error checking!
		
	status_t res = headers.RcvStatus(this);
	headers.RcvHeaders(this);
	
	free(buff);
	
	if (followRedirects && redirectCount < 5)
	{
		if (res >= 300 && res <= 303) {
			// redirect
			if (headers.szLocation) {
				//Close();
				PRINT(("redirect requested to %s\n",headers.szLocation));
				SetResourceRelative(headers.szLocation);
				PRINT(("new resource is %s\n",fResource));
				Connect();
				fRedirected = true;
				return Post(msg);
			}
		}
	}
	
	return res;
}

void
HTConnection::SetResourceRelative(const char *res)
{
	if (!res)
		return;
		
	if (!fResource) {
		fResource = strdup(res);
		return;
	}
	
	if (strncmp(res,"http://",7) == 0) {
		PRINT(("SetResourceRelative -- full url\n"));
		// if we start with :// then
		// new full url, new host
		
		// if we are using a proxy simply set the full resource
		// if we are not using a proxy pull out the url
		SetURL(res);
		
		return;
	}
	
	if (res[0] == '/')
	{
		PRINT(("SetResourceRelative -- full path\n"));

		// full path but relative to this server
		int32	thePort;
		if (GetProxy(thePort)) {
			PRINT(("SetResourceRelative -- using proxy\n"));
			
			// if we are using a proxy then we need to prepend the original servername
			char *c;
			c = strchr(fResource,':');
			if (!c)
				c = fResource;
			else
				c++;
		
			// skip any leading slashes after the :
			while (*c && *c == '/')
				c++;
			
			// find the end of the host
			c = strchr(c,'/');
			if (c) *c = 0;
			
			int prefixLen = strlen(fResource);
			int newLen = strlen(res);
			char *nfResource = (char *)malloc(prefixLen + newLen + 1);
			memcpy(nfResource,fResource,prefixLen);
			memcpy(nfResource+prefixLen,res,newLen+1);
			
			free(fResource);
			fResource = nfResource;
			PRINT(("fResource is %s\n",fResource));
		}
		else {
			PRINT(("SetResourceRelative -- not using proxy\n"));

			free(fResource);
			fResource = strdup(res);
		}
		PRINT(("SetResourceRelative -- fResource is %s\n",fResource));
		return;
	}
	
	if (res[0])
	{
		PRINT(("SetResourceRelative -- relative path\n"));

		// relative to existing path (no need to worry about proxy)

		int rlen = strlen(fResource);
		int i = rlen-1;
		while (i >= 0 && fResource[i] != '/') {
			i--;
		}
		// i is the index to goto
		if (i == 0) {
			free(fResource);
			fResource = strdup(res);
		}
		else {
			// calc length of new string
			int nlen = strlen(res);
			char *newRes = (char *)malloc(i + 1 + nlen + 1); // base + '/' + new + '0'
			memcpy(newRes,fResource,i+1);
			memcpy(newRes+i+1,res,nlen+1);
			free(fResource);
			fResource = newRes;
		}
		PRINT(("SetResourceRelative -- fResource is %s\n",fResource));
		return;
	}
}
