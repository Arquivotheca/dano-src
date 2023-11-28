#ifndef SELECT_VIEW_H
#define SELECT_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif

#include <Bitmap.h>
#include "font_machine.h"

class SelectView : public BView {
		BBitmap	*the_off;
		BView	*drawer;
		long	select;
public:
				SelectView(BRect frame, char *name);
		void    Reset(bool all);
virtual	void	Draw(BRect updateRect);
virtual	void	MouseDown(BPoint where);
		void	render(BRect r);
		void    drawCell(int h, int v, uint32 code, fc_char *ch);
		void	map_select(long in_select, long *px, long *py);
		void	do_new_select(long select);
		void 	ChangeSelect(long new_one);
};

#endif








