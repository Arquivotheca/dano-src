#include "SettingsWindow.h"
#include "TTabView.h"
#include "TCheckBox.h"
#include "LabelView.h"
#include "LogWindow.h"
#include "LogPrefsView.h"
#include "NameDialog.h"

#include "SettingsManager.h"
#include "MFilePanel.h"

#include "Util.h"
#include "RegInfoView.h"

#define MODULE_DEBUG 0

#include "MyDebug.h"
#include <ClassInfo.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <Button.h>
#include <Application.h>
#include <Path.h>
#include <RadioButton.h>
#include <MenuField.h>

SettingsWindow::SettingsWindow(SettingsManager *_set)
	:	BWindow(BRect(0,0,380,0),"SoftwareValet Settings",
					B_TITLED_WINDOW,
					B_NOT_RESIZABLE | B_NOT_ZOOMABLE, 0/*NULL*/),
		settings(_set)
{
	Lock();
	PositionWindow(this,settings->data.FindPoint("general/setwindow"),0.5,0.5);
	AddChild(new SettingsView(Bounds()));

	Unlock();	
	Show();
}

SettingsWindow::~SettingsWindow()
{
	settings->SaveSettings();
}

bool SettingsWindow::QuitRequested()
{
	TTabView *tview = (TTabView *)FindView("tabview");
	settings->data.ReplaceInt16("general/prefpanel",tview->CurrentView());
	settings->data.ReplacePoint("general/setwindow",Frame().LeftTop());
	
	/** if we are closing and a text view is in focus then
		we must make sure the text is saved first!!! ***/
	BTextView *f = cast_as(CurrentFocus(),BTextView);
	if (f) {
		f->MakeFocus(FALSE);
		PostMessage(B_QUIT_REQUESTED,this);
		return false;	// could be bad
	}
	else {
		return BWindow::QuitRequested();
	}
}

void SettingsWindow::WindowActivated(bool state)
{
	if (state == FALSE) {
		BTextView *f = cast_as(CurrentFocus(),BTextView);
		if (f) {
			f->MakeFocus(FALSE);
			f->MakeFocus(TRUE);
		}
	}
	BWindow::WindowActivated(state);
}

void ItemInvoke(BMenuItem *it);
void ItemInvoke(BMenuItem *it)
{
	BMessage *msg = new BMessage(*it->Message());
	msg->AddDouble("when",system_time());
	msg->AddPointer("source",it);
	msg->AddInt32("index",it->Menu()->IndexOf(it));
	
	it->Target()->Looper()->PostMessage(msg,it->Target());	
}

bool GetBoolControlValue(BMessage *m)
{
	BControl *p;
	m->FindPointer("source",(void **)&p);
	
	ASSERT(p);
	
	bool state = p->Value() == B_CONTROL_ON;
	return state;
}

/*********************************************************/

SettingsView::SettingsView(BRect frame)
	:	BView(frame,"settingsview",B_FOLLOW_ALL,B_WILL_DRAW)
{
}

void SettingsView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	SetViewColor(210,210,210);

	BRect r = Bounds();
	r.InsetBy(5,5);
	
	TTabView *tview = new TTabView(r,"tabview",
							B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(tview);
	tview->SetViewColor(210,210,210);
		
	BRect cr = tview->Bounds();
	//cr.InsetBy(12,12);
	//cr.top += 14;

	tview->AddChild(new DownloadSetView(cr));
	tview->AddChild(new CommSetView(cr));
	tview->AddChild(new InstallSetView(cr));
	tview->AddChild(new RegisterSetView(cr));
//	tview->AddChild(new UpdateSetView(cr));
//	tview->AddChild(new UninstallSetView(cr));
	tview->AddChild(new LogPrefsView(cr));
	
	tview->ActivateView(gSettings->data.FindInt16("general/prefpanel"));
}	

/////////////////////// Download Settings /////////////////////

