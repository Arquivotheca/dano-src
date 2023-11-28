#include "utils.h"

void dirname(const char *path, char *newpath)
{
  	char *slash;
  	int length;

  	slash = strrchr (path, '/');
	if (slash == 0){
      	path = ".";
      	length = 1;
	} else {
      while (slash > path && *slash == '/')
        --slash;

      length = slash - path + 1;
    }
  	strncpy (newpath, path, length);
  	newpath[length] = 0;
}

void path_from_entry_ref(const entry_ref *ref,char *p)
{
	BPath  		path;
	BEntry 		entry(ref);
	//
	//	perform validity checks
	//	
	if (entry.InitCheck() == B_NO_ERROR) {
		if (entry.GetPath(&path) == B_NO_ERROR)
			strcpy(p,path.Path());
		else
			p[0] = 0;
	} else
		p[0] = 0;
}

bool TraverseSymLink(entry_ref *ref)
{
	BEntry e(ref,false);
	
	if (e.IsSymLink()) {
		BEntry f(ref,true);
		f.GetRef(ref);
		return true;
	} else {
		return false;
	}
}

void What(char *msg)
{
	(new BAlert("What?", msg, "Cancel"))->Go();
}
