// (c) 1997 Be, Incorporated
//
// volume management utilities
//

#ifndef __DEVICE_MAP__
#define __DEVICE_MAP__

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include "TypedList.h"
#include <drive_setup.h>
#include <Drivers.h>
#include <Entry.h>
#include <Node.h>
#include <OS.h>

#if _INCLUDES_CLASS_DEVICE_MAP
#define _DEVICE_MAP_ONLY(x) x
#else
#define _DEVICE_MAP_ONLY(x) 
#endif

class Bitmap;

enum {
	P_UNKNOWN = 0, 
	P_UNREADABLE, 
	P_ADD_ON, 
	P_AUDIO
};

enum MountState {
	kMounted,
	kNotMounted,
	kUnknown
};

class Device;
class Session;
class Partition;

typedef bool (Partition::*EachPartitionMemberFunction)(void *);
typedef Partition *(*EachPartitionFunction)(Partition *, void *);
typedef bool (Device::*EachDeviceMemberFunction)(void *);
typedef Device *(*EachDeviceFunction)(Device *, void *);


// structure used to communicate between the client application,
// the fs addon and the Initialize call
struct InitializeControlBlock {
	BWindow *window;				// window is used by the addon to
									//	center it's dialog over and send the
									//	result message to
	sem_id completionSemaphore;		// semaphore used to block the Initialize
									//	call while waiting for the user to
									//	interact with the addon dialog and the
									//	initialization to finish
	int32 completionMessage;		// the message that will be sent back to
									//	window upon addon completion
	bool cancelOrFail;				// the receiving window finds result codes
									//	from the addon and stuffs them in here
									//	for the Initialize call to use currently
									//	only bool is returned by the addon
};


class Partition {
public:
	Partition(Session *, const char *name, const char *type, 
		const char *fsShortName, const char *fsLongName, 
		const char *volumeName, const char *mountedAt,
		uint32 logicalBlockSize, uint64 offset, uint64 blocks, 
		bool hidden = false);
	Partition(Session *, uint32 logicalBlockSize, uint64 offset, 
		uint64 blocks, bool hidden = false);
	Partition(Session *, const partition_data &);

	// getters/setters for partition info
	void SetName(const char *);
	const char *Name() const;
	void SetType(const char *);
	const char *Type() const;
	void SetFileSystemShortName(const char *);
	const char *FileSystemShortName() const;
	void SetFileSystemLongName(const char *);
	const char *FileSystemLongName() const;
	void SetVolumeName(const char *);
	const char *VolumeName() const;
	void SetMountedAt(const char *);
	const char *MountedAt() const;

	status_t GetMountPointNodeRef(node_ref*) const;
	void SetMountPointNodeRef(const node_ref*);

	MountState Mounted() const;
	void SetMountState(MountState state);
	
	uint32 LogicalBlockSize() const;
	uint64 Offset() const;
	uint64 Blocks() const;
	bool Hidden() const;

	Session *GetSession() const;
		// the session the partition is on
	Device *GetDevice() const;
		// the device the partition and it's session are on

	bool BuildFileSystemInfo(void *params);
		// set up the files system short and long names for this partition
	bool RebuildFileSystemInfo();
		// set up the files system short and long names for this partition

	partition_data *Data();
		// accessor to the low level data for low level calls

	bool IsBFSDisk() const;
	bool IsHFSDisk() const;
	bool IsOFSDisk() const;

	int32 Index() const;

	// utility calls for updating mounting info
	bool SetOneUnknownMountState(void *);
	bool ClearOneMountState(void *);

	// here is the real stuff
	status_t Mount(int32 mountflags = 0, void *params = NULL, int32 len = 0);
	status_t Unmount();
	status_t Initialize(InitializeControlBlock *params,
		const char *fileSystem = "bfs");
		// the drive setup addon call protocol requires a lot of extra stuff:
		// you have to pass a window onto which the drive setup addon will
		// center itself. Upon completion it will send it a message with
		// <completionMessage> signature; the MessageReceived in the window
		// needs to release <completionSemaphore> to unblock the call
	
	int32 UniqueID() const;
		// returns a number uinque to the volume in a given device list
		// used to identify unmounted volumes

	dev_t VolumeDeviceID() const;
	void SetVolumeDeviceID(dev_t);
		// only available for mounted volumes

	status_t AddVirtualDevice();
		// publishes a device in /dev...

	void Dump(const char *includeThisText = "");

private:
	status_t AddVirtualDevice(char *device);
		// fills out <device> with the path to the device driver in /dev...
	void InitialMountPointName(char *);

	partition_data data;
	Session *session;
	MountState mounted;
	
	int32 partitionUniqueID;
	dev_t volumeDeviceID;

	node_ref mountPointNodeRef;
	status_t mountPointNodeRefStatus;
	static int32 lastUniqueID;
};


class Session {
public:
	Session(Device *device, const char *, uint64 offset, uint64 blocks, 
		bool data);

	uint64 Blocks() const;

	void AddPartition(Partition *);
	void SetName(const char *);
	const char *Name() const;
	void SetType(int32 type);

	uint64 Offset() const;

	Device *GetDevice() const;

	int32 CountPartitions() const;
	Partition *PartitionAt(int32 index) const;
	bool VirtualPartitionOnly() const;


