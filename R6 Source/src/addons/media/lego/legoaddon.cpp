#include "legoaddon.h"
#include "tr_debug.h"
#include "LegoConsumer.h"
#include "LegoProducer.h"

#include <MediaRoster.h>
#include <Path.h>
#include <FindDirectory.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define LEGO_DIR "/dev/audio/old"
#define DEFAULT_BUF_SIZE 2048

#if !DEBUG
#define printf(x...)
#endif

const char* errorString = "BLegoAddOn::Standard Error String";
static const char* settings_file_name = "Media/LegacyAudio_settings";

struct names_and_bus {
  char* driver_name;
  char* flavor_name;
  bus_type bus;
};

static names_and_bus name_bus_table[] = {
  {"awe64", "AWE64", B_ISA_BUS},
  {"es137", "Ensoniq", B_PCI_BUS},
  {"opti931", "OPTi 931", B_ISA_BUS},
  {"ess1938", "Solo-1", B_PCI_BUS},
  {"sonic_vibes", "Sonic Vibes", B_PCI_BUS},
  {"au8820", "Vortex-1", B_PCI_BUS},
  {"au8830", "Vortex-2", B_PCI_BUS},
  {"ymf715", "Yamaha OPL3-SA3", B_ISA_BUS},
  {"ymf72", "Yamaha DS-1", B_PCI_BUS},
  {"ymf73", "Yamaha DS-1", B_PCI_BUS},
  {"ymf74", "Yamaha DS-1", B_PCI_BUS},
  {"ess18", "AudioDrive", B_ISA_BUS},
  {"null", "None", B_PCI_BUS},
  {"MVP4", "VIA MVP4", B_PCI_BUS},
  {"cs4280", "SoundFusion", B_PCI_BUS},
  {"i801", "Intel 810", B_PCI_BUS},
  {NULL, NULL, B_UNKNOWN_BUS}
};

float
sr_enum_to_float(sample_rate r)
{
  static const float rate_table[13] = {
	8000, 0, 16000, 11025, 0, 0, 32000, 22050, 0, 0, 0, 44100, 48000};
  if (r < 0 || r > 12 || rate_table[r] == 0)
	return 44100;
  return rate_table[r];
}

static char*
map_name (char* name, bus_type* type = NULL)
{
  char* flavor_name = NULL;
  bus_type bus = B_UNKNOWN_BUS;

  if (strstr(name, "sound")) {
	flavor_name = "Sound";
	system_info si;
	if (get_system_info(&si) == B_OK)
	  if (si.platform_type == B_MAC_PLATFORM) {
		bus = B_PCI_BUS;
		flavor_name = "Mac";
	  }
	  else if (si.platform_type == B_BEBOX_PLATFORM) {
		bus = B_ISA_BUS;
		flavor_name = "BeBox";
	  }
  }
  else for (names_and_bus* nab = name_bus_table; nab->driver_name; nab++)
	if (strstr(name, nab->driver_name)) {
	  bus = nab->bus;
	  flavor_name = nab->flavor_name;
	}

  if (type)
	*type = bus;
  return flavor_name;
}

static status_t
save_thread(void* arg)
{
  BLegoAddOn* addon = (BLegoAddOn*) arg;

  while (acquire_sem_etc(addon->fSaveSem, 1, B_TIMEOUT, 10500000) // 10.5s
		 == B_TIMED_OUT)
	addon->WriteSettingsFile();

  return B_OK;
}

dev_spec::dev_spec(char* name, int32 pref_size) : 
	open_count(0),
	fd(-1),
	lock(create_sem(1, "dev_spec lock")),
	bus(B_UNKNOWN_BUS),
	preferred_buffer_size(pref_size)
{
  strncpy(path, name, sizeof(path));
  path[sizeof(path) - 1] = 0;
  memset(&old_setup, 0, sizeof(old_setup));
  memset(&setup, 0, sizeof(setup));
  for (int i = 0; i < N_INPUT_BUFFERS; i++)
	fInputHeaders[i] = NULL;
  for (int i = 0; i < N_OUTPUT_BUFFERS; i++)
	fOutputHeaders[i] = NULL;
  is_null_device = !strcmp(path, "/dev/audio/old/null");
}

dev_spec::~dev_spec()
{
  delete_sem(lock);
}

