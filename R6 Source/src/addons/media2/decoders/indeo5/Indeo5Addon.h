#ifndef C_INDEO_5_ADDON_H

#define C_INDEO_5_ADDON_H

#include <media2/MediaAddon.h>

class CIndeo5DecoderAddon : public B::Media2::BMediaAddon
{
	CIndeo5DecoderAddon (const CIndeo5DecoderAddon &);
	CIndeo5DecoderAddon &operator= (const CIndeo5DecoderAddon &);
	
	B::Media2::flavor_info mFlavor;
	
	public:
		CIndeo5DecoderAddon (image_id image);
		virtual ~CIndeo5DecoderAddon();
		
		virtual status_t InitCheck (const char **out_failure_text);

		virtual int32 CountFlavors();
		virtual status_t GetFlavorAt (int32 n, const B::Media2::flavor_info **out_info);

		virtual B::Media2::BMediaNode *InstantiateNodeFor
												(const B::Media2::flavor_info *info,
												status_t *out_error);		
};

extern "C" B::Media2::BMediaAddon *make_media_addon (image_id image);

#endif
