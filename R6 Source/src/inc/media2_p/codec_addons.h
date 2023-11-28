#ifndef _MEDIA2_CODEC_ADDONS_PRIVATE_
#define _MEDIA2_CODEC_ADDONS_PRIVATE_

#include <media2/MediaDefs.h>
#include <support2/SupportDefs.h>
#include <OS.h>
#include <image.h>
#include <string.h>
#include <stdlib.h>

#include <support2/Locker.h>
#include <support2/String.h>
#include <support2/Vector.h>

namespace B {
namespace Media2 {
namespace Media2Private {

class addon_list {
public:
	addon_list() {
	}
	~addon_list() {
		destroy();
	}
	addon_list(const addon_list & other) {
		copy(other);
	}
	addon_list & operator=(const addon_list & other) {
		destroy();
		copy(other);
		return *this;
	}
	const char * string_at(size_t ix) {
		return (*this)[ix];
	}
	const char * operator[](size_t ix) {
		if (ix >= items.CountItems()) return 0;
		return (const char *)items.ItemAt(ix);
	}
	int size() {
		return items.CountItems();
	}
	int push_back(const char * str) {
		items.AddItem(strdup(str));
		return items.CountItems()-1;
	}
private:
	void copy(const addon_list & other) {
		for (size_t ix=0; ix<other.items.CountItems(); ix++) {
			items.AddItem(strdup((const char *)other.items.ItemAt(ix)));
		}
	}
	void destroy() {
		for (size_t ix=0; ix<items.CountItems(); ix++)
			free(items.ItemAt(ix));
	}
	BVector<char *> items;
};


class _AddonManager
{
public:
			 _AddonManager(char **paths,
						   void (*load_hook)(void *arg, image_id id) = NULL,
						   void (*unload_hook)(void *arg, image_id id) = NULL,
						   void *hook_arg = NULL);
			 _AddonManager(char **paths,
						   void (*init_hook)(const char *path, image_id id),
						   void (*load_hook)(void *arg, image_id id) = NULL,
						   void (*unload_hook)(void *arg, image_id id) = NULL,
						   void *hook_arg = NULL);
			 ~_AddonManager();

	void     InitAddons();
	image_id GetNextAddon(int32 *cookie, int32 *id, bool allowLoad = true, addon_list * filter = 0);
	status_t ReleaseAddon(int32 cookie);   // call after GetNextAddon and GetAddonAt
	image_id GetAddonAt(int32 id);

private:
	void	 Construct(char **paths,
						   void (*init_hook)(const char *path, image_id id),
						   void (*load_hook)(void *arg, image_id id),
						   void (*unload_hook)(void *arg, image_id id),
						   void *hook_arg);

	struct addon_info {
		image_id  imgid;
		char     *path;
		int       refcount;
		int       reserved;
	};

	addon_info    *fAddons;
	int            fMaxAddons;
	int			   fMaxAllocatedAddons;
	void		 (*fInitHook)(const char *path, image_id id);
	void		 (*fLoadHook)(void *arg, image_id id);
	void		 (*fUnloadHook)(void *arg, image_id id);
	void          *fHookArg;
	char         **fPaths;
	int			   fNumPaths;
	time_t		  *fPathModTimes;
	BLocker        locker;

	void		   ScanDirs(void);
};

_AddonManager *__get_extractor_manager(void);
_AddonManager *__get_writer_manager(void);
_AddonManager *__get_decoder_manager(void);
_AddonManager *__get_encoder_manager(void);

} } } // namespace B::Media2::Media2Private
#endif //_MEDIA2_CODEC_ADDONS_PRIVATE_
