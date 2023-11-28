#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "disp_window.h"
#include "protocol.h"

const char *app_signature = "application/x-vnd.Be-sat";

//-----------------------------------------------------------

uchar	*bits[33];

//-----------------------------------------------------------

void	pmask(uchar *p1, uchar *p2, long hs, long vs)
{
	long	sum;
	long	x,y;
	long	dx,dy;
	
	for (y = 4; y < (vs - 4); y++) {
		for (x = 4; x < (hs - 4); x++) {
			sum = 0;
			for (dx = -4; dx <= 4; dx++)
			for (dy = -4; dy <= 4; dy++) {
				sum += *(p2+ hs*(y+dy) + (dx+x));
			}
			sum /= 1150;
			sum = *(p2 + (y*hs) + x) - sum;
			sum *= 1.23;
			if (sum < 0) sum = 0;
			if (sum > 125) sum = 125;
			
			*(p1 + (y*hs) + x) = sum;
		}
	}
}

//-----------------------------------------------------------

TSearchApplication::TSearchApplication(char *search_string)
		  :BApplication(app_signature)
{
	gif_getter	*g;
	CGIF		*a_gif;
	BWindow		*w;
	BView		*v;
	long		i;
	TDispWindow	*d;
	BBitmap		*tmp;
	BBitmap		*tmp1;
	long		last_size = -1;
	uchar		*data;
	
	printf("a1\n");
	for (i = 0; i < 33; i++)
		bits[i] = 0;

	d = new TDispWindow(BRect(0,20,638,480 + 20), "benoit's private property");
	d->Show();
	tmp = new BBitmap(BRect(0,0,HS-1, VS-1), B_COLOR_8_BIT, TRUE);
	v = new BView(BRect(0, 0, 5000, 5000), "foo_view", B_FOLLOW_NONE, B_WILL_DRAW);
	tmp->Lock();
	tmp->AddChild(v);
	tmp->Unlock();
	g = new gif_getter();
	while(1) {
		printf("check\n");
			g->doit("http://www.atmo.arizona.edu/gifs/VISWEST.GIF");
		while(!g->done) {
			snooze(1000000);
		}

		if (g->get_gif_size() != last_size) {
			last_size = g->get_gif_size();
		

			a_gif = g->get_data();

			while(a_gif == 0) {
				g->doit("http://www.atmo.arizona.edu/gifs/VISWEST.GIF");
				while(!g->done) {
					snooze(1000000);
				}
				a_gif = g->get_data();
			}

			tmp->Lock();
			v->DrawBitmap(a_gif->pixels, BPoint(-50, -140));
			v->Sync();
			tmp->Unlock();
			
				
			for (i = 0; i < 31; i++)
				bits[i] = bits[i+1]; 
				
			bits[31] = (uchar *)malloc(HS*VS);
			pmask(bits[31], (uchar *)tmp->Bits(), HS, VS);
			
			//memcpy(bits[31], (char *)tmp->Bits(), HS*VS);
			
			delete a_gif->pixels;
			delete a_gif;
			
			for (i = 0; i < 400; i++)
				snooze(1000*1000);
		}
		else {
			printf("same\n");
			for (i = 0; i < 200; i++)
				snooze(1000*1000);
		}
	}
	delete g;
} 

//-------------------------------------------------------------------------

TSearchApplication::~TSearchApplication()
{
}

//-------------------------------------------------------------------------

void TSearchApplication::MessageReceived(BMessage *msg)
{
	BApplication::MessageReceived(msg);
}

//-----------------------------------------------------------

int	main(int argc, char *argv[])
{
	TSearchApplication	*my_app;
	
	printf("main\n");
	my_app = new TSearchApplication(argv[1]);
	

	my_app->Run();
	
	delete my_app;
	return 0;
}


//-----------------------------------------------------------

