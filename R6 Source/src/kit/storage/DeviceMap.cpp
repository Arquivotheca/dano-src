// (c) 1997 Be, Incorporated
//
// volume management utilities
//
// based on DriveSetup
//

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <image.h>
#include <fs_info.h>

#include <Bitmap.h>
#include <Debug.h>
#include <Directory.h>
#include <Drivers.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Mime.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Path.h>


#include "scsi.h"
#include "priv_syscalls.h"

#if _INCLUDES_CLASS_DEVICE_MAP

#include <private/storage/DeviceMap.h>


const int32 E_NOTSET = B_ERRORS_END + 1;

// add-on iterators

typedef bool (*EachAddOnFunction)(image_id, BEntry *, void *);

static bool
EachAddOnCommon(const char *addonPath, EachAddOnFunction func, void *params)
{
	BDirectory directory;
	BPath path;
	BEntry entry;

	find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
	directory.SetTo(path.Path());
	directory.FindEntry(addonPath, &entry);
	directory.SetTo(&entry);

	if (directory.InitCheck() != B_NO_ERROR)
		return false;

	directory.Rewind();
	for (;;) {
		if (directory.GetNextEntry(&entry) < 0)
			break;
 
		BPath entryPath;
		entry.GetPath(&entryPath);
		image_id image = load_add_on(entryPath.Path());
		bool done = false;
		if (image >= 0) {
			done = func(image, &entry, params);
			unload_add_on(image);
			PRINT(("failed loading addon %s", entryPath.Path()));
		}
		if (done)
			return true;
	}
	return false;
}

static bool
EachPartitioningAddOn(EachAddOnFunction func, void *params)
{
	return EachAddOnCommon(DS_PART_ADDONS, func, params);
}

static bool
EachSessionAddOn(EachAddOnFunction func, void *params)
{
	return EachAddOnCommon(DS_SESSION_ADDONS, func, params);
}

static bool
EachFileSystemAddOn(EachAddOnFunction func, void *params)
{
	return EachAddOnCommon(DS_FS_ADDONS, func, params);
}

int32 Partition::lastUniqueID = 0;

Partition::Partition(Session *session, const char *name, const char *type, 
		const char *fsShortName, const char *fsLongName, 
		const char *volumeName, const char *mountedAt,
		uint32 logicalBlockSize, uint64 offset, uint64 blocks, 
		bool hidden)
	:	session(session),
		mounted(kUnknown),
		mountPointNodeRefStatus(E_NOTSET)
{
	strcpy(data.partition_name, name);
	strcpy(data.partition_type, type);
	strcpy(data.file_system_short_name, fsShortName);
	strcpy(data.file_system_long_name, fsLongName);
	strcpy(data.volume_name, volumeName);
	strcpy(data.mounted_at, mountedAt);

	if (strlen(mountedAt) > 0)
		mountPointNodeRefStatus = BNode(mountedAt).GetNodeRef(
			&mountPointNodeRef);

	data.logical_block_size = logicalBlockSize;
	data.offset = offset;
	data.blocks = blocks;
	data.hidden = hidden;
	partitionUniqueID = lastUniqueID++;
	volumeDeviceID = -1;
}

Partition::Partition(Session *session, uint32 logicalBlockSize, uint64 offset, 
		uint64 blocks, bool hidden)
	:	session(session),
		mounted(kUnknown),
		mountPointNodeRefStatus(E_NOTSET)
{
	strcpy(data.partition_name, "");
	strcpy(data.partition_type, "");
	strcpy(data.file_system_short_name, "");
	strcpy(data.file_system_long_name, "");
	strcpy(data.volume_name, "");
	strcpy(data.mounted_at, "");
	data.logical_block_size = logicalBlockSize;
	data.offset = offset;
	data.blocks = blocks;
	data.hidden = hidden;
	partitionUniqueID = lastUniqueID++;
	volumeDeviceID = -1;
}

Partition::Partition(Session *session, const partition_data &data)
	:	data(data),
		session(session),
		mounted(kUnknown),
		mountPointNodeRefStatus(E_NOTSET)
{
	partitionUniqueID = lastUniqueID++;
	volumeDeviceID = -1;
}

Session *
Partition::GetSession() const
{ 
	return session;
}

Device *
Partition::GetDevice() const
{ 
	return session->GetDevice();
}

int32
Partition::Index() const
{
	return GetSession()->partitionList.IndexOf(const_cast<Partition *>(this));
}

int32
Partition::UniqueID() const
{
	return partitionUniqueID;
}

dev_t
Partition::VolumeDeviceID() const
{
	return volumeDeviceID;
}

void
Partition::SetVolumeDeviceID(dev_t dev)
{
	volumeDeviceID = dev;
}

void
Partition::SetName(const char *name)
{
	strcpy(data.partition_name, name);
}

const char * 
Partition::Name() const
{
	return data.partition_name;
}

void 
Partition::SetType(const char *name)
{
	strcpy(data.partition_type, name);
}

const char * 
Partition::Type() const
{
	return data.partition_type;
}

bool 
Session::VirtualPartitionOnly() const
{
	return virtualPartitionOnly;
}

void 
Partition::SetFileSystemShortName(const char *name)
{
	strcpy(data.file_system_short_name, name);
}

const char * 
Partition::FileSystemShortName() const
{
	return data.file_system_short_name;
}

void 
Partition::SetFileSystemLongName(const char *name)
{
	strcpy(data.file_system_long_name, name);
}

const char * 
Partition::FileSystemLongName() const
{
	return data.file_system_long_name;
}

bool 
Partition::IsBFSDisk() const
{
	return strcmp(FileSystemShortName(), "bfs") == 0;
}

bool 
Partition::IsHFSDisk() const
{
	return strcmp(FileSystemShortName(), "hfs") == 0;
}

bool 
Partition::IsOFSDisk() const
{
	return strcmp(FileSystemShortName(), "ofs") == 0;
}

void 
Partition::SetVolumeName(const char *name)
{
	strcpy(data.volume_name, name);
}

const char * 
Partition::VolumeName() const
{
	return data.volume_name;
}

