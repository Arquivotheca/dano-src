#include "videoaddon.h"
#include "VideoConsumer.h"

#include <stdio.h>
#include <string.h>

#if DEBUG
#define PRINTF printf
#else
#define PRINTF (void)
#endif

#define FUNCTION	PRINTF
#define ERROR		PRINTF

BVideoWindowAddOn::BVideoWindowAddOn(image_id myImage)
	: BMediaAddOn(myImage)
{
	FUNCTION("BVideoWindowAddOn::BVideoWindowAddOn\n");
	mFlavorInfo = new flavor_info;
	mFlavorInfo->internal_id = 0;
	mFlavorInfo->name = new char [256];
	strcpy(mFlavorInfo->name, "Video Window Consumer");
	mFlavorInfo->info = new char [256];
	strcpy(mFlavorInfo->info, "This node uses the app server for color space conversion and scaling");
	mFlavorInfo->kinds = B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT /*| B_CONTROLLABLE */;
	mFlavorInfo->flavor_flags = B_FLAVOR_IS_LOCAL;
	mFlavorInfo->possible_count = 1;
	
	// Input formats, for buffer consumer
	mFlavorInfo->in_format_count = 1;
	media_format *input = new media_format[1];
	input[0].type = B_MEDIA_RAW_VIDEO;
	input[0].u.raw_video = media_raw_video_format::wildcard;	
	mFlavorInfo->in_formats = input;
	mFlavorInfo->in_format_flags = 0;
	
	// Output formats, none
	mFlavorInfo->out_format_count = 0;
	mFlavorInfo->out_formats = 0;
	mFlavorInfo->out_format_flags = 0;
}

BVideoWindowAddOn::~BVideoWindowAddOn()
{
	FUNCTION("BVideoWindowAddOn::~BVideoWindowAddOn\n");
	delete [] mFlavorInfo->in_formats;
	delete [] mFlavorInfo->info;
	delete [] mFlavorInfo->name;
	delete mFlavorInfo;
}

const char *errorString = "BVideoWindowAddOn::Standard Error String";

status_t 
BVideoWindowAddOn::InitCheck(const char **out_failure_text)
{
	*out_failure_text = errorString;
	
	return B_OK;
}

int32 
BVideoWindowAddOn::CountFlavors()
{
	FUNCTION("BVideoWindowAddOn::CountFlavors()\n");

	return 1;
}

status_t 
BVideoWindowAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	FUNCTION("BVideoWindowAddOn::GetFlavorInfo(%d,...)\n", n);

	if (n != 0)
		return B_ERROR;		
	
	*out_info= mFlavorInfo;	
	return B_OK;
}

BMediaNode *
BVideoWindowAddOn::InstantiateNodeFor(const flavor_info *info, BMessage *config, status_t *out_error)
{
	FUNCTION("BWavegenAddOn::MakeFlavorInstance()\n");

	int32 id;
	bool more;
	BMediaNode *node;
	*out_error = AutoStart(0, &node, &id, &more);
	if (*out_error < B_OK) return NULL;
	return node;
}

status_t 
BVideoWindowAddOn::GetConfigurationFor(BMediaNode *your_node, BMessage *into_message)
{
	return B_OK;
}

status_t 
BVideoWindowAddOn::SaveConfigInfo(BMediaNode *your_node, BMessage *into_message)
{
	FUNCTION("BVideoWindowAddOn::SaveConfigInfo()\n");
	return B_OK;
}

bool 
BVideoWindowAddOn::WantsAutoStart()
{
	FUNCTION("BVideoWindowAddOn::WantsAutoStart()\n");
	return false; //true;
}

status_t 
BVideoWindowAddOn::AutoStart(int in_count, BMediaNode ** out_node,
	int32 * out_internal_id,
	bool * out_has_more)
{
	FUNCTION("BVideoWindowAddOn::AutoStart()\n");

	if (in_count != 0) {
		*out_node = NULL;
		*out_has_more = false;
		ERROR("BVideoWindowAddOn::AutoStart failed, BAD_INDEX\n");
		return B_BAD_INDEX;
	}
	
	*out_node  = new VideoConsumer(0, "Video Window Consumer", this);
	*out_has_more = false;
	*out_internal_id = 0;
	
	return B_OK;
}

/*
	Function: make_media_addon
	
	This is the function that is exported for the add-on.
	The server looks for it so it can instantiate addons.
*/

BMediaAddOn *
make_media_addon(image_id myImage)
{
	return new BVideoWindowAddOn(myImage);
}


