
#include <OS.h>
#include <stdlib.h>
#include <stdio.h>
#include <Directory.h>
#include <Drivers.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <fs_info.h>
#include <Path.h>
#include <priv_syscalls.h>
#include <URL.h>
#include <Volume.h>
#include "AutoMounterNode.h"
#include "VolumeNode.h"

#define	kSTART_AUTOMOUNTER		"_start_automounter"
#define kSTOP_AUTOMOUNTER		"_stop_automounter"
#define kFIRST_PASS_COMPLETED	"_first_pass_completed"
#define kMANUAL_MOUNT			"_mount"

using namespace Wagner;


/*=================================================================*/

/* local */
static status_t			mounter				(void*);
static char*			binder_friendly_name(const char*);
static void				log					(const char*, bool nl = true);

/*=================================================================*/

AutoMounterNode::AutoMounterNode(bool nomount)
 : fDieAutoMounter(false),
   fFirstPassCompleted(false),
   fMountNow(false),
   fAutoMounterThread(0),
   fDrives(NULL),
   fVolumes(NULL),
   fDisableVolumeMount(nomount)
{
	AddProperty(kSTART_AUTOMOUNTER, (double)0, permsWrite);
	AddProperty(kSTOP_AUTOMOUNTER, (double)0, permsWrite);
	AddProperty(kFIRST_PASS_COMPLETED, (double)0, permsWrite);
	AddProperty(kMANUAL_MOUNT, (double)0, permsWrite);
	printf("AutoMounterNode: starting automounter%s\n", (nomount?" (will not mount volumes)":""));
	StartAutoMounter();
}


/*-----------------------------------------------------------------*/

AutoMounterNode::~AutoMounterNode()
{
	StopAutoMounter();
}


/*-----------------------------------------------------------------*/

get_status_t AutoMounterNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (!strcmp(kSTART_AUTOMOUNTER, name))
	{
		if ((!fAutoMounterThread) && (args.Count() == 1))
			StartAutoMounter();
	}
	else if (!strcmp(kSTOP_AUTOMOUNTER, name))
	{
		if ((fAutoMounterThread) && (args.Count() == 1))
			StopAutoMounter();
	}
	else if (!strcmp(kFIRST_PASS_COMPLETED, name))
	{
		if ((fAutoMounterThread) && (args.Count() == 1))
		{
			while (fFirstPassCompleted != true)
				snooze(10000);
		}
		prop = property((int)fFirstPassCompleted);
	}
	else if ((!strcmp(kMANUAL_MOUNT, name)) && (args.Count() == 1))
	{
		if (!fAutoMounterThread)
			StartAutoMounter();

		fMountNow = true;
		while (fMountNow)
			snooze(10000);

		prop = property((int)1);
	}
	else
		return BinderContainer::ReadProperty(name, prop, args);
	return B_NO_ERROR;
}


/*-----------------------------------------------------------------*/

status_t AutoMounterNode::AutoMounter()
{
	int32				snooze_loop;
	device_list*		list = NULL;
	BEntry				entry(kSTARTING_POINT);
	node_ref			node;

	sem_id list_sem = create_sem(0, "device_list_access");
	fLooper = new TLooper(this, &list, list_sem);

	entry.GetNodeRef(&node);
	watch_node(&node, B_WATCH_ALL, fLooper);

	BuildDeviceList(kSTARTING_POINT, &list, fLooper);
	release_sem(list_sem);

	snooze(100000);

	while (!fDieAutoMounter)
	{
		acquire_sem(list_sem);
		ScanDevices(list);

		if ((!fFirstPassCompleted) && (list))
		{
			fFirstPassCompleted = true;
#if DEBUG
			list->PrintToStream();
			printf("\n");
#endif /* DEBUG */
		}
		release_sem(list_sem);

		fMountNow = false;
		snooze_loop = 100;
		while ((!fMountNow) && (snooze_loop))
		{
			snooze(10000);
			snooze_loop--;
		}
	}
	
	stop_watching(fLooper);
	fLooper->Lock();
	fLooper->Quit();

	acquire_sem(list_sem);
	delete list;
	release_sem(list_sem);
	delete_sem(list_sem);
	return B_NO_ERROR;
}


