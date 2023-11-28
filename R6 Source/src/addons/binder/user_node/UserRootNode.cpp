#include <OS.h>
#include <stdlib.h>
#include <stdio.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <kernel/image.h>
#include "UserRootNode.h"
#include "UserNode.h"

class Boolean {
	bool 	value;

	public:

						Boolean() { value = false; };
						Boolean(const bool &b) { value = b; };

						operator bool() const { return value; };
		const Boolean &	operator =(const bool &b) { value = b; return *this; };
};

BString empty;

class DirectoryListing : public AssociativeArray<BString,Boolean>
{
	private:

		int32				m_index;

	public:

		DirectoryListing() { m_index = 0; };
		
		BString& Next()
		{
			m_lock.Lock();
			BString &str = (m_index < m_list.CountItems()) ? (*this)[m_index++].key : empty;
			m_lock.Unlock();
			return str;
		}

};

void create_path(BPath &path)
{
	BPath parent;
	path.GetParent(&parent);
	if (parent.InitCheck()) return;

	create_path(parent);

	BDirectory dir(parent.Path()),other;
	if (dir.InitCheck()) return;
	dir.CreateDirectory(path.Leaf(),&other);
}

UserRootNode::UserRootNode()
{
	BPath path("/boot/binder/user/system");
	create_path(path);

	BDirectory dir("/boot/binder/user/system");
	m_defaultUser = new UserNode("system",dir);
	m_currentUserLink = new BinderNode;
	m_currentUserLink->InheritFrom(m_defaultUser);
}

status_t 
UserRootNode::OpenProperties(void **cookie, void *)
{
	DirectoryListing *list = new DirectoryListing;
	list->Insert("~",true);
	list->Insert("system",true);
	BDirectory dir(BINDER_USER_ROOT);
	BEntry entry;
	BPath path;
	while (dir.GetNextEntry(&entry,true) == B_OK) {
		if (entry.IsDirectory()) {
			entry.GetPath(&path);
			list->Insert(path.Leaf(),true);
		}
	}
	*cookie = list;
	return B_OK;
}

status_t 
UserRootNode::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	DirectoryListing *list = (DirectoryListing*)cookie;
	BString &name = list->Next();
	if (name.Length() == 0) return ENOENT;
	strncpy(nameBuf,name.String(),*len);
	*len = name.Length();
	return B_OK;
}

status_t 
UserRootNode::CloseProperties(void *cookie)
{
	DirectoryListing *list = (DirectoryListing*)cookie;
	delete list;
	return B_OK;
}

put_status_t 
UserRootNode::WriteProperty(const char *name, const property &prop)
{
	if (!strcmp("~",name)) {
		property realUser;
		BString theName;
		if (prop.String() == "~") return B_BAD_VALUE;
		printf("URN::WP '%s' => '%s'\n",name,prop.String().String());
		if (prop.IsUndefined()) {
			printf("URN::WP new user is undefined, setting to 'system'\n");
			theName = "system";
		}
		else theName = prop.String();
		status_t err = ReadProperty(theName.String(),realUser);
		if (!err && realUser.IsObject()) {
			m_currentUserLink->RenounceAncestry();
			m_currentUserLink->InheritFrom(realUser.Object());
			NotifyListeners(B_PROPERTY_CHANGED,"~");
			return B_OK;
		}
		return err;
	}
	return EPERM;
}

status_t simple_exec(int32 argc, const char **argv)
{
	printf("exec: ");
	for (int32 i=0;i<argc;i++) {
		printf("%s ",argv[i]);
	}
	printf("\n");

	int oldStdin = dup(STDIN_FILENO);
	int oldStdout = dup(STDOUT_FILENO);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);

	status_t err;
	thread_id spawned = load_image(argc,argv,(const char**)environ);
	resume_thread(spawned);
	wait_for_thread(spawned,&err);

	dup2(oldStdin,STDIN_FILENO);
	dup2(oldStdout,STDOUT_FILENO);
	close(oldStdin);
	close(oldStdout);

	return err;
}

int rm(const char *fn)
{
	const char *argv[5];
	argv[0] = "/bin/rm";
	argv[1] = "-rf";
	argv[2] = fn;
	return simple_exec(3,argv);
}

bool isdir(const char *pathname)
{
	BDirectory dir(pathname);
	return (dir.InitCheck() == 0);
}