DownloadSetView::DownloadSetView(BRect fr)
	:	ResizeView(fr,"Download",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(light_gray_background);
	
}

void DownloadSetView::AttachedToWindow()
{
	TRACE();

	ResizeView::AttachedToWindow();

	TRACE();

	BRect r = Bounds();
	r.InsetBy(12,12);
	r.bottom = r.top + 14;
	TRACE();
	
	BStringView *sv = new BStringView(r,"",
						"Place all downloads in:");
	AddChild(sv);
	sv->SetViewColor(ViewColor());
	
	r.OffsetBy(0,20);
	r.left += 16;
	sv = new BStringView(r,"dlpath",B_EMPTY_STRING);
	AddChild(sv);
	sv->SetViewColor(ViewColor());
	/* init */
	TRACE();

	sv->SetText(gSettings->data.FindString("download/path"));

	TRACE();
	
	r.OffsetBy(0,20);
	r.left += 20;
	r.right = r.left + 90;
	r.bottom = r.top + 20;
	
	TRACE();

	BButton *btn = new BButton(r,"selfolder",
				"Select Folder...",new BMessage(S_DLPATH));
		TRACE();
	AddChild(btn);
		TRACE();
	btn->SetFont(be_plain_font);
		TRACE();
		// crashes here
	btn->SetTarget(this,NULL);

	TRACE();
	
	BRect br = r;
	br.left = br.right + 40;
	br.right = br.left + 90;
	
	TRACE();

	btn = new BButton(br,"showlog","Display Log...",new BMessage('SLog'));
	AddChild(btn);
	btn->SetFont(be_plain_font);
	btn->SetTarget(be_app);

	TRACE();

	uint32 dlflag = gSettings->data.FindInt32("download/flags");
	
	r.top = r.bottom + 12;
	r.left -= 36;
	r.bottom = r.top + 16;
	r.right = Bounds().right - 12;
	
	//BCheckBox *cb = new BCheckBox(r,"autorestart",
	//						"Automatically resume downloads at Transceiver restart",
	//						new BMessage(S_RESUME));
	//TRACE();

	//AddChild(cb);
	//cb->SetFont(be_plain_font);
	//cb->SetTarget(this);
	//cb->SetValue(dlflag & SettingsManager::DL_AUTORESUME);
	//cb->SetEnabled(false);
	
	//r.OffsetBy(0,24);
	BCheckBox *cb;
	
	cb = new BCheckBox(r,"autoopen",
							"Automatically open installer upon completion",
							new BMessage(S_AUTOLAUNCH));
	AddChild(cb);
	cb->SetFont(be_plain_font);
	cb->SetTarget(this);
	cb->SetValue(dlflag & SettingsManager::DL_AUTOLAUNCH);
	
	winHeight = r.bottom + 50;

	TRACE();

	// inital resize of window
	SizeWindow();
}

