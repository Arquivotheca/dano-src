#include "multi_addon.h"
#include "multi.h"

#include <config_manager.h>
#include <dirent.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <List.h>
#include <MediaRoster.h>
#include <Path.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *multi_dir = "/dev/audio/multi/";
static const char *settings_file_name = "Media/multi_audio";

const int32 SETTING_NAME_SIZE = 32;
const int32 DEFAULT_BUFFER_SIZE = 128;		/* frames */
const int32 DEFAULT_SAMPLE_FORMAT = 0x4;
const float DEFAULT_FRAME_RATE = 44100.0;

const char* errorString = "BMultiAudioAddon failed to create device list";

extern "C" _EXPORT BMediaAddOn *
make_media_addon(image_id you)
{
	return new BMultiAudioAddon(you);
}

multi_dev::multi_dev(char *name)
	: open_count(0),
	fd(-1),
	lock(create_sem(1, "multi_dev_lock")),
	description()
{
	strncpy(path, name, sizeof(path));
	path[sizeof(path) - 1] = 0;
  	memset(&old, 0, sizeof(old));
  	memset(&setup, 0, sizeof(setup));
}


multi_dev::~multi_dev()
{
	delete_sem(lock);
}

status_t 
multi_dev::open()
{
	PRINT(("multi_dev::open() %s\n", path));
	if (!path)
		return B_ERROR;

	if (atomic_or(&open_count, 1))
		return B_OK;

	fd = ::open(path, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		atomic_and(&open_count, 0);
		PRINT(("multi_dev: cannot open %s\n", path));
		return B_RESOURCE_UNAVAILABLE;
	}

	description.info_size = sizeof(multi_description);
	ioctl(fd, B_MULTI_GET_DESCRIPTION, &description);
	if (!read_settings()) {
		setup.buffer_size = DEFAULT_BUFFER_SIZE;
		setup.sample_format = DEFAULT_SAMPLE_FORMAT;
		setup.frame_rate = DEFAULT_FRAME_RATE;
	}
	return B_OK;
}

void 
multi_dev::close()
{
	PRINT(("multi_dev::close() %s\n", path));
	if (atomic_and(&open_count, 0) == 1) {
		::close(fd);
		fd = -1;
	}
}

bool
multi_dev::read_settings()
{
	PRINT(("multi_dev::read_settings()\n"));
	BPath bpath;
	if (find_directory (B_COMMON_SETTINGS_DIRECTORY, &bpath, true) != B_OK)
		return false;
	bpath.Append(settings_file_name);
	int ref = ::open(bpath.Path(), O_RDONLY);
	if (ref < 0)
		return false;

	old = setup;
	char name[SETTING_NAME_SIZE];
	while (1) {
		PRINT(("multi_dev::read_settings - looking for %s\n", path));
		read(ref, name, sizeof(name));
		if (read(ref, (char*) &old, sizeof(old)) < sizeof(old)) {
			PRINT(("multi_dev::read_settings - nothing left to read\n"));
			::close(ref);
			return false;
		}
		if (!strncmp(path, multi_dir, strlen(multi_dir)))
			if (!strncmp(name, path + strlen(multi_dir), sizeof(name) - 1))
				break;
	}
	if (acquire_sem(lock) != B_OK) {
		::close(ref);
		return false;
	}

	setup.buffer_size = old.buffer_size;
	setup.sample_format = old.sample_format;
	setup.frame_rate = old.frame_rate;
	old = setup;
	PRINT(("multi_dev::read_settings - buffer_size %ld, sample_format 0x%x, frame_rate %f\n", setup.buffer_size, setup.sample_format, setup.frame_rate));

	release_sem(lock);
	::close(ref);
	return true;
}

