// (c) 1997 Be Incorporated
//
//
// You said "mounting tool", heh heh, heh heh
//

#include <Application.h>
#include <Debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <private/storage/DeviceMap.h>

bool silent = false;
int mountflags = 0;

const char *format = "%-20s %-20s %-20s %-8s\n";
const char *hiddenFormat = "%-20s %-20s %-20s %-8s %-8s\n";

static Partition *
DumpOnePartition(Partition *partition, void *)
{
	printf(format,
		partition->VolumeName(),
		partition->GetDevice()->DisplayName(),
		partition->MountedAt(),
		partition->FileSystemShortName());
	
	return NULL;
}

static Partition *
DumpOnePartitionInclHidden(Partition *partition, void *)
{
	printf(hiddenFormat,
		partition->VolumeName(),
		partition->GetDevice()->DisplayName(),
		partition->MountedAt(),
		partition->FileSystemShortName(),
		partition->Hidden() ? "hidden" : ""
		);
	
	return NULL;
}

static Partition *
TryMountingEveryOne(Partition *partition, void *)
{
	if (partition->Mounted() == kMounted) {
		if (!silent)
			printf("%s already mounted\n", partition->VolumeName());
	} else {
		status_t result = partition->Mount(mountflags);
		if (!silent) {
			if (result == B_NO_ERROR)
				printf("%s mounted OK\n", partition->VolumeName());
			else
				printf("Error '%s' mounting %s\n",
					strerror(result), partition->VolumeName());
		}
	}
	return NULL;
}

static Partition *
TryMountingOne(Partition *partition, void *castToName)
{
	const char *name = (const char *)castToName;
	
	if (strcmp(name, partition->VolumeName()) == 0) {
		Partition *result = TryMountingEveryOne(partition, NULL);
		if (result)
			return result;
		return partition;
	}
	return NULL;
}

static Partition *
TryMountingOneFS(Partition *partition, void *cookie)
{
	char *fs = (char *)cookie;
	if (strcmp(partition->FileSystemShortName(), fs) == 0) 
		return TryMountingEveryOne(partition, NULL);

	return NULL;
}

static Partition *
TryUnmountingOne(Partition *partition, void *castToName)
{
	const char *name = (const char *)castToName;
	
	if (strcmp(name, partition->MountedAt()) == 0) {
		status_t result = partition->Unmount();
		if (!silent) {
			if (result == B_NO_ERROR)
				printf("%s unmounted OK\n", name);
			else
				printf("Error '%s' unmounting %s\n",
					strerror(result), name);
		}
		return partition;
	}
	return NULL;
}

static Partition *
PublishEveryOne(Partition *partition, void *)
{
	status_t result = partition->AddVirtualDevice();

	// partition_data *data = partition->Data();
	// printf("partition: %s type %s offset %Ld blocks %Ld\n",
	//        partition->Name(), partition->Type(), data->offset,
	//        data->blocks);
	  
	if (!silent && result != B_NO_ERROR)
		printf("Error '%s' publishing %s\n", strerror(result),
			partition->VolumeName());

	return NULL;
}

static Partition *
PublishOne(Partition *partition, void *cookie)
{
	char *fs = (char *)cookie;
	if (strcmp(partition->FileSystemShortName(), fs) != 0)
		return NULL;

	status_t result = partition->AddVirtualDevice();
	if (!silent && result != B_NO_ERROR)
		printf("Error '%s' publishing %s\n", strerror(result),
			partition->VolumeName());

	return NULL;
}

struct AutomountParams {
	bool mountAll;
	bool mountBFS;
	bool mountHFS;
	bool mountDOS;
};

