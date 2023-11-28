#include <Be.h>
// ExtractUtils.cpp

#include "ExtractUtils.h"

#include "MyDebug.h"
#include <NodeInfo.h>
#include "Util.h"

void AppendPostfix(char *buf, int32 bufsiz, const char *postfix, long atIndex)
{
	long len;
	if (atIndex < 0)
		len = strlen(buf);
	else
		len = atIndex;
		
	long postfixLen = strlen(postfix) + 1;
	
	if (len > bufsiz - postfixLen) {
		memcpy(buf+bufsiz-postfixLen,postfix,postfixLen);
	}
	else {
		memcpy(buf+len,postfix,postfixLen);
	}
}


void RenameExisting(BDirectory *destDir, BEntry *item, char *postfix)
{
	if (!item)
		return;
			
	char buf[B_FILE_NAME_LENGTH];
	item->GetName(buf);

	AppendPostfix(buf,B_FILE_NAME_LENGTH,postfix);

	PRINT(("NEW NAME IS: %s\n",buf));
	if (destDir->Contains(buf) ){
		PRINT(("trying to find unused name"));
		long appendLoc = strlen(buf);
		long i;
		for( i = 1; i < LONG_MAX; i++) {
			char tail[32];
			sprintf(tail,"%d",postfix,i);
			AppendPostfix(buf,B_FILE_NAME_LENGTH,tail,appendLoc);
			if (destDir->Contains(buf) == FALSE)
				break;	
		}
		if (i == LONG_MAX)
			doError("Could not create a unique filename.");
	}
	item->Rename(buf);
}

status_t	SetEntryType(BEntry	*fi, const char *type)
{
	BNode		node(fi);
	BNodeInfo	nodeInf(&node);
	
	return nodeInf.SetType(type);
}

status_t	GetEntryType(BEntry *fi, char *type)
{
	BNode		node(fi);
	BNodeInfo	nodeInf(&node);
	
	return nodeInf.GetType(type);
}

status_t	GetEntrySignature(BEntry *fi, char *sig)
{
	BNode		node(fi);
	
	node.Lock();
	attr_info	ainfo;
	
	if (node.GetAttrInfo("BEOS:APP_SIG", &ainfo) == B_NO_ERROR) {
		node.ReadAttr("BEOS:APP_SIG", ainfo.type, 0, sig, ainfo.size);
	}
	else
		*sig = 0;
	
	node.Unlock();
	return B_NO_ERROR;
}

// delete everything recursively
long	RecursiveDelete(BEntry *ent)
{
	if (ent->IsDirectory()) {
		// directory
		BDirectory	dir(ent);
		dir.Rewind();
		BEntry dEnt;
		while(dir.GetNextEntry(&dEnt) >= B_NO_ERROR) {
			RecursiveDelete(&dEnt);
		}
		return ent->Remove();
	}
	else {
		// file
		return ent->Remove();
	}
}