void
multi_dev::write_settings(int ref)
{
	PRINT(("multi_dev::write_settings()\n"));
	if (ref < 0)
		return;
	
	char name[SETTING_NAME_SIZE];
	memset(name, 0, sizeof(name));
	if (!strncmp(path, multi_dir, strlen(multi_dir)))
		strncpy(name, path + strlen(multi_dir), sizeof(name) - 1);

	if (acquire_sem(lock) == B_OK) {
		PRINT(("multi_dev::write_settings writing %s\n", name));
		write(ref, name, sizeof(name));
		write(ref, (char*) &setup, sizeof(setup));
		old = setup;
		release_sem(lock);
	}
}

/* BMultiAudioAddon */

BMultiAudioAddon::BMultiAudioAddon(image_id me)
	: BMediaAddOn(me),
	mSaveSem(B_BAD_SEM_ID),
	mDevices(NULL),
	mSaveThread(B_BAD_THREAD_ID)
{
	mDevices = new BList();
	if (mDevices == NULL)
		return;
	char path[B_PATH_NAME_LENGTH];
	strcpy(path, "/dev/audio/multi");
	RecursiveScan(mDevices, path);
	PRINT(("BMultiAudioAddon created successfully\n"));
}


BMultiAudioAddon::~BMultiAudioAddon()
{
	if (mDevices){
		for (int i = 0; i < mDevices->CountItems(); i++)
			delete (multi_dev*) mDevices->ItemAt(i);
	}
	delete mDevices;
	PRINT(("BMultiAudioAddon destroyed successfully\n"));
}

status_t 
BMultiAudioAddon::InitCheck(const char **out_failure_text)
{
	PRINT(("BMultiAudioAddon::InitCheck\n"));
	*out_failure_text = errorString;
	return (mDevices ? B_OK : B_NO_MEMORY);
}

int32 
BMultiAudioAddon::CountFlavors()
{
	int32 count = (mDevices ? mDevices->CountItems() : 0);
	PRINT(("BMultiAudioAddon::CountFlavors returning %ld\n", count));
	return count;
}

status_t 
BMultiAudioAddon::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	PRINT(("BMultiAudioAddon::GetFlavorAt %ld\n", n));
	if (n < 0 || n >= CountFlavors())
		return B_BAD_VALUE;
	if (!mDevices)
		return B_ERROR;
	
	multi_dev *dev = (multi_dev *)mDevices->ItemAt(n);
	flavor_info *info = new flavor_info;
	info->internal_id = n;
	info->name = dev->description.friendly_name;
	info->info = dev->description.vendor_info;
	info->flavor_flags = B_FLAVOR_IS_GLOBAL;
	info->possible_count = 1;
	info->kinds = B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT |
					B_BUFFER_PRODUCER | B_PHYSICAL_INPUT |
					B_TIME_SOURCE | B_CONTROLLABLE;
	/* generic format */
	media_format *fmt = new media_format;
	fmt->type = B_MEDIA_RAW_AUDIO;
	fmt->u.raw_audio = media_raw_audio_format::wildcard;
	/* input formats */
	info->in_format_count = 1;
	info->in_formats = fmt;
	info->in_format_flags = 0;
	/* output formats */
	info->out_format_count = 1;
	info->out_formats = fmt;
	info->out_format_flags = 0;
	
	*out_info = info;
	return B_OK;
}

static status_t
save_thread(void* arg)
{
	BMultiAudioAddon* addon = (BMultiAudioAddon*) arg;
	status_t err = B_OK;
	while (1) {
	 	err = acquire_sem(addon->mSaveSem);
		if (err < B_OK)
			return err;
		addon->WriteSettingsFile();
	}
	return B_OK;
}

