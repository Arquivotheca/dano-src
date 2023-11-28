/* DecoderAddon.h by Simon Clarke */

#ifndef __DecoderAddon_H
#define __DecoderAddon_H

#include <MediaAddOn.h>

extern "C" _EXPORT BMediaAddOn *make_media_addon(image_id you);

class DecoderAddon : public BMediaAddOn {
public:

		DecoderAddon(image_id image);
		
		/* MediaAddOn functions */
		virtual status_t InitCheck(	const char **out_failure_text);
		virtual int32 CountFlavors();
		virtual	bool WantsAutoStart();
		
		virtual status_t AutoStart(	int 					in_count,
									BMediaNode 				**out_node,
									int32 					*out_internal_id,
									bool 					*out_has_more);
		
		virtual status_t GetFlavorAt(
									int32 				n,
									const flavor_info	**out_info);
		
		virtual BMediaNode * InstantiateNodeFor(
									const flavor_info 	*info,
									BMessage 			*config,
									status_t 			*out_error);
		
		virtual status_t GetConfigurationFor(
									BMediaNode 			*your_node,
									BMessage 			*into_message);

		virtual	status_t SniffRef(	const entry_ref 	&file,
									BMimeType 			*io_mime_type,
									float 				*out_quality,
									int32 				*out_internal_id);
		
		virtual	status_t SniffType(	const BMimeType 	&type,
									float 				*out_quality,
									int32 				*out_internal_id);

};

#endif
