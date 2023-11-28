#include <support2/Autolock.h>
#include <support2/StringBuffer.h>
//#include <Binder.h>
#include <www2/Resource.h>
#include <www2/SecurityManager.h>

namespace B {
namespace WWW2 {

enum AccessType{
	kRevokeAccess,
	kGrantAccess
};

const GroupID kInvalidGroup = static_cast<GroupID>(0xffffffff);
const GroupID kAllGroups = static_cast<GroupID>(0xffffffe);

struct Suffix
{
	Suffix(const char *p = "", GroupID gid = kInvalidGroup, Suffix *sib = 0)
		: 	pattern(p),
			children(0),
			sibling(sib),
			wildcard(0),
			group(gid)
	{}

	BString pattern;
	Suffix *children;
	Suffix *sibling;
	Suffix *wildcard;
	GroupID group;
};

class ResourceClass
{
	public:
		ResourceClass(const char *name);
		~ResourceClass();
		const char *GetName() const;
		bool CheckAccess(GroupID group);
		void Grant(GroupID group);
		void Revoke(GroupID group);
		GroupID GetGroupID() const;
		void SetGroupID(GroupID);
		void Print();

	private:
		void Clear();
		void Enter(GroupID group, AccessType type);

		BString fName;
		BLocker fLock;
		struct AccessSpecifier
		{
			AccessSpecifier(GroupID group, AccessType type)
			: fID(group), fType(type), fNext(0)
			{}


			GroupID fID;
			AccessType fType;
			AccessSpecifier *fNext;
		}
		*fAccessControlList;
		AccessType fDefaultType;
		GroupID fGroup;
};

SecurityManager securityManager;

} } // namespace B::WWW2

using namespace B::WWW2;

SecurityManager::SecurityManager()
	:	fInitCount(0)
{
	fRoot = new Suffix("_root_");
}

GroupID SecurityManager::GetGroupID(const BUrl &url)
{
	BAutolock _lock(fLock.Lock());
	if (fInitCount == 0 && atomic_add(&fInitCount, 1) == 0)
		LoadSettings();

	BStringBuffer str;
	url.GetMinimalString(str);

	// Walk the suffix tree to find the group.  
	Suffix *stack[1024];
	int sp = 0;
	Suffix *current = fRoot;
	const char *c = str.String();
	for (;;) {
		Suffix *child = 0;
		if (*c != '\0') {
			for (child = current->children; child; child = child->sibling) {
				if (child->pattern.ICompare(c, child->pattern.Length()) == 0) {
					c += child->pattern.Length();
					stack[sp++] = current;
					current = child;
					break;
				}
			}
		}
			
		if (child == 0) {
			if (current->wildcard && (*c != '\0' || current->group != kInvalidGroup
				|| current->wildcard != current)) {
				if (current->wildcard == current &&	(current->children == 0
					|| *c == '\0'))
					break;	// At a wildcard leaf.  Nothing else in url matters.
				
				current = current->wildcard;
				if (*c)
					c++;
			} else {
				// We've dead-ended on a specific match, backtrack and
				// take a more general one if possible
				current = 0;
				while (--sp >= 0) {
					if (stack[sp]->wildcard) {
						current = stack[sp]->wildcard;
						break;
					}
				}

				if (current == 0)
					break;
			}
		}
	}
	
	if (current == 0)
		return kInvalidGroup;
	
	return current->group;
}

void ResourceClass::SetGroupID(GroupID groupID)
{
	fGroup = groupID;
}

bool SecurityManager::CheckAccess(const BUrl &url, GroupID requestorGroup)
{
	return CheckAccess(GetGroupID(url), requestorGroup);
}

bool SecurityManager::CheckAccess(GroupID target, GroupID requestorGroup)
{
	BAutolock _lock(fLock.Lock());
	if (fInitCount == 0 && atomic_add(&fInitCount, 1) == 0)
		LoadSettings();

	if (target == kInvalidGroup)
		return false;
		
	if (!fClasses[target])
		return false;
		
	return fClasses[target]->CheckAccess(requestorGroup);
}

