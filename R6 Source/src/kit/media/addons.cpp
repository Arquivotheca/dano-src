
#include <stdio.h>
#include <sys/stat.h>
#include <alloca.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <Entry.h>
#include <Locker.h>
#include <Autolock.h>

#include <MediaFormats.h>
#include <MediaFile.h>
#include <MediaTrack.h>

#include "Extractor.h"
#include "Decoder.h"
#include "addons.h"

#define DIAGNOSTIC(x) fprintf x
#if !NDEBUG
#define FPRINTF(x) fprintf x
#else
#define FPRINTF(x) (void)0
#endif

static bool s_debug = (getenv("ADDON_DEBUG") != 0);

using namespace BPrivate;


_AddonManager::_AddonManager(char **paths,
							 void (*load_hook)(void *arg, image_id id),
							 void (*unload_hook)(void *arg, image_id id),
							 void *hook_arg)
	: locker("AddonManagerLock", true)
{
	int i;

	for (i=0; paths[i]; i++)
		;
	fNumPaths = i;
	
	fPaths              = paths;
	fPathModTimes       = (time_t *)calloc(fNumPaths, sizeof(time_t));

	fLoadHook			= load_hook;
	fUnloadHook			= unload_hook;
	fHookArg            = hook_arg;
	fAddons				= NULL;
	fMaxAddons          = 0;
	fMaxAllocatedAddons = 0;
}


_AddonManager::~_AddonManager()
{
	int i;
	for(i=0; i < fMaxAddons; i++) {
		if (fAddons[i].path == NULL)
			continue;

		if (fAddons[i].refcount > 0) {
			DIAGNOSTIC((stderr, "AddonManager destructor: refcount is %d for %s\n",
					fAddons[i].refcount, fAddons[i].path));
		}

		free(fAddons[i].path);
		fAddons[i].path = NULL;

		if (fAddons[i].imgid > 0) {
			if (fUnloadHook)
				fUnloadHook(fHookArg, fAddons[i].imgid);

			unload_add_on(fAddons[i].imgid);
		}
		fAddons[i].imgid = 0;
	}

	if (fPathModTimes)
		free(fPathModTimes);
	fPathModTimes = NULL;
}


void
_AddonManager::ScanDirs()
{
	int  i, j, break_out = 0, dup;
	char buff[PATH_MAX];
	DIR  *dir;
	struct dirent *dirent;
	struct stat    st;
	
	for(i=0; fPaths[i] != NULL; i++) {
		if (stat(fPaths[i], &st) < 0)
			continue;

		if (S_ISDIR(st.st_mode) == 0)
			continue;

		if (st.st_mtime <= fPathModTimes[i]) { // it's not newer, nothing to do
			continue;
		}

		fPathModTimes[i] = st.st_mtime;

		dir = opendir(fPaths[i]);
		if (dir == NULL)
			continue;
		
		dirent = NULL;
		while ((dirent = readdir(dir)) != NULL) {
			if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0)
				continue;
			
			if (fMaxAddons >= fMaxAllocatedAddons) {
				addon_info *ai;

				fMaxAllocatedAddons += 32;
				ai = (addon_info *)realloc(fAddons, fMaxAllocatedAddons*sizeof(addon_info));
				if (ai == NULL) {
					break_out = 1;
					break;
				}

				/* clear out the new guys to zero */
				memset(&ai[fMaxAddons], 0,
					   sizeof(addon_info)*fMaxAllocatedAddons -
					   sizeof(addon_info)*fMaxAddons);
				fAddons = ai;
			}

			/* now check if this add-on is a duplicate */
			sprintf(buff, "%s/%s", fPaths[i], dirent->d_name);

			for(j=0, dup=0; j < fMaxAddons; j++) {
				if (fAddons[j].path && strcmp(buff, fAddons[j].path) == 0) {
					dup = 1;
					break;
				}
			}

			if (dup == 0) {
				fAddons[fMaxAddons].path     = strdup(buff);
				fAddons[fMaxAddons].imgid    = 0;
				fAddons[fMaxAddons].refcount = 0;
				fMaxAddons++;
			}
		}
		
		if (break_out)
			break;
	}
}


