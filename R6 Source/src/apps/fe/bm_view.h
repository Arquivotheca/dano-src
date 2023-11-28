#ifndef BM_VIEW_H
#define BM_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif

#include <Bitmap.h>
#include "font_machine.h"

#define	NN	24

#define convert_to_utf8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = *uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|((*uni_str++)&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|((*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}	

class BMView : public BView {
 public:
	BBitmap		*the_off, *witness;
	BView		*drawer, *witness_drawer;
	FontMachine	*fm;
	uchar		data[NN][NN];
	uchar		undo_block[NN][NN];
	uchar       select[NN][NN];
	uchar       select_copy[NN][NN];
	uchar       undo_select[NN][NN];
	fc_char     char_header;
	fc_char     undo_header;
	uint32		cur_char;
	
 public:
					BMView(BRect frame, char *name);
					~BMView();
	    void        Reset();
	    void        ResetSelection();
	    void        PasteSelection();
virtual	void		AttachedToWindow();
virtual	void		Draw(BRect updateRect);
virtual	void		MouseDown(BPoint where);
	    void        ConvertToBitmap(fc_char *ch, uchar *data, fc_char *header);
	    void        PutBackChar();
	    void		RenderWitness(int32 index_char);
	    void		render(BRect r, BBitmap *the_off, bool background_only = false);
	    void        drawBottomRule(BRect r);
	    void        drawRightRule(BRect r);
		void		track_vertical(BPoint where);
		void		track_horizontal(BPoint where);
	    void        invert_select(int hmin, int hmax, int vmin, int vmax);
	    void        do_selection(BPoint where);
	    void        offset_selection(int dh, int dv);
		void		change_select(long new_one);
virtual	void 		KeyDown(const char *bytes, int32 numBytes);
		void 		Copy();
		void 		Cut();
		void 		Paste();
		void 		InvertH();
		void 		InvertV();
		void 		Save();
	    void        RefreshChange();
	    void		do_undo();
		void		do_reverse();
};

#endif
