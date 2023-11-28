/* ++++++++++
	FILE:	shared_fonts.h
	REVS:	$Revision: 1.14 $
	NAME:	pierre
	DATE:	Wed Mar 12 14:59:12 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _SHARED_FONTS_H
#define _SHARED_FONTS_H

#include <Font.h>
#include <SmartArray.h>

namespace BPrivate {

/* code used for font file type */
enum {
	FC_TRUETYPE_FONT    = 1,
	FC_TYPE1_FONT       = 2,
	FC_VIRTUAL_FONT     = 3,
	FC_BITMAP_FONT      = 4,
	FC_BAD_FILE         = 5,
	FC_NEW_FILE         = 6,
	FC_STROKE_FONT		= 7,

	FC_MAX_FILE_TYPE    = 8
};

/* mask use to control save_font_cache */
enum {
	FC_SAVE_FONT_FILE_LIST = 0x0001,
	FC_SAVE_FONT_CACHE     = 0x0002,
	FC_SAVE_PREFS          = 0x0004
};

/*
	mask and values used for encoding of the PrivatesFlags cache, as follow :
	0-1 : File format -- B_TRUETYPE_WINDOWS et al.
	      PF_INVALID if family/style ids used are invalid.
	2 : Has tuned fonts
	4-5 : Fixed size status - 0 for proportional
							  1 for fixed
							  2 for half-fixed.
*/

enum {
	PF_FILE_FORMAT_MASK		= 0x00000003,
	PF_INVALID				= 0x00000003,
	PF_HAS_TUNED_FONT		= 0x00000004,
	
	PF_FIX_MODE_MASK		= 0x00000030,
	PF_IS_FIXED				= 0x00000010,
	PF_IS_HIROSHI_FIXED		= 0x00000020
};

/* enum mask used for font file and dir status */
enum {
	FC_FONT_FILE        = 0x01,
	FC_FILE_ENABLE      = 0x02,
	FC_DIR_ENABLE       = 0x04,
	FC_FILE_SCANNED     = 0x08,


	FC_MAX_FILE_STATUS  = 0x10,
	FC_MAX_DIR_STATUS  = 0x10
};

/* code used for dir font type */
enum {
	FC_SYSTEM_FOLDER    = 1,
	FC_USER_FOLDER      = 2,

	FC_MAX_DIR_TYPE     = 3
};

enum {
	FC_CMD_CREATE_TUNED_FONT = 0x0000,
	FC_FLUSH_FONT_SET        = 0x0001,
	FC_CLOSE_ALL_FONT_FILES  = 0x0002,
	FC_RESCAN_ALL_FILES      = 0x0003
};

enum {
	FC_ESCAPE_AS_FLOAT			= 0,
	FC_ESCAPE_AS_BPOINT			= 1,
	FC_ESCAPE_AS_BPOINT_PAIR	= 2
};

/* private struct for folder/file font management */
struct font_folder_info {
	char        *pathname;
	uchar       folder_type;
	uchar       status;
	uint16      dir_id;
};

struct font_file_info {
	char        *filename;
	uchar       file_type;
	uchar       status;
	uint16      dir_id;
	font_family family;
	font_style  style;
	float       size;
};

typedef struct {
	uint16   f_id;
	uint16   s_id;
	float    size;
	float    rotation;
	float    shear;
	uchar    encoding;
	uchar    spacing_mode;
	uint16   faces;
	uint32   flags;
	uint32   overlay_size;
} fc_context_packet;

struct fc_tuned_font_info {
	float    size;
	float    shear; 
	float    rotation;
	uint32   flags;
	uint16   face;
};

typedef struct {
    int32    sheared_font_bonus;
    int32    rotated_font_bonus;
	float    oversize_threshold;
	int32    oversized_font_bonus;
	int32    cache_size;
	float    spacing_size_threshold;
} fc_font_cache_settings;

/*
 * Internal font overlay storage and lookup.
 */

struct font_overlay_entry {
	uint16	f_id;
};

// these are for the app_server to supply its own implementation;
// by default, these go through the app session to get the information
// from the app_server.
extern status_t (*font_family_name_to_id)(const font_family name, uint16* out_id);
extern status_t (*font_family_id_to_name)(uint16 id, font_family* out_name);

class FontOverlayMap {
public:
		FontOverlayMap();
		FontOverlayMap(const FontOverlayMap& o);
		~FontOverlayMap();

		FontOverlayMap& operator=(const FontOverlayMap& o);
		
		int32 Lookup(	uint32 code, font_overlay_entry** out_entry,
						uint32* out_first, uint32* out_last);
		int32 Lookup(	uint32 code, const font_overlay_entry** out_entry,
						uint32* out_first, uint32* out_last) const;
		
		int32 CountEntries() const;
		font_overlay_entry* EntryAt(int32 i,
									uint32* out_first, uint32* out_last);
		const font_overlay_entry* EntryAt(	int32 i,
											uint32* out_first, uint32* out_last) const;
		
		font_overlay_entry* AddEntry(uint32 first, uint32 last);
		bool RemoveEntry(uint32 code);
		bool RemoveEntries(uint32 first, uint32 last);
		
