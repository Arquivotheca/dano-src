#include <Application.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "termwindow.h"
#include "shell.h"


char *default_args[] = {
	"/bin/sh", "-login", NULL
};

bool bTermWindow::QuitRequested()
{
	BMessage death('win-');
	death.AddPointer("window",this);
	be_app->PostMessage(&death);
	return true;
}

#define BUFSIZE 1024

void 
bTermWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what){
	case 'win+':
		be_app->PostMessage(msg);
		break;
	case 'win?':
		msg->AddPointer("window", this);
		be_app->PostMessage(msg);
		break;
	}	
}


status_t bTermWindow::Reader()
{
	char read_buf[BUFSIZE];
	int len;
		
	while((len = read(fd, read_buf, BUFSIZE)) > 0){
		if(Lock()){
			tv->Write(read_buf,len);
			Unlock();
		}
	}
	
	PostMessage(B_QUIT_REQUESTED);
	return B_OK;
}

bTermWindow::bTermWindow(int argc, char *argv[])
 : BWindow(BRect(0,0,1,1), "bterm", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		   B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	rgb_color black = { 0,0,0 };
	BView *v = new BView(BRect(0,0,1,1),"border",B_FOLLOW_ALL,B_WILL_DRAW);
	
	AddChild(v);
	
	tv = new bTermView(80,24);
	v->AddChild(tv);
	tv->MakeFocus();
	BRect r = tv->Bounds();
	
	MoveTo(100,100);
	ResizeTo(r.Width()+6,r.Height()+4);
	v->ResizeTo(r.Width()+6,r.Height()+4);
	tv->MoveTo(3,2);
	v->SetViewColor(black);
	fd = -1;
	
	AddShortcut('n',B_COMMAND_KEY,new BMessage('win+'));
	AddShortcut('d',B_COMMAND_KEY,new BMessage('win?'));
	RemoveShortcut('q',B_COMMAND_KEY);
	
	if(argc){
		char **args = (char **) malloc(sizeof(char*)*(argc+1));
		int i;
		for(i=0;i<argc;i++) args[i] = argv[i];
		args[i] = NULL;
		fd = spawn_proc(args);
		free(args);
	} else {
		fd = spawn_proc(default_args);
	}	
	if(fd >= 0){
		tv->SetFD(fd);
		resume_thread(spawn_thread(bTermWindow::reader,"pty reader",B_NORMAL_PRIORITY,this));
	}
	
}