status_t
dev_spec::open(BLegoAddOn* addon)
{
  printf("dev_spec::open()\n");
  if (!path)
	return B_ERROR;

  if (atomic_add(&open_count, 1))
	return B_OK;

  fd = ::open(path, O_RDWR | O_CLOEXEC);
  if (fd < 0) {
	atomic_add(&open_count, -1);
	perror(path);
	fprintf(stderr, "ERROR: cannot open driver\n");
	return B_ERROR;
  }

  addon->ReadSettingsFile(this);
  set_preferred_buffer_size();
  if (addon->fSaveSem < 0) {
	addon->fSaveSem = create_sem(0, "lego settings");
	addon->fSaveThread = spawn_thread(save_thread, "lego settings",
									  B_LOW_PRIORITY, addon);
	resume_thread(addon->fSaveThread);
  }
  return B_OK;
}

void
dev_spec::close()
{
  printf("dev_spec::close()\n");
  if (atomic_add(&open_count, -1) == 1) {
	::close(fd);
	fd = -1;
	for (int i = 0; i < N_INPUT_BUFFERS; i++)
	  if (fInputHeaders[i]) {
		delete_area(fInputAreas[i]);
		fInputHeaders[i] = NULL;
	  }
	for (int i = 0; i < N_OUTPUT_BUFFERS; i++)
	  if (fOutputHeaders[i]) {
		delete_area(fOutputAreas[i]);
		fOutputHeaders[i] = NULL;
	  }
  }
}

void
dev_spec::set_preferred_buffer_size()
{
  status_t err;
  BMediaRoster* roster = BMediaRoster::Roster(&err);
  if (roster) {
	map_name(path, &bus);
	preferred_buffer_size =
	  roster->AudioBufferSizeFor(2, media_raw_audio_format::B_AUDIO_SHORT,
								 sr_enum_to_float(setup.sample_rate), bus);
  }
  else
	fprintf(stderr, "BLegoAddon: Roster not found: %lx\n", err);
  ioctl(fd, SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE, preferred_buffer_size);
  ioctl(fd, SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE, preferred_buffer_size);
  ioctl(fd, SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE, &preferred_buffer_size);
  //  fprintf(stderr, "BLegoAddon: %s preferred_buffer_size = %d\n",
  //		  path, preferred_buffer_size);
}

void
BLegoAddOn::AddDevice(BList* list, char* path)
{
//  int32 size = DEFAULT_BUF_SIZE;
  dev_spec* spec = new dev_spec(path, DEFAULT_BUF_SIZE);
  if (spec)
	list->AddItem(spec);
}

BLegoAddOn::BLegoAddOn(image_id myImage)
	: BMediaAddOn(myImage)
{
  printf("BLegoAddOn::BLegoAddOn() - BEGIN\n");

  fDevices = new BList;
  if (fDevices == NULL)
	return;
  fSaveSem = B_BAD_SEM_ID;

  char path[MAXPATHLEN];
  strcpy(path, LEGO_DIR);
  RecursiveScan(fDevices, path);
}

BLegoAddOn::~BLegoAddOn()
{
  status_t err;
  delete_sem(fSaveSem);
  wait_for_thread(fSaveThread, &err);
  if (fDevices)
	for (int i = 0; i < fDevices->CountItems(); i++)
	  delete (dev_spec*) fDevices->ItemAt(i);
  delete fDevices;
}

status_t 
BLegoAddOn::InitCheck(const char **out_failure_text)
{
  *out_failure_text = errorString;
  return (fDevices ? B_OK : B_NO_MEMORY);
}

int32 
BLegoAddOn::CountFlavors()
{
  printf("BLegoAddOn::CountFlavors()\n");
  return (fDevices ? 2 * fDevices->CountItems() : 0);
}

/*
	Note: n should be const
	Bugs: Server does not behave properly
	when status returned is B_ERROR, and *out_info == 0
*/

