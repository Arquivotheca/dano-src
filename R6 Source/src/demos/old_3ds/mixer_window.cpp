#include "mixer_window.h"
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
#include "main_view.h"
#include "title_view.h"
#include "header_view.h"

//-----------------------------------------------------------

#define		ADD_CHANNEL		0x100
#define		DELETE_CHANNEL	0x101
#define		QUIT			0x102


//-----------------------------------------------------------


TMixerWindow::TMixerWindow(BRect r, const char* t)
			 :BWindow(r, t, B_TITLED_WINDOW, B_NOT_RESIZABLE |
											 B_NOT_ZOOMABLE,
											 B_ALL_WORKSPACES)
{
	BMenu		*a_menu;
	BRect		a_rect;
	BMenuBar	*menubar;
	BMenuItem	*item;
	TMainView	*main_view;
	float 		mb_height;
	long		i;
	
	Lock();
	SetPulseRate(100000);
	a_rect.Set( 0, 0, 1000, 15);
	menubar = new BMenuBar(a_rect, "MB");
	
	a_menu = new BMenu("File");
	a_menu->AddItem(new BMenuItem("Add Channel", new BMessage(ADD_CHANNEL)));
	a_menu->AddItem(new BMenuItem("Delete Channel", new BMessage(DELETE_CHANNEL)));
	a_menu->AddSeparatorItem();
	a_menu->AddItem(new BMenuItem("Quit", new BMessage(QUIT)));
	menubar->AddItem(a_menu);


	AddChild(menubar);
	mb_height = menubar->Bounds().Height();
	
	
	main_view = new TMainView(BRect(0, mb_height, 3000, 3000));
	AddChild(main_view);
	main_view->SetViewColor(216,216,216);
	
	
	for (i = 0; i < 16; i++) {
		sliders[i] = new TMonoSlider (BPoint(64+(i*44),105), "", 0,
									   main_view, 0, 0, 0);
						   
		waves[i]   = new TWaveView(BPoint(64+(i*44),280), main_view);
		balance[i] = new TCtrlView(BPoint(63+(i*44),70), main_view);
		c2d[i] = new TCtrl2dView(BPoint(63+(i*44),20), main_view);
		//new TTitleView(BPoint(63+(i*44),3), main_view);
	}
	
	new HeaderView(BPoint(5, 1), main_view);
	sliders[0]->SetValue(0.5);
	sliders[1]->SetValue(0.5);
	sliders[2]->SetValue(0.5);
	Unlock(); 
	
	resume_thread(spawn_thread(init_p,"init_p",B_REAL_TIME_DISPLAY_PRIORITY,this));
}

//---------------------------------------------------------

bool TMixerWindow::QuitRequested( void )
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}
