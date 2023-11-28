/*******************************************************************************
/
/	File:			Font.h
/
/   Description:    BFont objects represent individual font styles.
/                   Global font cache and font info functions defined below.
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _FONT_H
#define _FONT_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Flattenable.h>
#include <InterfaceDefs.h>

/*----------------------------------------------------------------*/
/*----- BFont defines and structures -----------------------------*/

#define B_FONT_FAMILY_LENGTH 63
typedef char font_family[B_FONT_FAMILY_LENGTH + 1];
#define B_FONT_STYLE_LENGTH 63
typedef char font_style[B_FONT_STYLE_LENGTH + 1];

enum {
	B_CHAR_SPACING		= 0,
	B_STRING_SPACING	= 1,
	B_BITMAP_SPACING	= 2,
	B_FIXED_SPACING		= 3,
	
	B_SPACING_COUNT		= 4
};

enum font_direction {
	B_FONT_LEFT_TO_RIGHT = 0,
	B_FONT_RIGHT_TO_LEFT = 1
};

enum {
	B_DISABLE_ANTIALIASING		= 0x00000001,
	B_NORMAL_ANTIALIASING		= 0x00000002,
	B_TV_ANTIALIASING			= 0x00000003,
	B_ANTIALIASING_MASK			= 0x00000007,
	
	B_DISABLE_HINTING			= 0x00000100,
	B_ENABLE_HINTING			= 0x00000200,
	B_HINTING_MASK				= 0x00000300,
	
	B_DISABLE_GLOBAL_OVERLAY	= 0x00010000,
	B_GLOBAL_OVERLAY_MASK		= 0x00010000,
	
	B_FORCE_ANTIALIASING		= B_NORMAL_ANTIALIASING	// backwards compatibility
};

enum {
	B_TRUNCATE_END       = 0,
	B_TRUNCATE_BEGINNING = 1,
	B_TRUNCATE_MIDDLE    = 2,
	B_TRUNCATE_SMART     = 3
};

enum {
	B_UNICODE_UTF8    = 0,
	B_ISO_8859_1      = 1,
	B_ISO_8859_2      = 2,
	B_ISO_8859_3      = 3,
	B_ISO_8859_4      = 4,
	B_ISO_8859_5      = 5,
	B_ISO_8859_6      = 6,
	B_ISO_8859_7      = 7,
	B_ISO_8859_8      = 8,
	B_ISO_8859_9      = 9,
	B_ISO_8859_10     = 10,
	B_MACINTOSH_ROMAN = 11
};

enum {
	B_SCREEN_FONT_CACHE      = 0x0001,
	B_PRINTING_FONT_CACHE    = 0x0002,
	B_DEFAULT_CACHE_SETTING  = 0x0004,
	B_APP_CACHE_SETTING      = 0x0008
};

enum {
	B_HAS_TUNED_FONT         = 0x0001,
	B_IS_FIXED               = 0x0002,
	B_IS_FULL_AND_HALF_FIXED = 0x0004
};

enum {
	B_ITALIC_FACE		= 0x0001,
	B_UNDERSCORE_FACE	= 0x0002,
	B_NEGATIVE_FACE		= 0x0004,
	B_OUTLINED_FACE		= 0x0008,
	B_STRIKEOUT_FACE	= 0x0010,
	B_BOLD_FACE			= 0x0020,
	B_REGULAR_FACE		= 0x0040
};

enum {
	B_FONT_FAMILY_AND_STYLE	= 0x00000001,
	B_FONT_SIZE				= 0x00000002,
	B_FONT_SHEAR			= 0x00000004,
	B_FONT_ROTATION			= 0x00000008,
	B_FONT_SPACING     		= 0x00000010,
	B_FONT_ENCODING			= 0x00000020,
	B_FONT_FACE				= 0x00000040,
	B_FONT_FLAGS			= 0x00000080,
	B_FONT_OVERLAY			= 0x00000100,
	B_FONT_ALL				= 0x000001FF
};

