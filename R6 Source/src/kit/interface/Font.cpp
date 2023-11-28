// ============================================================
//  Font.cpp	by Hiroshi Lockheimer
// ============================================================

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <new>

#include <Debug.h>
#include <shared_defaults.h>

#ifndef _FONT_H
#include <Font.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _SESSION_H
#include <session.h>
#endif

#ifndef _INTERFACE_DEFS_H
#include "InterfaceDefs.h"
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _SHAPE_H
#include <Shape.h>
#endif

#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif
#ifndef _MESSAGES_H
#include <messages.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _SESSION_H
#include <session.h>
#endif
#ifndef _SHARED_FONTS_H
#include <shared_fonts.h>
#endif
#ifndef _STREAM_IO_H
#include <StreamIO.h>
#endif
#ifndef __BSTRING__
#include <String.h>
#endif
#ifndef _BYTEORDER_H
#include <ByteOrder.h>
#endif
#include <input_server_private.h>
#include <shared_support.h>

#define PRINT_CMD(x)
//#define PRINT_CMD(x) printf x


inline int32
UTF8_CHAR_LEN(uchar c)
	{ return (((0xE5000000 >> (((c) >> 3) & 0x1E)) & 3) + 1); }


const BFont* be_plain_font = NULL;
const BFont* be_bold_font = NULL;
const BFont* be_fixed_font = NULL;
const BFont* _be_symbol_font_ = NULL;		// private

const float	kInvalidHeight = -1048576.0;

// This is a really gross hack because BTextView's style run arrays
// used to use malloc() and free() without ever calling the BFont
// constructor or destructor.  So, when this flag is not set, we can
// not assume that the fExtra field is valid.  The flag gets set the
// first time the program calls any of the overlay functions, implying
// that it is a new program and can thus make sure it does the correct
// thing with BFont.
static bool gCanUseExtra = false;

static void		_get_flags_info_(uint16 f, uint16 s, int32 *flags);
static int32    _tuned_count_(uint16 f_id, uint16 s_id);
static status_t _get_tuned_info_(uint16 f_id, 
								 uint16 s_id, 
								 int32 index, 
								 tuned_font_info *info);
static void		_get_font_ids_(	const font_family family, const font_style style,
								uint16 faces, uint16 cur_fid,
								uint16* out_fid, uint16* out_sid,
								uint16* out_face, uint32* out_flags);
static void		_get_font_names_(	uint16 fid, uint16 sid,
									font_family* out_family, font_style* out_style);

status_t font_overlay::set_family(const font_family family)
{
	_get_font_ids_(family, NULL, 0xFFFF, 0xFFFF, &family_id, NULL, NULL, NULL);
	return family_id != 0xFFFF ? B_OK : B_NAME_NOT_FOUND;
}

status_t font_overlay::get_family(font_family* family) const
{
	_get_font_names_(family_id, 0xFFFF, family, NULL);
	return (family && **family) ? B_OK : B_BAD_VALUE;
}

font_overlay::font_overlay()
	: first_char(0), last_char(0xFFFFFFFF), family_id(0xFFFF)
{
}

font_overlay::~font_overlay()
{
}

font_overlay& font_overlay::operator=(const font_overlay& o)
{
	memcpy(this, &o, sizeof(*this));
	return *this;
}

namespace BPrivate {
	struct font_extra
	{
		FontOverlayMap overlay;
	};
	
	class BFontProxy : public BAbstractFont {
	public:
	inline						BFontProxy(BFont& target) : fTarget(target)	{ }
	inline virtual				~BFontProxy()								{ }
	
	virtual	bool				IsValid() const
	{
		return (fTarget.fFamilyID != 0xFFFF && fTarget.fStyleID != 0xFFFF);
	}
	
	virtual	BAbstractFont&		SetTo(const BAbstractFont &source, uint32 mask = B_FONT_ALL)
	{
		if (mask == B_FONT_ALL) {
			const BFontProxy* f = dynamic_cast<const BFontProxy*>(&source);
			if (f) {
				fTarget = f->fTarget;
				return *this;
			}
		}
		return BAbstractFont::SetTo(source, mask);
	}
	
	virtual	BAbstractFont&		SetTo(font_which which, uint32 mask = B_FONT_ALL)
	{ fTarget.SetTo(which, mask); return *this; }
		
	virtual	status_t			ApplyCSS(const char* css_style, uint32 mask = B_FONT_ALL)
	{
		const status_t result = BAbstractFont::ApplyCSS(css_style, mask);
		if ((mask&B_FONT_OVERLAY) && gCanUseExtra)
			fTarget.RemoveAllOverlays();
		return result;
	}
	
	virtual	status_t			SetFamilyAndStyle(const font_family family, 
												  const font_style style)
	{ return fTarget.SetFamilyAndStyle(family, style); }
	virtual	void				SetFamilyAndStyle(uint32 code, uint16 face)
	{ fTarget.SetFamilyAndStyle(code); fTarget.fFace = face; }
	virtual	status_t			SetFamilyAndFace(const font_family family, uint16 face)
	{ return fTarget.SetFamilyAndFace(family, face); }
			
	virtual	void				SetSize(float size)			{ fTarget.SetSize(size); }
	virtual	void				SetShear(float shear)		{ fTarget.SetShear(shear); }
	virtual	void				SetRotation(float rotation)	{ fTarget.SetRotation(rotation); }
	virtual	void				SetSpacing(uint8 spacing)	{ fTarget.SetSpacing(spacing); }
	virtual	void				SetEncoding(uint8 encoding)	{ fTarget.SetEncoding(encoding); }
	virtual	void				SetFace(uint16 face)		{ fTarget.SetFace(face); }
	virtual	void				SetFlags(uint32 flags)		{ fTarget.SetFlags(flags); }
	
	virtual	void				GetFamilyAndStyle(font_family *family,
												  font_style *style) const
	{ return fTarget.GetFamilyAndStyle(family, style); }
	virtual	uint32				FamilyAndStyle() const		{ return fTarget.FamilyAndStyle(); }
	virtual	float				Size() const				{ return fTarget.Size(); }
	virtual	float				Shear() const				{ return fTarget.Shear(); }
	virtual	float				Rotation() const			{ return fTarget.Rotation(); }
	virtual	uint8				Spacing() const				{ return fTarget.Spacing(); }
	virtual	uint8				Encoding() const			{ return fTarget.Encoding(); }
	virtual	uint16				Face() const				{ return fTarget.Face(); }
	virtual	uint32				Flags() const				{ return fTarget.Flags(); }
	
	virtual	FontOverlayMap*		EditOverlay()
	{
		if (!fTarget.fExtra && gCanUseExtra)
			fTarget.fExtra = new(std::nothrow) font_extra;
		if (fTarget.fExtra)
			return &fTarget.fExtra->overlay;
		return NULL;
	}
	
	virtual	const FontOverlayMap* Overlay() const			{ return fTarget.fExtra ? &fTarget.fExtra->overlay : NULL; }
	virtual	void				ClearOverlay()				{ fTarget.RemoveAllOverlays(); }

	private:
			BFont&				fTarget;
	};
}

BFont::BFont()
{
	fExtra = NULL;
	if ((be_plain_font != NULL) && (this != be_plain_font))
		*this = *be_plain_font;
	else
		Reset();
}


BFont::BFont(font_which which)
{
	fExtra = NULL;
	GetStandardFont(which, this);
}


BFont::BFont(
	const BFont	&font)
	: BFlattenable()
{
	fFamilyID = font.fFamilyID;
	fStyleID = font.fStyleID;
	fSize = font.fSize;
	fShear = font.fShear;
	fRotation = font.fRotation;
	fSpacing = font.fSpacing;
	fEncoding = font.fEncoding;
	fFace = font.fFace;
	fFlags = font.fFlags;
	fHeight = font.fHeight;
	fPrivateFlags = font.fPrivateFlags;
	if (font.fExtra && gCanUseExtra)
		fExtra = new(std::nothrow) font_extra(*font.fExtra);
	else
		fExtra = NULL;
}


BFont::BFont(
	const BFont	*font)
{
	fFamilyID = font->fFamilyID;
	fStyleID = font->fStyleID;
	fSize = font->fSize;
	fShear = font->fShear;
	fRotation = font->fRotation;
	fSpacing = font->fSpacing;
	fEncoding = font->fEncoding;
	fFace = font->fFace;
	fFlags = font->fFlags;
	fHeight = font->fHeight;
	fPrivateFlags = font->fPrivateFlags;
	if (font->fExtra && gCanUseExtra)
		fExtra = new(std::nothrow) font_extra(*font->fExtra);
	else
		fExtra = NULL;
}


void BFont::Reset()
{
	fFamilyID = 0xFFFF;
	fStyleID = 0xFFFF;
	fSize = B_DEFAULT_FONT_SIZE;
	fShear = 0.0;
	fRotation = 0.0;
	fSpacing = B_BITMAP_SPACING;
	fEncoding = B_UNICODE_UTF8;
	fFace = 0;
	fFlags = 0;
	fPrivateFlags = 0;
	fHeight.ascent = fHeight.descent = fHeight.leading = kInvalidHeight;
	if (fExtra && gCanUseExtra) {
		delete fExtra;
		fExtra = NULL;
	}
}

BFont::~BFont()
{
	// !!! WARNING !!!
	// The BFont destructor didn't exist in R5 and earlier.
	// And even now, there is code that won't call it (look at
	// the style array in BTextView for a particurily heinous
	// example).
	//
	// SO:
	//
	// DON'T DEPEND ON THIS BEING CALLED.
	//
	// But I -really- want to add more data to BFont.  So I am
	// going to, but we need to be careful that old applications
	// don't get fonts with this data.  We probably can't be
	// completely correct about this, though...
	
	if (gCanUseExtra) {
		delete fExtra;
		fExtra = NULL;
	}
}


