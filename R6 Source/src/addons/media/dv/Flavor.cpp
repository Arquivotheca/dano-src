#include <support/Autolock.h>
#ifndef DEBUG
	#define DEBUG 1
#endif
#include <support/Debug.h>
#include <media/MediaFormats.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "AddOn.h"
#include "Consumer.h"
#include "Producer.h"
#include "DVDefs.h"
#include "Flavor.h"

#include "oui.h"

#include "debug.h"

int32 Flavor::fLastAssignedFlavorID = 0;

#define DEBUG_PREFIX "Flavor::"

Flavor::Flavor(BMediaAddOn *addon, FlavorRoster *roster)
{
	ASSERT(fLastAssignedFlavorID == 0);

	Initialize(addon, roster, -1, 0ULL, false, true, "Default %s");
}

Flavor::Flavor(BMediaAddOn *addon, FlavorRoster *roster,
		int32 bus, uint64 GUID, bool PAL)
{
	PRINTF(1, ("Creating new flavor (id %lx)\n", fLastAssignedFlavorID));

	Initialize(addon, roster, bus, GUID, PAL, false, "%s (%s)");
}

void
Flavor::Initialize(BMediaAddOn *addon, FlavorRoster *roster,
		int32 bus, uint64 guid, bool PAL,
		bool is_default, const char *name)
{
	media_format *m;

	fInitStatus = B_ERROR;

	fAddOn = addon;
	fFlavorRoster = roster;
	fBus = bus;
	fGUID = guid;
	fPAL = PAL;
	fProducerNode = fConsumerNode = NULL;
	fDefault = is_default;
	fPresent = !fDefault;

	memset(&fProducerFlavorInfo, 0, sizeof(fProducerFlavorInfo));
	snprintf(fProducerName, sizeof(fProducerName), name,
			"DV Input", lookup_oui(guid >> 40));
	fProducerFlavorInfo.internal_id = atomic_add(&fLastAssignedFlavorID, 1);
	fProducerFlavorInfo.name = fProducerName;
	fProducerFlavorInfo.info = fProducerName;
	fProducerFlavorInfo.kinds = B_BUFFER_PRODUCER | B_CONTROLLABLE | B_PHYSICAL_INPUT;
	fProducerFlavorInfo.flavor_flags = 0;
	fProducerFlavorInfo.possible_count = 1;
	
	// Input formats, none
	fProducerFlavorInfo.in_format_count = 0;
	fProducerFlavorInfo.in_formats = 0;
	fProducerFlavorInfo.in_format_flags = 0;

	// Output formats, only one
	fProducerFlavorInfo.out_format_count = 1;
	m = new media_format[1];
		m[0].type = B_MEDIA_ENCODED_VIDEO;
		m[0].u.encoded_video = dv_ntsc_format;
	fProducerFlavorInfo.out_formats = m;
	fProducerFlavorInfo.out_format_flags = 0;

	memset(&fConsumerFlavorInfo, 0, sizeof(fConsumerFlavorInfo));
	snprintf(fConsumerName, sizeof(fConsumerName), name,
			"DV Output", lookup_oui(guid >> 40));
	fConsumerFlavorInfo.internal_id = atomic_add(&fLastAssignedFlavorID, 1);
	fConsumerFlavorInfo.name = fConsumerName;
	fConsumerFlavorInfo.info = fConsumerName;
	fConsumerFlavorInfo.kinds = B_BUFFER_CONSUMER | B_CONTROLLABLE | B_PHYSICAL_OUTPUT;
	fConsumerFlavorInfo.flavor_flags = 0;
	fConsumerFlavorInfo.possible_count = 1;

	// Only report the video input here; otherwise, the Media preferences panel
	// will think the flavor can be the default audio output.
	fConsumerFlavorInfo.in_format_count = 1;
	m = new media_format[1];
		m[0].type = B_MEDIA_ENCODED_VIDEO;
		m[0].u.encoded_video = dv_ntsc_format;
	fConsumerFlavorInfo.in_formats = m;
	fConsumerFlavorInfo.in_format_flags = 0;

	fConsumerFlavorInfo.out_format_count = 0;
	fConsumerFlavorInfo.out_formats = 0;
	fConsumerFlavorInfo.out_format_flags = 0;

	fInitStatus = B_OK;
}