	void SetAddOnEntry(const BEntry *entry);
		// the addon that knows how to handle this session

	bool BuildPartitionMap(int32 dev, uchar *block, bool singlePartition);
		// adds one or more partitions to the list
		// returns true if multiple partitions found
		// pass false in <singlePartition> if device cannot support
		// multiple partitions (floppy)

	bool BuildFileSystemInfo(int32 dev);

	int32 Index() const;

	bool EachPartition(EachPartitionMemberFunction, void *);
		// return true if terminated early

	bool IsDataSession() const;

	static status_t GetSessionData(int32 dev, int32 index, 
		int32 blockSize, session_data *session);
private:


	uint64 offset;
	uint64 blocks;
	char name[B_OS_NAME_LENGTH];	// map name
	int32 type;
	bool data;
	bool virtualPartitionOnly;
	BEntry add_on;	// we probably don't need this
	TypedList<Partition *> partitionList;
	Device *device;

friend class Partition;
friend class Device;
};

struct DeviceScanParams {
	bigtime_t shortestRescanHartbeat;
	bool removableOrUnknownOnly;
	bool checkFloppies;
	bool checkCDROMs;
	bool checkOtherRemovable;
};

class Device {
public:
	Device(const char *path, int devfd = -1);
	~Device();

	int32 CountSessions() const;
	Session *SessionAt(int32 index) const;

	int32 CountPartitions() const;
	
	int32 BlockSize() const;

	void SetPartitioningFlags(drive_setup_partition_flags newFlags);

	const char *Name() const;
		// device name, including path
	const char *DisplayName(bool includeBusID = true, 
		bool includeLUN = false) const;

	Session *NewSession(int32 dev, int32 index);

	bool FindMountedVolumes(void *);
	bool ReadOnly() const
		{ return readOnly; }
	bool Removable() const
		{ return removable; }

	void UpdateDeviceState();
	status_t Eject();

	bool NoMedia() const;

	void Dump(const char *);

	bool Dump(void *);
		// each function dump
	
	bool DeviceStateChanged(void *params);
	
	bool IsFloppy() const;

private:
	void InitNewDeviceState();
	void KillOldDeviceState();

	bool OneIfDeviceStateChangedAdaptor(void *params);

	void BuildDisplayName(bool includeBusID, bool includeLUN);
	
	char name[B_FILE_NAME_LENGTH];
	char shortName[B_FILE_NAME_LENGTH];
	int devfd;
	drive_setup_partition_flags	partitioningFlags;
	BBitmap *largeIcon;
	BBitmap *miniIcon;

	bool readOnly;
	bool removable;
	bool isFloppy;
	bool media_changed;
	bool eject_request;
	
	int32 blockSize;

	TypedList<Session *> sessionList;

friend class Session;
friend class DeviceList;
};


class DeviceList;
class EachPartitionAdaptor;
class EachPartitionMemberAdaptor;
class EachMountablePartitionAdaptor;
class EachInitializablePartitionAdaptor;
class EachMountedPartitionAdaptor;

template<class Adaptor, class EachFunction, class ResultType, class ParamType>
class EachPartitionIterator {
public:
	static ResultType EachPartition(DeviceList *, EachFunction func,
		ParamType params);
};

class DeviceList : private TypedList<Device *> {
public:
	DeviceList();
	~DeviceList();

	status_t RescanDevices(bool runRescanDriver = true);
	bool UpdateMountingInfo();
		// returns true if there was a change

	bool EachDevice(EachDeviceMemberFunction, void *);
		// return true if terminated early
	Device *EachDevice(EachDeviceFunction, void *);
		// return true if terminated early

	Partition *EachPartition(EachPartitionFunction func, void *params);
	// return Partition * if terminated early

	bool EachPartition(EachPartitionMemberFunction func, void *params);
	// return true if terminated early

	Partition *EachMountedPartition(EachPartitionFunction, void *);
	Partition *EachMountablePartition(EachPartitionFunction, void *);
	Partition *EachInitializablePartition(EachPartitionFunction, void *);
	
	Partition *PartitionWithID(int32);

	bool CheckDevicesChanged(DeviceScanParams *);
	void UpdateChangedDevices(DeviceScanParams *);
	bool UnmountDisappearedPartitions();
		// ToDo: pass a hook function for alerting user
	
private:

	bool EachChangedDevice(EachDeviceFunction, DeviceScanParams *, void *);
		// iterate through every device that is out of sync with current state
		// used to sync when media changes, etc.

	status_t ScanDirectory(const char *path);

friend class EachPartitionIterator<EachPartitionAdaptor,
			EachPartitionFunction, Partition *, void *>;
friend class EachPartitionIterator<EachMountedPartitionAdaptor,
			EachPartitionFunction, Partition *, void *>;
friend class EachPartitionIterator<EachMountablePartitionAdaptor,
			EachPartitionFunction, Partition *, void *>;
friend class EachPartitionIterator<EachInitializablePartitionAdaptor,
			EachPartitionFunction, Partition *, void *>;
friend class EachPartitionIterator<EachPartitionMemberAdaptor,
			EachPartitionMemberFunction, bool, void *>;
};


#endif