void
BFont::UseModernFonts()
{
	gCanUseExtra = true;
}


status_t
BFont::GetStandardFont(
	font_which which,
	BFont* into)
{
	if ((which < B_PLAIN_FONT || (which-B_PLAIN_FONT) >= B__NUM_FONT) &&
		(which < (B_PLAIN_FONT-100) || (which-B_PLAIN_FONT) >= B__NUM_FONT-100) &&
		which != 99 && which != (99-100))
		which = B_PLAIN_FONT;
		
	_BAppServerLink_ link;
	link.session->swrite_l(GR_GET_STANDARD_FONTS);
	link.session->swrite_l(which);
	link.session->flush();
	IKAccess::ReadFont(into, link.session);
	
	into->SetSpacing(which == B_FIXED_FONT ? B_FIXED_SPACING : B_BITMAP_SPACING);
	into->SetEncoding(B_UNICODE_UTF8);
	return B_OK;
}


status_t
BFont::SetStandardFont(
	font_which which,
	const BFont& from)
{
	if ((which < B_PLAIN_FONT || (which-B_PLAIN_FONT) > B__NUM_FONT) && which != 99)
		return B_BAD_VALUE;
	
	_BAppServerLink_ link;
	link.session->swrite_l(GR_SET_STANDARD_FONTS);
	link.session->swrite_l(which);
	IKAccess::WriteFont(&from, link.session);
	link.session->flush();
	return B_OK;
}


status_t
BFont::GetGlobalOverlay(
	BFont* into)
{
	// 99 is a magic index for the overlay font.
	return GetStandardFont((font_which)99, into);
}


status_t
BFont::SetGlobalOverlay(
	const BFont& from)
{
	// 99 is a magic index for the overlay font.
	return SetStandardFont((font_which)99, from);
}


BFont&
BFont::SetTo(const BFont &source, uint32 mask)
{
	if (mask == B_FONT_ALL)
		*this = source;
	else {
		BFontProxy thisProxy(*this);
		BFontProxy sourceProxy(const_cast<BFont&>(source));
		thisProxy.SetTo(sourceProxy, mask);
	}
	return *this;
}


BFont&
BFont::SetTo(font_which which, uint32 mask)
{
	if (mask == B_FONT_ALL)
		GetStandardFont(which, this);
	else {
		BFont font;
		GetStandardFont(which, &font);
		SetTo(font, mask);
	}
	return *this;
}


status_t
BFont::ApplyCSS(const char* in_string, uint32 mask)
{
	BFontProxy proxy(*this);
	return proxy.ApplyCSS(in_string, mask);
}


status_t
BFont::SetFamilyAndStyle(
	const font_family	family,
	const font_style	style)
{
	uint16				buffer[3];
	uint32				fake_faces, fake_flags;
	font_style			style_def;
	font_family			family_def;
	_BAppServerLink_	link;

	PRINT_CMD(("Setting family=%s, style=%s\n", family, style));
	
	if (family == 0L)
		family_def[0] = 0;
	else {
		strncpy(family_def, family, sizeof(font_family)-1);
		family_def[sizeof(font_family)-1] = 0;
	}		
	if (style == 0L)
		style_def[0] = 0;
	else {
		strncpy(style_def, style, sizeof(font_style)-1);
		style_def[sizeof(font_style)-1] = 0;
	}
	fake_faces = 0xffffffff;
	
	link.session->swrite_l(GR_GET_FONT_IDS);
	buffer[0] = fFamilyID;
	link.session->swrite(2, buffer);
	link.session->swrite(sizeof(font_family), (void*)&family_def[0]);
	link.session->swrite(sizeof(font_style), (void*)&style_def[0]);
	link.session->swrite(4, (void*)&fake_faces);
	link.session->flush();
	link.session->sread(6, buffer);

	if ((buffer[0] != 0xFFFF) && (buffer[1] != 0xFFFF)) {
		bool invalCachedHeight = false;
		
		if (fFamilyID != buffer[0]) {
			fFamilyID = buffer[0];
			invalCachedHeight = true;
		}

		if (fStyleID != buffer[1]) {
			fStyleID = buffer[1];
			invalCachedHeight = true;
		}
		fFace = buffer[2];

		link.session->sread(4, &fPrivateFlags); /* the direction is encoded here */ 
	
		if (invalCachedHeight)
			fHeight.ascent = fHeight.descent = fHeight.leading = kInvalidHeight;
		return B_OK;
	}
	else {
		link.session->sread(4, &fake_flags); /* the direction is encoded here */ 
		return B_NAME_NOT_FOUND;
	}
	PRINT_CMD(("FamilyID=%d, StyleID=%d\n", fFamilyID, fStyleID));
}


void
BFont::SetFamilyAndStyle(
	uint32	code)
{
	bool invalCachedHeight = false;

	uint16 newFamilyID = (code >> 16) & 0xFFFF;
	if (fFamilyID != newFamilyID) {
		fFamilyID = newFamilyID;
		invalCachedHeight = true;
	}

	uint16 newStyleID = code & 0xFFFF;
	if (fStyleID != newStyleID) {
		fStyleID = code & 0xFFFF;
		invalCachedHeight = true;
	}

	fFace = 0;
	if (invalCachedHeight) {
		fHeight.ascent = fHeight.descent = fHeight.leading = kInvalidHeight;
		fPrivateFlags = -1;
	}
}


status_t
BFont::SetFamilyAndFace(
	const font_family	family,
	uint16			face)
{
	uint16				buffer[3];
	uint32				extended_faces, fake_flags;
	font_style			style;
	font_family			family_def;
	_BAppServerLink_	link;

	PRINT_CMD(("Setting family=%s, face=0x%x\n", family, face));
	
	if (family == 0L) {
		family = family_def;
		family_def[0] = 0;
	}
	style[0] = 0;
	extended_faces = face;
	
	link.session->swrite_l(GR_GET_FONT_IDS);
	buffer[0] = fFamilyID;
	link.session->swrite(2, buffer);
	link.session->swrite(sizeof(font_family), (void*)&family[0]);
	link.session->swrite(sizeof(font_style), (void*)&style[0]);
	link.session->swrite(4, (void*)&extended_faces);
	link.session->flush();
	link.session->sread(6, buffer);

	if ((buffer[0] != 0xFFFF) && (buffer[1] != 0xFFFF)) {
		bool invalCachedHeight = false;
		
		if (fFamilyID != buffer[0]) {
			fFamilyID = buffer[0];
			invalCachedHeight = true;
		}

		if (fStyleID != buffer[1]) {
			fStyleID = buffer[1];
			invalCachedHeight = true;
		}
		fFace = buffer[2];

		link.session->sread(4, &fPrivateFlags); /* the direction is encoded here */ 
	
		if (invalCachedHeight)
			fHeight.ascent = fHeight.descent = fHeight.leading = kInvalidHeight;
		return B_OK;
	}
	else {
		link.session->sread(4, &fake_flags); /* the direction is encoded here */ 
		return B_NAME_NOT_FOUND;
	}
	PRINT_CMD(("FamilyID=%d, StyleID=%d\n", fFamilyID, fStyleID));
}


void
BFont::SetSize(
	float	size)
{
	size = (size < 0.0) ? 0.0 : size;
	size = (size > 10000.0) ? 10000.0 : size;

	if (fSize == size) 
		return;

	fSize = size;
	fHeight.ascent = fHeight.descent = fHeight.leading = kInvalidHeight;
}


void
BFont::SetShear(
	float	shear)
{
	if (shear < 45.0)
		fShear = -3.1415925635*0.25;
	else if (shear > 135.0)
		fShear = 3.1415925635*0.25;
	else
		fShear = (shear-90.0)*(3.1415925635/180.0);
}


void
BFont::SetRotation(
	float	rotation)
{
	fRotation = rotation*(3.1415925635/180.0);
}


void
BFont::SetSpacing(
	uint8	spacing)
{
	switch (spacing) {
		case B_CHAR_SPACING:
		case B_STRING_SPACING:
		case B_BITMAP_SPACING:
		case B_FIXED_SPACING:
			break;

		default:
			spacing = B_BITMAP_SPACING;
			break;
	}

	fSpacing = spacing;
}


void
BFont::SetEncoding(
	uint8	encoding)
{
	switch (encoding) {
		case B_UNICODE_UTF8:
		case B_ISO_8859_1: 
		case B_ISO_8859_2: 
		case B_ISO_8859_3:  
		case B_ISO_8859_4:  
		case B_ISO_8859_5:  
		case B_ISO_8859_6:  
		case B_ISO_8859_7:  
		case B_ISO_8859_8:  
		case B_ISO_8859_9:  
		case B_ISO_8859_10:     
		case B_MACINTOSH_ROMAN:
			break;

		default:
			encoding = B_UNICODE_UTF8;
			break; 
	}

	fEncoding = encoding;
}


void
BFont::SetFace(
	uint16	face)
{
	if ((fFace == face) || (face == 0)) 
		return;
	SetFamilyAndFace(NULL, face);
}


void
BFont::SetFlags(
	uint32	flags)
{
	fFlags = flags;
}


