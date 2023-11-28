#include "edit_window.h"
#include <FindDirectory.h>
#include <Path.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Debug.h>
#include <Application.h>
#include <MessageFilter.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollBar.h>
#include <Entry.h>
#include <OS.h>
#include <Bitmap.h>
#include <FilePanel.h>
//-----------------------------------------------------------

#define		QUIT			0x105

#define		TRANS_NORMAL	0x200
#define		TRANS_BLEND		0x201

#define		NORMAL_BUTTON	0x300
#define		RETURN_BUTTON	0x301

//-----------------------------------------------------------

typedef	struct {
	BRect	r;
	char	path[256];
	int		mode;
	int		transition;
	int		private1;
	int		private2;
	int		private3;
	int		private4;
} ilink;

//-----------------------------------------------------------
#define	MAX_LINK	128
//-----------------------------------------------------------

class	DataView	: public BView {
private:

	long		h_size;
	long		v_size;
	BBitmap		*bt;
	ilink		links[MAX_LINK];
	long		lcount;
	long		selected;
	char		cur_name[256];
	char		stack[16][256];
	char		sp;
public:
				DataView(BRect frame, char *name);
				~DataView();
virtual	void	Draw(BRect r);
virtual	void	MouseDown(BPoint p);
		void	new_rect(BPoint p);
		long	find_rect(BPoint p);
		void	pstrokeRect(long i, BRect r, char select = 0);
		void	mod_rect(long i, BPoint p);
		void	change_selected(long old, long news);
		void	KeyDown(const char *key, int32 count);
		void	del_cur();
		void	save();
		void	load();
		void	MouseMoved(BPoint p, uint32, const BMessage *m);
		bool	MessageDropped(BMessage *inMessage, BPoint where, BPoint offset);
		void	MessageReceived(BMessage *message);
		void	do_set_link(long ii, char *name);
		void	real_mode(BPoint p);
		void	reload(char *name);
		void	SetTransition(int trans);
		void	SetMode(int mode);
		void	push_path(char *p);
};

//-----------------------------------------------------------

class	StatusView	: public BView {
private:
		char	name[256];
public:
				StatusView(BRect frame, char *name);
virtual	void	Draw(BRect r);
		void	SetInfo(char *fname, short fmode, short ftransition);

		short	mode;
		short	transition;
};

//-----------------------------------------------------------

	StatusView::StatusView(BRect frame, char *name) :
	BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetViewColor(128,128,128);
}


//-----------------------------------------------------------

void	StatusView::SetInfo(char *fname, short fmode, short ftransition)
{
	char	changed = 0;

	if (fmode != mode)
		changed = 1;

	if (ftransition != transition)
		changed = 1;

	transition = ftransition;
	mode = fmode;

	if (fname[0] == 0) {
		if (strcmp(name, "no link") != 0) {
			strcpy(name, "no link");
			changed = 1;
		}
		goto out;
	}

	if (strcmp(name, fname) != 0) {
		strcpy(name,fname);
		changed = 1;
	}

out:
	if (changed) {
		Invalidate(); 
	}
}

//-----------------------------------------------------------

void	StatusView::Draw(BRect r)
{
	BRect	r1;
	char	buf[512];

	r1 = Bounds();
	SetHighColor(0,0,0);
	StrokeRect(r1);
	MovePenTo(20, 12);
	SetDrawingMode(B_OP_OVER);
	sprintf(buf, "link:<%s>", name);
	DrawString(buf);


	MovePenTo(300, 12);
	switch(mode) {
		case NORMAL_BUTTON:
			DrawString("Normal Button");break;
		case RETURN_BUTTON:
			DrawString("Return Button");break;
	}
	MovePenTo(500, 12);
	switch(transition) {
		case TRANS_NORMAL:
			DrawString("Normal transition");break;
		case TRANS_BLEND:
			DrawString("Blend Transition");break;
	}
}

//-----------------------------------------------------------

static	BFilePanel	*mOpenPanel;
		
void	do_open()
{
	if (mOpenPanel == NULL) {
		mOpenPanel = new BFilePanel(B_OPEN_PANEL, 
									&BMessenger(be_app), 
									0);
	}		
	mOpenPanel->Show();
}

//-----------------------------------------------------------

