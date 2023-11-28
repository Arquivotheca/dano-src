#ifndef _DV_BUFFER_PRODUCER_ADDON_H
#define _DV_BUFFER_PRODUCER_ADDON_H

#include <kernel/image.h>
#include <kernel/OS.h>
#include <media/MediaAddOn.h>
#include <support/List.h>
#include <support/Locker.h>

#include "Flavor.h"

#define TOUCH(x) ((void)(x))

extern "C" _EXPORT BMediaAddOn *make_media_addon(image_id you);

class DVAddOn : public BMediaAddOn
{
public:
						DVAddOn(image_id imid);
	virtual 			~DVAddOn();
	
	virtual	status_t	InitCheck(const char **out_failure_text);

	virtual	int32		CountFlavors();
	virtual	status_t	GetFlavorAt(int32 n, const flavor_info ** out_info);
	virtual	BMediaNode	*InstantiateNodeFor(
							const flavor_info * info,
							BMessage * config,
							status_t * out_error);

	virtual	status_t	GetConfigurationFor(BMediaNode *node, BMessage *message)
								{ TOUCH(node); TOUCH(message); return B_OK; }
	virtual	status_t	SaveConfigInfo(BMediaNode *node, BMessage *message)
								{ TOUCH(node); TOUCH(message); return B_OK; }

	virtual	bool		WantsAutoStart() { return false; }
	virtual	status_t	AutoStart(int in_count, BMediaNode **out_node,
								int32 *out_internal_id, bool *out_has_more)
								{	TOUCH(in_count); TOUCH(out_node);
									TOUCH(out_internal_id); TOUCH(out_has_more);
									return B_ERROR; }

	status_t			AcquireGUID(int32 bus, uint64 guid);
	status_t			ReleaseGUID(int32 bus, uint64 guid);
private:
	status_t			fInitStatus;
	BList				f1394Cards;
	FlavorRoster		fFlavorRoster;

	flavor_info			fFlavorInfo;

	static int32		_device_watcher_(void *data);
	int32				DeviceWatcher();
	void				FindCameras();
	thread_id			fWatcherThread;
	sem_id				fResetSem;
};

#endif