enum font_which {
	B_PLAIN_FONT			= 1,
	B_BOLD_FONT				= 2,
	B_FIXED_FONT			= 3,
	B_SYMBOL_FONT			= 4,
	B_SERIF_FONT			= 5,
	
	B__NUM_FONT				= 5
};

enum font_metric_mode {
	B_SCREEN_METRIC		= 0,
	B_PRINTING_METRIC	= 1
};

enum font_file_format {
	B_TRUETYPE_WINDOWS			= 0,
	B_POSTSCRIPT_TYPE1_WINDOWS	= 1,
	B_BITSTREAM_STROKE_WINDOWS	= 2
};

class unicode_block {
public:
	inline					unicode_block();
	inline					unicode_block(uint64 block2, uint64 block1);

	inline bool				Includes(const unicode_block &block) const;
	inline unicode_block	operator&(const unicode_block &block) const;		
	inline unicode_block	operator|(const unicode_block &block) const;		
	inline unicode_block	&operator=(const unicode_block &block);
	inline bool				operator==(const unicode_block &block) const;		
	inline bool				operator!=(const unicode_block &block) const;
	
private:
	uint64					fData[2];
};

struct edge_info {
	float	left;
	float	right;
};

struct font_height {
	float	ascent;
	float	descent;
	float	leading;
};

struct escapement_delta {
	float	nonspace;
	float	space;
};

struct font_cache_info {
    int32    sheared_font_penalty;
    int32    rotated_font_penalty;
	float    oversize_threshold;
	int32    oversize_penalty;
	int32    cache_size;
	float    spacing_size_threshold;
};

struct tuned_font_info {
	float    size;
	float    shear; 
	float    rotation;
	uint32   flags;
	uint16   face;
};

struct font_overlay {
	uint32   first_char;
	uint32   last_char;
	uint16   family_id;

	status_t set_family(const font_family family);
	status_t get_family(font_family* family) const;
	
	         font_overlay();
	         ~font_overlay();
	font_overlay& operator=(const font_overlay& o);
	
private:
	uint16   _pad;
	int32    _reserved[32];
};

class BDataIO;
class BShape;
class BString;
class BFont;
class BPoint;
namespace BPrivate {
class IKAccess;
struct font_extra;
class BFontProxy;
}

/*----------------------------------------------------------------*/
/*----- Private --------------------------------------------------*/

void _font_control_(BFont *font, int32 cmd, void *data);

/*----------------------------------------------------------------*/
/*----- BFont class ----------------------------------------------*/

class BFont : public BFlattenable {
public:
							BFont();
							BFont(font_which which);
							BFont(const BFont &font);	
							BFont(const BFont *font);			

virtual						~BFont();

		// Call this function to indicate that you are a modern Be
		// application, who will do polite things like call the
		// BFont constructor and destructor.  This allows you to use
		// the font overlay features.
static	void				UseModernFonts();

static	status_t			GetStandardFont(font_which which, BFont* into);
static	status_t			SetStandardFont(font_which which, const BFont& from);

static	status_t			GetGlobalOverlay(BFont* into);
static	status_t			SetGlobalOverlay(const BFont& from);

		BFont&				SetTo(const BFont &source, uint32 mask = B_FONT_ALL);
		BFont&				SetTo(font_which which, uint32 mask = B_FONT_ALL);
		
		status_t			ApplyCSS(const char* css_style, uint32 mask = B_FONT_ALL);
		
		status_t			SetFamilyAndStyle(const font_family family, 
											  const font_style style);
		status_t			SetFamilyAndFace(const font_family family, uint16 face);
		
		void				SetSize(float size);
		void				SetShear(float shear);
		void				SetRotation(float rotation);
		void				SetSpacing(uint8 spacing);
		void				SetEncoding(uint8 encoding);
		void				SetFace(uint16 face);
		void				SetFlags(uint32 flags);

