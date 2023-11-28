#ifndef __INTERNAL_GLUE__
#define __INTERNAL_GLUE__

// This file contains copy-pasted private headers that were needed to build
// installer independently of the BeOS build system.
// Care should be taken to make sure the structures in here are not out of date


#include <Drivers.h>
#include <Entry.h>
#include <List.h>
#include <Node.h>
#include <OS.h>


// from nvram.h ---------------------------
extern "C" int write_config_item (int item_id, void *buf);

#define CFG_boot_dev	0	/* (1) boot device code - see below */
#define CFG_boot_bus	1	/* (1) boot device bus */
#define CFG_boot_id		2	/* (1) boot device id */
#define CFG_boot_lun	3	/* (1) boot device logical unit */
#define CFG_boot_sid	4	/* (1) boot device sesion id */
#define CFG_boot_pid	5	/* (1) boot device partition id */

// from TypedList.h -----------------------

// this glue class is here to provide a better iterator function,
// support of owning and sorting with context
class PointerList : public BList {
public:
	PointerList(const PointerList &list);
	PointerList(int32 itemsPerBlock = 20, bool owning = false);
	virtual ~PointerList();

	typedef void *(* GenericEachFunction)(void *, void *);
	typedef int (* GenericCompareFunction)(const void *, const void *);
	void *EachElement(GenericEachFunction, void *);
	
	void SortItems(int (*cmp)(const void *, const void *, const void *), void *);
		// SortItems with context

	bool AddUnique(void *);
		// return true if item added or already in the list
	bool AddUnique(void *, GenericCompareFunction);

	bool Owning() const;
private:
	typedef BList _inherited;
	const bool owning;
};

// TypedList -
// adds proper typing, owning, sorting with context
// can be passed to calls with BList * parameters
template<class T>
class TypedList : public PointerList {
public:
	TypedList(int32 itemsPerBlock = 20, bool owning = false);
		// if <owning> passed as true, delete will delete all
		// the items first
	TypedList(const TypedList&);
	virtual ~TypedList();

	TypedList &operator=(const TypedList &);

	// iteration and sorting
	typedef T (* EachFunction)(T, void *);
	typedef const T (* ConstEachFunction)(const T, void *);
	typedef int (* CompareFunction)(const T *, const T *);
	typedef int (* CompareFunctionWithContext)(const T *, const T *, void *);

	// adding and removing
	bool AddItem(T);
	bool AddItem(T, int32);
	bool AddList(TypedList *);
	bool AddList(TypedList *, int32);
	bool AddUnique(T);
	bool AddUnique(T, CompareFunction);
	
	bool RemoveItem(T);
	T RemoveItem(int32);
	T RemoveItemAt(int32);
		// same as RemoveItem(int32), RemoveItem does not work when T is a scalar

	void MakeEmpty();

	// item access
	T ItemAt(int32) const;
	T ItemAtFast(int32) const;
		// does not do an index range check

	T FirstItem() const;
	T LastItem() const;
	
	T Items() const;

	// misc. getters
	int32 IndexOf(const T) const;
	bool HasItem(const T) const;
	bool IsEmpty() const;
	int32 CountItems() const;


	T EachElement(EachFunction, void *);
	const T EachElement(ConstEachFunction, void *) const;
		// Do for each are obsoleted by this list, possibly add
		// them for convenience
	void SortItems(CompareFunction);
	void SortItems(CompareFunctionWithContext, void *);
	bool ReplaceItem(int32 index, T item);
	bool SwapItems(int32 a, int32 b);
	bool MoveItem(int32 from, int32 to);

private:
	typedef PointerList _inherited;
	friend class ParseArray;
};

template<class T> 
TypedList<T>::TypedList(int32 itemsPerBlock, bool owning)
	:	PointerList(itemsPerBlock, owning)
{
	ASSERT(!owning);
}

template<class T> 
TypedList<T>::TypedList(const TypedList<T> &list)
	:	PointerList(list)
{
	ASSERT(!list.Owning());
	// copying owned lists does not work yet
}

template<class T> 
TypedList<T>::~TypedList()
{
	if (Owning())
		// have to nuke elements first
		MakeEmpty();
}

template<class T> 
TypedList<T> &
TypedList<T>::operator=(const TypedList<T> &from)
{
	ASSERT(!from.Owning());
	// copying owned lists does not work yet

	return (TypedList<T> &)_inherited::operator=(from);
}

template<class T> 
bool 
TypedList<T>::AddItem(T item)
{
	return _inherited::AddItem((void *)item);
		// cast needed for const flavors of T
}

