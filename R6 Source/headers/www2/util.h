#ifndef _UTIL_H
#define _UTIL_H

#include <support2/Debug.h>
#include <support2/Message.h>
#include <ctype.h>

using namespace B::Support2;

inline uint32 HashString(const char *string, uint32 hash = 0)
{
	while (*string)
		hash = (hash << 7) ^ (hash >> 24) ^ *string++;

	return hash;
}

// Case insensitive version of HashString.
inline uint32 HashStringI(const char *string, uint32 hash = 0)
{
	while (*string) {
		char c = *string++;
		if (isascii(c))
			c = tolower(c);

		hash = (hash << 7) ^ (hash >> 24) ^ c;
	}
	
	return hash;
}

inline void bindump(char *data, int size)
{
	int lineoffs = 0;
	while (size > 0) {
		printf("\n%04x  ", lineoffs);
		for (int offs = 0; offs < 16; offs++) {
			if (offs < size)
				printf("%02x ", (uchar)data[offs]);
			else
				printf("   ");
		}
			
		printf("     ");
		for (int offs = 0; offs < MIN(size, 16); offs++)
			printf("%c", (data[offs] > 31 && (uchar) data[offs] < 128) ? data[offs] : '.');

		data += 16;
		size -= 16;
		lineoffs += 16;
	}

	printf("\n");
}

inline char *append(char *start, const char *string)
{
	char *end = start;
	while (*string)
		*end++ = *string++;

	return end;
}

inline char *get_nth_string(char *buf, int index)
{
	char *c = buf;
	while (index-- > 0)
		while (*c++ != '\0')
			;
	
	return c;
}

inline char *append_decimal(char *start, int dec)
{
	char buf[11];
	buf[10] = '\0';
	char *c = &buf[10];
	while (dec > 0) {
		*--c = (dec % 10) + '0';
		dec /= 10;
	}

	return append(start, c);
}

// Decode an HTML color string -- #rrggbb.
// Returns B_TRANSPARENT_COLOR if 'str' is not in correct format.
//rgb_color decode_color(const char* str);
//void rgb_color_to_html(BStringBuffer &outHex, const rgb_color &inColor);
//void rgb_color_to_html(char *str, const rgb_color &inColor);

// Return a color from an encoded string in a message.
//status_t find_color(const BMessage* from, const char* name, rgb_color* out);

// Create a BFont from a CSS font description.  The resulting font
// is derived from 'orig_font'; if orig_font is NULL, then be_plain_font
// is used.
// This is a subset of CSS -- it implements all of the syntax except for
// length units in font sizes (em, ex, etc).  The line height value is
// parsed but not used.

//status_t decode_font(const char* in_string, BFont* out_font,
//					 const BFont* orig_font = NULL);
		
// Transform 'buffer' so that it can be displayed as-is in
// in HTML page.  Maybe this should be moved into BString?

const char* escape_for_html(BString* buffer,
							const char* initial=0, bool always_set_buffer=true);

#endif