void DownloadSetView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case S_DLPATH: {
			PRINT(("select download path\n"));
			// can only have one file panel at a time || close panel when done
			//if (be_app->IsFilePanelRunning())
			//	be_app->CloseFilePanel();
			status_t err;
			entry_ref	ref;
			err = msg->FindRef("refs",&ref);
			if (err == B_NO_ERROR) {
				PRINT(("got message from file panel\n"));
				BDirectory dir(&ref);
				err = dir.InitCheck();
				if (err >= B_NO_ERROR) {
					BEntry	dEntry(&ref);
					
					BPath	pathObj;
					dEntry.GetPath(&pathObj);
					
					BStringView *s = cast_as(FindView("dlpath"),BStringView);
					s->SetText(pathObj.Path());
					
					gSettings->data.ReplaceString("download/path",pathObj.Path());
				}
			}
			else {
				PRINT(("need to open file panel\n"));
				if (!TryActivate(panelWind)) {
					BFilePanel *pan = new MFilePanel(B_OPEN_PANEL,
													new BMessenger(this),
													NULL,
													true,
													true,
													msg->what);
					panelWind = BMessenger(pan->Window());
					pan->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
					pan->Window()->SetTitle("Select folder for downloads");
					pan->Show();
				}
			}
			break;
		}
		case S_RESUME: {
			uint32 dlflags = gSettings->data.FindInt32("download/flags");
			bool v = GetBoolControlValue(msg);
			if (v) dlflags |= SettingsManager::DL_AUTORESUME;
			else dlflags &= ~SettingsManager::DL_AUTORESUME;
			gSettings->data.ReplaceInt32("download/flags",dlflags);
			break;
		}
		case S_AUTOLAUNCH: {
			uint32 dlflags = gSettings->data.FindInt32("download/flags");
			bool v = GetBoolControlValue(msg);
			if (v) dlflags |= SettingsManager::DL_AUTOLAUNCH;
			else dlflags &= ~SettingsManager::DL_AUTOLAUNCH;
			gSettings->data.ReplaceInt32("download/flags",dlflags);
			break;
		}
		default:
			ResizeView::MessageReceived(msg);
			break;
	}
}

/////////////////////// Install Settings /////////////////////

InstallSetView::InstallSetView(BRect fr)
	:	ResizeView(fr,"Install",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(light_gray_background);
}

void InstallSetView::AttachedToWindow()
{
	ResizeView::AttachedToWindow();

	BRect r = Bounds();
	r.InsetBy(12,8);
	
	r.left += 10;
	r.bottom = r.top + 16;
	
	///// create log files checkbox //////
	BCheckBox *cb = new BCheckBox(r,"autolog","Automatically create log file",
							new BMessage(S_AUTOLOG));					
	AddChild(cb);
	cb->SetFont(be_plain_font);
	/* init */
	cb->SetValue(gSettings->data.FindBool("install/log"));
	cb->SetTarget(this);
	
	BRect br = r;
	br.OffsetBy(18,14);
	
	///// place log files in: string view /////
	BStringView *sv = new BStringView(br,B_EMPTY_STRING,
						"Place log files in:");
	AddChild(sv);
	sv->SetViewColor(ViewColor());
	
	br.OffsetBy(0,14);
	br.left += 16;
	br.right = Bounds().right - 110;
	sv = new BStringView(br,"logpath",B_EMPTY_STRING);
	AddChild(sv);
	sv->SetViewColor(ViewColor());
	/* init */
	sv->SetText(gSettings->data.FindString("install/logpath"));
	
	//br.OffsetBy(0,20);
	br.right = Bounds().right - 10;
	br.left = br.right - 90;
	br.right = br.left + 90;
	br.top -= 8;
	br.bottom = br.top + 20;

	BButton *btn = new BButton(br,"selfolder",
				"Select Folder...",new BMessage(S_LOGPATH));
	AddChild(btn);
	btn->SetFont(be_plain_font);
	btn->SetTarget(this);
	
	//////////////////////////////////
	r.OffsetBy(0,58);
	//////////////////////////////////
	
	cb = new BCheckBox(r,"usefolder","Install to selected default folder",
							new BMessage(S_USEINSTPATH));
	cb->SetFont(be_plain_font);
	AddChild(cb);
	cb->SetTarget(this);
	/* init */
	cb->SetValue(gSettings->data.FindBool("install/usepath"));
	
	br = r;
	br.OffsetBy(18,14);
							
	sv = new BStringView(br,B_EMPTY_STRING,
						"Default install folder:");
	AddChild(sv);
	sv->SetViewColor(ViewColor());
	
	br.OffsetBy(0,14);
	br.left += 16;
	br.right = Bounds().right - 110;
	sv = new BStringView(br,"installpath",B_EMPTY_STRING);
	AddChild(sv);
	sv->SetViewColor(ViewColor());
	/* init */
	sv->SetText(gSettings->data.FindString("install/path"));
	
	//br.OffsetBy(0,20);
	br.right = Bounds().right - 10;
	br.left = br.right - 90;
	br.right = br.left + 90;
	br.top -= 8;
	br.bottom = br.top + 20;

	btn = new BButton(br,B_EMPTY_STRING,
				"Select Folder...",new BMessage(S_INSTPATH));
	AddChild(btn);
	btn->SetFont(be_plain_font);
	btn->SetTarget(this);

	//////////////////////////////////
	r.OffsetBy(0,64);
	//////////////////////////////////
	cb = new BCheckBox(r,"autopreview","Preview files before installing",
					new BMessage(S_IPREVIEW));
	AddChild(cb);
	cb->SetFont(be_plain_font);
	/* init */
	cb->SetValue(gSettings->data.FindBool("install/preview"));
	cb->SetTarget(this);


	// winHeight = r.bottom + 100;
	r.OffsetBy(0,30);
	
	BRadioButton *rb = new BRadioButton(r,"libsys",
			"Install shared libraries to home/lib",new BMessage(S_LIBTOSYS));
	AddChild(rb);
	rb->SetFont(be_plain_font);
	rb->SetEnabled(FALSE);
	rb->SetTarget(this);

	r.OffsetBy(0,18);
	rb = new BRadioButton(r,"libapp","Install shared libraries to install folder",
					new BMessage(S_LIBTOAPP));
	AddChild(rb);
	rb->SetFont(be_plain_font);
	rb->SetEnabled(FALSE);
	rb->SetTarget(this);
	
	winHeight = r.bottom + 50;
}

