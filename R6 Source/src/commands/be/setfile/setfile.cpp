
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>
#include <pef.h>

#include <Application.h>
#include <Roster.h>
#include <File.h>
#include <ResourceFile.h>
#include <Directory.h>
#include <Volume.h>
#include <Query.h>
#include <Database.h>
#include <Debug.h>
#include <AppDefsPrivate.h>


#define		iAppType		'BAPP'
#define		iFolderType		-1
#define		iFileType  		0	

//----------------------------------------------------------------------------
// NOTE - expects file to be open when called

bool IsApp(BFile* file, long* version)
{
	Elf32_Ehdr	elfHdr;
	pef_cheader	pefHdr;
	bool		isApp;

	isApp = FALSE;

	// check for elf executable
	if (file->Read(&elfHdr, sizeof(Elf32_Ehdr)) == sizeof(Elf32_Ehdr)) {
		if (*(ulong*)elfHdr.e_ident == ElfId) {
			if (elfHdr.e_type == ET_EXEC) {
				isApp = TRUE;
				*version = elfHdr.e_version;
			}
		}
	}

	// check for pef executable
	if (!isApp) {
		file->Seek(0, SEEK_SET);
		if (file->Read(&pefHdr, sizeof(pef_cheader)) == sizeof(pef_cheader))
			if ((pefHdr.magic1 == PEF_MAGIC1) &&
				(pefHdr.magic2 == PEF_MAGIC2) &&
				(pefHdr.cont_id == PEF_CONTID)) {
					isApp = TRUE;
					*version = pefHdr.c_version;
			}
	}

	return(isApp);
}

//-------------------------------------------------------------------

BQuery*	GetDocuments(BDatabase* db, ulong signature)
{
	BQuery*	query;

	query = new BQuery();
	query->AddTree(db->FindTable("BrowserItem"));
	query->PushField("fsCreator");
	query->PushLong(signature);
	query->PushOp(B_EQ);

	query->PushField("fsType");
	query->PushLong('BAPP');
	query->PushOp(B_EQ);
	query->PushOp(B_NOT);

	query->PushOp(B_AND);
	query->Fetch();

	return(query);
}

//-------------------------------------------------------------------
record_id FindIcon(BDatabase* db, ulong creator, ulong type, bool ensureMatch)
{
	BQuery*		query;
	record_id	iconRecID;

	query = new BQuery();
	query->AddTable(db->FindTable("Icon"));
	query->PushField("creator");
	query->PushLong(creator);
	query->PushOp(B_EQ);
	query->PushField("type");
	query->PushLong(type);
	query->PushOp(B_EQ);
	query->PushOp(B_AND);
	query->FetchOne();

	if (query->CountRecordIDs())
		iconRecID = query->RecordIDAt(0);
	else
		iconRecID = 0;

	delete(query);

	// if not found using type and creator then use generics
	if ((iconRecID <= 0) && ensureMatch) {
		if ((type != iFileType) && (type != iFolderType) && (type != iAppType))
				type = iFileType;
		creator = 0;
		iconRecID = FindIcon(db, creator, type, TRUE);
	}

	return(iconRecID);
}

//-------------------------------------------------------------------
record_id AddIconToDB(BFile* file, ulong creator, ulong type,
					   void* largeBits, void* smallBits)
{
	BDatabase*	db;
	BRecord*	rec;
	record_id	iconRecID;

	if ((largeBits == NULL) || (smallBits == NULL))
		return(0);

	db = file->Record()->Database();
	ASSERT(db);

	// if icon already exists then set bits to new new bits

	if ((creator != 0) && (iconRecID = FindIcon(db, creator, type, FALSE)))
		rec = new BRecord(db, iconRecID);
	 else
		rec = new BRecord(db->FindTable("Icon"));

	rec->Lock();
	rec->SetLong("creator", creator);
	rec->SetLong("type", type);

	if (largeBits)
		rec->SetRaw("largeBits", largeBits, 1024);

	if (smallBits)
		rec->SetRaw("smallBits", smallBits, 256);

	iconRecID = rec->Commit();
	rec->Unlock();
	delete(rec);

	return(iconRecID);
}

