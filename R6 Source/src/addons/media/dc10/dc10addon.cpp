#include "dc10addon.h"
#include "dc10producer.h"
#include "dc10consumer.h"

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <Autolock.h>


#define PRINTF(x...)

#define TRACE 		PRINTF
#define FUNCTION	PRINTF
#define ERROR		PRINTF

dc10MediaAddOn::dc10MediaAddOn(image_id myImage)
	: BMediaAddOn(myImage),
	m_lock("dc10 AddOn Lock")
{
	FUNCTION("dc10MediaAddOn::dc10MediaAddOn\n");
	m_card_count = 0;
	struct stat st;
	char path[256];
	while (m_card_count < MAX_CARDS) {
		sprintf(path, "/dev/video/dc10/%ld", m_card_count);
		if (stat(path, &st) < 0)
			 break;
		//input
		m_flavor_info[2*m_card_count] = new flavor_info;
		m_flavor_info[2*m_card_count]->internal_id = 2*m_card_count;
		m_flavor_info[2*m_card_count]->name = new char[64];
		sprintf(m_flavor_info[2*m_card_count]->name, "dc10 In %ld", m_card_count+1);
		m_flavor_info[2*m_card_count]->info = new char[64];;
		sprintf(m_flavor_info[2*m_card_count]->info, "dc10 In %ld", m_card_count+1);
		m_flavor_info[2*m_card_count]->kinds = B_BUFFER_PRODUCER | B_CONTROLLABLE | B_PHYSICAL_INPUT;
		m_flavor_info[2*m_card_count]->flavor_flags = 0;
		m_flavor_info[2*m_card_count]->possible_count = 1;
		
		// Input formats, none
		m_flavor_info[2*m_card_count]->in_format_count = 0;
		m_flavor_info[2*m_card_count]->in_formats = 0;
		m_flavor_info[2*m_card_count]->in_format_flags = 0;
		
		// Output formats, only one
		m_flavor_info[2*m_card_count]->out_format_count = 1;

		media_format *outputs = new media_format[1];
		outputs[0].type = B_MEDIA_ENCODED_VIDEO;
		outputs[0].u.encoded_video = media_encoded_video_format::wildcard;
		
		m_flavor_info[2*m_card_count]->out_formats = outputs;
		m_flavor_info[2*m_card_count]->out_format_flags = 0;


		//output
		m_flavor_info[2*m_card_count+1] = new flavor_info;
		m_flavor_info[2*m_card_count+1]->internal_id = 2*m_card_count+1;
		m_flavor_info[2*m_card_count+1]->name = new char[64];
		sprintf(m_flavor_info[2*m_card_count+1]->name, "dc10 Out %ld", m_card_count+1);
		m_flavor_info[2*m_card_count+1]->info = new char[64];;
		sprintf(m_flavor_info[2*m_card_count+1]->info, "dc10 Out %ld", m_card_count+1);
		m_flavor_info[2*m_card_count+1]->kinds = B_BUFFER_CONSUMER | B_CONTROLLABLE | B_PHYSICAL_OUTPUT;
		m_flavor_info[2*m_card_count+1]->flavor_flags = B_FLAVOR_IS_GLOBAL;
		m_flavor_info[2*m_card_count+1]->possible_count = 1;
		
		// Output formats, none
		m_flavor_info[2*m_card_count+1]->out_format_count = 0;
		m_flavor_info[2*m_card_count+1]->out_formats = 0;
		m_flavor_info[2*m_card_count+1]->out_format_flags = 0;
		
		// Input formats, only one
		m_flavor_info[2*m_card_count+1]->in_format_count = 1;

		media_format *inputs = new media_format[1];
		inputs[0].type = B_MEDIA_ENCODED_VIDEO;
		inputs[0].u.encoded_video = media_encoded_video_format::wildcard;
		
		m_flavor_info[2*m_card_count+1]->in_formats = inputs;
		m_flavor_info[2*m_card_count+1]->in_format_flags = 0;
				
		m_card_count++;
	}
}