void 
Partition::SetMountedAt(const char *name)
{
	char correctName[1024];
	strncpy(correctName, name, 1024);

    //
    //  Don't watch the /boot volume (it doesn't do anything interesting),
    //  instead, watch the symlink that points to it.
    //
    if (strcmp(name, "/boot") == 0) {
        BVolume boot;
        if (BVolumeRoster().GetBootVolume(&boot) == B_OK) {
            correctName[0] = '/';
            boot.GetName(correctName + 1);
        }                               
	}

	strcpy(data.mounted_at, correctName);

	if (strlen(name) != 0)
		mountPointNodeRefStatus = BNode(correctName).GetNodeRef(&mountPointNodeRef);
}

const char * 
Partition::MountedAt() const
{
	return data.mounted_at;
}

status_t Partition::GetMountPointNodeRef(node_ref *node) const
{
	ASSERT(mountPointNodeRefStatus != E_NOTSET);
	*node = mountPointNodeRef;
	return mountPointNodeRefStatus; 
}

void Partition::SetMountPointNodeRef(const node_ref *node)
{
	mountPointNodeRefStatus = B_OK;
	mountPointNodeRef = *node;
}


uint32
Partition::LogicalBlockSize() const
{
	return data.logical_block_size;
}

uint64
Partition::Offset() const
{
	return data.offset;
}

uint64
Partition::Blocks() const
{
	return data.blocks;
}

bool
Partition::Hidden() const
{
	return data.hidden;
}

MountState 
Partition::Mounted() const
{ 
	return mounted; 
}

void 
Partition::SetMountState(MountState state)
{ 
	mounted = state; 
}

struct FindMountedVolumesParams {
	bool changed;
};

bool
Partition::SetOneUnknownMountState(void *)
{
	SetMountState(kUnknown);
	return false;
}

void
Partition::Dump(const char *log)
{
	char buffer[256];
	
	if (Mounted() != kMounted)
		sprintf(buffer, "%smounted", 
			Mounted() == kNotMounted ? "not " : "?? not ");
	else
		sprintf(buffer, "mnt:%s", MountedAt());


	printf("%s: vol:%s prt:%s, tpe:%s, ss:%s %s, dv: %s, fss:%s%s uid%ld vol dev:%ld, %s inode: %Ld\n",
		log,
		VolumeName(), 
		Name(),
		Type(),
		GetSession()->Name(),
		buffer,
		GetDevice()->Name(),
		FileSystemShortName(),
		Hidden() ? " hidden" : "",
		UniqueID(),
		VolumeDeviceID(),
		Mounted() == kMounted ? "mounted" : (Mounted() == kNotMounted ?  "not mounted" : "mount state unknown"),
		mountPointNodeRef.node	
		);
}

bool
Partition::ClearOneMountState(void *castToParams)
{
	// called as the last step of updating mounting info
	// clear out every device that is not mounted
	FindMountedVolumesParams *params = (FindMountedVolumesParams *)castToParams;
	if (Mounted() == kUnknown) {
		SetMountState(kNotMounted);
		// if had a mounted at name before, got unmounted
		if (MountedAt()[0] != '\0')
			params->changed = true;
		SetMountedAt("");
	}
	return false;
}

status_t
Partition::AddVirtualDevice(char *path)
{
	Session *session = GetSession();
	Device *device = GetDevice();
	
	char virtualDeviceName[256];
	// create the name for the virtual device
	int32 len = strlen(device->Name()) - strlen("/raw");
	memcpy(virtualDeviceName, device->Name(), len);
	virtualDeviceName[len] = '\0';
	sprintf(path, "%s/%ld_%ld", virtualDeviceName, session->Index(), Index());
	
	PRINT(("adding virtual device %s\n", path));

	unlink(path);
	
	// create the virtual device driver file
	int32 dev = creat(path, 0666);
	if (dev < 0) {
		PRINT(("error %s adding virtual device %s\n",
			strerror(dev), path));

		return dev;
	}
		
	struct stat st;
	
	status_t result = stat(device->Name(), &st);
	if (result != B_NO_ERROR)
		return result;

	// set up the virtual device
	partition_info partitionInfo;

	partitionInfo.offset = session->Offset() * device->BlockSize()
		+ Offset() * LogicalBlockSize();
		
	partitionInfo.size = Blocks() * LogicalBlockSize();
	partitionInfo.logical_block_size = LogicalBlockSize();
	partitionInfo.session = session->Index();
	partitionInfo.partition = Index();
	strcpy(partitionInfo.device, device->Name());
	result = ioctl(dev, B_SET_PARTITION, &partitionInfo);
	close(dev);

	if (result != B_OK)
		PRINT(("error device %s\n", strerror(result)));

	return result;
}

status_t
Partition::AddVirtualDevice()
{
	if (Mounted() == kMounted)
		// we are already mounted
		return B_NO_ERROR;
	
	char driverPath[B_FILE_NAME_LENGTH];
	
	if (GetSession()->GetDevice()->CountPartitions() == 1)
		return B_NO_ERROR;
		
	// for devices with more than one partition have to do more work
	if (Hidden()
		// don't mount hidden partitions
		|| strcmp(FileSystemShortName(), "audio") == 0) 
		// don't mount audio partitions
		return B_NO_ERROR;

	return AddVirtualDevice(driverPath);
}

extern "C" int mount(const char*, const char*, const char*, ulong, void*, int);
extern "C" int unmount(const char*);

void 
Partition::InitialMountPointName(char *buffer)
{
	if (!strlen(VolumeName()))
		// no volume name, go with default
		sprintf(buffer, "/disk");	
	else {	
		// got name, make sure it's ok
		sprintf(buffer, "/%s", VolumeName());
		
		// don't allow special system directories as a volume mountpoint
		char *tmp = &buffer[1];
		if (strcmp(tmp, "dev") == 0
			|| strcmp(tmp, "pipe") == 0
			|| strcmp(tmp, "var") == 0
			|| strcmp(tmp, "system") == 0
			|| strcmp(tmp, "bin") == 0
			|| strcmp(tmp, "tmp") == 0
			|| strcmp(tmp, "etc") == 0)
			// just uppercase the first character
			tmp[0] = toupper(tmp[0]);
	
		for (uint32 index = 1; index < strlen(buffer); index++)
			// can't do '/' in volume names
			if (buffer[index] == '/')
				buffer[index] = ':';
	}
}

