/*
	Camera.cpp
	Main application and window.
	All communication between the UI elements and the camera is
	handled through the main window class, CameraWnd.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <Application.h>
#include <Alert.h>
#include <Button.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <ScrollView.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <FilePanel.h>
#include <TranslationUtils.h>
#include <Screen.h>
#include "appconfig.h"
#include "BmpUtils.h"
#include "CameraView.h"
#include "SharedCamera.h"
#include "PhotoPC.h"
#include "KodakDC260.h"
#include "KodakDC260USB.h"

const char		*gAppName = STR_APPNAME;
const char		*gAppSig = STR_APPSIG;
const char		*gPrefsFile = STR_PREFSFILE;
TPreferences	*gPrefs;

#define NUM_SUPPORTED_CAM_TYPES 3

class PatternPanel : public BFilePanel {
public:
	PatternPanel(BMessenger *messenger) : BFilePanel(
						B_SAVE_PANEL,
						messenger,
						NULL,
						0,
						false,
						NULL,
						NULL,
						true)
	{
		int32			c;
		float			extraSpace = 24;
		BView			*v, *v2;
		BTextControl	*tc;
		BTextView		*tv;
		BWindow			*wnd = Window();
		BRect			r;

		if (wnd && wnd->Lock())
		{
			v = wnd->ChildAt(0); // background view
			// The following children, accessed by number, are not
			// documented in the BeBook - I hope things don't break later on...
			v2 = v->ChildAt(2);
			if (v2)
				v2->ResizeBy(0, -extraSpace); // file list
			v2 = v->ChildAt(4);
			if (v2)
				v2->MoveBy(0, -extraSpace); // h scrollbar
			v2 = v->ChildAt(5);
			if (v2)
				v2->ResizeBy(0, -extraSpace); // v scrollbar
			v2 = v->ChildAt(9);
			if (v2)
				v2->MoveBy(0, -extraSpace); // item counter

			v = wnd->FindView("text view");
			r = v->Frame();
			v->MoveBy(0, -extraSpace);
			tc = (BTextControl *)v;
			tc->SetDivider(72);
			tc->SetLabel(STR_BASENAME);
			v = v->Parent();
			tc = new BTextControl(r, "base num", STR_BASENUM, "1", NULL, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
			tc->SetDivider(72);
			tv = (BTextView *)(tc->ChildAt(0));
			tv->SetMaxBytes(7);
			for (c = 0; c < 256; c++)
			{
				if (!isdigit(c))
					tv->DisallowChar(c);
			}
			v->AddChild(tc);

			wnd->Unlock();
		}
	}
	~PatternPanel()
	{
	}
	int32 GetBaseNum()
	{
		BTextControl	*tc;
		const char		*buffer;
		int32			val = 1;
		BWindow			*wnd = Window();

		if (wnd && wnd->Lock())
		{
			tc = (BTextControl *)(wnd->FindView("base num"));
			buffer = tc->Text();
			sscanf(buffer, "%ld", &val);
			wnd->Unlock();
		}
		return val;
	}
};

class CameraWnd : public BWindow {
public:
	CameraWnd();
	~CameraWnd();

	void Quit();
	void MessageReceived(BMessage *msg);
	void MenusBeginning();
private:
	void SelectAll();
	void UpdateSelection();
	void GetThumbnail(BMessage *msg);
	void SaveToDir(entry_ref dir, const char *baseName=NULL, int32 startNum=-1);
	void CopyToTracker(BMessage *msg);

	// send requests to camera
	void CmdProbe();
	void CmdConnect();
	void CmdSave(BMessage *msg);
	void CmdDelete();
	// handle responses from camera
	void DidProbe(BMessage *msg);
	void DidConnect(BMessage *msg);
	void DidDelete(BMessage *msg);

	bool			fInOperation;
	bool			fCameraToProbe[NUM_SUPPORTED_CAM_TYPES];
	bool			fCameraToConnect[NUM_SUPPORTED_CAM_TYPES];
	bool			fConnectButtonPressed;
	bool			fInitialProbe;
	BLooper			*fCamera[NUM_SUPPORTED_CAM_TYPES];
	int32			fActiveCam;
	BFilePanel		*fSaveSinglePanel;
	PatternPanel	*fSaveMultiPanel;
};

CameraWnd::CameraWnd() :
	BWindow(BRect(80, 80, 80 + 430, 80 + 254), gAppName, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BRect		r;

	fSaveSinglePanel = NULL;
	fSaveMultiPanel = NULL;

	BMenuBar	*menuBar = new BMenuBar(Bounds(), "keyBar");
	AddChild(menuBar);
	BMenu		*fileMenu = new BMenu(STR_FILE);
	fileMenu->AddItem(new BMenuItem(STR_SAVE, new BMessage(MSG_SAVE), 'S'));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(STR_ABOUT, new BMessage(B_ABOUT_REQUESTED)));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(STR_QUIT, new BMessage(B_QUIT_REQUESTED), 'Q'));
	menuBar->AddItem(fileMenu);

	ResizeTo(Frame().Width(), Frame().Height() + menuBar->Frame().Height() + 1);
	SetSizeLimits(Frame().Width(), 10000.0f, Frame().Height(), 10000.0f);

	if (gPrefs->GetRect("CameraWndFrame", &r) == B_NO_ERROR)
		ResizeTo(r.Width(), r.Height());

	fCamera[0] = new PhotoPC;
	fCamera[1] = new KodakDC260;
	fCameraToProbe[0] = fCameraToProbe[1] = false;
	fCameraToConnect[0] = fCameraToConnect[1] = false;
	fCamera[2] = new KodakDC260USB;
	fCameraToProbe[2] = false;
	fCameraToConnect[2] = false;
	fConnectButtonPressed = false;
	fInitialProbe = false;
	fActiveCam = -1;
	fInOperation = false;

	r = Bounds();
	r.top = menuBar->Frame().bottom + 1;
	AddChild(new CameraView(r));

	// center the window
	BScreen		screen(this);
	BRect		wndR = Frame();
	r = screen.Frame();
	MoveTo((r.left + r.Width() / 2) - wndR.Width() / 2,
		(r.top + r.Height() / 2) - wndR.Height() / 2);
	Show();

	PostMessage(MSG_PROBE_USB);

//	fCameraToProbe[2] = true;
//	CmdProbe();
}

CameraWnd::~CameraWnd()
{
	if (fSaveSinglePanel)
		delete fSaveSinglePanel;
	if (fSaveMultiPanel)
		delete fSaveMultiPanel;
}

void CameraWnd::Quit()
{
	int32		i, n;
	BMenu		*menu;
	BMenuField	*mf;

	mf = (BMenuField *)FindView("SerialPort");
	if (mf != NULL)
	{
		menu = mf->Menu();
		n = menu->CountItems();
		for (i = 0; i < n; i++)
		{
			if (menu->ItemAt(i)->IsMarked())
				break;
		}
		if (i < n)
			gPrefs->SetInt32("port", i);
	}

	mf = (BMenuField *)FindView("SerialSpeed");
	if (mf != NULL)
	{
		menu = mf->Menu();
		n = menu->CountItems();
		for (i = 0; i < n; i++)
		{
			if (menu->ItemAt(i)->IsMarked())
				break;
		}
		if (i < n)
			gPrefs->SetInt32("speed", i);
	}

	gPrefs->SetRect("CameraWndFrame", Frame());

	for(i=0; i<NUM_SUPPORTED_CAM_TYPES; i++) {
		if (fCamera[i]->LockWithTimeout(5000000) == B_NO_ERROR)
			fCamera[i]->Quit();
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	BWindow::Quit();
}

void CameraWnd::MessageReceived(BMessage *msg)
{
	BAlert		*alert;
	void		*ptr;
	const char	*dir;
	PictureList	*list;
	int 		i;

	switch (msg->what)
	{
		case MSG_PROBE:
			for(i=0; i<NUM_SUPPORTED_CAM_TYPES; i++) {
				fCameraToProbe[i] = true;
			}
			CmdProbe();
			break;
		case MSG_PROBE_USB:
			fCameraToProbe[2] = true;
			fInitialProbe = true;
			CmdProbe();
			break;
		case MSG_CONNECT:
			fConnectButtonPressed = true;
			for(i=0; i<NUM_SUPPORTED_CAM_TYPES; i++) {
				fCameraToConnect[i] = true;
			}
			CmdConnect();
			break;
		case MSG_SELALL:
			SelectAll();
			break;
		case MSG_SAVE:
			list = (PictureList *)FindView("Pictures");
			if (list->CountSelection() == 1)
			{
				if (fSaveSinglePanel == NULL)
				{
					fSaveSinglePanel = new BFilePanel(
						B_SAVE_PANEL,
						new BMessenger(this),
						NULL,
						0,
						false,
						NULL,
						NULL,
						true
					);
				}
				if (gPrefs->GetString("SaveDir", &dir) == B_NO_ERROR)
					fSaveSinglePanel->SetPanelDirectory(dir);
				fSaveSinglePanel->Show();
			}
			else if (list->CountSelection() > 1)
			{
				if (fSaveMultiPanel == NULL)
					fSaveMultiPanel = new PatternPanel(new BMessenger(this));
				if (gPrefs->GetString("SaveDir", &dir) == B_NO_ERROR)
					fSaveMultiPanel->SetPanelDirectory(dir);
				fSaveMultiPanel->Show();
			}
			break;
		case B_CANCEL:
			if (msg->FindPointer("source", &ptr) == B_NO_ERROR)
			{
				if (ptr == fSaveSinglePanel && fSaveSinglePanel != NULL)
				{
					entry_ref	ref;
					fSaveSinglePanel->GetPanelDirectory(&ref);
					BEntry		entry(&ref);
					BPath		path(&entry);
					gPrefs->SetString("SaveDir", path.Path());
					delete fSaveSinglePanel;
					fSaveSinglePanel = NULL;
				}
				if (ptr == fSaveMultiPanel && fSaveMultiPanel != NULL)
				{
					entry_ref	ref;
					fSaveMultiPanel->GetPanelDirectory(&ref);
					BEntry		entry(&ref);
					BPath		path(&entry);
					gPrefs->SetString("SaveDir", path.Path());
					delete fSaveMultiPanel;
					fSaveMultiPanel = NULL;
				}
			}
			break;
		case B_SAVE_REQUESTED:
			CmdSave(msg);
			break;
		case B_COPY_TARGET:
			CopyToTracker(msg);
			break;
		case MSG_DELETE:
		case B_TRASH_TARGET:
			alert = new BAlert(gAppName, STR_CONFIRMDELETE, STR_YES, STR_NO);
			if (alert->Go() == 0)
				CmdDelete();
			break;
		case MSG_UPDATESEL:
			UpdateSelection();
			break;

		case CAM_PROBE:
			DidProbe(msg);
			break;
		case CAM_QUERY:
			DidConnect(msg);
			break;
		case CAM_DELETE:
			DidDelete(msg);
			break;
		case CAM_THUMB:
			GetThumbnail(msg);
			break;
		case B_ABOUT_REQUESTED:
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;
		case MSG_USB:
			{
				BMenuField *mf = (BMenuField *)FindView("SerialSpeed");
				if (mf != NULL) {
					BMenu *menu = mf->Menu();
					menu->SetEnabled(false);
				}
			}
			break;
		case MSG_SERIALPORT:
			{
				BMenuField *mf = (BMenuField *)FindView("SerialSpeed");
				if (mf != NULL) {
					BMenu *menu = mf->Menu();
					menu->SetEnabled(true);
				}
			}
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}

void CameraWnd::MenusBeginning()
{
	BMenuBar	*menuBar = (BMenuBar *)FindView("keyBar");
	BMenu		*fileMenu = menuBar->SubmenuAt(0);
	fileMenu->ItemAt(0)->SetEnabled(((BButton *)FindView("Save"))->IsEnabled());
}

void CameraWnd::SelectAll()
{
	PictureList		*list = (PictureList *)FindView("Pictures");

	if (list != NULL)
	{
		int32		i, n = list->CountPictures();

		for (i = 0; i < n; i++)
			list->Select(i);
		list->Invalidate();
	}
	UpdateSelection();
}

void CameraWnd::UpdateSelection()
{
	PictureList		*list = (PictureList *)FindView("Pictures");

	if (list != NULL)
	{
		int32		i, n = list->CountPictures();
		bool		hasSel = false;

		for (i = 0; i < n; i++)
		{
			if (list->IsSelected(i))
			{
				hasSel = true;
				break;
			}
		}
		((BButton *)FindView("Save"))->SetEnabled(hasSel);
		((BButton *)FindView("Delete"))->SetEnabled(hasSel);
	}
}

void CameraWnd::GetThumbnail(BMessage *msg)
{
	int32			i;
	PictureList		*list = (PictureList *)FindView("Pictures");

	if (msg->FindInt32("frame", &i) == B_NO_ERROR)
	{ // response from the camera
		const char	*path;

		if (msg->FindString("path", &path) == B_NO_ERROR)
		{
			BBitmap		*img = LoadBitmap(path);

			if (img != NULL)
			{
				list->SetThumbnail(i, img);
				delete img;
			}
		}
		PostMessage(CAM_THUMB);
	}
	else
	{ // ask the camera
		int32			n;
		BMessage		message(CAM_THUMB);
		char			path[128];

		n = list->CountPictures();
		for (i = 0; i < n; i++)
		{
			if (!list->HasThumbnail(i))
			{
				sprintf(path, "/tmp/Camera_thumb_%s", list->PictureName(i));
				message.AddString("path", path);
				message.AddInt32("frame", i);
				fCamera[fActiveCam]->PostMessage(&message, fCamera[fActiveCam], this);
				break;
			}
		}
	}
}

void CameraWnd::SaveToDir(entry_ref dir, const char *baseName, int32 startNum)
{
	BMessage	message(CAM_SAVE);
	char		name[256];

	message.AddRef("dir", &dir);

	PictureList		*list = (PictureList *)FindView("Pictures");

	if (list != NULL)
	{
		int32		i, n, id;
	
		n = list->CountPictures();
		for (i = 0, id = 0; i < n; i++)
		{
			if (list->IsSelected(i))
			{
				message.AddInt32("frame", i);
				if (baseName != NULL)
				{
					if (startNum == -1)
					{ // only one being saved - use the baseName as its name
						message.AddString("name", baseName);
					}
					else
					{ // multi-save - construct a name from the base and a number
						sprintf(name, "%s.%03ld.jpg", baseName, startNum + id);
						message.AddString("name", name);
						id++;
					}
				}
				else
					message.AddString("name", list->PictureName(i));
			}
		}
	}
	fCamera[fActiveCam]->PostMessage(&message, fCamera[fActiveCam], this);
}

void CameraWnd::CopyToTracker(BMessage *msg)
{
	const char	*type, *name;
	entry_ref	dir;

	if (msg->FindString("be:types", &type) == B_NO_ERROR)
	{
		if (strcasecmp(type, B_FILE_MIME_TYPE) == 0)
		{
			if (msg->FindRef("directory", &dir) == B_NO_ERROR &&
			msg->FindString("name", &name) == B_NO_ERROR)
			{
				// erase the clipping file - it isn't useful for this app
				BDirectory	d(&dir);
				BEntry		entry(&d, name);
				entry.Remove();
				SaveToDir(dir);
			}
		}
	}
}

void CameraWnd::CmdProbe()
{
	if (fInOperation)
		return;

	BMessage	message(CAM_PROBE);

	((BStringView *)FindView("Make"))->SetText(STR_UNKNOWN_MAKE);
	((BStringView *)FindView("Model"))->SetText(STR_UNKNOWN_MODEL);
	((BStringView *)FindView("Version"))->SetText(STR_UNKNOWN_VERSION);
	((BStringView *)FindView("NumPictures"))->SetText(STR_UNKNOWN_NUMPICS);
	((PictureList *)FindView("Pictures"))->Empty();
	((BButton *)FindView("SelAll"))->SetEnabled(false);
	((BButton *)FindView("Save"))->SetEnabled(false);
	((BButton *)FindView("Delete"))->SetEnabled(false);
	
	// Loop through a list of flags to send the probe command to
	// any cameras that haven't been probed yet
	for(int i=0; i<NUM_SUPPORTED_CAM_TYPES;i++) {	
		if(fCameraToProbe[i] == true) {
			fCameraToProbe[i] = false;
			message.AddInt32("cam_num", i);
			fCamera[i]->PostMessage(&message, fCamera[i], this);
			fInOperation = true;
			return;
		}
	}
	// We made it through the list of cameras and they all have
	// been probed. Therefore there are no cameras connected.
	if(fInitialProbe == false) {
		BAlert	*alert = new BAlert(STR_APPNAME, STR_FAILEDCONNECT, STR_OK);
		alert->Go();
	}
	fInOperation = false;
	fInitialProbe = false;
}

void CameraWnd::CmdConnect()
{
	if (fInOperation)
		return;

	int32		i, n, camera;
	BMessage	message(CAM_QUERY);
	BMenu		*menu;
	BMenuField	*mf;

	((BStringView *)FindView("Make"))->SetText(STR_UNKNOWN_MAKE);
	((BStringView *)FindView("Model"))->SetText(STR_UNKNOWN_MODEL);
	((BStringView *)FindView("Version"))->SetText(STR_UNKNOWN_VERSION);
	((BStringView *)FindView("NumPictures"))->SetText(STR_UNKNOWN_NUMPICS);
	((PictureList *)FindView("Pictures"))->Empty();
	((BButton *)FindView("SelAll"))->SetEnabled(false);
	((BButton *)FindView("Save"))->SetEnabled(false);
	((BButton *)FindView("Delete"))->SetEnabled(false);

	// find which serial port the user chose
	mf = (BMenuField *)FindView("SerialPort");
	if (mf != NULL)
	{
		menu = mf->Menu();
		n = menu->CountItems();
		for (i = 0; i < n; i++)
		{
			if (menu->ItemAt(i)->IsMarked())
			{
				message.AddString("port", menu->ItemAt(i)->Label());
				break;
			}
		}
	}
	// find what port speed the user chose
	mf = (BMenuField *)FindView("SerialSpeed");
	if (mf != NULL)
	{
		menu = mf->Menu();
		n = menu->CountItems();
		for (i = 0; i < n; i++)
		{
			if (menu->ItemAt(i)->IsMarked())
			{
				message.AddInt32("portSpeed", i);
				break;
			}
		}
	}

	// Find the first camera that is slated to be connected to
	for(i=0; i<NUM_SUPPORTED_CAM_TYPES; i++) {
		if(fCameraToConnect[i] == true) {
			fCameraToConnect[i] = false;
			message.AddInt32("cam_num", i);
			fCamera[i]->PostMessage(&message, fCamera[i], this);
			fInOperation = true;
			return;
		}
	}
	// We made it through the list of cameras and they all have
	// been attempted to connect to. There must not be any cameras attached.
	if(fConnectButtonPressed) {
		fConnectButtonPressed = false;
		BAlert	*alert = new BAlert(STR_APPNAME, STR_FAILEDCONNECT, STR_OK);
		alert->Go();
	}
	fInOperation = false;
}

void CameraWnd::CmdSave(BMessage *msg)
{
	entry_ref	ref;
	const char	*fileName;
	BEntry		entry;
	BPath		path;
	int32		startNum = -1;

	if (msg->FindRef("directory", &ref) != B_NO_ERROR)
		return;
	if (msg->FindString("name", &fileName) != B_NO_ERROR)
		return;
	entry.SetTo(&ref);
	entry.GetPath(&path);
	gPrefs->SetString("SaveDir", path.Path());
	if (fSaveMultiPanel != NULL)
		startNum = fSaveMultiPanel->GetBaseNum();
	SaveToDir(ref, fileName, startNum);
}

void CameraWnd::CmdDelete()
{
	if (fInOperation)
		return;

	BMessage	message(CAM_DELETE);
	PictureList	*list = (PictureList *)FindView("Pictures");

	if (list != NULL)
	{
		int32		i, n;

		n = list->CountPictures();
		for (i = n - 1; i >= 0; i--)
		{
			if (list->IsSelected(i))
				message.AddInt32("frame", i);
		}
	}
	fCamera[fActiveCam]->PostMessage(&message, fCamera[fActiveCam], this);
	fInOperation = true;
}

void CameraWnd::DidProbe(BMessage *msg)
{
	int32		i, n, cam_num;
	const char	*port;
	BMenuField	*mf;
	BMenu		*menu;
	int32 		portspeed;

	fInOperation = false;
	if (msg->FindString("port", &port) == B_NO_ERROR)
	{
//		printf("port = '%s'\n", port);
			
		// set the serial port the the one found during probing
		mf = (BMenuField *)FindView("SerialPort");
		if (mf != NULL)
		{
			menu = mf->Menu();
			n = menu->CountItems();
			for (i = 0; i < n; i++)
			{
				if (strcmp(menu->ItemAt(i)->Label(), port) == 0)
					menu->ItemAt(i)->SetMarked(true);
				else
					menu->ItemAt(i)->SetMarked(false);
			}
		}
		if(msg->FindInt32("portSpeed", &portspeed) == B_NO_ERROR) {
			mf = (BMenuField *)FindView("SerialSpeed");
			if (mf != NULL)
			{
				menu = mf->Menu();
				n = menu->CountItems();
				if(portspeed < n) {
					for (i = 0; i < n; i++)
					{
						menu->ItemAt(i)->SetMarked(false);
					}
					menu->ItemAt(portspeed)->SetMarked(true);
				}
			}
		}

		// If it was USB, post this message, which
		// should disable the serial speed menu
		if(strcmp(port, "USB") == 0) {
			PostMessage(MSG_USB);	
		} else {
			PostMessage(MSG_SERIALPORT);
		}
		// If the camera returned a ok, go ahead and connect to it
		// This makes the camera app connect to the first one it sees
		{
			if (msg->FindInt32("cam_num", 0, &cam_num) != B_NO_ERROR)
				CmdProbe();
			fCameraToConnect[cam_num] = true;
			CmdConnect();
		}
//		PostMessage(MSG_CONNECT);
	} else {
		// didn't connect, continue probing
		CmdProbe();
	}
}

void CameraWnd::DidConnect(BMessage *msg)
{
	int32		n;
	const char	*inStr;
	char		outStr[256];
	PictureList	*list;

	fInOperation = false;
	if (msg->FindInt32("numFrames", &n) != B_NO_ERROR) {
		// It must have not connected
		CmdConnect();
		return;
	}
	
	if (msg->FindInt32("cam_num", &fActiveCam) != B_NO_ERROR) {
		CmdConnect();
		return;
	}
	
	if (msg->FindString("make", &inStr) == B_NO_ERROR)
	{
		sprintf(outStr, STR_KNOWN_MAKE, inStr);
		((BStringView *)FindView("Make"))->SetText(outStr);
	}
	if (msg->FindString("model", &inStr) == B_NO_ERROR)
	{
		sprintf(outStr, STR_KNOWN_MODEL, inStr);
		((BStringView *)FindView("Model"))->SetText(outStr);
	}
	if (msg->FindString("version", &inStr) == B_NO_ERROR)
	{
		sprintf(outStr, STR_KNOWN_VERSION, inStr);
		((BStringView *)FindView("Version"))->SetText(outStr);
	}
	sprintf(outStr, STR_KNOWN_NUMPICS, n);
	((BStringView *)FindView("NumPictures"))->SetText(outStr);

	list = (PictureList *)FindView("Pictures");
	list->Empty();
	// If the camera returns a list of filenames,
	// pre-fill the PictureList with the filenames
	{
		int i;
		
		if (list != NULL) {
			for(i=0; msg->FindString("filename", i, &inStr) == B_NO_ERROR; i++) {
				char *temp = new char[strlen(inStr) + 1];
				strcpy(temp, inStr);
				list->AddFileName(temp);
			}
		}
	}
	list->CreateNewList(n);

	if (n > 0)
		((BButton *)FindView("SelAll"))->SetEnabled(true);
	PostMessage(CAM_THUMB); // begin the thumbnail capture process
}

void CameraWnd::DidDelete(BMessage *msg)
{
	int32		i, frm, n;
	char		outStr[256];
	PictureList	*list = (PictureList *)FindView("Pictures");

	fInOperation = false;
	if (list != NULL)
	{
		for (i = 0; msg->FindInt32("frame", i, &frm) == B_NO_ERROR; i++)
			list->DeletePicture(frm);
	}

	n = list->CountPictures();
	if (n == 0)
	{
		((BButton *)FindView("SelAll"))->SetEnabled(false);
	}
	UpdateSelection();

	sprintf(outStr, STR_KNOWN_NUMPICS, n);
	((BStringView *)FindView("NumPictures"))->SetText(outStr);
}

#include "AboutText.h"

class AboutWnd;

AboutWnd		*gAboutWnd = NULL;

class AboutWnd : public BWindow {
public:
	AboutWnd() :
		BWindow(BRect(0, 0, 400, 300), NULL, B_MODAL_WINDOW_LOOK,
			B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
	{
		BRect		r, r2;
		BTextView	*tv;
		BButton		*but;
		BView		*back;

		back = new BView(Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW);
		back->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		r = Bounds();
		r.bottom -= 40;
		r.right -= B_V_SCROLL_BAR_WIDTH;
		r2 = r;
		r2.InsetBy(8, 8);
		tv = new BTextView(r, NULL, r2, B_FOLLOW_ALL, B_WILL_DRAW);
		tv->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		tv->MakeEditable(false);
		tv->MakeSelectable(false);
		tv->SetWordWrap(true);
		tv->SetText(gAboutText);
		back->AddChild(new BScrollView(NULL, tv, B_FOLLOW_ALL, 0, false, true, B_FANCY_BORDER));
		r = Bounds();
		r.bottom -= 8;
		r.top = r.bottom - 24;
		r.left += 160;
		r.right -= 160;
		but = new BButton(r, NULL, STR_OK, new BMessage(B_QUIT_REQUESTED));
		back->AddChild(but);
		but->MakeDefault(true);

		AddChild(back);

		// center the window
		BScreen		screen(this);
		BRect		wndR = Frame();
		r = screen.Frame();
		MoveTo((r.left + r.Width() / 2) - wndR.Width() / 2,
			(r.top + r.Height() / 2) - wndR.Height() / 2);
		Show();
	}
	~AboutWnd()
	{
	}
	void Quit()
	{
		gAboutWnd = NULL;
		BWindow::Quit();
	}
};

class Camera : public BApplication {
public:
	Camera();
	~Camera();

	void ReadyToRun();
	void AboutRequested();
private:
	CameraWnd		*fWnd;
};

Camera::Camera() :
	BApplication(gAppSig)
{
}

Camera::~Camera()
{
}

void Camera::ReadyToRun()
{
	fWnd = new CameraWnd;
}

void Camera::AboutRequested()
{
	if (gAboutWnd == NULL)
		gAboutWnd = new AboutWnd();
	else
		gAboutWnd->Activate();
}

int main()
{
	gPrefs = new TPreferences(gPrefsFile);
	Camera		app;
	app.Run();
	delete gPrefs;
	return 0;
}