/*-----------------------------------------------------------------*/

void AutoMounterNode::StartAutoMounter()
{
	AddProperty("drives", (BinderContainer*)(fDrives = new BinderContainer), permsRead|permsWrite|permsDelete);
	AddProperty("volumes", (BinderContainer*)(fVolumes = new BinderContainer), permsRead|permsWrite|permsDelete);

	while ((fAutoMounterThread = spawn_thread((status_t (*)(void *))mounter,
				 "auto_mounter", B_DISPLAY_PRIORITY, this)) < 0)
		// failed for some reason, keep trying
		snooze(1000);
	resume_thread(fAutoMounterThread);
}


/*-----------------------------------------------------------------*/

status_t AutoMounterNode::StopAutoMounter()
{
	status_t	result;

	fDieAutoMounter = true;
	if (fAutoMounterThread)
		wait_for_thread(fAutoMounterThread, &result);

	RemoveProperty("drives");
	fDrives = NULL;

	RemoveProperty("volumes");
	fVolumes = NULL;

	fAutoMounterThread = 0;
	fDieAutoMounter = false;
	return result;
}

/*-----------------------------------------------------------------*/

void AutoMounterNode::BinderizeDevice(device_list* device)
{
	char*	name = binder_friendly_name(device->name);
	int32	type = -1;
	char*	product = NULL;
	char*	vendor = NULL;
	char*	version = NULL;

	if (device->type)
	{
		type = device->type->type;
		product = device->type->product;
		vendor = device->type->vendor;
		version = device->type->version;
	}

	if (fDrives) {
		fDrives->AddProperty(name, device->drive_node = new DriveNode(device->name,
												 device->removable,
												 type,
												 vendor,
												 product,
												 version),
							 permsRead|permsWrite|permsDelete);
		// fDrives->NotifyListeners(B_PROPERTY_ADDED, name); // protected
	}

	free(name);
}


/*-----------------------------------------------------------------*/

void AutoMounterNode::BuildDeviceList(const char* directory, device_list** list, BLooper* looper)
{
	BDirectory		dir;

	dir.SetTo(directory);
	if (dir.InitCheck() == B_NO_ERROR)
	{
		BEntry			entry;

		dir.Rewind();
		while (dir.GetNextEntry(&entry) >= 0)
		{
			const char*		name;
			BPath			path;

			entry.GetPath(&path);
			name = path.Path();
			if (entry.IsDirectory())
			{
				node_ref	node;

				entry.GetNodeRef(&node);
				watch_node(&node, B_WATCH_ALL, looper);
				BuildDeviceList(name, list, looper);
			}
			/* only look at raw devices */
			else if (strstr(name, "raw"))
			{
				device_list*	device = *list;
				/* make sure it's unique (there might be a race condition with node_monitor */
				while (device)
				{
					if (strcmp(device->name, name) == 0)
						break;
					device = device->next;
				}

				if (!device)
				{
					bool			removable = false;
					int				fd = 0;
					node_ref		node;
					device_type*	type = NULL;

					if ((fd = open(name, O_RDONLY)) > 0)
					{
						device_geometry		geometry;

						if (ioctl(fd, B_GET_GEOMETRY, &geometry) == B_NO_ERROR)
							removable = geometry.removable;

						type = GetDeviceType(fd);
					}

					entry.GetNodeRef(&node);
					device = new device_list(fd, name, node, removable, type);
					device->next = *list;
					*list = device;
					BinderizeDevice(device);
				}
				else
				{
#ifdef DEBUG
					printf("*** ignoring duplicate device: %s\n", name);
#endif /* DEBUG */
				}
			}
		}
	}
}


