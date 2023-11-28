#include "wave_window.h"
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
#include "global.h"
//-----------------------------------------------------------

#define		SAVE			0x100
#define		SAVE_P			0x101
#define		SAVE_WAVE		0x102
#define		OPEN			0x103
#define		ABOUT			0x104
#define		QUIT			0x105
#define		MSG_DEST_CHOSEN	0x106

#define		TEST			0x200
#define		UNDO			0x201
#define		CLEAR			0x202
#define		CUT				0x203
#define		COPY			0x204
#define		PASTE			0x205
#define		PREFERENCE		0x206

#define		TOGGLE_VIEW		0x300
#define		TOGGLE_CTRL		0x301
#define		BEAT_MARKS		0x302
#define		FAST_DISPLAY	0x303

#define		MUTE			0x400
#define		SOLO			0x401
#define		ALL				0x402
#define		NEWT			0x403

#define		BPM				0x501
#define		BPM1			0x502
#define		DELC			0x503

#define		PAUSE			0x600

//-----------------------------------------------------------
#define	TIME_VSIZE	40
#define	KK			64
//-----------------------------------------------------------
extern	char		slow_anim;
		char		sub_size = 0;
		char		fast_display=0;
		char		in_h_move = 0;
		TimeView	*the_time = 0;
		TWaveWindow	*the_window = 0;
extern	void		do_about(BWindow *dad);
extern	void		mix_master(char *p);
extern	void	stop_mixer();
extern	void	start_mixer();
//-----------------------------------------------------------

#define	SHOW_TRACK	0
#define	SHOW_MIX	1

//-----------------------------------------------------------

class	xBView	: public BView {
		long	v;

public:
				xBView(BRect frame, char *name);
				~xBView();
	
virtual	void	Draw(BRect r);
virtual	void	MouseDown(BPoint p);
		void	MoveV(long dv);
};

//-----------------------------------------------------------

	xBView::xBView(BRect frame, char *name) :
	BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	v = -2;
}

//-----------------------------------------------------------

void	enable_undo()
{
	if (the_window->Lock()) {
		the_window->undo_item->SetEnabled(true);
		the_window->Unlock();
	}
}

//-----------------------------------------------------------

void	disable_undo()
{
	if (the_window->Lock()) {
		the_window->undo_item->SetEnabled(false);
		the_window->Unlock();
	}
}

//-----------------------------------------------------------

void	xBView::MoveV(long dv)
{
	v -= dv;
	Draw(BRect(0,0,32000,32000));
}

//-----------------------------------------------------------

	xBView::~xBView()
{
}

//-----------------------------------------------------------

void	xBView::MouseDown(BPoint p)
{
	printf("%f %f\n", p.x, p.y);
}

//-----------------------------------------------------------

void	xBView::Draw(BRect r)
{
	SetHighColor(120,120,120);
	MovePenTo(BPoint(KK+1, 0 + v));
	StrokeLine(BPoint(KK+1,   v + 2000));
	SetHighColor(90,90,90);
	MovePenTo(BPoint(KK, v));
	StrokeLine(BPoint(KK, v+2000));


	SetHighColor(12,12,12);
	MovePenTo(BPoint(KK+1, 0 + v));
	StrokeLine(BPoint(KK+1,   -3000));
	SetHighColor(20,20,20);
	MovePenTo(BPoint(KK, v));
	StrokeLine(BPoint(KK, -3000));
}

//-----------------------------------------------------------

void	update_menus()
{
	the_window->UpdateMenus();
}

//-----------------------------------------------------------

void	TWaveWindow::UpdateMenus()
{
	char	v;
	long	i;

	v = tmain_view->HasChannelSelected();

	for (i = 0;i < 3; i++) {
		t_i[i]->SetEnabled(v);
	}

	v = tmain_view->HasChannel();

	for (i = 0;i < 4; i++) {
		h_c[i]->SetEnabled(v);
	}

	v = tmain_view->has_selection();

	for (i = 0; i < 4; i++) {
		os[i]->SetEnabled(v);
	}

	v = tmain_view->HasClip();

	for (i = 0; i < 1; i++) {
		h_cl[i]->SetEnabled(v);
	}
}

//-----------------------------------------------------------