template<class T> 
bool 
TypedList<T>::AddItem(T item, int32 atIndex)
{
	return _inherited::AddItem((void *)item, atIndex);
}

template<class T> 
bool 
TypedList<T>::AddList(TypedList<T> *newItems)
{
	return _inherited::AddList(newItems);
}

template<class T> 
bool
TypedList<T>::AddList(TypedList<T> *newItems, int32 atIndex)
{
	return _inherited::AddList(newItems, atIndex);
}

template<class T>
bool 
TypedList<T>::AddUnique(T item)
{
	return _inherited::AddUnique((void *)item);
}

template<class T>
bool 
TypedList<T>::AddUnique(T item, CompareFunction function)
{
	return _inherited::AddUnique((void *)item, (GenericCompareFunction)function);
}


template<class T> 
bool 
TypedList<T>::RemoveItem(T item)
{
	bool result = _inherited::RemoveItem((void *)item);
	
	if (result && Owning()) {
		delete item;
	}

	return result;
}

template<class T> 
T 
TypedList<T>::RemoveItem(int32 index)
{
	return (T)_inherited::RemoveItem(index);
}

template<class T> 
T 
TypedList<T>::RemoveItemAt(int32 index)
{
	return (T)_inherited::RemoveItem(index);
}

template<class T> 
T 
TypedList<T>::ItemAt(int32 index) const
{
	return (T)_inherited::ItemAt(index);
}

template<class T> 
T 
TypedList<T>::ItemAtFast(int32 index) const
{
	return (T)_inherited::ItemAtFast(index);
}

template<class T> 
int32 
TypedList<T>::IndexOf(const T item) const
{
	return _inherited::IndexOf((void *)item);
}

template<class T> 
T 
TypedList<T>::FirstItem() const
{
	return (T)_inherited::FirstItem();
}

template<class T> 
T 
TypedList<T>::LastItem() const
{
	return (T)_inherited::LastItem();
}

template<class T> 
bool 
TypedList<T>::HasItem(const T item) const
{
	return _inherited::HasItem((void *)item);
}

template<class T> 
bool 
TypedList<T>::IsEmpty() const
{
	return _inherited::IsEmpty();
}

template<class T> 
int32 
TypedList<T>::CountItems() const
{
	return _inherited::CountItems();
}

template<class T> 
void 
TypedList<T>::MakeEmpty()
{
	if (Owning()) {
		int32 numElements = CountItems();
		
		for (int32 count = 0; count < numElements; count++)
			// this is probably not the most efficient, but
			// is relatively indepenent of BList implementation
			// details
			RemoveItem(LastItem());
	}
	_inherited::MakeEmpty();
}

template<class T> 
T
TypedList<T>::EachElement(EachFunction func, void *params)
{ 
	return (T)_inherited::EachElement((GenericEachFunction)func, params); 
}


template<class T> 
const T
TypedList<T>::EachElement(ConstEachFunction func, void *params) const
{ 
	return (const T)
		const_cast<TypedList<T> *>(this)->_inherited::EachElement(
		(GenericEachFunction)func, params); 
}


template<class T>
T
TypedList<T>::Items() const
{
	return (T)_inherited::Items();
}

template<class T> 
void
TypedList<T>::SortItems(CompareFunction function)
{ 
	ASSERT(sizeof(T) == sizeof(void *));
	BList::SortItems((GenericCompareFunction)function);
}

template<class T> 
void
TypedList<T>::SortItems(CompareFunctionWithContext function,
	void *params)
{ 
	ASSERT(sizeof(T) == sizeof(void *));
	_inherited::SortItems((int (*)(const void *,
		const void *, const void *))function, params);
}

template<class T>
bool TypedList<T>::ReplaceItem(int32 index, T item)
{
	return PointerList::ReplaceItem(index, (void *)item);
}

template<class T>
bool TypedList<T>::SwapItems(int32 a, int32 b)
{
	return PointerList::SwapItems(a, b);
}

template<class T>
bool TypedList<T>::MoveItem(int32 from, int32 to)
{
	return PointerList::MoveItem(from, to);
}

// from drive_setup.h ------------------------------

