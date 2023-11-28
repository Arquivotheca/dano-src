#include <Be.h>
// create a full pathname
long CreateFullPath(const char *path, BEntry *final);

// create a path relative to dir
long CreatePathAt(const BDirectory *dir, const char *path, BEntry *final);

// create a full pathname
long CreateFullPath(const char *path, BEntry *final)
{
	status_t err;
	BDirectory rootDir("/");
	err = rootDir.InitCheck();
	if (err < B_NO_ERROR)
		return err;
	return CreatePathAt(&rootDir,path,final);
}

// create a path relative to dir
// set final to the final path
long CreatePathAt(const BDirectory *dir,
				  const char *path,
				  BEntry *final)
{
	long err;
	BDirectory curDir(*dir);
	BEntry entry;
	char dname[B_FILE_NAME_LENGTH];

	const char *c = path;
	while(*c) {
		// skip leading slahes
		while (*c && *c == '/')
			c++;
			
		char *dst = dname;
		while (*c && *c != '/' && dst < dname+B_FILE_NAME_LENGTH-1) {
			*dst++ = *c++;
		}
		*dst = 0;
		// null name
		if (!*dname)
			break;
			
		BDirectory	pDir(curDir);
		
		if (pDir.FindEntry(dname,&entry) < B_NO_ERROR) {
			// couldn't get the entry
			err = pDir.CreateDirectory(dname, &curDir);
			if (err < B_NO_ERROR) {
				return err;
			}
			curDir.GetEntry(&entry);
		}
		else {
			// got the entry
			err = curDir.SetTo(&entry);
			if (err < B_NO_ERROR)
				return err;
		}
	}
	*final = entry;
	return B_NO_ERROR;
}				  

// call find directory
// but strip out the first component of the path
const char 	*root_find_directory(directory_which	which,
	BPath			*path);

const char 	*root_find_directory(directory_which	which,
	BPath			*path)
{
	status_t err;
	err = find_directory(which, path, false);
	if (err < B_OK) return NULL;
		
	const char *c = path->Path();
	
	while(*c && *c == '/')
		c++;
	while(*c && *c != '/')
		c++;
	while(*c && *c == '/')
		c++;
	
	return c;
}