void	TWaveWindow::Switch()
{
	BRect	bnd;
	long	dd;
	float	step;
	float	dt;
	double	t0,t1;
	float	k;


	Lock();
	bnd = Bounds();

	k = 1.1;

	if (mode == SHOW_TRACK) {
		dd = 0;
		step = 1;
		while((dd+step) < VS) {
			t0 = system_time();
			mix_view->ScrollBy(0,(int)(-step));
			master_view->MoveV((int)-step);
			UpdateIfNeeded();
			mix_view->Sync();
			t1 = system_time();
			dt = (t1-t0)/1000.0;

			if (dt > 15.0)
				k = 1.3;
			else
				k = 1.1;

			dd += (int)step;
			step *= k;
		}
		mix_view->ScrollBy(0,-(VS-dd));
		master_view->MoveV(-(VS-dd));
		UpdateIfNeeded();
		mix_view->ResizeBy(0, -(VS));
		UpdateIfNeeded();
		mode = SHOW_MIX;
		slow_anim = 0;
	}
	else {
		dd = 0;
		step = 1;
		mix_view->ResizeBy(0, (VS));
		while((dd+step) < VS) {
			t0 = system_time();
			mix_view->ScrollBy(0,(int)step);
			master_view->MoveV((int)step);
			UpdateIfNeeded();
			mix_view->Sync();
			t1 = system_time();
			dt = (t1-t0)/1000.0;

			if (dt > 15.0)
				k = 1.3;
			else
				k = 1.1;

			dd += (int)step;
			step *= k;
		}
		mix_view->ScrollBy(0,(VS-dd));
		master_view->MoveV((VS-dd));
		mode = SHOW_TRACK;
		slow_anim = 1;
	}
	Unlock();
}

//-----------------------------------------------------------
#define	HH	64

void	TWaveWindow::Switch_H()
{
	BRect			bnd;
	long			dd;
	float			step;
	float			k;
	float			t0,t1,dt;
	TCtrlView 		*ctrl = (TCtrlView *)FindView("ctrl");
	float			r;	
	float			pi = 3.1415926;

	//Lock();
	bnd = Bounds();
	sub_size ^= 0x01;

	k = 1.1;

	if (ctr == 0) {
		dd = 0;
		step = 1;
		r = pi;
		while(dd < HH) {
			r -= (pi)/40.0;
			ctrl->SetRot(r);
			t0 = system_time();
			master_view->ScrollBy((int)(-step), 0);
			UpdateIfNeeded();
			master_view->Sync();
			t1 = system_time();
			dt = (t1-t0)/1000.0;

			if (dt > 15.0)
				k = 1.3;
			else
				k = 1.1;

			dd += (int)step;
			step *= k;
		}
		master_view->ScrollBy(-(HH-dd), 0);
		master_view->ResizeBy(-(HH), 0);
		UpdateIfNeeded();

float	step = pi/40.0;

		if (mode == SHOW_MIX)
			step *= 2.0;

		while(r > 0) {
			r -= step;
			ctrl->SetRot(r);
			Unlock();
			snooze(10000);
			Lock();
		}
		ctrl->SetRot(0);
	}
	else {
		dd = 0;
		step = 1;
		master_view->ResizeBy((HH), 0);
		UpdateIfNeeded();
		r = 0;
		while(dd < HH) {
			r += (pi)/40.0;
			ctrl->SetRot(r);
			t0 = system_time();
			master_view->ScrollBy((int)step, 0);
			UpdateIfNeeded();
			master_view->Sync();
			t1 = system_time();

			dt = (t1-t0)/1000.0;

			if (dt > 15.0)
				k = 1.3;
			else
				k = 1.1;

			dd += (int)step;
			step *= k;
		}
		master_view->ScrollBy((HH-dd), 0);
		UpdateIfNeeded();
		
float	step = pi/40.0;

		if (mode == SHOW_MIX)
			step *= 2.0;
		
		while(r < pi) {
			r += step;
			ctrl->SetRot(r);
			Unlock();
			snooze(10000);
			Lock();
		}
		ctrl->SetRot(pi);
	}
	//Unlock();

	ctr ^= 0x01;
}

//-----------------------------------------------------------

TWaveWindow::~TWaveWindow()
{
	delete (mDirEntry);
	delete (mOpenPanel);
	delete (mSavePanel);
}

//-----------------------------------------------------------

static filter_result 
key_down_filter(BMessage *msg, BHandler **, BMessageFilter *filter)
{
	ulong 		key;

	//msg->PrintToStream();

	if (msg->FindInt32("raw_char", (int32 *)&key) != B_OK)
		return B_DISPATCH_MESSAGE;

	if (key == 0x0a) {
		the_window->handle_special_char(key);
		return B_SKIP_MESSAGE;
	}

	if (key == 0x09) {
		the_window->Switch_H();
	}
	
	return B_DISPATCH_MESSAGE;
}