#ifdef __cplusplus
extern "C" {
#endif

// from partition.h ------------------------------

typedef struct {
	uint64	offset;				/* in device blocks */
	uint64	blocks;
	bool	data;				/* audio or data session */
} session_data;

typedef struct {
	char	partition_name[B_FILE_NAME_LENGTH];
	char	partition_type[B_FILE_NAME_LENGTH];
	char	file_system_short_name[B_FILE_NAME_LENGTH];
	char	file_system_long_name[B_FILE_NAME_LENGTH];
	char	volume_name[B_FILE_NAME_LENGTH];
	char	mounted_at[B_FILE_NAME_LENGTH];
	uint32	logical_block_size;
	uint64	offset;				/* in logical blocks from start of session */
	uint64	blocks;				/* in logical blocks */
	bool	hidden;				/* non-file system partition */
	uchar	partition_code;
	bool	reserved1;
	uint32	reserved2;
} partition_data;

#define DS_SESSION_ADDONS	"drive_setup/session/"
#define DS_PART_ADDONS		"drive_setup/partition/"
#define DS_FS_ADDONS		"drive_setup/fs/"


/* Session add-on entry points */
/*-----------------------------*/

_EXPORT status_t	ds_get_nth_session	(int32 /* dev */, int32 /* index */,
								 int32 /*block_size */, session_data*);

#define DS_GET_NTH_SESSION	"ds_get_nth_session"


/* Partition add-on entry points */
/*-------------------------------*/

typedef struct {
	bool	can_partition;
	bool	can_repartition;
} drive_setup_partition_flags;

_EXPORT bool		ds_partition_id		(uchar* /* sb */, int32 /* block_size */);
_EXPORT char*		ds_partition_name	(void);
_EXPORT status_t	ds_get_nth_map		(int32 /* dev */, uchar* /* sb */,
								 uint64 /* block_num */, int32 /* block_size */,
								 int32 /* index */, partition_data*);
_EXPORT void		ds_partition_flags	(drive_setup_partition_flags*);
_EXPORT void		ds_partition		(BMessage*);
_EXPORT status_t	ds_update_map		(int32 /* dev */, int32 /* index */,
								 partition_data*);

#define DS_PARTITION_ID		"ds_partition_id"
#define DS_PARTITION_NAME	"ds_partition_name"
#define DS_PARTITION_MAP	"ds_get_nth_map"
#define DS_PARTITION_FLAGS	"ds_partition_flags"
#define DS_PARTITION		"ds_partition"
#define DS_UPDATE_MAP		"ds_update_map"


/* File system add-on entry points */
/*---------------------------------*/

typedef struct {
	bool	can_initialize;
	bool	has_options;
} drive_setup_fs_flags;

_EXPORT bool	ds_fs_id			(partition_data*, int32 /* dev */,
							 uint64 /* offset */, int32 /* block_size */);
_EXPORT void	ds_fs_flags			(drive_setup_fs_flags*);
_EXPORT void	ds_fs_initialize	(BMessage*);

#define DS_FS_ID			"ds_fs_id"
#define DS_FS_FLAGS			"ds_fs_flags"
#define DS_FS_INITIALIZE	"ds_fs_initialize"

#ifdef __cplusplus
}
#endif

// from DeviceMap.h --------------------------------

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

	// the following two functions are hacks to support fs in file partitions
	void AddSession(Session *session)
		{ sessionList.AddItem(session); }
	
	Device()
		:	devfd(-1),
			largeIcon(NULL),
			miniIcon(NULL),
			readOnly(false),
			removable(false),
			isFloppy(false),
			media_changed(false),
			eject_request(false),
			blockSize(0)
		{
			name[0] = '\0';
			shortName[0] = '\0';
		}

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

class DeviceList : public TypedList<Device *> {
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

namespace BPrivate {

void AddVirtualFileSystems(DeviceList *);

}

#define bootdev_ata  0x00
#define bootdev_scsi 0x01
#define bootdev_atapi 0x02

#define kChecksumAttrName "META:md4_checksum"
const unsigned long kChecksumAttrType = 'i128';

class MD4Checksum {
public:

	inline MD4Checksum();
	inline void	Reset();
	inline void Process(char *data, size_t size);
	inline void	GetResult(char[16]);
	inline bool Equals(char[16]);

private:
	inline void	Process16Bytes(const char[16]);	