status_t 
Partition::Mount(int32 mountflags, void *params, int32 paramslen)
{
	if (Mounted() == kMounted)
		// we are already mounted
		return B_NO_ERROR;
	
	Device *device = GetSession()->GetDevice();

	//
	// The user may have re-formatted a partition that wasn't
	// mounted, or put in a new floppy.  Since we don't receive
	// notifications when these things change, re-scan the 
	// file system type before continuing.
	//
	RebuildFileSystemInfo();

	if (Hidden()
		// don't mount hidden partitions
		|| strcmp(FileSystemShortName(), "audio") == 0
		// don't mount audio partitions
		|| strcmp(FileSystemShortName(), "unknown") == 0 ) {
		// can't mount this either
		printf("cannot mount hidden or audio partition\n");			
		return B_ERROR;
	}

	status_t result;
	char driverPath[B_FILE_NAME_LENGTH];
	if (GetSession()->GetDevice()->CountPartitions() == 1)
		sprintf(driverPath, device->Name());
	else {
		// for devices with more than one partition have to do more work
		result = AddVirtualDevice(driverPath);
		if (result != B_NO_ERROR)
			return result;
	}

	char tmp[B_FILE_NAME_LENGTH];
	InitialMountPointName(tmp);

	char mountAt[B_FILE_NAME_LENGTH];
	BDirectory directory;

	strcpy(mountAt, tmp);

	int32 index = 1;
	for (;;) {
		// try creating directory for mount point
		if (!mkdir(mountAt, 0777))
			break;

		// couldn't create directory, must be already taken, try making unique
		directory.SetTo(mountAt);
		if (directory.InitCheck() == B_NO_ERROR && directory.CountEntries() == 0)
			break;
		sprintf(mountAt, "%s%ld", tmp, index++);
	}

//	printf("trying to mount at %s, file system %s, driver %s\n", mountAt,
//		FileSystemShortName(), driverPath);


	result = mount(FileSystemShortName(), mountAt, driverPath, mountflags, 
		params, paramslen);

	//
	//	If it failed again, give up.
	//
	if (result != B_OK)
		PRINT(("error %s using %s, mounting at %s\n", strerror(result), 
		driverPath, mountAt));


	if (result == B_NO_ERROR) {
		// mounted fine, update mounting info
		SetMountState(kMounted);
		SetMountedAt(mountAt);

		struct fs_info info;
		_kstatfs_(-1, NULL, -1, mountAt, &info);
		SetVolumeDeviceID(info.dev);
	}
	return result;
}

status_t
Partition::Unmount()
{
	if (Mounted() == kNotMounted)
		// not mounted, ignore
		return B_NO_ERROR;

	status_t result;	
	{
		// update the current mount point to make sure it is correct
		// before we unmount
		// can get rid of this when MountedAt uses volume device ID to
		// get the right string
		BVolume volume(VolumeDeviceID());
		ASSERT(volume.InitCheck() == B_NO_ERROR);
		BDirectory root;
		result = volume.GetRootDirectory(&root);
		ASSERT(result == B_NO_ERROR);	
		if (result != B_NO_ERROR)
			return result;

		BEntry entry;
		root.GetEntry(&entry);
		result = entry.InitCheck();
		ASSERT(result == B_NO_ERROR);
		// something bad happened, bail out
		if (result != B_NO_ERROR)
			return result;

		BPath path;
		entry.GetPath(&path);
		SetMountedAt(path.Path());
	}
	
	PRINT(("unmounting %s\n", MountedAt()));

	result = unmount(MountedAt());
	if (result) {
		PRINT(("unmount - result %s\n", strerror(result)));
		return result;
	}
	
	// remove the old mount point
	result = rmdir(MountedAt());

	if (result == B_NO_ERROR) {
		SetMountedAt("");
		SetMountState(kNotMounted);
	} else {
		PRINT(("rmdir - result %s\n", strerror(result)));
	}

	return result;
}

status_t
Partition::Initialize(InitializeControlBlock *params, const char *fileSystem)
{
	status_t result = Unmount();
	if (result != B_NO_ERROR)
		return result;

	Session *session = GetSession();
	Device *device = session->GetDevice();

	char driverPath[B_FILE_NAME_LENGTH];
	
	bool singlePartition = false;
	if (GetSession()->GetDevice()->CountPartitions() == 1) {
		sprintf(driverPath, device->Name());
		singlePartition = true;
	} else {
		result = AddVirtualDevice(driverPath);
		if (result != B_NO_ERROR)
			return result;
	}

	BPath path;
	char addonPath[B_FILE_NAME_LENGTH];
		
	find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
	sprintf(addonPath, "%s/%s%s", path.Path(), DS_FS_ADDONS, fileSystem);

	image_id image = load_add_on(addonPath);
	if (image < 0)
		return image;

	params->cancelOrFail = false;

	void (*initialize)(BMessage*);
	result = get_image_symbol(image, DS_FS_INITIALIZE, B_SYMBOL_TYPE_TEXT,
		(void **)&initialize);
	if (result >= 0) {
		BMessage *msg = new BMessage(params->completionMessage);
		msg->AddInt32("session", session->Index());
		msg->AddInt32("partition", Index());
		msg->AddPointer("window", params->window);
		msg->AddData("image", B_ANY_TYPE, &image, sizeof(image_id));
		//msg->AddPointer("item", item);
		msg->AddString("device", driverPath);
		msg->AddInt32("block_size", device->BlockSize());
		msg->AddInt64("blocks", data.blocks);
		msg->AddInt64("offset", data.offset);
		msg->AddPointer("part_code", &data.partition_code);
		if (strlen(VolumeName()))
			msg->AddString("name", VolumeName());

		(*initialize)(msg);
		acquire_sem(params->completionSemaphore);
		
		PRINT(("returned from initialize addon %s\n", params->cancelOrFail
			? "cancelled or failed" : "OK"));
	}

	unload_add_on(image);

	if (result != B_NO_ERROR)
		return result;
	else if ((session->Index() >= 0) && (Index() >= 0)) {
		int32 dev;
		void (*update_map)(int32, int32, partition_data*);

		if ((dev = open(device->Name(), O_RDWR | O_CLOEXEC)) >= 0) {
			find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
			sprintf(addonPath, "%s/%s%s", path.Path(), DS_PART_ADDONS, GetSession()->Name());
			if ((image = load_add_on(addonPath)) >= 0) {
				if (get_image_symbol(image, DS_UPDATE_MAP, B_SYMBOL_TYPE_TEXT,
					(void **)&update_map) >= 0)
					(*update_map)(dev, Index(), Data());
				unload_add_on(image);
			}
			close(dev);
		}
	}

	if (params->cancelOrFail)
		return B_OK;

	// get the logical block size
	// update the file system info
	if (!RebuildFileSystemInfo()) {
//	Dump("bad rebuild");
		return B_ERROR;
	}
//	Dump("after rebuild");

	return B_NO_ERROR;
}

