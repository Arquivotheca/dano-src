//******************************************************************************
//
//	File:			MSWordContent.cpp
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
//  Written by: Adam Haberlach
//
//******************************************************************************

#include "MSWordContent.h"
#include "oleutils.h"

char DebuggerStr[1024];
char* const DebugStrPtr(void)
{
	return DebuggerStr;
}

using namespace Wagner;

MSWordContentInstance::MSWordContentInstance(Content *content, GHandler *handler) :
	ContentInstance(content, handler)
{
	printf("MSWordContentInstance::MSWordContentInstance\n");
}

MSWordContent::MSWordContent(void *handle) :
	Content(handle),
	fIOBuffer(NULL),
	fDone(false)
{
	printf("MSWordContent::MSWordContent\n");
	fIOBuffer = new BMallocIO();
	
}

MSWordContent::~MSWordContent()
{
	printf("MSWordContent::~MSWordContent\n");
	if(fIOBuffer) {
		delete fIOBuffer;
	}
}

status_t 
MSWordContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &msg)
{
	printf("MSWordContent::CreateInstance\n");

	*outInstance = new MSWordContentInstance(this, handler);
	return B_OK;
}

ssize_t 
MSWordContent::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
	fIOBuffer->Write(buffer, bufferLen);
	
	fDone = done;

	if(fDone) {
		printf("Feed finished\n");
		if (Translate() == B_NO_ERROR) {
			printf("Done with Translate()\n");
		}
	}

	return bufferLen;
}

status_t 
MSWordContent::Translate()
{
/* by now we should have a valid document in fIOBuffer */
	BMallocIO	*tTransbuffer = new BMallocIO;
	TBlockStreamWriter *tWriter = new TBlockStreamWriter(tTransbuffer);

	TImportWORD	*tImporter = new TImportWORD(fIOBuffer, tWriter, NULL);
	tImporter->DoTranslate();
	

	tTransbuffer->Seek(0, SEEK_SET);
	
	/* theoretically, we now have a tTransbuffer full of block data */
	delete fIOBuffer;
	fIOBuffer = new BMallocIO;
	
	TBlockStreamReader *tReader = new TBlockStreamReader(tTransbuffer);

	TExportHTML *tExporter = new TExportHTML(tReader, fIOBuffer, B_ISO1_CONVERSION);
	if(tExporter->DoTranslate(NULL) != B_OK) {
		printf("DoTranslate Kacked\n");
	}


	char buffer[160];
	int	bytes;

//	printf("================Results============\n");
//	fIOBuffer->Seek(0, SEEK_SET);
//	while(bytes = fIOBuffer->Read(buffer, 160)) {
//		printf("%*s\n", bytes, buffer);
//	}
//	printf("================Results============\n");
	
	delete(tImporter);
	delete(tWriter);

	delete(tReader);
	delete(tExporter);
	
	return B_OK;
}


size_t 
MSWordContent::GetMemoryUsage()
{
	printf("MSWordContent::GetMemoryUsage()\n");
	return 0;
}


