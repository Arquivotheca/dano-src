//
//	These are functions that are called by the plugin to make requests
//	to the browser.
//	See: http://developer.netscape.com/docs/manuals/communicator/plugin/index.htm
//

#include "npapi.h"

void NPN_Version(int* plugin_major, int* plugin_minor,
	int* netscape_major, int* netscape_minor)
{
}

NPError NPN_GetURLNotify(NPP instance, const char* url, const char* target,
	void* notifyData)
{
}

NPError NPN_GetURL(NPP instance, const char* url, const char* target)
{
}

NPError NPN_PostURLNotify(NPP instance, const char* url,
  const char* target, uint32 len, const char* buf, NPBool file,
  void* notifyData)
{
}

NPError NPN_PostURL(NPP instance, const char* url,
	const char* target, uint32 len,	const char* buf, NPBool file)
{
}

void NPN_Status(NPP instance, const char* message)
{
}

const char* NPN_UserAgent(NPP instance)
{
	return "Mozilla 4.0 (compatible; Netscape)";
}

uint32 NPN_MemFlush(uint32 size)
{
}

void NPN_ReloadPlugins(NPBool reloadPages)
{
}

JRIEnv*	NPN_GetJavaEnv(void)
{
}

jref NPN_GetJavaPeer(NPP instance)
{
}

void* NPN_MemAlloc(uint32 size)
{
	return malloc(size);
}

void NPN_MemFree(void* ptr)
{
	free(ptr);
}

NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
}

NPError NPN_NewStream(NPP instance, NPMIMEType type,
							  const char* target, NPStream** stream)
{
}

int32 NPN_Write(NPP instance, NPStream* stream, int32 len,
						  void* buffer)
{
}

NPError NPN_DestroyStream(NPP instance, NPStream* stream,
								  NPReason reason)
{
}