partition_data *
Partition::Data()
{
	// let's get down and dirty
	return &data;
}


struct BuildFSInfoParams {
	Partition *partition;
	int32 dev;
	uint64 offset;
	int32 blockSize;
	bool result;
};

static bool
BuildOneFileSystemInfo(image_id image, BEntry */*addonEntry*/, void *castToParams)
{
	BuildFSInfoParams *params = (BuildFSInfoParams *)castToParams;

	bool (*ds_fs_id)(partition_data*, int32, uint64, int32);
	if (get_image_symbol(image, DS_FS_ID, B_SYMBOL_TYPE_TEXT, (void **)&ds_fs_id) < 0) {
		PRINT(("failed while looking for symbol %s\n", DS_FS_ID));
		return false;
	}
 	
 	params->result = (*ds_fs_id)(params->partition->Data(), params->dev, 
 		params->offset, params->blockSize);

//	char buffer[255];
//	addonEntry->GetName(buffer);
//	PRINT(("volume:%s, type:%s, device:%s, addon %s, offset %d, blockSize %d, %s\n", 
//		params->partition->Data()->volume_name, params->partition->Data()->partition_type, 
//		buffer, "", (long)params->offset, (long)params->blockSize, result ? "ok" : "failed"));

	// if we found our addon and the call worked, returning true, which will stop
	// the search
	return params->result;
}

bool
Partition::BuildFileSystemInfo(void *castToParams)
{
// allways return result, actual result in params.result
	BuildFSInfoParams *params = (BuildFSInfoParams *)castToParams;
	if (!Hidden()) {
		if (!Blocks()) {
			SetFileSystemShortName("n/a");
			SetFileSystemLongName("n/a");
			return false;
		}
		SetFileSystemShortName("unknown");
		SetFileSystemLongName("unknown file system");
		
		params->partition = this;
		EachFileSystemAddOn(BuildOneFileSystemInfo, params);
//		PRINT(("build file system info: volume %s, fsshort %s, fslong %s\n",
//			VolumeName(), FileSystemShortName(), FileSystemLongName()));

	} else {

		SetFileSystemShortName("");
		SetFileSystemLongName("");
		SetVolumeName("");
	}
	return false;
}

bool
Partition::RebuildFileSystemInfo()
{
	BuildFSInfoParams params;
	Device *device = GetDevice();
	int32 dev = open(device->Name(), O_RDONLY | O_CLOEXEC);
	ASSERT(dev >= 0);
	params.result = false;

	if (dev >= 0) {
		params.dev = dev;
		params.result = false;
		params.partition = this;
		params.blockSize = GetDevice()->BlockSize();
		params.offset = GetSession()->Offset();
	
		BuildFileSystemInfo(&params);
	}
	close(dev);
	return params.result;
}

Session::Session(Device *device, const char *mapName, uint64 offset, 
	uint64 blocks, bool data)
	:	offset(offset),
		blocks(blocks),
		data(data),
		virtualPartitionOnly(true),
		partitionList(10, true),
		device(device)
{
	strcpy(name, mapName);
	type = P_UNKNOWN;
}

struct GetSessionDataParams {
	int32 dev;
	int32 index;
	int32 blockSize;
	session_data *session;
	status_t result;
};

static bool
OneGetSessionData(image_id image, BEntry *, void *castToParams)
{
	GetSessionDataParams *params = (GetSessionDataParams *)castToParams;

	status_t (*ds_get_nth_session)(int32, int32, int32, session_data*);

	if (get_image_symbol(image, DS_GET_NTH_SESSION, B_SYMBOL_TYPE_TEXT, 
		(void **)&ds_get_nth_session) >= 0) {
		params->result = (*ds_get_nth_session)(params->dev, params->index, 
			params->blockSize, params->session);
		// done
		return true;
	} else 
		PRINT(("failed while looking for symbol %s\n", DS_GET_NTH_SESSION));

	return false;
}

bool
Session::BuildFileSystemInfo(int32 dev)
{
	BuildFSInfoParams params;
	
	params.dev = dev;
	params.result = false;
	params.partition = NULL;
	params.blockSize = GetDevice()->BlockSize();
	params.offset = Offset();
	
	EachPartition(&Partition::BuildFileSystemInfo, &params);
	
	return params.result;
}

	
status_t
Session::GetSessionData(int32 dev, int32 index, 
	int32 blockSize, session_data *session)
{
	GetSessionDataParams params;
	params.session = session;
	params.dev = dev;
	params.index = index;
	params.result = B_ERROR;	// in case we don't find anything
	params.blockSize = blockSize;

	EachSessionAddOn(OneGetSessionData, &params);
	
	return params.result;
}

bool
Session::EachPartition(EachPartitionMemberFunction func, void *params)
{
	int32 count = partitionList.CountItems();
	for (int32 index = 0; index < count; index++) {
		Partition *partition = partitionList.ItemAt(index);
		if ((partition->*func)(params))
			return true;
	}
	return false;
}

void 
Session::SetType(int32 type)
{ 
	this->type = type;
}

uint64 
Session::Offset() const
{ 
	return offset; 
}

Device *
Session::GetDevice() const
{ 
	return device; 
}

void 
Session::SetName(const char *newName)
{
	strcpy(name, newName);
}

const char * 
Session::Name() const
{
	return name;
}

uint64 
Session::Blocks() const
{
	return blocks;
}

int32 
Session::Index() const
{
	return GetDevice()->sessionList.IndexOf(const_cast<Session *>(this));
}


int32
Session::CountPartitions() const
{ 
	return partitionList.CountItems(); 
}

Partition *
Session::PartitionAt(int32 index) const
{
	return partitionList.ItemAt(index); 
}

void 
Session::SetAddOnEntry(const BEntry *entry)
{ 
	add_on = *entry; 
}

struct BuildPartMapParams {
	Session *session;
	int dev;
	uchar *block;
	long blockSize;
};

