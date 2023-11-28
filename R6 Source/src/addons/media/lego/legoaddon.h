#ifndef _LEGO_ADD_ON_H
#define _LEGO_ADD_ON_H

#include "MediaAddOn.h"

#include <unistd.h>
#include <image.h>
#include <sound.h>
#include <string.h>
#include <config_manager.h>

// export this for the input_server
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id you);

#if __POWERPC__
#define N_INPUT_BUFFERS 2
#define N_OUTPUT_BUFFERS 2
#else
#define N_INPUT_BUFFERS 1
#define N_OUTPUT_BUFFERS 1
#endif

struct audio_buffer_header;
class BLegoAddOn;

extern float sr_enum_to_float(sample_rate);

struct dev_spec {
  dev_spec(char* name, int32 pref_size);
  ~dev_spec();
  status_t open(BLegoAddOn* addon);
  void close();
  void set_preferred_buffer_size();

  int32 open_count;
  int fd;
  char path[80];
  sem_id lock;
  sound_setup old_setup;
  sound_setup setup;
  bus_type bus;
  int32 preferred_buffer_size;
  bool is_null_device;

  area_id				fInputAreas[N_INPUT_BUFFERS];
  audio_buffer_header*	fInputHeaders[N_INPUT_BUFFERS];
  area_id				fOutputAreas[N_OUTPUT_BUFFERS];
  audio_buffer_header*	fOutputHeaders[N_OUTPUT_BUFFERS];
};


class BLegoAddOn : public BMediaAddOn
{
public:
			BLegoAddOn(image_id myImage);
	virtual		~BLegoAddOn();
			
	friend struct dev_spec;
	virtual	status_t	InitCheck(const char ** out_failure_text);
	virtual	int32		CountFlavors();
	// Q. Should the out_info be malloc or new?
	virtual	status_t	GetFlavorAt(int32 n, const flavor_info ** out_info);
	virtual	BMediaNode * InstantiateNodeFor(const flavor_info * info,
							BMessage * config,
							status_t * out_error);
	virtual	status_t	GetConfigurationFor(BMediaNode * your_node,
							BMessage * into_message);
	virtual	bool		WantsAutoStart();
	virtual	status_t	AutoStart(int in_count, BMediaNode ** out_node,
							int32 * out_internal_id,
							bool * out_has_more);

	virtual char*		NewFlavorName(int32);

	virtual void		WriteSettingsFile();
	virtual bool		ReadSettingsFile(dev_spec* dev);

	sem_id			fSaveSem;

protected:
private:
	void			RecursiveScan(BList* list, char* path);
	void			AddDevice(BList* list, char* path);

	BList*			fDevices;
	thread_id		fSaveThread;
};

#endif

