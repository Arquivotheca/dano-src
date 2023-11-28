
//
// WWW Server  File: headers.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#ifndef _HTHEADERS_H_
#define _HTHEADERS_H_

class BTSBuffSocket;

class HTHeaders
{
  public:

	HTHeaders();
	~HTHeaders();
	
	status_t	RcvStatus(BTSBuffSocket *sClient);
  	status_t	RcvHeaders(BTSBuffSocket *sClient);

  char *szMethod,
       *szUri,
       *szAccept,
       *szAcceptCharset,
       *szAcceptEncoding,
       *szAcceptLanguage,
       *szAcceptRanges,
       *szAge,
       *szAllow,
       *szAuth,
       *szCacheControl,
       *szConnection,
       *szContentBase,
       *szContentEncoding,
       *szContentLanguage,
       *szContentLength,
       *szContentLocation,
       *szContentMD5,
       *szContentRange,
       *szContentType,
       *szDate,
       *szETag,
       *szExpires,
       *szFrom,
       *szHost,
       *szIfModSince,
       *szIfMatch,
       *szIfNoneMatch,
       *szIfRange,
       *szIfUnmodSince,
       *szLastMod,
       *szLocation,
       *szMaxForwards,
       *szPragma,
       *szPublic,
       *szRange,
       *szReferer,
       *szRetryAfter,
       *szServer,
       *szTransferEncoding,
       *szUpgrade,
       *szUserAgent,
       *szVary,
       *szVia,
       *szWarning,
       *szWWWAuth,
       *szIpName,
       *szIpAddr,
       *szRealm;
  time_t 	ttIfModSince,
         	ttIfUnmodSince;
  bool	 	bPersistent;
  ulong		ulContentLength;
  char		*statusLine;

private:
	void	InitHeaders();
	void	FreeHeaders();
	void	ResetHeaders();
	bool	first;
};

// ------------------------------------------------------------------
// ------------------------------------------------------------------


#endif
