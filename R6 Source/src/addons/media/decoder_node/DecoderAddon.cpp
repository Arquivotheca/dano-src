/* DecoderAddon.cpp by Simon Clarke */

#include <string.h>
#include <stdio.h>
#include <Mime.h>

#include <MediaFormats.h>
#include "Decoder.h"
#include "DecoderAddon.h"
#include "DecoderNode.h"

#define CALL(x...) //printf
#define ERRORPRINT(x...) //printf

const char *gReaderName = "Decoder Node";

extern Decoder *find_decoder(const media_format *fmt, int32 *id);

DecoderAddon::DecoderAddon(image_id image) :
BMediaAddOn(image)
{
	_AddonManager *mgr = __get_decoder_manager();
	image_id       imgid;
	int32          cookie=0, id=0;

	// this forces all the decoder add-ons to get loaded so
	// that their register_decoder() function will be called.
	// we also then release them so that they'll get unloaded
	// right away.
	while ((imgid = mgr->GetNextAddon(&cookie, &id)) > 0) {
		mgr->ReleaseAddon(id);
	}
}


status_t DecoderAddon::InitCheck(const char **out_failure_text)
{
	CALL("Decoder::InitCheck()\n");

	*out_failure_text = "Error";

	return B_OK;
}

int32 DecoderAddon::CountFlavors()
{
	CALL("Decoder::CountFlavors()\n");
	
	return 1;
}

bool DecoderAddon::WantsAutoStart()
{
	/* auto starting of node */
	return false;
}

status_t DecoderAddon::AutoStart(
						int 					/*in_count*/,
						BMediaNode 				**/*out_node*/,
						int32 					*/*out_internal_id*/,
						bool 					*/*out_has_more*/)
{
	/* this should not get called */
	return B_OK;
}

status_t DecoderAddon::GetFlavorAt(
						int32 					n,
						const flavor_info 		**out_info)
{
	CALL("Decoder::GetFlavorAt(n:%ld)\n",n);

	flavor_info					*currentFlavor;
	media_format				*inFormat,*outFormat;

	if ((n < CountFlavors()-1) && (n < 0)) {
		ERRORPRINT("DecoderAddon::GetFlavorAt() - flavor out of range.\n");
		return B_ERROR;
	}

	/* describe what we can produce and consume */

	currentFlavor = new flavor_info;
	memset(currentFlavor, 0, sizeof(flavor_info));

	currentFlavor->internal_id = 0;
	currentFlavor->name = new char[strlen(gReaderName) + 1];
	strcpy(currentFlavor->name, gReaderName);
	currentFlavor->info = 0;
	currentFlavor->kinds = B_BUFFER_PRODUCER | B_BUFFER_CONSUMER;
	currentFlavor->flavor_flags = 0;
	currentFlavor->possible_count = 10;
	
	/* output settings - outputs encoded video and raw video */
	outFormat = new media_format[2];
	outFormat[0].type = B_MEDIA_RAW_VIDEO;
	outFormat[0].u.raw_video = media_raw_video_format::wildcard;
	outFormat[1].type = B_MEDIA_RAW_AUDIO;
	outFormat[1].u.raw_audio = media_raw_audio_format::wildcard;

	/* describe the media_format data */
	currentFlavor->out_format_count = 2;
	currentFlavor->out_format_flags = 0;
	currentFlavor->out_formats = outFormat;

	inFormat = new media_format[2];
	inFormat[0].type = B_MEDIA_ENCODED_VIDEO;
	inFormat[0].u.encoded_video = media_encoded_video_format::wildcard;
	inFormat[1].type = B_MEDIA_ENCODED_AUDIO;
	inFormat[1].u.encoded_audio = media_encoded_audio_format::wildcard;

	currentFlavor->in_format_count = 2;
	currentFlavor->in_format_flags = 0;
	currentFlavor->in_formats = inFormat;									
	
	out_info[0] = currentFlavor;

	return B_OK;
}

BMediaNode *DecoderAddon::InstantiateNodeFor(
						const flavor_info 	*info,
						BMessage 			*/*config*/,
						status_t 			*out_error)
{
	BMediaNode			*returnNode = 0;

	if (!info) {
		ERRORPRINT("DecoderAddon::InstantiateNodeFor() - no info\n");
		*out_error = B_ERROR;
		return 0;
	}

	CALL("DecoderAddon::InstantiateNodeFor(id:%ld)\n",info->internal_id);

	switch (info->internal_id) {
		case 0: {
			*out_error = B_OK;
			returnNode =  new DecoderNode(this,0);
			break;
		}
		
		default: {
			*out_error = B_ERROR;
			break;
		}
	}

	CALL("DecoderAddon::InstantiateNodeFor(id:%ld) - done\n",info->internal_id);
	
	return returnNode;
}

status_t DecoderAddon::GetConfigurationFor(
						BMediaNode 			*/*your_node*/,
						BMessage 			*/*into_message*/)
{
	CALL("DecoderAddon::GetConfigurationFor()\n");

	return B_ERROR;
}

status_t DecoderAddon::SniffRef(
							const entry_ref 	&/*file*/,
							BMimeType 			*/*io_mime_type*/,
							float 				*/*out_quality*/,
							int32 				*/*out_internal_id*/)
{
	CALL("DecoderAddon::SniffRef()\n");
	return B_ERROR;
}

status_t DecoderAddon::SniffType(
							const BMimeType 	&/*type*/,
							float 				*/*out_quality*/,
							int32 				*/*out_internal_id*/)
{
	return B_ERROR;
}

BMediaAddOn *make_media_addon(image_id you)
{
	return new DecoderAddon(you);
}