get_status_t 
UserRootNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	BString tmp;
	if (!strcmp("addUser",name)) {
		BDirectory dir,newDir;
		if (args.Count() < 1) return B_BAD_VALUE;
		BString newName = args[0];
		if ((newName == "~") || (newName == "system")) return B_BAD_VALUE;
		BString username(BINDER_USER_ROOT);
		username << "/";
		username << newName;
		dir.SetTo(username.String());
		if (!dir.InitCheck()) return B_NAME_IN_USE;
		dir.SetTo(BINDER_USER_ROOT);
		dir.CreateDirectory(newName.String(),&newDir);
		if (newDir.InitCheck()) return newDir.InitCheck();
		tmp = args[0];
		name = tmp.String();
		NotifyListeners(B_PROPERTY_ADDED, name);
	} else if (!strcmp("deleteUser",name)) {
		BDirectory dir;
		BEntry entry;
		if (args.Count() < 1) return B_BAD_VALUE;
		BString newName = args[0];

		if (m_currentUserLink->Property("name") == newName) {
			m_currentUserLink->RenounceAncestry();
			m_currentUserLink->InheritFrom(m_defaultUser);
			printf("URN::deleteUser: trying to delete current user; setting default user to 'system'\n");
			NotifyListeners(B_PROPERTY_CHANGED,"~");
		}

		{
			m_activeUserNodes.Lock();
			printf("URN::deleteUser: deleting node for user '%s'\n", newName.String());
			binder_node node = m_activeUserNodes.Lookup(newName);
			if (node) {
				node->ClearMessages();
				
				XMLBinderNode *xnode = dynamic_cast<XMLBinderNode*>((BinderNode*)node);
				if (xnode) xnode->SetPathname("/dev/null");
					// HACK: We set the XML backing-store pathname for
					// the dying usernode to an invalid location.  This
					// way, if clients with stale references into the
					// user node attempts to flush XML data back to disk
					// (bookmarks plugin, etc.), they will not end up
					// re-creating files in /boot/binder/user/foo.
					// NB: This will not repair clients that store their
					// own XML beneath /boot/user/foo (PostOffice in
					// libwww for example).
					//                                 -dsandler 4/23/01
					
				m_activeUserNodes.Remove(newName);
			}
			m_activeUserNodes.Unlock();
		}
		
		BString username(BINDER_USER_ROOT);
		username << "/";
		username << newName;
		if (isdir(username.String())) {
			if (!rm(username.String()) && !isdir(username.String())) {
				prop = "ok";
				NotifyListeners(B_PROPERTY_REMOVED, newName.String());
				return B_OK;
			}
			prop = "unknown error";
			return B_OK;
		}
		prop = "bad username";
		return B_OK;
	} else if (!strcmp("renameUser",name)) {
		if (args.Count() < 2) return B_BAD_VALUE;
		BString was = args[0].String();
		BString is = args[1].String();
		BPath wasPath(BINDER_USER_ROOT,was.String());
		BPath isPath(BINDER_USER_ROOT,is.String());

		m_activeUserNodes.Lock();

		printf("rename(%s,%s)\n",wasPath.Path(),isPath.Path());
		if (rename(wasPath.Path(),isPath.Path())) {
			m_activeUserNodes.Unlock();
			return B_BAD_VALUE;
		}

		binder_node node = m_activeUserNodes.Lookup(was);
		if (node) {
			m_activeUserNodes.Remove(was);
			UserNode *unode = dynamic_cast<UserNode*>(((BinderNode*)node));
			if (unode) unode->SetName(is.String());
			m_activeUserNodes.Insert(is,node);
		}

		m_activeUserNodes.Unlock();

		NotifyListeners(B_PROPERTY_ADDED, is.String());
		NotifyListeners(B_PROPERTY_REMOVED, was.String());
		tmp = is;
		name = tmp.String();
	} else if (!strcmp("~",name)) {
		prop = m_currentUserLink;
		return B_OK;
	} else if (!strcmp("system",name)) {
		prop = m_defaultUser;
		return B_OK;
	}
	
	m_activeUserNodes.Lock();

	binder_node node = m_activeUserNodes.Lookup(name);
	if (!node) {
		BString username(BINDER_USER_ROOT);
		username << "/";
		username << name;
		BDirectory dir(username.String());
		if (dir.InitCheck()) {
			m_activeUserNodes.Unlock();
			return dir.InitCheck();
		}
		node = new UserNode(name,dir);
		m_activeUserNodes.Insert(name,node);
	}

	prop = node;
	m_activeUserNodes.Unlock();
	
	return node?B_OK:ENOENT;
}

extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new UserRootNode();
}
