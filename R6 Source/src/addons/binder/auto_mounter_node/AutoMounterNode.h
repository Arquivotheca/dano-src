
#ifndef _AUTO_MOUNTER_NODE_H_
#define _AUTO_MOUNTER_NODE_H_

#include <Application.h>
#include <Binder.h>
#include <drive_setup.h>
#include <Looper.h>
#include <NodeMonitor.h>
#include <scsi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "DriveNode.h"

//#define DEBUG					1
#define kSTARTING_POINT			"/dev/disk/"

char* nice_string(char* src, int32 len);


/*-----------------------------------------------------------------*/

struct device_type
{
					device_type(uchar* inquiry)
					{
						type = inquiry[0] & 0x1f;
						vendor = nice_string((char*)&inquiry[8], 8);
						product = nice_string((char*)&inquiry[16], 16);
						version = nice_string((char*)&inquiry[32], 4);
					}

					~device_type()
					{
						free(vendor);
						free(product);
						free(version);
					}

	void			PrintToStream()
					{
#if DEBUG
						printf("\tdevice type:    %d\n", type);
						printf("\tdevice vendor:  %s\n", vendor);
						printf("\tdevice product: %s\n", product);
						printf("\tdevice version: %s\n", version);
#endif
					}

	char*			product;
	char*			vendor;
	char*			version;
	uint8			type;
};

/*-----------------------------------------------------------------*/

struct volume_list
{
					volume_list(const char* new_name, const char* fs, uint64 new_offset, uint64 new_length)
					{
						name = (char*)malloc(strlen(new_name) + 1);
						strcpy(name, new_name);
						mounted_at = NULL;
						file_system = (char*)malloc(strlen(fs) + 1);
						strcpy(file_system, fs);
						offset = new_offset;
						length = new_length;
						volume_mounted = false;
						next = NULL;
					}

					~volume_list()
					{
						if (next)
							delete next;
#if DEBUG
						printf("\t\tdeleting volume %s\n", name);
#endif /* DEBUG */
						free(name);
						free(mounted_at);
						free(file_system);
					}

	void			PrintToStream()
					{
#if DEBUG
						if (next)
						{
							next->PrintToStream();
							printf("\n");
						}
						printf("\t\t%d: volume name: %s\n", (int)partition, name);
						printf("\t\t   volume offset: %Ld\n", offset);
						printf("\t\t   volume length: %Ld\n", length);
						printf("\t\t   file system: %s\n", file_system);
						(volume_mounted) ?
							printf("\t\t   mounted at %s\n", mounted_at)
						:
							printf("\t\t   not mounted\n");
#endif /* DEBUG */
					}

	void			mounted(const char* location)
					{
						if (location)
						{
							free(mounted_at);
							mounted_at = (char*)malloc(strlen(location) + 1);
							strcpy(mounted_at, location);
							volume_mounted = true;
#if DEBUG
							printf("volume %s mounted at %s\n", name, mounted_at);
#endif /* DEBUG */
						}
						else
						{
#if DEBUG
							printf("volume %s unmounted from %s\n", name, (mounted_at) ? mounted_at : "(weird, it's not mounted)");
#endif /* DEBUG */
							free(mounted_at);
							mounted_at = NULL;
							volume_mounted = false;
						}
					}

	char*			name;
	char*			mounted_at;
	char*			file_system;
	uint64			offset;
	uint64			length;
	int32			partition;
	bool			volume_mounted;
	volume_list*	next;
};

/*-----------------------------------------------------------------*/

struct session_list
{
					session_list(uint64 new_offset, uint64 new_length, int32 index, bool is_data)
					{
						offset = new_offset;
						length = new_length;
						session = index;
						data = is_data;
						volumes = NULL;
						next = NULL;
					}

					~session_list()
					{
						if (next)
							delete next;
#if DEBUG
						printf("\tdeleting session %d\n", (int)session);
#endif /* DEBUG */
						delete volumes;
					}

	void			PrintToStream()
					{
#if DEBUG
						if (next)
						{
							next->PrintToStream();
							printf("\n");
						}
						printf("\t%d: session offset: %Ld\n", (int)session, offset);
						printf("\t   session length: %Ld\n", length);
						printf("\t   %s\n", (data) ? "DATA" : "AUDIO");
						if (volumes)
							volumes->PrintToStream();
#endif /* DEBUG */
					}

	uint64			offset;
	uint64			length;
	int32			session;
	bool			data;
	volume_list*	volumes;
	session_list*	next;
};

/*-----------------------------------------------------------------*/

struct device_list
{
					device_list(int driver, const char* new_name, node_ref new_node,
								bool is_removable, device_type* dev_type)
					{
						fd = driver;
						name = (char*)malloc(strlen(new_name) + 1);
						strcpy(name, new_name);
						node = new_node;
						removable = is_removable;
						type = dev_type;
						has_media = false;
						sessions = NULL;
						next = NULL;
					}

					~device_list()
					{
						if (next)
							delete next;
#if DEBUG
						printf("deleting device %s\n", name);
#endif /* DEBUG */
						delete sessions;
						delete type;
						free(name);
						if (fd > 0)
							close(fd);
					}

	void			PrintToStream()
					{
#if DEBUG
						if (next)
						{
							next->PrintToStream();
							printf("\n");
						}
						printf("%s%s%s\n", name,
										   (has_media) ? "" : " - NO MEDIA",
										   (removable) ? " - REMOVABLE" : "");
						if (type)
							type->PrintToStream();
						if (sessions)
							sessions->PrintToStream();
#endif /* DEBUG */
					}

	char*			name;
	bool			has_media;
	bool			removable;
	int				fd;
	uint32			block_size;
	node_ref		node;
	DriveNode*		drive_node;
	device_type*	type;
	session_list*	sessions;	
	device_list*	next;
};


/*=================================================================*/

class AutoMounterNode;

class TLooper : public BLooper
{
	public:
							TLooper				(AutoMounterNode*, device_list**, sem_id);
		virtual void		MessageReceived		(BMessage*);

	private:
		device_list**		fDevices;
		sem_id				fDeviceSem;
		AutoMounterNode*	fAutoMounter;
};


/*=================================================================*/

class AutoMounterNode : public BinderContainer
{
	public:
									AutoMounterNode		(bool nomount);
									~AutoMounterNode	();
		virtual	get_status_t		ReadProperty		(const char *name,
														 property &prop,
														 const property_list &args = empty_arg_list);

		status_t					AutoMounter			();
		void						StartAutoMounter	();
		status_t					StopAutoMounter		();
		void						BinderizeDevice		(device_list*);
		void						BuildDeviceList		(const char*,
														 device_list**,
														 BLooper*);
		device_type*				GetDeviceType		(int);
		void						MediaRemoved		(device_list*);
		void						DeviceRemoved		(device_list*);
		status_t					UnmountVolume		(device_list*,
														 volume_list*);

	private:
		void						BuildSessionList	(int,
														 device_list*);
		void						BuildVolumeList		(int,
														 device_list*,
														 session_list*);
		session_list*				GetSession			(int,
														 device_list*,
														 int32);
		volume_list*				IDFileSystem		(int,
														 device_list*,
														 partition_data*,
														 uint64);
		void						ScanDevices			(device_list*);
		void						ScanVolumes			(device_list*);

		volatile bool				fDieAutoMounter;
		volatile bool				fFirstPassCompleted; 
		volatile bool				fMountNow;
		thread_id					fAutoMounterThread;
		atom<BinderContainer>		fDrives;
		atom<BinderContainer>		fVolumes;
		TLooper*					fLooper;
		
		bool 						fDisableVolumeMount;
};
#endif