/*-----------------------------------------------------------------*/

void AutoMounterNode::BuildSessionList(int fd, device_list* device)
{
	device_geometry		geometry;
	session_list*		session;

	if (ioctl(fd, B_GET_GEOMETRY, &geometry) == B_NO_ERROR)
	{
		device->block_size = geometry.bytes_per_sector;
		/* only CDs can have multiple sessions */
		if (geometry.device_type == B_CD)
		{
			int32			index = 0;

			while ((session = GetSession(fd, device, index)) != NULL)
			{
				session->next = device->sessions;
				device->sessions = session;
				index++;
				if (session->data)
					BuildVolumeList(fd, device, session);
			}
		}
		else
		{
			/* fake a session for non-cd devices */
			session = new session_list(0, geometry.sectors_per_track *
							  			  geometry.cylinder_count *
							  			  geometry.head_count,
							  			  0, true);
			device->sessions = session;
			BuildVolumeList(fd, device, session);
		}
	}
}


/*-----------------------------------------------------------------*/

void AutoMounterNode::BuildVolumeList(int fd, device_list* device, session_list* session)
{
	uchar*		block;

	/* read block 0, the partition map or first block of file system */
	block = (uchar*)malloc(device->block_size);
	lseek(fd, session->offset * device->block_size, 0);
	if (read(fd, block, device->block_size) >= 0)
	{
		/* locate partition add-ons */
		BDirectory		dir;
		BEntry			entry;
		BPath			path;

		find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
		dir.SetTo(path.Path());
		dir.FindEntry(DS_PART_ADDONS, &entry);
		dir.SetTo(&entry);
		if (dir.InitCheck() == B_NO_ERROR)
		{
			bool			found = false;

			dir.Rewind();
			while (!found)
			{
				if (dir.GetNextEntry(&entry) >= 0)
				{
					image_id		image;

					entry.GetPath(&path);
					if ((image = load_add_on(path.Path())) >= 0)
					{
						bool		(*ds_partition_id)(uchar *, int32);

						if (get_image_symbol(image, DS_PARTITION_ID, B_SYMBOL_TYPE_TEXT,
								(void **)&ds_partition_id) >= 0)
						{
							/* try to ID the parition map */
							if ((*ds_partition_id)(block, device->block_size))
							{
								status_t		(*ds_get_nth_map)(int32, uchar*, uint64, int32, int32, partition_data*);

								/* we've ID'd the map, start parsing partitions */
								if (get_image_symbol(image, DS_PARTITION_MAP,
										B_SYMBOL_TYPE_TEXT, (void **)&ds_get_nth_map) >= 0)
								{
									for (int32 loop = 0; ; loop++)
									{
										partition_data	partition;

										if ((*ds_get_nth_map)(fd, block, session->offset, device->block_size,
												loop, &partition) == B_NO_ERROR)
										{
											volume_list*		volume;

											if ((volume = IDFileSystem(fd, device, &partition, session->offset)) != 0)
											{
												volume->partition = loop;
												volume->next = session->volumes;
												session->volumes = volume;
											}
										}
										else
											break;
									}
								}
								found = true;
							}
						}
						unload_add_on(image);
					}
				}
				else
				{
					/* couldn't ID the map, so assume it's a file system */
					partition_data		partition;
					volume_list*		volume;

					partition.offset = session->offset;
					partition.blocks = session->length;
					partition.logical_block_size = device->block_size;
					partition.hidden = false;

					if ((volume = IDFileSystem(fd, device, &partition, session->offset)) != 0)
					{
						volume->partition = 0;
						volume->next = session->volumes;
						session->volumes = volume;
					}
					found = true;
				}
			}
		}
	}
	free(block);
}

/*-----------------------------------------------------------------*/

