#include "DestManager.h"
#include "DestinationList.h"
#include "InstallerType.h"
#include "PTreeItem.h"

#include <Path.h>
#include <FindDirectory.h>
#include "RList.h"
#include "Util.h"

#include "MyDebug.h"

#include <string.h>

#if (!SEA_INSTALLER)


const char 	*root_find_directory(directory_which	which,
	BPath			*path);

PTreeItem *AddFilePath(const char *path, TreeItem *parent);


PTreeItem *DestManager::TreeItemFor(int32 destIndex,
									PTreeItem *parentItem,
									TreeItem *root,
									bool custom)
{
	DestItem	*di = NULL;
	PTreeItem	*result;

	if (custom) {
		PRINT(("preview custom path\n"));
		di = fCustList->ItemAt(destIndex);
		
		if (!di) doError(errBADDESTLIST);
		// has full pathname/query
	}
	else {
		if (destIndex == D_PARENT_FOLDER) {
			//PRINT(("parent folder\n"));
			result = parentItem;
			return result;
		}
		else if (destIndex == D_INSTALL_FOLDER) {
			PRINT(("preview install folder\n"));
			// special item for install folder
			if (!instFolderTreeItem) {
				// if this item hasn't been created
				if (dirName) {
					// relative to the install volume
					BEntry rootEnt;
					rootDir.GetEntry(&rootEnt);
					
					BPath	thePath;
					rootEnt.GetPath(&thePath);
					thePath.Append(dirName);
					/***
					if (len > B_NO_ERROR) {
						char *c = &buf[len-1];
						// pointing at the null
						if (c < buf+PATH_MAX-1)
							*c++ = '/';
							
						char *src = dirName;
						while (*src && c < buf+PATH_MAX-1)
							*c++ = *src++;
						*c = 0;
					}
					***/
					instFolderTreeItem = AddFilePath(thePath.Path(),root);
				}
				else if (fInstall) {
					BEntry	installEnt;
					fInstall->GetEntry(&installEnt);
					
					// char buf[PATH_MAX];
					BPath	thePath;
					installEnt.GetPath(&thePath);
					PRINT(("preview install path is %s\n",thePath.Path()));
					instFolderTreeItem = AddFilePath(thePath.Path(),root);
				}
			}
			return instFolderTreeItem;
		}
		else {
			PRINT(("preview custom path\n"));
			di = fDefList->ItemAt(destIndex);
			if (!di) doError(errBADDESTLIST);
		}
	}
	
	/*
	if (di && di->treeItem) {
		PRINT(("tree item already initialized, di->dir is %s\n",di->dir));
		result = di->treeItem;
	}*/
	// slow for many items in custom folders
	if (di) {
		if (di->findCode >= 0) {
			PRINT(("preview find directory\n"));
			// find directory call
			BPath		tpath;
			const char	*path;
			path = root_find_directory((directory_which)di->findCode,&tpath);
			if (path) {
				BPath		thePath(&rootDir,path);
				return AddFilePath(thePath.Path(),root);
			}
			return NULL;
		}
		else if (di->path[0] == '/') {
			PRINT(("preview full path\n"));
			// can check if error and can't find start to path
			return AddFilePath(di->path,root);
		}
		else {
			PRINT(("preview partial path\n"));
			// AppendPath(&rootDir,di->path,buf,PATH_MAX);
			BPath		thePath(&rootDir,di->path);
			return AddFilePath(thePath.Path(),root);
		}
	}
	return NULL;
}


PTreeItem *AddFilePath(const char *path, TreeItem *parent)
{
	char buf[B_FILE_NAME_LENGTH];
	const char *s;
	char *d;
	TreeItem *tItem = NULL;
	s = path;
	while (*s) {	
		tItem = parent->Children();
		d = buf;
		while(*s && *s != '/') {
			*d++ = *s++;
		}
		*d = 0;
		
		PRINT(("BUF IS %s\n",buf));
		
		// either null or / was encountered
		// skip slash if not at end
		if (*s) {
			*s++;
		}
		if (!(*buf))
			continue;
			
		// now loop to find the child
		while (tItem) {
			if (strcmp(tItem->Label(),buf) == 0) {
				break;
			}
			tItem = tItem->Sibling();
		}
		if (!tItem) {
			// wasn't found
			tItem = new PTreeItem(buf,gGenericFolderSIcon);
			parent->AddChild(tItem);
		}
		parent = tItem;
	}
	return (PTreeItem *)tItem;
}

#endif
