#include "FontOverview.h"

#include <Bitmap.h>
#include <ByteOrder.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <UTF8.h>

#include <stdio.h>

const struct unicode_block_descr block_descriptions[] = {
	{ 0x0000, 0x007F, "Basic Latin" },
	{ 0x0080, 0x00FF, "Latin-1 Supplement" },
	{ 0x0100, 0x017F, "Latin Extended-A" },
	{ 0x0180, 0x024F, "Latin Extended-B" },
	{ 0x0250, 0x02AF, "IPA Extensions" },
	{ 0x02B0, 0x02FF, "Spacing Modifier Letters" },
	{ 0x0300, 0x036F, "Combining Diacritical Marks" },
	{ 0x0370, 0x03FF, "Greek" },
	{ 0x0400, 0x04FF, "Cyrillic" },
	{ 0x0530, 0x058F, "Armenian" },
	{ 0x0590, 0x05FF, "Hebrew" },
	{ 0x0600, 0x06FF, "Arabic" },
	{ 0x0700, 0x074F, "Syriac" },
	{ 0x0780, 0x07BF, "Thaana" },
	{ 0x0900, 0x097F, "Devanagari" },
	{ 0x0980, 0x09FF, "Bengali" },
	{ 0x0A00, 0x0A7F, "Gurmukhi" },
	{ 0x0A80, 0x0AFF, "Gujarati" },
	{ 0x0B00, 0x0B7F, "Oriya" },
	{ 0x0B80, 0x0BFF, "Tamil" },
	{ 0x0C00, 0x0C7F, "Telugu" },
	{ 0x0C80, 0x0CFF, "Kannada" },
	{ 0x0D00, 0x0D7F, "Malayalam" },
	{ 0x0D80, 0x0DFF, "Sinhala" },
	{ 0x0E00, 0x0E7F, "Thai" },
	{ 0x0E80, 0x0EFF, "Lao" },
	{ 0x0F00, 0x0FFF, "Tibetan" },
	{ 0x1000, 0x109F, "Myanmar" },
	{ 0x10A0, 0x10FF, "Georgian" },
	{ 0x1100, 0x11FF, "Hangul Jamo" },
	{ 0x1200, 0x137F, "Ethiopic" },
	{ 0x13A0, 0x13FF, "Cherokee" },
	{ 0x1400, 0x167F, "Unified Canadian Aboriginal Syllabics" },
	{ 0x1680, 0x169F, "Ogham" },
	{ 0x16A0, 0x16FF, "Runic" },
	{ 0x1780, 0x17FF, "Khmer" },
	{ 0x1800, 0x18AF, "Mongolian" },
	{ 0x1E00, 0x1EFF, "Latin Extended Additional" },
	{ 0x1F00, 0x1FFF, "Greek Extended" },
	{ 0x2000, 0x206F, "General Punctuation" },
	{ 0x2070, 0x209F, "Superscripts and Subscripts" },
	{ 0x20A0, 0x20CF, "Currency Symbols" },
	{ 0x20D0, 0x20FF, "Combining Marks for Symbols" },
	{ 0x2100, 0x214F, "Letterlike Symbols" },
	{ 0x2150, 0x218F, "Number Forms" },
	{ 0x2190, 0x21FF, "Arrows" },
	{ 0x2200, 0x22FF, "Mathematical Operators" },
	{ 0x2300, 0x23FF, "Miscellaneous Technical" },
	{ 0x2400, 0x243F, "Control Pictures" },
	{ 0x2440, 0x245F, "Optical Character Recognition" },
	{ 0x2460, 0x24FF, "Enclosed Alphanumerics" },
	{ 0x2500, 0x257F, "Box Drawing" },
	{ 0x2580, 0x259F, "Block Elements" },
	{ 0x25A0, 0x25FF, "Geometric Shapes" },
	{ 0x2600, 0x26FF, "Miscellaneous Symbols" },
	{ 0x2700, 0x27BF, "Dingbats" },
	{ 0x2800, 0x28FF, "Braille Patterns" },
	{ 0x2E80, 0x2EFF, "CJK Radicals Supplement" },
	{ 0x2F00, 0x2FDF, "Kangxi Radicals" },
	{ 0x2FF0, 0x2FFF, "Ideographic Description Characters" },
	{ 0x3000, 0x303F, "CJK Symbols and Punctuation" },
	{ 0x3040, 0x309F, "Hiragana" },
	{ 0x30A0, 0x30FF, "Katakana" },
	{ 0x3100, 0x312F, "Bopomofo" },
	{ 0x3130, 0x318F, "Hangul Compatibility Jamo" },
	{ 0x3190, 0x319F, "Kanbun" },
	{ 0x31A0, 0x31BF, "Bopomofo Extended" },
	{ 0x3200, 0x32FF, "Enclosed CJK Letters and Months" },
	{ 0x3300, 0x33FF, "CJK Compatibility" },
	{ 0x3400, 0x4DB5, "CJK Unified Ideographs Extension A" },
	{ 0x4E00, 0x9FFF, "CJK Unified Ideographs" },
	{ 0xA000, 0xA48F, "Yi Syllables" },
	{ 0xA490, 0xA4CF, "Yi Radicals" },
	{ 0xAC00, 0xD7A3, "Hangul Syllables" },
	{ 0xD800, 0xDB7F, "High Surrogates" },
	{ 0xDB80, 0xDBFF, "High Private Use Surrogates" },
	{ 0xDC00, 0xDFFF, "Low Surrogates" },
	{ 0xE000, 0xF8FF, "Private Use" },
	{ 0xF900, 0xFAFF, "CJK Compatibility Ideographs" },
	{ 0xFB00, 0xFB4F, "Alphabetic Presentation Forms" },
	{ 0xFB50, 0xFDFF, "Arabic Presentation Forms-A" },
	{ 0xFE20, 0xFE2F, "Combining Half Marks" },
	{ 0xFE30, 0xFE4F, "CJK Compatibility Forms" },
	{ 0xFE50, 0xFE6F, "Small Form Variants" },
	{ 0xFE70, 0xFEFE, "Arabic Presentation Forms-B" },
	{ 0xFEFF, 0xFEFF, "Specials" },
	{ 0xFF00, 0xFFEF, "Halfwidth and Fullwidth Forms" },
	{ 0xFFF0, 0xFFFD, "Specials" },
	{ 0, 0, NULL }
};