device_type* AutoMounterNode::GetDeviceType(int fd)
{
	device_type*	type = NULL;
	scsi_inquiry	inq;

	if (ioctl(fd, B_SCSI_INQUIRY, &inq) == B_NO_ERROR)
	{
		if (((inq.inquiry_data[0] & 0xe0) != 0x60) &&
			(inq.inquiry_data[4] >= 31) &&
			((inq.inquiry_data[3] & 0xf) < 3))
			type = new device_type(&inq.inquiry_data[0]);
	}
	return type;
}

/*-----------------------------------------------------------------*/

session_list* AutoMounterNode::GetSession(int fd, device_list* device, int32 index)
{
	BDirectory		dir;
	BEntry			entry;
	BPath			path;
	session_list*	session = NULL;

	/* locate session add-ons */
	find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
	dir.SetTo(path.Path());
	dir.FindEntry(DS_SESSION_ADDONS, &entry);
	dir.SetTo(&entry);
	if (dir.InitCheck() == B_NO_ERROR)
	{
		bool		found = false;

		dir.Rewind();
		while (!found)
		{
			if (dir.GetNextEntry(&entry) >= 0)
			{
				image_id		image;

				entry.GetPath(&path);
				if ((image = load_add_on(path.Path())) >= 0)
				{
					status_t		(*ds_get_nth_session)(int32, int32, int32, session_data*);

					if (get_image_symbol(image, DS_GET_NTH_SESSION,
							B_SYMBOL_TYPE_TEXT, (void **)&ds_get_nth_session) >= 0)
					{
						session_data	dev_session;

						if ((*ds_get_nth_session)(fd, index, device->block_size, &dev_session) == B_NO_ERROR)
							session = new session_list(dev_session.offset, dev_session.blocks, index, dev_session.data);
						found = true;
					}
					unload_add_on(image);
				}
			}
			else
				found = true;
		}
	}
	return session;
}


/*-----------------------------------------------------------------*/

volume_list* AutoMounterNode::IDFileSystem(int fd, device_list* device, partition_data* partition, uint64 offset)
{
	BDirectory		dir;
	BEntry			entry;
	BPath			path;
	volume_list*	volume = NULL;

	/* locate file_system add-ons */
	find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
	dir.SetTo(path.Path());
	dir.FindEntry(DS_FS_ADDONS, &entry);
	dir.SetTo(&entry);
	if (dir.InitCheck() == B_NO_ERROR)
	{
		bool		found = false;

		dir.Rewind();
		while (!found)
		{
			if (dir.GetNextEntry(&entry) >= 0)
			{
				image_id	image;

				entry.GetPath(&path);
				if ((image = load_add_on(path.Path())) >= 0)
				{
					bool		(*ds_fs_id)(partition_data*, int32, uint64, int32);

					if (get_image_symbol(image, DS_FS_ID, B_SYMBOL_TYPE_TEXT,
							(void **)&ds_fs_id) >= 0)
					{
						if ((*ds_fs_id)(partition, fd, offset, device->block_size))
						{
							if (!partition->hidden)
								volume = new volume_list(partition->volume_name, partition->file_system_short_name, partition->offset, partition->blocks);
							found = true;
						}
					}
					unload_add_on(image);
				}
			}
			else
				found = true;
		}
	}
	return volume;
}


/*-----------------------------------------------------------------*/

void AutoMounterNode::MediaRemoved(device_list* device)
{
	bool			error = false;
	session_list*	session = device->sessions;

	while (session)
	{
		volume_list*	volume = session->volumes;

		while (volume)
		{
			if (volume->volume_mounted)
			{
				if (UnmountVolume(device, volume) != B_NO_ERROR)
					error = true;
			}
			volume = volume->next;
		}
		session = session->next;
	}
	if ((!error) && (device->sessions))
	{
		delete device->sessions;
		device->sessions = NULL;
		device->has_media = false;
	}
}


/*-----------------------------------------------------------------*/

void AutoMounterNode::DeviceRemoved(device_list* device)
{
	char*	name = binder_friendly_name(device->name);

	fDrives->RemoveProperty(name);
	// fDrives->NotifyListeners(B_PROPERTY_REMOVED, name); // protected

	free(name);
}

