#ifndef _HTCONNECTION_H_
#define _HTCONNECTION_H_


#include "BTSBuffSocket.h"

#include "HTHeaders.h"
class BMessage;

// how to support proxies???
class HTConnection : public BTSBuffSocket
{
public:
	HTConnection();
	virtual	~HTConnection();
	
	// return http errors
	// os/net kit errors
	// private errors
	status_t	Connect(const char *fullURL = NULL);
	status_t	SetURL(const char *fullURL);
	
	// should return a full url!!
	//const char 	*GetURL();
	// return true if the resource was relocated, if so call get url
	// to get the new value
	bool		WasRelocated() {
					return fRedirected;
				};
	
	status_t	Get(const char *queryString = NULL, int redirectCount = 0);
	status_t	Post(const BMessage *msg, int redirectCount = 0);
	status_t	Post(const char *data);
	

	
	// -1 if unknown
	int32		ContentLength(); // ?? maybe get from headers
	const char	*StatusLine();
	
	void		SetFollowRedirect(bool f) {
					followRedirects = f;
				}
	bool		GetFollowRedirect() {
					return followRedirects;
				};
	// inherit readline
	// inherit read
	
	//
	
	// malformed url errors
	//void 		SetURL(const char *url);

	//
	void		SetProxy(const char *, int32 port);
	const char	*GetProxy(int32 &port);

	void		SetUserAgent(const char *);
	const char	*GetUserAgent();

	void		SetResourceRelative(const char *res);	
	HTHeaders	headers;
private:
	bool		followRedirects;
	bool		fRedirected;
	
	char		*fUserAgent;	// name of browser
	
	// information for a proxy (if any)
	char		*fProxy;
	int32		fProxyPort;

	// the server we are using
	char		*fHost;			// name of host
	int32		fHostPort;
	
	// the requestline sent to the server
	char		*fResource;		// resource-line
};


#endif