GroupID SecurityManager::RegisterGroup(const char *groupName)
{
	BAutolock autoLock(fLock.Lock());
	GroupID groupID = kInvalidGroup;
	ResourceClass *rc = LookupClass(groupName);
	if (rc)
		groupID = rc->GetGroupID();
	else {
		rc = new ResourceClass(groupName);
		groupID = fClasses.AddItem(rc);
		rc->SetGroupID(groupID);
	}

	return groupID;
}

void SecurityManager::RegisterGroup(const char *urlSpec, const char *groupName)
{
	BAutolock autoLock(fLock.Lock());
	GroupID groupID = RegisterGroup(groupName);
	Suffix *current = fRoot;
	for (const char *c = urlSpec; *c;) {
		if (*c == '*') {
			while (*c == '*')
				c++;
		
			// Wildcard match.  If the current->wildcard == current, this node
			// is already "wild", thus we can use it as is.
			if (current->wildcard != current) {
				if (current->wildcard == 0)
					current->wildcard = new Suffix("*");

				if (*c == '\0')
					current->wildcard->group = groupID;
		
				current->wildcard->wildcard = current->wildcard;
				current = current->wildcard;
			}
		} else {
			// Normal match
			Suffix *child;
			for (child = current->children; child; child = child->sibling) {
				int matchLength = 0;
				int possibleLength = MIN(child->pattern.Length(), strlen(c));
				for (matchLength = 0; matchLength < possibleLength; matchLength++)
					if (child->pattern.String()[matchLength] != c[matchLength])
						break;
				
				if (matchLength == strlen(c))
					break;
				else if (matchLength == possibleLength && child->pattern.Length() < strlen(c)) {
					// The child is fully matched
					current = child;
					c += matchLength;
				} else if (matchLength > 0) {
					// Partial match, split this suffix 
					Suffix *bastardChild = new Suffix(child->pattern.String() + matchLength);
					bastardChild->children = child->children;
					bastardChild->wildcard = child->wildcard;
					child->children = bastardChild;
					child->wildcard = 0;
					child->pattern.SetTo(c, matchLength);
					c += matchLength;
					current = child;
				}
			}
		
			if (child == 0) {
				// No match, add a new child.  Make sure to stop at wildcards
				BStringBuffer tmp;
				while (*c && *c != '*')
					tmp << *c++;

				if (tmp.Length() > 0) {
					if (*c == '\0')	{
						// Completely eaten
						current->children = new Suffix(tmp.String(), groupID, current->children);
						current = current->children;
						break;
					} else {
						// Have to resolve wildcards.
						current->children = new Suffix(tmp.String(), kInvalidGroup, current->children);
						current = current->children;
					}
				}
			}
		}
	}

	current->group = groupID;
}

void SecurityManager::EnterACL(const char *resourceClass, const char *accessingGroup,
	const char *access)
{
	BAutolock autoLock(fLock.Lock());

	ResourceClass *target = LookupClass(resourceClass);
	if (target == 0) {
		printf("Warning: class %s undefined\n", resourceClass);
		return;
	}

	GroupID groupID = kInvalidGroup;
	
	if (strcasecmp(accessingGroup, "all") == 0)
		groupID = kAllGroups;
	else {
		ResourceClass *group = LookupClass(accessingGroup);
		if (group == 0) {
			printf("Warning: class %s undefined\n", accessingGroup);
			return;
		}
		
		groupID = group->GetGroupID();
	}


	if (strcasecmp(access, "grant") == 0 || strcasecmp(access, "allow") == 0)
		target->Grant(groupID);
	else if (strcasecmp(access, "deny") == 0 || strcasecmp(access, "revoke") == 0)
		target->Revoke(groupID);
	else
		printf("Unknown access type %s\n", access);
}

const char* SecurityManager::GetGroupName(GroupID group) const
{
	if (group < 0 || group > fClasses.CountItems())
		return "<invalid group>";
	return fClasses[group]->GetName();	
}

ResourceClass* SecurityManager::LookupClass(const char *name)
{ 
	for (int32 index = 0; index < fClasses.CountItems(); index++)
		if (strcasecmp(fClasses[index]->GetName(), name) == 0)
			return fClasses[index];

	return 0;
}

