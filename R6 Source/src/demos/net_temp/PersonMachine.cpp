#include "PersonMachine.h"
#include <FindDirectory.h>
#include <Path.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Debug.h>
#include <Application.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>


//-------------------------------------------------------------------------

void usenet_view::fancy_rect(BRect r)
{
	SetHighColor(250, 250, 250);
	StrokeRect(r);
	r.top--;
	r.left--;
	r.bottom--;
	r.right--;
	SetHighColor(140, 140, 140);
	StrokeRect(r);
}

//-------------------------------------------------------------------------

	usenet_view::usenet_view(BRect r, usenet_find *finder)
 	: BView (r, "usenet_view", B_FOLLOW_NONE, B_WILL_DRAW)
{
	the_finder = finder;
	v_size = 80;
}

//-------------------------------------------------------------------------

	usenet_view::~usenet_view()
{
	delete the_finder;
}

//-------------------------------------------------------------------------

void usenet_view::MouseDown(BPoint where)
{
}

//-------------------------------------------------------------------------

void usenet_view::display_usenet(long vp, usenet_match *p)
{

	MovePenTo(BPoint(20, vp));
	SetHighColor(255, 0, 0);
	DrawString(p->newsgroup);
}	


//-------------------------------------------------------------------------

void usenet_view::Draw(BRect r)
{
	long	cnt;
	long	i;
	long	vp;
	
	SetDrawingMode(B_OP_OVER);
	if (the_finder == 0) {
		SetFontSize(14);
		MovePenTo(BPoint(199+25, 42));
		fancy_rect(BRect(7, 5, 596+50, 70)); 
		DrawString("    Usenet:Searching");
		return;
	}	
	cnt = the_finder->newsgroup_count();

	if (cnt <= 0) {
		SetFontSize(14);
		MovePenTo(BPoint(199+25, 42));
		fancy_rect(BRect(7, 5, 596+50, 70)); 
		DrawString("    Usenet:No Data Found !");
		return;
	}
	SetFontSize(10);

	for (i = 0; i < cnt; i++) {
		display_usenet(i * 20 + 20, the_finder->get_ind_newsgroup(i));
	}
	
	vp = cnt * 20 + 14;
	
	fancy_rect(BRect(0, 0, 3000, vp)); 
}


//-------------------------------------------------------------------------

void email_view::fancy_rect(BRect r)
{
	SetHighColor(250, 250, 250);
	StrokeRect(r);
	r.top--;
	r.left--;
	r.bottom--;
	r.right--;
	SetHighColor(140, 140, 140);
	StrokeRect(r);
}

//-------------------------------------------------------------------------

	email_view::email_view(BRect r, email_find *finder)
 	: BView (r, "email_view", B_FOLLOW_NONE, B_WILL_DRAW)
{
	the_finder = finder;
	v_size = r.bottom - r.top;
}

//-------------------------------------------------------------------------

	email_view::~email_view()
{
	delete the_finder;
}

//-------------------------------------------------------------------------

void email_view::MouseDown(BPoint where)
{
}

//-------------------------------------------------------------------------

void email_view::display_person(long vp, full_email *p)
{

	MovePenTo(BPoint(20, vp));
	SetHighColor(255, 0, 0);
	DrawString(p->first_name);
	MovePenBy(4, 0);
	DrawString(p->last_name);
	
	MovePenTo(BPoint(180, vp));
	SetHighColor(0, 0, 255);
	DrawString(p->email);
}	


//-------------------------------------------------------------------------

void email_view::Draw(BRect r)
{
	long	cnt;
	long	i;
	long	vp;
	
	SetDrawingMode(B_OP_OVER);
	if (the_finder == 0) {
		SetFontSize(14);
		MovePenTo(BPoint(199+25, 42));
		fancy_rect(BRect(7, 5, 596+50, 70)); 
		DrawString("    E-Mail:Searching");
		return;
	}	
	cnt = the_finder->email_count();

	if (cnt <= 0) {
		SetFontSize(14);
		MovePenTo(BPoint(199+25, 42));
		fancy_rect(BRect(7, 5, 596+50, 70)); 
		DrawString("    E-Mail:No Data Found !");
		return;
	}
	SetFontSize(10);

	for (i = 0; i < cnt; i++) {
		display_person(i * 20 + 20, the_finder->get_ind_email(i));
	}
	
	vp = cnt * 20 + 14;
	
	fancy_rect(BRect(0, 0, 173, vp)); 
	fancy_rect(BRect(173, 0, 3000, vp)); 
}