static bool
BuildPartitioningMapOne(image_id image, BEntry *entry, void *castToParams)
{
	BuildPartMapParams *params = (BuildPartMapParams *)castToParams;

	bool (*ds_partition_id)(uchar *, int32);
	if (get_image_symbol(image, DS_PARTITION_ID, B_SYMBOL_TYPE_TEXT,
		(void **)&ds_partition_id) < 0) {
		// not the right partitioning addon, bail
		PRINT(("failed while looking for symbol %s\n", DS_GET_NTH_SESSION));
		return false;
	}
	if (!(*ds_partition_id)(params->block, params->blockSize))
		// no partition info, bail
		return false;
						
	// get a partition name
	char *(*ds_partition_name)(void);
	if (get_image_symbol(image, DS_PARTITION_NAME, B_SYMBOL_TYPE_TEXT, 
		(void **)&ds_partition_name) >= 0)
		// found a partition name
		params->session->SetName((*ds_partition_name)());
	else
		PRINT(("failed while looking for symbol %s\n", DS_PARTITION_NAME));
	
	// go through all the maps and create partition objects for each
	status_t (*ds_get_nth_map)(int32, uchar*, uint64, int32, int32, 
		partition_data*);
	if (get_image_symbol(image, DS_PARTITION_MAP, B_SYMBOL_TYPE_TEXT, 
		(void **)&ds_get_nth_map) >= 0) {
		int index;
		for (index = 0; ; index++) {
			partition_data info;
			if ((*ds_get_nth_map)(params->dev, params->block, 
				params->session->Offset(), 
				params->blockSize, index, &info) != B_NO_ERROR)
				break;

			// not mounted yet
			info.mounted_at[0] = 0;
			// add a new partition
			params->session->AddPartition(new Partition(params->session, info));
		}
		if(index == 0) {
			PRINT(("no partitions\n"));
			return false;
		}
	} else
		PRINT(("failed while looking for symbol %s\n", DS_PARTITION_MAP));

	// read partitioning flags if possible
	void (*ds_partition_flags)(drive_setup_partition_flags*);
	if (get_image_symbol(image, DS_PARTITION_FLAGS, B_SYMBOL_TYPE_TEXT, 
		(void **)&ds_partition_flags) >= 0) {
		drive_setup_partition_flags	flags;
		(*ds_partition_flags)(&flags);
		params->session->GetDevice()->SetPartitioningFlags(flags);
	} else
		PRINT(("failed while looking for symbol %s\n", DS_PARTITION_FLAGS));
	

	// we are going to use this addon from now on
	params->session->SetAddOnEntry(entry);
	// you have to ask the addon about what type this session is
	params->session->SetType(P_ADD_ON);
	return true;
}

bool 
Session::BuildPartitionMap(int32 dev, uchar *block, bool singlePartition)
{
	if (!singlePartition) {

		BuildPartMapParams params;
		params.session = this;
		params.dev = dev;
		params.block = block;
		params.blockSize = GetDevice()->BlockSize();

		if (EachPartitioningAddOn(BuildPartitioningMapOne, &params)) {
			virtualPartitionOnly = false;
			return true;
		}
	}
	
	// single partition the size of the whole session
	Partition *partition = new Partition(this, GetDevice()->BlockSize(), 0, 
		Blocks());
	AddPartition(partition);
	SetType(P_ADD_ON);
	SetName("unknown");

	return false;
}

bool 
Session::IsDataSession() const
{
	return data;
}

void
Session::AddPartition(Partition *partition)
{
	partitionList.AddItem(partition);
}

Device::Device(const char *path, int ndevfd)
	:	devfd(ndevfd),
		largeIcon(NULL),
		miniIcon(NULL),
		readOnly(false),
		removable(false),
		isFloppy(false),
		media_changed(false),
		eject_request(false),
		sessionList(10, true)
{
	shortName[0] = '\0';

	strcpy(name, path);
	if (strstr(name, "floppy"))
		isFloppy = true;
	
	InitNewDeviceState();
}

Device::~Device()
{
	delete miniIcon;
	delete largeIcon;
	if(devfd >= 0) {
		close(devfd);
	}
}

void
Device::InitNewDeviceState()
{
	device_geometry geometry;
	status_t media_status;
	
	memset(&geometry, 0, sizeof(geometry));
	if (strstr(name, "floppy"))
		isFloppy = true;
		
	if(eject_request) {
		eject_request = false;
		if(devfd < 0) {
			Dump("eject request on closed device");
		}
		else {
			ioctl(devfd, B_EJECT_DEVICE);
		}
	}

	if(devfd < 0) {
		devfd = open(name, O_RDONLY | O_CLOEXEC);
	}

	if (devfd >= 0 
		&& (ioctl(devfd, B_GET_MEDIA_STATUS, &media_status) != B_NO_ERROR
		    || media_status == B_NO_ERROR)
		&& (ioctl(devfd, B_GET_GEOMETRY, &geometry) == B_NO_ERROR
		    || errno == B_DEV_UNREADABLE) ) {
		readOnly = geometry.read_only;
		removable = geometry.removable;
		blockSize = geometry.bytes_per_sector;
		media_changed = false;
		
		uchar *block = new uchar[blockSize];
		for (;;) {
			// go through all sessions (only CDs can have more than one)
			Session *session;
			if (geometry.device_type == B_CD) {
				// for CDs use a session handler addon to figure out session
				// info
//				PRINT(("adding CD session\n"));
				session = NewSession(devfd, CountSessions());
				if (!session)
					break;
				
				sessionList.AddItem(session);
				if (!session->IsDataSession()) {
					session->SetName("audio");
					session->SetType(P_AUDIO);

					session->AddPartition(new Partition(session, "", "", 
						"audio", "audio", "", "", blockSize, 0, 
						session->Blocks(), true));

					continue;
				}
			} else {
				// for disks build a session from the geometry info
				session = new Session(this, "", 0, geometry.sectors_per_track 
					* geometry.cylinder_count * geometry.head_count, false);

				sessionList.AddItem(session);
			}

			lseek(devfd, session->Offset() * blockSize, 0);
			if (read(devfd, block, blockSize) >= 0) {
				session->SetType(P_UNKNOWN);
				session->SetName("unknown");
				bool virtualPartitionOnly = !session->BuildPartitionMap(devfd, block, 
					isFloppy);
				
				if (session->BuildFileSystemInfo(devfd) && virtualPartitionOnly)
					session->SetName("none");
			}
			else {
				session->SetType(P_UNREADABLE);
				removable = true;
				readOnly = false;
			}

			if (geometry.device_type != B_CD)
				break;
		}
		delete [] block;
	}

	BRect rect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);
	largeIcon = new BBitmap(rect, B_COLOR_8_BIT);
	if (get_device_icon(name, largeIcon->Bits(), B_LARGE_ICON) != B_NO_ERROR) {
		delete largeIcon;
		largeIcon = NULL;
	}

	rect.Set(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1);
	miniIcon = new BBitmap(rect, B_COLOR_8_BIT);
	if (get_device_icon(name, miniIcon->Bits(), B_MINI_ICON) != B_NO_ERROR) {
		delete miniIcon;
		miniIcon = NULL;
	}
}

