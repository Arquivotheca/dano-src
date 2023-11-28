#include "VHTUtils.h"
#include "VHTConnection.h"
#include "SettingsManager.h"
#include "ValetVersion.h"

#define DEBUG 0
#include <Debug.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <String.h>

extern SettingsManager *gSettings;

const char *kRegConnect = 		"/services/registration.asp";
const char *kGetUpdateConnect = "/services/get_update.asp";
const char *kUpdateConnect = 	"/services/update_response.asp";

VHTConnection::VHTConnection()
{
	BString userAgent("Mozilla/2.0 (compatible; ");
	userAgent << kVersionString;
#if __INTEL__
	userAgent << "; BeOS Intel)";
#elif __arm__
	userAgent << "; BeOS ARM)";
#elif __POWERPC__
	userAgent << "; BeOS PowerPC)";
#else
#error Unknown userAgent
#endif
	SetUserAgent(userAgent.String());
	
	const char *proxy;
	gSettings->data.FindString("comm/proxy",&proxy);
	if (proxy && *proxy) {
		PRINT(("setting proxy\n"));
		SetProxy(proxy,gSettings->data.FindInt32("comm/proxyport"));
	}
	//char buf[256];
	//sprintf(buf,"http://%s",gSettings->data.FindString("comm/servername"));
	
	SetURL(gSettings->data.FindString("comm/servername"));
}



status_t	GetHttpResponse(HTConnection &ht, char *buffer, int bufsz)
{
	bool intag = false;
	char *lbuf = (char *)malloc(bufsz);
	strncpy(buffer,ht.StatusLine(),bufsz);

	int res = ht.Read(lbuf,bufsz-1);
	if (res > 0)
	{
		lbuf[res] = '\0';
		HTMLTextify(buffer,lbuf,false);
		return 0;
	}
	return res;
}

status_t	GetValetResponse(HTConnection &ht, BMessage *msg)
{
	PRINT(("GETTING VALET REPSONSE!\n"));
	char linebuf[256];
	char *max = linebuf+256;
	int linenumber = 0;
	
	while (ht.ReadLine(linebuf,256) >= 0) {
		PRINT((linebuf));
		PRINT(("\n"));
		
		char *hname = linebuf;
		char *c = hname;
		
		while(*c && *c != ':' && c < max) {
        	*c = tolower(*c);
        	c++;
        }
        if (*c)
        	*c++ = 0;
        while (*c == ' ' && *c)
        	c++;
        
        char *hval = c;
        
        PRINT(("hname IS:\n%s\n**************\n\n",hname));
        PRINT(("hval IS:\n%s\n**************\n\n",hval));
        
        /***
        int dlen = 0;
        while (*c) {
        	if (!isdigit(*c) && !(*c == '-' && dlen == 0)) {
        		dlen = 0;
        		break;
        	}
        	c++;
        	dlen++;
        }
        ***/
        if (*hname && *hval) {
	        //if (dlen > 0 && dlen < 12)
	        //	msg->AddInt32(hname,atol(hval));
	        //else
	        msg->AddString(hname,hval);
       	}
       	else if ((*hname || *hval) && linenumber < 3) {
       		char message[512];


			GetHttpResponse(ht, message, 512);
			msg->AddInt32("status",-1);
       		msg->AddString("message",message);
       		break;
       	}
       	linenumber++;
	}
	
	// translate status string to integer	
	if (!msg->HasString("status")) {
		msg->AddInt32("status",-1);
		msg->AddString("message","Protocol error, expected status field");	
	}
	else {
		int32 status = atol(msg->FindString("status"));
		msg->RemoveName("status");
		msg->AddInt32("status",status);
	}
	return 0;
}

// not dope
// problems when a tag name is split across a line buffer
// works better with a single memory buffer
bool	HTMLTextify(char *dst, char *src, bool intag)
{
	if (intag && *src)
		*src = '<';
	while (*src) {
		// try to swallow the tag
		if (*src == '<') {
			src++;
			while (*src && isspace(*src))
				src++;
			
			// simple unbroken tags
			if (!intag) {
				intag = true;
			
				char *tag = src;
				while (*src && !isspace(*src) && *src != '>')
					src++;
				if (*src) {
					// not null
					if (*src == '>')
						intag = false;
						
					*src++ = 0;
					if (!strcasecmp(tag,"P") || !strcasecmp(tag,"BR") ||
						 !strcasecmp(tag,"/TITLE"))
					{
						*dst++ = '\n';
						*dst++ = '\n';
					}
				}
			}
			if (intag) {
				// try to skip to the tag close
				while (*src && *src != '>')
					src++;
				if (*src == '>') {
					src++;
					intag = false;
				}
			}
		}
		// don't want extra returns or newlines
		else if (*src != '\r' && *src != '\n')
			*dst++ = *src++;
		else
			src++;
	}
	*dst = 0;
	return intag;
}

