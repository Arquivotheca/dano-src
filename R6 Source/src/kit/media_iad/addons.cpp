
#include <stdio.h>
#include <sys/stat.h>
#include <alloca.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <utime.h>

#include <Entry.h>
#include <Locker.h>
#include <Autolock.h>
#include <fs_attr.h>

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
	Construct(paths, NULL, load_hook, unload_hook, hook_arg);
}


_AddonManager::_AddonManager(char **paths,
							 void (*init_hook)(const char *path, image_id id),
							 void (*load_hook)(void *arg, image_id id),
							 void (*unload_hook)(void *arg, image_id id),
							 void *hook_arg)
	: locker("AddonManagerLock", true)
{
	Construct(paths, init_hook, load_hook, unload_hook, hook_arg);
}


void
_AddonManager::Construct(char **paths,
							 void (*init_hook)(const char *path, image_id id),
							 void (*load_hook)(void *arg, image_id id),
							 void (*unload_hook)(void *arg, image_id id),
							 void *hook_arg)
{
	int i;

	for (i=0; paths[i]; i++)
		;
	fNumPaths = i;
	
	fPaths              = paths;
	fPathModTimes       = (time_t *)calloc(fNumPaths, sizeof(time_t));

	fInitHook			= init_hook;
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
	char buff[PATH_MAX], *ptr;
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


void
_AddonManager::InitAddons()
{
	size_t nr;
	int i;

	if(!fInitHook) {
		return;
	}

	BAutolock autolock(&locker);
	
	ScanDirs();

	nr = 0;
	for(i = 0; i < fMaxAddons; i++) {
		if(fAddons[i].path == NULL)
			continue;

		fInitHook(fAddons[i].path, fAddons[i].imgid);
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
					goto gotit;
				}
			}
			FPRINTF((stderr, "addon list with %d items excludes %s\n", addcnt, fAddons[id].path));
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
if (s_debug) fprintf(stderr, "%ld: %s: %sloaded (image %d)\n", id, fAddons[id].path, (fAddons[id].imgid <= 0) ? "not " : "", fAddons[id].imgid);
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
		DIAGNOSTIC((stderr, "WARNING: addon %s (%d) released too many times!\n",
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

namespace BPrivate {
	void codec_init_hook(const char *arg, image_id imgid);
	void register_codec_addon(const char *path, const BMessage &msg);
}

#define CODEC_CACHE_ATTR	"codec:cache"
enum {
	C_OK = B_OK,
	C_NOMEM = B_NO_MEMORY,
	C_BADVAL = B_BAD_VALUE,
	C_ABORT = B_ERRORS_END + 1,
	C_REFRESH,
	C_BAD_ADDON
};

static int
codec_cache_lookup(const char *path, const BNode &node, struct stat &st, BMessage &msg)
{
	attr_info ai;
	char *buf, *nbuf;
	time_t cache_mtime;
	time_t node_mtime;
	status_t err;
	int ret;

	buf = NULL;

retry:
	// find the last time this addon was modified
	//
	err = stat(path, &st);
	if(err < B_OK) {
		DIAGNOSTIC((stderr, "'%s': stat failed (%s)\n", path, strerror(err)));
		ret = C_ABORT;
		goto exit;
	}
	if(S_ISREG(st.st_mode) == 0 && S_ISLNK(st.st_mode) == 0) {
		DIAGNOSTIC((stderr, "codec_cache_lookup: '%s' is weird file type 0x%x\n", path, st.st_mode & S_IFMT));
		ret = C_ABORT;
		goto exit;
	}

	node_mtime = st.st_mtime;
	//
	// it's possible that our stat happened between another thread's
	// WriteAttr and utime, in which case we're about to decide that
	// the otherwise valid cache is out of date.  What can you do.

	//
	// check out the attribute
	// it should consist of a time_t optionally followed by a flattened BMessage
	//

	err = node.GetAttrInfo(CODEC_CACHE_ATTR, &ai);
	if(err != B_OK) {
		// couldn't find the attr
		//
		goto refresh;
	}
	if(ai.type != B_ANY_TYPE || ai.size < sizeof(time_t)) {
		// that's not right... keep going
		// to see if the situation rights itself
		//
		goto refresh;
	}

#define FUDGE	4

	// the fudge factor is to see if the attr has grown since we sniffed it.
	//
	nbuf = (char*)realloc(buf, ai.size + FUDGE);
	if(nbuf == NULL) {
		ret = C_NOMEM;
		goto exit;
	}
	buf = nbuf;

	// read in the contents of the attribute
	//
	ret = node.ReadAttr(CODEC_CACHE_ATTR, ai.type, (off_t)0, buf, ai.size + FUDGE);
	if(ret < B_OK) {
		DIAGNOSTIC((stderr, "codec_cache_lookup: can't read attr %s from '%s': %s\n", CODEC_CACHE_ATTR, path, strerror(ret)));
		goto refresh;
	}
	if(ret != ai.size) {
		// looks like someone changed the attribute.
		// start over, since we'll need the new modification time.
		//
		// it's really only necessary to jump back to the GetAttrInfo,
		// but refreshing the stat shouldn't hurt.
		//
		goto retry;
	}

	// check the timestamp
	//
	cache_mtime = *(time_t*)buf;
	if(cache_mtime < node_mtime) {
printf("old cache %d < node %d for %s\n", cache_mtime, node_mtime, path);
		// the addon has been updated since
		// we last cached its info
		//
		goto refresh;
	}

	// so the cache is valid..
	// is there anything actually cached?
	//
	if(ai.size == sizeof(time_t)) {
		// nope, the addon doesn't support any formats
		// or doesn't have the right hook functions.
		// read: bad addon, don't bother loading it
		//
		ret = C_BAD_ADDON;
		goto exit;
	}

	// rehydrate the message
	//
	err = msg.Unflatten(buf + sizeof(time_t));
	if(err != B_OK) {
		// oh, for chrissake.
		//
		goto refresh;
	}

	// ok, we have a valid message.
	//
	ret = C_OK;
	goto exit;

refresh:
	ret = C_REFRESH;
exit:
	free(buf);
	return ret;
}


static int
codec_cache_refresh(const char *path, BNode &node, const struct stat &st, image_id imgid, BMessage &msg)
{
	status_t (*get_next_description)(int32 *cookie, media_type *otype, const media_format_description **odesc, int32 *ocount);
	char *buf;
	utimbuf utb;
	media_type type;
	const media_format_description *desc;
	ssize_t nbytes;
	status_t err;
	int32 cookie;
	int32 desc_count;
	int num_added;
	int do_unload;
	int ret;

	if(path == NULL) {
		return C_BADVAL;
	}

	if(imgid <= 0) {
		imgid = load_add_on(path);
		do_unload = true;
	}
	else {
		do_unload = false;
	}
	if(imgid <= 0) {
		return C_BAD_ADDON;
	}

	//////////////////////////////////////////////////////////
	// Oi!  The following cacheing code needs to match up with
	// similar code in src/tools/codeccache/codeccache.cpp.
	// Don't screw it up;  hopefully they'll one day use
	// a common codebase.
	//////////////////////////////////////////////////////////

	if (get_image_symbol(imgid, "get_next_description",
						 B_SYMBOL_TYPE_TEXT, (void **)&get_next_description) != B_OK)
	{
		DIAGNOSTIC((stderr, "get_next_description() not found for addon %s\n", path));
		if(do_unload) {
			unload_add_on(imgid);
		}
		return C_BAD_ADDON;
	}

	msg.MakeEmpty();

	cookie = 0;
	num_added = 0;
	while(1) {
		err = get_next_description(&cookie, &type, &desc, &desc_count);
		if(err != B_OK)
			break;
		
		err = msg.AddInt32("codec:ndesc", desc_count);
		if(err != B_OK) {
			break;
		}

		err = msg.AddData("codec:desc", B_RAW_TYPE, (const void*)desc, desc_count * sizeof(media_format_description), false);
		if(err != B_OK) {
			printf("AddData failed: %s\n", strerror(err));
			break;
		}
		
		err = msg.AddInt32("codec:type", (int32)type);
		if(err != B_OK) {
			break;
		}

		num_added++;
	}

	if(do_unload) {
		unload_add_on(imgid);
		imgid = (image_id)B_BAD_IMAGE_ID;
	}

	if(num_added <= 0) {
		msg.MakeEmpty();
		return C_BAD_ADDON;
	}

	nbytes = msg.FlattenedSize();
	buf = (char*)malloc(sizeof(time_t) + nbytes);
	if(buf == NULL) {
		return C_NOMEM;
	}

	err = msg.Flatten(buf + sizeof(time_t), nbytes);
	if(err != B_OK) {
		free(buf);
		return C_BAD_ADDON;
	}
	nbytes += sizeof(time_t);

	*(time_t*)buf = st.st_mtime;

	ret = node.WriteAttr(CODEC_CACHE_ATTR, B_ANY_TYPE, (off_t)0, buf, nbytes);
	if(ret != nbytes) {
		DIAGNOSTIC((stderr, "codec_cache_refresh: error %s writing attribute %s (len %lu) to '%s'\n",
				strerror(ret), CODEC_CACHE_ATTR, nbytes, path));
		//
		// well, the cache won't be updated but we can keep going.
	}

	//
	// this is the crux of the race.  the modtime of the node changed
	// when we wrote the attribute, which means the time_t in the attribute
	// is older than the node's mtime.
	//

	// reset the modification time on the node.
	// time_t only has second resolution, so most of the time
	// this won't even change anything.
	//
	utb.actime = st.st_atime;
	utb.modtime = st.st_mtime;
	err = (status_t)utime(path, &utb);
	if(err < B_OK) {
		DIAGNOSTIC((stderr, "codec_cache_refresh: can't fix mtime for '%s'\n", path));
		// whatever.
	}

	free(buf);
	return C_OK;
}


void
BPrivate::register_codec_addon(const char *path, const BMessage &msg)
{
	BMediaFormats format_obj;
	media_format_description *mfd, *nmfd;
	media_format *fmts;
	media_type mt;
	ssize_t nbytes;
	size_t nfmts;
	int32 max_index;
	int32 index;
	int32 ndesc;

	// how many media_formats will this addon produce?
	//
	for(max_index = 0; msg.HasInt32("codec:ndesc", max_index); max_index++)
		;
	
	if(max_index == 0) {
		return;
	}

	fmts = new media_format[max_index];
	if(fmts == NULL) {
		return;
	}

	nfmts = 0;
	for(index = 0; index < max_index; index++) {
		media_format *fmt;
		media_type type;
		status_t err;

		err = msg.FindInt32("codec:ndesc", index, &ndesc);
		if(err != B_OK) {
			break;
		}
		if(ndesc <= 0) {
			DIAGNOSTIC((stderr, "codec_register: bad ndesc %ld for index %ld\n", ndesc, index));
			continue;
		}

		err = msg.FindData("codec:desc", B_RAW_TYPE, index, (const void**)&mfd, &nbytes);
		if(err != B_OK || nbytes != ndesc * sizeof(media_format_description)) {
			DIAGNOSTIC((stderr, "codec_register: FindData inconsistency or error: %s\n", strerror(err)));
			continue;
		}

		err = msg.FindInt32("codec:type", index, (int32*)&type);
		if(err != B_OK) {
			DIAGNOSTIC((stderr, "codec_register: can't get media_type: %s\n", strerror(err)));
			continue;
		}

		fmt = &fmts[nfmts];

		fmt->type = type;
		switch(fmt->type) {
		case B_MEDIA_RAW_AUDIO:
			fmt->u.raw_audio = media_multi_audio_format::wildcard;
			break;
		case B_MEDIA_RAW_VIDEO:
			fmt->u.raw_video = media_raw_video_format::wildcard;
			break;
		case B_MEDIA_MULTISTREAM:
			fmt->u.multistream = media_multistream_format::wildcard;
			break;
		case B_MEDIA_ENCODED_AUDIO:
			fmt->u.encoded_audio = media_encoded_audio_format::wildcard;
			break;
		case B_MEDIA_ENCODED_VIDEO:
			fmt->u.encoded_video = media_encoded_video_format::wildcard;
			break;
		default:
			memset(&fmt->u, 0, sizeof(fmt->u));
			break;
		}

		// ok, we have all the info we need to call MakeFormatFor
		//
		err = format_obj.MakeFormatFor(mfd, ndesc, fmt);
		if(err != B_OK) {
			DIAGNOSTIC((stderr, "register_codec_addon: MakeFormatFor failed: %s\n", strerror(err)));
			continue;
		}

		nfmts++;
	}

	if (nfmts > 0) {
		BMediaFormats::bind_addon(path, fmts, nfmts);
	}
	else {
		DIAGNOSTIC((stderr, "register_codec_addon: no formats to register for addon %s\n", path));
	}

	delete [] fmts;
}


void
BPrivate::codec_init_hook(const char *path, image_id imgid)
{
	BMessage msg;
	struct stat st;
	status_t err;
	ssize_t nbytes;
	int ret;


	if(path == NULL) {
		return;
	}

	//
	// we're already locked at this point
	//
	
	BNode node(path);
	if(node.InitCheck() != B_OK) {
		printf("codec_init_hook: couldn't create node for addon '%s'\n", path);
		return;
	}

	//
	// it's one big race between here and the call to utime() in refresh().
	// though the worst thing that will happen is all racing threads
	//   might load the addon (like they used to) and the final mtime of
	//   the file may be a little later than the one it started with.
	//   No-one's going to get stuck in a loop or anything; the only
	//   loop occurs when the attr size has changed since the sniff, in
	//   which case the entire process begins anew.
	// 
	ret = codec_cache_lookup(path, node, st, msg);
	if(ret != C_OK) {
		if(ret == C_REFRESH) {
printf("cache refresh %s\n", path);
			ret = codec_cache_refresh(path, node, st, imgid, msg);
			if(ret != C_OK) {
				// just write the timestamp into the attr.
				// read: this addon sucks as of this time.
				//       don't bother loading it unless it's changed.
				// only do this if the failure was the addon's fault.
				//
				if(ret == C_BAD_ADDON) {
					utimbuf utb;

printf("caching bad addon status for %s\n", path);
					node.WriteAttr(CODEC_CACHE_ATTR, B_ANY_TYPE, (off_t)0, (void*)&st.st_mtime, sizeof(time_t));
					utb.actime = st.st_atime;
					utb.modtime = st.st_mtime;
					utime(path, &utb);
				}
				return;
			}
		}
		else {
			return;
		}
	}

	register_codec_addon(path, msg);
}



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
BPrivate::extractor_load_hook(void *arg, image_id imgid)
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
//			FPRINTF((stderr, "Binding extractor %s\n", info.name));
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
		extractor_mgr = new _AddonManager(extractor_dirs, codec_init_hook, extractor_load_hook,
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
BPrivate::dec_load_hook(void *arg, image_id imgid)
{
	void (*register_dec)(media_format ** out_list, int32 * out_count);

	image_info info;
	get_image_info(imgid, &info);
//	FPRINTF((stderr, "dec_load_hook(%s)\n", info.name));

	if (get_image_symbol(imgid, "register_decoder",
						 B_SYMBOL_TYPE_TEXT, (void **)&register_dec) != B_OK) {
		DIAGNOSTIC((stderr, "register_decoder() not found\n"));
		return;
	}

	media_format * list = 0;
	int32 count = 0;

	register_dec(&list, &count);

	if ((list != 0) && (count > 0)) {
//		FPRINTF((stderr, "Binding decoder %s\n", info.name));
		BMediaFormats::bind_addon(info.name, list, count);
	}
	else {
		FPRINTF((stderr, "Decoder had no formats?\n"));
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
		decoder_mgr = new _AddonManager(decoder_dirs, BPrivate::codec_init_hook, BPrivate::dec_load_hook, NULL, NULL);
	} else {
		while(decoder_mgr == NULL)
			snooze(3000);
	}
	
	return decoder_mgr;
}		



static void
enc_load_hook(void *arg, image_id imgid)
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
