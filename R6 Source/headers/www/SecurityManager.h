#ifndef _SECURITY_MANAGER_H
#define _SECURITY_MANAGER_H

#include "BArray.h"
#include "Locker.h"
#include "Resource.h"
#include "URL.h"

namespace Wagner {

struct Suffix;

class SecurityManager {
public:
	SecurityManager();
	void RegisterGroup(const char *urlSpec, const char *groupName);
	GroupID RegisterGroup(const char *groupName);
	GroupID GetGroupID(const URL &url);
	bool CheckAccess(const URL &url, GroupID requestorGroup);
	bool CheckAccess(GroupID target, GroupID requestorGroup);
	void EnterACL(const char *resourceClass, const char *accessingGroup, const char *action);
	const char *GetGroupName(GroupID group) const;

private:
	class ResourceClass* LookupClass(const char *name);
	void LoadSettings();
	void PrintSuffixTree(Suffix *suffix, int level) const;
	
	Suffix *fRoot;
	BArray<class ResourceClass*> fClasses;
	int32 fInitCount;
	BLocker fLock;
};

}

extern Wagner::SecurityManager securityManager;

#endif
