#include <media2/MediaAddon.h>

class CAC3AudioAddon : public B::Media2::BMediaAddon
{
	CAC3AudioAddon (const CAC3AudioAddon &);
	CAC3AudioAddon &operator= (const CAC3AudioAddon &);
	
	B::Media2::flavor_info mFlavor;
	
	public:
		CAC3AudioAddon (image_id image);
		virtual ~CAC3AudioAddon();
		
		virtual status_t InitCheck (const char **out_failure_text);

		virtual int32 CountFlavors();
		virtual status_t GetFlavorAt (int32 n, const B::Media2::flavor_info **out_info);

		virtual B::Media2::BMediaNode *InstantiateNodeFor
												(const B::Media2::flavor_info *info,
												status_t *out_error);		
};

extern "C" B::Media2::BMediaAddon *make_media_addon (image_id image);