void InstallSetView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case S_AUTOLOG: {
			gSettings->data.ReplaceBool("install/log",GetBoolControlValue(msg));
			break;
		}
		case S_USEINSTPATH: {
			gSettings->data.ReplaceBool("install/usepath",GetBoolControlValue(msg));
			break;
		}
		case S_IPREVIEW: {
			gSettings->data.ReplaceBool("install/preview",GetBoolControlValue(msg));
			break;
		}
		case S_LOGPATH:
		case S_INSTPATH: {
			if (msg->HasRef("refs")) {
				entry_ref	dRef;
				msg->FindRef("refs",&dRef);
				BDirectory d(&dRef);
				if (d.InitCheck() >= B_NO_ERROR) {
					BEntry	e;
					d.GetEntry(&e);
					
					BPath	pathObj;
					e.GetPath(&pathObj);

					BView *v;
					BStringView *s;
					if (msg->what == S_LOGPATH) {
						v = FindView("logpath");
						gSettings->data.ReplaceString("install/logpath",pathObj.Path());
					}
					else {
						v = FindView("installpath");
						gSettings->data.ReplaceString("install/path",pathObj.Path());
					}
					s = cast_as(v,BStringView);
					s->SetText(pathObj.Path());
				}
			}
			else {
				if (msg->what == S_LOGPATH) {
					if (!TryActivate(logPanel)) {
						BFilePanel *pan = new MFilePanel(B_OPEN_PANEL,
												new BMessenger(this),
												NULL, true, false, msg->what);
						logPanel = BMessenger(pan->Window());
						pan->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
						pan->Window()->SetTitle("Select folder for install logs");
						pan->Show();
					}
				}
				else {
					if (!TryActivate(instPanel)) {
						BFilePanel *pan = new MFilePanel(B_OPEN_PANEL,
												new BMessenger(this),
												NULL, true, false, msg->what);
						instPanel = BMessenger(pan->Window());
						pan->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
						pan->Window()->SetTitle("Select default installation folder");
						pan->Show();
					}
				}
			}
			break;
		}
		case S_LIBTOSYS:
		case S_LIBTOAPP:
			break;
		default:
			ResizeView::MessageReceived(msg);
			break;
	}
}

