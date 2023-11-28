#ifndef _VIDEO_WINDOW_ADD_ON_H
#define _VIDEO_WINDOW_ADD_ON_H

#include <image.h>
#include <MediaAddOn.h>

// export this for the input_server
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id you);


class BVideoWindowAddOn : public BMediaAddOn
{
public:
			BVideoWindowAddOn(image_id myImage);
			~BVideoWindowAddOn();
			
	virtual	status_t	InitCheck(const char ** out_failure_text);
	virtual	int32		CountFlavors();
	virtual	status_t	GetFlavorAt(int32 n, const flavor_info ** out_info);
	virtual	BMediaNode*	InstantiateNodeFor(	const flavor_info * info,
											BMessage * config,
											status_t * out_error);
	
	// Saving and restoring configuration information
	virtual	status_t 	GetConfigurationFor(BMediaNode * your_node, BMessage * into_message);
	virtual	status_t	SaveConfigInfo(	BMediaNode * your_node,
										BMessage * into_message);
	
	virtual	bool		WantsAutoStart();
	virtual	status_t	AutoStart(	int in_count, BMediaNode ** out_node,
									int32 * out_internal_id,
									bool * out_has_more);

protected:
private:
		//	BMediaNode		*mNode;
			flavor_info		*mFlavorInfo;

};

#endif