/*-----------------------------------------------------------------*/

void AutoMounterNode::ScanDevices(device_list* devices)
{
	device_list*	device = devices;

	while (device)
	{
		if (device->fd)
		{
			status_t		media_status;
			status_t		result;

#if 1
			if (strstr(device->name, "floppy"))
			{
			}
			else
#endif
			if ((result = ioctl(device->fd, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status))) >= 0)
			{
				if (media_status == B_NO_ERROR)
				{
					if (device->has_media)
						/* device HAD media and still HAS media */
						ScanVolumes(device);
					else
					{
						/* device DIDN'T have media and NOW has media */
						device->has_media = true;
						BuildSessionList(device->fd, device);
						ScanVolumes(device);
					}
				}
				else if (device->has_media)
					/* device HAD media but DOESN'T now */
					MediaRemoved(device);
				else
				{
					/* simple case, device DIDN'T have media and STILL doesn't */
				}
			}
			else
			{
				/* GET_MEDIA_STATUS failed, unmount volumes */
#if DEBUG
				printf("*** GET_MEDIA_STATUS failed for %s: %s\n", device->name, strerror(result));
#endif /* DEBUG */
				MediaRemoved(device);
			}
		}
		else
		{
			/* device no longer opens */
#if DEBUG
			printf("*** device no longer opens %s: %s\n", device->name, strerror(device->fd));
#endif /* DEBUG */
			MediaRemoved(device);
		}
		device = device->next;
	}
}


/*-----------------------------------------------------------------*/