//-----------------------------------------------------------

void	TWaveWindow::handle_special_char(char c)
{
	Lock();
	tmain_view->special_char(c);
	Unlock();
}

//-----------------------------------------------------------

TWaveWindow::TWaveWindow(BRect r, const char* t)
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
	THSB		*fHSB;
	BScrollBar	*fVSB;
	ZoomControl	*zoom;
	ToolView	*tool_view;
	long		vp;
	long		right;

	the_window = this;
	Lock();
	fDestFilePanel = 0;
	mOpenPanel = 0;
	mSavePanel = 0;
	mDirEntry = 0;
	slow_anim = 1;
	wave_save = 0;
	is_paused = 0;

	mode = SHOW_TRACK;
	ctr = 0;
	SetPulseRate(100000);  
	a_rect.Set( 0, 0, 1000, 15);
	menubar = new BMenuBar(a_rect, "MB");
	

	a_menu = new BMenu("File");
	a_menu->AddItem(new BMenuItem("Open Project...", new BMessage(OPEN), 'O'));
	a_menu->AddItem(h_c[0] = new BMenuItem("Save Project", new BMessage(SAVE_P), 'S'));
	a_menu->AddItem(h_c[1] = new BMenuItem("Save Project As...", new BMessage(SAVE)));
	a_menu->AddSeparatorItem();
	a_menu->AddItem(h_c[2] = new BMenuItem("Save Mix As WAVE...", new BMessage(SAVE_WAVE)));
	a_menu->AddSeparatorItem();
	a_menu->AddItem(new BMenuItem("About 3DmiX...", new BMessage(ABOUT)));
	a_menu->AddSeparatorItem();
	a_menu->AddItem(new BMenuItem("Quit", new BMessage(QUIT)));
	menubar->AddItem(a_menu);

	a_menu = new BMenu("Edit");
	a_menu->AddItem(undo_item = new BMenuItem("Undo", new BMessage(UNDO), 'Z'));
	a_menu->AddSeparatorItem();
	a_menu->AddItem(os[0] = new BMenuItem("Clear", new BMessage(CLEAR)));
	a_menu->AddItem(os[1] = new BMenuItem("Cut", new BMessage(CUT), 'X'));
	a_menu->AddItem(os[2] = new BMenuItem("Copy", new BMessage(COPY), 'C'));
	a_menu->AddItem(h_cl[0] = new BMenuItem("Paste", new BMessage(PASTE), 'V'));
	//a_menu->AddSeparatorItem();
	//a_menu->AddItem(new BMenuItem("Preferences...", new BMessage(PREFERENCE)));
	menubar->AddItem(a_menu);

	a_menu = new BMenu("Display");
	a_menu->AddItem(new BMenuItem("Show/Hide Mixer", new BMessage(TOGGLE_VIEW), 'T'));
	a_menu->AddItem(new BMenuItem("Show/Hide Controls", new BMessage(TOGGLE_CTRL), 'U'));
	a_menu->AddItem(new BMenuItem("Show/Hide beat marks", new BMessage(BEAT_MARKS), 'B'));
	a_menu->AddSeparatorItem();
	a_menu->AddItem(fast_item = new BMenuItem("Fast Display", new BMessage(FAST_DISPLAY)));
	menubar->AddItem(a_menu);
	
	channel_menu = new BMenu("Tracks");
	channel_menu->AddItem(t_i[0] = new BMenuItem("(Un)Mute Selection", new BMessage(MUTE), 'M'));
	channel_menu->AddItem(t_i[1] = new BMenuItem("(Un)Solo Selection", new BMessage(SOLO), 'L'));
	channel_menu->AddSeparatorItem();
	channel_menu->AddItem(new BMenuItem("New Track", new BMessage(NEWT)));
	channel_menu->AddItem(t_i[2] = new BMenuItem("Delete Tracks", new BMessage(DELC)));
	channel_menu->AddItem(h_c[3] = new BMenuItem("Select All Tracks", new BMessage(ALL), 'A'));
	menubar->AddItem(channel_menu);

	channel_menu = new BMenu("Misc");
	channel_menu->AddItem(new BMenuItem("Ticks from Time Selection", new BMessage(BPM)));
	channel_menu->AddItem(os[3] = new BMenuItem("Ticks from sample length", new BMessage(BPM1)));
	channel_menu->AddSeparatorItem();
	channel_menu->AddItem(pause_item = new BMenuItem("Pause", new BMessage(PAUSE), 'P'));

	menubar->AddItem(channel_menu);
	
	AddChild(menubar);
	mb_height = menubar->Bounds().Height();