	char partialChunk[16];
	int partialChunkOffs;
	unsigned long aa, bb, cc, dd;
};

inline MD4Checksum::MD4Checksum() 
{
	Reset();
}

inline void 
MD4Checksum::Reset()
{
	partialChunkOffs = 0;
	
	aa = 0x01 | (0x23 << 8) | (0x45 << 16) | (0x67 << 24);
	bb = 0x89 | (0xab << 8) | (0xcd << 16) | (0xef << 24);
	cc = 0xfe | (0xdc << 8) | (0xba << 16) | (0x98 << 24);
	dd = 0x76 | (0x54 << 8) | (0x32 << 16) | (0x10 << 24);
}

#define F(x,y,z) (((x) & (y)) | ((~x) & (z)))
#define G(x,y,z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x,y,z) ((x) ^ (y) ^ (z))
#define ROL(x, n) (((x) << (n)) | ((x) >> (32-(n)))) 
#define R1(a,b,c,d,k,s) (a) = ROL(((a) + F((b),(c),(d)) + block[k]), (s))
#define R2(a,b,c,d,k,s) (a) = ROL((((a) + G((b),(c),(d)) + block[k] + 0x5a827999)), (s))
#define R3(a,b,c,d,k,s) (a) = ROL((((a) + H((b),(c),(d)) + block[k] + 0x6ed9eba1)), (s))

inline void
MD4Checksum::Process16Bytes(const char block[16])
{
	register unsigned long a = aa;
	register unsigned long b = bb;
	register unsigned long c = cc;
	register unsigned long d = dd;

	R1(a,b,c,d,0,3);  R1(d,a,b,c,1,7);  R1(c,d,a,b,2,11);  R1(b,c,d,a,3,19);
	R1(a,b,c,d,4,3);  R1(d,a,b,c,5,7);  R1(c,d,a,b,6,11);  R1(b,c,d,a,7,19);
	R1(a,b,c,d,8,3);  R1(d,a,b,c,9,7);  R1(c,d,a,b,10,11); R1(b,c,d,a,11,19);
	R1(a,b,c,d,12,3); R1(d,a,b,c,13,7); R1(c,d,a,b,14,11); R1(b,c,d,a,15,19);

	R2(a,b,c,d,0,3);  R2(d,a,b,c,4,5);  R2(c,d,a,b,8,9);   R2(b,c,d,a,13,13);
	R2(a,b,c,d,1,3);  R2(d,a,b,c,5,5);  R2(c,d,a,b,9,9);   R2(b,c,d,a,13,13);
	R2(a,b,c,d,2,3);  R2(d,a,b,c,6,5);  R2(c,d,a,b,10,9);  R2(b,c,d,a,14,13);
	R2(a,b,c,d,3,3);  R2(d,a,b,c,7,5);  R2(c,d,a,b,11,9);  R2(b,c,d,a,15,13);

	R3(a,b,c,d,0,3);  R3(d,a,b,c,8,9);  R3(c,d,a,b,4,11);  R3(b,c,d,a,12,15);
	R3(a,b,c,d,2,3);  R3(d,a,b,c,10,9); R3(c,d,a,b,6,11);  R3(b,c,d,a,14,15);
	R3(a,b,c,d,1,3);  R3(d,a,b,c,9,9);  R3(c,d,a,b,5,11);  R3(b,c,d,a,13,15);
	R3(a,b,c,d,3,3);  R3(d,a,b,c,11,9); R3(c,d,a,b,7,11);  R3(b,c,d,a,15,15);

	aa += a;
	bb += b;
	cc += c;
	dd += d;
}

inline void 
MD4Checksum::Process(char *data, size_t size)
{
	// Fill in any remaining chunks
	if (partialChunkOffs > 0) {
		while (size > 0) {
			partialChunk[partialChunkOffs++] = *data++;
			size--;
			
			if (partialChunkOffs == 16) {
				Process16Bytes(partialChunk);
				partialChunkOffs = 0;
				break;
			}
		}		
	}
	
	// Chunk through whole chunks
	while (size >= 16) {
		Process16Bytes(data);
		data += 16;
		size -= 16;	
	}

	// Fill in remaining data
	while (size > 0) {
		partialChunk[partialChunkOffs++] = *data++;
		size--;
	}
}

#define GET_BYTE(a, b) ((a >> (8 * b)) & 0xff)

inline void 
MD4Checksum::GetResult(char buf[16])
{
	if (partialChunkOffs > 0) {
		while (partialChunkOffs < 16)
			partialChunk[partialChunkOffs++] = 0;
			
		Process16Bytes(partialChunk);
		partialChunkOffs = 0;
	}

	buf[0]  = GET_BYTE(aa, 0);
	buf[1]  = GET_BYTE(aa, 1);
	buf[2]  = GET_BYTE(aa, 2);
	buf[3]  = GET_BYTE(aa, 3);
	buf[4]  = GET_BYTE(bb, 0);
	buf[5]  = GET_BYTE(bb, 1);
	buf[6]  = GET_BYTE(bb, 2);
	buf[7]  = GET_BYTE(bb, 3);
	buf[8]  = GET_BYTE(cc, 0);
	buf[9]  = GET_BYTE(cc, 1);
	buf[10] = GET_BYTE(cc, 2);
	buf[11] = GET_BYTE(cc, 3);
	buf[12] = GET_BYTE(dd, 0);
	buf[13] = GET_BYTE(dd, 1);
	buf[14] = GET_BYTE(dd, 2);
	buf[15] = GET_BYTE(dd, 3);
}

inline bool 
MD4Checksum::Equals(char sum[16])
{
	char myResult[16];
	GetResult(myResult);
	return (memcmp(myResult, sum, 16) == 0);
}


namespace BPrivate {

class BInfoWindow;

#ifndef _STANDALONE_INSTALLER_BUILD_
#define _CopyLoopControl CopyLoopControl
#endif

class _CopyLoopControl {
public:
	virtual ~_CopyLoopControl();
	virtual bool FileError(const char *message, const char *name, bool allowContinue) = 0;
		// inform that a file error occurred while copying <name>
		// returns true if user decided to continue

