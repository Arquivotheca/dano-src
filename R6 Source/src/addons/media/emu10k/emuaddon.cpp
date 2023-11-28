#include "emuaddon.h"
#include "tr_debug.h"
#include "EmuConsumer.h"
#include "EmuProducer.h"

#include <MediaRoster.h>
#include <Path.h>
#include <FindDirectory.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define ENABLE_CAPTURE 1

#define AUTO_LAUNCH true
#define AUDIO_DIR "/dev/audio/"
#define DRIVER_NAME "emu10k"
#define EMU_DIR AUDIO_DIR DRIVER_NAME
#define DEFAULT_BUF_SIZE 2048

#define IOCTL(FD,OP,AD,SZ) (ioctl(FD,OP,AD,SZ) < 0 ? errno : B_OK)

#if !DEBUG
#define printf
#endif

const char* errorString = "BEmuAddOn::Standard Error String";
static const char* settings_file_name = "Media/Emu10kAudio_settings";
static const int32 settings_file_magic = 8010;
static const int32 settings_file_version = 1;
static void recursive_scan(BList*, char*);


dev_spec::dev_spec(char* name)
  : lock(create_sem(1, "dev_spec lock")),
	open_count(0),
	fPageTable(-1),
	fRecordBuffer(-1),
	fHRMID(0),
	fOutputArea(-1),
	fOutputAreaPages(0),
	fOutputBufferBase(NULL),
	fOutputClient(NULL),
	fd(-1),
	preferred_buffer_size(DEFAULT_BUF_SIZE)
{
  strncpy(path, name, sizeof(path));
  path[sizeof(path) - 1] = 0;

  /* set default settings */
  memset(&settings, 0, sizeof(settings));
  memset(&old_settings, 0, sizeof(old_settings));
  settings.mxr_regs[mxrPhoneVolume]		= 0x8008;
  settings.mxr_regs[mxrMICVolume]		= 0x8008;
  settings.mxr_regs[mxrLineInVolume]	= 0x0808;
  settings.mxr_regs[mxrCDVolume]		= 0x0808;
  settings.mxr_regs[mxrVideoVolume]		= 0x8808;
  settings.mxr_regs[mxrAuxVolume]		= 0x8808;
  settings.mxr_regs[mxrPCMOutVolume]	= 0x0808;
  settings.mxr_regs[mxrRecordSelect]	= 0x0404;
  settings.mxr_regs[mxrRecordGain]		= 0x0808;
  settings.mxr_regs[mxrADCGain]			= 0xe0e0;
  settings.mxr_regs[mxrMuxSelect2]		= 1;		/* E-Drive analog */
  settings.input_channel				= 0x10;		/* AC97 */
  settings.send_channel_2				= 2;		/* ECARD SPDIF */
}

dev_spec::~dev_spec()
{
  delete_sem(lock);
}