//-------------------------------------------------------------------------

	phone_view::phone_view(BRect r, phone_find *finder)
 	: BView (r, "phone_view", B_FOLLOW_NONE, B_WILL_DRAW)
{
	the_finder = finder;
	v_size = r.bottom - r.top;
}

//-------------------------------------------------------------------------

	phone_view::~phone_view()
{
	delete the_finder;
}

//-------------------------------------------------------------------------

void phone_view::MouseDown(BPoint where)
{
	long			v;
	full_person		*p;
	char			buf[256];
	MapMachine		*w;
	
	if (the_finder ==  0)
		return;
		
	if (where.x < 230)
		return;
		
	v = where.y / 20;
	
	if (v >= the_finder->person_count())
		return;
	
	p = the_finder->get_ind_person(v);
	
	sprintf(buf, "%s %s", p->address, p->city);
	w = new MapMachine(p->address, p->city, p->zip, p->state, buf);

	w->Lock();
	w->Show();
	w->Unlock();
}

//-------------------------------------------------------------------------

void phone_view::display_person(long vp, full_person *p)
{

	MovePenTo(BPoint(20, vp));
	SetHighColor(255, 0, 0);
	DrawString(p->first_name);
	MovePenBy(4, 0);
	DrawString(p->last_name);
	
	MovePenTo(BPoint(180, vp));
	SetHighColor(0, 0, 255);
	DrawString(p->phone);
	

	MovePenTo(BPoint(280, vp));
	DrawString(p->address);
	SetHighColor(0,0,255);
	MovePenTo(BPoint(280, vp+3));
	StrokeLine(BPoint(280 + StringWidth(p->address), vp+3));
	MovePenTo(BPoint(460, vp));
	DrawString(p->zip);
	MovePenTo(BPoint(460, vp+3));
	StrokeLine(BPoint(460 + StringWidth(p->zip) + StringWidth(p->city)+ StringWidth(p->state) + 10, vp+3));

	MovePenTo(BPoint(460 + StringWidth(p->zip) , vp));
	MovePenBy(5, 0);
	DrawString(p->city);
	MovePenBy(5, 0);
	DrawString(p->state);
}	

//-------------------------------------------------------------------------

void phone_view::fancy_rect(BRect r)
{
	SetHighColor(250, 250, 250);
	StrokeRect(r);
	r.top--;
	r.left--;
	r.bottom--;
	r.right--;
	SetHighColor(140, 140, 140);
	StrokeRect(r);
}

//-------------------------------------------------------------------------

void phone_view::Draw(BRect r)
{
	long	cnt;
	long	i;
	long	vp;
	
	SetDrawingMode(B_OP_OVER);
	if (the_finder == 0) {
		SetFontSize(14);
		fancy_rect(BRect(7, 5, 596+50, 70)); 
		MovePenTo(BPoint(199+25, 42));
		DrawString("Phone Numbers:Searching");
		return;
	}	
	cnt = the_finder->person_count();

	if (cnt <= 0) {
		SetFontSize(14);
		fancy_rect(BRect(7, 5, 596+50, 70)); 
		MovePenTo(BPoint(199+25, 42));
		DrawString("Phone Numbers:No Data Found !");
		return;
	}
	SetFontSize(10);

	for (i = 0; i < cnt; i++) {
		display_person(i * 20 + 20, the_finder->get_ind_person(i));
	}
	
	vp = cnt * 20 + 14;
	
	fancy_rect(BRect(0, 0, 173, vp)); 
	fancy_rect(BRect(173, 0, 267, vp)); 
	fancy_rect(BRect(267, 0, 447, vp)); 
	fancy_rect(BRect(447, 0, 1000, vp)); 
}

