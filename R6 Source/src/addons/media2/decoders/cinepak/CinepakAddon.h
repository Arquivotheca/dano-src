#ifndef C_INDEO_5_ADDON_H

#define C_INDEO_5_ADDON_H

#include <media2/MediaAddon.h>

class CCinepakAddon : public B::Media2::BMediaAddon
{
	CCinepakAddon (const CCinepakAddon &);
	CCinepakAddon &operator= (const CCinepakAddon &);
	
	B::Media2::flavor_info mFlavor;
	
	public:
		CCinepakAddon (image_id image);
		virtual ~CCinepakAddon();
		
		virtual status_t InitCheck (const char **out_failure_text);

		virtual int32 CountFlavors();
		virtual status_t GetFlavorAt (int32 n, const B::Media2::flavor_info **out_info);

		virtual B::Media2::BMediaNode *InstantiateNodeFor
												(const B::Media2::flavor_info *info,
												status_t *out_error);		
};

extern "C" B::Media2::BMediaAddon *make_media_addon (image_id image);

#endif