status_t
dev_spec::open(BEmuAddOn* addon)
{
  printf("dev_spec::open()\n");
  if (!path)
	return B_ERROR;

  if (atomic_add(&open_count, 1))
	return B_OK;

  fd = ::open(path, O_RDWR | O_CLOEXEC);
  if (fd < 0) {
	atomic_add(&open_count, -1);
	fprintf(stderr, "ERROR: cannot open driver\n");
	char buf[128];
	sprintf(buf, "Cannot open %100s", path);
	dlog(buf);
	return B_ERROR;
  }

  set_preferred_buffer_size();

  int32 ptsize = ((8192 + 1024) * sizeof(void*));
  HRMCHIPCONFIG chipconfig;
  uint32 subsystem_id;

  status_t err = IOCTL(fd, EMU_GET_SUBSYSTEM_ID, &subsystem_id, 4);
  if (err < 0) {
	printf("SetupEmu: ioctl(%d, EMU_GET_SUBSYSTEM_ID) failed\n", fd);
	return err;
  }
  printf("subsystem id = %04x\n", subsystem_id);

  //halInit();
  hrmInit();
  memset(&chipconfig, 0, sizeof(chipconfig));
  //chipconfig.dwAudioBaseAddress = info->u.h0.base_registers[0];
  chipconfig.hrmDiscoverFlags = HRMDISC_SOUND_ENGINE;
  chipconfig.dwUserHardwareID = fd;
  chipconfig.dwHardwareInterruptID = (DWORD) fd;
 
  // allocate physical locked contiguous memory in lower 2GB somehow
  void* ptaddr = osAllocContiguousPages(ptsize / 4096,
  									&chipconfig.smPageTable.osPhysAddr,
  									(uint32*) &fPageTable);
  if (ptaddr == NULL)
  	return B_ERROR;
  chipconfig.smPageTable.dwSize = ptsize;
  chipconfig.smPageTable.osVirtAddr = ptaddr;

  // allocate input buffer in lower 2GB
  void* buf = osAllocContiguousPages((2*preferred_buffer_size + 4095) / 4096,
  									&chipconfig.hbSRCRecord.osPhysAddr,
  									(uint32*) &fRecordBuffer);
  if (buf == NULL) {
  	osFreePages(fPageTable);
  	return B_ERROR;
  }
  chipconfig.hbSRCRecord.osVirtAddr = buf;
  chipconfig.hbSRCRecord.dwSize = preferred_buffer_size;

  chipconfig.bInitSoundEngine = TRUE;
  chipconfig.dwMixerID = (subsystem_id == 0x4001 ? MXRID_ECARD : MXRID_AC97);
  chipconfig.hrmMixerFlags = HRMDISC_SOUND_ENGINE;
  chipconfig.totalSystemPhysRam = 0;

  if (hrmDiscoverChip(&fHRMID, &chipconfig) != SUCCESS) {
  	printf("hrmDiscoverChip FAILED.\n");
  	osFreePages(fPageTable);
  	osFreePages(fRecordBuffer);
	fHRMID = 0;
    return B_ERROR;
  }

  printf("emu tester: hrmDiscoverChip():  SUCCESS\n");

  fHALID = hrmGetHALID(fHRMID);
  fMXRID = hrmGetMXRID(fHRMID);
  fMXRTYPE = mxrGetMixerType(fMXRID);
  fHC = L8010SERegRead(fHALID, HC);

  printf("HALID: 0x%08x\n", fHALID);
  printf("WC: 0x%08x\n", L8010SERegRead(fHALID, WC));
  printf("HC: 0x%08x\n", fHC);
  printf("PTR: 0x%08x\n", L8010SERegRead(fHALID, PTRREG));
  printf("DTA: 0x%08x\n", L8010SERegRead(fHALID, DATAREG));

  addon->ReadSettingsFile(this);
  return B_OK;
}

void
dev_spec::close()
{
  printf("dev_spec::close()\n");
  if (atomic_add(&open_count, -1) == 1) {
	printf("dev_spec::close(): calling hrmUndiscoverChip()\n");
	if (fHRMID)
	  hrmUndiscoverChip(fHRMID, HRMDISC_SOUND_ENGINE);
	::close(fd);
	fd = -1;
	if (fOutputBufferBase)
	  osFreePages(fOutputArea);
	fOutputBufferBase = 0;
	fOutputArea = -1;
  }
}

void
dev_spec::set_preferred_buffer_size()
{
  status_t err;
  BMediaRoster* roster = BMediaRoster::Roster(&err);
  if (roster)
	preferred_buffer_size =
	  roster->AudioBufferSizeFor(2, media_raw_audio_format::B_AUDIO_SHORT,
								 44100.0, B_PCI_BUS);
  else
	fprintf(stderr, "BLegoAddon: Roster not found: %x\n", err);
  //  fprintf(stderr, "BLegoAddon: %s preferred_buffer_size = %d\n",
  //		  path, preferred_buffer_size);
}

static void
add_item(BList* list, char* path)
{
  dev_spec* spec = new dev_spec(path);
  if (spec)
	list->AddItem(spec);
}