BMediaNode *
BMultiAudioAddon::InstantiateNodeFor(const flavor_info *info, BMessage *config, status_t *out_error)
{
	int32 id = info->internal_id;
	PRINT(("BMultiAudioAddon::InstantiateNodeFor %ld\n", id));
	*out_error = B_OK;
	if (id < 0 || id >= CountFlavors() || NULL == mDevices) {
		*out_error = B_ERROR;
		return NULL;
	}
	
	multi_dev *dev = (multi_dev *)mDevices->ItemAt(id);	
	*out_error = dev->open();
	if (*out_error < B_OK)
		return NULL;
	
	BMediaNode *node = new BMultiNode(info->name, this, id, dev->fd, dev->setup);
	if (NULL == node)
		*out_error = B_ERROR;
	else if (mSaveSem < 0) {
		PRINT(("BMultiAudioAddon::InstantiateNodeFor starting save thread\n", id));
		mSaveSem = create_sem(0, "multi_audio settings");
		mSaveThread = spawn_thread(save_thread, "multi_audio settings", B_LOW_PRIORITY, this);
		resume_thread(mSaveThread);
	}
	return node;
}

static bool 
CloseFunc(void *one, void *two)
{
	multi_dev *md = (multi_dev *)one;
	int *fd = (int *)two;
	PRINT(("fd %ld md->fd %ld\n", *fd, md->fd));
	if (*fd == md->fd){
		PRINT(("#######CloseFunc closing %ld\n", fd));
		md->close();
		return true;
	}
	return false;
}

void 
BMultiAudioAddon::CloseDevice(int fd)
{
	PRINT(("BMultiAudioAddon::CloseDevice %d\n", fd));
	mDevices->DoForEach(CloseFunc, (void *)&fd);
}

void
BMultiAudioAddon::WriteSettingsFile()
{
	PRINT(("BMultiAudioAddon::WriteSettingsFile\n"));
	if (!mDevices)
		return;

	bool changed = false;
	for (int i = 0; i < mDevices->CountItems(); i++) {
		multi_dev* dev = (multi_dev*) mDevices->ItemAt(i);
		if (memcmp(&dev->setup, &dev->old, sizeof(dev->setup))) {
			changed = true;
			break;
		}
	}
	if (!changed)
		return;

	BPath path;
	if (find_directory (B_COMMON_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;
	path.Append(settings_file_name);

	int ref = open(path.Path(), O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (ref < 0)
		return;

	for (int i = 0; i < mDevices->CountItems(); i++) {
		multi_dev* dev = (multi_dev*) mDevices->ItemAt(i);
		dev->write_settings(ref);
	}
	::close(ref);
}

void 
BMultiAudioAddon::SetupChanged(int fd, const multi_setup &setup)
{
	PRINT(("BMultiAudioAddon::SetupChanged %ld, 0x%x, %f, 0x%x, %ld, 0x%x, 0x%x\n",
			setup.buffer_size, setup.sample_format, setup.frame_rate,
			setup.lock_source, setup.lock_data, setup.timecode_source,
			setup.enable_all_channels));
	for (int i = 0; i < mDevices->CountItems(); i++) {
		multi_dev* dev = (multi_dev*) mDevices->ItemAt(i);
		if (dev->fd == fd) {
			dev->old = dev->setup;
			dev->setup = setup;
			release_sem(mSaveSem);
			break;
		}
	}
}

void
BMultiAudioAddon::RecursiveScan(BList* list, char* path)
{
	char* tail = path + strlen(path);
	DIR* dir = opendir(path);
	if (dir == NULL)
		return;

	while (1) {
		struct dirent* dent = readdir(dir);
		if (dent == NULL)
			break;
		if (!strcmp(dent->d_name, "."))
			continue;
		if (!strcmp(dent->d_name, ".."))
			continue;

		strcpy(tail, "/");
		strcat(tail, dent->d_name);

		struct stat stbuf;
		if (stat(path, &stbuf))
			continue;

		if (S_ISDIR(stbuf.st_mode))
			RecursiveScan(list, path);
		else
			AddDevice(list, path);
	}

	closedir(dir);
}

void 
BMultiAudioAddon::AddDevice(BList *list, char *path)
{
	PRINT(("BMultiAudioAddon::AddDevice %s\n", path));
	multi_dev* dev = new multi_dev(path);
	if (dev){
		if (dev->open() < B_OK)
			return;	
		dev->close();
		list->AddItem(dev);
	}
}

