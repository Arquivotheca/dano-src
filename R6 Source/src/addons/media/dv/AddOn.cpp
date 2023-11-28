#include <support/Autolock.h>
#include <media/MediaFormats.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ieee1394.h"
#include "avc.h"
#include "vcr.h"

#include "AddOn.h"

int32 debug_level = 0;

DVAddOn::DVAddOn(image_id imid)
	: BMediaAddOn(imid),
	  fFlavorRoster(this)
{
	if (char *s = getenv("DEBUG_DV"))
		debug_level = strtol(s, NULL, 0);

	memset(&fFlavorInfo, 0, sizeof(fFlavorInfo));

	for (int32 i=0;;i++) {
		int fd = open_1394(i);
		if (fd < 0)
			break;
		f1394Cards.AddItem((void *)fd);
	}
	if (f1394Cards.IsEmpty()) {
		fInitStatus = ENODEV;
		goto err1;
	}

	fResetSem = create_sem(0, "bus reset");
	if (fResetSem < B_OK) {
		fInitStatus = fResetSem;
		goto err2;
	}
	fWatcherThread = spawn_thread(_device_watcher_, "Device Watcher",
			B_NORMAL_PRIORITY, this);
	if (fWatcherThread < B_OK) {
		fInitStatus = fWatcherThread;
		goto err3;
	}

	fInitStatus = B_OK;

	fFlavorRoster.CreateDefaultFlavor();
	FindCameras();
	resume_thread(fWatcherThread);
	
	return;

//err4:
//	kill_thread(fWatcherThread);
err3:
	delete_sem(fResetSem);
err2:
	while (f1394Cards.IsEmpty() == false)
		close_1394((int)f1394Cards.RemoveItem((int32)0));
err1:
	return;
}

DVAddOn::~DVAddOn()
{
	status_t _;

	if (fInitStatus < B_OK)
		return;

	if (fFlavorInfo.name) delete [] fFlavorInfo.name;
	if (fFlavorInfo.info) delete [] fFlavorInfo.info;
	if (fFlavorInfo.in_formats) delete [] fFlavorInfo.in_formats;
	if (fFlavorInfo.out_formats) delete [] fFlavorInfo.out_formats;

	delete_sem(fResetSem);
	resume_thread(fWatcherThread);
	wait_for_thread(fWatcherThread, &_);
	while (f1394Cards.IsEmpty() == false)
		close_1394((int)f1394Cards.RemoveItem(0L));
}


status_t 
DVAddOn::InitCheck(const char **out_failure_text)
{
	if (fInitStatus < B_OK) {
		*out_failure_text = "Unknown error";
		return fInitStatus;
	}

	return B_OK;
}

int32 
DVAddOn::CountFlavors()
{
	BAutolock _(&fFlavorRoster);
	return 2 * fFlavorRoster.CountFlavors();
}

/*
 * The pointer to the flavor received only needs to be valid between calls to
 * BMediaAddOn::GetFlavorAt().
 */
status_t 
DVAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	BAutolock _(&fFlavorRoster);
	Flavor *f = fFlavorRoster.GetFlavorAt(n / 2);
	const flavor_info *fi;
	media_format *mf;

	*out_info = NULL;
	if (!f)
		return B_BAD_INDEX;
	fi = (!(n & 1)) ? f->ProducerFlavorInfo() : f->ConsumerFlavorInfo();

	if (fFlavorInfo.name) delete [] fFlavorInfo.name;
	if (fFlavorInfo.info) delete [] fFlavorInfo.info;
	if (fFlavorInfo.in_formats) delete [] fFlavorInfo.in_formats;
	if (fFlavorInfo.out_formats) delete [] fFlavorInfo.out_formats;

	memset(&fFlavorInfo, 0, sizeof(fFlavorInfo));
	fFlavorInfo.name = new char[strlen(fi->name) + 1];
	strcpy(fFlavorInfo.name, fi->name);
	fFlavorInfo.info = new char[strlen(fi->info) + 1];
	strcpy(fFlavorInfo.info, fi->info);
	fFlavorInfo.kinds = fi->kinds;
	fFlavorInfo.flavor_flags = fi->flavor_flags;
	fFlavorInfo.internal_id = fi->internal_id;
	fFlavorInfo.possible_count = fi->possible_count;
	fFlavorInfo.in_format_count = fi->in_format_count;
	fFlavorInfo.in_format_flags = fi->in_format_flags;
	mf = new media_format[fFlavorInfo.in_format_count];
	memcpy(mf, fi->in_formats,
			sizeof(media_format) * fFlavorInfo.in_format_count);
	fFlavorInfo.in_formats = mf;
	fFlavorInfo.out_format_count = fi->out_format_count;
	fFlavorInfo.out_format_flags = fi->out_format_flags;
	mf = new media_format[fFlavorInfo.out_format_count];
	memcpy(mf, fi->out_formats,
			sizeof(media_format) * fFlavorInfo.out_format_count);
	fFlavorInfo.out_formats = mf;

	*out_info = &fFlavorInfo;
	return B_OK;
}

