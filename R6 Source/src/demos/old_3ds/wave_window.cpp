#include "wave_window.h"
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

//-----------------------------------------------------------

#define		A		0x100
#define		B		0x101
#define		C		0x102



//-----------------------------------------------------------

TWaveWindow::TWaveWindow(BRect r, const char* t)
			 :BWindow(r, t, B_TITLED_WINDOW, 
											 B_NOT_ZOOMABLE,
											 B_ALL_WORKSPACES)
{
	BMenu		*a_menu;
	BRect		a_rect;
	BMenuBar	*menubar;
	BMenuItem	*item;
	TSoundView	*main_view;
	float 		mb_height;
	long		i;
	
	Lock();
	SetPulseRate(100000);  
	a_rect.Set( 0, 0, 1000, 15);
	menubar = new BMenuBar(a_rect, "MB");
	
	a_menu = new BMenu("File");
	a_menu->AddItem(new BMenuItem("A", new BMessage(A)));
	a_menu->AddItem(new BMenuItem("B", new BMessage(B)));
	a_menu->AddSeparatorItem();
	a_menu->AddItem(new BMenuItem("C", new BMessage(C)));
	menubar->AddItem(a_menu);


	AddChild(menubar);
	mb_height = menubar->Bounds().Height();
	
	 
	main_view = new TSoundView(BRect(0, mb_height, 912, 912));
	AddChild(main_view);
	Unlock();
}

//---------------------------------------------------------