void InstallSetView::Draw(BRect r)
{
	ResizeView::Draw(r);
	BRect b = Bounds();
	BRect f = FindView("autopreview")->Frame();
	DrawHSeparator(b.left + 4,b.right - 4 ,f.top - 4,this);
}

/////////////////////// Register Settings /////////////////////

RegisterSetView::RegisterSetView(BRect fr)
	:	ResizeView(fr,"Register",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(light_gray_background);	
}

void RegisterSetView::AttachedToWindow()
{
	ResizeView::AttachedToWindow();
		
		
	const long rbheight = 17;
	
	BRect r = Bounds();
	r.InsetBy(8,8);
	r.bottom = r.top + 36;

	AddChild(new LabelView(r,"When installing software, your can choose to have your registration\
 information automatically sent to the software developer."));
	

	r.left += 20;
	
	r.top = r.bottom;
	r.bottom = r.top + rbheight;
	
	const char *regopts[] = {"Automatically prompt for registration",
							"Do not register"};
	
	/* init */
	long curMode = gSettings->data.FindInt32("register/mode");	
	for (long i = 0; i < nel(regopts); i++) {
		BMessage *msg = new BMessage(S_REGMODE);
		msg->AddInt32("mode",i);
		
		BRadioButton *rb = new BRadioButton(r,B_EMPTY_STRING,
								regopts[i], msg);
		AddChild(rb);
		rb->SetFont(be_plain_font);
		rb->SetTarget(this);

		/* init */		
		if (i == curMode)
			rb->SetValue(B_CONTROL_ON);
			
		r.OffsetBy(0,rbheight);
	}
	
	r.left -= 20;
	r.OffsetBy(0,10);

	///////////////////////////
	srect = r;
	///////////////////////////
	r.OffsetBy(0,5);
	
	r.bottom = r.top + 0;	// view will set its own height
	r.right = r.left + 212;
	
	BView *v = new RegInfoView(r,&gSettings->reg,&gSettings->reg);
	AddChild(v);
	
	winHeight = v->Frame().bottom + 40;
	
	///////////////////////////
	SetupValues();
}

void RegisterSetView::SetupValues()
{
}


void RegisterSetView::Draw(BRect up)
{
	ResizeView::Draw(up);
	DrawHSeparator(srect.left,srect.right,srect.top,this);
}


void RegisterSetView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case S_REGMODE:
			gSettings->data.ReplaceInt32("register/mode",msg->FindInt32("mode"));
			break;
		default:
			ResizeView::MessageReceived(msg);
			break;
	}
}


UpdateSetView::UpdateSetView(BRect fr)
	:	ResizeView(fr,"Update",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(light_gray_background);	
}