	virtual void UpdateStatus(const char *name, entry_ref ref, int32 count, 
		bool optional = false) = 0;

	virtual bool CheckUserCanceled() = 0;
		// returns true if canceled
	
	enum OverwriteMode {
		kSkip,				// do not replace, go to next entry
		kReplace,			// remove entry before copying new one
		kMerge				// for folders: leave existing folder, update contents leaving
							//  nonconflicting items
							// for files: save original attributes on file.
	};

	virtual OverwriteMode OverwriteOnConflict(const BEntry *srcEntry, 
		const char *destName, const BDirectory *destDir, bool srcIsDir, 
		bool dstIsDir) = 0;
		// override to always overwrite, never overwrite, let user decide, 
		// compare dates, etc.
	
	virtual bool SkipEntry(const BEntry *, bool file) = 0;
		// override to prevent copying of a given file or directory

	virtual void ChecksumChunk(const char *block, size_t size);
		// during a file copy, this is called every time a chunk of data
		// is copied.  Users may override to keep a running checksum.
	
	virtual bool ChecksumFile(const entry_ref*);
		// This is called when a file is finished copying.  Users of this
		// class may override to verify that the checksum they've been
		// computing in ChecksumChunk matches.  If this returns true,
		// the copy will continue.  If false, if will abort.

	virtual bool SkipAttribute(const char *attributeName);
	virtual bool PreserveAttribute(const char *attributeName);
};


class TrackerCopyLoopControl : public _CopyLoopControl {
public:
	TrackerCopyLoopControl(thread_id);
	virtual ~TrackerCopyLoopControl() {}

	virtual bool FileError(const char *message, const char *name, bool allowContinue);
		// inform that a file error occurred while copying <name>
		// returns true if user decided to continue

	virtual void UpdateStatus(const char *name, entry_ref ref, int32 count, bool optional = false);

	virtual bool CheckUserCanceled();
		// returns true if canceled
	
	virtual OverwriteMode OverwriteOnConflict(const BEntry *srcEntry, 
		const char *destName, const BDirectory *destDir, bool srcIsDir, 
		bool dstIsDir);

	virtual bool SkipEntry(const BEntry *, bool file);
		// override to prevent copying of a given file or directory

	virtual bool SkipAttribute(const char *attributeName);

private:
	thread_id fThread;
};

inline 
TrackerCopyLoopControl::TrackerCopyLoopControl(thread_id thread)
	:	fThread(thread)
	{
	}

#if __GNUC__
// using std::stat instead of just stat here because of what
// seems to be a gcc bug involving namespace and struct stat interaction
typedef struct std::stat StatStruct;
#else
// on mwcc std isn't turned on but there is no bug either.
typedef struct stat StatStruct;
#endif


status_t FSCopyFile(BEntry *, StatStruct *, BDirectory *, _CopyLoopControl*, 
	BPoint * = 0, bool = false);
status_t FSCopyFolder(BEntry *, BDirectory *, _CopyLoopControl *, BPoint * = 0,
	bool = false);
void FSMakeOriginalName(char* name, BDirectory* destDir, const char* suffix);
status_t FSDeleteFolder(BEntry *, _CopyLoopControl *, bool updateStatus, 
	bool deleteTopDir = true, bool upateFileNameInStatus = false);

enum ReadAttrResult {
	kReadAttrFailed,
	kReadAttrNativeOK,
	kReadAttrForeignOK
};

#define	kAttrPoseInfo_be				"_trk/pinfo"
#define	kAttrPoseInfo_le				"_trk/pinfo_le"
#if B_HOST_IS_LENDIAN
#define	kAttrPoseInfo					kAttrPoseInfo_le
#define	kAttrPoseInfoForeign			kAttrPoseInfo_be
#else
#define	kAttrPoseInfo					kAttrPoseInfo_be
#define	kAttrPoseInfoForeign			kAttrPoseInfo_le
#endif


} // namespace BPrivate

using namespace BPrivate;

#endif
