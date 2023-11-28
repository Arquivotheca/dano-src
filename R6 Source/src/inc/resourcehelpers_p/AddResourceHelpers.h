/*
01234567890123456789012345678901234567890123456789012345678901234567890123456789
012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901
*/

#ifndef _ADDRESOURCEHELPERS_H_
#define _ADDRESOURCEHELPERS_H_

#include <Message.h>
#include <Resources.h>
#include <Application.h>

status_t ResString(BResources*r,const char*str,int32 id,const char* name);
void ResStringAbort(BResources*r,const char*str,int32 id,const char* name);

status_t ResMessage(BResources*r,BMessage*msg,int32 id,const char* name);
void ResMessageAbort(BResources*r,BMessage*msg,int32 id,const char* name);

status_t ResArchivable(BResources*r,BArchivable*src,int32 id,const char* name);
void ResArchivableAbort(BResources*r,BArchivable*src,int32 id,const char* name);

status_t ArchiveBitmap(BMessage*m,const char*src);
void ArchiveBitmapAbort(BMessage*m,const char*src);

class AApplication:public BApplication {
public:
	AApplication();
	virtual void ReadyToRun()=0;
	void ArgvReceived(int32 argc,char** argv);
	char* target;
};

#endif // _ADDRESOURCEHELPERS_H_
