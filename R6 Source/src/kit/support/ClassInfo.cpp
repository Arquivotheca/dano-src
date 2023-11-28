/*

	ClassInfo.cpp
	
	Copyright 1994 Be, Inc. All Rights Reserved.
	
*/

#include <string.h>

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#ifndef _CLASS_INFO_H
#include <ClassInfo.h>
#endif

// Define the runtime type info for the BClassInfo class itself!
B_DEFINE_ROOT_CLASS_INFO(BClassInfo);

BClassInfo::BClassInfo(const char *name, const BClassInfo *base)
{
	fClassName = name;
	fBaseClass = base;
}

const char *BClassInfo::Name() const
{
	return fClassName;
}

bool	BClassInfo::IsSameAs(const BClassInfo *c) const
{
	return (this == c) || (strcmp(fClassName, c->fClassName) == 0);
}

bool	BClassInfo::CanCast(const BClassInfo *c) const
{
	// can "this" be cast to "c"?
//	PRINT(("\n## Can \"%s\" be cast to a \"%s\"?\n", this->Name(), c->Name()));
	
	return IsSameAs(c) || DerivesFrom(c);
}

bool	BClassInfo::DerivesFrom(const BClassInfo *c, bool DirectOnly) const
{
	// does "this" have a base class "c"?
	
//	PRINT(("## Does \"%s\" have a base class \"%s\"?\n", this->Name(), c->Name()));
	
	const BClassInfo	*base = fBaseClass;
	
	if (!base) {
		return FALSE;
	}
	
	if (DirectOnly) {
//		PRINT(("  ## checking %s\n", base->Name()));
		return base->IsSameAs(c);
	}
	
	// walk up the chain of base classes looking for a match
	do {
//		PRINT(("  ## checking %s\n", base->Name()));
		if (base->IsSameAs(c))
			return TRUE;
	} while (base = base->fBaseClass);
	
	return FALSE;	
}
