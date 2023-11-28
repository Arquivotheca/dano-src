#include "util.h"

#include <Font.h>
#include <String.h>
#include <StringBuffer.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

struct html_color {
	const char* name;
	rgb_color color;
};

static html_color html_colors[] = {
	{ "black",		{ 0x00, 0x00, 0x00, 0xff } },
	{ "silver",		{ 0xc0, 0xc0, 0xc0, 0xff } },
	{ "gray",		{ 0x80, 0x80, 0x80, 0xff } },
	{ "white",		{ 0xff, 0xff, 0xff, 0xff } },
	{ "maroon",		{ 0x80, 0x00, 0x00, 0xff } },
	{ "red",		{ 0xff, 0x00, 0x00, 0xff } },
	{ "purple",		{ 0x80, 0x00, 0x80, 0xff } },
	{ "fuschsia",	{ 0xff, 0x00, 0xff, 0xff } },
	{ "green",		{ 0x00, 0x80, 0x00, 0xff } },
	{ "lime",		{ 0x00, 0xff, 0x00, 0xff } },
	{ "olive",		{ 0x80, 0x80, 0x00, 0xff } },
	{ "yellow",		{ 0xff, 0xff, 0x00, 0xff } },
	{ "navy",		{ 0x00, 0x00, 0x80, 0xff } },
	{ "blue",		{ 0x00, 0x00, 0xff, 0xff } },
	{ "teal",		{ 0x00, 0x80, 0x80, 0xff } },
	{ "aqua",		{ 0x00, 0xff, 0xff, 0xff } },
	{ 0, { 0, 0, 0, 0 } }
};

rgb_color decode_color(const char* str)
{
	if( *str != '#' ) {
		html_color* map = html_colors;
		while( map->name ) {
			if( strcasecmp(map->name, str) == 0 ) return map->color;
			map++;
		}
		return B_TRANSPARENT_COLOR;
	}
	
	str++;
	
	rgb_color color = { 0, 0, 0, 255 };
	uint8* gun = &color.red;
	for( int i=0; i<6; i++ ) {
		uint8 val = 0;
		if( *str >= '0' && *str <= '9' ) val = *str - '0';
		else if( *str >= 'a' && *str <= 'f' ) val = *str - 'a' + 10;
		else if( *str >= 'A' && *str <= 'F' ) val = *str - 'A' + 10;
		else return B_TRANSPARENT_COLOR;
		*gun = ((*gun) << 4) | val;
		str++;
		if( (i%2) == 1 ) gun++;
	}
	
	return color;
}

inline char HexDigit(int i)
{
	if (i <= 9)
		return i + '0';

	return i + ('A' - 10);
}

void rgb_color_to_html(StringBuffer &outHex, const rgb_color &inColor)
{
	outHex << HexDigit(inColor.red >> 4);
	outHex << HexDigit(inColor.red & 0xf);
	outHex << HexDigit(inColor.green >> 4);
	outHex << HexDigit(inColor.green & 0xf);
	outHex << HexDigit(inColor.blue >> 4);
	outHex << HexDigit(inColor.blue & 0xf);
}

void rgb_color_to_html(char *str, const rgb_color &inColor)
{
	str[0] = HexDigit(inColor.red >> 4);
	str[1] = HexDigit(inColor.red & 0xf);
	str[2] = HexDigit(inColor.green >> 4);
	str[3] = HexDigit(inColor.green & 0xf);
	str[4] = HexDigit(inColor.blue >> 4);
	str[5] = HexDigit(inColor.blue & 0xf);
	str[6] = 0;
}

status_t find_color(const BMessage* from, const char* name, rgb_color* out)
{
	const char* str;
	status_t err = from->FindString(name, &str);
	if( err != B_OK ) return err;
	*out = decode_color(str);
	if( out->alpha == 0 ) return B_ERROR;
	return B_OK;
}

// -------------------------------------------------------------------------