BMediaNode *
DVAddOn::InstantiateNodeFor(
		const flavor_info *info, BMessage *config, status_t *out_error)
{
	BAutolock _(&fFlavorRoster);

	if (info->kinds & B_BUFFER_PRODUCER)
		return fFlavorRoster.InstantiateProducerNodeFor(
				info->internal_id, config, out_error);
	else
		return fFlavorRoster.InstantiateConsumerNodeFor(
				info->internal_id, config, out_error);
}

status_t
DVAddOn::AcquireGUID(int32 bus, uint64 guid)
{
	BAutolock _(&fFlavorRoster);

	if ((bus < 0) || (bus >= f1394Cards.CountItems()))
		return B_BAD_INDEX;

	return ioctl((int)f1394Cards.ItemAt(bus),
			B_1394_ACQUIRE_GUID, &guid, sizeof(guid));
}

status_t
DVAddOn::ReleaseGUID(int32 bus, uint64 guid)
{
	BAutolock _(&fFlavorRoster);

	if ((bus < 0) || (bus >= f1394Cards.CountItems()))
		return B_BAD_INDEX;

	return ioctl((int)f1394Cards.ItemAt(bus),
			B_1394_RELEASE_GUID, &guid, sizeof(guid));
}

void
DVAddOn::FindCameras()
{
	// XXX multiple subunits
	BAutolock _(&fFlavorRoster);

	fFlavorRoster.MarkFlavorsMissing();

	for (int32 card=0;card<f1394Cards.CountItems();card++) {
		int fd = (int)f1394Cards.ItemAt(card);
		ieee1394_bus_info b;

		b.bus_status = B_1394_BUS_RESET;
		for (int32 i=0;(i<30) && (b.bus_status == B_1394_BUS_RESET);i++) {
			snooze(50000);
	
			if (get_1394_bus_info(fd, &b) < 0) {
				printf("Error getting bus info for card %ld\n", card);
				break;
			}
		}

		if (b.bus_status != B_OK) {
			printf("Timed out waiting for bus reset for card %ld\n", card);
			continue;
		}
	
		for (uint32 i=0;i<b.nodes;i++) {
			struct ieee1394_node_info n;

			if (get_1394_node_info(fd, b.guids[i], &n) < 0)
				continue;

			if (	(n.num_units == 1) &&
					(	((n.units[0].sw_version & 0xff0001) == 0x10001) ||
						// Support Sony DVMC-DA1, which reports a 
						// bad unit sw version
						(n.units[0].sw_version == 0x10000))) {
				struct avc_subunit subunit;
				uchar mode = 0;

				subunit.fd = fd;
				subunit.guid = b.guids[i];
				subunit.type = AVC_TAPE_PLAYER_RECORDER;
				subunit.id = 0;
				/* XXX: error */
				vcr_get_output_signal_mode(&subunit, &mode, 500000LL);
				fFlavorRoster.CreateFlavor(card, b.guids[i], mode == 0x80);
			}
		}
	}

	fFlavorRoster.NotifyFlavors();
}

int32 
DVAddOn::_device_watcher_(void *data)
{
	return ((DVAddOn *)data)->DeviceWatcher();
}

int32 
DVAddOn::DeviceWatcher()
{
	int32 card;

	for (card=0;card<f1394Cards.CountItems();card++)
		register_1394_bus_reset_notification((int)f1394Cards.ItemAt(card),
				fResetSem);

	while (acquire_sem(fResetSem) == B_OK) {
		snooze(1000);
		while (acquire_sem_etc(fResetSem, 1, B_TIMEOUT, 0) == B_OK)
			;

		FindCameras();

		NotifyFlavorChange();
	}

	for (card=0;card<f1394Cards.CountItems();card++)
		unregister_1394_bus_reset_notification((int)f1394Cards.ItemAt(card),
				fResetSem);

	return B_OK;
}

BMediaAddOn *
make_media_addon(image_id myImage)
{
	return new DVAddOn(myImage);
}