void
Device::KillOldDeviceState()
{
	sessionList.MakeEmpty();
	delete miniIcon;
	miniIcon = NULL;
	delete largeIcon;
	largeIcon = NULL;
	media_changed = false;
}

void
Device::UpdateDeviceState()
{
// auto-unmount here, if needed
	KillOldDeviceState();
	InitNewDeviceState();
}

bool
Device::IsFloppy() const
{
	return isFloppy;
}

bool
Device::NoMedia() const
{
	return CountSessions() == 0;
}

status_t
Device::Eject()
{
	if(devfd < 0) {
		PRINT(("Device::Eject: %s is not open\n", name));
		return B_ERROR;
	}
	ioctl(devfd, B_EJECT_DEVICE);
	UpdateDeviceState();
	return B_OK;
}

Session *
Device::NewSession(int32 dev, int32 index)
{
	session_data sessionData;

	if (Session::GetSessionData(dev, index, BlockSize(), &sessionData) 
		!= B_NO_ERROR) {
		PRINT(("no session %d for %d\n", index, dev));
		return NULL;
	}

//	PRINT(("session %d for %d\n", index, dev));

	Session *result = new Session(this, "", sessionData.offset, 
		sessionData.blocks, sessionData.data);
//	sessionList.AddItem(result, index);
	return result;	
}

void
Device::Dump(const char *extraMessage)
{
	printf("%s dev:%s name:%s, %s%s%s %ld\n",
		extraMessage ? extraMessage : "",
		Name(),
		DisplayName(),
		ReadOnly() ? " read only" : " writable",
		Removable() ? " removable" : "",
		!NoMedia() ? "" : " no media",
		CountSessions()
		);
}

bool
Device::Dump(void *castToString)
{
	Dump((const char *)castToString);
	return false;
}

bool
Device::DeviceStateChanged(void *castToParams)
{
	DeviceScanParams *params = (DeviceScanParams *)castToParams;

	device_geometry geometry;
	status_t media_status = B_NO_ERROR;
	
	if (!params->checkFloppies && isFloppy)
		return false;

	bool hadMedia = !NoMedia();
	if (hadMedia && !Removable() && params->removableOrUnknownOnly)
		return false;
	
	if(media_changed)
		return true;
	
	bool haveMedia = false;
	if(devfd >= 0) {
		if(ioctl(devfd, B_GET_MEDIA_STATUS, &media_status) != B_NO_ERROR) {
			// old driver
			close(devfd);
			devfd = -1;
		}
	}
	if(devfd < 0)
		devfd = open(name, O_RDONLY | O_CLOEXEC);
	if(devfd >= 0) {
		if(media_status == B_NO_ERROR)
			haveMedia = true;

		// check for media with old driver
		if((ioctl(devfd, B_GET_GEOMETRY, &geometry) != B_NO_ERROR)
		   && (errno != B_DEV_UNREADABLE))
			haveMedia = false;
	}

	if(media_status == B_DEV_NOT_READY) {
		return false;
	}
	if(media_status == B_DEV_MEDIA_CHANGED) {
//		Dump("media changed");
		media_changed = true;
		return true;
	}
	if(media_status == B_DEV_MEDIA_CHANGE_REQUESTED) {
		Dump("eject request");
		media_changed = true;
		eject_request = true;
		return true;
	}

	return haveMedia != hadMedia;
}

bool
Device::FindMountedVolumes(void *castToParams)
{
	FindMountedVolumesParams *params = (FindMountedVolumesParams *)castToParams;
	const char *deviceName = Name();

	fs_info fileSystemDeviceInfo;
	int32 cookie = 0;

	// loop through all the volumes
	while (_kstatfs_(-1, &cookie, -1, NULL, &fileSystemDeviceInfo) == B_NO_ERROR) {			
		int32 len = strlen(fileSystemDeviceInfo.device_name);

		if (!len)
			continue;
			
		int32 sessionIndex = 0;
		int32 partitionIndex = 0;
		const char *volumeDeviceName = 0;
		partition_info partitionInfo;

		// get partition info for volume
		if (!strstr(fileSystemDeviceInfo.device_name, "/raw")) {
			int32 dev;
			dev = open(fileSystemDeviceInfo.device_name, O_RDONLY | O_CLOEXEC);
			if (dev >= 0) {

				if (ioctl(dev, B_GET_PARTITION_INFO, &partitionInfo) 
					== B_NO_ERROR) {
					volumeDeviceName = partitionInfo.device;
					sessionIndex = partitionInfo.session;
					partitionIndex = partitionInfo.partition;
				}
				else
					volumeDeviceName = fileSystemDeviceInfo.device_name;
				close(dev);
			}
		}
		else
			volumeDeviceName = fileSystemDeviceInfo.device_name;

//		PRINT(("matching %s with %s\n", deviceName, volumeDeviceName));

		// match volume info with a volume from the device list
		// ignoring /raw here??
		if (!volumeDeviceName
			|| strncmp(deviceName, volumeDeviceName, strlen(deviceName)))
			continue;

		// got a volume that matches this device
		// get entry for volume
		node_ref node;
		node.device = fileSystemDeviceInfo.dev;
		node.node = fileSystemDeviceInfo.root;
		BDirectory dir(&node);
		if (dir.InitCheck() != B_NO_ERROR)
			continue;
	
		BEntry entry;
		dir.GetEntry(&entry);
		if (entry.InitCheck() != B_NO_ERROR)
			continue;
		
		BPath volumePath;
		entry.GetPath(&volumePath);

		// update mounting info
		Session *session = sessionList.ItemAt(sessionIndex);
		ASSERT(session);
		if (!session)
			continue;
		Partition *partition = session->partitionList.ItemAt(partitionIndex);
		ASSERT(partition);
		if (!partition)
			continue;
//		PRINT(("got match, old mountpoint %s new %s\n", partition->MountedAt(),
//			volumePath.Path()));

		if (strcmp(partition->MountedAt(), volumePath.Path()) != 0
			|| partition->Mounted() != kMounted) {
			// has the mount state changed from last time?
			params->changed = true;
			partition->SetMountedAt(volumePath.Path());
			partition->SetMountState(kMounted);
			partition->SetVolumeDeviceID(fileSystemDeviceInfo.dev);
		}
	}	
	return false;
}


