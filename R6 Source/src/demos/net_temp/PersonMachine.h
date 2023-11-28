#ifndef _WINDOW_H
#include <Window.h>
#endif
#include "person_view.h"
#include "protocol.h"

//-------------------------------------------------------------------------

class email_view : public BView {
public:
 	 				email_view (BRect r, email_find *finder);
  					~email_view();
virtual  	void	Draw(BRect r);
virtual		void	MouseDown(BPoint where);
			void 	display_person(long vp, full_email *p);
			void 	fancy_rect(BRect r);

public:
		email_find	*the_finder;
		long		v_size;
};

//-------------------------------------------------------------------------

class phone_view : public BView {
public:
 	 				phone_view (BRect r, phone_find *finder);
  					~phone_view();
virtual  	void	Draw(BRect r);
virtual		void	MouseDown(BPoint where);
			void 	display_person(long vp, full_person *p);
			void 	fancy_rect(BRect r);

public:
		phone_find	*the_finder;
		long		v_size;
};

//-------------------------------------------------------------------------

class usenet_view : public BView {
public:
 	 				usenet_view (BRect r, usenet_find *finder);
  					~usenet_view();
virtual  	void	Draw(BRect r);
virtual		void	MouseDown(BPoint where);
			void 	display_usenet(long vp, usenet_match *p);
			void 	fancy_rect(BRect r);

public:
		usenet_find	*the_finder;
		long		v_size;
};

//-------------------------------------------------------------------------

class PersonMachine : public BWindow {

public:
									PersonMachine(char *fn, char *ln, char *title);
virtual					bool		QuitRequested( void );


			phone_view	*pv;
			email_view	*ev;
			usenet_view	*news_v;
			BView		*top_view;
			char		first_name[256];
			char		last_name[256];
};

//-------------------------------------------------------------------------

class map_view : public BView {
public:
 	 				map_view (BRect r);
  					~map_view();
virtual  	void	Draw(BRect r);
virtual		void	MouseDown(BPoint where);
			void 	fancy_rect(BRect r);

public:
		BBitmap		*the_map;
		char		done;
};

//-------------------------------------------------------------------------

class MapMachine : public BWindow {

public:
						MapMachine(char *fstreet, char *fcity, char *fzip, char *fstate, char *ftitle);
virtual					bool		QuitRequested( void );

			map_view	*mv;
			BView		*top_view;
			char		street[256];
			char		city[256];
			char		zip[32];
			char		state[32];
};

//-------------------------------------------------------------------------

