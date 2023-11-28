#ifndef _MIME_PRIVATE_H
#define _MIME_PRIVATE_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

#ifndef _MESSAGE_STRINGS_H
#include <message_strings.h>
#endif

#include <Mime.h>


// ----
// ---- Strings used by the MIME stuff
// ----
extern const char *B_SUPPORTED_MIME_ENTRY;	// "types"
extern const char *B_FILE_EXTENSIONS_ENTRY;	// "extensions"
extern const char *B_MIME_TYPE_ATTR;			// "BEOS:TYPE"
extern const char *B_APP_SIGNATURE_ATTR;		// "BEOS:APP_SIG"
extern const char *B_PREFERRED_APP_ATTR;		// "BEOS:PREF_APP"
extern const char *B_PREFERRED_PATH_ATTR;	// "BEOS:PPATH"

extern const char	 	*B_APP_MIME_TYPE			;

extern const char		*APP_FLAGS_ATTR				;
extern const char		*APP_VERSION_ATTR			;
extern const char		*APP_SUPPORTED_TYPES_ATTR	;

extern const int32		RSRC_ID						;
extern const int32		RESERVED_ICON_ID			;
extern const char		*MIME_PREFIX				;
extern const char		*META_PREFIX				;
extern const char		*STD_ICON_SUFFIX			;

extern const uint32	APP_FLAGS_TYPE				;
extern const uint32	APP_VERSION_TYPE			;
extern const uint32	LARGE_ICON_TYPE				;
extern const uint32	MINI_ICON_TYPE				;

/*
 The following extern constants are used for storing information in the meta-mime
 file.
*/
extern const char		*META_PREFERRED_APP			;
extern const char		*META_ATTR_INFO				;
extern const char		*META_EXTENSION_INFO		;
extern const char		*META_SHORT_DESC			;
extern const char		*META_LONG_DESC				;
extern const char		*META_CACHED_REF			;
extern const char		*META_SUPPORTED_TYPES_ATTR	;
extern const char		*META_TYPE_ATTR				;
extern const uint32		META_PREFERRED_APP_TYPE		;
extern const uint32		META_SHORT_DESC_TYPE		;
extern const uint32		META_LONG_DESC_TYPE			;
extern const uint32		META_CACHED_REF_TYPE		;

extern const long		LARGE_ICON_SIZE				;
extern const long		MINI_ICON_SIZE				;

extern const char		*META_MIME_ROOT				;

#define _MIME_PARENT_DIR_ B_COMMON_SETTINGS_DIRECTORY

class BResources;
class BNode;

extern bool _is_valid_app_sig_(const char *sig);

status_t get_attr_size(BNode *, const char *, type_code, size_t *);
status_t get_attr(BNode *, const char *, type_code, void *, size_t);
status_t set_attr(BNode *, const char *, type_code, const void *, size_t);
status_t get_icon_attr(BNode *, const char *, type_code, void *);
status_t set_icon_attr(BNode *, const char *, type_code, const void *,uint32);
status_t get_msg_attr(BNode *, const char *, type_code, BMessage *);
status_t set_msg_attr(BNode *, const char *, type_code, const BMessage *);

status_t get_rsrc_size(BResources *rsrc, const char *name, int32 id,
  							type_code type, size_t *l);
status_t get_rsrc(BResources *rsrc, const char *name, int32 id,
							type_code type, void *data, size_t l);
status_t set_rsrc(BResources *rsrc, const char *name, int32 id,
							type_code type, const void *data, size_t l);
status_t get_icon_rsrc(BResources *rsrc, const char *rsrc_name,
								type_code type, void *);
status_t set_icon_rsrc(BResources *rsrc, const char *name, type_code type,
								const void *, uint32, bool required_id = false);
status_t get_msg_rsrc(BResources *rsrc, const char *name, type_code type,
								BMessage *);
status_t set_msg_rsrc(BResources *rsrc, const char *name, type_code type,
								const BMessage *);

status_t get_data_size(int where, BNode *node, BResources *rsrc,
							const char *name, int32 id, type_code type,
							size_t *l);
status_t get_data(int where, BNode *node, BResources *rsrc,
							const char *name, int32 id, type_code type,
							void *data, size_t l);
status_t set_data(int where, BNode *node, BResources *rsrc,
							const char *name, int32 id, type_code type,
							const void *data, size_t l);
status_t get_icon_data(int where, BNode *node, BResources *rsrc,
							const char *rsrc_name, type_code type, void *);
status_t set_icon_data(int where, BNode *node, BResources *rsrc,
							const char *name, type_code type, const void *,
							uint32, bool required_id = false);
status_t get_msg_data(int where, BNode *node, BResources *rsrc,
							const char *name, type_code type, BMessage *);
status_t set_msg_data(int where, BNode *node, BResources *rsrc,
							const char *name, type_code type, const BMessage *);
status_t get_string_data(int where, BNode *node, BResources *rsrc, const char *name,
	int32 id, type_code type, BString *data);

extern char *tolower_str(char *str);

	// private calls used by the Tracker

extern status_t GetTrackerIconPrivate(BBitmap *, BNode *, const char *, icon_size);

namespace BPrivate {
extern bool CheckNodeIconHintPrivate(const BNode *, bool);
}
  bool has_icon_attr(const BNode *, const char *, type_code );


#endif
