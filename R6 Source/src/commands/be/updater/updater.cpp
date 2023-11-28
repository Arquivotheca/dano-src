
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <TextControl.h>
#include <StringView.h>
#include <TextView.h>
#include <Button.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

class UpdaterWindow : public BWindow
{
	BTextControl *vol;
	BTextControl *dir;
	BTextView *txt;
	BButton *button;
	bool interactive;
	bool updating;
	bool alldone;
		
public:
	UpdaterWindow(bool interactive, const char *volname);
	void SetVolume(const char *string);
	void SetDirectory(const char *string);
	virtual void MessageReceived(BMessage *msg);
};

void
UpdaterWindow::SetVolume(const char *string)
{
	Lock();	
	vol->SetText(string);
	Unlock();
}

void
UpdaterWindow::SetDirectory(const char *string)
{
	Lock();
	dir->SetText(string);
	Unlock();
}

const char *msg_running =
"This volume is being updated to fix some file ownership and permission issues "
"that exist in the earlier version of BeOS it was created under.  This process "
"can take a few minutes, but will not harm the volume.  Please wait.";

const char *msg_interactive =
"This volume was created in a previous version of BeOS.  It needs to be updated "
"to insure that file ownership and permissions are correct for this version. "
"This will take a few minutes, but is important to insure that everything works "
"correctly.\n\n"
"You may opt not to do it -- you'll continue to get this offer each "
"time the volume is mounted until it is updated. ";

UpdaterWindow::UpdaterWindow(bool _interactive, const char *volname) 
	: BWindow(BRect(100,100,500,300), "Volume Updater", B_TITLED_WINDOW_LOOK,
				B_NORMAL_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_CLOSABLE |
				B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE)
{
	interactive = _interactive;
	updating = false;
	alldone = false;
	
	BView *bkg = new BView(Bounds(),"",0,0);
	bkg->SetViewColor(222,219,222);
	AddChild(bkg);

	BRect r = Bounds(); r.InsetBy(10,10);
	vol = new BTextControl(r, "", "Volume:", volname, NULL);
//	vol->SetEnabled(false);
	vol->SetDivider(60);
	bkg->AddChild(vol);

	r = Bounds(); r.InsetBy(10,40);
	dir = new BTextControl(r, "", "Directory:", volname, NULL);
//	dir->SetEnabled(false);
	dir->SetDivider(60);
	bkg->AddChild(dir);

	r = Bounds(); r.InsetBy(10,10); r.top += 60; r.bottom -=30;
	txt = new BTextView(r, "", r, 0);
	txt->SetTextRect(txt->Bounds());
	txt->SetText(interactive ? msg_interactive : msg_running);
	txt->SetViewColor(222,219,222);
	txt->MakeEditable(false);
	bkg->AddChild(txt);

	if(interactive){
		button = new BButton(BRect(320,170,390,190),"","Start",new BMessage('PUSH'));
		button->MakeDefault(true);
		
		bkg->AddChild(button);
	}
}

void start_updating(void);

void UpdaterWindow::MessageReceived(BMessage *msg) 
{
	if(msg->what == 'PUSH'){
		if(alldone){
			be_app->Quit(); //XXX is this right?
		}			
		if(interactive){
			if(!updating) {
				button->SetEnabled(false);
				button->SetLabel("Busy");
				txt->SetText(msg_running);
				updating = true;
				start_updating();
			}				
		}
		return;
	}
	if(msg->what == 'D0NE'){
		if(interactive) {
			button->SetLabel("Exit");
			button->SetEnabled(true);
			txt->SetText("Update is complete.");
			dir->SetText("");
			alldone = true;
		} else {
			be_app->Quit(); //XXX is this right?
		}
		return;
	}
	
}

class UpdaterApp : public BApplication
{
	bool interactive;
public:
	UpdaterApp(bool interactive, const char *volname);
	UpdaterWindow *window;
	virtual void ReadyToRun();
};

void 
UpdaterApp::ReadyToRun()
{
	if(!interactive) start_updating();
}


UpdaterApp::UpdaterApp(bool interactive, const char *volname) 
  : BApplication("application/x-vnd.be-volume-updater") 
{
	this->interactive = interactive;
	window = new UpdaterWindow(interactive,volname);
	window->Show();		
}

UpdaterWindow *win;

void set_volume(const char *volume)
{
	win->SetVolume(volume);
}

void set_directory(const char *dir)
{
	win->SetDirectory(dir);
}

void FixVolumes(int force, int isboot, const char *);

static int force = 0;
static int isboot = 0;
static char *volname = NULL;

status_t update_thread(void *data)
{
	setgid(0);	
	setuid(0);	
	FixVolumes(force,isboot,(const char *) data);
	win->PostMessage('D0NE');
	return 0;
}

void start_updating(void)
{
	resume_thread(spawn_thread(update_thread,"updater_thread",B_NORMAL_PRIORITY,volname));
}

int main(int argc, char *argv[])
{
	bool interactive = false;
	
	if(argc == 3){
		if(!strcmp(argv[1],"-force")){
			force = 1;
			isboot = 0;
		} else if(!strcmp(argv[1],"-boot")) {
			force = 0;
			isboot = 1;
		} else if(!strcmp(argv[1],"-update")) {
			force = 0;
			isboot = 0;
		} else if(!strcmp(argv[1],"-interactive")) {
			force = 0;
			isboot = 0;
			interactive = true;
		} else {
			fprintf(stderr,"Be Internal Tool. No user servicable parts inside\n");
			exit(0);
		}
		volname = argv[2];
		UpdaterApp app(interactive,volname);
		win = app.window;
		app.Run();
	}
}



