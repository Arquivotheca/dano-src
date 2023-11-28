/* ExtractorAddon.cpp by Simon Clarke */

#include "ExtractorAddon.h"
#include "ExtractorNode.h"
#include "addons.h"

#include <string.h>
#include <stdio.h>
#include <Mime.h>
#include <GraphicsDefs.h>

const char *gReaderName = "Extractor Node";
const char *gReaderInfo = "Extractor Node";

#define CALL(x)			 printf x
#define ERROR(x)		 printf x

ExtractorAddon::ExtractorAddon(image_id image) :
	BMediaAddOn(image)
{
	// fill out our flavor_info for later use
	// XXX we *should* iterate over the low-level extractors to see what they support,
	// but that API isn't here yet....
	static char NODE_NAME[64];
	if (!NODE_NAME[0]) {
		sprintf(NODE_NAME, "Be media reader v%X.%X.%X", (B_BEOS_VERSION>>8)&0xf,
			(B_BEOS_VERSION>>4)&0xf, (B_BEOS_VERSION)&0xf);
	}
	static char* NODE_INFO = "Extractor Node, copyright 1999 Be, Incorporated";

	m_flavorInfo.internal_id = 0;
	m_flavorInfo.name = NODE_NAME;
	m_flavorInfo.info = NODE_INFO;

	m_flavorInfo.kinds = B_BUFFER_PRODUCER | B_FILE_INTERFACE;
	m_flavorInfo.flavor_flags = 0;
	m_flavorInfo.possible_count = 255;

	/* reader output settings */
	media_format* mediaFormat = new media_format[2];
	memset(mediaFormat, 0, sizeof(media_format) * 2);

	mediaFormat[0].type = B_MEDIA_RAW_AUDIO;
	mediaFormat[0].u.raw_audio = media_raw_audio_format::wildcard;

	mediaFormat[1].type = B_MEDIA_RAW_VIDEO;
	mediaFormat[1].u.raw_video = media_raw_video_format::wildcard;
	mediaFormat[1].u.raw_video.display = media_video_display_info::wildcard;

	/* describe the media_format data */
	m_flavorInfo.out_format_count = 2;
	m_flavorInfo.out_format_flags = 0;
	m_flavorInfo.out_formats = mediaFormat;

	m_flavorInfo.in_format_count = 0;
	m_flavorInfo.in_format_flags = 0;
	m_flavorInfo.in_formats = 0;									
}

ExtractorAddon::~ExtractorAddon()
{
	// dispose of the dynamically-allocated portions of our flavor_info struct
	delete [] m_flavorInfo.out_formats;
}

int32 ExtractorAddon::DepthForSpace(color_space space)
{
	size_t		alignment, pixelsPerChunk;
	size_t		pixelChunk = 0;

	get_pixel_size_for(space, &pixelChunk, &alignment, &pixelsPerChunk);

	return pixelChunk * 8;
}

status_t ExtractorAddon::InitCheck(const char **out_failure_text)
{
	CALL(("ExtractorAddon::InitCheck()\n"));

	*out_failure_text = "QuickTime Error";

	return B_OK;
}

int32 ExtractorAddon::CountFlavors()
{
	CALL(("ExtractorAddon::CountFlavors()\n"));
	return 1;
}

bool ExtractorAddon::WantsAutoStart()
{
	/* auto starting of node */
	return false;
}

status_t ExtractorAddon::AutoStart(
						int 					in_count,
						BMediaNode 				**out_node,
						int32 					*out_internal_id,
						bool 					*out_has_more)
{
	/* this should not get called */
	return B_OK;
}

status_t ExtractorAddon::GetFlavorAt(
						int32 					n,
						const flavor_info 		**out_info)
{
	CALL(("ExtractorAddon::GetFlavorAt(n:%ld)\n",n));

	if ((n < CountFlavors()-1) && (n < 0)) {
		ERROR(("ExtractorAddon::GetFlavorAt() - flavor out of range.\n"));
		return B_ERROR;
	}

	// return a pointer to our flavor descriptor member
	*out_info = &m_flavorInfo;
	return B_OK;
}

BMediaNode *ExtractorAddon::InstantiateNodeFor(
						const flavor_info 	*info,
						BMessage 			*config,
						status_t 			*out_error)
{
	CALL(("ExtractorAddon::InstantiateNodeFor(id:%ld)\n",info->internal_id));

	BMediaNode *returnNode = 0;
	*out_error = B_ERROR;

	if (!info) {
		ERROR(("ExtractorAddon::InstantiateNodeFor() - no info\n"));
		return 0;
	}

	returnNode = new ExtractorNode(this,"some node");
	*out_error = B_OK;

	CALL(("ExtractorAddon::InstantiateNodeFor(id:%ld) - done\n",info->internal_id));
	
	return returnNode;
}

status_t ExtractorAddon::GetConfigurationFor(
						BMediaNode 			*your_node,
						BMessage 			*into_message)
{
	CALL(("ExtractorAddon::GetConfigurationFor()\n"));

	return B_ERROR;
}

status_t ExtractorAddon::SniffRef(
							const entry_ref 	&ref,
							BMimeType 			*io_mime_type,
							float 				*out_quality,
							int32 				*out_internal_id)
{
	CALL(("ExtractorAddon::SniffRef()\n"));
	int32      trackcount, extractor_id;
	status_t   err;
	media_file_format mfi;

	//
	// XXXdbg -- what should happen here is that this info is saved
	// off and then passed to the ExtractorNode() in InstantiateNodeFor()
	// so that it doesn't have to go and sniff the damn file twice.
	//

	BFile file(&ref, B_READ_ONLY);
	if ((err = file.InitCheck()) != B_OK)
		return err;

	MediaExtractor	*extractor = new MediaExtractor(0);
	err = extractor->SetSource(&file, &trackcount);
	if(err != B_NO_ERROR) {
		delete extractor;
		return err;
	}

	extractor->GetFileFormatInfo(&mfi);

	delete extractor;
	
	*out_internal_id = 0;
	io_mime_type->SetType(mfi.mime_type);
	*out_quality = 1.0;

	return B_OK;
}

status_t ExtractorAddon::SniffType(
							const BMimeType 	&type,
							float 				*out_quality,
							int32 				*out_internal_id)
{
	CALL(("ExtractorAddon::SniffType()\n"));
	return B_OK;
}

BMediaAddOn *make_media_addon(image_id you)
{
	return new ExtractorAddon(you);
}