		// Mark this overlay as being "nothing".  This is different than
		// empty in that, though it also doesn't contain anything, you are
		// saying that is exactly what you want.
		void SetToNothing();
		bool IsNothing() const;
		
		void MakeEmpty();
		
		// Flattening and unflattening (for system-independent
		// persistent storage).  This generates and parses an array
		// of flat_font_overlay structures.
		ssize_t			FlattenedSize() const;
		status_t		Flatten(void *buffer, ssize_t size) const;
		status_t		Unflatten(const void *buf, ssize_t size, bool keep_invalid=false);

		// Font packet suffix generation, to be placed after the
		// standard font packet data when sending over a session.
		ssize_t			PacketSize() const;
		status_t		Packetize(void* buffer, ssize_t size) const;
		status_t		Unpacketize(const void* buf, ssize_t size);
		
private:
		int32 Find(uint32 key, bool* found) const;
		int32 FindRange(uint32 low, uint32 high, int32* length) const;

		struct range {
			uint32 low;
			uint32 high;
			font_overlay_entry value;
		};
		SmartArray<range> fEntries;
		bool fNothing;
};

/*
 * Abstract font interface -- can be implemented either by BFont on the
 * client side or SFont in the server.  This allows us to implement generic
 * algorithms that can work on either type of font.
 */

class BAbstractFont {
public:
inline						BAbstractFont()		{ }
inline virtual				~BAbstractFont()	{ }

virtual	bool				IsValid() const = 0;

virtual	BAbstractFont&		SetTo(const BAbstractFont &source, uint32 mask = B_FONT_ALL);
virtual	BAbstractFont&		SetTo(font_which which, uint32 mask = B_FONT_ALL) = 0;

virtual	status_t			ApplyCSS(const char* css_style, uint32 mask = B_FONT_ALL);

virtual	status_t			SetFamilyAndStyle(const font_family family, 
											  const font_style style) = 0;
virtual	void				SetFamilyAndStyle(uint32 code, uint16 face) = 0;
virtual	status_t			SetFamilyAndFace(const font_family family, uint16 face) = 0;
		
virtual	void				SetSize(float size) = 0;
virtual	void				SetShear(float shear) = 0;
virtual	void				SetRotation(float rotation) = 0;
virtual	void				SetSpacing(uint8 spacing) = 0;
virtual	void				SetEncoding(uint8 encoding) = 0;
virtual	void				SetFace(uint16 face) = 0;
virtual	void				SetFlags(uint32 flags) = 0;

virtual	void				GetFamilyAndStyle(font_family *family,
											  font_style *style) const = 0;
virtual	uint32				FamilyAndStyle() const = 0;
virtual	float				Size() const = 0;
virtual	float				Shear() const = 0;
virtual	float				Rotation() const = 0;
virtual	uint8				Spacing() const = 0;
virtual	uint8				Encoding() const = 0;
virtual	uint16				Face() const = 0;
virtual	uint32				Flags() const = 0;

virtual	FontOverlayMap*		EditOverlay() = 0;
virtual	const FontOverlayMap* Overlay() const = 0;
virtual	void				ClearOverlay() = 0;

virtual	ssize_t				FlattenedSize() const;
virtual	status_t			Flatten(void *buffer, ssize_t size) const;
virtual	status_t			Unflatten(type_code c, const void *buf, ssize_t size,
									  bool force_valid);
};

/*
 * Format of the flattened font representation.  Unlike an
 * fc_context_packet, this must be portable outside of the system
 * it is created in.  All fields are stored as little-endian.
 */

// Header.  This is all the basic font information.  It is designed
// to be the same as my old FFont class.  (Backwards compatibility, oh my.)
struct flat_font_header {
	int32 mask;
	float size;
	float shear;
	float rotation;
	uint32 flags;
	uint16 face;
	uint8 spacing;
	uint8 encoding;
	char family[64];
	char style[64];
};

// Optional part: extended basic information.
struct flat_font_extra {
	int32 size;				// number of bytes in this structure
	int32 overlay_size;		// bytes of overlay entries that follow
};

// If 'overlay_size' is > 0, that number of BYTES of the following structures appear.
struct flat_font_overlay {
	int32 size;				// number of bytes in this structure
	uint32 start, end;		// character range
	char family[64];
};

}
using namespace BPrivate;

class BFont;

/* private API to control and set font the font cache */

status_t    _add_to_font_folder_list_(char *pathname);

status_t    _remove_from_font_folder_list_(char *pathname);

status_t    _enable_font_folder_(char *pathname, bool enable);

status_t    _enable_font_file_(int32 dir_id, char *filename, bool enable);

int32       _count_font_folders_();

status_t    _get_nth_font_folder_(int32 index, font_folder_info **info);

int32       _count_font_files_(int32 dir_id);

status_t    _get_nth_font_file_(int32 dir_id, font_file_info **info);


void        _save_font_cache_state_(int32 flags);


void        _restore_key_map_();


void        _font_control_(BFont *font, int32 cmd, void *data);

/* function to be called at the end of BApplication constructor */
void _init_global_fonts_();
/* function to be called in BApplication destructor */
void _clean_global_fonts_();

#endif