//move left by 2 ?

  	mix_view = new BView(BRect(KK+2,0,5000,(r.bottom-r.top)+VS), "top", B_FOLLOW_ALL, B_WILL_DRAW);
  	
	master_view = new xBView(BRect(0,mb_height+1,5000,(r.bottom-r.top)+VS), "top");
	
	AddChild(master_view);
	master_view->AddChild(mix_view);
	mix_view->ScrollBy(0,VS);
	 
	sound_view = new TSoundView(BRect(0, 0, HS-1, VS));
	mix_view->AddChild(sound_view);
	right = r.right-r.left;


//left -2
  	track_view = new BView(BRect(0,VS,r.right-r.left,r.bottom-r.top+50+VS), "top", B_FOLLOW_ALL, 0);
	mix_view->AddChild(track_view);

	SetPulseRate(100000);  
	a_rect.Set( 0, 0, 1000, 15);

	
	mb_height -= 19;
	vp = (int)(r.bottom - r.top - 34 - mb_height);
	
	mb_height = 0;

	a_rect.right = 109;
	a_rect.bottom = vp +  1 + B_H_SCROLL_BAR_HEIGHT;
	a_rect.top = vp + 1;
	a_rect.left = 0;
	zoom = new ZoomControl(a_rect);

	track_view->AddChild(zoom);
	
	mb_height = 0; 
	a_rect.left = 0;
	a_rect.top = mb_height + 1;
	a_rect.bottom = mb_height + TIME_VSIZE;
	a_rect.right = 119;


	tmain_view = new TrackView(BRect(0, mb_height + 1 + TIME_VSIZE, right - 14, vp), "tv", sound_view);
	track_view->AddChild(tmain_view);
	
	a_rect.left = 0;
	a_rect.right = KK-1;
	a_rect.top = 0;
	a_rect.bottom = 32000;

	master_view->AddChild(sv = new StatusView(a_rect, "stat"));

	a_rect.left = HEADER_SPACE;	
	a_rect.top = mb_height + 1;
	a_rect.bottom = mb_height + TIME_VSIZE;
	a_rect.right = right;	

	time_view = new TimeView(a_rect, "time");
	the_time = time_view;
	track_view->AddChild(time_view);
	

	a_rect.left = 0;
	a_rect.right = (HEADER_SPACE - 1);				//new hpos
	a_rect.bottom = mb_height + TIME_VSIZE;
	a_rect.top = mb_height + 1;
	
	ctrl_view = new TCtrlView(a_rect, "ctrl");
	track_view->AddChild(ctrl_view);
	
	
	a_rect.right = right - 14 + 1;
	a_rect.bottom = vp +  1 + B_H_SCROLL_BAR_HEIGHT;
	a_rect.top = vp + 1;
	a_rect.left = 110;
	fHSB = new THSB("_HSB_", a_rect);

	track_view->AddChild(fHSB);
	
	a_rect.left = right - 13;
	a_rect.top = mb_height + 1 + TIME_VSIZE;
	a_rect.right = a_rect.left + B_V_SCROLL_BAR_WIDTH;
	a_rect.bottom = vp + 1;
	fVSB = new BScrollBar(a_rect, "_VSB_", tmain_view, 0, 500,
						  B_VERTICAL);
	track_view->AddChild(fVSB);
	fVSB->SetSteps(12, 50);
	zoom->MouseDown(BPoint(40, 10));
	mix_view->SetViewColor(0,0,0);
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN, key_down_filter));
	master_view->ScrollBy(KK + 2, 0);
	time_view->Toggle();
	UpdateMenus();
	Unlock();
}

//---------------------------------------------------------

void	TWaveWindow::TakeRef(entry_ref ref)
{
	Lock();
	tmain_view->TakeRef(ref);
	Unlock();
}

//---------------------------------------------------------

void	TWaveWindow::MixMaster()
{
	wave_save = 1;
	if (!mSavePanel) {
		entry_ref dirEntry;
		mSavePanel = new BFilePanel(B_SAVE_PANEL, &BMessenger(this), 0);
		mSavePanel->Window()->SetTitle("Save Wave As");
	}

	mSavePanel->Show();
}