char*
BLegoAddOn::NewFlavorName(int32 n)
{
  char* name = new char[256];
  if (name == NULL)
	return NULL;

  dev_spec* dev = (dev_spec*) fDevices->ItemAt(n >> 1);
  char* devName = dev->path;
  if (strncmp(devName, LEGO_DIR "/", strlen(LEGO_DIR "/")))
	strcpy(name, "unknown");
  else {
	devName += strlen(LEGO_DIR "/");
	char* mapped_name = map_name(devName, &dev->bus);
	if (mapped_name == NULL)
	  strcpy(name, devName);
	else {
	  int32 total = 0;
	  int32 this_one = 0;
	  dev_spec* odev;
	  for (int i = 0; (odev = (dev_spec*) fDevices->ItemAt(i))!=NULL; i++)
		if (i == (n >> 1))
		  this_one = ++total;
		else if (mapped_name == map_name(odev->path + strlen(LEGO_DIR "/")))
		  ++total;
	  strcpy(name, mapped_name);
	  if (total > 1) {
		char suffix[4];
		suffix[0] = ' ';
		suffix[1] = '0' + this_one;
		suffix[2] = 0;
		strcat(name, suffix);
	  }
	}
  }
  strcat(name, n & 1 ? " Out" : " In");
  return name;
}

status_t 
BLegoAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
  printf("BLegoAddOn::GetFlavorAt(%d,...)\n", n);

  if (n < 0 || n > CountFlavors())
	return B_BAD_VALUE;
  if (fDevices == NULL)
	return B_ERROR;

  flavor_info *newInfo = new flavor_info;
  newInfo->internal_id = n;
  newInfo->name = NewFlavorName(n);
  newInfo->info = new char [256];
  strcpy(newInfo->info, "Good information that will be very useful to you.");
  newInfo->flavor_flags = B_FLAVOR_IS_GLOBAL;
  newInfo->possible_count = 1;

  if (n & 1) {
	newInfo->kinds = B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT;

	// Output formats, none
	newInfo->out_format_count = 0;
	newInfo->out_formats = 0;
	newInfo->out_format_flags = 0;

	// Input formats, only one
	newInfo->in_format_count = 1;
	media_format* aFormat = new media_format;
	aFormat->type = B_MEDIA_RAW_AUDIO;
	aFormat->u.raw_audio = media_raw_audio_format::wildcard;
	newInfo->in_formats = aFormat;
	newInfo->in_format_flags = 0;
  }
  else {
	newInfo->kinds = B_BUFFER_PRODUCER | B_PHYSICAL_INPUT;

	// Input formats, none
	newInfo->in_format_count = 0;
	newInfo->in_formats = 0;
	newInfo->in_format_flags = 0;

	// Output formats, only one
	newInfo->out_format_count = 1;
	media_format* aFormat = new media_format;
	aFormat->type = B_MEDIA_RAW_AUDIO;
	aFormat->u.raw_audio = media_raw_audio_format::wildcard;
	newInfo->out_formats = aFormat;
	newInfo->out_format_flags = 0;
  }

  *out_info= newInfo;
  return B_OK;
}

BMediaNode *
BLegoAddOn::InstantiateNodeFor(const flavor_info *info, BMessage */*config*/,
							   status_t *out_error)
{
	status_t err = B_OK;
	BMediaNode* aNode = NULL;
	int32 id = info->internal_id;
	dev_spec* spec = (dev_spec*) fDevices->ItemAt(id >> 1);

	printf("BLegoAddOn::InstantiateNodeFor(%d)\n", id);

	err = spec->open(this);
	if (err < 0) {
	  if (out_error)
		*out_error = err;
	  return NULL;
	}

	if (id & 1)
	  aNode = new BLegoConsumer(this, spec, info->name, id, &err);
	else
	  aNode = new BLegoProducer(this, spec, info->name, id, &err);

	if (err != B_OK) {
	  aNode->Release();
	  if (out_error)
		*out_error = err;
	  return NULL;
	}

	return aNode;
}

status_t 
BLegoAddOn::GetConfigurationFor(BMediaNode */*your_node*/, BMessage */*into_message*/)
{
	printf("BLegoAddOn::GetConfiguratoinFor()\n");
	// We don't really save anything at this point
	return B_ERROR;
}

bool 
BLegoAddOn::WantsAutoStart()
{
	printf("BLegoAddOn::WantsAutoStart()\n");
	return false;
}

status_t 
BLegoAddOn::AutoStart(int in_count, BMediaNode** out_node,
						int32* out_internal_id,
						bool* out_has_more)
{
	status_t err = B_OK;
//	BMediaNode* aNode = NULL;
	dev_spec* spec = (dev_spec*) fDevices->ItemAt(in_count >> 1);

	printf("BLegoAddOn::AutoStart(%d)\n", in_count);

	if (spec == NULL)
	  return B_ERROR;

	err = spec->open(this);
	if (err < 0)
	  return err;

	char* name = NewFlavorName(in_count);
	if (in_count & 1)
	  *out_node = new BLegoConsumer(this, spec, name, in_count, &err);
	else
	  *out_node = new BLegoProducer(this, spec, name, in_count, &err);
	delete [] name;

	printf("BLegoAddOn::AutoStart(): %s %s\n",
		   (err == B_OK ? "started" : "failed"),
		   (*out_node)->Name());

	*out_internal_id = in_count;
	*out_has_more = (in_count < CountFlavors() - 1);
	return B_OK;
}