//----------------------------------------------------------------------------

void DumpAppi(BResourceFile* file)
{
	char		name[B_FILE_NAME_LENGTH];
	long		signature;
	long		appFlags;
	long		numItems;
	long		size;
	long*		appiBuf;
	long		idx;
	long		i;
	char		sigBuf[5];

	if (file->Open(B_READ_ONLY) == B_NO_ERROR) {
		if (appiBuf = (long*)file->FindResource('APPI', "app info", &size)) {
			idx = 0;
			signature = appiBuf[idx++];
			appFlags = appiBuf[idx++];
			numItems = appiBuf[idx++];

			memcpy(sigBuf, &signature, 4);
			sigBuf[4] = 0;
			printf("signature = %s\n", sigBuf);
			printf("appFlags = %lx\n", appFlags);
			switch (appFlags & B_LAUNCH_MASK) {
				case B_SINGLE_LAUNCH:		printf("SINGLE_LAUNCH ");		break;
				case B_MULTIPLE_LAUNCH:		printf("MULTIPLE_LAUNCH ");		break;
				case B_EXCLUSIVE_LAUNCH:	printf("EXCLUSIVE_LAUNCH ");	break;
				default:					printf("Bad launch value! ");	break;
			}
			if (appFlags & 	B_BACKGROUND_APP)
				printf("BACKGROUND_APP ");
			if (appFlags & 	B_ARGV_ONLY)
				printf("ARGV_ONLY ");
			printf("\n");
			printf("number of document types = %d\n", numItems);

			// add all icons for this application to database
			for (i = 0; i < numItems; i++) {
				memcpy(sigBuf, &(appiBuf[idx++]), 4);
				sigBuf[4] = 0;
				printf("document %d : %s\n", i + 1, sigBuf);
			}

			free(appiBuf);
		} else {
			file->GetName(name);
			printf("%s has no APPI resource\n", name);
		}

		file->Close();
	} else
		printf("error open resoure file\n");

}

//----------------------------------------------------------------------------

void HandleFile(BResourceFile* file)
{
	char		name[B_FILE_NAME_LENGTH];
	char		iconName[9];
	long		version;
	long		numItems;
	ulong		signature;
	ulong		fileType;
	long		appFlags;
	BRecord*	rec;
	BQuery*		query;
	BDatabase*	db;
	record_id	iconRecID;
	long*		appiBuf;
	void*		iconPtr;
	void*		micnPtr;
	long		size;
	long		i;
	long		idx;

	version = 0;

	// is it a resource file? if not, skip it.
	if (file->Open(B_READ_ONLY) != B_NO_ERROR)
		return;

	if (IsApp(file, &version)) {

		file->GetName(name);

		// default app flags for apps without resources (or incorrect ones)
		appFlags = DEFAULT_APP_FLAGS;
		signature = 0;

		if (appiBuf = (long*)file->FindResource('APPI', "app info", &size)) {

			idx = 0;
			signature = appiBuf[idx++];
			appFlags = appiBuf[idx++];
			numItems = appiBuf[idx++];
			fileType = 'BAPP';

			if (signature == 0)
				printf("WARNING: no application signature for '%s'.\n",
						name);

			// add all icons for this application to database
			for (i = 0; i <= numItems; i++) {
				memcpy(iconName, &fileType, 4);
				iconName[4] = 0;

				iconPtr = file->FindResource('ICON', iconName, &size);
				micnPtr = file->FindResource('MICN', iconName, &size);
				AddIconToDB(file, signature, fileType, iconPtr, micnPtr);

				free(iconPtr);
				free(micnPtr);

				if (i < numItems)
					fileType = appiBuf[idx++];
			}

			free(appiBuf);
		}
				
		// set info for app
		file->Record()->SetLong("version", version);	// from data fork
		file->Record()->SetLong("appFlags", appFlags);
		file->Record()->Commit();
		if (file->SetTypeAndApp('BAPP', signature) != B_NO_ERROR) {
			printf("error setting Type/Creator on file %s\n", name);
		}

		// set iconref for app
		db = file->Record()->Database();
		iconRecID = FindIcon(db, signature, 'BAPP', TRUE);
		file->Record()->SetRecordID("iconRef", iconRecID);
		file->Record()->Commit();

		// connect all db documents with their icons now
		if (signature != 0) {
			query = GetDocuments(db, signature);
			numItems = query->CountRecordIDs();

			for (i = 0; i < numItems; i++) {
				rec = new BRecord(db, query->RecordIDAt(i));
				rec->Lock();
				iconRecID = FindIcon(db, signature, rec->FindInt32("fsType"), TRUE);
				rec->SetRecordID("iconRef", iconRecID);
				rec->Commit();
				rec->Unlock();
				delete(rec);
			}

			delete(query);
		}
	}
	file->Close();
}

