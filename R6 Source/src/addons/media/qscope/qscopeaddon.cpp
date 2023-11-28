#include "qscopeaddon.h"
#include "QScopeConsumer.h"

#include <stdio.h>
#include <string.h>

QScopeAddOn::QScopeAddOn(image_id myImage)
	: BMediaAddOn(myImage)
{
	printf("QScopeAddOn::QScopeAddOn() - BEGIN\n");
}

const char *errorString = "QScopeAddOn::Standard Error String";

status_t 
QScopeAddOn::InitCheck(const char **out_failure_text)
{
	*out_failure_text = errorString;
	
	return B_OK;
}

int32 
QScopeAddOn::CountFlavors()
{
	printf("QScopeAddOn::CountFlavors()\n");

	return 1;
}

/*
	Note: n should be const
	Bugs: Server does not behave properly
	when status returned is B_ERROR, and *out_info == 0
*/

status_t 
QScopeAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	printf("QScopeAddOn::GetFlavorAt(%d,...)\n", n);

	if (n != 0)
		return B_ERROR;
		
	flavor_info *newInfo = new flavor_info;
	newInfo->internal_id = n;
	newInfo->name = new char [256];
	strcpy(newInfo->name, "qscope");
	newInfo->info = new char [256];
	strcpy(newInfo->info, "Good information that will be very useful to you.");
	newInfo->kinds = B_BUFFER_CONSUMER;
	newInfo->flavor_flags = 0;
	newInfo->possible_count = 0;
	
	// Output formats, none
	newInfo->out_format_count = 0;
	newInfo->out_formats = 0;
	newInfo->out_format_flags = 0;
	
	// Input formats, only one
	newInfo->in_format_count = 1;
	media_format * aFormat = new media_format;
	aFormat->type = B_MEDIA_RAW_AUDIO;
	aFormat->u.raw_audio = media_raw_audio_format::wildcard;
	newInfo->in_formats = aFormat;
	newInfo->in_format_flags = 0;
	
	*out_info= newInfo;
	
	return B_OK;
}

BMediaNode *
QScopeAddOn::InstantiateNodeFor(const flavor_info *info, BMessage *config, status_t *out_error)
{
	printf("QScopeAddOn::InstantiateNodeFor()\n");

	QScopeConsumer *aNode = new QScopeConsumer(this);
	
	return aNode;
}

status_t 
QScopeAddOn::GetConfigurationFor(BMediaNode *your_node, BMessage *into_message)
{
	printf("QScopeAddOn::GetConfiguratoinFor()\n");
	// We don't really save anything at this point
}

bool 
QScopeAddOn::WantsAutoStart()
{
	printf("QScopeAddOn::WantsAutoStart()\n");
	return true;
}


status_t 
QScopeAddOn::AutoStart(int in_count, BMediaNode ** out_node,
	int32 * out_internal_id,
	bool * out_has_more)
{
	printf("QScopeAddOn::AutoStart()\n");
	QScopeConsumer *aNode = new QScopeConsumer(this);
	*out_node = aNode;
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
	return new QScopeAddOn(myImage);
}