void
BFont::GetFamilyAndStyle(
	font_family	*family,
	font_style	*style) const
{
	uint16				buffer[2];
	font_style			style_def;
	font_family			family_def;
	_BAppServerLink_	link;

	if (family == 0L)
		family = (font_family*)&family_def[0];
	if (style == 0L)
		style = (font_style*)&style_def[0];
	link.session->swrite_l(GR_GET_FONT_NAMES);
	buffer[0] = fFamilyID;
	buffer[1] = fStyleID;
	link.session->swrite(4, buffer);
	link.session->flush();
	link.session->sread(sizeof(font_family), family); 
	link.session->sread(sizeof(font_style), style);
	PRINT_CMD(("Returning family=%s, style=%s\n", family, style));
}


uint32
BFont::FamilyAndStyle() const
{
	return ((fFamilyID << 16) | (fStyleID));
}


float
BFont::Size() const
{
	return (fSize);
}


float
BFont::Shear() const
{
	return (fShear*(180.0/3.1415925635)+90.0);
}


float
BFont::Rotation() const
{
	return fRotation*(180.0/3.1415925635);
}


uint8
BFont::Spacing() const
{
	return (fSpacing);
}


uint8
BFont::Encoding() const
{
	return (fEncoding);
}


uint16
BFont::Face() const
{
	return (fFace);
}


uint32
BFont::Flags() const
{
	return (fFlags);
}


status_t
BFont::AddOverlay(const font_overlay& overlay)
{
	if (!fExtra) {
		fExtra = new(std::nothrow) font_extra;
		if (!fExtra)
			return B_NO_MEMORY;
	}
	
	font_overlay_entry* entry =
		fExtra->overlay.AddEntry(overlay.first_char, overlay.last_char);
	if (!entry)
		return B_NO_MEMORY;
	entry->f_id = overlay.family_id;
	return B_OK;
}


status_t
BFont::FindOverlay(uint32 for_char, font_overlay* into) const
{
	if (!fExtra || !gCanUseExtra) {
		*into = font_overlay();
		return B_NAME_NOT_FOUND;
	}
	
	const font_overlay_entry* entry;
	int32 i = fExtra->overlay.Lookup(for_char, &entry,
									 &into->first_char, &into->last_char);
	if (i < 0 && !entry) {
		into->family_id = 0xffff;
		return B_NAME_NOT_FOUND;
	}
	
	into->family_id = entry->f_id;
	return B_OK;
}


status_t
BFont::RemoveOverlay(uint32 for_char)
{
	if (fExtra && gCanUseExtra && fExtra->overlay.RemoveEntry(for_char)) {
		if (fExtra->overlay.CountEntries() <= 0) {
			delete fExtra;
			fExtra = NULL;
		}
		return B_OK;
	}
	return B_NAME_NOT_FOUND;
}


status_t
BFont::RemoveOverlays(uint32 from_char, uint32 to_char)
{
	if (fExtra && gCanUseExtra && fExtra->overlay.RemoveEntries(from_char, to_char)) {
		if (fExtra->overlay.CountEntries() <= 0) {
			delete fExtra;
			fExtra = NULL;
		}
		return B_OK;
	}
	return B_NAME_NOT_FOUND;
}


void
BFont::RemoveAllOverlays()
{
	if (fExtra && gCanUseExtra) {
		delete fExtra;
		fExtra = NULL;
	}
}


int32
BFont::CountOverlays() const
{
	if (fExtra && gCanUseExtra)
		return fExtra->overlay.CountEntries();
	return 0;
}


status_t
BFont::OverlayAt(int32 index, font_overlay* into) const
{
	if (!fExtra || !gCanUseExtra) {
		*into = font_overlay();
		return B_BAD_INDEX;
	}
	
	const font_overlay_entry* entry =
		fExtra->overlay.EntryAt(index, &into->first_char, &into->last_char);
	if (!entry) {
		into->family_id = 0xffff;
		return B_BAD_INDEX;
	}
	
	into->family_id = entry->f_id;
	return B_OK;
}


font_direction
BFont::Direction() const
{
	return (B_FONT_LEFT_TO_RIGHT);
}


bool
BFont::IsFixed() const
{
	if (fPrivateFlags < 0)
		_get_flags_info_(fFamilyID, fStyleID, (int32*)&fPrivateFlags);
	return ((fPrivateFlags & PF_FIX_MODE_MASK) == PF_IS_FIXED);
}


bool
BFont::IsFullAndHalfFixed() const
{
	if (fPrivateFlags < 0)
		_get_flags_info_(fFamilyID, fStyleID, (int32*)&fPrivateFlags);
	return ((fPrivateFlags & PF_FIX_MODE_MASK) == PF_IS_HIROSHI_FIXED);
}


BRect
BFont::BoundingBox() const
{
	BRect				rect;
	uint16				buffer[2];
	_BAppServerLink_	link;

	link.session->swrite_l(GR_GET_FONT_BBOX);
	buffer[0] = fFamilyID;
	buffer[1] = fStyleID;
	link.session->swrite(4, buffer);
	link.session->flush();
	link.session->sread(sizeof(BRect), &rect);
	PRINT_CMD(("Returning bounding box: ")); /*rect.PrintToStream();*/
	return rect;
}


unicode_block
BFont::Blocks() const
{
	uint16				buffer[2];
	unicode_block		block;				
	_BAppServerLink_	link;

	link.session->swrite_l(GR_GET_FONT_BLOCKS);
	buffer[0] = fFamilyID;
	buffer[1] = fStyleID;
	link.session->swrite(4, buffer);
	link.session->flush();
	link.session->sread(sizeof(unicode_block), &block);
	PRINT_CMD(("Returning blocks: 0x016x 0x016x\n", *(uint64*)&block, *(((uint64*)&block)+1)));
	return block;
}


font_file_format
BFont::FileFormat() const
{
	if (fPrivateFlags < 0)
		_get_flags_info_(fFamilyID, fStyleID, (int32*)&fPrivateFlags);
	PRINT_CMD(("Returning file format from flags: 0x%lx\n", fPrivateFlags));
	return (font_file_format)(fPrivateFlags & PF_FILE_FORMAT_MASK);
}


int32
BFont::CountTuned() const
{
	PRINT_CMD(("Returning CountTuned()\n"));
	return _tuned_count_(fFamilyID, fStyleID);
}


void
BFont::GetTunedInfo(
	int32	        index,
	tuned_font_info *info) const
{
	PRINT_CMD(("Returning GetTunedInfo()\n"));
	_get_tuned_info_(fFamilyID, fStyleID, index, info);
}


float
BFont::StringWidth(
	const char	*string) const
{
	float result = 0.0;
	
	if (string != NULL) {
		int32 length = strlen(string);
		if (length > 0)
			GetStringWidths((const char **)&string, &length, 1, &result);
	}

	PRINT_CMD(("Returning string width %.2f for %s\n", result, string));
	return (result);
}


float
BFont::StringWidth(
	const char	*string,
	int32		length) const
{
	float result = 0.0;
	
	if ((string != NULL) && (length > 0))
		GetStringWidths((const char **)&string, &length, 1, &result);

	PRINT_CMD(("Returning N string width %.2f for %s\n", result, string));
	return (result);
}


void
BFont::GetStringWidths(
	const char	*stringArray[],
	const int32	lengthArray[],
	int32		numStrings,
	float		widthArray[]) const
{
	int					i;
	uint16				tmp16;
	_BAppServerLink_	link;

	PRINT_CMD(("Doing GetStringWidths()\n"));
	link.session->swrite_l(GR_STRING_WIDTH);
	IKAccess::WriteFont(this, link.session);
	link.session->swrite_l(numStrings);
	for (i=0; i<numStrings; i++) {
		tmp16 = lengthArray[i];
		if (tmp16 == 0)
			tmp16 = strlen(stringArray[i]);
		link.session->swrite(2, &tmp16);
		link.session->swrite(tmp16, (char *)stringArray[i]);
	}
	link.session->flush();
	link.session->sread(numStrings * sizeof(float), widthArray); 
}


void
BFont::TruncateString(
	BString*	in_out,
	uint32		mode,
	float		width) const
{
	PRINT_CMD(("Doing TruncateString()\n"));
	// This could be optimized to not require a strlen() computation
	// by re-implementing the whole server communication here...  but
	// really, why bother?
	const char* orig = in_out->String();
	GetTruncatedStrings64(&orig, 1, mode, width, in_out);
}

void
BFont::GetTruncatedStrings(
	const char	*stringArray[],
	int32		numStrings,
	uint32		mode,					   
    float		width,
	BString		stringResult[]) const
{
	PRINT_CMD(("Doing GetTruncatedStrings()\n"));
	if (numStrings < 0)
		return;
	
	while (numStrings > 64) {
		GetTruncatedStrings64(stringArray, 64, mode, width, stringResult);
		stringArray += 64;
		stringResult += 64;
		numStrings -= 64;
	}
	GetTruncatedStrings64(stringArray, numStrings, mode, width, stringResult);
}


void
BFont::GetTruncatedStrings64(
	const char	*stringArray[],
	int32		numStrings,
	uint32		mode,					   
    float		width,
	BString		stringResult[]) const
{
	int					i;
	int32               lengths[64];
	int32               total_length;
	float               wcopy;
	uint16				tmp16;
	_BAppServerLink_	link;

	if ((numStrings <= 0) || (numStrings > 64))
		return;

	total_length = 0;
	for (i=0; i<numStrings; i++)
		total_length += (lengths[i] = strlen(stringArray[i])+1);

	link.session->swrite_l(GR_TRUNCATE_STRINGS);
	IKAccess::WriteFont(this, link.session);
	wcopy = width;
	link.session->swrite(4, &wcopy);
	link.session->swrite_l(mode);
	link.session->swrite_l(numStrings);
	link.session->swrite_l(total_length);
	for (i=0; i<numStrings; i++)
		link.session->swrite(lengths[i], (char *)stringArray[i]);
	link.session->flush();

	for (i=0; i<numStrings; i++) {
		link.session->sread(2, &tmp16);
		char* buf = stringResult[i].LockBuffer(tmp16+1);
		link.session->sread(tmp16, buf);
		buf[tmp16] = 0;
		stringResult[i].UnlockBuffer(tmp16);
	}
}