		void				GetFamilyAndStyle(font_family *family,
											  font_style *style) const;
		float				Size() const;
		float				Shear() const;
		float				Rotation() const;
		uint8				Spacing() const;
		uint8				Encoding() const;
		uint16				Face() const;
		uint32				Flags() const;
		
		status_t			AddOverlay(const font_overlay& overlay);
		status_t			FindOverlay(uint32 for_char, font_overlay* into) const;
		status_t			RemoveOverlay(uint32 for_char);
		status_t			RemoveOverlays(uint32 from_char, uint32 to_char);
		void				RemoveAllOverlays();
		
		int32				CountOverlays() const;
		status_t			OverlayAt(int32 index, font_overlay* into) const;
		
		font_direction		Direction() const; 
		bool				IsFixed() const;	
		bool				IsFullAndHalfFixed() const;	
		BRect				BoundingBox() const;
		unicode_block		Blocks() const;
		font_file_format	FileFormat() const;

		int32				CountTuned() const;
		void				GetTunedInfo(int32 index, tuned_font_info *info) const;

		void				TruncateString(BString* in_out,
										   uint32 mode,
										   float width) const;
		void            	GetTruncatedStrings(const char *stringArray[], 
												int32 numStrings, 
												uint32 mode,
												float width, 
												BString resultArray[]) const;
		void            	GetTruncatedStrings(const char *stringArray[], 
												int32 numStrings, 
												uint32 mode,
												float width, 
												char *resultArray[]) const;

		float				StringWidth(const char *string) const;
		float				StringWidth(const char *string, int32 length) const;
		void				GetStringWidths(const char *stringArray[], 
											const int32 lengthArray[], 
											int32 numStrings, 
											float widthArray[]) const;

		void				GetEscapements(const char charArray[], 
										   int32 numChars,
										   float escapementArray[]) const;
		void				GetEscapements(const char charArray[], 
										   int32 numChars,
										   escapement_delta *delta, 
										   float escapementArray[]) const;
		void				GetEscapements(const char charArray[], 
										   int32 numChars,
										   escapement_delta *delta, 
										   BPoint escapementArray[]) const;
		void				GetEscapements(const char charArray[], 
										   int32 numChars,
										   escapement_delta *delta, 
										   BPoint escapementArray[],
										   BPoint offsetArray[]) const;
									   
		void				GetEdges(const char charArray[], 
									 int32 numBytes,
									 edge_info edgeArray[]) const;
		void				GetHeight(font_height *height) const;
		
		void				GetBoundingBoxesAsGlyphs(const char charArray[], 
													 int32 numChars, 
													 font_metric_mode mode,
													 BRect boundingBoxArray[]) const;
		void				GetBoundingBoxesAsString(const char charArray[], 
													 int32 numChars, 
													 font_metric_mode mode,
													 escapement_delta *delta,
													 BRect boundingBoxArray[]) const;
		void				GetBoundingBoxesForStrings(const char *stringArray[],
													   int32 numStrings,
													   font_metric_mode mode,
													   escapement_delta deltas[],
													   BRect boundingBoxArray[]) const;
		
		void				GetGlyphShapes(const char charArray[],
										   int32 numChars,
										   BShape *glyphShapeArray[]) const;							   

		void				GetHasGlyphs(const char charArray[],
										 int32 numChars,
										 bool hasArray[]) const;
									 
		BFont&				operator=(const BFont &font); 
		bool				operator==(const BFont &font) const;
		bool				operator!=(const BFont &font) const; 

		int					Compare(const BFont &font,
									uint32 mask = B_FONT_ALL) const;
		
		void				PrintToStream() const;
		
		// BFlattenable interface.
virtual	bool				IsFixedSize() const;
virtual	type_code			TypeCode() const;
virtual	ssize_t				FlattenedSize() const;
virtual	status_t			Flatten(void *buffer, ssize_t size) const;
virtual	bool				AllowsTypeCode(type_code code) const;
virtual	status_t			Unflatten(type_code c, const void *buf, ssize_t size);

/*----- Private or reserved -----------------------------------------*/
private:

friend class BApplication;
friend class BPrivate::IKAccess;
friend class BPrivate::BFontProxy;
friend class DecorVariableFont;
friend class DecorStream;
friend void _font_control_(BFont*, int32, void*);

