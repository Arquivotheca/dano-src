// This file contains copy-pasted cruft that were needed to build
// installer independently of the BeOS build system.
// Care should be taken to make sure the structures in here are not out of date

#include <Alert.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <Directory.h>
#include <InterfaceDefs.h>
#include <NodeInfo.h>
#include <Path.h>
#include <String.h>
#include <SymLink.h>
#include <TypeConstants.h>
#include <Node.h>
#include <Volume.h>
#include <Window.h>

#include <ctype.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "InternalGlue.h"

const char *folderErrorString = "Error copying folder \"%s\". Would you like to continue?";
const char *noFreeSpace = "Sorry, there is not enough free space on the destination "
	"volume to copy the selection.";
const char *fileErrorString = "Error copying file \"%s\". Would you like to continue?";

#define	B_LINK_MIMETYPE		"application/x-vnd.Be-symlink"

enum {
	USER_CANCELED = B_ERRORS_END + 1,
	COPY_CANCELED = USER_CANCELED,
	TRASH_CANCELED
};

enum ConflictCheckResult {
	kCanceled = USER_CANCELED,
	kPrompt,
	kReplace,
	kReplaceAll,
	kNoConflicts
};

namespace BPrivate {

extern "C" int _kstatfs_(dev_t dev, long *pos, int fd, const char *path,
	struct fs_info *fs);

void 
AddVirtualFileSystems(DeviceList *list)
{
	fs_info info;
	int32 cookie = 0;
	while (!_kstatfs_(-1, &cookie, -1, NULL, &info)) {
		
		if ((info.flags & B_FS_IS_PERSISTENT) == 0)
			// only interrested in persistent volumes
			continue;

		if (strncmp(info.device_name, "/dev/", 5) == 0)
			// only interrested in volumes not mounted in the dev tree
			continue;

		node_ref node;
		node.device = info.dev;
		node.node = info.root;

		BDirectory dir(&node);
		BEntry entry;
		BPath mount;

		status_t err = dir.SetTo(&node);
		if (err != B_OK)
			continue;
		
		err = dir.GetEntry(&entry);
		if (err != B_OK)
			continue;

		err = entry.GetPath(&mount);
		if (err != B_OK)
			continue;

		char name[256];
		BVolume volume(info.dev);
		if (volume.GetName(name) != B_OK)
			continue;

		Device *device = new Device();
		Session *session = new Session(device, mount.Path(), 0, info.total_blocks, true);
		Partition *partition = new Partition(session, name, "bfs",
			"bfs", "bfs", name, mount.Path(), info.block_size,
			0, info.total_blocks, false);

		partition->SetVolumeDeviceID(info.dev);
		partition->SetMountState(kMounted);

		session->AddPartition(partition);
		device->AddSession(session);
		list->AddItem(device);
	}
}


ReadAttrResult ReadAttr(const BNode *, const char *hostAttrName, const char *foreignAttrName,
	type_code , off_t , void *, size_t , void (*swapFunc)(void *) = 0,
	bool isForeign = false);

}

// Skip these attributes when copying in Tracker
const char *kSkipAttributes[] = {
	kAttrPoseInfo,
	NULL
};

_CopyLoopControl::~_CopyLoopControl()
{
}

