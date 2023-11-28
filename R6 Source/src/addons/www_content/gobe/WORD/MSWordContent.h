//******************************************************************************
//
//	File:			MSWordContent.h
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
//  Written by: Adam Haberlach
//
//******************************************************************************

#include <stdio.h>

#include <DataIO.h>

#include "ImportWORD.h"
#include "HTMLExport.h"
#include <Content.h>

using namespace Wagner;

class MSWordContentInstance : public ContentInstance {
public:
	MSWordContentInstance(Content *content, GHandler *handler);
};

class MSWordContent : public Content {
public:
	MSWordContent(void *handle);
	~MSWordContent();
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &msg);
	virtual	ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual	size_t 	GetMemoryUsage();

	status_t		Translate();

private:
	BMallocIO	*fIOBuffer;
	bool		fDone;
};

// ----------------------- GIFContentFactory -----------------------

class MSWordContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-msword");
		into->AddString(S_CONTENT_MIME_TYPES, "application/msword");
		into->AddString(S_CONTENT_EXTENSIONS, "doc");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char* mime,
								   const char* extension)
	{
		(void)mime;
		(void)extension;
		return new MSWordContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you,
	uint32 flags, ...)
{
	if( n == 0 ) return new MSWordContentFactory;
	return 0;
}