static Partition *
Automount(Partition *partition, void *castToParams)
{
	AutomountParams *params = (AutomountParams *)castToParams;

	if (params->mountAll)
		return TryMountingEveryOne(partition, NULL);
	if (params->mountBFS)
		return TryMountingOneFS(partition, const_cast<char *>("bfs"));
	if (params->mountHFS)
		return TryMountingOneFS(partition, const_cast<char *>("hfs"));
	if (params->mountDOS)
		return TryMountingOneFS(partition, const_cast<char *>("dos"));
	
	return NULL;
}

const char *usage =	"[-r][-p][-s][-allbfs][-allhfs][-alldos][-all][-ro] [-unmount mountname ...] [volname ...] \n"
					"\t-r ... rescan\n"
					"\t-s ... silent\n"
					"\t-all ... mount all volumes\n"
					"\t-allbfs ... mount all bfs volumes\n"
					"\t-allhfs ... mount all hfs volumes\n"
					"\t-alldos ... mount all dos volumes\n"
					"\t-ro ... mount volume read-only\n"
					"\t-unmount ... unmount specified volume[s]\n"
					"\t-p ... list mounted and mountable volumes\n"
					"\t-l ... list mounted and mountable volumes\n"
					"\t-lh ... list all volumes\n"
					"\t-dd ... list all devices\n"
					"\t--help ... show this message\n"
					"\t-publishall ... create entries in /dev folder for all unmounted volumes\n"
					"\t-publishbfs ... create entries in /dev folder for unmounted bfs volumes\n"
					"\t-publishhfs ... create entries in /dev folder for unmounted hfs volumes\n"
					"\t-publishdos ... create entries in /dev folder for unmounted dos volumes\n"
					;
