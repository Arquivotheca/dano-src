#include <MediaAddOn.h>
#include <image.h>
#include <Locker.h>

// export this for the input_server
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id you);

class dc10MediaAddOn : public BMediaAddOn
{
public:
						dc10MediaAddOn(image_id myImage);
	virtual 			~dc10MediaAddOn();
	
	virtual	status_t	InitCheck(const char ** out_failure_text);

	virtual	int32		CountFlavors();
	virtual	status_t	GetFlavorAt(int32 n, const flavor_info ** out_info);
	virtual	BMediaNode	*InstantiateNodeFor(
							const flavor_info * info,
							BMessage * config,
							status_t * out_error);

	virtual	status_t	GetConfigurationFor(BMediaNode * your_node, BMessage * into_message);
	virtual	status_t	SaveConfigInfo(BMediaNode * your_node, BMessage * into_message);

	virtual	bool		WantsAutoStart();
	virtual	status_t	AutoStart(int in_count, BMediaNode ** out_node, int32 * out_internal_id, bool * out_has_more);

protected:
private:

		enum {
			MAX_CARDS = 4
		};
		
		BLocker				m_lock;
		int32				m_card_count;
		flavor_info			*m_flavor_info[2*MAX_CARDS];
};