status_t decode_font(const char* in_string, BFont* out_font, const BFont* orig_font)
{
	enum font_state {
		kFaceState, kSizeState, kLineState, kFamilyState
	};
	
	font_state state = kFaceState;
	
	uint16 face = 0;
	float size = 0;
	font_family family;
	font_style style;		// not used
	
	if (!orig_font)
		orig_font = be_plain_font;
	
	*out_font = *orig_font;
	face = orig_font->Face() & ~B_REGULAR_FACE;
	size = orig_font->Size();
	orig_font->GetFamilyAndStyle(&family, &style);
	
	status_t err = B_OK;
	
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
		if (len <= 0)
			continue;
		
		switch (state) {
			case kFaceState: {
				if (strncasecmp(pos, "normal", len) == 0)		face = 0;
				else if (strncasecmp(pos, "bold", len) == 0)	face |= B_BOLD_FACE;
				else if (strncasecmp(pos, "bolder", len) == 0)	face |= B_BOLD_FACE;
				else if (strncasecmp(pos, "lighter", len) == 0)	face &= ~B_BOLD_FACE;
				else if (strncasecmp(pos, "100", len) == 0)		face &= ~B_BOLD_FACE;
				else if (strncasecmp(pos, "200", len) == 0)		face &= ~B_BOLD_FACE;
				else if (strncasecmp(pos, "300", len) == 0)		face &= ~B_BOLD_FACE;
				else if (strncasecmp(pos, "400", len) == 0)		face &= ~B_BOLD_FACE;
				else if (strncasecmp(pos, "500", len) == 0)		face &= ~B_BOLD_FACE;
				else if (strncasecmp(pos, "600", len) == 0)		face |= B_BOLD_FACE;
				else if (strncasecmp(pos, "700", len) == 0)		face |= B_BOLD_FACE;
				else if (strncasecmp(pos, "800", len) == 0)		face |= B_BOLD_FACE;
				else if (strncasecmp(pos, "900", len) == 0)		face |= B_BOLD_FACE;
				else if (strncasecmp(pos, "italic", len) == 0)	face |= B_ITALIC_FACE;
				else if (strncasecmp(pos, "oblique", len) == 0)	face |= B_ITALIC_FACE;
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
				
				if (strncasecmp(pos+len-2, "pt", 2) == 0) {
					// This is an absolute point size.
					newSize = ceil(atof(pos));
				} else if (pos[len-1] == '%') {
					// This is a relative size change.
					float val = atof(pos);
					newSize = ceil(size*val/100);
				}
				else if (strncasecmp(pos, "larger", len) == 0)		newSize = ceil(size*1.5);
				else if (strncasecmp(pos, "smaller", len) == 0)		newSize = ceil(size/1.5);
				else if (strncasecmp(pos, "xx-small", len) == 0)	newSize = 7;
				else if (strncasecmp(pos, "x-small", len) == 0)		newSize = 9;
				else if (strncasecmp(pos, "small", len) == 0)		newSize = 10;
				else if (strncasecmp(pos, "medium", len) == 0)		newSize = 12;
				else if (strncasecmp(pos, "large", len) == 0)		newSize = 15;
				else if (strncasecmp(pos, "x-large", len) == 0)		newSize = 18;
				else if (strncasecmp(pos, "xx-large", len) == 0)	newSize = 24;
				else if (*pos == '/') state = kLineState;
				else {
					// Restart parsing with the next state.
					state = kFamilyState;
					in_string = back;
					continue;
				}
				
				if (state == kSizeState)
					size = newSize;
				
				if (*c == '/')
					state = kFamilyState;
				else if (state == kSizeState)
					state = kLineState;
				else
					state = kFamilyState;
			} break;
			
			case kFamilyState: {
				if (	strncasecmp(pos, "serif", len) == 0 ||
						strncasecmp(pos, "sans-serif", len) == 0 ||
						strncasecmp(pos, "cursive", len) == 0 ||
						strncasecmp(pos, "fantasy", len) == 0) {
					be_plain_font->GetFamilyAndStyle(&family, &style);
				} else if (strncasecmp(pos, "monospace", len) == 0) {
					be_fixed_font->GetFamilyAndStyle(&family, &style);
				} else {
					family[0] = 0;
					strncat(family, pos, (len < sizeof(family) ? len : sizeof(family)));
				}
				err = out_font->SetFamilyAndFace(family, B_REGULAR_FACE);
				if (err == B_OK) {
					// Found a font, skip remaining data.
					while (*in_string) in_string++;
				} else {
					// Reset to try again.
					*out_font = *orig_font;
				}
			} break;
		}
	}
	
	if (face == 0) face = B_REGULAR_FACE;
	out_font->SetFace(face);
	
	out_font->SetSize(size);
	
	return B_OK;
}

// -------------------------------------------------------------------------

const char* escape_for_html(BString* string,
							const char* initial, bool always_set_buffer)
{
	BString buffer;
	const char* original = initial ? initial : string->String();
	BString* buf = initial ? string : &buffer;
	
	for (int32 i=0; i<2; i++) {
	
		if (i) *buf = "";
		
		const char* c = original;
		const char* f = c;
		bool changes = false;
		
		while (*c && !changes) {
			const char* replace = 0;
			switch (*c) {
				case '&':	replace = "&amp;";	break;
				case '<':	replace = "&lt;";	break;
				case '>':	replace = "&gt;";	break;
				case '"':	replace = "&quot;";	break;
				case '\'':	replace = "&#x27;";	break;
				case '\r':	replace = "";		break;
				case '\n':	replace = "\r\n";	break;
				case ' ': {
					// ack, work around Opera bug with PRE lines ending in spaces.
					const char* e = c;
					while (*e == ' ') e++;
					if (*e != '\r' && *e != '\n' && *e != 0) {
						// line continues after spaces, copy as-is.
						c = e;
					} else {
						// spaces are at end of line, skip them.
						if (i) buf->Append(f, size_t(c-f));
						else changes = true;
						f = c = e;
					}
					break;
				}
				default:	c++;				break;
			}
			
			if (replace) {
				if (i) {
					buf->Append(f, size_t(c-f));
					buf->Append(replace);
				} else changes = true;
				c++;
				f = c;
			}
		}
		
		if (!i && !changes) {
			// if this is the first time through and nothing
			// needs to be changed, that's all.
			if (always_set_buffer && initial) *string = initial;
			return original;
		} else if (i) {
			// if this is the second time through, write any
			// final characters.
			buf->Append(f, size_t(c-f));
		}
	}
	
	if (buf != string) *string = *buf;
	return string->String();
}
