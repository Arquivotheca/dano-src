/*
	
	HelloView.h
	
	Copyright 1995 Be Incorporated, All Rights Reserved.
	
*/



#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef	_BITMAP_H
#include <Bitmap.h>
#endif
#include <stdio.h>

typedef	struct {
	float	lat1;
	float	long1;
	float	lat2;
	float	long2;
	char	code[3];
	char	type[4];
	char	name[20];
}	s_e;

//------------------------------------------------

typedef	struct {
	float	ra;
	float	dec;
	float	mag;
} star;


class HelloView : public BView {

//------------------------------------------------

#define	NS	985930

public:
		BView	*off_view;
		BBitmap	*the_off;
		float	sx,sy;
		float	dx,dy;
		star 	stars[NS];
		
		
				HelloView(BRect frame, char *name); 
virtual			~HelloView();
virtual	void	MouseDown(BPoint where);

virtual	void	AttachedToWindow();
virtual	void	Draw(BRect updateRect);
		void	LoadData();
		void	render(char *base);
		void	sort_by_dec();
		void	sub(char *base, long v);

};


#endif