static status_t
save_thread(void* arg)
{
  BEmuAddOn* addon = (BEmuAddOn*) arg;

  while (acquire_sem_etc(addon->fSaveSem, 1, B_TIMEOUT, 10500000) // 10.5s
		 == B_TIMED_OUT)
	addon->WriteSettingsFile();

  return B_OK;
}

BEmuAddOn::BEmuAddOn(image_id myImage)
	: BMediaAddOn(myImage)
{
#if DEBUG_DO_MESSAGE
  printf("BEmuAddOn::BEmuAddOn() - BEGIN\n");
#endif

  fSaveSem = B_BAD_SEM_ID;
  fDevices = new BList;
  if (fDevices == NULL)
	return;

  char path[MAXPATHLEN];
  strcpy(path, EMU_DIR);
  recursive_scan(fDevices, path);
}

BEmuAddOn::~BEmuAddOn()
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
BEmuAddOn::InitCheck(const char **out_failure_text)
{
  *out_failure_text = errorString;
  return (fDevices ? B_OK : B_NO_MEMORY);
}

int32 
BEmuAddOn::CountFlavors()
{
#if DEBUG_DO_MESSAGE
  printf("BEmuAddOn::CountFlavors()\n");
#endif
  return (fDevices ? 2 * fDevices->CountItems() : 0);
}

/*
	Note: n should be const
	Bugs: Server does not behave properly
	when status returned is B_ERROR, and *out_info == 0
*/

char*
BEmuAddOn::NewFlavorName(int32 n)
{
  char* name = new char[256];
  if (name) {
	strcpy(name, "be:unknown");
	char* devName = ((dev_spec*) fDevices->ItemAt(n >> 1))->path;
	if (!strncmp(devName, EMU_DIR "/", strlen(EMU_DIR "/"))) {
	  strcpy(name, DRIVER_NAME);
	  if (CountFlavors() > 2) {
		strcat(name, " ");
		strcat(name, devName + strlen(EMU_DIR "/"));
	  }
	}
	strcat(name, n & 1 ? " Out" : " In");
  }
  return name;
}

status_t 
BEmuAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
  printf("BEmuAddOn::GetFlavorAt(%d,...)\n", n);

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

#if ENABLE_CAPTURE
	// Output formats, only one
	newInfo->out_format_count = 1;
	media_format* aFormat = new media_format;
	aFormat->type = B_MEDIA_RAW_AUDIO;
	aFormat->u.raw_audio = media_raw_audio_format::wildcard;
	newInfo->out_formats = aFormat;
#else
	newInfo->in_format_count = 0;
	newInfo->in_formats = 0;
#endif
	newInfo->out_format_flags = 0;
  }

  *out_info= newInfo;
  return B_OK;
}

BMediaNode*
BEmuAddOn::InstantiateNodeFor(const flavor_info *info, BMessage *config,
							  status_t *out_error)
{
	status_t err = B_OK;
	BMediaNode* aNode = NULL;
	int32 id = info->internal_id;
	dev_spec* spec = (dev_spec*) fDevices->ItemAt(id >> 1);

	printf("BEmuAddOn::InstantiateNodeFor(%d)\n", id);

	err = spec->open(this);
	if (err < 0) {
	  if (out_error)
		*out_error = err;
	  return NULL;
	}

	if (id & 1)
	  aNode = new BEmuConsumer(this, spec, info->name, id, &err);
	else
	  aNode = new BEmuProducer(this, spec, info->name, id, &err);

	if (err != B_OK) {
	  aNode->Release();
	  if (out_error)
		*out_error = err;
	  return NULL;
	}

	return aNode;
}

status_t 
BEmuAddOn::GetConfigurationFor(BMediaNode *your_node, BMessage *into_message)
{
	printf("BEmuAddOn::GetConfiguratoinFor()\n");
	// We don't really save anything at this point
	return B_ERROR;
}

bool 
BEmuAddOn::WantsAutoStart()
{
	printf("BEmuAddOn::WantsAutoStart()\n");
	return false;
}

status_t 
BEmuAddOn::AutoStart(int in_count, BMediaNode** out_node,
						int32* out_internal_id,
						bool* out_has_more)
{
	printf("BEmuAddOn::AutoStart()\n");
	return B_ERROR;
}


