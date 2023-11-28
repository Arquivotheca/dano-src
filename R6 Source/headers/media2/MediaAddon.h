#ifndef _MEDIA2_MEDIAADDON_H_
#define _MEDIA2_MEDIAADDON_H_

#include <support2/SupportDefs.h>
#include <image.h>

namespace B {
namespace Media2 {

class BMediaConstraint;
class BMediaNode;

struct flavor_info {
    char *              name;
    char *              info;
    uint64              kinds;          /* node_kind */
    uint32              flavor_flags;
    int32               internal_id;    /* For BMediaAddon internal use */
    int32               possible_count; /* 0 for "any number" */

    BMediaConstraint	*in_formats;
    BMediaConstraint	*out_formats;

    uint32              _reserved_[16];

private:
    flavor_info & operator=(const flavor_info & other);
};

class BMediaAddon;

// Use the BMediaAddonManager class to iterate over
// all the available BMediaAddons.
class BMediaAddonManager
{
	public:
		explicit BMediaAddonManager();
				~BMediaAddonManager();

	    BMediaAddon *AddonAt(uint32 index);
};

// A BMediaAddon manufactures BMediaNodes.
// Perhaps we should name it differently.
class BMediaAddon
{
	public:
		explicit	BMediaAddon(image_id image);
			virtual	~BMediaAddon();

			virtual	status_t InitCheck(const char ** out_failure_text);
			virtual	int32 CountFlavors();
			virtual	status_t GetFlavorAt(
								int32 n,
								const flavor_info ** out_info);

			// instantiate a BMediaNode based on the given flavor_info
			// contrary to the "old" BMediaAddon, no "config" argument
			// is passed. Configuration should be part of the node itself,
			// not the BMediaAddon that created it.
			virtual	BMediaNode * InstantiateNodeFor(
							const flavor_info * info,
							status_t * out_error);

			/* not sure we need this */
/*
			virtual	status_t GetFileFormatList(
							int32 flavor_id,			//	for this node flavor (if it matters)
							media_file_format * out_writable_formats, 	//	don't write here if NULL
							int32 in_write_items,		//	this many slots in out_writable_formats
							int32 * out_write_items,	//	set this to actual # available, even if bigger than in count
							media_file_format * out_readable_formats, 	//	don't write here if NULL
							int32 in_read_items,		//	this many slots in out_readable_formats
							int32 * out_read_items,		//	set this to actual # available, even if bigger than in count
							void * _reserved);			//	ignore until further notice
*/
		private:
				/* Mmmh, stuffing! */
			virtual		status_t _Reserved_MediaAddon_1(void *);
			virtual		status_t _Reserved_MediaAddon_2(void *);
			virtual		status_t _Reserved_MediaAddon_3(void *);
			virtual		status_t _Reserved_MediaAddon_4(void *);
			virtual		status_t _Reserved_MediaAddon_5(void *);
			virtual		status_t _Reserved_MediaAddon_6(void *);

				BMediaAddon();	/* private unimplemented */
				BMediaAddon( const BMediaAddon & clone);
				BMediaAddon & operator=( const BMediaAddon & clone);

				image_id fAddonImage;
				uint32 _media_add_on_private[49];					// make some room until we know what the private members will be

};

}; }; // namespace B::Media2
#endif //_MEDIA2_MEDIAADDON_H_
