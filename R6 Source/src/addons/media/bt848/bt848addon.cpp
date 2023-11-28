#include "bt848addon.h"
#include "Bt848Controllable.h"

#include <stdio.h>
#include <sys/stat.h>
#include <Autolock.h>

#define DO_NOTHING(x...)

#if DEBUG
#define PRINTF printf
#else
#define PRINTF DO_NOTHING
#endif

#define TRACE 		PRINTF
#define FUNCTION	PRINTF
#define ERROR		PRINTF

Bt848MediaAddOn::Bt848MediaAddOn(image_id myImage)
	: BMediaAddOn(myImage),
	m_lock("Bt848 AddOn Lock")
{
	FUNCTION("Bt848MediaAddOn::Bt848MediaAddOn\n");
	m_card_count = 0;
	struct stat st;
	char path[256];
	while (m_card_count < MAX_CARDS) {
		sprintf(path, "/dev/video/bt848/%ld", m_card_count);
		if (stat(path, &st) < 0)
			 break;
		
		m_flavor_info[m_card_count] = new flavor_info;
		m_flavor_info[m_card_count]->internal_id = m_card_count;
		m_flavor_info[m_card_count]->name = new char[64];
		sprintf(m_flavor_info[m_card_count]->name, "Bt848 In %ld", m_card_count+1);
		m_flavor_info[m_card_count]->info = new char[64];;
		sprintf(m_flavor_info[m_card_count]->info, "Bt848 In %ld", m_card_count+1);
		m_flavor_info[m_card_count]->kinds = B_BUFFER_PRODUCER | B_CONTROLLABLE | B_PHYSICAL_INPUT;
		m_flavor_info[m_card_count]->flavor_flags = 0;
		m_flavor_info[m_card_count]->possible_count = 1;
		
		// Input formats, none
		m_flavor_info[m_card_count]->in_format_count = 0;
		m_flavor_info[m_card_count]->in_formats = 0;
		m_flavor_info[m_card_count]->in_format_flags = 0;
		
		// Output formats, only one
		m_flavor_info[m_card_count]->out_format_count = 1;

		media_format *outputs = new media_format[1];
		outputs[0].type = B_MEDIA_RAW_VIDEO;
		outputs[0].u.raw_video = media_raw_video_format::wildcard;
		
		m_flavor_info[m_card_count]->out_formats = outputs;
		m_flavor_info[m_card_count]->out_format_flags = 0;
		
		m_card_count++;
	}
}

Bt848MediaAddOn::~Bt848MediaAddOn()
{
	FUNCTION("Bt848MediaAddOn::~Bt848MediaAddOn\n");
	for (int i = 0; i < m_card_count; i++)
	{
		delete [] m_flavor_info[i]->out_formats;
		delete [] m_flavor_info[i]->info;
		delete [] m_flavor_info[i]->name;
		delete m_flavor_info[i];		
	}
}


status_t 
Bt848MediaAddOn::InitCheck(const char **out_failure_text)
{
	FUNCTION("Bt848MediaAddOn::InitCheck\n");
	
	status_t err = B_OK;
	static char errorString[256];
	errorString[0] = 0;
	
	if (m_card_count < 1) {
		strcpy(errorString, "There are no Bt848 compatible video input cards available.");
		ERROR(errorString);
		err = ENOENT;
	}
	*out_failure_text = errorString;
	return err;
}

int32 
Bt848MediaAddOn::CountFlavors()
{
	FUNCTION("Bt848MediaAddOn::CountFlavors\n");

	return m_card_count;
}

status_t 
Bt848MediaAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	FUNCTION("Bt848MediaAddOn::GetFlavorAt(%d,...)\n", n);

	if (n >= m_card_count)
		return B_ERROR;		
	
	*out_info= m_flavor_info[n];
	
	return B_OK;
}

BMediaNode *
Bt848MediaAddOn::InstantiateNodeFor(const flavor_info *info, BMessage */*config*/, status_t *out_error)
{
	FUNCTION("Bt848MediaAddOn::InstantiateNodeFor\n");

	char path[256];
	sprintf(path, "/dev/video/bt848/%ld", info->internal_id);
	BBt848Controllable *bt848 = new BBt848Controllable(info->internal_id, path,
		m_flavor_info[info->internal_id]->name, this);
	if(bt848 == NULL) {
		*out_error = B_NO_MEMORY;
		return NULL;
	}
	if(bt848->InitCheck() < B_OK) {
		*out_error = bt848->InitCheck();
		bt848->Release();
		return NULL;
	}
	*out_error = B_OK;
	return bt848;
}

status_t
Bt848MediaAddOn::GetConfigurationFor(BMediaNode * /*your_node*/, BMessage * /*into_message*/)
{
	FUNCTION("Bt848MediaAddOn::GetConfigurationFor\n");
	return B_OK;
}

status_t 
Bt848MediaAddOn::SaveConfigInfo(BMediaNode */*your_node*/, BMessage */*into_message*/)
{
	FUNCTION("Bt848MediaAddOn::SaveConfigInfo()\n");
	return B_OK;
}

bool 
Bt848MediaAddOn::WantsAutoStart()
{
	FUNCTION("Bt848MediaAddOn::WantsAutoStart()\n");
	return false;
}

status_t 
Bt848MediaAddOn::AutoStart(int /*in_count*/, BMediaNode ** /*out_node*/,
	int32 * /*out_internal_id*/,
	bool * /*out_has_more*/)
{
	FUNCTION("Bt848MediaAddOn::AutoStart\n");
	return B_ERROR;
}

/*
	Function: make_media_addon
	
	This is the function that is exported for the add-on.
	The server looks for it so it can instantiate addons.
*/

BMediaAddOn *
make_media_addon(image_id myImage)
{
	FUNCTION("Bt848MediaAddOn - make_media_addon\n");
	return new Bt848MediaAddOn(myImage);
}


