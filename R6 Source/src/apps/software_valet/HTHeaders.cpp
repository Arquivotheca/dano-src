
//
// WWW Server  File: headers.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DEBUG 0
#include <Debug.h>

#include "HTHeaders.h"
#include "BTSBuffSocket.h"

static time_t ConvertDate(char *szDate);




// ------------------------------------------------------------------
// ------------------------------------------------------------------
//
// ConvertDate()
//
// This routine will convert from any of the three recognized
// date formats used in HTTP/1.0 to a time_t value, the number
// of seconds since the epoch date: 00:00:00 1 January 1970 GMT.

static time_t ConvertDate(char *szDate)
{
  char szMonth[64];  // Allow extra for bad formats.
  struct tm tmData;

  if (strlen(szDate) > 34)  // Catch bad/unknown formatting.
    {
      return( (time_t) 0 );
    }

  if (szDate[3] == ',') // RFC 822, updated by RFC 1123
    {
      sscanf(szDate, "%*s %d %s %d %d:%d:%d %*s",
             &(tmData.tm_mday), szMonth, &(tmData.tm_year), &(tmData.tm_hour), 
             &(tmData.tm_min), &(tmData.tm_sec));
      tmData.tm_year -= 1900;
    }
  else if (szDate[3] == ' ') // ANSI C's asctime() format
    {
      sscanf(szDate, "%*s %s %d %d:%d:%d %d",
             szMonth, &(tmData.tm_mday), &(tmData.tm_hour), &(tmData.tm_min), 
             &(tmData.tm_sec), &(tmData.tm_year));
      tmData.tm_year -= 1900;
    }
  else if (szDate[3] < 128) // RFC 850, obsoleted by RFC 1036
    {
      sscanf(szDate, "%*s %d-%3s-%d %d:%d:%d %*s",
             &(tmData.tm_mday), szMonth, &(tmData.tm_year), &(tmData.tm_hour), 
             &(tmData.tm_min), &(tmData.tm_sec));
    }
  else  // Unknown time format
    {
      return ((time_t)0);
    }

  // Now calculate the number of seconds since 1970.
  if (strcasecmp(szMonth, "jan") == 0) tmData.tm_mon = 0;
  else if (strcasecmp(szMonth, "feb") == 0) tmData.tm_mon = 1;
  else if (strcasecmp(szMonth, "mar") == 0) tmData.tm_mon = 2;
  else if (strcasecmp(szMonth, "apr") == 0) tmData.tm_mon = 3;
  else if (strcasecmp(szMonth, "may") == 0) tmData.tm_mon = 4;
  else if (strcasecmp(szMonth, "jun") == 0) tmData.tm_mon = 5;
  else if (strcasecmp(szMonth, "jul") == 0) tmData.tm_mon = 6;
  else if (strcasecmp(szMonth, "aug") == 0) tmData.tm_mon = 7;
  else if (strcasecmp(szMonth, "sep") == 0) tmData.tm_mon = 8;
  else if (strcasecmp(szMonth, "oct") == 0) tmData.tm_mon = 9;
  else if (strcasecmp(szMonth, "nov") == 0) tmData.tm_mon = 10;
  else if (strcasecmp(szMonth, "dec") == 0) tmData.tm_mon = 11;

  tmData.tm_isdst = 0;  // There should be no daylight savings time factor.

  return(mktime(&tmData));
}

// ------------------------------------------------------------------
//
// RcvStatus()
//
// Receive and decode the status line.
//

HTHeaders::HTHeaders()
{
	InitHeaders();
	first = true;
}

HTHeaders::~HTHeaders()
{
	FreeHeaders();
}

status_t
HTHeaders::RcvStatus(BTSBuffSocket *sClient)
{
  	char lineBuf[1024];

	// get the status line
	int lineLen = sClient->ReadLine(lineBuf, 1024);
		
	// this may need fixing
	if (lineLen < 0)
		return lineLen;				// read error
	if (lineLen == 0)		
		return B_ERROR;				// there is no statusline
		
	char *c = lineBuf;
	char code[12];
	
	while(*c && !isspace(*c)) c++;
	while(*c && isspace(*c)) c++;
	
	char *d = code;
	while(*c && isdigit(*c)) *d++ = *c++;
	*d = 0;
	
	int statusCode = atol(code);
	
	return statusCode;
}