void	DataView::do_set_link(long ii, char *name)
{
	long	l;
	strcpy(links[ii].path, name);

	l = strlen(links[ii].path);

	if (links[ii].path[l-1] == 'p')
	if (links[ii].path[l-2] == 'm')
	if (links[ii].path[l-3] == 'b')
	if (links[ii].path[l-4] == '.') {
		links[ii].path[l-4] = 0;
	}

	Invalidate();
}

//-----------------------------------------------------------

bool	DataView::MessageDropped(BMessage *inMessage, BPoint where, BPoint offset)
{
	entry_ref	ref;
	BEntry		*an_entry;
	BPath		p;

	switch (inMessage->what ){
   		case B_SIMPLE_DATA:
   		case B_REFS_RECEIVED:
			if (inMessage->HasRef("refs")) {
				for (int32 index = 0; ; index++) {
					if(inMessage->FindRef("refs", index, &ref) != B_OK)
						break;

					an_entry = new BEntry(&ref);
					an_entry->GetPath(&p);
					p.GetParent(&p);

					delete an_entry;
					long ii = find_rect(where);

					if (ii >= 0) {
						do_set_link(ii, ref.name);
						save();
					}
   				}
   			}
		break;
	}
}

//-----------------------------------------------------------

void	DataView::MessageReceived(BMessage *message)
{
	bool handled = false;
	
	// was this message dropped?

	if (message->WasDropped()) {
		BPoint dropLoc;
		BPoint offset;
		
		dropLoc = message->DropPoint(&offset);
		ConvertFromScreen(&dropLoc);
		ConvertFromScreen(&offset);

		handled = MessageDropped(message, dropLoc, offset);
	}
}

//-----------------------------------------------------------

void	DataView::MouseMoved(BPoint p, uint32, const BMessage *m)
{
	BPoint	w;
	ulong	b;
	float	obj_x;
	float	obj_y;
	float	dx,dy;
	float	s,e;

	long ii = find_rect(p);

	if (m)
	if (m->HasRef("refs")) {

		if (ii >= 0) {
			change_selected(selected, ii);
			selected = ii;
		}
	}

	if (ii >= 0) {
		((TEditWindow*)Window())->sv->SetInfo(links[ii].path, links[ii].mode, links[ii].transition);
	}
	else
		((TEditWindow*)Window())->sv->SetInfo("empty", 0, 0);
}

//-----------------------------------------------------------

void	DataView::save()
{
	long	ref;
	char	buf[256];

	sprintf(buf, "%s.map", cur_name);
	ref = open(buf, O_RDWR | O_CREAT);
	write(ref, &lcount, sizeof(lcount));
	write(ref, links, sizeof(links));
	close(ref);
}

//-----------------------------------------------------------

void	DataView::load()
{
	long	ref;
	char	buf[256];

	sprintf(buf, "%s.map", cur_name);

	ref = open(buf, O_RDWR);
	if (ref > 0) {
		read(ref, &lcount, sizeof(lcount));
		read(ref, links, sizeof(links));
		close(ref);
	}
}

//-----------------------------------------------------------

void	DataView::reload(char *name)
{
	long	ref;
	uchar	header[0x30];
	uchar	*data;
	ulong	p;
	ulong	*pp;
	long	x,y;
	ulong	r,g,b;
	long	i;
	char	buf[256];

	for (i = 0; i < MAX_LINK; i++) {
		links[i].mode = NORMAL_BUTTON;
		links[i].transition = TRANS_NORMAL;
		links[i].private1 = 0;
		links[i].private2 = 0;
		links[i].private3 = 0;
		links[i].private4 = 0;
	}
	
	lcount = 0;
	strcpy(cur_name, name);
	load();




	sprintf(buf, "%s.bmp", cur_name);
	ref = open(buf, O_RDONLY);
	selected = -1;
	read(ref, &header, 0x30);

	h_size = header[0x12] + header[0x13]*256;	
	v_size = header[0x16] + header[0x17]*256;	


	if (bt)
		delete bt;

	bt = new BBitmap(BRect(0,0,h_size - 1,v_size - 1), B_RGB_32_BIT, FALSE);

	lseek(ref, 0x36, SEEK_SET);
	
	data = (uchar *)malloc(h_size * v_size * 3);

	read(ref, data, h_size * v_size * 3);

	for (y = 0; y < v_size; y++) {
		pp = (ulong *)bt->Bits();
		pp += (((v_size-1) - y) * (bt->BytesPerRow()>>2));
		for (x = 0; x < h_size; x++) {
			r = *data++;
			g = *data++;
			b = *data++;
			*pp++ = (r<<24)|(g<<16)|b<<8;
		}
	}
	free((char *)data);
	close(ref);
}

