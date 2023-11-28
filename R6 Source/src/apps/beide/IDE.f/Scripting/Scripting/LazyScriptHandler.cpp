//	LazyScriptHandler.cpp

#include "LazyScriptHandler.h"


LazyScriptHandler::LazyScriptHandler(
	const unsigned long		id,
	const char *			name) :
	ScriptHandler(id, name)
{
	fRefCount = 1;
}


LazyScriptHandler::~LazyScriptHandler()
{
}


void
LazyScriptHandler::Done()
{
	fRefCount--;
	if (fRefCount == 0) {
		delete this;
	}
}


ScriptHandler *
LazyScriptHandler::Reference()
{
	fRefCount++;
	return this;
}