dc10MediaAddOn::~dc10MediaAddOn()
{
	FUNCTION("dc10MediaAddOn::~dc10MediaAddOn\n");
	for (int i = 0; i < 2*m_card_count; i++)
	{
		delete [] m_flavor_info[i]->out_formats;
		delete [] m_flavor_info[i]->info;
		delete [] m_flavor_info[i]->name;
		delete m_flavor_info[i];		
	}
}


status_t 
dc10MediaAddOn::InitCheck(const char **out_failure_text)
{
	FUNCTION("dc10MediaAddOn::InitCheck\n");
	
	status_t err = B_OK;
	static char errorString[256];
	errorString[0] = 0;
	
	if (m_card_count < 1) {
		strcpy(errorString, "There are no dc10 compatible video input cards available.");
		ERROR(errorString);
		err = ENOENT;
	}
	else
	{
		TRACE("%d dc10 compatible video input cards available/n",m_card_count);
	}
	*out_failure_text = errorString;
	return err;
}

int32 
dc10MediaAddOn::CountFlavors()
{
	FUNCTION("dc10MediaAddOn::CountFlavors\n");

	return 2*m_card_count;
}

status_t 
dc10MediaAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	FUNCTION("dc10MediaAddOn::GetFlavorAt(%d,...)\n", n);

	if (n >= 2*m_card_count)
		return B_ERROR;		
	
	*out_info= m_flavor_info[n];
	
	return B_OK;
}

BMediaNode *
dc10MediaAddOn::InstantiateNodeFor(const flavor_info *info, BMessage */*config*/, status_t *out_error)
{
	FUNCTION("dc10MediaAddOn::InstantiateNodeFor %d\n",info->internal_id);

	char path[256];
	sprintf(path, "/dev/video/dc10/%ld", info->internal_id / 2);
	if((info->internal_id&1)==0)
	{
		dc10producer *dc10p = new dc10producer(info->internal_id, path,
				m_flavor_info[info->internal_id]->name, this);
		if(dc10p == NULL) 
		{
			FUNCTION("dc10MediaAddOn::InstantiateNodeFor out of memory\n");
			*out_error = B_NO_MEMORY;
			return NULL;
		}
		/*
		if(dc10->InitCheck() < B_OK) 
		{
			FUNCTION("dc10MediaAddOn::InstantiateNodeFor init check ko\n");
			*out_error = dc10->InitCheck();
			dc10->Release();
			return NULL;
		}
		*/
		*out_error = B_OK;
		return dc10p;
	}
	if((info->internal_id&1)==1)
	{
		dc10consumer *dc10c = new dc10consumer(info->internal_id, 
											   path,
											   m_flavor_info[info->internal_id]->name,
											   this);
		if(dc10c == NULL) 
		{
			FUNCTION("dc10MediaAddOn::InstantiateNodeFor out of memory\n");
			*out_error = B_NO_MEMORY;
			return NULL;
		}
		/*
		if(dc10->InitCheck() < B_OK) 
		{
			FUNCTION("dc10MediaAddOn::InstantiateNodeFor init check ko\n");
			*out_error = dc10->InitCheck();
			dc10->Release();
			return NULL;
		}
			*/
		*out_error = B_OK;
		return dc10c;
	}
	*out_error = B_ERROR;
	return NULL;
}

status_t
dc10MediaAddOn::GetConfigurationFor(BMediaNode * /*your_node*/, BMessage * /*into_message*/)
{
	FUNCTION("dc10MediaAddOn::GetConfigurationFor\n");
	return B_OK;
}

status_t 
dc10MediaAddOn::SaveConfigInfo(BMediaNode */*your_node*/, BMessage */*into_message*/)
{
	FUNCTION("dc10MediaAddOn::SaveConfigInfo()\n");
	return B_OK;
}

bool 
dc10MediaAddOn::WantsAutoStart()
{
	FUNCTION("dc10MediaAddOn::WantsAutoStart()\n");
	return false;
}

status_t 
dc10MediaAddOn::AutoStart(int /*in_count*/, BMediaNode ** /*out_node*/,
	int32 * /*out_internal_id*/,
	bool * /*out_has_more*/)
{
	FUNCTION("dc10MediaAddOn::AutoStart\n");
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
	FUNCTION("dc10MediaAddOn - make_media_addon\n");
	return new dc10MediaAddOn(myImage);
}