//-----------------------------------------------------------

	DataView::DataView(BRect frame, char *name) :
	BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	bt = 0;
	sp = -1;
	reload(name);
}

//-----------------------------------------------------------

void	DataView::del_cur()
{
	long	i;

	
	if (selected >= 0) {
		SetDrawingMode(B_OP_INVERT);
		pstrokeRect(selected, links[selected].r, 1);
		SetDrawingMode(B_OP_COPY);
		for (i = selected;i < lcount; i++) {
			links[i] = links[i + 1]; 
		}
		lcount--;
	}
	selected = -1;
	save();
}

//-----------------------------------------------------------

void	DataView::KeyDown(const char *key, int32 count)
{
	if (*key == 8) {
		del_cur();
	}
}

//-----------------------------------------------------------


	DataView::~DataView()
{
}


//-----------------------------------------------------------

BRect	sort_rect(BRect r)
{
	float	tmp;

	if (r.top > r.bottom) {
		tmp = r.top;
		r.top = r.bottom;
		r.bottom = tmp;
	}
	if (r.left > r.right) {
		tmp = r.left;
		r.left = r.right;
		r.right = tmp;
	}
	return r;
}

//-----------------------------------------------------------

long	DataView::find_rect(BPoint p)
{	
	long	i;
	BRect	r;

	for (i = 0; i < lcount; i++) {
		r = links[i].r;
		r.top -= 2;
		r.bottom += 2;
		r.left -= 2;
		r.right += 2;


		if (r.Contains(p)) {
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------

void	DataView::pstrokeRect(long i, BRect r, char select)
{
	BRect	r1;
	char	inited = 0;

	if (r.Width() > 8 && r.Height() > 8) {
		r1.top = r.top-2;r1.bottom = r.top + 2;
		r1.left = r.left-2;r1.right = r.left + 2;
		FillRect(r1);
		r1.top = r.bottom-2;r1.bottom = r.bottom + 2;
		r1.left = r.left-2;r1.right = r.left + 2;
		FillRect(r1);

		r1.top = r.top-2;r1.bottom = r.top + 2;
		r1.left = r.right-2;r1.right = r.right + 2;
		FillRect(r1);

		r1.top = r.bottom-2;r1.bottom = r.bottom + 2;
		r1.left = r.right-2;r1.right = r.right + 2;
		FillRect(r1);
	}
	
	if (i >= 0) {
		if (links[i].path[0] != 0) {
			inited = 1;
		}
			
	}
	
	if (inited == 0) {
		SetPenSize(3);
		StrokeRect(r);
		SetPenSize(1);
	}
	else
		StrokeRect(r);

	if (select) {
		MovePenTo(BPoint(r.left, r.top));
		StrokeLine(BPoint(r.right, r.bottom));
		MovePenTo(BPoint(r.right, r.top));
		StrokeLine(BPoint(r.left, r.bottom));
	}
	
}


//-----------------------------------------------------------
#define	TOPLEFT	 1
#define	TOPRIGHT 2
#define	BOTLEFT	 3
#define	BOTRIGHT 4
#define	MOVE     5


void	DataView::mod_rect(long i, BPoint p)
{
	BRect	r;
	BRect	r0;
	BRect	r1;
	long	code;
	ulong	b;
	BPoint	p0, last;

	r = links[i].r;

	code = MOVE;

	r1.top = r.top-2;r1.bottom = r.top + 2;
	r1.left = r.left-2;r1.right = r.left + 2;

	if (r1.Contains(p)) {
		code = TOPLEFT;
	}
	
	r1.top = r.bottom-2;r1.bottom = r.bottom + 2;
	r1.left = r.left-2;r1.right = r.left + 2;

	if (r1.Contains(p)) {
		code = BOTLEFT;
	}

	r1.top = r.top-2;r1.bottom = r.top + 2;
	r1.left = r.right-2;r1.right = r.right + 2;

	if (r1.Contains(p)) {
		code = TOPRIGHT;
	}
	
	r1.top = r.bottom-2;r1.bottom = r.bottom + 2;
	r1.left = r.right-2;r1.right = r.right + 2;

	if (r1.Contains(p)) {
		code = BOTRIGHT;
	}

	last = p;

long	dh, dv;

	r = links[i].r;
	
	if (code == MOVE) {
		do {
			GetMouse(&p0, &b);
			if (p0 != last) {
				dh = last.x - p0.x;
				dv = last.y - p0.y;
				last = p0;
				r0 = r;
				SetDrawingMode(B_OP_INVERT);
				pstrokeRect(i, r0, (i == selected));
				r.OffsetBy(-dh, -dv);
				pstrokeRect(i, r, (i == selected));
				SetDrawingMode(B_OP_COPY);
			}
			else
				snooze(20000);
		} while(b);
		links[i].r = r;
		return;
	}

	r1 = r;
	do {
		GetMouse(&p0, &b);
		if (p0 != last) {
			dh = p.x - p0.x;
			dv = p.y - p0.y;
			last = p0;
			r0 = r1;
			r1 = r;
			if (code == TOPLEFT) {
				r1.top -= dv;
				r1.left -= dh;
				r1 = sort_rect(r1);
			}
			if (code == BOTLEFT) {
				r1.bottom -= dv;
				r1.left -= dh;
				r1 = sort_rect(r1);
			}
			if (code == BOTRIGHT) {
				r1.bottom -= dv;
				r1.right -= dh;
				r1 = sort_rect(r1);
			}
			if (code == TOPRIGHT) {
				r1.top -= dv;
				r1.right -= dh;
				r1 = sort_rect(r1);
			}


			SetDrawingMode(B_OP_INVERT);
			pstrokeRect(i, r0, (i == selected));
			pstrokeRect(i, r1, (i == selected));
			SetDrawingMode(B_OP_COPY);
		}
		else
			snooze(20000);
	} while(b);
	links[i].r = r1;
}

//-----------------------------------------------------------


void	DataView::change_selected(long old, long news)
{
	if (old == news)
		return;

	SetDrawingMode(B_OP_INVERT);
	if (old >= 0) {
		pstrokeRect(old, links[old].r, 1);
		pstrokeRect(old, links[old].r, 0);
	}
	
	if (news >= 0) {
		pstrokeRect(news, links[news].r, 0);
		pstrokeRect(news, links[news].r, 1);
	}
	SetDrawingMode(B_OP_COPY);
}

//-----------------------------------------------------------

void	DataView::new_rect(BPoint pp)
{
	BPoint	p0;
	BPoint	p1;
	BPoint	np;
	ulong	b;
	BRect	r;


	p0 = pp;
	r.top = -32000;
	r.bottom = -32000;
	r.left = -32000;
	r.right = -32000;

	SetDrawingMode(B_OP_INVERT);
	do {
		GetMouse(&np, &b);
		
		if (np != p1) {
			pstrokeRect(-1, r, 1);
			p1 = np;
			r.top = min(p0.y, p1.y);
			r.bottom = max(p0.y, p1.y);
			r.left = min(p0.x, p1.x);
			r.right = max(p0.x, p1.x);
			pstrokeRect(-1, r, 1);
		} 
		else
			snooze(20000);
	
	} while(b);
	
	if (r.Height() > 4 && r.Width() > 4) {
		links[lcount].r = r;
		links[lcount].path[0] = 0;
		links[lcount].mode = NORMAL_BUTTON;
		links[lcount].transition = TRANS_NORMAL;
		lcount++;
	}
	else {
			pstrokeRect(-1, r, 1);
	}

	SetDrawingMode(B_OP_COPY);
}

//-----------------------------------------------------------

void	DataView::push_path(char *p)
{
	sp++;
	sp = sp % 16;

	strcpy(stack[sp], p);
}

//-----------------------------------------------------------

void	DataView::real_mode(BPoint p)
{
	long	i = find_rect(p);

	printf("rec = %ld\n", i);
	
	if (i >= 0) {
		printf("mode == %ld\n", links[i].mode);

		if (links[i].mode == RETURN_BUTTON) {
		
			printf("return %ld -- %s\n", sp, stack[sp]);
			reload(stack[sp]);
			sp--;
			if (sp < 0)
				sp = 15;
			Draw(BRect(0,0,32000,32000));
			return;
		}
		
		if (links[i].mode == NORMAL_BUTTON) 
		if (links[i].path[0]) {
			printf("load %s\n", links[i].path);
			push_path(cur_name);
			reload(links[i].path);
			Draw(BRect(0,0,32000,32000));
		}
	}
}

//-----------------------------------------------------------

void	DataView::MouseDown(BPoint p)
{
	long	i;

	if (modifiers() & B_SHIFT_KEY) {
		real_mode(p);
		return;
	}

	if ((i = find_rect(p)) >= 0) {
		change_selected(selected, i);
		selected = i;
		mod_rect(i, p);
		save();
		return;
	}
	else {
		change_selected(selected, -1);
		selected = lcount;
		new_rect(p);
		save();
	}
}

//-----------------------------------------------------------

void	DataView::Draw(BRect r)
{
	long	i;

	DrawBitmap(bt, BPoint(0,0));

	SetDrawingMode(B_OP_INVERT);
	for (i = 0; i < lcount; i++) {
		pstrokeRect(i, links[i].r, (i==selected));
	}
	SetDrawingMode(B_OP_COPY);
}

//-----------------------------------------------------------

TEditWindow::~TEditWindow()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
}

//-----------------------------------------------------------

TEditWindow::TEditWindow(BRect r, const char* t)
			 :BWindow(r, t, B_DOCUMENT_WINDOW, 
											 B_NOT_ZOOMABLE
											 //B_WILL_ACCEPT_FIRST_CLICK
											 )
{
	BMenu		*a_menu;
	BRect		a_rect;
	BMenuBar	*menubar;
	BMenuItem	*item;
	float 		mb_height;
	long		i;
	long		vp;
	long		right;

	Lock();
	
	a_rect.Set( 0, 0, 1000, 15);
	menubar = new BMenuBar(a_rect, "MB");
	AddChild(menubar);
	mb_height = menubar->Bounds().Height();

	a_menu = new BMenu("File");
	a_menu->AddItem(new BMenuItem("Quit", new BMessage(QUIT)));
	menubar->AddItem(a_menu);
	
	a_menu = new BMenu("Flags");
	a_menu->AddItem(new BMenuItem("Normal button", new BMessage(NORMAL_BUTTON)));
	a_menu->AddItem(new BMenuItem("Return button", new BMessage(RETURN_BUTTON)));
	menubar->AddItem(a_menu);
	
	a_menu = new BMenu("Transition");
	a_menu->AddItem(new BMenuItem("Normal", new BMessage(TRANS_NORMAL)));
	a_menu->AddItem(new BMenuItem("Blend", new BMessage(TRANS_BLEND)));
	menubar->AddItem(a_menu);
	
	sv = new StatusView(BRect(0,mb_height + 1,32000,mb_height + 1 + 15), "status");
	AddChild(sv);
	
	dv = new DataView(BRect(0,mb_height + 1 + 16,32000,32000), "try1");

	AddChild(dv);
	dv->MakeFocus();

	Unlock();
}

//---------------------------------------------------------

void	TEditWindow::TakeRef(entry_ref ref)
{
	Lock();
	//tmain_view->TakeRef(ref);
	Unlock();
}

//---------------------------------------------------------
	


void	TEditWindow::MessageReceived(BMessage *b)
{
	switch(b->what) {
		case 	QUIT			:   this->Close();break;
		case	TRANS_NORMAL	:	dv->SetTransition(b->what);break;
		case	TRANS_BLEND		:	dv->SetTransition(b->what);break;
		case	NORMAL_BUTTON	:	dv->SetMode(b->what);break;
		case	RETURN_BUTTON	:	dv->SetMode(b->what);break;
	}
	_inherited::MessageReceived(b);
}

//-----------------------------------------------------------

void	DataView::SetTransition(int trans)
{
	if (selected >= 0) {
		links[selected].transition = trans;
	}
}

//-----------------------------------------------------------

void	DataView::SetMode(int mode)
{
	if (selected >= 0) {
		links[selected].mode = mode;
	}
}

//-----------------------------------------------------------
