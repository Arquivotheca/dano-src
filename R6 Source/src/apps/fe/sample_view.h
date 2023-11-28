#ifndef SAMPLE_VIEW_H
#define SAMPLE_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif
#include <Bitmap.h>
#include "font_machine.h"

class SampleView : public BView {
public:
		BBitmap		*the_off;
		FontMachine	*fm;
		long		x,y;
		uint16      test_string[32];
		int         cur_test;

					SampleView(BRect frame, char *name, FontMachine *fm); 
virtual	void		Draw(BRect updateRect);
		void		clear_off();
		void		plot(int h, int v, fc_char *ch);	
		void		draw_string(char *p);
		void		draw_test_string();
		void		render();
		void        add_test_char(uint16 code, bool redraw = TRUE);
};

#endif




