#include "UninstallThread.h"
#include "TreeView.h"

#include <Entry.h>
#include <Looper.h>

enum {
	U_PROGRESS	= 'UPrg',
	U_DONE		= 'UDon'
};

UninstallThread::UninstallThread(BHandler *creator, TreeItem *root)
	:	MThread("uninstall"),
		fTarget(creator),
		fRoot(root)
{
	Run();
}

long UninstallThread::Execute()
{
	BDirectory dir("/");
	dir.SetTo(&dir,fRoot->Label());
	RemoveFiles(fRoot, &dir);
	
	BMessage *m = new BMessage(U_PROGRESS);
	m->AddInt32("value",100);
	fTarget->Looper()->PostMessage(m,fTarget);
	fTarget->Looper()->PostMessage(U_DONE,fTarget);

	return B_NO_ERROR;
}

void UninstallThread::RemoveFiles(TreeItem *folder, BDirectory *parent)
{
	TreeItem *it = folder->Children();
	BEntry	entry;
	while (it) {
		if (it->HasChildren() || it->CanHaveChildren()) {
			BDirectory	foo(parent,it->Label());
			if (!foo.InitCheck())
				RemoveFiles(it,&foo);
		}
		entry.SetTo(parent,it->Label());
		if (!entry.InitCheck())
			entry.Remove();
			
		it = it->Sibling();
	}
	if (parent->CountEntries() == 0) {
		parent->GetEntry(&entry);
		entry.Remove();
	}
	BMessage *m = new BMessage(U_PROGRESS);
	m->AddInt32("value",1);
	fTarget->Looper()->PostMessage(m,fTarget);
}

void UninstallThread::LastCall(long)
{
	// the creator must delete me
}
