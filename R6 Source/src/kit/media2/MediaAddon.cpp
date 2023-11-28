
#include <MediaAddon.h>
#include <support2/Vector.h>
#include <support2/Locker.h>
#include <support2/Autolock.h>
#include <storage2/Directory.h>
#include <storage2/Entry.h>
#include <storage2/Path.h>
#include <stdio.h>

namespace B {
namespace Media2 { 

using namespace B::Storage2;
using namespace B::Support2;

class addoninfo
{
	public:
		entry_ref ref;
		image_id image;
		int32 refcount;
};


static BNestedLocker addonlistlocker("addonlistlocker");
static BVector<addoninfo> addonlist;
static bool listisinitialized=false;


static void scanpath(entry_ref *ref)
{
	BDirectory folder(ref);
	if(B_OK==folder.InitCheck())
	{
		entry_ref childref;
		while(B_OK==folder.GetNextRef(&childref))
		{
			BEntry child(&childref);
			if(child.IsFile())
			{
				// load file and see if it's an add-on
				BPath path;
				if(B_OK==child.GetPath(&path))
				{
					image_id img = load_add_on(path.Path());
					if(img>=0)
					{
						status_t (*func)(image_id);
						if(B_OK==get_image_symbol(img,"make_media_addon",B_SYMBOL_TYPE_TEXT, (void**)&func))
						{
							// it appears to be one of our add-ons, so store it in the list
							BAutolock lock(addonlistlocker.Lock());
							addonlist.AddItem();
							int32 index=addonlist.CountItems()-1;
							addonlist.EditItemAt(index).ref = childref;
							addonlist.EditItemAt(index).image = img;
							addonlist.EditItemAt(index).refcount = 0;
						}
					}
				}
			}
			else if(child.IsDirectory())
			{
				entry_ref childref;
				if(B_OK==child.GetRef(&childref))
					scanpath(&childref);
			}
		}
	}
}

BMediaAddonManager::BMediaAddonManager()
{
	BAutolock lock(addonlistlocker.Lock());
	if(!listisinitialized)
	{
		// possibly set up a connection to some global add-on manager daemon here,
		// or simply scan the folder(s) that contain the add-ons.
		listisinitialized=true;
		entry_ref addonpath;
		if(B_OK==get_ref_for_path("/boot/beos/system/add-ons/media2/",&addonpath))
			scanpath(&addonpath);
	}
}

BMediaAddonManager::~BMediaAddonManager()
{

}

BMediaAddon *BMediaAddonManager::AddonAt(uint32 index)
{
	BAutolock lock(addonlistlocker.Lock());
	if(addonlist.CountItems()>index)
	{
		if(addonlist[index].image<0)
		{
			// no image currently loaded for this add-on, so load it
			BPath path(&addonlist[index].ref);
			image_id img = load_add_on(path.Path());
			if(img>=0)
			{
				BMediaAddon* (*func)(image_id);
				if(B_OK==get_image_symbol(img,"make_media_addon",B_SYMBOL_TYPE_TEXT, (void**)&func))
				{
					// it appears to be one of our add-ons, so store it in the list
					addonlist.EditItemAt(index).image = img;
					addonlist.EditItemAt(index).refcount = 0;
				}
			}
		}
		// the image *should* be loaded now, but check again
		if(addonlist[index].image>=0)
		{
			BMediaAddon* (*func)(image_id);
			if(B_OK==get_image_symbol(addonlist[index].image,"make_media_addon",B_SYMBOL_TYPE_TEXT, (void**)&func))
			{
				BMediaAddon *addon = func(addonlist[index].image);
				if(addon)
				{
					addonlist.EditItemAt(index).refcount++;
					return addon;
				}
			}
		}
	}
	return NULL;
}


#if 0
#pragma mark ----------------------------------------
#endif

BMediaAddon::BMediaAddon(image_id image)
{
	fAddonImage = image;
}

BMediaAddon::~BMediaAddon()
{
	// find the image in the list and decrease its refcount
	BAutolock lock(addonlistlocker.Lock());
	for(uint32 i=0;i<addonlist.CountItems();i++)
	{
		if(addonlist[i].image == fAddonImage)
		{
			if(addonlist.EditItemAt(i).refcount-- < 0)
				debugger("ERROR: media-addon refcount <0");
			break;
		}
	}
}

status_t BMediaAddon::InitCheck(const char **out_failure_text)
{
	(void)out_failure_text;
	return B_OK;
}

int32 BMediaAddon::CountFlavors()
{
	return 0;
}

status_t BMediaAddon::GetFlavorAt(int32 n, const flavor_info **info)
{
	(void)n;
	(void)info;
	return B_ERROR;
}

BMediaNode *BMediaAddon::InstantiateNodeFor(const flavor_info *info, status_t *out_error)
{
	(void)info;
	(void)out_error;
	return NULL;
}

#if 0
#pragma mark ---- vtable padding ------
#endif

status_t BMediaAddon::_Reserved_MediaAddon_1(void *) {}
status_t BMediaAddon::_Reserved_MediaAddon_2(void *) {}
status_t BMediaAddon::_Reserved_MediaAddon_3(void *) {}
status_t BMediaAddon::_Reserved_MediaAddon_4(void *) {}
status_t BMediaAddon::_Reserved_MediaAddon_5(void *) {}
status_t BMediaAddon::_Reserved_MediaAddon_6(void *) {}
 


}; // namespace Media2
}; // namespace B