void
BFont::GetTruncatedStrings(
	const char	*stringArray[],
	int32		numStrings,
	uint32		mode,					   
    float		width,
	char		*stringResult[]) const
{
	PRINT_CMD(("Doing GetTruncatedStrings()\n"));
	if (numStrings < 0)
		return;
	
	while (numStrings > 64) {
		GetTruncatedStrings64(stringArray, 64, mode, width, stringResult);
		stringArray += 64;
		stringResult += 64;
		numStrings -= 64;
	}
	GetTruncatedStrings64(stringArray, numStrings, mode, width, stringResult);
}


void
BFont::GetTruncatedStrings64(
	const char	*stringArray[],
	int32		numStrings,
	uint32		mode,					   
    float		width,
	char		*stringResult[]) const
{
	int					i;
	int32               lengths[64];
	int32               total_length;
	float               wcopy;
	uint16				tmp16;
	_BAppServerLink_	link;

	if ((numStrings <= 0) || (numStrings > 64))
		return;

	total_length = 0;
	for (i=0; i<numStrings; i++)
		total_length += (lengths[i] = strlen(stringArray[i])+1);

	link.session->swrite_l(GR_TRUNCATE_STRINGS);
	IKAccess::WriteFont(this, link.session);
	wcopy = width;
	link.session->swrite(4, &wcopy);
	link.session->swrite_l(mode);
	link.session->swrite_l(numStrings);
	link.session->swrite_l(total_length);
	for (i=0; i<numStrings; i++)
		link.session->swrite(lengths[i], (char *)stringArray[i]);
	link.session->flush();

	for (i=0; i<numStrings; i++) {
		link.session->sread(2, &tmp16);
		link.session->sread(tmp16, stringResult[i]);
		stringResult[i][tmp16] = 0;
	}
}


void
BFont::GetEscapements(
	const char	charArray[],
	int32		numChars,
	float		escapementArray[]) const
{
	escapement_delta	delta;

	PRINT_CMD(("Doing GetEscapements()\n"));
	delta.nonspace = 0.0;
	delta.space = 0.0;	
	_GetEscapements_(charArray, numChars, &delta, FC_ESCAPE_AS_FLOAT, escapementArray);
}


void
BFont::GetEscapements(
	const char			charArray[],
	int32				numChars, 
	escapement_delta	*delta,
	float				escapementArray[]) const
{
	PRINT_CMD(("Doing GetEscapements()\n"));
	_GetEscapements_(charArray, numChars, delta, FC_ESCAPE_AS_FLOAT, escapementArray);
}


void
BFont::GetEscapements(
	const char			charArray[],
	int32				numChars, 
	escapement_delta	*delta,
	BPoint				escapementArray[]) const
{
	PRINT_CMD(("Doing GetEscapements()\n"));
	_GetEscapements_(charArray, numChars, delta, FC_ESCAPE_AS_BPOINT, (float*)escapementArray);
}


void
BFont::GetEscapements(
	const char			charArray[],
	int32				numChars, 
	escapement_delta	*delta,
	BPoint				escapementArray[],
	BPoint				offsetArray[]) const
{
	PRINT_CMD(("Doing GetEscapements()\n"));
	_GetEscapements_(charArray, numChars, delta, FC_ESCAPE_AS_BPOINT_PAIR,
					 (float*)escapementArray, (float*)offsetArray);
}


void
BFont::_GetEscapements_(
	const char			charArray[],
	int32				numChars, 
	escapement_delta	*delta,
	uint8				mode,
	float				*escapements,
	float				*offsets) const
{
	int					main_loop, main_count;						
	float				*result, *junk;
	uint8				command;
	uint16				numBytes, charCount;
	uint32				junkSize;
	_BAppServerLink_	link;

	if (numChars <= 0)
		return;
	
	link.session->swrite_l(GR_GET_ESCAPEMENTS);
	IKAccess::WriteFont(this, link.session);

	if (Encoding() == B_UNICODE_UTF8) {
		/* calculate length in bytes */
		numBytes = 0;
		for (int32 i = 0; i < numChars; i++) {
			numBytes += UTF8_CHAR_LEN(charArray[numBytes]);
			if (numBytes > 32763) break;
		}
	}
	else {
		if (numChars < 32768)
			numBytes = numChars;
		else
			numBytes = 32767;
	}
	
	command = mode;

	link.session->swrite(4, (void*)&delta->nonspace);
	link.session->swrite(4, (void*)&delta->space);
	link.session->swrite(1, &command);
	link.session->swrite(2, &numBytes);
	link.session->swrite(numBytes, (void *)charArray);
	link.session->flush();
	
	if (command >= FC_ESCAPE_AS_BPOINT)
		numChars *= 2;
	
	if (command == FC_ESCAPE_AS_BPOINT_PAIR)
		main_count = 2;
	else
		main_count = 1;
		
	// number of float (double for BPoint) coming back from the app_server
	charCount = 0;
	link.session->sread(2, &charCount);

	// oops, the client didn't expect this much data from the app_server
	if (charCount > numChars) {
		junkSize = (charCount - numChars) * sizeof(float);
		junk = (float *)malloc(junkSize);
		
		for (main_loop=0; main_loop<main_count; main_loop++) {
			if (main_loop == 0)
				result = escapements;
			else
				result = offsets;
			link.session->sread((uint32)numChars * sizeof(float), result);
			link.session->sread(junkSize, junk);
		}
		free(junk);
	}
	// client expected just the right amount, or was a bit greedy
	else {
		for (main_loop=0; main_loop<main_count; main_loop++) {
			if (main_loop == 0)
				result = escapements;
			else
				result = offsets;
			if (charCount > 0)
				link.session->sread((uint32)charCount * sizeof(float), result);
			for (int32 i = charCount; i < numChars; i++)
				result[i] = 0.0;
		}
	}
}

void
BFont::GetBoundingBoxesAsGlyphs(
	const char			charArray[], 
	int32				numChars, 
	font_metric_mode	mode,
	BRect				boundingBoxArray[]) const
{
	PRINT_CMD(("Doing GetBoundingBoxesAsGlyphs()\n"));
	_GetBoundingBoxes_(charArray, numChars, mode, false, NULL, boundingBoxArray);
}


void
BFont::GetBoundingBoxesAsString(
	const char			charArray[], 
	int32				numChars, 
	font_metric_mode	mode,
	escapement_delta	*delta,
	BRect				boundingBoxArray[]) const
{
	PRINT_CMD(("Doing GetBoundingBoxesAsString()\n"));
	_GetBoundingBoxes_(charArray, numChars, mode, true, delta, boundingBoxArray);
}


void
BFont::_GetBoundingBoxes_(
	const char			charArray[], 
	int32				numChars, 
	font_metric_mode	mode,
	bool				string_escapement,
	escapement_delta	*delta,
	BRect				boundingBoxArray[]) const
{
	float				nsp, sp;
	uint8				flags[2];
	uint16				numBytes, RectCount;
	_BAppServerLink_	link;

	if (numChars <= 0)
		return;
	
	if (Encoding() == B_UNICODE_UTF8) {
		/* calculate length in bytes */
		numBytes = 0;
		for (int32 i = 0; i < numChars; i++) {
			numBytes += UTF8_CHAR_LEN(charArray[numBytes]);
			if (numBytes > 32763) break;
		}
	}
	else {
		if (numChars < 32768)
			numBytes = numChars;
		else
			numBytes = 32767;
	}

	flags[0] = (uint8)mode;
	flags[1] = (uint8)string_escapement;
	
	if (delta == NULL) {
		nsp = 0.0;
		sp = 0.0;
	}
	else {
		nsp = delta->nonspace;
		sp = delta->space;
	}
	
	link.session->swrite_l(GR_GET_GLYPHS_BBOX);
	IKAccess::WriteFont(this, link.session);
	link.session->swrite(2, flags);
	link.session->swrite(4, &nsp);
	link.session->swrite(4, &sp);
	link.session->swrite(2, &numBytes);
	link.session->swrite(numBytes, (void*)charArray);
	link.session->flush();

	RectCount = 0;
	link.session->sread(2, &RectCount);	// number of BRect from app_server

	if (RectCount > numChars) {
		uint32		junkSize;
		BRect		*junk;
	
		// oops, the client didn't expect this much data from the app_server
		link.session->sread((uint32)numChars * sizeof(BRect), boundingBoxArray);

		// purge leftover data
		junkSize = (uint32)(RectCount - numChars) * sizeof(BRect);
		junk = (BRect*)malloc(junkSize);
		link.session->sread(junkSize, junk);
		free(junk);
	}
	else {
		// client expected just the right amount, or was a bit greedy
		if (RectCount > 0)
			link.session->sread((uint32)RectCount * sizeof(BRect), boundingBoxArray);

		// reset the remaining floats (there may not be any) 
		for (int32 i = RectCount; i < numChars; i++) {
			boundingBoxArray[i].left = 0.0;
			boundingBoxArray[i].top = 0.0;
			boundingBoxArray[i].right = -1.0;
			boundingBoxArray[i].bottom = -1.0;
		}
	}
}

