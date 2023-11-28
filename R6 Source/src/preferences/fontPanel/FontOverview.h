//
//	FontOverview.h
//
//	Dianne Hackborn 11/2000
//
//	Graphical view on the characters available in a font.
// 
//	(c) 2000 Be Incorporated

#ifndef FONT_OVERVIEW_H
#define FONT_OVERVIEW_H

#include <Font.h>
#include <View.h>

class BMenu;

struct unicode_block_descr {
	uint32 first_char;
	uint32 last_char;
	const char* name;
};

extern const struct unicode_block_descr block_descriptions[];

#define kFirstChar "first_char"
#define kLastChar "last_char"
void fill_block_menu(BMenu* into, uint32 msg_code);

const unicode_block_descr* find_block_description(uint32 code);

class TCharacterView : public BView {
public:
						TCharacterView(BRect frame, const char* name, const BFont& font,
										uint32 resizeMask = B_FOLLOW_LEFT|B_FOLLOW_TOP,
										uint32 flags = B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
virtual					~TCharacterView();

		void			SetTargetFont(const BFont& font);
		
virtual	void			Draw(BRect);

virtual	void			GetPreferredSize(float *width, float *height);

private:
		void			build_character_map();
		void			build_character_image();
		
		BFont			fFont;
		uint8			fMap[0x10000/8];
		BBitmap*		fMapImage;
};

#endif