bool 
TrackerCopyLoopControl::FileError(const char *message, const char *name,
	bool allowContinue)
{
	char buf[500];
	sprintf(buf, message, name);
	
	if (allowContinue) 
		return (new BAlert("", buf, "Cancel", "OK", 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go() != 0;

	(new BAlert("", buf, "Cancel", 0, 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
	return false;
}

void 
TrackerCopyLoopControl::UpdateStatus(const char */*name*/, entry_ref, int32 /*count*/, 
	bool /*optional*/)
{
//	if (gStatusWindow && gStatusWindow->HasStatus(fThread))
//		gStatusWindow->UpdateStatus(fThread, const_cast<char *>(name),  
//			count, optional);
}

bool 
TrackerCopyLoopControl::CheckUserCanceled()
{
//	return gStatusWindow && gStatusWindow->CheckCanceledOrPaused(fThread);
	return false;
}

TrackerCopyLoopControl::OverwriteMode
TrackerCopyLoopControl::OverwriteOnConflict(const BEntry *, const char *, 
	const BDirectory *, bool, bool)
{
	return kReplace;
}

bool 
TrackerCopyLoopControl::SkipEntry(const BEntry *, bool)
{
	// tracker makes no exceptions
	return false;
}

bool 
TrackerCopyLoopControl::SkipAttribute(const char *attributeName)
{
	for (const char **skipAttribute = kSkipAttributes; *skipAttribute;
		skipAttribute++) {
		if (strcmp(*skipAttribute, attributeName) == 0)
			return true;
	}

	return false;
}

void 
_CopyLoopControl::ChecksumChunk(const char *, size_t)
{
}

bool 
_CopyLoopControl::ChecksumFile(const entry_ref *)
{
	return true;
}

bool _CopyLoopControl::SkipAttribute(const char*)
{
	return false;
}

bool _CopyLoopControl::PreserveAttribute(const char*)
{
	return false;
}

class PoseInfo {
public:
	static void EndianSwap(void *castToThis);
	void PrintToStream();
	
	bool invisible;
	ino_t inited_dir;
	BPoint location;	
};

void 
PoseInfo::EndianSwap(void *castToThis)
{
	PoseInfo *self = (PoseInfo *)castToThis;

	PRINT(("swapping PoseInfo\n"));

//	ASSERT(sizeof(ino_t) == sizeof(int64));
	self->inited_dir = B_SWAP_INT64(self->inited_dir);
	swap_data(B_POINT_TYPE, &self->location, sizeof(BPoint), B_SWAP_ALWAYS);
	
	// do a sanity check on the icon position
	if (self->location.x < -20000 || self->location.x > 20000
		|| self->location.y < -20000 || self->location.y > 20000) {
		// position out of range, force autoplcemement
		PRINT((" rejecting icon position out of range\n"));
		self->inited_dir = -1LL;
		self->location = BPoint(0, 0);
	}
}

void 
PoseInfo::PrintToStream()
{
	PRINT(("%s, inode:%Lx, location %f %f\n", invisible ? "hidden" : "visible",
		inited_dir, location.x, location.y));
}

namespace BPrivate {

ReadAttrResult
ReadAttr(const BNode *node, const char *hostAttrName, const char *foreignAttrName,
	type_code type, off_t offset, void *buffer, size_t length,
	void (*swapFunc)(void *), bool isForeign)
{
	if (!isForeign && node->ReadAttr(hostAttrName, type, offset, buffer, length) == (ssize_t)length)
		return kReadAttrNativeOK;

	// PRINT(("trying %s\n", foreignAttrName));
	// try the other endianness	
	if (node->ReadAttr(foreignAttrName, type, offset, buffer, length) != (ssize_t)length)
		return kReadAttrFailed;
	
	// PRINT(("got %s\n", foreignAttrName));
	if (!swapFunc)
		return kReadAttrForeignOK;

	(swapFunc)(buffer);
		// run the endian swapper

	return kReadAttrForeignOK;
}


static bool
FSGetPoseLocation(const BNode *node, BPoint *point)
{
	PoseInfo poseInfo;
	if (ReadAttr(node, kAttrPoseInfo, kAttrPoseInfoForeign,
		B_RAW_TYPE, 0, &poseInfo, sizeof(poseInfo), &PoseInfo::EndianSwap)
		== kReadAttrFailed)
		return false;
	
	if (poseInfo.inited_dir == -1LL)
		return false;

	*point = poseInfo.location;

	return true;
}

status_t
FSSetPoseLocation(ino_t destDirInode, BNode *destNode, BPoint point)
{
	PoseInfo poseInfo;
	poseInfo.invisible = false;
	poseInfo.inited_dir = destDirInode;
	poseInfo.location = point;

	status_t result = destNode->WriteAttr(kAttrPoseInfo, B_RAW_TYPE, 0,
		&poseInfo, sizeof(poseInfo));
		
	if (result == sizeof(poseInfo))
		return B_OK;
	
	return result;
}

status_t
FSSetPoseLocation(BEntry *entry, BPoint point)
{
	BNode node(entry);
	status_t result = node.InitCheck();
	if (result != B_OK)
		return result;
	
	BDirectory parent;
	result = entry->GetParent(&parent);
	if (result != B_OK)
		return result;
	
	node_ref destNodeRef;
	result = parent.GetNodeRef(&destNodeRef);
	if (result != B_OK)
		return result;
	
	return FSSetPoseLocation(destNodeRef.node, &node, point);
}

static void
SetUpPoseLocation(ino_t sourceParentIno, ino_t destParentIno,
	const BNode *sourceNode, BNode *destNode, BPoint *loc)
{
	BPoint point;
	if (!loc
		// we don't have a position yet
		&& sourceParentIno != destParentIno
		// we aren't  copying into the same directory
		&& FSGetPoseLocation(sourceNode, &point))
		// the original has a valid inited location
		loc = &point;
		// copy the originals location

	if (loc && loc != (BPoint *)-1) {
		// loc of -1 is used when copying/moving into a window in list mode
		// where copying positions would not work
		// ToSo:
		// should push all this logic to upper levels
		FSSetPoseLocation(destParentIno, destNode, *loc);
	}
}

static void
CopyAttributes(_CopyLoopControl *control, BNode *srcNode, BNode *destNode, void *buf,
	size_t bufsize)
{
	// ToDo:
	// Add error checking

	// When calling CopyAttributes on files, have to make sure destNode
	// is a BFile opened R/W
	
	srcNode->RewindAttrs();
	char name[256];
	while (srcNode->GetNextAttrName(name) == B_OK) {
		// Check to see if this attribute should be skipped.
		if (control->SkipAttribute(name))
			continue;

		attr_info info;
		if (srcNode->GetAttrInfo(name, &info) != B_OK)
			continue;

		// Check to see if this attribute should be overwritten when it
		// already exists.
		if (control->PreserveAttribute(name)) {
			attr_info dest_info;
			if (destNode->GetAttrInfo(name, &dest_info) == B_OK)
				continue;
		}

		ssize_t	 bytes;
		ssize_t numToRead = info.size;
		for (off_t offset = 0; numToRead > 0; offset += bytes) {
			ssize_t chunkSize = numToRead;
			if (chunkSize > (ssize_t)bufsize)
				chunkSize = bufsize;

			bytes = srcNode->ReadAttr(name, info.type, offset,
				buf, chunkSize);

			if (bytes <= 0) 
				break;
			
			destNode->WriteAttr(name, info.type, offset, buf, bytes);

			numToRead -= bytes;
		}
	}
}


status_t
FSCopyFolder(BEntry *srcEntry, BDirectory *destDir, _CopyLoopControl *loopControl,
	BPoint *loc, bool makeOriginalName)
{	
	BDirectory	newDir;
	BDirectory	srcDir;
	char		destName[B_FILE_NAME_LENGTH];
	thread_id	thread;
	entry_ref	ref;
	BEntry		entry;
	StatStruct statbuf;
	status_t	err;
	bool		createDirectory = true;
	BEntry		existingEntry;

	if (loopControl->SkipEntry(srcEntry, false))
		return B_OK;

	thread = find_thread(NULL);

	srcEntry->GetRef(&ref);
	strcpy(destName, ref.name);

	loopControl->UpdateStatus(ref.name, ref, 1024, true);

	if (makeOriginalName)
		FSMakeOriginalName(destName, destDir, " copy");

	// ### RESOLVEALIAS ####
	if (destDir->FindEntry(destName, &existingEntry) == B_OK) {

		// some entry with a conflicting name is already present in destDir
		// decide what to do about it
		bool isDirectory = existingEntry.IsDirectory();

		switch (loopControl->OverwriteOnConflict(srcEntry, destName, destDir, 
			true, isDirectory)) {
			case TrackerCopyLoopControl::kSkip:
				// we are about to ignore this entire directory
				return B_OK;
			case TrackerCopyLoopControl::kReplace:
				if (isDirectory) {
					// remove existing folder recursively
					err = FSDeleteFolder(&existingEntry, loopControl, false);
					if (err!= B_OK)
						return err;
				} else if ((err = existingEntry.Remove()) != B_OK)
							// conflicting with a file or symbolic link, remove entry
					return err;
				break;
			case TrackerCopyLoopControl::kMerge:
				ASSERT(isDirectory);
				// do not create a new directory, use the current one
				newDir.SetTo(&existingEntry);
				createDirectory = false;
				break;
		}
	}

	// loop through everything in src folder and copy it to new folder
	srcDir.SetTo(srcEntry);
	srcDir.Rewind();
	srcEntry->Unset();

	// create a new folder inside of destination folder
	if (createDirectory && ((err = destDir->CreateDirectory(destName, &newDir))
		!= B_OK)) {
		PRINT(("folder copy error %s\n", strerror(err)));
		if (!loopControl->FileError(folderErrorString, destName, true))
			return err;
		else
			return B_OK;			// will allow rest of copy to continue
	}

	char *buf;
	if (createDirectory && err == B_OK && (buf = (char*)malloc(32768)) != 0) {
		CopyAttributes(loopControl, &srcDir, &newDir, buf, 32768);
			// don't copy original pose location if new location passed
		free(buf);
	}
	
	srcDir.GetStat(&statbuf);
	dev_t sourceDeviceID = statbuf.st_dev;

	// copy or write new pose location
	node_ref destNodeRef;
	destDir->GetNodeRef(&destNodeRef);
	SetUpPoseLocation(ref.directory, destNodeRef.node, &srcDir,
		&newDir, loc);

	// ### RESOLVE ###
	while (srcDir.GetNextEntry(&entry) == B_OK) {

		if (loopControl->CheckUserCanceled())
			return USER_CANCELED;

		entry.GetStat(&statbuf);
				
		if (S_ISDIR(statbuf.st_mode)) {

			// entry is a mount point, do not copy it
			if (statbuf.st_dev != sourceDeviceID) {
				PRINT(("Avoiding mount point %d, %d	\n", statbuf.st_dev, sourceDeviceID));
				continue;
			}
		
			if ((err = FSCopyFolder(&entry, &newDir, loopControl)) != B_OK)
				return err;
		} else if ((err = FSCopyFile(&entry, &statbuf, &newDir, loopControl))
			!= B_OK)
			return err;
	}

	return B_OK;
}

static status_t
LowLevelCopy(BEntry *srcEntry, StatStruct *src_stat,
	BDirectory *destDir, const char *dest_name, _CopyLoopControl *loopControl,
	BPoint *loc)
{
	status_t err;

	entry_ref ref;
	if ((err = srcEntry->GetRef(&ref)) != B_OK)
		return err;

	// handle symbolic links
	if (S_ISLNK(src_stat->st_mode)) {
		BSymLink srcLink;
		BSymLink newLink;
		char linkpath[MAXPATHLEN];

		if ((err = srcLink.SetTo(srcEntry)) != B_OK)
			return err;

		if ((err = srcLink.ReadLink(linkpath, MAXPATHLEN-1)) < 0)
		  return err;

		// ### note that there is a race here between writing the PoseInfo
		// attribute and the pose view reading it, we really need a better
		// way to set the icon location in the future
		err = destDir->CreateSymLink(dest_name, linkpath, &newLink);

		if (err != B_OK) 
			return err;

		node_ref destNodeRef;
		destDir->GetNodeRef(&destNodeRef);
		// copy or write new pose location as a first thing
		SetUpPoseLocation(ref.directory, destNodeRef.node, &srcLink,
			&newLink, loc);

		BNodeInfo nodeInfo(&newLink);
		err = nodeInfo.SetType(B_LINK_MIMETYPE);

		newLink.SetPermissions(src_stat->st_mode);
		newLink.SetOwner(src_stat->st_uid);
		newLink.SetGroup(src_stat->st_gid);
		newLink.SetModificationTime(src_stat->st_mtime);
		newLink.SetCreationTime(src_stat->st_crtime);

		return err;
	}

	BFile srcFile(srcEntry, O_RDONLY);
	if ((err = srcFile.InitCheck()) != B_OK)
		return err;

	size_t bufsize = src_stat->st_blksize;
	if (bufsize == 0)
		bufsize = 32768;

	BFile destFile(destDir, dest_name, O_RDWR | O_CREAT);
	err = destFile.InitCheck();
	if (err != B_OK)
		return err;

	node_ref destNodeRef;
	destDir->GetNodeRef(&destNodeRef);
	// copy or write new pose location as a first thing
	SetUpPoseLocation(ref.directory, destNodeRef.node, &srcFile,
		&destFile, loc);

	char *buf = new char[bufsize];

	// copy data portion of file
	while (true) {
		if (loopControl->CheckUserCanceled()) {
			// if copy was canceled, remove partial destination file
			destFile.Unset();

			BEntry destEntry;
			if (destDir->FindEntry(dest_name, &destEntry) == B_OK)
				destEntry.Remove();
			delete [] buf;
			return(COPY_CANCELED);
		}

		ASSERT(buf);
		ssize_t bytes = srcFile.Read(buf, bufsize);

		if (bytes > 0) {
			loopControl->ChecksumChunk(buf, bytes);

			ssize_t result = destFile.Write(buf, bytes);
			if (result != bytes) {
				delete [] buf;
				return(B_ERROR);
			}

			loopControl->UpdateStatus(NULL, ref, bytes, true);
		} else
			if (bytes < 0) {
				// read error
				delete [] buf;
				return(B_ERROR);
			} else
				break;
	}


	CopyAttributes(loopControl, &srcFile, &destFile, buf, bufsize);

	destFile.SetPermissions(src_stat->st_mode);
	destFile.SetOwner(src_stat->st_uid);
	destFile.SetGroup(src_stat->st_gid);
	destFile.SetModificationTime(src_stat->st_mtime);
	destFile.SetCreationTime(src_stat->st_crtime);

	delete [] buf;

	if (!loopControl->ChecksumFile(&ref)) {
		// File no good.  Remove and quit.
		destFile.Unset();

		BEntry destEntry;
		if (destDir->FindEntry(dest_name, &destEntry) == B_OK)
			destEntry.Remove();
		return USER_CANCELED;
	}

	return B_OK;
}

status_t
FSCopyFile(BEntry* srcFile, StatStruct *src_stat, BDirectory* destDir,
	_CopyLoopControl *loopControl, BPoint* loc, bool makeOriginalName)
{
	BVolume		volume;
	char		destName[B_FILE_NAME_LENGTH];
	status_t	err;
	entry_ref	ref;
	node_ref	node;

	if (loopControl->SkipEntry(srcFile, true))
		return B_OK;

	// check for free space first (add 2 sector fudge factor)
	destDir->GetNodeRef(&node);
	volume.SetTo(node.device);


	if ((src_stat->st_size + 1024) >= volume.FreeBytes()) {
		loopControl->FileError(noFreeSpace, "", false);
		return B_NO_MEMORY;
	}


	srcFile->GetName(destName);
	srcFile->GetRef(&ref);


	loopControl->UpdateStatus(destName, ref, 1024, true);


	if (makeOriginalName)
		FSMakeOriginalName(destName, destDir, " copy");


	BEntry conflictingEntry;
	// #### RESOLVE ###
	if (destDir->FindEntry(destName, &conflictingEntry) == B_OK) {
		switch (loopControl->OverwriteOnConflict(srcFile, destName, destDir, 
			false, false)) {
			case TrackerCopyLoopControl::kSkip:
				// we are about to ignore this entire directory
				return B_OK;
			case TrackerCopyLoopControl::kReplace:
				if (conflictingEntry.IsDirectory()) {
					// remove existing folder recursively
					if ((err = FSDeleteFolder(&conflictingEntry, loopControl, false)) != B_OK)
						return err;
				} else if ((err = conflictingEntry.Remove()) != B_OK)
							// conflicting with a file or symbolic link, remove entry
					return err;
				break;
			case TrackerCopyLoopControl::kMerge:
				// This flag implies that the attributes should be kept
				// on the file.  Just ignore it.
				break;
		}
	}

	err = LowLevelCopy(srcFile, src_stat, destDir, destName, loopControl, loc);

	if (err == COPY_CANCELED)
		return err;

	if (err != B_OK) {
		PRINT(("file copy error %s\n", strerror(err)));
		if (!loopControl->FileError(fileErrorString, destName, true))
			return err;
		else
			// user selected continue in spite of error, update status bar
			loopControl->UpdateStatus(NULL, ref, src_stat->st_size);
	}

	return B_OK;
}

status_t
FSDeleteFolder(BEntry *dir_entry, _CopyLoopControl *loopControl, bool update_status, 
	bool delete_top_dir, bool upateFileNameInStatus)
{
	entry_ref	ref;
	BEntry		entry;
	BDirectory	dir;
	status_t	err;

	dir.SetTo(dir_entry);
	dir.Rewind();

	// loop through everything in folder and delete it, skipping trouble files
	for(;;) {
		// #### RESOLVE ####
		if (dir.GetNextEntry(&entry) != B_OK)
			break;

		entry.GetRef(&ref);

		if (loopControl->CheckUserCanceled())
			return TRASH_CANCELED;

		if (entry.IsDirectory())
			err = FSDeleteFolder(&entry, loopControl, update_status, true,
				upateFileNameInStatus);
		else {
			err = entry.Remove();
			if (update_status)
				loopControl->UpdateStatus(upateFileNameInStatus ? ref.name : "", ref, 1, true);
		}

		if (err == TRASH_CANCELED)
			return TRASH_CANCELED;
		else if (err == B_OK)
			dir.Rewind();
		else 
			loopControl->FileError("There was an error deleting \"%s\"", 
				ref.name, false);
	}

	if (loopControl->CheckUserCanceled())
		return(TRASH_CANCELED);

	dir_entry->GetRef(&ref);

	if (update_status && delete_top_dir)
		loopControl->UpdateStatus(NULL, ref, 1);

	if (delete_top_dir) {
		return dir_entry->Remove();
	} else
		return B_OK;
}


void 
FSMakeOriginalName(BString &string, const BDirectory *destDir, const char *suffix)
{
	if (!destDir->Contains(string.String()))
		return;

	FSMakeOriginalName(string.LockBuffer(B_FILE_NAME_LENGTH),
		const_cast<BDirectory *>(destDir), suffix ? suffix : " copy");
	string.UnlockBuffer();
}

void
FSMakeOriginalName(char *name, BDirectory *destDir, const char *suffix)
{
	char		root[B_FILE_NAME_LENGTH];
	char		copybase[B_FILE_NAME_LENGTH];
	char		temp_name[B_FILE_NAME_LENGTH + 10];
	long		fnum;

	// is this name already original?
	if (!destDir->Contains(name))
		return;


	// Determine if we're copying a 'copy'. This algorithm isn't perfect.
	// If you're copying a file whose REAL name ends with 'copy' then
	// this method will return "<filename> 1", not "<filename> copy"

	// However, it will correctly handle file that contain 'copy' 
	// elsewhere in their name.

	bool	copycopy = false;		// are we copying a copy?
	long	len = strlen(name);
	char	*p = name + len - 1;	// get pointer to end os name

	// eat up optional numbers (if were copying "<filename> copy 34")
	while ((p > name) && isdigit(*p))
		p--;
	
	// eat up optional spaces
	while ((p > name) && isspace(*p))
		p--;

	// now look for the phrase " copy"
	if (p > name) {
		// p points to the last char of the word. For example, 'y' in 'copy'

		if ((p - 4 > name) && (strncmp(p - 4, suffix, 5) == 0)) {
			// we found 'copy' in the right place. 
			// so truncate after 'copy'
			*(p + 1) = '\0';
			copycopy = TRUE;

			// save the 'root' name of the file, for possible later use.
			// that is copy everything but trailing " copy". Need to
			// NULL terminate after copy
			strncpy(root, name, (p - name) - 4);
			root[(p - name) - 4] = '\0';
		}
	}

	if (!copycopy) {
		/*
		 The name can't be longer than B_FILE_NAME_LENGTH.
		 The algoritm adds " copy XX" to the name. That's 8 characters.
		 B_FILE_NAME_LENGTH already accounts for NULL termination so we
		 don't need to save an extra char at the end.
		*/
		if (strlen(name) > B_FILE_NAME_LENGTH - 8) {
			// name is too long - truncate it!
			name[B_FILE_NAME_LENGTH - 8] = '\0';
		}

		strcpy(root, name);		// save root name
		strcat(name, suffix);
	}

	strcpy(copybase, name);

	// if name already exists then add a number
	fnum = 1;
	strcpy(temp_name, name);
	while (destDir->Contains(temp_name)) {
		sprintf(temp_name, "%s %ld", copybase, ++fnum);

		if (strlen(temp_name) > (B_FILE_NAME_LENGTH - 1)) {
			/*
			 The name has grown too long. Maybe we just went from
			 "<filename> copy 9" to "<filename> copy 10" and that extra
			 character was too much. The solution is to further
			 truncate the 'root' name and continue.
			 ??? should we reset fnum or not ???
			*/
			root[strlen(root) - 1] = '\0';
			sprintf(temp_name, "%s%s %ld", root, suffix, fnum);
		}
	}
	
	ASSERT((strlen(temp_name) <= (B_FILE_NAME_LENGTH - 1)));
	strcpy(name, temp_name);
}

} // namespace BPrivate