void
BFont::GetBoundingBoxesForStrings(
	const char			*stringArray[],
	int32				numStrings,
	font_metric_mode	mode,
	escapement_delta	deltas[],
	BRect				boundingBoxArray[]) const
{
	int					i, imin, imax, length;
	uint16				short_length;
	_BAppServerLink_	link;

	PRINT_CMD(("Doing GetBoundingBoxesForStrings()\n"));
	imin = 0;
	while (numStrings > 0) {
		if (numStrings > 64) {
			imax = imin+64;
			numStrings -= 64;
		}
		else {
			imax = imin+numStrings;
			numStrings = 0;
		}
	
		link.session->swrite_l(GR_GET_STRINGS_BBOX);
		IKAccess::WriteFont(this, link.session);
		link.session->swrite_l(mode);
		link.session->swrite_l(imax-imin);
		for (i=imin; i<imax; i++) {
			link.session->swrite(sizeof(escapement_delta), deltas+i);
			length = strlen(stringArray[i]);
			if (length > 32767)
				length = 32767;
			short_length = length;
			link.session->swrite(sizeof(uint16), &short_length);
			link.session->swrite(length, (void*)(stringArray[i]));
		}
		link.session->flush();
		link.session->sread((imax-imin)*sizeof(BRect), boundingBoxArray+imin);
		imin = imax;
	}
}

		
void
BFont::GetGlyphShapes(
	const char	charArray[],
	int32		numChars,
	BShape		*glyphShapeArray[]) const
{
	int32				i;
	int32				opCount, ptCount, opCountMax, ptCountMax;
	uint16				numBytes;
	uint32				*opList;
	BPoint				*ptList;
	_BAppServerLink_	link;

	PRINT_CMD(("Doing GetGlyphShapes()\n"));
	if (numChars <= 0)
		return;
	
	link.session->swrite_l(GR_GET_FONT_GLYPHS);
	IKAccess::WriteFont(this, link.session);

	if (Encoding() == B_UNICODE_UTF8) {
		/* calculate length in bytes */
		numBytes = 0;
		for (int32 i = 0; i < numChars; i++) {
			numBytes += UTF8_CHAR_LEN(charArray[numBytes]);
			if (numBytes > 32763) break;
		}
	}
	else {
		if (numChars < 32768)
			numBytes = numChars;
		else
			numBytes = 32767;
	}

	link.session->swrite(2, &numBytes);
	link.session->swrite(numBytes, (void *)charArray);
	link.session->flush();

	opList = 0L;
	ptList = 0L;
	opCountMax = 0;
	ptCountMax = 0;
	
	for (i=0; i<numChars; i++) {
		glyphShapeArray[i]->Clear();
		link.session->sread(4, &opCount);
		link.session->sread(4, &ptCount);
		if ((opCount != 0) && (ptCount != 0)) {
			if (opCount > opCountMax) {
				opList = (uint32*)realloc(opList, sizeof(uint32)*opCount);
				if (opList == NULL)
					return;
				opCountMax = opCount; 
			}
			if (ptCount > ptCountMax) {
				ptList = (BPoint*)realloc(ptList, sizeof(BPoint)*ptCount);
				if (ptList == NULL)
					return;
				ptCountMax = ptCount; 
			}
			link.session->sread(sizeof(uint32)*opCount, opList);
			link.session->sread(sizeof(BPoint)*ptCount, ptList);
			glyphShapeArray[i]->SetData(opCount, ptCount, opList, ptList);
		}
	}
	
	if (opCountMax > 0)
		free (opList);
	if (ptCountMax > 0)
		free (ptList);
}							   


void
BFont::GetHasGlyphs(
	const char	charArray[],
	int32		numChars,
	bool		hasArray[]) const
{
	char				*junk;
	uint16				numBytes, charCount;
	_BAppServerLink_	link;

	PRINT_CMD(("Doing GetHasGlyphs()\n"));
	if (numChars <= 0)
		return;
	
	if (Encoding() == B_UNICODE_UTF8) {
		/* calculate length in bytes */
		numBytes = 0;
		for (int32 i = 0; i < numChars; i++) {
			numBytes += UTF8_CHAR_LEN(((uchar)charArray[numBytes]));
			if (numBytes > 32763) break;
		}
	}
	else {
		if (numChars < 32768)
			numBytes = numChars;
		else
			numBytes = 32767;
	}

	link.session->swrite_l(GR_GET_HAS_GLYPHS);
	IKAccess::WriteFont(this, link.session);
	link.session->swrite(2, &numBytes);
	link.session->swrite(numBytes, (void *)charArray);
	link.session->flush();

	charCount = 0;
	link.session->sread(2, &charCount);	// number of bools from app_server

	if (charCount > numChars) {
		// oops, the client didn't expect this much data from the app_server
		link.session->sread(numChars, hasArray);

		// purge leftover data
		junk = (char*)malloc(charCount - numChars);
		link.session->sread(charCount - numChars, junk);
		free(junk);
	}
	else {
		// client expected just the right amount, or was a bit greedy
		link.session->sread(charCount, hasArray);

		// reset the remaining floats (there may not be any) 
		for (int32 i = charCount; i < numChars; i++)
			hasArray[i] = false;
	}
}									 


void
BFont::GetEdges(
	const char	charArray[],
	int32		numChars,
	edge_info	edgeArray[]) const
{
	uint16				numBytes;
	_BAppServerLink_	link;

	PRINT_CMD(("Doing GetEdges()\n"));
	if (numChars <= 0)
		return;
	
	link.session->swrite_l(GR_GET_EDGES);
	IKAccess::WriteFont(this, link.session);

	if (Encoding() == B_UNICODE_UTF8) {
		/* calculate length in bytes */
		numBytes = 0;
		for (int32 i = 0; i < numChars; i++) {
			numBytes += UTF8_CHAR_LEN(charArray[numBytes]);
			if (numBytes > 32763) break;
		}
	}
	else {
		if (numChars < 32768)
			numBytes = numChars;
		else
			numBytes = 32767;
	}

	link.session->swrite(2, &numBytes);
	link.session->swrite(numBytes, (void *)charArray);
	link.session->flush();

	uint16 charCount = 0;
	link.session->sread(2, &charCount);	// number of glyphs from app_server

	if (charCount > numChars) {
		// oops, the client didn't expect this much data from the app_server
		link.session->sread((uint32)numChars * sizeof(edge_info), edgeArray);

		// purge leftover data
		ulong	junkSize = (charCount - numChars) * sizeof(edge_info);
		float	*junk = (float *)malloc(junkSize);
		link.session->sread(junkSize, junk);
		free(junk);
	}
	else {
		// client expected just the right amount, or was a bit greedy
		if (charCount > 0)
			link.session->sread((uint32)charCount * sizeof(edge_info), edgeArray);

		// reset the remaining edges (there may not be any) 
		for (int32 i = charCount; i < numChars; i++)
			edgeArray[i].left = edgeArray[i].right = 0.0;
	}
}


void
BFont::GetHeight(
	font_height	*height) const
{
	if ( (fHeight.ascent != kInvalidHeight) && 
		 (fHeight.descent != kInvalidHeight) &&
		 (fHeight.leading != kInvalidHeight) ) {
		*height = fHeight;
		return;
	}

	_BAppServerLink_	link;

	link.session->swrite_l(GR_GET_HEIGHT);
	IKAccess::WriteFont(this, link.session);
	link.session->flush();
	link.session->sread(sizeof(font_height), (font_height *)&fHeight);

	PRINT_CMD(("Returning ascent=%.2f, descent=%.2f, leading=%.2f\n",
			fHeight.ascent, fHeight.descent, fHeight.leading));
	*height = fHeight;
}


BFont&
BFont::operator=(
	const BFont	&font)
{
	if (this != &font) {
		if (fExtra) {
			if (gCanUseExtra)
				delete fExtra;
			fExtra = NULL;
		}
		memcpy(&fFamilyID, &font.fFamilyID,
				(int32)(((char*)&((BFont*)0)->fExtra)-((char*)&((BFont*)0)->fFamilyID)));
		if (font.fExtra && gCanUseExtra)
			fExtra = new font_extra(*font.fExtra);
	}
	return (*this);
}


bool
BFont::operator==(
	const BFont	&font) const
{
	return ( (fFamilyID == font.fFamilyID) &&
			 (fStyleID == font.fStyleID) &&
			 (fSize == font.fSize) &&
			 (fShear == font.fShear) &&
			 (fRotation == font.fRotation) &&
			 (fSpacing == font.fSpacing) &&
			 (fEncoding == font.fEncoding) &&
			 (fFace == font.fFace) && 
			 (fFlags == font.fFlags) );
}


bool
BFont::operator!=(
	const BFont	&font) const
{
	return ( (fFamilyID != font.fFamilyID) ||
			 (fStyleID != font.fStyleID) ||
			 (fSize != font.fSize) ||
			 (fShear != font.fShear) ||
			 (fRotation != font.fRotation) ||
			 (fSpacing != font.fSpacing) ||
			 (fEncoding != font.fEncoding) ||
			 (fFace != font.fFace) ||
			 (fFlags != font.fFlags) );
}