void AutoMounterNode::ScanVolumes(device_list* device)
{
	session_list*	session = device->sessions;

	while (session)
	{
		bool			repeat = false;
		volume_list*	volume = session->volumes;

		while (volume)
		{
			bool			found;
			int32			cookie = 0;
			fs_info			info;

			found = false;
			/* check each mounted volume */
			while (_kstatfs_(-1, &cookie, -1, NULL, &info) == B_NO_ERROR)
			{
				char*			name = NULL;
				int32			partition_index;
				int32			session_index;
				partition_info	p_info;

				if (strlen(info.device_name) != 0)
				{
					session_index = partition_index = 0;
					if (!strstr(info.device_name, "/raw"))
					{
						int			fd;

						if ((fd = open(info.device_name, O_RDONLY)) >= 0)
						{
							if (ioctl(fd, B_GET_PARTITION_INFO, &p_info) == B_NO_ERROR)
							{
								name = (char *)&p_info.device;
								session_index = p_info.session;
								partition_index = p_info.partition;
							}
							else
								name = (char *)&info.device_name;
							close(fd);
						}
					}
					else
						name = (char *)&info.device_name;

					/* does the mounted volume match our volume? */
					if ((name) &&
						(!strncmp(device->name, name, strstr(device->name, "raw") - device->name)) &&
						(session->session == session_index) &&
						(volume->partition == partition_index)) 
					{
						BDirectory		dir;
						BEntry			entry;
						node_ref		node;

						node.device = info.dev;
						node.node = info.root;
						dir.SetTo(&node);
						if (dir.GetEntry(&entry) == B_OK)
						{
							/* if volume is now mounted but wasn't before, send message */
							if (!volume->volume_mounted)
							{
								BPath			dev;

								entry.GetPath(&dev);
								volume->mounted(dev.Path());
								if (fVolumes)
								{
									char*		name = binder_friendly_name(device->name);

									fVolumes->AddProperty(&volume->mounted_at[1], new VolumeNode(fLooper,
																		 volume->name,
																		 volume->mounted_at,
																		 device->name,
																		 name,
																		 info.block_size,
																		 info.total_blocks,
																		 info.free_blocks,
																		 device->removable,
																		 info.flags & B_FS_IS_READONLY),
														 permsRead|permsWrite|permsDelete);
									device->drive_node->AddVolume(&volume->mounted_at[1], volume->mounted_at);
									// fVolumes->NotifyListeners(B_PROPERTY_ADDED, &volume->mounted_at[1]); // protected
									free(name);
								}
							}
							else
							{
								binder_node node = BinderNode::Root()["service"]["auto_mounter"]["volumes"][&volume->mounted_at[1]];

								if (node->IsValid())
								{
									BinderNode::property	prop = node;
									BinderNode::property	result;

									if ((prop->GetProperty("free_blocks", result) == B_NO_ERROR) &&
										(info.free_blocks != (int)result.Number()))
										prop->PutProperty("free_blocks", info.free_blocks);
								}
							}
							found = true;
							break;
						}
					}
				}
			}

			/* the device has media, but the volume isn't mounted */
			if (!found)
			{
				if (volume->volume_mounted)
					/* hmmm, we think the volume is mounted but the system doesn't */
					UnmountVolume(device, volume);
				else if (!fDisableVolumeMount)
				{
				
					/* try mounting the volume */
					char		mount_point[B_FILE_NAME_LENGTH];
					char		logical_device[B_FILE_NAME_LENGTH];
					int			dev;
					status_t	result;

					/* create a name for the logical device */
					strcpy(mount_point, device->name);
					mount_point[strstr(mount_point, "raw") - mount_point] = 0;
					sprintf(logical_device, "%s%ld_%ld", mount_point, session->session, volume->partition);

					result = unlink(logical_device);
#if DEBUG
					if (result != B_NO_ERROR)
						printf("*** unlink of device %s failed: %s\n", logical_device, strerror(result));
#endif /* DEBUG */

					if ((dev = creat(logical_device, 0666)) >= 0)
					{
						struct stat		st;
						partition_info	p_info;
						status_t		result;

						if ((result = stat(device->name, &st)) == B_NO_ERROR)
						{
							/* set the parameters for the logical volume */
							char		buf[B_FILE_NAME_LENGTH];

							p_info.offset = (session->offset * device->block_size) +
											(volume->offset * device->block_size);
							p_info.size = volume->length * device->block_size;
							p_info.logical_block_size = device->block_size;
							p_info.session = session->session;
							p_info.partition = volume->partition;
							strcpy(p_info.device, device->name);
							ioctl(dev, B_SET_PARTITION, &p_info);

							/* create a mount point based on volume name */
							if (!strlen(volume->name))
								sprintf(mount_point, "/disk");
							else
							{
								/* escape bad characters */
								sprintf(mount_point, "/%s", volume->name);
								for (int loop = 1; loop < (int32)strlen(volume->name); loop++)
									if (mount_point[loop] == '/')
										mount_point[loop] = '\\';
							}
							strcpy(buf, mount_point);
							while (1)
							{
								int32			index = 0;
								BDirectory		dir;

								if (mkdir(mount_point, 0777) >= 0)
									break;
								dir.SetTo(mount_point);
								if ((!dir.InitCheck()) && (!dir.CountEntries()))
									break;
								sprintf(mount_point, "%s%ld", buf, index++);
							}
							result = mount(volume->file_system, mount_point, logical_device, 0, NULL, 0);
							if (result == B_NO_ERROR)
								repeat = true;
#if DEBUG
							else
								printf("*** mount of %s at %s failed: %s\n", volume->file_system, mount_point, strerror(result));
#endif /* DEBUG */
						}
						else
						{
#if DEBUG
							printf("*** stat of %s failed: %s\n", device->name, strerror(result));
#endif /* DEBUG */
						}
						close(dev);
					}
					else
					{
#if DEBUG
						printf("*** creat of %s failed: %s\n", logical_device, strerror(dev));
#endif /* DEBUG */
					}
				}
			}
			volume = volume->next;
		}
		if (!repeat)
			session = session->next;
	}
}


/*-----------------------------------------------------------------*/