// ------------------------------------------------------------------
//
// RcvHeaders()
//
// Receive the rest of the headers sent by the client.
//

status_t
HTHeaders::RcvHeaders(BTSBuffSocket *sClient)
{
	if (!first)	{
		ResetHeaders();
		first = false;
	}

  	char *szTmp;
  	char lineBuf[1024];
  	int lineLen;

	const int headerSize = 256;
  	char szHdr[headerSize];

	// get a whole line
	while (1)
	{
		lineLen = sClient->ReadLine(lineBuf, 1024);
		
		if (lineLen == 1024)
			break;				// line too long, for now just ignore it
								// this may need fixing
		if (lineLen < 0)		
			break;				// read error
		if (lineLen == 0)		
			break;				// there are no more headers
		
		PRINT(("Header ------------\n"));
		PRINT((lineBuf));
		PRINT(("\nEnd Header ----------\n"));
		
		szTmp = lineBuf;
		if (!isspace(szTmp[0]) )  // Replace the header if not continuation.
			// if the line begins with whitespace it is a continuation header
        {
        	char *c = szHdr;
        	char *max = c + headerSize - 1;
        	// copy in the lowercase name of the header
        	while(*szTmp && *szTmp != ':' && c < max)
        		*c++ = tolower(*szTmp++);
        	
        	// null terminate
        	*c = 0;
        	
        	//if (!szTmp) {
        		//!!! not found
        		// bad header???		
        	//}
        }
      	szTmp++;
      	// Go past the ':' or ' '.
      	while ((*szTmp == ' ') && (*szTmp))
        {
          	szTmp++;  // Eliminate leading spaces.
        }

		char *szBuf;
		
		PRINT(("RcvHeaders. Header = %s\n",szHdr));
		// switch on the header name
		switch(szHdr[0])
        {
			case 'a':
			{
				if (strcmp(szHdr, "accept") == 0)
                {
					if (szAccept)
                   	{
                   		szBuf = (char *)malloc(strlen(szAccept) + strlen(szTmp) + 2);
                      	sprintf(szBuf, "%s,%s", szAccept, szTmp);
                      	free(szAcceptCharset);
                      	szAccept = szBuf;
                    }
                  	else
                    {
                    	szAccept = strdup(szTmp);
                    }
                }
              	else if (strcmp(szHdr, "accept-charset") == 0)
                {
					if (szAcceptCharset)
                    {
						szBuf = (char *)malloc(strlen(szAcceptCharset) + strlen(szTmp) + 2);
						sprintf(szBuf, "%s,%s", szAcceptCharset, szTmp);
						free(szAcceptCharset);
                      	szAcceptCharset = szBuf;
                    }
                  	else
                    {
                      	szAcceptCharset = strdup(szTmp);
                    }
                }
              	else if (strcmp(szHdr, "accept-encoding") == 0)
                {
                	if (szAcceptEncoding)
                    {
                      	szBuf = (char *)malloc(strlen(szAcceptEncoding) + strlen(szTmp) + 2);
                      	sprintf(szBuf, "%s,%s", szAcceptEncoding, szTmp);
                      	free(szAcceptEncoding);
                      	szAcceptEncoding = szBuf;
                    }
                  	else
                    {
                      	szAcceptEncoding = strdup(szTmp);
                    }
                }
              	else if (strcmp(szHdr, "accept-language") == 0)
                {
                	if (szAcceptLanguage)
                    {
                     	szBuf = (char *)malloc(strlen(szAcceptLanguage) + strlen(szTmp) + 2);
						sprintf(szBuf, "%s,%s", szAcceptLanguage, szTmp);
						free(szAcceptLanguage);
						szAcceptLanguage = szBuf;
                    }
					else
                    {
						szAcceptLanguage = strdup(szTmp);
                    }
                }
				else if (strcmp(szHdr, "authorization") == 0)
                {
					free(szAuth);
                  	szAuth = strdup(szTmp);
                }
				break;
			}
			case 'c':
            {
				if (strcmp(szHdr, "connection") == 0)
                {
					free(szConnection);
					szConnection = strdup(szTmp);
					if (strcasecmp(szConnection, "close") == 0)
                    {
						bPersistent = false;
                    }
                }
                /**
                else if (strcmp(szHdr, "content-base") == 0) {
                
                }
                else if (strcmp(szHdr, "content-encoding") == 0) {
                
                }
                else if (strcmp(szHdr, "content-language") == 0) {
                
                }
                **/
                else if (strcmp(szHdr, "content-length") == 0)
                {
					free(szContentLength);
					szContentLength = strdup(szTmp);
					ulContentLength = atol(szContentLength);
                }
                else if (strcmp(szHdr, "content-location") == 0)
                {
                	free(szContentLocation);
					szContentLocation = strdup(szTmp);
				}
				else if (strcmp(szHdr, "content-type") == 0)
                {
					free(szContentType);
					szContentType = strdup(szTmp);
                }
               
				break;
			}
			case 'd':
			{
				if (strcmp(szHdr, "date") == 0)
                {
					free(szDate);
					szDate = strdup(szTmp);
                }
				break;
            }
			case 'e':
            {
				if (strcmp(szHdr, "etag") == 0)
                {
					free(szETag);
					szETag = strdup(szTmp);
                }
				break;
            }
			case 'f':
            {
				if (strcmp(szHdr, "from") == 0)
                {
					free(szFrom);
					szFrom = strdup(szTmp);
                }
				break;
            }
			case 'h':
			{
				if (strcmp(szHdr, "host") == 0)
                {
					free(szHost);
					szHost = strdup(szTmp);
                }
				break;
			}
			case 'i':
            {
				if (strcmp(szHdr, "if-modified-since") == 0)
                {
                  free(szIfModSince);
                  szIfModSince = strdup(szTmp);
                  ttIfModSince = ConvertDate(szIfModSince);
                }
				else if (strcmp(szHdr, "if-match") == 0)
                {
					if (szIfMatch)
                    {
						szBuf = (char *)malloc(strlen(szIfMatch) + strlen(szTmp) + 2);
						sprintf(szBuf, "%s,%s", szIfMatch, szTmp);
                      	free(szIfMatch);
						szIfMatch = szBuf;
                    }
					else
                    {
						szIfMatch = strdup(szTmp);
                    }
                }
				else if (strcmp(szHdr, "if-none-match") == 0)
                {
					if (szIfNoneMatch)
                    {
						szBuf = (char *)malloc(strlen(szIfNoneMatch) + strlen(szTmp) + 2);
						sprintf(szBuf, "%s,%s", szIfNoneMatch, szTmp);
						free(szIfNoneMatch);
						szIfNoneMatch = szBuf;
                    }
					else
                    {
						szIfNoneMatch = strdup(szTmp);
                    }
				}
				else if (strcmp(szHdr, "if-range") == 0)
                {
					free(szIfRange);
					szIfRange = strdup(szTmp);
                }
				else if (strcmp(szHdr, "if-unmodified-since") == 0)
                {
					free(szIfUnmodSince);
					szIfUnmodSince = strdup(szTmp);
					ttIfUnmodSince = ConvertDate(szIfUnmodSince);
                }
				break;
            }
            case 'l':
            {
            	if (strcmp(szHdr, "location") == 0)
                {
                	free(szLocation);
					szLocation = strdup(szTmp);
				}
            	break;
			}
			case 'r':
            {
				if (strcmp(szHdr, "range") == 0)
                {
					free(szRange);
					szRange = strdup(szTmp);
                }
				else if (strcmp(szHdr, "referer") == 0)
                {
                  	free(szReferer);
					szReferer = strdup(szTmp);
                }
				break;
			}
			case 't':
            {
				if (strcmp(szHdr, "transfer-encoding") == 0)
                {
					free(szTransferEncoding);
					szTransferEncoding = strdup(szTmp);
                }
				break;
            }
			case 'u':
            {
				if (strcmp(szHdr, "upgrade") == 0)
                {
					free(szUpgrade);
					szUpgrade = strdup(szTmp);
                }
				else if (strcmp(szHdr, "user-agent") == 0)
                {
					free(szUserAgent);
					szUserAgent = strdup(szTmp);
                }
              	break;
            }
			case 'w':
            {
				if (strcmp(szHdr, "www-authenticate") == 0)
                {
					free(szWWWAuth);
					szWWWAuth = strdup(szTmp);
                }
				break;
            }
		}
	}

	if (lineLen < 0)
		return lineLen;
	
	return 0;
}