void SecurityManager::LoadSettings()
{
#warning "Fix me SecurityManager::LoadSettings"
//	BinderNode::property groups = BinderNode::Root()["service"]["web"]["security"]["groups"]();
//	BinderNode::iterator specIter = groups->Properties();
//	for (;;) {
//		BString spec = specIter.Next();
//		if (spec == "")
//			break;
//
//		RegisterGroup(spec.String(), groups[spec.String()].String().String());
//	}
//
//	BinderNode::property acl = BinderNode::Root()["service"]["web"]["security"]["acl"]();
//	BinderNode::iterator targetIter = acl->Properties();
//	for (;;) {
//		BString target = targetIter.Next();
//		if (target == "")
//			break;
//			
//		BinderNode::property groupObject = acl[target.String()]();
//		BinderNode::iterator groupIter = groupObject->Properties();
//		for (;;) {
//			BString group = groupIter.Next();
//			if (group == "")
//				break;	
//	
//			EnterACL(target.String(), group.String(), groupObject[group.String()]
//				.String().String());
//		}
//	}
}

void SecurityManager::PrintSuffixTree(Suffix *suffix, int level) const
{
	for (int i = 0; i < level; i++)
		printf("    ");
		
	printf("%s", suffix->pattern.String());
	if (suffix->group != kInvalidGroup)
		printf(" (%ld)\n", suffix->group);
	else
		printf("\n");
		
	for (Suffix *child = suffix->children; child; child = child->sibling)
		PrintSuffixTree(child, level + 1);

	if (suffix->wildcard == suffix) {
		for (int i = 0; i < level + 1; i++)
			printf("    ");

		printf("* <---\n");
	} else if (suffix->wildcard)
		PrintSuffixTree(suffix->wildcard, level + 1);
}

ResourceClass::ResourceClass(const char *name)
	:	fName(name),
		fLock("Resource Class Lock"),
		fAccessControlList(0),
		fDefaultType(kRevokeAccess),
		fGroup(kInvalidGroup)
{
}

ResourceClass::~ResourceClass()
{
	Clear();
}

const char* ResourceClass::GetName() const
{
	return fName.String();
}

bool ResourceClass::CheckAccess(GroupID group)
{
	BAutolock _lock(fLock.Lock());
	bool grant = (fDefaultType == kGrantAccess) ? true : false;
	for (AccessSpecifier *ac = fAccessControlList; ac; ac = ac->fNext)
		if (ac->fID == group)
			return (ac->fType == kGrantAccess);
	return grant;
}

void ResourceClass::Grant(GroupID group)
{
	BAutolock _lock(fLock.Lock());
	if (group == kAllGroups)
		fDefaultType = kGrantAccess;
	else
		Enter(group, kGrantAccess);
}

void ResourceClass::Revoke(GroupID group)
{
	BAutolock _lock(fLock.Lock());
	if (group == kAllGroups)
		fDefaultType = kRevokeAccess;
	else
		Enter(group, kRevokeAccess);
}

GroupID ResourceClass::GetGroupID() const
{
	return fGroup;
}

void ResourceClass::Print()
{
	printf("Access Control List for ResourceClass \"%s\"\n", GetName());
	for (AccessSpecifier *specifier = fAccessControlList; specifier;
		specifier = specifier->fNext)
		printf("  %ld %s\n", specifier->fID, specifier->fType == kGrantAccess ? "Grant" : "Revoke");
	
	printf("\n");
}

void ResourceClass::Clear()
{
	while (fAccessControlList) {
		AccessSpecifier *tmp = fAccessControlList;
		fAccessControlList = fAccessControlList->fNext;
		delete tmp;
	}
}

void ResourceClass::Enter(GroupID group, AccessType type)
{
	AccessSpecifier **link;
	for (link = &fAccessControlList; *link;) {
		if ((*link)->fID == group) {
			AccessSpecifier *tmp = *link;
			*link = (*link)->fNext;
			delete tmp;
		} else
			 link = &(*link)->fNext;
	}
	
	*link = new AccessSpecifier(group, type);
}