int
main(int argc, char *argv[])
{
	bool dumpVolumes = false;
	bool rescan = false;
	bool showHidden = false;
	bool mountAll = false;
	bool mountAllBFS = false;
	bool mountAllHFS = false;
	bool mountAllDOS = false;
	bool publishAll = false;
	bool publishAllBFS = false;
	bool publishAllHFS = false;
	bool publishAllDOS = false;
	bool unmount = false;
	bool deviceDump = false;
	bool automountAll = false;
	bool automountBFS = false;
	bool automountHFS = false;
	bool automountDOS = false;
	
	BApplication app("application/x-vnd.Be-mntV");
	
	if (argc == 1) {
		printf("%s %s", argv[0], usage);
		return 0;
	}
	else {
		for (int32 index = 1; index < argc; index++) {
			if (strcmp(argv[index], "-p") == 0
				|| strcmp(argv[index], "-l") == 0)
				dumpVolumes = true;
			else if (strcmp(argv[index], "-lh") == 0)
				showHidden = true;
			else if (strcmp(argv[index], "-all") == 0)
				mountAll = true;
			else if (strcmp(argv[index], "-allbfs") == 0)
				mountAllBFS = true;
			else if (strcmp(argv[index], "-allhfs") == 0)
				mountAllHFS = true;
			else if (strcmp(argv[index], "-alldos") == 0)
				mountAllDOS = true;
			else if (strcmp(argv[index], "-unmount") == 0)
				unmount = true;
			else if (strcmp(argv[index], "-publishall") == 0)
				publishAll = true;
			else if (strcmp(argv[index], "-publishbfs") == 0)
				publishAllBFS = true;
			else if (strcmp(argv[index], "-publishhfs") == 0)
				publishAllHFS = true;
			else if (strcmp(argv[index], "-publishdos") == 0)
				publishAllDOS = true;
			else if (strcmp(argv[index], "-ro") == 0)
				mountflags = B_MOUNT_READ_ONLY;
			else if (strcmp(argv[index], "-r") == 0)
				rescan = true;
			else if (strcmp(argv[index], "-s") == 0)
				silent = true;
			else if (strcmp(argv[index], "-dd") == 0)
				deviceDump = true;
			else if (strcmp(argv[index], "-autoall") == 0)
				automountAll = true;
			else if (strcmp(argv[index], "-autobfs") == 0)
				automountBFS = true;
			else if (strcmp(argv[index], "-autohfs") == 0)
				automountHFS = true;
			else if (strcmp(argv[index], "-autodos") == 0)
				automountDOS = true;
			else if (strcmp(argv[index], "--help") == 0) {
				printf("%s %s", argv[0], usage);
				return 0;
			} else if (*argv[index] == '-') {
				printf("%s: unknown option %s", argv[0], argv[index]);
				printf("%s %s", argv[0], usage);
				return 0;
			}
		}
	}

	if (unmount && (mountAll || mountAllBFS || mountAllHFS || mountAllDOS || publishAll)) {
		printf("%s %s, illegal parameter combination", argv[0], usage);
		return 0;
	}
	
	if (!silent)
		printf("scanning volumes...\n");
	DeviceList list;
	list.RescanDevices(rescan);
	list.UpdateMountingInfo();
		
	if (publishAll)
		list.EachPartition(PublishEveryOne, NULL);
	if (publishAllBFS)
		list.EachMountablePartition(PublishOne, const_cast<char *>("bfs"));
	if (publishAllHFS)
		list.EachMountablePartition(PublishOne, const_cast<char *>("hfs"));
	if (publishAllDOS)
		list.EachMountablePartition(PublishOne, const_cast<char *>("dos"));

	if (mountAll)
		list.EachMountablePartition(TryMountingEveryOne, NULL);
	if (mountAllHFS)
		list.EachMountablePartition(TryMountingOneFS, const_cast<char *>("hfs"));
	if (mountAllBFS)
		list.EachMountablePartition(TryMountingOneFS, const_cast<char *>("bfs"));
	if (mountAllDOS)
		list.EachMountablePartition(TryMountingOneFS, const_cast<char *>("dos"));
	if (deviceDump)
		list.EachDevice(&Device::Dump, NULL);
	
	for (int32 index = 1; index < argc; index++) {
		if (*argv[index] != '-') {
			if (unmount)
				list.EachMountedPartition(TryUnmountingOne, argv[index]);
			else
				list.EachMountablePartition(TryMountingOne, argv[index]);
		}
	}

	if (dumpVolumes) {
		printf(format,
			"Volume",
			"Device",
			"Mounted at",
			"FS");
		printf(format,
			"--------------------",
			"--------------------",
			"--------------------",
			"--------"
			);
	
		list.EachMountedPartition(DumpOnePartition, NULL);
		list.EachMountablePartition(DumpOnePartition, NULL);
	}

	if (showHidden) {
		printf(hiddenFormat,
			"Volume",
			"Device",
			"Mounted at",
			"FS",
			"Hidden");
		printf(hiddenFormat,
			"--------------------",
			"--------------------",
			"--------------------",
			"--------",
			"--------"
			);
	
		list.EachPartition(DumpOnePartitionInclHidden, NULL);
	}

	if (automountAll || automountHFS || automountBFS || automountDOS) {
		if (!silent)
			printf("automounting scan...\n");
		DeviceScanParams scanParams;
		scanParams.shortestRescanHartbeat = 2000000;
		scanParams.checkFloppies = true;
		scanParams.checkCDROMs = true;
		scanParams.checkOtherRemovable = true;
		scanParams.removableOrUnknownOnly = true;
		
		AutomountParams automountParams;
		automountParams.mountAll = automountAll;
		automountParams.mountBFS = automountBFS;
		automountParams.mountHFS = automountHFS;
		automountParams.mountDOS = automountDOS;

		for (;;) {
			if (list.CheckDevicesChanged(&scanParams)) {
				list.UnmountDisappearedPartitions();
				list.UpdateChangedDevices(&scanParams);
				list.EachMountablePartition(Automount, &automountParams);
				list.EachMountedPartition(DumpOnePartition, NULL);
				list.EachMountablePartition(DumpOnePartition, NULL);
			}
			snooze(scanParams.shortestRescanHartbeat);
		}
	}
	
	return 0;
}