int32
Device::CountSessions() const
{ 
	return sessionList.CountItems();
}

Session *
Device::SessionAt(int32 index) const
{ 
	return sessionList.ItemAt(index); 
}

static Session *
CountPartitionsOne(Session *session, void *castToSum)
{
	*((int32 *)castToSum) += session->CountPartitions();
	return NULL;
}

int32
Device::CountPartitions() const
{
	int32 sum = 0;
	((Device *)this)->sessionList.EachElement(CountPartitionsOne, &sum);
	return sum;
}

int32 
Device::BlockSize() const
{
	return blockSize;
}

void 
Device::SetPartitioningFlags(drive_setup_partition_flags newFlags)
{
	partitioningFlags = newFlags;
}

const char *
Device::Name() const
{
	return name;
}

void
Device::BuildDisplayName(bool includeBusID, bool includeLUN)
{
	char *tmp;
	tmp = strstr(name, "/scsi/");
	if (tmp) {
		int busID, scsiID, lun;
		strcpy(shortName, "Invalid SCSI device");
		if (sscanf(tmp, "/scsi/%d/%d/%d/",
				&busID, &scsiID, &lun) != 3)
			return;
		if (includeBusID && includeLUN)
			sprintf(shortName, "SCSI bus:%d id:%d lun:%d", busID, scsiID, lun);
		else if (!includeBusID && includeLUN)
			sprintf(shortName, "SCSI id:%d lun:%d", scsiID, lun);
		else if (includeBusID && !includeLUN)
			sprintf(shortName, "SCSI bus:%d id:%d", busID, scsiID);
		else if (!includeBusID && !includeLUN)
			sprintf(shortName, "SCSI id:%d", scsiID);
		return;
	}
	tmp = strstr(name, "/ide/ata/");
	if (tmp) {
		bool master = strstr(name, "master") != NULL;
		tmp += strlen("/ide/ata/");
		sprintf(shortName, "IDE %s bus:%c", master ? "master" : "slave", *tmp);
		return;
	}
	tmp = strstr(name, "/ide/atapi/");
	if (tmp) {
		bool master = strstr(name, "master") != NULL;
		tmp += strlen("/ide/atapi/");
		sprintf(shortName, "IDE %s bus:%c", master ? "master" : "slave", *tmp);
		return;
	}
	tmp = strstr(name, "/ide/");
	if (tmp) {
		bool master = strstr(name, "master") != NULL;
		tmp += strlen("/ide/");
		sprintf(shortName, "IDE %s bus:%c", master ? "master" : "slave", *tmp);
		return;
	}
	// just strip the path and raw
	tmp = strstr(name, "/raw");
	char *src = name;
	if (strstr(name, "/dev/disk/"))
		src += strlen("/dev/disk/");
	
	if (!tmp)
		strcpy(src, shortName);
	else {
		char *dst = shortName;
		for(;;) 
			if (src == tmp)
				break;
			else
				*dst++ = *src++;
		*dst = '\0';
	}
}

const char *
Device::DisplayName(bool includeBusID, bool includeLUN) const
{
//	if (shortName[0] == '\0') 
		((Device *)this)->BuildDisplayName(includeBusID, includeLUN);
	
	return shortName;
}

bool 
DeviceList::UpdateMountingInfo()
{
	FindMountedVolumesParams params;
	params.changed = false;
	
	EachPartition(&Partition::SetOneUnknownMountState, &params);
	EachDevice(&Device::FindMountedVolumes, &params);
	EachPartition(&Partition::ClearOneMountState, &params);

	return params.changed;
}

status_t
DeviceList::RescanDevices(bool)
{
	MakeEmpty();
	status_t result = ScanDirectory("/dev/disk");

	return result;
}

status_t 
DeviceList::ScanDirectory(const char *path)
{
	status_t result;
	BDirectory directory(path);

	result = directory.InitCheck();
	if (result == B_NO_ERROR) {
		BEntry entry;

		directory.Rewind();
		while (directory.GetNextEntry(&entry) >= 0) {
			BPath name;

			entry.GetPath(&name);
			if (entry.IsDirectory()) {
				// descend into subdir
				result = ScanDirectory(name.Path());
				if (result != B_NO_ERROR)
					break;
			} else if (!strstr(name.Path(), "rescan")) {
				bool add = true;
				int32 dev;
				partition_info info;

				if ((dev = open(name.Path(), O_RDONLY | O_CLOEXEC)) >= 0) {
					if (ioctl(dev, B_GET_PARTITION_INFO, &info) == B_NO_ERROR)
						add = false;
				}
				if (add)
					AddItem(new Device(name.Path(), dev));
				else if(dev >= 0)
					close(dev);
			}
		}
	}
	return result;
}

bool 
DeviceList::EachDevice(EachDeviceMemberFunction func, void *params)
{
	int32 count = CountItems();
	for (int32 index = 0; index < count; index++) {
		Device *device = ItemAt(index);
		if ((device->*func)(params))
			return true;
	}
	return false;
}

Device *
DeviceList::EachDevice(EachDeviceFunction func, void *params)
{
	int32 count = CountItems();
	for (int32 index = 0; index < count; index++) {
		Device *device = ItemAt(index);
		if ((func)(device ,params))
			return device;
	}
	return 0;
}



struct OneIfDeviceStateChangedParams {
	DeviceScanParams *scanParams;
	void *passThruParams;
	EachDeviceFunction func;
};

bool
Device::OneIfDeviceStateChangedAdaptor(void *castToParams)
{
	OneIfDeviceStateChangedParams *params = 
		(OneIfDeviceStateChangedParams *)castToParams;
	
	if (DeviceStateChanged(params->scanParams))
		return (params->func)(this, params->passThruParams);
	
	return false;
}

bool 
DeviceList::EachChangedDevice(EachDeviceFunction func, 
	DeviceScanParams *scanParams, void *params)
{
	OneIfDeviceStateChangedParams adaptorParams;
	adaptorParams.func = func;
	adaptorParams.scanParams = scanParams;
	adaptorParams.passThruParams = params;
	
	return EachDevice(&Device::OneIfDeviceStateChangedAdaptor, &adaptorParams);
}

