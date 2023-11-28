#ifndef _EXTRACTUTILS_H
#define _EXTRACTUTILS_H


// ExtractUtils.h

void AppendPostfix(char *buf,
					int32 bufsiz,
					const char *postfix,
					long atIndex = -1);
					
void RenameExisting(BDirectory *destDir,
					 BEntry *item,
					 char *postfix = ".old");

long	RecursiveDelete(BEntry *ent);

// BStore *GetStore(  BDirectory *, const char *, BFile *, BDirectory *);

long SetCreationTime(BFile *fi,long time);
long SetVersion(BFile *fi,long vers);
status_t	SetEntryType(BEntry	*fi, const char *type);
status_t	GetEntryType(BEntry	*fi, char *type);
status_t	GetEntrySignature(BEntry *fi, char *sig);
#endif
