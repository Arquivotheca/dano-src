// ---------------------------------------------------------------------------
/*
	GCCBuilderHandler.h
	
	Template class to handle both a GCCLinker and GCCBuilder.  Used by the
	CommandLineTextView to ask for BuildStandardArgv without having to
	introduce templates into CommandLineTextView.
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			2 November 1998

*/
// ---------------------------------------------------------------------------

#ifndef _GCCBUILDERHANDLER_H
#define _GCCBUILDERHANDLER_H


#include "GCCBuilder.h"
#include "GCCLinker.h"

#include <String.h>
#include <stdlib.h>

class MProject;

class GCCBuildHandler
{
public:
	virtual void GetOptionText(MProject* project, BString& outString) = 0;

};

// ---------------------------------------------------------------------------
//	class GCCBuilderHandler - templates implementiong GCCBuildHandler
// ---------------------------------------------------------------------------

template <class T>
class GCCBuilderHandler : public GCCBuildHandler
{
public:
	GCCBuilderHandler(T* builder);
	
	virtual void GetOptionText(MProject* project, BString& outString);

private:
	T* fBuilder;
};

// ---------------------------------------------------------------------------
//	GCCBuilderHandler member functions
// ---------------------------------------------------------------------------

template <class T>
GCCBuilderHandler<T>::GCCBuilderHandler(T* builder)
{
	fBuilder = builder;
}

// ---------------------------------------------------------------------------

template <class T>
void
GCCBuilderHandler<T>::GetOptionText(MProject* project, BString& outString)
{
	BList argv;
	fBuilder->BuildStandardArgv(project, argv);
	for (int i = 0; i < argv.CountItems(); i++) {
		char* option = (char*) argv.ItemAt(i);
		outString.Append(option);
		outString.Append(" ");
		// delete as we go
		// we don't look at this item again (they are malloc'ed)
		free(option);
	}
	argv.MakeEmpty();	
}

#endif