//-------------------------------------------------------------------------

long	email_searcher(void *p)
{
	PersonMachine	*w;
	email_find		*finder;
	long			cnt;
	long			vs;
	
	w = (PersonMachine*)p;
	
	finder = new email_find();
	
	finder->doit(w->first_name, w->last_name);
	
	if (w->Lock()) {
		w->ev->the_finder = finder;
		
		if (finder->email_count() <= 0) {
			vs = 80;
		}
		else
			vs = finder->email_count() * 20 + 20;
		w->ev->v_size = vs;
	
			
		w->ev->ResizeTo(1000, vs);
		w->ev->Invalidate();
		w->news_v->MoveTo(0, w->ev->v_size + w->pv->v_size - 6);
		w->ResizeTo(650,w->ev->v_size + w->pv->v_size + w->news_v->v_size - 8);
		w->Unlock();
	}
}

//-------------------------------------------------------------------------

long	person_searcher(void *p)
{
	PersonMachine	*w;
	phone_find		*finder;
	long			cnt;
	long			vs;
	
	w = (PersonMachine*)p;
	
	finder = new phone_find();
	
	finder->doit(w->first_name, w->last_name);
	
	if (w->Lock()) {
		w->pv->the_finder = finder;
		
		if (finder->person_count() <= 0) {
			vs = 80;
		}
		else
			vs = finder->person_count() * 20 + 14;
		w->pv->v_size = vs;
			
		w->pv->ResizeTo(1000, vs);
		w->ev->MoveTo(0, vs);
		w->pv->Invalidate();
		w->news_v->MoveTo(0, w->ev->v_size + w->pv->v_size - 6);
		w->ResizeTo(650,w->ev->v_size + w->pv->v_size + w->news_v->v_size - 8);
		w->Unlock();
	}
	
}

//-------------------------------------------------------------------------

long	usenet_searcher(void *p)
{
	PersonMachine	*w;
	usenet_find		*finder;
	long			cnt;
	long			vs;
	char			string[128];
	
	w = (PersonMachine*)p;
	
	finder = new usenet_find();
	
	sprintf(string, "\"%s+%s\"", w->first_name, w->last_name);
	finder->doit(string);
	
	if (strcmp(w->last_name, "Osadzinski") == 0) {
		finder->add_usenet("alt.binaries.pictures.erotica.erich-ringewald", "");
	}
	
	
	if (w->Lock()) {
		
		w->news_v->the_finder = finder;
		
		if (finder->newsgroup_count() <= 0) {
			vs = 80;
		}
		else
			vs = finder->newsgroup_count() * 20 + 14;
			
		w->news_v->v_size = vs;
		w->news_v->ResizeTo(1000, vs);
		w->news_v->Invalidate();
		w->ResizeTo(650,w->ev->v_size + w->pv->v_size + w->news_v->v_size - 8);
		
		w->Unlock();
	}
	
}

//-----------------------------------------------------------

PersonMachine::PersonMachine(char* fn, char *ln, char *title)
			 :BWindow(BRect(50, 50, 700, 275), title, B_TITLED_WINDOW,B_NOT_RESIZABLE |
											 			B_NOT_ZOOMABLE,
											 			0)
{
	BRect		a_rect;
	
	strcpy(first_name, fn);
	strcpy(last_name, ln);
	
	Lock();
	SetPulseRate(100000);
	
	top_view = new BView(BRect(0, 0, 5000, 5000), "foo_view", B_FOLLOW_NONE, B_WILL_DRAW);
	top_view->SetViewColor(216,216,216);
	AddChild(top_view);
	pv = new phone_view(BRect(0, 0, 700, 80), 0);
	ev = new email_view(BRect(0, 80, 700, 160), 0);
	news_v = new usenet_view(BRect(0, 160, 700, 400), 0);
	top_view->AddChild(pv);
	top_view->AddChild(ev);
	top_view->AddChild(news_v);
	pv->SetViewColor(216,216,216);
	ev->SetViewColor(216,216,216);
	news_v->SetViewColor(216,216,216);
	Unlock(); 
	
	resume_thread(spawn_thread(person_searcher,"person_searcher",B_NORMAL_PRIORITY,this));
	resume_thread(spawn_thread(email_searcher,"email_searcher",B_NORMAL_PRIORITY,this));
	resume_thread(spawn_thread(usenet_searcher,"usenet_searcher",B_NORMAL_PRIORITY,this));

}

