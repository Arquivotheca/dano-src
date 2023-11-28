#ifndef _SECURITY_MANAGER_H
#define _SECURITY_MANAGER_H

#include <support2/Locker.h>
#include <www2/BArray.h>
#include <support2/URL.h>
#include <www2/Resource.h>

namespace B {
namespace WWW2 {

struct Suffix;

class SecurityManager {
public:
	SecurityManager();
	void RegisterGroup(const char *urlSpec, const char *groupName);
	GroupID RegisterGroup(const char *groupName);
	GroupID GetGroupID(const BUrl &url);
	bool CheckAccess(const BUrl &url, GroupID requestorGroup);
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

} } // namespace B::WWW2

extern B::WWW2::SecurityManager securityManager;

#endif
