#ifndef _DV_FLAVOR_H
#define _DV_FLAVOR_H

#include <media/MediaAddOn.h>
#include <support/List.h>
#include <support/Locker.h>

class DVAddOn;
class DVMediaEventLooper;

class Flavor {
	friend class FlavorRoster;
public:
	status_t			InitCheck() const { return fInitStatus; }
	int					Bus() const { return fBus; }
	uint64				GUID() const { return fGUID; }
	bool				PAL() const { return fPAL; }
	const flavor_info	*ProducerFlavorInfo() const
								{ return &fProducerFlavorInfo; }
	const flavor_info	*ConsumerFlavorInfo() const
								{ return &fConsumerFlavorInfo; }
	BMediaNode			*ProducerNode();
	BMediaNode			*ConsumerNode();

private:
	Flavor(BMediaAddOn *addon, FlavorRoster *roster);
	Flavor(BMediaAddOn *addon, FlavorRoster *roster, int32 bus, uint64 guid,
			bool PAL);
	~Flavor();

	void				Initialize(BMediaAddOn *addon, FlavorRoster *roster,
								int32 bus, uint64 guid, bool PAL,
								bool is_default, const char *name);

	status_t			SetTo(int32 bus=-1, uint64 guid=0, bool PAL=false);

	status_t			fInitStatus;
	BMediaAddOn			*fAddOn;
	FlavorRoster		*fFlavorRoster;
	int					fBus;
	uint64				fGUID;
	bool				fPAL;
	flavor_info			fProducerFlavorInfo, fConsumerFlavorInfo;
	char				fProducerName[64], fConsumerName[64];
	DVMediaEventLooper	*fProducerNode, *fConsumerNode;

	bool				fDefault;
	bool				fPresent;

static 	int32			fLastAssignedFlavorID;
};

class FlavorRoster : public BLocker {
public:
	FlavorRoster(DVAddOn *addon);
	~FlavorRoster();

	void			CreateDefaultFlavor();

	int32			CountFlavors();
	Flavor			*GetFlavorAt(int32 n);
	BMediaNode		*InstantiateProducerNodeFor(
							int32 id, BMessage *config, status_t *out_error);
	BMediaNode		*InstantiateConsumerNodeFor(
							int32 id, BMessage *config, status_t *out_error);

	status_t		AcquireBusAndGUID(int32 bus, uint64 guid);
	status_t		ReleaseBusAndGUID(int32 bus, uint64 guid);

	status_t		CreateFlavor(int32 bus, uint64 guid, bool PAL);
	void			MarkFlavorsMissing();
	void			NotifyFlavors();
	status_t		RemoveNode(BMediaNode *node);

private:
	DVAddOn			*fAddOn;

	BList			fFlavors;
	Flavor			*fDefaultFlavor;
};

#endif