int
BFont::Compare(
	const BFont	&font,
	uint32 mask) const
{
	if (mask&B_FONT_FAMILY_AND_STYLE) {
		if (fFamilyID != font.fFamilyID)
			return (fFamilyID < font.fFamilyID ? -1 : 1);
		if (fStyleID != font.fStyleID)
			return (fStyleID < font.fStyleID ? -1 : 1);
		if (fFace != font.fFace)
			return (fFace < font.fFace ? -1 : 1);
	} else if (mask&B_FONT_FACE) {
		if (fFace != font.fFace)
			return (fFace < font.fFace ? -1 : 1);
	}
	if (mask&B_FONT_SIZE) {
		if (fSize != font.fSize)
			return (fSize < font.fSize ? -1 : 1);
	}
	if (mask&B_FONT_SHEAR) {
		if (fShear != font.fShear)
			return (fShear < font.fShear ? -1 : 1);
	}
	if (mask&B_FONT_ROTATION) {
		if (fRotation != font.fRotation)
			return (fRotation < font.fRotation ? -1 : 1);
	}
	if (mask&B_FONT_SPACING) {
		if (fSpacing != font.fSpacing)
			return (fSpacing < font.fSpacing ? -1 : 1);
	}
	if (mask&B_FONT_ENCODING) {
		if (fEncoding != font.fEncoding)
			return (fEncoding < font.fEncoding ? -1 : 1);
	}
	if (mask&B_FONT_FLAGS) {
		if (fFlags != font.fFlags)
			return (fFlags < font.fFlags ? -1 : 1);
	}
	return 0;
}


void
BFont::PrintToStream() const
{
#if SUPPORTS_STREAM_IO
	BOut << *this << endl;
#endif
}

void BPrivate::IKAccess::ReadFont(BFont* font, AppSession* s)
{
	fc_context_packet ctxt;
	s->sread(sizeof(fc_context_packet), (void*)&ctxt);
	
	font->SetFamilyAndStyle((((uint32)ctxt.f_id)<<16) | ctxt.s_id);
	font->fSize = ctxt.size;
	font->fRotation = ctxt.rotation;
	font->fShear = ctxt.shear;
	font->fEncoding = ctxt.encoding;
	font->fSpacing = ctxt.spacing_mode;
	font->fFace = ctxt.faces;
	font->fFlags = ctxt.flags;
	font->fHeight.ascent = font->fHeight.descent
		= font->fHeight.leading = kInvalidHeight;
	font->fPrivateFlags = -1;
	
	if (ctxt.overlay_size > 0) {
		session_buffer buffer;
		s->sread(ctxt.overlay_size, &buffer);
		if (gCanUseExtra) {
			if (!font->fExtra)
				font->fExtra = new(std::nothrow) font_extra;
			if (font->fExtra)
				font->fExtra->overlay.Unpacketize(buffer.retrieve(), buffer.size());
		}
	} else if (font->fExtra) {
		delete font->fExtra;
		font->fExtra = NULL;
	}
}

void BPrivate::IKAccess::WriteFont(const BFont* font, AppSession* s)
{
	fc_context_packet* ctxt
		= (fc_context_packet*)s->inplace_write(sizeof(fc_context_packet));

	ctxt->f_id = font->fFamilyID;
	ctxt->s_id = font->fStyleID;
	ctxt->size = font->fSize;
	ctxt->rotation = font->fRotation;
	ctxt->shear = font->fShear;
	ctxt->encoding = font->fEncoding;
	ctxt->spacing_mode = font->fSpacing;
	ctxt->faces = font->fFace;
	ctxt->flags = font->fFlags;
	
	if (font->fExtra && gCanUseExtra &&
			(ctxt->overlay_size=font->fExtra->overlay.PacketSize()) > 0) {
		const ssize_t osize = ctxt->overlay_size;
		void* data = s->inplace_write(osize);
		if (data) {
			font->fExtra->overlay.Packetize(data, osize);
		} else {
			session_buffer buffer;
			data = buffer.reserve(osize);
			if (data)
				font->fExtra->overlay.Packetize(data, osize);
		}
	} else {
		ctxt->overlay_size = 0;
	}
}

void BPrivate::IKAccess::ReadFontPacket(BFont* font, const void* data)
{
	fc_context_packet* ctxt = (fc_context_packet*)data;
	
	font->SetFamilyAndStyle((((uint32)ctxt->f_id)<<16) | ctxt->s_id);
	font->fSize = ctxt->size;
	font->fRotation = ctxt->rotation;
	font->fShear = ctxt->shear;
	font->fEncoding = ctxt->encoding;
	font->fSpacing = ctxt->spacing_mode;
	font->fFace = ctxt->faces;
	font->fFlags = ctxt->flags;
	font->fHeight.ascent = font->fHeight.descent
		= font->fHeight.leading = kInvalidHeight;
	font->fPrivateFlags = -1;
	
	if (ctxt->overlay_size > 0) {
		if (gCanUseExtra) {
			if (!font->fExtra)
				font->fExtra = new(std::nothrow) font_extra;
			if (font->fExtra)
				font->fExtra->overlay.Unpacketize(((const uint8*)data)+ctxt->overlay_size,
													ctxt->overlay_size);
		}
	} else if (font->fExtra) {
		delete font->fExtra;
		font->fExtra = NULL;
	}
}

void BPrivate::IKAccess::WriteFontBasicPacket(const BFont* font, void* data)
{
	fc_context_packet* ctxt = (fc_context_packet*)data;

	ctxt->f_id = font->fFamilyID;
	ctxt->s_id = font->fStyleID;
	ctxt->size = font->fSize;
	ctxt->rotation = font->fRotation;
	ctxt->shear = font->fShear;
	ctxt->encoding = font->fEncoding;
	ctxt->spacing_mode = font->fSpacing;
	ctxt->faces = font->fFace;
	ctxt->flags = font->fFlags;
	ctxt->overlay_size = 0;
}

// ----------------------- BFlattenable Interface -----------------------

bool BFont::IsFixedSize() const
{
	return true;
}

type_code BFont::TypeCode() const
{
	return B_FONT_TYPE;
}

ssize_t BFont::FlattenedSize() const
{
	BFontProxy proxy(*const_cast<BFont*>(this));
	return proxy.FlattenedSize();
}

bool BFont::AllowsTypeCode(type_code code) const
{
	return code == B_FONT_TYPE;
}

status_t
BFont::Flatten(void *buffer, ssize_t size) const
{
	BFontProxy proxy(*const_cast<BFont*>(this));
	return proxy.Flatten(buffer, size);
}

status_t
BFont::Unflatten(type_code c, const void *buf, ssize_t size)
{
	BFontProxy proxy(*this);
	return proxy.Unflatten(c, buf, size, true);
}

