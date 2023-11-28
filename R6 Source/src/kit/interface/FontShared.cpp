// Font implementation that is shared with the app_server.

#include <shared_fonts.h>

#include <messages.h>
#include <session.h>
#include <message_util.h>

#include <ctype.h>

namespace BPrivate {

static status_t my_font_family_name_to_id(const font_family name, uint16* out_id)
{
	uint16				buffer[3];
	uint32				extended_faces, fake_flags;
	font_style			style;
	_BAppServerLink_	link;

	style[0] = 0;
	extended_faces = B_REGULAR_FACE;
	
	link.session->swrite_l(GR_GET_FONT_IDS);
	buffer[0] = 0xffff;
	link.session->swrite(2, buffer);
	link.session->swrite(sizeof(font_family), (void*)&name[0]);
	link.session->swrite(sizeof(font_style), (void*)&style[0]);
	link.session->swrite(4, (void*)&extended_faces);
	link.session->flush();
	link.session->sread(6, buffer);
	link.session->sread(4, &fake_flags);

	if ((*out_id=buffer[0]) != 0xFFFF) {
		return B_OK;
	}
	
	return B_ERROR;
}

static status_t my_font_family_id_to_name(uint16 id, font_family* out_name)
{
	uint16				buffer[2];
	font_style			style;
	_BAppServerLink_	link;

	link.session->swrite_l(GR_GET_FONT_NAMES);
	buffer[0] = id;
	buffer[1] = 0;
	link.session->swrite(4, buffer);
	link.session->flush();
	link.session->sread(sizeof(font_family), out_name); 
	link.session->sread(sizeof(font_style), style);
	return B_OK;
}

status_t (*font_family_name_to_id)(const font_family name, uint16* out_id)
	= my_font_family_name_to_id;
status_t (*font_family_id_to_name)(uint16 id, font_family* out_name)
	= my_font_family_id_to_name;

// --------------------------------------------------------------

FontOverlayMap::FontOverlayMap()
	: fNothing(false)
{
}

FontOverlayMap::FontOverlayMap(const FontOverlayMap& o)
	: fEntries(o.fEntries), fNothing(o.fNothing)
{
}

FontOverlayMap::~FontOverlayMap()
{
}

FontOverlayMap& FontOverlayMap::operator=(const FontOverlayMap& o)
{
	fEntries = o.fEntries;
	return *this;
}

int32
FontOverlayMap::Lookup(	uint32 code, font_overlay_entry** out_entry,
							uint32* out_first, uint32* out_last)
{
	return Lookup(code, (const font_overlay_entry**)out_entry, out_first, out_last);
}

int32
FontOverlayMap::Lookup(	uint32 code, const font_overlay_entry** out_entry,
							uint32* out_first, uint32* out_last) const
{
	bool found;
	int32 i = Find(code, &found);
	if (!found) {
		// Entry doesn't exist -- range is from after previous entry
		// to before next.
		*out_entry = NULL;
		if (i <= 0) *out_first = 0;
		else *out_first = fEntries[i].high+1;
		if (i >= fEntries.CountItems()) *out_last = 0xffffffff;
		else *out_last = fEntries[i].low-1;
		return -1;
	}
	const range& r = fEntries[i];
	*out_entry = &r.value;
	*out_first = r.low;
	*out_last = r.high;
	return i;
}

int32
FontOverlayMap::CountEntries() const
{
	return fEntries.CountItems();
}

font_overlay_entry*
FontOverlayMap::EntryAt(int32 i,
							uint32* out_first, uint32* out_last)
{
	range& r = fEntries[i];
	*out_first = r.low;
	*out_last = r.high;
	return &r.value;
}

const font_overlay_entry*
FontOverlayMap::EntryAt(int32 i,
							uint32* out_first, uint32* out_last) const
{
	const range& r = fEntries[i];
	*out_first = r.low;
	*out_last = r.high;
	return &r.value;
}
		
font_overlay_entry*
FontOverlayMap::AddEntry(uint32 first, uint32 last)
{
	fNothing = false;
	
	if (first > last)
		first = last;
	
	int32 length;
	int32 i = FindRange(first, last, &length);
	if (length > 1)
		fEntries.RemoveItems(i+1, length-1);
	else if (length <= 0)
		fEntries.InsertItem(i);
	
	range& r = fEntries[i];
	r.low = first;
	r.high = last;
	return &r.value;
}

bool
FontOverlayMap::RemoveEntry(uint32 code)
{
	bool found;
	int32 i = Find(code, &found);
	if (found) {
		fEntries.RemoveItem(i);
		return true;
	}
	return false;
}

bool
FontOverlayMap::RemoveEntries(uint32 first, uint32 last)
{
	int32 length;
	int32 i = FindRange(first, last, &length);
	if (length > 0) {
		fEntries.RemoveItems(i, length);
		return true;
	}
	return false;
}

void
FontOverlayMap::SetToNothing()
{
	fEntries.MakeEmpty();
	fNothing = true;
}

bool
FontOverlayMap::IsNothing() const
{
	return fNothing;
}

void
FontOverlayMap::MakeEmpty()
{
	fEntries.MakeEmpty();
	fNothing = false;
}

ssize_t
FontOverlayMap::FlattenedSize() const
{
	return (fNothing ? 1 : CountEntries()) * sizeof(flat_font_overlay);
}

status_t
FontOverlayMap::Flatten(void *buffer, ssize_t size) const
{
	if (fNothing) {
		flat_font_overlay* item = (flat_font_overlay*)buffer;
		item->size = B_HOST_TO_LENDIAN_INT32(sizeof(*item));
		item->start = uint32(B_HOST_TO_LENDIAN_INT32(0xffffffff));
		item->end = 0;
		memset(item->family, 0, sizeof(item->family));
	} else {
		const int32 N = CountEntries();
		flat_font_overlay* item = (flat_font_overlay*)buffer;
		for (int32 i=0; i<N; i++, item++) {
			size -= sizeof(*item);
			if (size < 0) return B_ERROR;
			
			const range& r = fEntries[i];
			item->size = B_HOST_TO_LENDIAN_INT32(sizeof(*item));
			item->start = uint32(B_HOST_TO_LENDIAN_INT32(r.low));
			item->end = uint32(B_HOST_TO_LENDIAN_INT32(r.high));
			font_family_id_to_name(r.value.f_id, &item->family);
		}
	}
	
	return B_OK;
}

status_t
FontOverlayMap::Unflatten(const void *buf, ssize_t size, bool keep_invalid)
{
	MakeEmpty();
	
	while (size >= (ssize_t)sizeof(flat_font_overlay)) {
		const flat_font_overlay* item = (const flat_font_overlay*)buf;
		size -= B_LENDIAN_TO_HOST_INT32(item->size);
		buf = ((char*)buf) + B_LENDIAN_TO_HOST_INT32(item->size);
		
		const uint32 start = uint32(B_LENDIAN_TO_HOST_INT32(item->start));
		const uint32 end = uint32(B_LENDIAN_TO_HOST_INT32(item->end));
		
		if (start > end) {
			SetToNothing();
		} else {
			uint16 id;
			font_family_name_to_id(item->family, &id);
			if (id != 0xffff || keep_invalid) {
				font_overlay_entry* e = AddEntry(start, end);
				if (e) e->f_id = id;
			}
		}
	}
	return B_OK;
}

struct packet_font_overlay {
	uint32 start;
	uint32 end;
	uint16 id;
	uint16 pad;
};

ssize_t
FontOverlayMap::PacketSize() const
{
	return (fNothing ? 1 : CountEntries()) * sizeof(packet_font_overlay);
}

status_t
FontOverlayMap::Packetize(void* buffer, ssize_t size) const
{
	if (fNothing) {
		packet_font_overlay* item = (packet_font_overlay*)buffer;
		item->start = 0xffffffff;
		item->end = 0;
		item->id = 0xffff;
	} else {
		const int32 N = CountEntries();
		packet_font_overlay* item = (packet_font_overlay*)buffer;
		for (int32 i=0; i<N; i++, item++) {
			size -= sizeof(*item);
			if (size < 0) return B_ERROR;
			
			const range& r = fEntries[i];
			item->start = r.low;
			item->end = r.high;
			item->id = r.value.f_id;
		}
	}
	
	return B_OK;
}

status_t
FontOverlayMap::Unpacketize(const void* buf, ssize_t size)
{
	MakeEmpty();
	
	while (size >= (ssize_t)sizeof(packet_font_overlay)) {
		const packet_font_overlay* item = (const packet_font_overlay*)buf;
		size -= sizeof(packet_font_overlay);
		buf = ((char*)buf) + sizeof(packet_font_overlay);
		
		if (item->start > item->end) {
			SetToNothing();
		} else {
			font_overlay_entry* e = AddEntry(item->start, item->end);
			if (e) e->f_id = item->id;
		}
	}
	return B_OK;
}

int32
FontOverlayMap::Find(uint32 key, bool* found) const
{
	int32 mid, low = 0, high = fEntries.CountItems()-1;
	while (low <= high) {
		mid = (low + high)/2;
		if (key < fEntries[mid].low) {
			high = mid-1;
		} else if (key > fEntries[mid].high) {
			low = mid+1;
		} else {
			if (found) *found = true;
			return mid;
		}
	}
	
	if (found) *found = false;
	return low;
}

int32
FontOverlayMap::FindRange(uint32 low, uint32 high, int32* length) const
{
	const int32 N = fEntries.CountItems();
	const int32 first = Find(low, NULL);
	int32 i = first;
	while (i < N) {
		const range& r = fEntries[i];
		if (r.low > high)
			break;
		i++;
	}
	*length = i-first;
	return first;
}

// --------------------------------------------------------------

BAbstractFont& BAbstractFont::SetTo(const BAbstractFont &source, uint32 mask)
{
	if (mask & B_FONT_FAMILY_AND_STYLE) {
		SetFamilyAndStyle(source.FamilyAndStyle(), source.Face());
	} else if (mask & B_FONT_FACE)			SetFace(source.Face());
	if (mask & B_FONT_SIZE) 				SetSize(source.Size());
	if (mask & B_FONT_SHEAR) 				SetShear(source.Shear());
	if (mask & B_FONT_ROTATION) 			SetRotation(source.Rotation());
	if (mask & B_FONT_SPACING) 				SetSpacing(source.Spacing());
	if (mask & B_FONT_ENCODING) 			SetEncoding(source.Encoding());
	if (mask & B_FONT_FLAGS) 				SetFlags(source.Flags());
	if (mask & B_FONT_OVERLAY) {
		const FontOverlayMap* src_o = source.Overlay();
		if (src_o && (src_o->CountEntries() || src_o->IsNothing())) {
			FontOverlayMap* dst_o = EditOverlay();
			if (dst_o) *dst_o = *src_o;
		} else {
			ClearOverlay();
		}
	}
	return *this;
}

status_t BAbstractFont::ApplyCSS(const char* in_string, uint32 mask)
{
	enum font_state {
		kFaceState, kSizeState, kLineState, kFamilyState
	};
	
	font_state state = kFaceState;
	
	uint16 add_face = 0;
	uint16 rem_face = 0;
	float size = 1.0;
	bool relativeSize = true;
	
	status_t err = B_OK;
	
	printf("Parsing CSS: %s\n", in_string);
	
	while (*in_string) {
		// Position to return to, if need to restart parse of this field.
		const char* back = in_string;
		
		while (isspace(*in_string)) in_string++;
		const char* pos = in_string;
		size_t len = 0;
		if (*in_string == '"') {
			in_string++;
			pos++;
			while (*in_string && *in_string != '"') in_string++;
			len = (size_t)(in_string-pos);
			if (*in_string == '"') in_string++;
		} else if (*in_string == '\'') {
			in_string++;
			pos++;
			while (*in_string && *in_string != '\'') in_string++;
			len = (size_t)(in_string-pos);
			if (*in_string == '\'') in_string++;
		} else {
			if (state == kFamilyState) {
				// The family section is parsed differently than the others.
				// TO DO: strip white space from name.
				if (*in_string == ',') in_string++;
				while (isspace(*in_string)) in_string++;
				pos = in_string;
				while (*in_string && *in_string != ',')
					in_string++;
			} else {
				while (*in_string && !isspace(*in_string)) in_string++;
			}
			len = (size_t)(in_string-pos);
		}
		
		printf("At state %d, token=", state);
		fwrite(pos, len, 1, stdout);
		printf("\n");
		
		if (len <= 0)
			continue;
		
		switch (state) {
			case kFaceState: {
				if (strncasecmp(pos, "normal", len) == 0)		{ add_face = 0; rem_face = ~0; }
				else if (strncasecmp(pos, "bold", len) == 0)	{ add_face |= B_BOLD_FACE; rem_face &= B_BOLD_FACE; }
				else if (strncasecmp(pos, "bolder", len) == 0)	{ add_face |= B_BOLD_FACE; rem_face &= B_BOLD_FACE; }
				else if (strncasecmp(pos, "lighter", len) == 0)	{ add_face &= B_BOLD_FACE; rem_face |= B_BOLD_FACE; }
				else if (strncasecmp(pos, "100", len) == 0)		{ add_face &= B_BOLD_FACE; rem_face |= B_BOLD_FACE; }
				else if (strncasecmp(pos, "200", len) == 0)		{ add_face &= B_BOLD_FACE; rem_face |= B_BOLD_FACE; }
				else if (strncasecmp(pos, "300", len) == 0)		{ add_face &= B_BOLD_FACE; rem_face |= B_BOLD_FACE; }
				else if (strncasecmp(pos, "400", len) == 0)		{ add_face &= B_BOLD_FACE; rem_face |= B_BOLD_FACE; }
				else if (strncasecmp(pos, "500", len) == 0)		{ add_face &= B_BOLD_FACE; rem_face |= B_BOLD_FACE; }
				else if (strncasecmp(pos, "600", len) == 0)		{ add_face = B_BOLD_FACE; rem_face &= B_BOLD_FACE; }
				else if (strncasecmp(pos, "700", len) == 0)		{ add_face = B_BOLD_FACE; rem_face &= B_BOLD_FACE; }
				else if (strncasecmp(pos, "800", len) == 0)		{ add_face = B_BOLD_FACE; rem_face &= B_BOLD_FACE; }
				else if (strncasecmp(pos, "900", len) == 0)		{ add_face = B_BOLD_FACE; rem_face &= B_BOLD_FACE; }
				else if (strncasecmp(pos, "italic", len) == 0)	{ add_face = B_ITALIC_FACE; rem_face &= B_ITALIC_FACE; }
				else if (strncasecmp(pos, "oblique", len) == 0)	{ add_face = B_ITALIC_FACE; rem_face &= B_ITALIC_FACE; }
				else if (strncasecmp(pos, "small-caps", len) == 0) ; // ignore
				else {
					// Restart parsing with the next state.
					state = kSizeState;
					in_string = back;
					continue;
				}
			} break;
		
			case kSizeState:
			case kLineState: {
				// For now, ignore line height specification if it is
				// supplied.
				const char* c = pos;
				while (*c != '/' && c < in_string) c++;
				if (*c == '/')
					len = (size_t)(c-pos);
				
				float newSize = size;
				bool newRelative = relativeSize;
				
				if (strncasecmp(pos+len-2, "pt", 2) == 0) {
					// This is an absolute point size.
					newSize = atof(pos);
					newRelative = false;
				} else if (pos[len-1] == '%') {
					// This is a relative size change.
					float val = atof(pos);
					newSize = size*val/100;
				}
				else if (strncasecmp(pos, "larger", len) == 0)		newSize = size*1.5;
				else if (strncasecmp(pos, "smaller", len) == 0)		newSize = size/1.5;
				else if (strncasecmp(pos, "xx-small", len) == 0)	{ newSize = 7; newRelative = false; }
				else if (strncasecmp(pos, "x-small", len) == 0)		{ newSize = 9; newRelative = false; }
				else if (strncasecmp(pos, "small", len) == 0)		{ newSize = 10; newRelative = false; }
				else if (strncasecmp(pos, "medium", len) == 0)		{ newSize = 12; newRelative = false; }
				else if (strncasecmp(pos, "large", len) == 0)		{ newSize = 15; newRelative = false; }
				else if (strncasecmp(pos, "x-large", len) == 0)		{ newSize = 18; newRelative = false; }
				else if (strncasecmp(pos, "xx-large", len) == 0)	{ newSize = 24; newRelative = false; }
				else if (*pos == '/') state = kLineState;
				else {
					// Restart parsing with the next state.
					state = kFamilyState;
					in_string = back;
					continue;
				}
				
				if (state == kSizeState) {
					size = newSize;
					relativeSize = newRelative;
				}
				
				if (*c == '/')
					state = kFamilyState;
				else if (state == kSizeState)
					state = kLineState;
				else
					state = kFamilyState;
			} break;
			
			case kFamilyState: {
				err = B_OK;
				const uint32 localMask = mask&(B_FONT_FAMILY_AND_STYLE|B_FONT_SIZE|B_FONT_FACE);
				const uint32 curID = FamilyAndStyle();
				const uint16 curFace = Face();
				if (	strncasecmp(pos, "sans-serif", len) == 0 ||
						strncasecmp(pos, "cursive", len) == 0 ||
						strncasecmp(pos, "fantasy", len) == 0) {
					SetTo(B_PLAIN_FONT, localMask&~B_FONT_SIZE);
					SetFace(curFace);
				} else if (strncasecmp(pos, "serif", len) == 0) {
					SetTo(B_SERIF_FONT, localMask&~B_FONT_SIZE);
					SetFace(curFace);
				} else if (strncasecmp(pos, "monospace", len) == 0) {
					SetTo(B_FIXED_FONT, localMask&~B_FONT_SIZE);
					SetFace(curFace);
				} else if (strncasecmp(pos, "vnd.be.plain", len) == 0) {
					SetTo(B_PLAIN_FONT, localMask);
				} else if (strncasecmp(pos, "vnd.be.bold", len) == 0) {
					SetTo(B_BOLD_FONT, localMask);
				} else if (strncasecmp(pos, "vnd.be.fixed", len) == 0) {
					SetTo(B_FIXED_FONT, localMask);
				} else if (strncasecmp(pos, "vnd.be.serif", len) == 0) {
					SetTo(B_SERIF_FONT, localMask);
				} else {
					font_family family;
					family[0] = 0;
					strncat(family, pos, (len < sizeof(family) ? len : sizeof(family)));
					err = SetFamilyAndFace(family, curFace);
				}
				if (err == B_OK) {
					// If the requested font was found, that's it.  Skip
					// any remaining data.
					while (*in_string) in_string++;
				} else {
					// Reset to try again.
					SetFamilyAndStyle(curID, curFace);
				}
			} break;
		}
	}
	
	// Now apply the size and face changes that we had previously
	// parsed out.
	
	if (mask&B_FONT_SIZE) {
		if (relativeSize)
			SetSize(ceil(Size()*size));
		else
			SetSize(ceil(size));
	}
	
	if (mask&B_FONT_FACE) {
		uint16 face = Face();
		if (face == B_REGULAR_FACE) face = 0;
		face = (face&~rem_face)|add_face;
		if (face == 0) face = B_REGULAR_FACE;
		SetFace(face);
	}
	
	// Finally, any attributes that were asked to be changed but can
	// not be specified in CSS should revert to their default value.
	
	if (mask&B_FONT_SHEAR)
		SetShear(90);
	if (mask&B_FONT_ROTATION)
		SetRotation(0);
	if (mask&B_FONT_SPACING)
		SetSpacing(B_BITMAP_SPACING);
	if (mask&B_FONT_ENCODING)
		SetEncoding(B_UNICODE_UTF8);
	if (mask&B_FONT_FLAGS)
		SetFlags(0);

	return err;
}

ssize_t BAbstractFont::FlattenedSize() const
{
	ssize_t size = sizeof(flat_font_header);
	const FontOverlayMap* o = Overlay();
	if (o) {
		ssize_t osize = o->FlattenedSize();
		if (osize > 0) size += sizeof(flat_font_extra) + osize;
	}
	return size;
}

status_t BAbstractFont::Flatten(void *buffer, ssize_t size) const
{
	if( size < (ssize_t)sizeof(flat_font_header) ) return B_BAD_VALUE;
		
	// Easy reference to the buffer.
	flat_font_header* fdat = (flat_font_header*)buffer;
	memset(fdat,0,sizeof(*fdat));
	
	// Stash away name of family and style.
	GetFamilyAndStyle(&fdat->family,&fdat->style);
	
	// Byte-swap size, shear, and rotation into the flattened
	// structure.
	fdat->size = B_HOST_TO_LENDIAN_FLOAT(Size());
	fdat->shear = B_HOST_TO_LENDIAN_FLOAT(Shear());
	fdat->rotation = B_HOST_TO_LENDIAN_FLOAT(Rotation());
	
	// Byte-swap the remaining data into the flattened structure.
	fdat->flags = B_HOST_TO_LENDIAN_INT32(Flags());
	fdat->face = B_HOST_TO_LENDIAN_INT16(Face());
	fdat->spacing = Spacing();
	fdat->encoding = Encoding();
	fdat->mask = B_HOST_TO_LENDIAN_INT32(B_FONT_ALL);
	
	const FontOverlayMap* o = Overlay();
	if (o) {
		const int32 overlay_size = o->FlattenedSize();
		if (overlay_size > 0) {
			size -= sizeof(flat_font_header) + sizeof(flat_font_extra) + overlay_size;
			if (size >= 0) {
				flat_font_extra* xdat = (flat_font_extra*)(fdat+1);
				xdat->size = B_HOST_TO_LENDIAN_INT32(sizeof(flat_font_extra));
				xdat->overlay_size = B_HOST_TO_LENDIAN_INT32(overlay_size);
				o->Flatten(xdat+1, overlay_size);
			}
		}
	}
	
	return B_NO_ERROR;
}

status_t BAbstractFont::Unflatten(type_code c, const void *buf, ssize_t size, bool force_valid)
{
	if( c != B_FONT_TYPE ) return B_BAD_TYPE;
	
	// Make sure buffer contains at least the basic data.
	if( size < (ssize_t)sizeof(flat_font_header) ) return B_BAD_VALUE;
	
	// Easy reference to the buffer.
	const flat_font_header* fdat = (const flat_font_header*)buf;

	// Initialize from default font, just in case.
	if (!IsValid() && force_valid)
		SetTo(B_PLAIN_FONT, B_FONT_ALL);
	
	// Set up family and style for font.
	if (SetFamilyAndStyle(fdat->family,fdat->style) != B_OK) {
		SetFamilyAndFace(fdat->family,B_LENDIAN_TO_HOST_INT16(fdat->face));
	}
	
	// Byte-swap size, shear, and rotation out of the flattened
	// structure.
	SetSize(B_LENDIAN_TO_HOST_FLOAT(fdat->size));
	SetShear(B_LENDIAN_TO_HOST_FLOAT(fdat->shear));
	SetRotation(B_LENDIAN_TO_HOST_FLOAT(fdat->rotation));
	
	// Byte-swap the remaining data from the flattened structure.
	SetFlags(B_LENDIAN_TO_HOST_INT32(fdat->flags));
	SetSpacing(fdat->spacing);
	SetEncoding(fdat->encoding);
	
	size -= sizeof(flat_font_header);
	if (size > 0) {
		const flat_font_extra* xdat = (const flat_font_extra*)(fdat+1);
		const int32 xsize = B_LENDIAN_TO_HOST_INT32(xdat->size);
		const int32 osize = B_LENDIAN_TO_HOST_INT32(xdat->overlay_size);
		size -= xsize+osize;
		if (size >= 0 && osize > 0) {
			FontOverlayMap* o = EditOverlay();
			if (o) o->Unflatten(((const char*)xdat)+xsize, osize);
		} else {
			ClearOverlay();
		}
	} else {
		ClearOverlay();
	}
	
	return B_NO_ERROR;
}

}	// namespace BPrivate