// ------------------------------------------------------------------

void	HTHeaders::InitHeaders()
{
      szMethod = NULL;
      szUri = NULL;
      szAccept = NULL;
      szAcceptCharset = NULL;
      szAcceptEncoding = NULL;
      szAcceptLanguage = NULL;
      szAcceptRanges = NULL;
      szAge = NULL;
      szAllow = NULL;
      szAuth = NULL;
      szCacheControl = NULL;
      szConnection = NULL;
      szContentBase = NULL;
      szContentEncoding = NULL;
      szContentLanguage = NULL;
      szContentLength = NULL;
      szContentLocation = NULL;
      szContentMD5 = NULL;
      szContentRange = NULL;
      szContentType = NULL;
      szDate = NULL;
      szETag = NULL;
      szExpires = NULL;
      szFrom = NULL;
      szHost = NULL;
      szIfModSince = NULL;
      szIfMatch = NULL;
      szIfNoneMatch = NULL;
      szIfRange = NULL;
      szIfUnmodSince = NULL;
      szLastMod = NULL;
      szLocation = NULL;
      szMaxForwards = NULL;
      szPragma = NULL;
      szPublic = NULL;
      szRange = NULL;
      szReferer = NULL;
      szRetryAfter = NULL;
      szServer = NULL;
      szTransferEncoding = NULL;
      szUpgrade = NULL;
      szUserAgent = NULL;
      szVary = NULL;
      szVia = NULL;
      szWarning = NULL;
      szWWWAuth = NULL;
      szIpName = NULL;
      szIpAddr = NULL;
      szRealm = NULL;
      ttIfModSince = 0;
      bPersistent = TRUE;
      ulContentLength = 0;
      statusLine = strdup("");
}