void UpdateSetView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BRect r = Bounds();
	r.InsetBy(5,8);
	r.bottom = r.top + 30;

	AddChild(new LabelView(r,"SoftwareValet can automatically check\
 for new versions of software and report when new versions are found."));
 	r.top = r.bottom + 10;

	r.left += 5;
	r.bottom = r.top + 16;
	r.right = r.left + 200;
	BPopUpMenu	*menu;
	
	menu = new BPopUpMenu("servers");
	menu->AddItem(new BMenuItem(gSettings->data.FindString("comm/servername"),
								new BMessage('????')));
	//menu->AddItem(new BMenuItem("www.beware.com",new BMessage('????')));
	//menu->AddItem(new BMenuItem("www.beatware.com",new BMessage('????')));
	menu->AddSeparatorItem();
	//menu->AddItem(new BMenuItem("All available",new BMessage('????')));
	menu->AddItem(new BMenuItem("Other...",new BMessage(S_SET_SERVER)));
	//menu->AddItem(new BMenuItem("Remove Current",new BMessage('????')));
	menu->SetTargetForItems(this);

	BMenuField *mf = new BMenuField(r,"checkservers","Check server",menu);
	AddChild(mf);
	mf->MenuBar()->SetFont(be_plain_font);
	mf->SetFont(be_plain_font);
	mf->SetDivider(75);
	menu->SetFont(be_plain_font);
	
	menu = new BPopUpMenu("time");
	menu->AddItem(new BMenuItem("Hourly",new BMessage(S_CHECK_FREQ)));
	menu->AddItem(new BMenuItem("Daily",new BMessage(S_CHECK_FREQ)));
	menu->AddItem(new BMenuItem("Weekly",new BMessage(S_CHECK_FREQ)));
	menu->AddItem(new BMenuItem("Never",new BMessage(S_CHECK_FREQ)));		
	menu->SetTargetForItems(this);
	
	BRect nr = r;
	nr.left = nr.right + 12;
	nr.right = nr.left + 120;
	mf = new BMenuField(nr,"checktimes",B_EMPTY_STRING,menu);
	AddChild(mf);
	mf->MenuBar()->SetFont(be_plain_font);
	mf->SetFont(be_plain_font);
	mf->SetDivider(0);
	menu->SetFont(be_plain_font);
	
	r.OffsetBy(0,30);
	
	r.right = r.left + 120;
	menu = new BPopUpMenu("day");
	menu->AddItem(new BMenuItem("Sunday",new BMessage(S_SELECT_DAY)));
	menu->AddItem(new BMenuItem("Monday",new BMessage(S_SELECT_DAY)));
	menu->AddItem(new BMenuItem("Tuesday",new BMessage(S_SELECT_DAY)));
	menu->AddItem(new BMenuItem("Wednesday",new BMessage(S_SELECT_DAY)));
	menu->AddItem(new BMenuItem("Thursday",new BMessage(S_SELECT_DAY)));
	menu->AddItem(new BMenuItem("Friday",new BMessage(S_SELECT_DAY)));
	menu->AddItem(new BMenuItem("Saturday",new BMessage(S_SELECT_DAY)));
	menu->SetTargetForItems(this);
	mf = new BMenuField(r,"checkday","On",menu);
	AddChild(mf);
	mf->MenuBar()->SetFont(be_plain_font);
	mf->SetFont(be_plain_font);
	mf->SetDivider(20);
	menu->SetFont(be_plain_font);
	
	nr = r;
	nr.left = nr.right + 10;
	nr.right = nr.left + 65;
	
	menu = new BPopUpMenu("hour");
	for (long i = 1; i <= 12; i++) {
		char buf[12];
		sprintf(buf,"%d:00",i);
		menu->AddItem(new BMenuItem(buf,new BMessage(S_SELECT_HOUR)));
	}
	menu->SetTargetForItems(this);
	mf = new BMenuField(nr,"checkhour","At",menu);
	AddChild(mf);
	mf->MenuBar()->SetFont(be_plain_font);
	mf->SetFont(be_plain_font);
	mf->SetDivider(20);
	menu->SetFont(be_plain_font);
	
	nr.left = nr.right + 8;
	nr.right = nr.left + 40;
	menu = new BPopUpMenu("ampm");
	menu->AddItem(new BMenuItem("AM",new BMessage(S_SELECT_AMPM)));
	menu->AddItem(new BMenuItem("PM",new BMessage(S_SELECT_AMPM)));
	menu->SetTargetForItems(this);
	mf = new BMenuField(nr,"checkampm",B_EMPTY_STRING,menu);
	AddChild(mf);
	mf->MenuBar()->SetFont(be_plain_font);
	mf->SetFont(be_plain_font);
	mf->SetDivider(0);
	menu->SetFont(be_plain_font);
	
	////////////////////
	
	winHeight = nr.bottom + 60;
	SetupDefaults();
}