		uint16				fFamilyID;
		uint16				fStyleID;
		float				fSize;
		float				fShear;
		float				fRotation;
		uint8				fSpacing;
		uint8				fEncoding;
		uint16				fFace;
		uint32				fFlags;
		font_height			fHeight;
		int32				fPrivateFlags;
		BPrivate::font_extra* fExtra;

		void				SetFamilyAndStyle(uint32 code);
		uint32				FamilyAndStyle() const;
		void				Reset();
		void           		GetTruncatedStrings64(const char *stringArray[], 
												  int32 numStrings, 
												  uint32 mode,
												  float width, 
												  char *resultArray[]) const;
		void           		GetTruncatedStrings64(const char *stringArray[], 
												  int32 numStrings, 
												  uint32 mode,
												  float width, 
												  BString resultArray[]) const;
		void				_GetEscapements_(const char charArray[],
											 int32 numChars, 
											 escapement_delta *delta,
											 uint8 mode,
											 float *escapements,
											 float *offsets = NULL) const;
		void				_GetBoundingBoxes_(const char charArray[], 
											   int32 numChars, 
											   font_metric_mode mode,
											   bool string_escapement,
											   escapement_delta *delta,
											   BRect boundingBoxArray[]) const;
};

/*----------------------------------------------------------------*/
/*----- BFont related declarations -------------------------------*/

extern const BFont* be_plain_font;
extern const BFont* be_bold_font;
extern const BFont* be_fixed_font;

int32       count_font_families();
status_t    get_font_family(int32		index, 
									   font_family	*name,
									   uint32		*flags = NULL);

int32       count_font_styles(font_family name);
status_t    get_font_style(font_family	family, 
									  int32			index,
									  font_style	*name, 
									  uint32		*flags = NULL);
status_t    get_font_style(font_family	family, 
									  int32			index,
									  font_style	*name, 
									  uint16		*face, 
									  uint32		*flags = NULL);

bool        update_font_families(bool check_only);

status_t    get_font_cache_info(uint32 id, void  *set);
status_t    set_font_cache_info(uint32 id, void  *set);

BDataIO& operator<<(BDataIO& io, const BFont& font);

/*----------------------------------------------------------------*/
/*----- unicode_block inlines ------------------------------------*/

unicode_block::unicode_block() { fData[0] = fData[1] = 0LL; }

unicode_block::unicode_block(uint64 block2, uint64 block1) {
	fData[0] = block1;
	fData[1] = block2;
}

bool unicode_block::Includes(const unicode_block &block) const {
	return (((fData[0] & block.fData[0]) == block.fData[0]) &&
			((fData[1] & block.fData[1]) == block.fData[1]));
}

unicode_block unicode_block::operator&(const unicode_block &block) const {
	unicode_block		res;
	
	res.fData[0] = fData[0] & block.fData[0];
	res.fData[1] = fData[1] & block.fData[1];
	return res;
}
		
unicode_block unicode_block::operator|(const unicode_block &block) const {
	unicode_block		res;
	
	res.fData[0] = fData[0] | block.fData[0];
	res.fData[1] = fData[1] | block.fData[1];
	return res;
}
		
unicode_block &unicode_block::operator=(const unicode_block &block) {
	fData[0] = block.fData[0];
	fData[1] = block.fData[1];
	return *this;
}

bool unicode_block::operator==(const unicode_block &block) const {
	return ((fData[0] == block.fData[0]) && (fData[1] == block.fData[1]));
}

bool unicode_block::operator!=(const unicode_block &block) const {
	return ((fData[0] != block.fData[0]) || (fData[1] != block.fData[1]));
}

/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/

#endif /* _FONT_H */