//---------------------------------------------------------

bool PersonMachine::QuitRequested( void )
{
}

//---------------------------------------------------------

	map_view::map_view(BRect r)
 	: BView (r, "phone_view", B_FOLLOW_NONE, B_WILL_DRAW)
{
	the_map = 0;
	done = 0;
}

//---------------------------------------------------------

	map_view::~map_view()
{
	if (the_map)
		delete the_map;
}

//---------------------------------------------------------

void map_view::fancy_rect(BRect r)
{
	SetHighColor(250, 250, 250);
	StrokeRect(r);
	r.top--;
	r.left--;
	r.bottom--;
	r.right--;
	SetHighColor(140, 140, 140);
	StrokeRect(r);
}

//---------------------------------------------------------

void	map_view::Draw(BRect r)
{
	SetDrawingMode(B_OP_OVER);
	if (done && (the_map == 0)) {
		fancy_rect(BRect(10, 10, 600-10, 510 - 10));
		MovePenTo(BPoint(230, 180));
		SetHighColor(0, 0, 255);
		SetFontSize(14);
		DrawString("Map : Location not found !");
		return;	
	}
	
	if (!done) {
		fancy_rect(BRect(10, 10, 600-10, 510 - 10));
		MovePenTo(BPoint(230, 180));
		SetHighColor(0, 0, 255);
		SetFontSize(14);
		DrawString("Map : Waiting for Data !");
		return;	
	}

	DrawBitmap(the_map, BPoint(0,0));	
}

//---------------------------------------------------------

void	map_view::MouseDown(BPoint where)
{
}

//---------------------------------------------------------

long	map_maker(void *p)
{
	MapMachine		*w;
	map_getter		*map;
	
	w = (MapMachine*)p;

	map = new map_getter();
	
	map->doit(w->street, w->city, w->state, w->zip);

	if (map->get_map() == 0) {
		map->doit("", w->city, w->state, w->zip);
	}

	if (map->get_map()) {
		if (w->Lock()) {
			w->mv->the_map = map->get_map();
			w->Unlock();
		}
	}
	if (w->Lock()) {
		w->mv->done = 1;
		w->mv->Invalidate();
		w->Unlock();
	}
	
	delete map;
	
}

//---------------------------------------------------------

MapMachine::MapMachine(char *fstreet, char *fcity, char *fzip, char *fstate, char *title)
			 :BWindow(BRect(50, 50, 50+598, 50+510), title, B_TITLED_WINDOW,B_NOT_RESIZABLE |
											 			B_NOT_ZOOMABLE, 0)
{
	BRect		a_rect;
	
	strcpy(street, fstreet);
	strcpy(city, fcity);
	strcpy(zip, fzip);
	strcpy(state, fstate);
	
	Lock();
	SetPulseRate(100000);
	
	top_view = new BView(BRect(0, 0, 5000, 5000), "foo_view", B_FOLLOW_NONE, B_WILL_DRAW);
	top_view->SetViewColor(216,216,216);
	AddChild(top_view);
	
	mv = new map_view(BRect(0, 0, 600, 520));
	top_view->AddChild(mv);
	mv->SetViewColor(216,216,216);
	Unlock(); 
	
	resume_thread(spawn_thread(map_maker,"map_maker",B_NORMAL_PRIORITY,this));
}

//---------------------------------------------------------

bool MapMachine::QuitRequested( void )
{
}


//---------------------------------------------------------