Flavor::~Flavor()
{
	delete [] fProducerFlavorInfo.out_formats;
	delete [] fConsumerFlavorInfo.in_formats;
}

status_t Flavor::SetTo(int32 bus, uint64 guid, bool PAL)
{
	ASSERT(fFlavorRoster->IsLocked());
	ASSERT(fDefault);

	fBus = bus;
	fGUID = guid;
	fPAL = PAL;
	fPresent = (bus >= 0);

	media_format *m = (media_format *)
		fProducerFlavorInfo.out_formats;
	m[0].u.encoded_video = PAL ? dv_pal_format : dv_ntsc_format;
	m = (media_format *)fConsumerFlavorInfo.in_formats;
	m[0].u.encoded_video = PAL ? dv_pal_format : dv_ntsc_format;

	if (fProducerNode) {
		fProducerNode->SetTo(bus, guid, PAL);
		fProducerNode->NotifyPresence(fPresent);
	}

	if (fConsumerNode) {
		fConsumerNode->SetTo(bus, guid, PAL);
		fConsumerNode->NotifyPresence(fPresent);
	}

	return B_OK;
}

BMediaNode *Flavor::ProducerNode()
{
	ASSERT(fFlavorRoster->IsLocked());

	if (fProducerNode == NULL) {
		DVBufferProducer *node;

		node = new DVBufferProducer(fAddOn, fFlavorRoster,
				fProducerName, fProducerFlavorInfo.internal_id,
				fBus, fGUID, fPAL);
		if (!node)
			return NULL;

		PRINTF(1, ("Instantiated flavor %lx for GUID %Lx\n", \
				fProducerFlavorInfo.internal_id, fGUID));

		if (node->InitCheck() < B_OK) {
			delete node;
			return NULL;
		}

		fProducerNode = node;
	}

	return fProducerNode;
}

BMediaNode *Flavor::ConsumerNode()
{
	ASSERT(fFlavorRoster->IsLocked());

	if (fConsumerNode == NULL) {
		DVBufferConsumer *node;

		node = new DVBufferConsumer(fAddOn, fFlavorRoster,
				fConsumerName, fConsumerFlavorInfo.internal_id,
				fBus, fGUID, fPAL);
		if (!node)
			return NULL;

		PRINTF(1, ("Instantiated flavor %lx for GUID %Lx\n", \
				fConsumerFlavorInfo.internal_id, fGUID));

		if (node->InitCheck() < B_OK) {
			delete node;
			return NULL;
		}

		fConsumerNode = node;
	}

	return fConsumerNode;
}

#undef DEBUG_PREFIX
#define DEBUG_PREFIX "FlavorRoster::"

FlavorRoster::FlavorRoster(DVAddOn *addon)
{
	fAddOn = addon;
}

FlavorRoster::~FlavorRoster()
{
}

void FlavorRoster::CreateDefaultFlavor()
{
	ASSERT(fFlavors.IsEmpty());
	fDefaultFlavor = new Flavor(fAddOn, this);
	fFlavors.AddItem(fDefaultFlavor);
}

int32 FlavorRoster::CountFlavors()
{
	int32 n = 0;

	ASSERT(IsLocked());

	for (int32 i=0;;i++) {
		Flavor *f = (Flavor *)fFlavors.ItemAt(i);
		if (!f)
			break;
		if (f->fPresent || f->fProducerNode || f->fConsumerNode || f->fDefault)
			n++;
	}
	PRINTF(1, ("0x%lx flavors\n", n));
	return n;
}

Flavor *FlavorRoster::GetFlavorAt(int32 n)
{
	ASSERT(IsLocked());

	for (int32 i=0;;i++) {
		Flavor *f = (Flavor *)fFlavors.ItemAt(i);
		if (!f)
			break;
		if (f->fPresent || f->fProducerNode || f->fConsumerNode || f->fDefault) {
			if (n-- == 0) {
				PRINTF(1, ("(0x%lx) = %p\n", n, f));
				return f;
			}
		}
	}

	PRINTF(1, ("(0x%lx) = NULL\n", n));

	return NULL;
}

