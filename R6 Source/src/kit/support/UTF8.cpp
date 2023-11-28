#include <support_misc.h>	// for the deprecated calls
#include <UTF8.h>

#include <image.h>
#include <OS.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <Locker.h>

static image_id lib = -1;
static BLocker loadLock;

static void unload_libtextencoding();
static image_id get_libtextencoding();


void unload_libtextencoding() {
	if (loadLock.Lock()) {
		unload_add_on(lib);
		lib = -1;
		loadLock.Unlock();
	}
}

image_id
get_libtextencoding() {
	image_id id = -1;
	if (loadLock.Lock()) {
		if (lib >= B_OK)
			id = lib;
		else {
			// see if libtextencoding might already be loaded in the team
			thread_info tinfo;
			if (get_thread_info(find_thread(NULL), &tinfo) == B_OK) {
				image_info info;
				int32 cookie = 0;
				while (id == -1 && get_next_image_info(tinfo.team, &cookie, &info) == B_OK) {
					if (info.type == B_LIBRARY_IMAGE && (strstr(info.name, "libtextencoding.so"))) {
						// found the image!
						id = info.id;
					}
				}
			}
			
			if (id < B_OK) {
				// try to load libtextencoding as an addon
				id = load_add_on("/system/lib/libtextencoding.so");
				if (id >= B_OK) {
					// register the unload_libtextencoding atexit to cleanup
					atexit(unload_libtextencoding);
				}
			}
			if (id >= B_OK)
				lib = id;
		}
		loadLock.Unlock();
	}
	return id;
}

typedef status_t (*cvt_t)(uint32, const char *, int32 *, char *, int32 *, int32 *, char);

// Deprecated as of R4
status_t
convert_to_utf8(uint32		srcEncoding,
			    const char	*src, 
			    int32		*srcLen, 
			    char		*dst, 
			    int32		*dstLen)
{
	status_t status = B_ERROR;
	image_id id = get_libtextencoding();
	if (id >= B_OK) {
		void *to_utf8;
		status = get_image_symbol(lib, "convert_to_utf8__FUlPCcPlPcT2T2c", B_SYMBOL_TYPE_TEXT, &to_utf8);
		if (status == B_OK) {
			int32 state = 0;
			status = (*((cvt_t) to_utf8))(srcEncoding, src, srcLen, dst, dstLen, &state, 0x1a);
		}
	}
	return status;
}


// Deprecated as of R4
status_t
convert_from_utf8(uint32		dstEncoding,
				  const char	*src, 
				  int32			*srcLen, 
				  char			*dst, 
				  int32			*dstLen)
{
	status_t status = B_ERROR;
	image_id id = get_libtextencoding();
	if (id >= B_OK) {
		void *from_utf8;
		status = get_image_symbol(lib, "convert_from_utf8__FUlPCcPlPcT2T2c", B_SYMBOL_TYPE_TEXT, &from_utf8);
		if (status == B_OK) {
			int32 state = 0;
			status = (*((cvt_t) from_utf8))(dstEncoding, src, srcLen, dst, dstLen, &state, 0x1a);
		}
	}
	return status;
}