void	HTHeaders::FreeHeaders()
{
	   free(szMethod);
	   free(szUri);
	   free(szAccept);
		free(szAcceptCharset);
		free(szAcceptEncoding);
		free(szAcceptLanguage);
		free(szAcceptRanges);
		free(szAge);
		free(szAllow);
		free(szAuth);
		free(szCacheControl);
		free(szConnection);
		free(szContentBase);
		free(szContentEncoding);
       free(szContentLanguage);
       free(szContentLength);
       free(szContentLocation);
       free(szContentMD5);
       free(szContentRange);
       free(szContentType);
       free(szDate);
       free(szETag);
       free(szExpires);
       free(szFrom);
       free(szHost);
       free(szIfModSince);
       free(szIfMatch);
       free(szIfNoneMatch);
       free(szIfRange);
       free(szIfUnmodSince);
       free(szLastMod);
       free(szLocation);
       free(szMaxForwards);
       free(szPragma);
       free(szPublic);
       free(szRange);
       free(szReferer);
       free(szRetryAfter);
       free(szServer);
       free(szTransferEncoding);
       free(szUpgrade);
       free(szUserAgent);
       free(szVary);
       free(szVia);
       free(szWarning);
       free(szWWWAuth);
       free(szIpName);
       free(szIpAddr);
       free(szRealm);
       
       free(statusLine);
}

void	HTHeaders::ResetHeaders()
{
	FreeHeaders();
	InitHeaders();
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------