BMediaNode *FlavorRoster::InstantiateProducerNodeFor(
		int32 id, BMessage *config, status_t *out_error)
{
	TOUCH(config);

	ASSERT(IsLocked());
	
	for (int32 i=0;;i++) {
		Flavor *f = (Flavor *)fFlavors.ItemAt(i);
		if (!f)
			break;
		if (f->ProducerFlavorInfo()->internal_id == id) {
			if (f->fProducerNode) {
				*out_error = EALREADY;
				return NULL;
			}
			*out_error = B_OK;
			return f->ProducerNode();
		}
	}

	*out_error = B_BAD_INDEX;
	return NULL;
}

BMediaNode *FlavorRoster::InstantiateConsumerNodeFor(
		int32 id, BMessage *config, status_t *out_error)
{
	TOUCH(config);

	ASSERT(IsLocked());
	
	for (int32 i=0;;i++) {
		Flavor *f = (Flavor *)fFlavors.ItemAt(i);
		if (!f)
			break;
		if (f->ConsumerFlavorInfo()->internal_id == id) {
			if (f->fConsumerNode) {
				*out_error = EALREADY;
				return NULL;
			}
			*out_error = B_OK;
			return f->ConsumerNode();
		}
	}

	*out_error = B_BAD_INDEX;
	return NULL;
}

status_t FlavorRoster::AcquireBusAndGUID(int32 bus, uint64 guid)
{
	BAutolock _(this);

	PRINTF(2, ("(%lx, %Lx)\n", bus, guid));

	for (int32 i=1;;i++) {
		Flavor *f = (Flavor *)fFlavors.ItemAt(i);
		if (!f)
			break;
		if ((f->Bus() == bus) && (f->GUID() == guid)) {
			status_t err;

			err = fAddOn->AcquireGUID(bus, guid);
			if (err < B_OK) {
				PRINTF(2, ("(%lx, %Lx) : Error acquiring GUID\n", bus, guid));
				return err;
			}
			return B_OK;
		}
	}

	PRINTF(2, ("(%lx, %Lx) : Not found\n", bus, guid));

	return ENOENT;
}

status_t FlavorRoster::ReleaseBusAndGUID(int32 bus, uint64 guid)
{
	BAutolock _(this);
	status_t err;

	PRINTF(2, ("(%lx, %Lx)\n", bus, guid));

	/* This needs to be done first because the node corresponding to this
	 * GUID might have gone away (can happen with the default node). 
	 */
	err = fAddOn->ReleaseGUID(bus, guid);
	if (err < B_OK) {
		PRINTF(2, ("(%lx, %Lx) : Error releasing GUID (%lx)\n", bus, guid, err));
		return err;
	}

	for (int32 i=1;;i++) {
		Flavor *f = (Flavor *)fFlavors.ItemAt(i);
		if (!f)
			break;
		if ((f->Bus() == bus) && (f->GUID() == guid))
			return B_OK;
	}

	PRINTF(2, ("(%lx, %Lx) : Not found\n", bus, guid));

	return ENOENT;
}

status_t FlavorRoster::CreateFlavor(int32 bus, uint64 guid, bool PAL)
{
	status_t err;
	Flavor *f;

	ASSERT(IsLocked());

	PRINTF(1, ("bus %lx guid %Lx\n", bus, guid));

	for (int32 i=1;;i++) {
		f = (Flavor *)fFlavors.ItemAt(i);
		if (!f)
			break;
		if ((f->Bus() == bus) && (f->GUID() == guid)) {
			PRINTF(1, ("Reusing old flavor\n"));
			f->fPresent = true;
			return B_OK;
		}
	}

	f = new Flavor(fAddOn, this, bus, guid, PAL);
	if (!f)
		return B_NO_MEMORY;

	err = f->InitCheck();
	if (err < 0) {
		delete f;
		return err;
	}

	fFlavors.AddItem((void *)f);

	/* if only other flavor is default, set it to this flavor */
	if (fFlavors.CountItems() == 0)
		fDefaultFlavor->SetTo(bus, guid, PAL);

	return B_OK;
}