void UpdateSetView::SetupDefaults()
{
	/** default selections **/
	SettingsManager *settings = gSettings;
	BMenuField *mf;
	long value;
	
	// server choices
	mf = cast_as(FindView("checkservers"),BMenuField);
	mf->Menu()->ItemAt(0)->SetMarked(TRUE);
	
	// hour || day || week || never
	long freq = settings->data.FindInt32("comm/checkfreq");
	mf = cast_as(FindView("checktimes"),BMenuField);
	mf->Menu()->ItemAt(freq)->SetMarked(TRUE);
	// ItemInvoke(menu->ItemAt(freq));
	
	// day
	value = settings->data.FindInt16("comm/checkday");
	mf = cast_as(FindView("checkday"),BMenuField);
	mf->Menu()->ItemAt(value)->SetMarked(TRUE);
	if (freq == EVERY_HOUR || freq == EVERY_DAY || freq == CHECK_NEVER)
		mf->Hide();

	// hour	
	curHr = settings->data.FindInt16("comm/checkhr");
	long hour;
	if (curHr == 0)
		hour = 11; // menu item 11
	else
		hour = (curHr-1) % 12;
		
	mf = cast_as(FindView("checkhour"),BMenuField);
	mf->Menu()->ItemAt(hour)->SetMarked(TRUE);
	if (freq == EVERY_HOUR || freq == CHECK_NEVER)
		mf->Hide();

	// ampm
	mf = cast_as(FindView("checkampm"),BMenuField);
	
	pmHr = (curHr >= 12);
	
	mf->Menu()->ItemAt(pmHr)->SetMarked(TRUE);
	if (freq == EVERY_HOUR || freq == CHECK_NEVER)
		mf->Hide();
}

void UpdateSetView::SetDayEnabled(bool state)
{
	BMenuField *v = (BMenuField *)FindView("checkday");
	if (state) {
		if (v->IsHidden()) {
			v->Show();
			v->MenuBar()->Invalidate();
		}
	}
	else {
		if (!v->IsHidden())
			v->Hide();
	}
}

void UpdateSetView::SetHourEnabled(bool state)
{
	BMenuField *v = (BMenuField *)FindView("checkhour");
	BMenuField *av = (BMenuField *)FindView("checkampm");
	if (state) {
		if (v->IsHidden()) v->Show();
		if (av->IsHidden()) av->Show();
		v->MenuBar()->Invalidate();
		av->MenuBar()->Invalidate();
	}
	else {
		if (!v->IsHidden()) v->Hide();
		if (!av->IsHidden()) av->Hide();
	}
}

void UpdateSetView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case S_CHECK_FREQ: {
			PRINT(("got check freq msg\n"));
			short f = msg->FindInt32("index");
			
			gSettings->data.ReplaceInt32("comm/checkfreq",f);
			
			switch(f) {
				case EVERY_HOUR:
					SetDayEnabled(FALSE);
					SetHourEnabled(FALSE);
					break;
				case EVERY_DAY:
					SetDayEnabled(FALSE);
					SetHourEnabled(TRUE);
					// enable hr
					break;
				case EVERY_WEEK:
					SetHourEnabled(TRUE);
					SetDayEnabled(TRUE);
					break;
				case CHECK_NEVER:
					SetDayEnabled(FALSE);
					SetHourEnabled(FALSE);
					break;
			}
			break;
		}
		case S_SELECT_DAY: {
			gSettings->data.ReplaceInt16("comm/checkday",msg->FindInt32("index"));
			break;
		}
		case S_SELECT_HOUR: {
			int f = msg->FindInt32("index");
			curHr = (f + 1) % 12 + (pmHr ? 12 : 0);
			PRINT(("%d",curHr));
			gSettings->data.ReplaceInt16("comm/checkhr",curHr);
			break;
		}
		case S_SELECT_AMPM: {
			pmHr = msg->FindInt32("index") == 1;
			
			if (pmHr && curHr < 12)
				curHr += 12;
			else if (!pmHr && curHr > 12)
				curHr -= 12;		
			gSettings->data.ReplaceInt16("comm/checkhr",curHr);
			break;
		}
		case S_SET_SERVER: {
			if (msg->HasString("text")) {
				BMenuField *mf;
				mf = cast_as(FindView("checkservers"),BMenuField);
				
				mf->Menu()->ItemAt(0)->SetLabel(msg->FindString("text"));
				mf->Menu()->ItemAt(0)->SetMarked(true);
				
				gSettings->data.ReplaceString("comm/servername",msg->FindString("text"));
			}
			else {
				BMessage *response = new BMessage(S_SET_SERVER);
				NameDialog *nd = new NameDialog(BRect(0,0,300,90),
												"Server address",
												gSettings->data.FindString("comm/servername"),
												response,
												this);
			}
			break;
		}
		default:
			ResizeView::MessageReceived(msg);
			break;
	}
}