image_id
_AddonManager::GetNextAddon(int32 *cookie, int32 *id_for_user, bool allowLoad, addon_list * addons)
{
	int32    id;
	image_id imgid = 0;
	int32	addcnt = addons ? addons->size() : 0;

	BAutolock autolock(&locker);
	
	id = *cookie;
	if (id == 0)
		ScanDirs();

	do {
		if (id < 0 || id >= fMaxAddons || fAddons[id].path == NULL)
			return B_BAD_INDEX;

		//	if there was a list of add-ons supplied, match against it
		if (addcnt > 0) {
			for (int ix=0; ix<addons->size(); ix++) {
				if (!strcmp((*addons)[ix], fAddons[id].path)) {
//					FPRINTF((stderr, "addon list contains item %s\n", (*ptr).c_str()));
					goto gotit;
				}
			}
			FPRINTF((stderr, "addon list with %ld items excludes %s\n", addcnt, fAddons[id].path));
			goto donext;
		}
gotit:
		if ((fAddons[id].imgid <= 0) && allowLoad) {
			fAddons[id].imgid = load_add_on(fAddons[id].path);

if (s_debug) fprintf(stderr, "LOADING: %s\n", fAddons[id].path);
			if (fLoadHook && fAddons[id].imgid > 0) {
				fLoadHook(fHookArg, fAddons[id].imgid);
			}
		}
	
if (s_debug) fprintf(stderr, "%ld: %s: %sloaded (image %ld)\n", id, fAddons[id].path, (fAddons[id].imgid <= 0) ? "not " : "", fAddons[id].imgid);
		imgid        = fAddons[id].imgid;
donext:
		*id_for_user = id;
		*cookie      = ++id;

	} while(imgid <= 0 && id < fMaxAddons);
	
if (s_debug) fprintf(stderr, "got addon image %ld\n", imgid);
	if (imgid <= 0) {  // hmmm, didn't find any 
		return B_BAD_INDEX;
	}

	fAddons[*id_for_user].refcount++;

	return imgid;
}

image_id
_AddonManager::GetAddonAt(int32 id)
{
	BAutolock autolock(&locker);

	if (id < 0 || id >= fMaxAddons || fAddons[id].path == NULL)
		return B_BAD_INDEX;
	

	if (fAddons[id].imgid <= 0) {
		fAddons[id].imgid = load_add_on(fAddons[id].path);

		if (fLoadHook && fAddons[id].imgid > 0) {
			fLoadHook(fHookArg, fAddons[id].imgid);
		}
	}

	fAddons[id].refcount++;
	
	return fAddons[id].imgid;
}

status_t
_AddonManager::ReleaseAddon(int32 id)
{
	BAutolock autolock(&locker);

	if (id < 0 || id >= fMaxAddons)
		return B_BAD_INDEX;
	
	if (fAddons[id].refcount == 0 || fAddons[id].imgid == 0) {
		DIAGNOSTIC((stderr, "WARNING: addon %s (%ld) released too many times!\n",
				fAddons[id].path, id));
		return B_BAD_INDEX;
	}

	fAddons[id].refcount--;
	if (fAddons[id].refcount == 0) {

		if (fUnloadHook)
			fUnloadHook(fHookArg, fAddons[id].imgid);
		
		unload_add_on(fAddons[id].imgid);
		fAddons[id].imgid = 0;
	}

	return B_OK;
}



//---------------------------------------------------------------



static char *extractor_dirs[] =
{
	"/boot/home/config/add-ons/media/extractors",
	"/boot/config/add-ons/media/extractors",
	"/boot/beos/system/add-ons/media/extractors",
	NULL
};

namespace BPrivate {
	void BPrivate::extractor_load_hook(void *arg, image_id imgid);
}