void FlavorRoster::MarkFlavorsMissing()
{
	ASSERT(IsLocked());

	for (int32 i=1;;i++) {
		Flavor *f = (Flavor *)fFlavors.ItemAt(i);
		if (!f)
			break;
		f->fPresent = false;
	}
}

void FlavorRoster::NotifyFlavors()
{
	Flavor *default_flavor = NULL, *original_default_flavor = NULL;

	ASSERT(IsLocked());

	for (int32 i=1;;i++) {
		Flavor *f = (Flavor *)fFlavors.ItemAt(i);
		if (!f)
			break;

		PRINTF(1, ("%lx: %x,%Lx %p %p %s\n", i, f->Bus(), f->GUID(), \
				f->fProducerNode, f->fConsumerNode, \
				f->fPresent ? "present" : "not present"));

		if (f->fPresent) {
			if (	(original_default_flavor == NULL) &&
					(f->fProducerNode || f->fConsumerNode) &&
					(f->Bus() == fDefaultFlavor->Bus()) &&
					(f->GUID() == fDefaultFlavor->GUID()))
				original_default_flavor = f;
			if (default_flavor == NULL)
				default_flavor = f;
		}

		if ((!f->fProducerNode) && (!f->fConsumerNode)) {
			if (!f->fPresent) {
				PRINTF(1, ("Deleting node %x,%Lx\n", f->Bus(), f->GUID()));
				fFlavors.RemoveItem(i);
				delete f;
				i--;
			}
			continue;
		}

		if (f->fProducerNode)
			f->fProducerNode->NotifyPresence(f->fPresent);
		if (f->fConsumerNode)
			f->fConsumerNode->NotifyPresence(f->fPresent);
	}

	/* reset default node */
	if (default_flavor) {
		if (original_default_flavor == NULL) {
			PRINTF(1, ("Setting default flavor to bus %x, GUID %Lx\n", \
					default_flavor->Bus(), default_flavor->GUID()));
			if (	(default_flavor->Bus() != fDefaultFlavor->Bus()) ||
					(default_flavor->GUID() != fDefaultFlavor->GUID())) {
				fDefaultFlavor->SetTo();
				fDefaultFlavor->SetTo(default_flavor->Bus(),
						default_flavor->GUID(), default_flavor->PAL());
			}

			if (fDefaultFlavor->fProducerNode)
				fDefaultFlavor->fProducerNode->NotifyPresence(true);
			if (fDefaultFlavor->fConsumerNode)
				fDefaultFlavor->fConsumerNode->NotifyPresence(true);
		}
	} else {
		PRINTF(1, ("Clearing default flavor\n"));
		if (fDefaultFlavor->fProducerNode)
			fDefaultFlavor->fProducerNode->NotifyPresence(false);
		if (fDefaultFlavor->fConsumerNode)
			fDefaultFlavor->fConsumerNode->NotifyPresence(false);
		fDefaultFlavor->SetTo();
	}
}

status_t FlavorRoster::RemoveNode(BMediaNode *node)
{
	BAutolock _(this);

	PRINTF(1, ("\n"));

	for (int32 i=0;;i++) {
		Flavor *f = (Flavor *)fFlavors.ItemAt(i);

		if (!f)
			break;

		if (f->fProducerNode == node) {
			PRINTF(1, ("removing node for id %lx\n", \
					f->ProducerFlavorInfo()->internal_id));
			f->fProducerNode = NULL;
			if (!f->fPresent && !f->fDefault && !f->fConsumerNode) {
				PRINTF(1, ("removing id %lx\n", \
						f->ProducerFlavorInfo()->internal_id));
				fFlavors.RemoveItem(i);
				delete f;
			}
			return B_OK;
		}

		if (f->fConsumerNode == node) {
			PRINTF(1, ("removing node for id %lx\n", \
					f->ConsumerFlavorInfo()->internal_id));
			f->fConsumerNode = NULL;
			if (!f->fPresent && !f->fDefault && !f->fProducerNode) {
				PRINTF(1, ("removing id %lx\n", \
						f->ConsumerFlavorInfo()->internal_id));
				fFlavors.RemoveItem(i);
				delete f;
			}
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}
