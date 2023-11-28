#ifndef _MULTI_ADDON_H
#define _MULTI_ADDON_H

#include <image.h>
#include <multi_audio.h>
#include <MediaAddOn.h>

class BList;
class BMultiAudioAddon;

// export this for the media_server
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id you);

struct multi_setup {
	int32 buffer_size;
	uint32 sample_format;
	float frame_rate;
	uint32 lock_source;
	int32 lock_data;
	uint32 timecode_source;
	bool enable_all_channels;
};

struct multi_dev
{
	multi_dev(char* name);
	~multi_dev();
	status_t open();
	void close();
	bool read_settings();
	void write_settings(int ref);
	
	multi_setup old;
	multi_setup setup;
	int32 open_count;
	int fd;
	char path[B_PATH_NAME_LENGTH];
	sem_id lock;
	multi_description description;
};

class BMultiAudioAddon : public BMediaAddOn
{
public:
	BMultiAudioAddon(image_id myImage);
	~BMultiAudioAddon();

	status_t	InitCheck(const char ** out_failure_text);
	int32		CountFlavors();
	status_t	GetFlavorAt(int32 n, const flavor_info ** out_info);
	BMediaNode *InstantiateNodeFor(const flavor_info * info,
										BMessage * config,
										status_t * out_error);							

	void		CloseDevice(int fd);
	void 		WriteSettingsFile();
	void		SetupChanged(int fd, const multi_setup &setup);
								
	sem_id		mSaveSem;
private:
	void 		RecursiveScan(BList *list, char *path);
	void		AddDevice(BList* list, char* path);

	BList *mDevices;
	thread_id	mSaveThread;
};

#endif //_MULTI_ADDON_H