status_t AutoMounterNode::UnmountVolume(device_list* device, volume_list* volume)
{
	status_t	result;

	result = unmount(volume->mounted_at);
	if (result == B_NO_ERROR)
	{
		device->drive_node->RemoveVolume(&volume->mounted_at[1]);
		fVolumes->RemoveProperty(&volume->mounted_at[1]);
		// fVolumes->NotifyListeners(B_PROPERTY_REMOVED, &volume->mounted_at[1]); // protected
		volume->mounted(NULL);
	}	
	else
	{
#if DEBUG
		printf("*** unmount of %s at %s FAILED: %s\n", volume->name, volume->mounted_at, strerror(result));
#endif /* DEBUG */
	}
	return result;
}


/*=================================================================*/

status_t mounter(void* data)
{
#if DEBUG
	bool				first = true;
#endif /* DEBUG */
	return ((AutoMounterNode *)data)->AutoMounter();
	
}


/*=================================================================*/

char* binder_friendly_name(const char* src)
{
	int32	index = 0;
	char* name = (char*)malloc(strlen(src) + 1);
	strcpy(name, &src[strlen(kSTARTING_POINT)]);

	while(name[index])
	{
		if (name[index] == '/')
			name[index] = '_';
		index++;
	}

	return name;
}


/*=================================================================*/

void log(const char* str, bool nl)
{
#if DEBUG
	BFile	f("/tmp/auto_mounter_log", B_READ_WRITE | B_CREATE_FILE);

	f.Seek(0, SEEK_END);
	f.Write(str, strlen(str));
	if (nl)
		f.Write("\n", 1);
#else
	// Silence the compiler!
	(void)str;
	(void)nl;
#endif
}


/*=================================================================*/

char* nice_string(char* src, int32 len)
{
	char*	result;
	int32	cnt = 0;
	int32	index = 0;
	int32	start = 0;

	while ((index < len) && (src[index] <= ' '))
		index++;
	if (index < len)
	{
		start = index;

		index = len - 1;
		while ((index > start) && (src[index] <= ' '))
			index--;
		cnt = index - start + 1;
	}
	result = (char*)malloc(cnt + 1);
	strncpy(result, &src[start], cnt);
	result[cnt] = 0;
	return result;
}


/*=================================================================*/

TLooper::TLooper(AutoMounterNode* auto_mounter, device_list** list, sem_id sem)
	: BLooper("auto_mounter"),
	  fDevices(list),
	  fDeviceSem(sem),
	  fAutoMounter(auto_mounter)
{
	Run();
}


/*-----------------------------------------------------------------*/