// -------------- UninstallSetView ----------------

UninstallSetView::UninstallSetView(BRect fr)
	:	ResizeView(fr,"Uninstall",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(light_gray_background);
	
}

void UninstallSetView::AttachedToWindow()
{
	ResizeView::AttachedToWindow();
	
	BRect r = Bounds();
	r.InsetBy(18,18);
	r.bottom = r.top + 18;

	BMessage *msg;

	msg = new BMessage(S_DELETEMODE);
	msg->AddInt32("index",SettingsManager::UNINST_TRASH);
	BRadioButton *rb = new BRadioButton(r,"movetrash",
							"Move uninstalled files to Trash",
							msg);
	AddChild(rb);
	rb->SetFont(be_plain_font);
	rb->SetTarget(this);
	rb->SetEnabled(false);

	/* init */
	// long mode;
	//mode = gSettings->GetUninstMode();
	//if (mode == SettingsManager::UNINST_TRASH)
	//	rb->SetValue(B_CONTROL_ON);

	r.OffsetBy(0,18);
	
	msg = new BMessage(S_DELETEMODE);
	msg->AddInt32("index",SettingsManager::UNINST_DELETE);
	rb = new BRadioButton(r,"delete","Immediately delete files",msg);
	AddChild(rb);
	rb->SetFont(be_plain_font);
	rb->SetTarget(this);
	rb->SetEnabled(false);
	
	/* init */
	//if (mode == SettingsManager::UNINST_DELETE)
	//	rb->SetValue(B_CONTROL_ON);
		
	r.OffsetBy(0,24);
	
	BCheckBox *cb = new BCheckBox(r,"archive",
							"Copy files to archive for undo",
							new BMessage(S_UNINSTARC));
	AddChild(cb);
	cb->SetFont(be_plain_font);
	cb->SetTarget(this);
	cb->SetEnabled(false);
		
	winHeight = r.bottom + 40;
}

void UninstallSetView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case S_DELETEMODE:
		{
			//gSettings->SetUninstMode(msg->FindInt32("index"));
			break;
		}
		case S_UNINSTARC:
		{
			bool v = GetBoolControlValue(msg);
			//uint32 f = gSettings->GetUninstFlags();
			//if (v) f |= SettingsManager::UNINST_ARCHIVE;
			//else f &= ~SettingsManager::UNINST_ARCHIVE;
			//gSettings->SetUninstFlags(f);
			break;
		}
		default:
		{
			ResizeView::MessageReceived(msg);
			break;
		}
	}
}


//////////////////// Resize View ////////////////////////

ResizeView::ResizeView(	BRect frame,
				const char *name,
				ulong resizeMask,
				ulong flags)
	:	BView(frame,name,resizeMask,flags),
		winHeight(-1)
{
}

void ResizeView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (winHeight < 0)
		winHeight = Window()->Frame().Height();
}

void ResizeView::Show()
{
	BView::Show();
	SizeWindow();	
}

void ResizeView::SizeWindow()
{
	Window()->ResizeTo(Window()->Frame().Width(),winHeight);
}
