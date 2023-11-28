#ifndef EDIT_WINDOW_H
#define EDIT_WINDOW_H

#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef	_BITMAP_H
#include <Bitmap.h>
#endif
#include <Window.h>

#include <stdio.h>

//------------------------------------------------
class	EditView;
//------------------------------------------------

class EditWindow : public BWindow {

friend	EditView;

		float		cur_zoom;
		float		cur_time;
		BMenuItem	*undo_itm;
public:
				EditWindow(BRect, const char*, SampleCache *the_cache);
				~EditWindow();
virtual	void	HandleZoom(float v);
virtual	void	new_h(float t);
		void	MessageReceived(BMessage *b);
};

//-------------------------------------------------------------------------

class EditView : public BView {
public:
 				 EditView (BRect r, char *name, SampleCache *the_cache);
  				~EditView();
		void	MouseDown(BPoint where);
		void	Draw(BRect r);
		void	Render();
		void	vl(long x1,long y1,long y2, uchar c);
		void	set_param(float t, float zo);
		void	NewSelection(float s, float e, char draw);
		long	time_to_h(float t);
		void	DoMessage(BMessage *b);


		void	do_zero(float start, float end);
		void	do_echo(float start, float end);
		void	do_chorus(float start, float end);
		void	do_fade(float start, float end);
		void	do_metal(float start, float end);
		void	do_maxvolume(float start, float end);
		void	do_compress(float start, float end);
		void	do_volume(float start, float end);
		void	do_clear(float start, float end);
		void	do_cut(float start, float end);
		void	do_copy(float start, float end);
		void	do_paste(float start, float end);
		void	SaveUndo();
		void	Undo();
private:

	char		undo_state;
	long		undo_length;
	SampleCache	*cache;
	short		*undo;
	BBitmap		*b;
	BView		*view;
	long		last_h_size;
	long		last_v_size;
	float		zoom;
	float		cur_time;
	float		select_left;
	float		select_right;
	float		last_z;
	float		last_ct;
	float		last_sl;
	float		last_sr;
	long		last_vs;
	float		g_intens;
	float		g_delay;
	char		dirty;
};

//------------------------------------------------


#endif
