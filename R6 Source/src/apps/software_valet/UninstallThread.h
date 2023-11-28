#ifndef _UNINSTALLEDTHREAD_H_
#define _UNINSTALLEDTHREAD_H_


// UninstallThread.h
#include "MThread.h"
#include <Directory.h>
#include <Handler.h>

class TreeItem;


class UninstallThread : public MThread
{
public:
	UninstallThread(BHandler *creator, TreeItem *root);
	
	virtual long	Execute();
	virtual void	LastCall(long);
	
			void	RemoveFiles(TreeItem *folder, BDirectory *parent);
private:
	BHandler	*fTarget;
	TreeItem	*fRoot;
};

#endif