static void
recursive_scan(BList* list, char* path)
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
	  recursive_scan(list, path);
	else
	  add_item(list, path);
  }

  closedir(dir);
}

#define SETTINGS_NAME_SIZE 32

void
BEmuAddOn::WriteSettingsFile()
{
	if (!fDevices)
	  return;

	bool changed = false;
	for (int i = 0; i < fDevices->CountItems(); i++) {
	  dev_spec* dev = (dev_spec*) fDevices->ItemAt(i);
	  if (memcmp(&dev->settings, &dev->old_settings, sizeof(dev->settings))) {
		changed = true;
		break;
	  }
	}
	if (!changed)
	  return;

	BPath	path;
	if (find_directory (B_COMMON_SETTINGS_DIRECTORY, &path, true) != B_OK)
	  return;
	path.Append(settings_file_name);

	int ref = open(path.Path(), O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (ref < 0)
	  return;

	write(ref, &settings_file_magic, 4);
	write(ref, &settings_file_version, 4);
	char name[SETTINGS_NAME_SIZE];
	for (int i = 0; i < fDevices->CountItems(); i++) {
	  dev_spec* dev = (dev_spec*) fDevices->ItemAt(i);
	  memset(name, 0, sizeof(name));
	  if (!strncmp(dev->path, AUDIO_DIR, strlen(AUDIO_DIR)))
		strncpy(name, dev->path + strlen(AUDIO_DIR), sizeof(name) - 1);

	  if (acquire_sem(dev->lock) == B_OK) {
		write(ref, name, sizeof(name));
		write(ref, (char*) &dev->settings, sizeof(dev->settings));
		dev->old_settings = dev->settings;
		release_sem(dev->lock);
	  }
	}
	close(ref);
}


void
BEmuAddOn::ReadSettingsFile(dev_spec* dev)
{
	BPath path;
	int ref;

	if (find_directory (B_COMMON_SETTINGS_DIRECTORY, &path, true) != B_OK)
	  goto set_controls;

	path.Append(settings_file_name);
	ref = open(path.Path(), O_RDONLY);
	if (ref < 0)
	  goto set_controls;

	int32 magic[2];
	if (read(ref, magic, 8) < 8
		|| magic[0] != settings_file_magic
		|| magic[1] != settings_file_version)
	  goto close_file;

	char name[SETTINGS_NAME_SIZE];
	while (1) {
	  read(ref, name, sizeof(name));
	  if (read(ref, (char*) &dev->old_settings, sizeof(dev->old_settings))
		  < sizeof(dev->old_settings))
		goto close_file;

	  if (!strncmp(dev->path, AUDIO_DIR, strlen(AUDIO_DIR)))
		if (!strncmp(name, dev->path + strlen(AUDIO_DIR), sizeof(name) - 1))
		  break;
	}

	dev->settings = dev->old_settings;

close_file:
	close(ref);

set_controls:
	if (dev->fMXRTYPE == MXRTYPE_AC97)
	  for (int i = (int)mxrMasterVolume; i <= (int)mxrRecordGainMic; i++)
		mxrSetControlValue(dev->fMXRID, (MXRCONTROLID)i, dev->settings.mxr_regs[i]);
	if (dev->fMXRTYPE == MXRTYPE_ECARD)
	  for (int i = (int)mxrADCGain; i <= (int)mxrMuxSelect2; i++)
		mxrSetControlValue(dev->fMXRID, (MXRCONTROLID)i, dev->settings.mxr_regs[i]);

	if (fSaveSem < 0) {
	  fSaveSem = create_sem(0, "emu10k settings");
	  fSaveThread = spawn_thread(save_thread, "emu10k settings",
								 B_LOW_PRIORITY, this);
	  resume_thread(fSaveThread);
	}
}


/*
	Function: make_media_addon
	
	This is the function that is exported for the add-on.
	The server looks for it so it can instantiate addons.
*/

BMediaAddOn *
make_media_addon(image_id myImage)
{
	return new BEmuAddOn(myImage);
}