template<class Adaptor, class EachFunction, class ResultType, class ParamType>
ResultType
EachPartitionIterator<Adaptor, EachFunction, ResultType, ParamType>::EachPartition(
	DeviceList *list, EachFunction func, ParamType params)
{
	int32 absolutePartitionCount = 0;
	int32 deviceCount = list->CountItems();
	for (int32 deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
		Device *device = list->ItemAt(deviceIndex);
		int32 sessionCount = device->CountSessions();
		for (int32 sessionIndex = 0; sessionIndex < sessionCount; sessionIndex++) {
			Session *session = device->SessionAt(sessionIndex);
			ASSERT(session);
			int32 partitionCount = session->CountPartitions();
			for (int32 partitionIndex = 0; partitionIndex < partitionCount; 
				partitionIndex++) {
				Partition *partition = session->PartitionAt(partitionIndex);
				ASSERT(partition);
				ResultType result = Adaptor::Adapt(partition,
					absolutePartitionCount, func, params);
					
				if (result)
					// terminate early because each function says so
					return result;
				absolutePartitionCount++;
			}
		}
	}
	return (ResultType)0;
}

class EachPartitionAdaptor {
public:
	static Partition *Adapt(Partition *partition, int32 , EachPartitionFunction func,
		void *params)
	{
		return (func)(partition, params);
	}
};

Partition *
DeviceList::EachPartition(EachPartitionFunction func, void *params)
{
	return EachPartitionIterator<EachPartitionAdaptor,
		EachPartitionFunction, Partition *, void *>::EachPartition(this,
		func, params);
}

class EachPartitionMemberAdaptor {
public:
	static bool Adapt(Partition *partition, int32 , EachPartitionMemberFunction func,
		void *params)
	{
		return (partition->*func)(params);
	}
};

bool
DeviceList::EachPartition(EachPartitionMemberFunction func, void *params)
{
	return EachPartitionIterator<EachPartitionMemberAdaptor,
		EachPartitionMemberFunction, bool, void *>::EachPartition(this,
			func, params);
}

class EachMountedPartitionAdaptor {
public:
	static Partition *Adapt(Partition *partition, int32 , EachPartitionFunction func,
		void *params)
	{
		return (partition->Mounted() == kMounted) ? (func)(partition, params) : NULL;
	}
};

Partition *  
DeviceList::EachMountedPartition(EachPartitionFunction func, void *params)
{
	return EachPartitionIterator<EachMountedPartitionAdaptor,
		EachPartitionFunction, Partition *, void *>::EachPartition(this,
		func, params);
}

class EachMountablePartitionAdaptor {
public:
	static Partition *Adapt(Partition *partition, int32 , EachPartitionFunction func,
		void *params)
	{
//		partition->Dump("each mountable:");
		return (partition->Mounted() == kNotMounted
			&& (partition->GetDevice()->IsFloppy() ||
				(!partition->Hidden()
				&& strcmp(partition->FileSystemShortName(), "audio") != 0
				&& strcmp(partition->FileSystemShortName(), "unknown") != 0)))
					? (func)(partition, params) : NULL;
	}
};

class EachInitializablePartitionAdaptor {
public:
	static Partition *Adapt(Partition *partition, int32 , EachPartitionFunction func,
		void *params)
	{
//		partition->Dump("each initializable:");
		return (partition->Mounted() == kNotMounted
			&& !partition->Hidden()
			&& !partition->GetDevice()->ReadOnly()
			&& strcmp(partition->FileSystemShortName(), "unknown") == 0)
				? (func)(partition, params) : NULL;
	}
};

Partition *  
DeviceList::EachMountablePartition(EachPartitionFunction func, void *params)
{
	return EachPartitionIterator<EachMountablePartitionAdaptor,
		EachPartitionFunction, Partition *, void *>::EachPartition(this,
		func, params);
}

Partition *
DeviceList::EachInitializablePartition(EachPartitionFunction func, void *params)
{
	return EachPartitionIterator<EachInitializablePartitionAdaptor,
		EachPartitionFunction, Partition *, void *>::EachPartition(this,
		func, params);
}

static Partition *
OneMatchID(Partition *partition, void *castToID)
{
	if (partition->UniqueID() == (int32)castToID)
		return partition;

	return NULL;
}

Partition *  
DeviceList::PartitionWithID(int32 id)
{
	return EachPartition(OneMatchID, (void *)id);
}

static Device *
OneDeviceChanged(Device *device, void *)
{
	device->Dump("device changed:");
	return device;
}

static Device *
OneDeviceUpdate(Device *device, void *)
{
	device->Dump("updating device:");
	device->UpdateDeviceState();
	return NULL;
}

DeviceList::DeviceList()
	:	TypedList<Device *>(10, true)
{
}

DeviceList::~DeviceList()
{
}

bool 
DeviceList::CheckDevicesChanged(DeviceScanParams *params)
{
	return EachChangedDevice(OneDeviceChanged, params, NULL);
}

void 
DeviceList::UpdateChangedDevices(DeviceScanParams *params)
{
	EachChangedDevice(OneDeviceUpdate, params, NULL);
	UpdateMountingInfo();
}

struct HandleDisappearedParams {
	bool hitMissingPartition;
};

static Partition *
OneHandleIfDisappeared(Partition *partition, void *castToParams)
{
	HandleDisappearedParams *params = (HandleDisappearedParams *)castToParams;
	Device *device = partition->GetDevice();
	
	DeviceScanParams scanParams;
	scanParams.removableOrUnknownOnly = false;
	scanParams.checkFloppies = false;
	scanParams.checkCDROMs = true;
	scanParams.checkOtherRemovable = true;
	if (device->DeviceStateChanged(&scanParams)) {
		if (strcmp(partition->MountedAt(), "/boot") == 0) 
			PRINT(("/boot device unmounted, ignore for now\n"));
		else {
			PRINT(("whoa, volume mounted at %s gone\n", partition->MountedAt()));
			partition->Unmount();
			params->hitMissingPartition = true;
		}
	}
	return NULL;
}

bool
DeviceList::UnmountDisappearedPartitions()
{
	HandleDisappearedParams params;
	params.hitMissingPartition = false;
	EachMountedPartition(OneHandleIfDisappeared, &params);
	return params.hitMissingPartition;
}

#endif