void TLooper::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case B_NODE_MONITOR:
			int32	opcode;

			acquire_sem(fDeviceSem);
			if (msg->FindInt32("opcode", &opcode) == B_NO_ERROR)
			{
				node_ref	node;

				msg->FindInt32("device", &node.device); 
				msg->FindInt64("node", &node.node);
				switch(opcode)
				{
					case B_ENTRY_CREATED:
					{
						BDirectory		dir(&node);

						if (dir.InitCheck() == B_NO_ERROR)
						{
							BEntry		entry;
							BPath		path;

							dir.GetEntry(&entry);
							path.SetTo(&entry);

							fAutoMounter->BuildDeviceList(path.Path(), fDevices, this);
#if DEBUG
							printf("new device directory: %s\n", path.Path());
#endif /* DEBUG */
						}
						else
						{
							const char*		name;
							entry_ref		ref;

							msg->FindInt32("device", &ref.device); 
							msg->FindInt64("directory", &ref.directory); 
							msg->FindString("name", &name);
							ref.set_name(name);

							BEntry	entry(&ref);
							BPath	path(&entry);
							if (strcmp("raw", name) == 0)
							{
								bool			removable = false;
								int				fd = 0;
								device_list*	device;
								device_type*	type = NULL;

								if ((fd = open(path.Path(), O_RDONLY)) > 0)
								{
									device_geometry		geometry;

									if (ioctl(fd, B_GET_GEOMETRY, &geometry) == B_NO_ERROR)
										removable = geometry.removable;

									type = fAutoMounter->GetDeviceType(fd);
								}

								device = new device_list(fd, path.Path(), node, removable, type);
#if DEBUG
								printf("adding new device...\n");
								device->PrintToStream();
#endif /* DEBUG */
								device->next = *fDevices;
								*fDevices = device;
								fAutoMounter->BinderizeDevice(device);
							}
						}
					}
					break;

					case B_ENTRY_REMOVED:
					{
						device_list*	device = *fDevices;
						device_list*	previous = NULL;

						watch_node(&node, B_STOP_WATCHING, this);

						while(device)
						{
							if (device->node == node)
							{
								if (previous)
									previous->next = device->next;
								else
									*fDevices = device->next;
#if DEBUG
								printf("removing device: %s\n", device->name);
#endif /* DEBUG */
								device->next = NULL;
								fAutoMounter->MediaRemoved(device);
								fAutoMounter->DeviceRemoved(device);
								delete device;
#if DEBUG
								if (*fDevices)
									(*fDevices)->PrintToStream();
								else
									printf("*** weird, there are no more devices\n");
#endif /* DEBUG */
								break;
							}
							previous = device;
							device = device->next;
						}
					}
					break;

				}
			}
			release_sem(fDeviceSem);
			break;

		case 'MNTR':
			{
				int32	function;

				msg->FindInt32("function", &function);
				switch (function)
				{
					case 'gvol':
						{
							BMessage		reply('vols');
							device_list*	device = *fDevices;

							if (msg->HasMessage("reply"))
								msg->FindMessage("reply", &reply);

							acquire_sem(fDeviceSem);
							while (device)
							{
								session_list*	session = device->sessions;

								while (session)
								{
									volume_list*	volume = session->volumes;

									while (volume)
									{
										if (volume->volume_mounted)
										{
											reply.AddString("volume_name", volume->name);
											reply.AddString("mounted_at", volume->mounted_at);
										}
										volume = volume->next;
									}
									session = session->next;
								}
								device = device->next;
							}
							release_sem(fDeviceSem);
							msg->SendReply(&reply);
						}
						break;

					case 'ejct':
						{
							const char*		driver;

							if (msg->FindString("device", &driver) == B_NO_ERROR)
							{
								device_list*	device = *fDevices;

								acquire_sem(fDeviceSem);
								while (device)
								{
									if (strcmp(device->name, driver) == 0)
									{
										session_list*	session = device->sessions;

										while (session)
										{
											volume_list*	volume = session->volumes;

											while (volume)
											{
												if (volume->volume_mounted)
												{
													status_t	result;
													if ((result = fAutoMounter->UnmountVolume(device, volume)) != B_NO_ERROR)
													{
														BMessage	error('fbER');
							
														error.AddString("device", driver);
														error.AddString("message", strerror(result));
														error.AddInt32("error code", result);
														be_app->PostMessage(&error);
														goto done;
													}
												}
												volume = volume->next;
											}
											session = session->next;
										}
										if (device->sessions)
										{
											delete device->sessions;
											device->sessions = NULL;
										}
										device->has_media = false;

										if (device->fd)
											ioctl(device->fd, B_SCSI_EJECT);
										break;
									}
									else
										device = device->next;
								}
done:;
								release_sem(fDeviceSem);
							}
						}
						break;
				}
					
			}
			break;

		default:
		{
#if DEBUG
			printf("*** unexpected message\n");
#endif /* DEBUG */
		}
	}
}


/*=================================================================*/

extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new AutoMounterNode(false);
}

extern "C" _EXPORT BinderNode *return_binder_node_etc(const char *arg)
{
	bool nomount = (arg && (strcmp(arg, "nomount") == 0));
	return new AutoMounterNode(nomount);
}