void
BLegoAddOn::RecursiveScan(BList* list, char* path)
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


#define SETTING_NAME_SIZE 32

void
BLegoAddOn::WriteSettingsFile()
{
	if (!fDevices)
	  return;

	bool changed = false;
	for (int i = 0; i < fDevices->CountItems(); i++) {
	  dev_spec* dev = (dev_spec*) fDevices->ItemAt(i);
	  if (memcmp(&dev->setup, &dev->old_setup, sizeof(dev->setup))) {
		changed = true;
		break;
	  }
	}
	if (!changed)
	  return;

	printf("BLegoAddOn::WriteSettingsFile()\n");
	BPath	path;
	if (find_directory (B_COMMON_SETTINGS_DIRECTORY, &path, true) != B_OK)
	  return;
	path.Append (settings_file_name);

	int ref = open(path.Path(), O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (ref < 0)
	  return;

	char name[SETTING_NAME_SIZE];
	for (int i = 0; i < fDevices->CountItems(); i++) {
	  dev_spec* dev = (dev_spec*) fDevices->ItemAt(i);
	  memset(name, 0, sizeof(name));
	  if (!strncmp(dev->path, LEGO_DIR "/", strlen(LEGO_DIR "/")))
		strncpy(name, dev->path + strlen(LEGO_DIR "/"), sizeof(name) - 1);

	  if (acquire_sem(dev->lock) == B_OK) {
		write(ref, name, sizeof(name));
		write(ref, (char*) &dev->setup, sizeof(dev->setup));
		dev->old_setup = dev->setup;
		release_sem(dev->lock);
	  }
	}
	close(ref);
}


bool
BLegoAddOn::ReadSettingsFile(dev_spec* dev)
{
	BPath	path;

	ioctl(dev->fd, SOUND_GET_PARAMS, &dev->setup, sizeof(dev->setup));
	dev->old_setup = dev->setup;
	if (find_directory (B_COMMON_SETTINGS_DIRECTORY, &path, true) != B_OK)
	  return false;

	path.Append(settings_file_name);
	int ref = open(path.Path(), O_RDONLY);
	if (ref < 0)
	  return false;

	char name[SETTING_NAME_SIZE];
	while (1) {
	  read(ref, name, sizeof(name));
	  if (read(ref, (char*) &dev->old_setup, sizeof(dev->old_setup))
		  < (ssize_t)sizeof(dev->old_setup)) {
		close(ref);
		return false;
	  }
	  if (!strncmp(dev->path, LEGO_DIR "/", strlen(LEGO_DIR "/")))
		if (!strncmp(name, dev->path + strlen(LEGO_DIR "/"), sizeof(name) - 1))
		  break;
	}
	if (acquire_sem(dev->lock) != B_OK) {
	  close(ref);
	  return false;
	}

	dev->setup.left = dev->old_setup.left;
	dev->setup.right = dev->old_setup.right;
	dev->setup.sample_rate = dev->old_setup.sample_rate;
	dev->setup.loop_attn = dev->old_setup.loop_attn;
	dev->setup.loop_enable = dev->old_setup.loop_enable;
	dev->setup.mono_gain = dev->old_setup.mono_gain;
	dev->setup.mono_mute = dev->old_setup.mono_mute;
	dev->old_setup = dev->setup;

	ioctl(dev->fd, SOUND_SET_PARAMS, &dev->setup, sizeof(sound_setup));
	ioctl(dev->fd, SOUND_GET_PARAMS, &dev->setup, sizeof(dev->setup));
	release_sem(dev->lock);
	close(ref);

	//printf("BLegoAddOn::ReadSettingsFile(): %d\n", dev->setup.left.dac_attn);
	return true;
}


/*
	Function: make_media_addon
	
	This is the function that is exported for the add-on.
	The server looks for it so it can instantiate addons.
*/

BMediaAddOn *
make_media_addon(image_id myImage)
{
	return new BLegoAddOn(myImage);
}