BDataIO& operator<<(BDataIO& io, const BFont& font)
{
#if SUPPORTS_STREAM_IO
	font_family	family;
	font_style	style;
	font.GetFamilyAndStyle(&family, &style);

	font_height fontHeight;
	font.GetHeight(&fontHeight);

	io << "BFont(" << family << "/" << style << "/" << font.Size()
	   << ", shear=" << font.Shear() << ", rot=" << font.Rotation()
	   << ", height=" << fontHeight.ascent << "+" << fontHeight.descent
	   << "+" << fontHeight.leading << ")";
#else
	(void)font;
#endif
	return io;
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

static bigtime_t         _family_last_update;
static int32             _family_count;
static font_family       *_family_array;
static uchar             *_family_flags;
static int32             _style_count;
static font_style        *_style_array;
static uchar             *_style_flags;				
static uint16            *_style_faces;				
static font_family       _current_family;
static int32             _folder_count;
static font_folder_info  *_folder_array;
static int32             _file_count;
static font_file_info    *_file_array;
static sem_id            _tuned_list_sem_;
static long              _tuned_list_lock_;
static int32             _tuned_list_count_;
static tuned_font_info   *_tuned_list_;
static uint16            _cur_tuned_style_;
static uint16            _cur_tuned_family_;

/*-------------------------------------------------------------*/

void _init_global_fonts_() {	
	int              i;
	
	// init font family and style list cache
	_family_last_update = 0;
	_family_count = 0;
	_family_array = 0L;
	_family_flags = 0L;
	_style_array = 0L;
	_style_count = 0;
	_style_flags = 0L;				
	_style_faces = 0L;				
	_current_family[0] = 0;
	_folder_count = 0;
	_folder_array = 0L;
	_file_count = 0;
	_file_array = 0L;
	_tuned_list_sem_ = create_sem(0, "tuned font list sem");
	_tuned_list_lock_ = 0L;
	_tuned_list_count_ = 0;
	_tuned_list_ = 0L;
	_cur_tuned_style_ = 0xfffd;
	_cur_tuned_family_ = 0xfffd;
	
	// global BFont objects
	for (i = B_PLAIN_FONT; i <= B__NUM_FONT; i++) {
		BFont* fnt = new BFont;
		BFont::GetStandardFont((font_which)i, fnt);
		switch (i) {
		case B_PLAIN_FONT: be_plain_font = fnt; break;
		case B_BOLD_FONT: be_bold_font = fnt; break;
		case B_FIXED_FONT: be_fixed_font = fnt; break;
		case B_SYMBOL_FONT: _be_symbol_font_ = fnt; break;
		}
	}
}

/*-------------------------------------------------------------*/

static void release_folder_array() {
	int        i;

	if (_folder_array != 0) {
		for (i=0; i<_folder_count; i++)
			free(_folder_array[i].pathname);
		free(_folder_array);
		_folder_array = 0L;
	}
}

static void release_file_array() {
	int        i;

	if (_file_array != 0) {
		for (i=0; i<_file_count; i++)
			free(_file_array[i].filename);
		free(_file_array);
		_file_array = 0L;
	}
}

void _clean_global_fonts_() {
	/* free the font family and style list buffers */
	if (_family_array != 0L)
		free(_family_array);
	if (_family_flags != 0L)
		free(_family_flags);
	if (_style_array != 0L)
		free(_style_array);
	if (_style_flags != 0L)				
		free(_style_flags);
	if (_style_faces != 0L)				
		free(_style_faces);
	delete_sem(_tuned_list_sem_);
	if (_tuned_list_ != 0L)
		free(_tuned_list_);
	
	// delete the global BFont objects
	if (be_plain_font != NULL) {
		delete (be_plain_font);
		be_plain_font = NULL;
	}
	if (be_bold_font != NULL) {
		delete (be_bold_font);
		be_bold_font = NULL;
	}
	if (be_fixed_font != NULL) {
		delete (be_fixed_font);
		be_fixed_font = NULL;
	}
	if (_be_symbol_font_ != NULL) {
		delete (_be_symbol_font_);
		_be_symbol_font_ = NULL;
	}
}

/*-------------------------------------------------------------*/

static void get_family_list() {
	int32            i, count;
	_BAppServerLink_ link;

	/* free previous buffers */
	if (_family_array != 0L)
		free(_family_array);
	if (_family_flags != 0L)
		free(_family_flags);
	/* get the new ones */
	link.session->swrite_l(GR_GET_FAMILY_LIST);
	link.session->flush();
	link.session->sread(4, &count);
	_family_count = count;
	_family_array = (font_family*)malloc(sizeof(font_family)*count);
    _family_flags = (uchar*)malloc(sizeof(uchar)*count);
	for (i=0; i<count; i++) {
		link.session->sread(sizeof(font_family), _family_array+i);
		link.session->sread(1, _family_flags+i);
	}
	link.session->sread(sizeof(bigtime_t), (void*)&_family_last_update);
	/* reset the style cache */
	_current_family[0] = 0;
}

/*-------------------------------------------------------------*/

static void get_style_list(font_family family) {
	int32            i, count;
	_BAppServerLink_ link;

	/* check that this family is not already in the cache */
	if (strcmp((char*)family, _current_family) == 0)
		return;
	memcpy(&_current_family, family, sizeof(font_family));
	/* free previous buffers */
	if (_style_array != 0L)
		free(_style_array);
	if (_style_flags != 0L)
		free(_style_flags);
	if (_style_faces != 0L)
		free(_style_faces);
	/* get the new ones */
	link.session->swrite_l(GR_GET_STYLE_LIST);
	link.session->swrite(sizeof(font_family), (void*)family);
	link.session->flush();
	link.session->sread(4, &count);
	_style_count = count;
	_style_array = (font_style*)malloc(sizeof(font_style)*count);
	_style_flags = (uchar*)malloc(sizeof(uchar)*count);
	_style_faces = (uint16*)malloc(sizeof(uint16)*count);
	for (i=0; i<count; i++) {
		link.session->sread(sizeof(font_style), _style_array+i);
		link.session->sread(1, _style_flags+i);
		link.session->sread(2, _style_faces+i);
	}
}

/*-------------------------------------------------------------*/

static bool check_font_change_time() {
	bigtime_t        time;
	_BAppServerLink_ link;

	link.session->swrite_l(GR_FONT_CHANGE_TIME);
	link.session->flush();
	link.session->sread(sizeof(bigtime_t), &time);
	return (_family_last_update != time);
}

/*-------------------------------------------------------------*/

bool update_font_families(bool check_only) {
	bool         ret_val;

	ret_val = check_font_change_time();
	if (ret_val && (!check_only))
		get_family_list();
	return ret_val;
}

/*-------------------------------------------------------------*/

int32 count_font_families() {
	if (_family_array == 0L)
		get_family_list();
	return _family_count;
}

/*-------------------------------------------------------------*/

status_t get_font_family(int32 index, font_family *name, uint32 *flag) {	
	if (_family_array == 0L)
		get_family_list();
	if ((index < 0) || (index >= _family_count)) {
		(*name)[0] = 0;
		if (flag != 0)
			*flag = 0;
		return B_ERROR;
	}
	memcpy(name, _family_array+index, sizeof(font_family));
	if (flag != NULL)
		*flag = _family_flags[index];
	return B_NO_ERROR;
}

/*-------------------------------------------------------------*/

int32 count_font_styles(font_family name) {
	get_style_list(name);
	return _style_count;
}

/*-------------------------------------------------------------*/

status_t get_font_style(font_family family, int32 index, font_style *name, uint32 *flag) {
	get_style_list(family);
	if ((index < 0) || (index >= _style_count)) {
		(*name)[0] = 0;
		if (flag != NULL) *flag = 0;
		return B_ERROR;
	}
	memcpy(name, _style_array+index, sizeof(font_style));
	if (flag != NULL)
		*flag = _style_flags[index];
	return B_NO_ERROR;
}

/*-------------------------------------------------------------*/

status_t get_font_style(font_family family, int32 index, font_style *name, uint16 *face, uint32 *flag) {
	get_style_list(family);
	if ((index < 0) || (index >= _style_count)) {
		(*name)[0] = 0;
		if (face != NULL) *face = 0;
		if (flag != NULL) *flag = 0;
		return B_ERROR;
	}
	memcpy(name, _style_array+index, sizeof(font_style));
	if (face != NULL)
		*face = _style_faces[index];
	if (flag != NULL)
		*flag = _style_flags[index];
	return B_NO_ERROR;
}

/*-------------------------------------------------------------*/

status_t get_font_cache_info(uint32 id, void *set) {
	int32            size;
	_BAppServerLink_ link;

	link.session->swrite_l(GR_GET_FONT_CACHE_SETTINGS);
	link.session->swrite_l(0);
	link.session->swrite_l(id);
	link.session->flush();	

	link.session->sread(4, &size);
	if (size > 0) {
		link.session->sread(size, set);
		return B_NO_ERROR;
	}
	return B_ERROR;
}

/*-------------------------------------------------------------*/

status_t set_font_cache_info(uint32 id, void *set) {
	int32            err;
	_BAppServerLink_ link;

	link.session->swrite_l(GR_SET_FONT_CACHE_SETTINGS);
	link.session->swrite_l(0);
	link.session->swrite_l(id);
	link.session->swrite_l(sizeof(font_cache_info));
	link.session->swrite(sizeof(font_cache_info), set);
	link.session->flush();	

	link.session->sread(4, &err);
	return err;	
}

/*-------------------------------------------------------------*/

              /* PRIVATE FONT CACHE CALLS */

/*-------------------------------------------------------------*/

status_t _add_to_font_folder_list_(char *pathname) {
	return _enable_font_folder_(pathname, TRUE);
}

/*-------------------------------------------------------------*/

status_t _remove_from_font_folder_list_(char *pathname) {
	int32            err, size;
	_BAppServerLink_ link;

	size = strlen(pathname)+1;
	
	link.session->swrite_l(GR_REMOVE_FONT_DIR);
	link.session->swrite_l(size);
	link.session->swrite(size, pathname);
	link.session->flush();	

	link.session->sread(4, &err);
	return err;			
}

/*-------------------------------------------------------------*/

status_t _enable_font_folder_(char *pathname, bool enable) {
	int32            err, size;
	_BAppServerLink_ link;

	size = strlen(pathname)+1;
	
	link.session->swrite_l(GR_CHANGE_FONT_DIR_STATUS);
	link.session->swrite_l(size);
	link.session->swrite_l(enable);
	link.session->swrite(size, pathname);
	link.session->flush();	

	link.session->sread(4, &err);
	return err;			
}

/*-------------------------------------------------------------*/

status_t _enable_font_file_(int32 dir_id, char *filename, bool enable) {
	int32            err, size;
	_BAppServerLink_ link;

	size = strlen(filename)+1;
	
	link.session->swrite_l(GR_CHANGE_FONT_FILE_STATUS);
	link.session->swrite_l(size);
	link.session->swrite_l(dir_id);
	link.session->swrite_l(enable);
	link.session->swrite(size, filename);
	link.session->flush();	

	link.session->sread(4, &err);
	return err;			
}

/*-------------------------------------------------------------*/

static void get_folder_list() {
	int32            i, count, length, dir_id;
	char             *name;
	_BAppServerLink_ link;

	/* free previous buffers */
	release_folder_array();
	/* get the new ones */
	link.session->swrite_l(GR_GET_FONT_DIR_LIST);
	link.session->flush();
	link.session->sread(4, &count);
	_folder_count = count;
	_folder_array = (font_folder_info*)malloc(sizeof(font_folder_info)*count);
	for (i=0; i<count; i++) {
		link.session->sread(4, (void*)&dir_id);
		link.session->sread(4, (void*)&length);
		name = (char*)malloc(length);
		link.session->sread(length, (void*)name);
		link.session->sread(1, (void*)&_folder_array[i].status);
		link.session->sread(1, (void*)&_folder_array[i].folder_type);
		_folder_array[i].pathname = name;
		_folder_array[i].dir_id = dir_id;
	}
}

/*-------------------------------------------------------------*/

static void get_file_list() {
	int32            i, count, length;
	char             *name;
	_BAppServerLink_ link;

	/* free previous buffers */
	release_file_array();
	/* get the new ones */
	link.session->swrite_l(GR_GET_FONT_FILE_LIST);
	link.session->flush();
	link.session->sread(4, &count);
	_file_count = count;
	_file_array = (font_file_info*)malloc(sizeof(font_file_info)*count);
	for (i=0; i<count; i++) {
		link.session->sread(4, (void*)&length);
		name = (char*)malloc(length);
		link.session->sread(length, name);
		link.session->sread(1, &_file_array[i].file_type);
		link.session->sread(1, &_file_array[i].status);
		link.session->sread(2, &_file_array[i].dir_id);
		link.session->sread(sizeof(font_family), _file_array[i].family);
		link.session->sread(sizeof(font_style), _file_array[i].style);
		link.session->sread(4, &_file_array[i].size);
		_file_array[i].filename = name;
	}
}

/*-------------------------------------------------------------*/

int32 _count_font_folders_() {
	get_folder_list();
	return _folder_count;
}

/*-------------------------------------------------------------*/

status_t _get_nth_font_folder_(int32 index, font_folder_info **info) {
	if ((index >= 0) && (index < _folder_count) && (_folder_array != 0L)) {		
		*info = _folder_array+index;
		return B_NO_ERROR;
	}
	else {
		*info = 0L;
		return B_ERROR;
	}
}

/*-------------------------------------------------------------*/

int32 _count_font_files_(int32) {
	get_file_list();
	return _file_count;	
}

/*-------------------------------------------------------------*/

status_t _get_nth_font_file_(int32 index, font_file_info **info) {
	if ((index >= 0) && (index < _file_count) && (_file_array != 0L)) {		
		*info = _file_array+index;
		return B_NO_ERROR;
	}
	else {
		*info = 0L;
		return B_ERROR;
	}	
}

/*-------------------------------------------------------------*/

void _save_font_cache_state_(int32 flags) {
	_BAppServerLink_ link;

	link.session->swrite_l(GR_SAVE_FONT_CACHE);
	link.session->swrite_l(flags);
	link.session->flush();
}

/*-------------------------------------------------------------*/

void _font_control_(BFont *font, int32 cmd, void *data) {
	int32               length;
	_BAppServerLink_	link;
	
	link.session->swrite_l(GR_FONT_SET_CONTROL);
	IKAccess::WriteFont(font, link.session);
	link.session->swrite_l(cmd);
	switch (cmd) {
	case FC_CMD_CREATE_TUNED_FONT:
		link.session->flush();		
		link.session->sread(4, &length);
		link.session->sread(length, data);
		break;
	case FC_FLUSH_FONT_SET:
	case FC_CLOSE_ALL_FONT_FILES:
	case FC_RESCAN_ALL_FILES:
		link.session->flush();		
		break;
	default:
		break;
	}
}

/*-------------------------------------------------------------*/

static void get_tuned_infos(uint16 f_id, uint16 s_id) {
	uint16              buffer[2];
	_BAppServerLink_    link;
	
	if ((f_id != _cur_tuned_family_) || (s_id != _cur_tuned_style_)) {
		_cur_tuned_family_ = f_id;
		_cur_tuned_style_ = s_id;
		if (_tuned_list_ != 0L) {
			free(_tuned_list_);
			_tuned_list_ = 0L;
		}
		link.session->swrite_l(GR_GET_TUNED_FONT_LIST);
		buffer[0] = f_id;
		buffer[1] = s_id;
		link.session->swrite(4, (void*)buffer);
		link.session->flush();
		link.session->sread(4, &_tuned_list_count_);
		if (_tuned_list_count_ > 0) {
			_tuned_list_ =
				(tuned_font_info*)malloc(sizeof(tuned_font_info)*_tuned_list_count_);
			link.session->sread(sizeof(tuned_font_info)*_tuned_list_count_, _tuned_list_);
		}
	}
}

/*-------------------------------------------------------------*/

static void _get_flags_info_(uint16 f, uint16 s, int32 *flags) {
	uint16			 buffer[2];
	_BAppServerLink_ link;

	buffer[0] = f;
	buffer[1] = s;	
	link.session->swrite_l(GR_GET_FONT_FLAGS_INFO);
	link.session->swrite(4, buffer);
	link.session->flush();	
	link.session->sread(4, flags);
}

/*-------------------------------------------------------------*/

static int32 _tuned_count_(uint16 f, uint16 s) {
	int32    ret_val;
	
	if (atomic_add(&_tuned_list_lock_, 1) > 0)
		while(acquire_sem(_tuned_list_sem_) == B_INTERRUPTED)
			;

	get_tuned_infos(f, s);
	ret_val = _tuned_list_count_;
	
	if (atomic_add(&_tuned_list_lock_, -1) > 1)
		release_sem(_tuned_list_sem_);

	return ret_val;
}

/*-------------------------------------------------------------*/

static status_t _get_tuned_info_(uint16 f, uint16 s, int32 index, tuned_font_info *info) {
	status_t   err;

	if (atomic_add(&_tuned_list_lock_, 1) > 0)
		while (acquire_sem(_tuned_list_sem_) == B_INTERRUPTED)
			;

	get_tuned_infos(f, s);
	if ((index >= 0) && (index < _tuned_list_count_)) {
		memcpy(info, _tuned_list_+index, sizeof(tuned_font_info));
		err = B_NO_ERROR;
	}
	else
		err = B_ERROR;

	if (atomic_add(&_tuned_list_lock_, -1) > 1)
		release_sem(_tuned_list_sem_);

	return err;
}

/*-------------------------------------------------------------*/

static void _get_font_ids_(	const font_family family, const font_style style,
							uint16 faces, uint16 cur_fid,
							uint16* out_fid, uint16* out_sid,
							uint16* out_face, uint32* out_flags)
{
	uint32				dummy;
	font_style			style_def;
	font_family			family_def;
	_BAppServerLink_	link;

	if (family == 0L)
		family_def[0] = 0;
	else {
		strncpy(family_def, family, sizeof(font_family)-1);
		family_def[sizeof(font_family)-1] = 0;
	}		
	if (style == 0L)
		style_def[0] = 0;
	else {
		strncpy(style_def, style, sizeof(font_style)-1);
		style_def[sizeof(font_style)-1] = 0;
	}
	dummy = faces != 0xFFFF ? ((uint32)faces) : 0xFFFFFFFF;
	
	link.session->swrite_l(GR_GET_FONT_IDS);
	link.session->swrite(2, &cur_fid);
	link.session->swrite(sizeof(font_family), (void*)&family_def[0]);
	link.session->swrite(sizeof(font_style), (void*)&style_def[0]);
	link.session->swrite(4, &dummy);	// faces
	link.session->flush();
	
	link.session->sread(2, out_fid ? (void*)out_fid : (void*)&dummy);
	link.session->sread(2, out_sid ? (void*)out_sid : (void*)&dummy);
	link.session->sread(2, out_face ? (void*)out_face : (void*)&dummy);
	link.session->sread(4, out_flags ? (void*)out_flags : (void*)&dummy);
}

/*-------------------------------------------------------------*/

static void _get_font_names_(	uint16 fid, uint16 sid,
								font_family* family, font_style* style)
{
	uint16				buffer[2];
	font_style			style_def;
	font_family			family_def;
	_BAppServerLink_	link;

	if (family == 0L)
		family = (font_family*)&family_def[0];
	if (style == 0L)
		style = (font_style*)&style_def[0];
	link.session->swrite_l(GR_GET_FONT_NAMES);
	buffer[0] = fid;
	buffer[1] = sid;
	link.session->swrite(4, buffer);
	link.session->flush();
	link.session->sread(sizeof(font_family), family); 
	link.session->sread(sizeof(font_style), style); 
}

/*-------------------------------------------------------------*/

void _restore_key_map_() {
	BMessage reply;
	BMessage command(IS_SET_KEY_MAP);
	
	_control_input_server_(&command, &reply);
}

/*-------------------------------------------------------------*/

void get_key_map(key_map **map, char **key_buffer)
{
	key_map	*theMap = NULL;
	char	*theKeyBuffer = NULL;

	BMessage reply;
	BMessage command(IS_GET_KEY_MAP);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
	{
		*map = NULL;
		*key_buffer = NULL;
		return;
	}

	status_t result;
	const void *keymap = NULL;
	ssize_t keymapSize = 0;
	result = reply.FindData(IS_KEY_MAP, B_ANY_TYPE, &keymap, &keymapSize);

	if (result == B_OK)
	{
		theMap = (key_map *)malloc(keymapSize);
		if (theMap)
			memcpy(theMap, keymap, keymapSize);
	}

	const void *keybuffer = NULL;
	ssize_t keybufferSize = 0;
	result = reply.FindData(IS_KEY_BUFFER, B_ANY_TYPE, &keybuffer, &keybufferSize);		

	if (result == B_OK)
	{
		theKeyBuffer = (char *)malloc(keybufferSize);
		if (theKeyBuffer)
			memcpy(theKeyBuffer, keybuffer, keybufferSize);
	}

	*map = theMap;
	*key_buffer = theKeyBuffer;
}

/*-------------------------------------------------------------*/

void	set_modifier_key(ulong modifier, ulong key)
{
	BMessage reply;
	BMessage command(IS_SET_MODIFIER_KEY);
	command.AddInt32(IS_MODIFIER, modifier);
	command.AddInt32(IS_KEY, key);

	_control_input_server_(&command, &reply);
}
/*-------------------------------------------------------------*/

void	set_keyboard_locks(ulong modifiers)
{
	BMessage reply;
	BMessage command(IS_SET_KEYBOARD_LOCKS);
	command.AddInt32(IS_KEY_LOCKS, modifiers);

	_control_input_server_(&command, &reply);
}

/*-------------------------------------------------------------*/

ulong	modifiers()
{
	BMessage reply;
	BMessage command(IS_GET_MODIFIERS);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	return (reply.FindInt32(IS_MODIFIERS));
}

/*-------------------------------------------------------------*/

status_t	get_keyboard_id(uint16 *ID)
{
	BMessage reply;
	BMessage command(IS_GET_KEYBOARD_ID);

	status_t err = _control_input_server_(&command, &reply);
	if (err != B_NO_ERROR)
		return (err);

	return (reply.FindInt16(IS_KEYBOARD_ID, (int16 *)ID));

}