//---------------------------------------------------------

void
TWaveWindow::SaveAs()
{
	wave_save = 0;
	if (!mSavePanel) {
		entry_ref dirEntry;
		mSavePanel = new BFilePanel(B_SAVE_PANEL, &BMessenger(this), 0);
		//if (mEntry != NULL)
			mSavePanel->Window()->SetTitle("Save As");
	}

	mSavePanel->Show();
}

//---------------------------------------------------------

void	TWaveWindow::DoSave(entry_ref *dir, const char	*name)
{
	BEntry		*an_entry;
	BPath		p;

	an_entry = new BEntry(dir);
	an_entry->GetPath(&p);

	if (wave_save) {
		char	buf[2048];

		sprintf(buf, "%s/%s", p.Path(), name);
		mix_master(buf);
		wave_save = 0;
	}
	else
		tmain_view->Save(const_cast<char *>(p.Path()), const_cast<char *>(name));
}

//---------------------------------------------------------

void	TWaveWindow::MessageReceived(BMessage *b)
{
	switch(b->what) {
		case 	'txt!' 			: 	tmain_view->txt_msg();break;
		case 	'bet!' 			: 	ctrl_view->txt_msg();break;
		case NEW_BPM   			: 	ctrl_view->new_bpm(b->FindInt32("v"));break;
		case 	QUIT			:   this->Close();break;
		case 	SAVE 			:   SaveAs();break;
		case 	SAVE_P 			:   tmain_view->save_named();break;
		case 	SAVE_WAVE		:   MixMaster();break;
		case 	TEST 			:	ChooseDestination();break;
		case 	TOGGLE_VIEW		:	Switch();break;
		case	TOGGLE_CTRL		:	this->Switch_H();break;
		case	BEAT_MARKS		:	time_view->Toggle();break;
		case	UNDO			:	tmain_view->undo();break;
		case	COPY			:	tmain_view->Copy();break;
		case	PASTE			:	tmain_view->Paste();break;
		case	CLEAR			:	tmain_view->Clear();break;
		case	CUT				:	tmain_view->Cut();break;
		case	BPM				:	tmain_view->DoBPM();break;
		case	BPM1			:	tmain_view->DoBPM1();break;
		case	DELC			:	tmain_view->DeleteChannels();break;
		case	ABOUT			:	do_about(this);break;
		case	PAUSE			:	{
									is_paused ^= 1;
									pause_item->SetMarked(is_paused);
									if (is_paused)
										stop_mixer();
									else
										start_mixer();
									break;
									}
		case	FAST_DISPLAY	:
						{
							fast_display ^= 1;
							tmain_view->Refresh();
							fast_item->SetMarked(fast_display);
							break;
						}
				
		case	ALL				:	sound_view->select_all();break;			
		case	NEWT			:	tmain_view->new_track();break;			
		case	SOLO			:	sound_view->solo_selection();break;			
		case	MUTE			:	sound_view->mute_selection();break;

		case 	B_SAVE_REQUESTED:
						{
							entry_ref dir;
							b->FindRef("directory", &dir);

							const char *name = NULL;
							b->FindString("name", &name);

							DoSave(&dir, name);
							break;
						}
		case 	OPEN :
		{
			if (mOpenPanel == NULL) {
				BMessenger app(be_app);
				mOpenPanel = new BFilePanel(B_OPEN_PANEL, 
											&app, 
											0);
			}		
			mOpenPanel->Show();
			break;
		}


		case 	MSG_DEST_CHOSEN:				// folder picked in file panel
			{
				b->FindRef("refs",&fDestRef);
				BPath p;
				BEntry e(&fDestRef);
				
				e.GetPath(&p);
				
				if (fDestFilePanel) {
					delete fDestFilePanel;
					fDestFilePanel = NULL;
				}
			}
			break;
	}

	update_menus();
	_inherited::MessageReceived(b);
}

//---------------------------------------------------------

void
TWaveWindow::ChooseDestination(void)
{
	if (fDestFilePanel)
		delete fDestFilePanel;
		
		BMessage	*msg;
		
		msg = new BMessage(MSG_DEST_CHOSEN);	
		fDestFilePanel = new TDirFilePanel(new BMessenger(NULL,this),&fDestRef,msg,
			&fDirFilter);
			
		float offset = 10;
	  	(fDestFilePanel->Window())->MoveTo(Frame().left+offset,Frame().top+ offset);

		fDestFilePanel->Show();	
}

			
//---------------------------------------------------------

