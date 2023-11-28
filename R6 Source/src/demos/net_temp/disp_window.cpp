#include "disp_window.h"
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
#include "protocol.h"


//-------------------------------------------------------------------------

extern	uchar	*bits[33];


//-------------------------------------------------------------------------

void	mix(uchar *b1, uchar *b2, long vcase, BBitmap *out)
{
	uchar	*p1;
	uchar	*p2;
	uchar	*p3;
	int		sum;
	long	size;

	size = out->BitsLength();
	p1 = (uchar *)b1;;
	p2 = (uchar *)b2;
	p3 = (uchar *)out->Bits();


	switch(vcase) {
		case 1:
			while(size-=2) {
				sum = *p1++;
				sum = 8 + sum + sum + sum + *p2++;
				*p3++ = sum >> 4;

				sum = *p1++;
				sum = 8 + sum + sum + sum + *p2++;
				*p3++ = sum >> 4;
			}
			break;
		case 2:
			while(size-=2) {
				sum = 4 + (*p1++) +
					      (*p2++);
				*p3++ = sum >> 3;

				sum = 4 + (*p1++) +
					      (*p2++);
				*p3++ = sum >> 3;
			}
			break;
		case 3:
			while(size-=2) {
				sum = *p2++;
				sum = 8 + sum + sum + sum + *p1++;
				*p3++ = sum >> 4;

				sum = *p2++;
				sum = 8 + sum + sum + sum + *p1++;
				*p3++ = sum >> 4;
			}
			break;
		case 4:
			while(size-=2) {
				sum = 2 + *p2++;
				*p3++ = sum >> 2;
				
				sum = 2 + *p2++;
				*p3++ = sum >> 2;
			}
			break;
	}
}


//-------------------------------------------------------------------------

void	TDispWindow::disp()
{
	long	i;
	BBitmap	*out;
	
	out = new BBitmap(BRect(0,0,HS-1, VS-1), B_COLOR_8_BIT);
again:;

	
	while(1) {
		bits[32] = bits[31];
		if (bits[0]) {
			free((char *)bits[0]);
			bits[0] = 0;
		}
		Lock();
		view->SetDrawingMode(B_OP_COPY);
		Unlock();
		for (i = 0; i < 32; i++) {
			if (bits[i] && bits[i+1]) {
				mix(bits[i], bits[i+1], 1, out);
				Lock();
				view->DrawBitmap(out, BPoint(0,0));
				Sync();
				Unlock();
				snooze(10000);
				
				mix(bits[i], bits[i+1], 2, out);
				Lock();
				view->DrawBitmap(out, BPoint(0,0));
				Sync();
				Unlock();
				snooze(10000);
				
				mix(bits[i], bits[i+1], 3, out);
				Lock();
				view->DrawBitmap(out, BPoint(0,0));
				Sync();
				Unlock();
				snooze(10000);
				
				mix(bits[i], bits[i+1], 4, out);
				Lock();
				view->DrawBitmap(out, BPoint(0,0));
				Sync();
				Unlock();
				snooze(20000);
			}
		}
	}
}

//-------------------------------------------------------------------------

long	displayer(void *p)
{
	TDispWindow	*pp;

	pp = (TDispWindow *)p;

	pp->disp();
}

//-----------------------------------------------------------

TDispWindow::TDispWindow(BRect r, const char* t)
			 :BWindow(r, t, B_TITLED_WINDOW,B_NOT_ZOOMABLE)
{
	BMenu		*a_menu;
	BRect		a_rect;
	BMenuItem	*item;
	long		i;
	
	Lock();

	view = new BView(BRect(0, 0, 5000, 5000), "foo_view", B_FOLLOW_NONE, B_WILL_DRAW);
	AddChild(view);
	Unlock(); 
	
	resume_thread(spawn_thread(displayer,"displayer",8,this));
}

//---------------------------------------------------------

bool TDispWindow::QuitRequested( void )
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}

//---------------------------------------------------------