void fill_block_menu(BMenu* into, uint32 msg_code)
{
	const unicode_block_descr* blk = block_descriptions;
	while (blk && blk->name) {
		char lab[128];
		sprintf(lab, "%s (%04lx-%04lx)", blk->name, blk->first_char, blk->last_char);
		BMessage* msg = new BMessage(msg_code);
		msg->AddInt32(kFirstChar, (int32)blk->first_char);
		msg->AddInt32(kLastChar, (int32)blk->last_char);
		BMenuItem* item = new BMenuItem(lab, msg);
		into->AddItem(item);
		blk++;
	}
}

const unicode_block_descr* find_block_description(uint32 code)
{
	int32 mid, low = 0;
	int32 high = (sizeof(block_descriptions)/sizeof(block_descriptions[0]))-2;
	while (low <= high) {
		mid = (low + high)/2;
		if (code < block_descriptions[mid].first_char) {
			high = mid-1;
		} else if (code > block_descriptions[mid].last_char) {
			low = mid+1;
		} else {
			return &block_descriptions[mid];
		}
	}
	return NULL;
}

TCharacterView::TCharacterView(BRect frame, const char* name, const BFont& font,
								uint32 resizeMask, uint32 flags)
	: BView(frame, name, resizeMask, flags),
	  fMapImage(NULL)
{
	SetTargetFont(font);
}

TCharacterView::~TCharacterView()
{
	delete fMapImage;
}

void TCharacterView::SetTargetFont(const BFont& font)
{
	fFont = font;
	fFont.SetFlags(fFont.Flags()|B_DISABLE_GLOBAL_OVERLAY);
	build_character_map();
	if (Window())
		Invalidate();
}

void TCharacterView::Draw(BRect)
{
	if (fMapImage)
		DrawBitmap(fMapImage, Bounds());
}

void TCharacterView::GetPreferredSize(float *width, float *height)
{
	*width = 256;
	*height = 256;
}

void TCharacterView::build_character_map()
{
	printf("Building map for "); fFont.PrintToStream();
	
	for (int32 i=0; i<256; i++) {
		uint16 uniChars[256];
		char utf8Chars[256*3+1];
		int32 j;
		for (j=0; j<256; j++)
			uniChars[j] = B_HOST_TO_BENDIAN_INT16((i+j == 0) ? 1 : uint16(i)*256+uint16(j));
		int32 state = 0;
		int32 srcLen = 512;
		int32 destLen = sizeof(utf8Chars);
		convert_to_utf8(B_UNICODE_CONVERSION, (const char*)uniChars, &srcLen,
						utf8Chars, &destLen, &state);
		utf8Chars[destLen] = 0;
		bool hasGlyph[sizeof(utf8Chars)];
		//printf("Retrieving glyphs for: %s\n", utf8Chars);
		fFont.GetHasGlyphs(utf8Chars, sizeof(utf8Chars), hasGlyph);
		
		uint8* curMap = fMap + ((i*256)/8);
		for (j=0; j<256; j+=8) {
			uint8 val = 0;
			for (int32 k=0; k<8; k++) {
				val = (val<<1) | (hasGlyph[j+k] ? 1 :0);
				//printf("%s", hasGlyph[j+k] ? "*" : ".");
			}
			*curMap = val;
			curMap++;
		}
		//printf("\n");
	}
	
	build_character_image();
}

#define RGB_COLOR_TO_CMAP32(c) \
	B_HOST_TO_LENDIAN_INT32((c.blue)|(c.green<<8)|(c.red<<16)|(c.alpha<<24))

void TCharacterView::build_character_image()
{
	delete fMapImage;
	fMapImage = NULL;
	fMapImage = new BBitmap(BRect(0, 0, 255, 255), 0, B_RGB32);
	
	static const rgb_color kBlackColor = { 0, 0, 0, 255 };
	const uint32 kBack = RGB_COLOR_TO_CMAP32(ui_color(B_PANEL_BACKGROUND_COLOR));
	const uint32 kFore = RGB_COLOR_TO_CMAP32(kBlackColor);
	
	uint8* bits = (uint8*)fMapImage->Bits();
	int32 bpr = fMapImage->BytesPerRow();
	
	for (int32 y=0; y<256; y++) {
		uint32* pixel = (uint32*)(bits+bpr*y);
		const uint8* map = fMap + ((y*256)/8);
		for (int32 x=0; x<256; x++) {
			const int32 bit = x&7;
			*pixel++ = ((*map)&(0x80>>bit)) ? kFore : kBack;
			if (bit == 7)
				map++;
		}
	}
}
