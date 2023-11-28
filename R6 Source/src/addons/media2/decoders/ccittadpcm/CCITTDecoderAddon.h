#include <media2/MediaAddon.h>

class CCCITTADPCMDecoderAddon : public B::Media2::BMediaAddon
{
	CCCITTADPCMDecoderAddon (const CCCITTADPCMDecoderAddon &);
	CCCITTADPCMDecoderAddon &operator= (const CCCITTADPCMDecoderAddon &);
	
	B::Media2::flavor_info mFlavor;
	
	public:
		CCCITTADPCMDecoderAddon (image_id image);
		virtual ~CCCITTADPCMDecoderAddon();
		
		virtual status_t InitCheck (const char **out_failure_text);

		virtual int32 CountFlavors();
		virtual status_t GetFlavorAt (int32 n, const B::Media2::flavor_info **out_info);

		virtual B::Media2::BMediaNode *InstantiateNodeFor
												(const B::Media2::flavor_info *info,
												status_t *out_error);		
};

extern "C" B::Media2::BMediaAddon *make_media_addon (image_id image);