void
BPrivate::extractor_load_hook(void */*arg*/, image_id imgid)
{
	status_t err;
	void (*register_extractor)(const media_format ** out_formats, int32 * out_count);

	err = get_image_symbol(imgid, "register_extractor", B_SYMBOL_TYPE_TEXT,
	                       (void **)&register_extractor);

	if (err != B_OK)
		return;

	const media_format * list = 0;
	int32 count = 0;

	register_extractor(&list, &count);
	if ((list != 0) && (count > 0)) {
		image_info info;
		if (!get_image_info(imgid, &info)) {
			FPRINTF((stderr, "Binding add-on %s\n", info.name));
			BMediaFormats::bind_addon(info.name, list, count);
		}
	}
}

_AddonManager *
__get_extractor_manager(void)
{
	static long didit = 0;
	static _AddonManager *extractor_mgr = NULL;
	
	if (atomic_add(&didit, 1) == 0) {
		extractor_mgr = new _AddonManager(extractor_dirs, extractor_load_hook,
		                                  NULL, NULL);
	} else {
		while(extractor_mgr == NULL)
			snooze(3000);
	}
	
	return extractor_mgr;
}		





static char *writer_dirs[] =
{
	"/boot/home/config/add-ons/media/writers",
	"/boot/config/add-ons/media/writers",
	"/boot/beos/system/add-ons/media/writers",
	NULL
};


_AddonManager *
__get_writer_manager(void)
{
	static long didit = 0;
	static _AddonManager *writer_mgr = NULL;
	
	if (atomic_add(&didit, 1) == 0) {
		writer_mgr = new _AddonManager(writer_dirs);
	} else {
		while(writer_mgr == NULL)
			snooze(3000);
	}
	
	return writer_mgr;
}		

namespace BPrivate {
	void dec_load_hook(void *arg, image_id imgid);
}

void
BPrivate::dec_load_hook(void */*arg*/, image_id imgid)
{
	void (*register_dec)(media_format ** out_list, int32 * out_count);

	if (get_image_symbol(imgid, "register_decoder",
						 B_SYMBOL_TYPE_TEXT, (void **)&register_dec) != B_OK)
		return;

	media_format * list = 0;
	int32 count = 0;

	register_dec(&list, &count);
	if ((list != 0) && (count > 0)) {
		image_info info;
		if (!get_image_info(imgid, &info)) {
			FPRINTF((stderr, "Binding add-on %s\n", info.name));
			BMediaFormats::bind_addon(info.name, list, count);
		}
	}
}


static char *decoder_dirs[] =
{
	"/boot/home/config/add-ons/media/decoders",
	"/boot/config/add-ons/media/decoders",
	"/boot/beos/system/add-ons/media/decoders",
	NULL
};


_AddonManager *
__get_decoder_manager(void)
{
	static long didit = 0;
	static _AddonManager *decoder_mgr = NULL;
	
	if (atomic_add(&didit, 1) == 0) {
		decoder_mgr = new _AddonManager(decoder_dirs, BPrivate::dec_load_hook,NULL,NULL);
	} else {
		while(decoder_mgr == NULL)
			snooze(3000);
	}
	
	return decoder_mgr;
}		



static void
enc_load_hook(void */*arg*/, image_id imgid)
{
	void (*register_enc)(void);

	if (get_image_symbol(imgid, "register_encoder",
						 B_SYMBOL_TYPE_TEXT, (void **)&register_enc) != B_OK)
		return;

	register_enc();
}


static char *encoder_dirs[] =
{
	"/boot/home/config/add-ons/media/encoders",
	"/boot/config/add-ons/media/encoders",
	"/boot/beos/system/add-ons/media/encoders",
	NULL
};


_AddonManager *
__get_encoder_manager(void)
{
	static long didit = 0;
	static _AddonManager *encoder_mgr = NULL;
	
	if (atomic_add(&didit, 1) == 0) {
		encoder_mgr = new _AddonManager(encoder_dirs, enc_load_hook,NULL,NULL);
	} else {
		while(encoder_mgr == NULL)
			snooze(3000);
	}
	
	return encoder_mgr;
}		