//----------------------------------------------------------------------------

void RecursiveSet(BDirectory* dir)
{
	BResourceFile		file;
	BDirectory			child;
	long				i;

	i = 0;
	while (dir->GetFile(i++, &file) == B_NO_ERROR)
		HandleFile(&file);

	i = 0;
	while (dir->GetDirectory(i++, &child) == B_NO_ERROR)
		RecursiveSet(&child);
}

//----------------------------------------------------------------------------

long GetBFile(const char *path, BFile *file)
{
	record_ref	ref;

	if (get_ref_for_path(path, &ref) != B_NO_ERROR)
		return B_FILE_NOT_FOUND;		

	return file->SetRef(ref);
}

//====================================================================

class TSetfileApp : public BApplication {

public:
					TSetfileApp();
virtual		void	ArgvReceived(int argc, char** argv);
virtual		void	ReadyToRun();

			void	Do();

			int		fArgc;
			char	**fArgv;
};

int main()
{	
	BApplication* myApp = new TSetfileApp();
	myApp->Run();

	delete myApp;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TSetfileApp::TSetfileApp()
		  :BApplication("application/x-vnd.Be-cmd-STFL")
{
	fArgc = 0;
}

//--------------------------------------------------------------------

void TSetfileApp::ArgvReceived(int argc, char** argv)
{
	int		i;

	fArgc = argc;
	fArgv = (char **) malloc((argc+1) * sizeof(void *));
	for(i=0; i<argc; i++) {
		fArgv[i] = (char *) malloc(strlen(argv[i]) + 1);
		strcpy(fArgv[i], argv[i]);
	}
	fArgv[i] = NULL;
}

void TSetfileApp::Do()
{
	BVolume			volume;
	BResourceFile	file;
	BDirectory		root;
	long			i;
	
	switch (fArgc) {
		case 2:
			if (strcmp(fArgv[1], "-r") == 0) {
				printf("setfile - setting files for entire hierarchy...\n");
				for(i=0; volume = volume_at(i), volume.Error() == B_NO_ERROR; i++) {
					volume.GetRootDirectory(&root);
					ASSERT(root.Error() == B_NO_ERROR);
					RecursiveSet(&root);
				}
				printf("setfile - done.\n");
			} else {
				if (GetBFile(fArgv[1], &file) == B_NO_ERROR)
					HandleFile(&file);
				else
					goto usage;
			}
			break;

		case 3:
			if (strcmp(fArgv[2], "-d") == 0) {
				if (GetBFile(fArgv[1], &file) == B_NO_ERROR)
					DumpAppi(&file);
				else
					goto usage;
			} else
				goto usage;
			break;

		default:
			goto usage;
	}
	return;

usage:
	printf("usage: setfile <filename>\nusage: setfile -r\nusage: setfile <filename> -d\n");
	return;
}

//--------------------------------------------------------------------

void TSetfileApp::ReadyToRun()
{
	Do();
	PostMessage(B_QUIT_REQUESTED);
}
