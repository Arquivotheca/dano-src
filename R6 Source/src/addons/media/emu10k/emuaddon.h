#ifndef _EMU_ADD_ON_H
#define _EMU_ADD_ON_H

#include "MediaAddOn.h"

#include <unistd.h>
#include <image.h>
#include <string.h>
#include <config_manager.h>

extern "C" {
 #include "emu10k_driver.h"
 #include "hrm8210d.h"
 #include "hal8210.h"
 #include "mxr8210d.h"
};

#define N_INPUT_BUFFERS 2
#define N_OUTPUT_BUFFERS 2

// export this for the input_server
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id you);

struct card_settings {
  int32 mxr_regs[16];
  int32 ac97_record_gain[8];
  int32 input_channel;				// channel # of left channel
  int32 send_channel_1;				// channel # of left channel
  int32 send_channel_2;				// channel # of left channel
};

class BEmuAddOn;
class SAOutputClient;

struct dev_spec {
  dev_spec(char* name);
  ~dev_spec();
  status_t open(BEmuAddOn* addon);
  void close();
  void set_preferred_buffer_size();

  int32 open_count;
  int fd;
  char path[80];
  sem_id lock;
  area_id fPageTable;
  area_id fRecordBuffer;
  int32 preferred_buffer_size;

  HRMID fHRMID;
  HALID fHALID;
  MXRID fMXRID;
  int32 fMXRTYPE;
  int32 fHC;

  card_settings settings;
  card_settings old_settings;

  area_id	fOutputArea;
  int32		fOutputAreaPages;
  char*		fOutputBufferBase;
  int32		fOutputBufferPhysical;

  SAOutputClient*	fOutputClient;
};


class BEmuAddOn : public BMediaAddOn
{
public:
			BEmuAddOn(image_id myImage);
	virtual		~BEmuAddOn();
			
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
	virtual void		ReadSettingsFile(dev_spec* dev);

	sem_id			fSaveSem;

protected:
private:
	BList*			fDevices;
	thread_id		fSaveThread;
};

#endif

