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

class HelloView : public BView {

//------------------------------------------------


public:
		s_e		data[86618];
		float	length[86618];
		long	*arrays[100][100];
		long	cnts[100][100];
		BView	*off_view;
		BBitmap	*the_off;
		float	px0;
		float	py0;
		float	s;
		
		
				HelloView(BRect frame, char *name); 
virtual			~HelloView();
virtual	void	MouseDown(BPoint where);

virtual	void	AttachedToWindow();
virtual	void	Draw(BRect updateRect);
		void	LoadData();
		void 	AddNames();

};


#endif
