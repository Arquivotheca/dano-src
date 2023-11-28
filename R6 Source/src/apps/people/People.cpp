//--------------------------------------------------------------------
//	
//	People.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include "People.h"

#include <Clipboard.h>
#include <Roster.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//====================================================================

int main(void)
{	
	TPeopleApp	*app;

	app = new TPeopleApp();
	app->Run();

	delete app;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TPeopleApp::TPeopleApp(void)
		  :BApplication(APP_SIG)
{
	bool			valid = FALSE;
	const char		*str;
	int32			index = 0;
	BDirectory		dir;
	BEntry			entry;
	BMessage		msg;
	BMessage		info;
	BMimeType		mime;
	BPath			path;
	BPoint			pos;
	BVolume			vol;
	BVolumeRoster	roster;
	TPeopleWindow	*window;

	fHaveWindow = FALSE;

	fPosition.Set(6, TITLE_BAR_HEIGHT, 6 + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT);
	pos = fPosition.LeftTop();

	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	dir.SetTo(path.Path());
	if (dir.FindEntry("People_data", &entry) == B_NO_ERROR) {
		fPrefs = new BFile(&entry, O_RDWR);
		if (fPrefs->InitCheck() == B_NO_ERROR) {
			fPrefs->Read(&pos, sizeof(BPoint));
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(pos))
				fPosition.OffsetTo(pos);
		}
	}
	else {
		fPrefs = new BFile();
		if (dir.CreateFile("People_data", fPrefs) != B_NO_ERROR) {
			delete fPrefs;
			fPrefs = NULL;
		}
	}

	// Create indexes on each mounted volume that supports queries and attributes
	while (roster.GetNextVolume(&vol) == B_OK)
	{
		if (vol.KnowsAttr() && vol.KnowsQuery())
		{
			fs_create_index(vol.Device(), P_NAME, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_COMPANY, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_ADDRESS, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_CITY, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_STATE, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_ZIP, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_COUNTRY, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_HPHONE, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_WPHONE, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_FAX, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_EMAIL, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_URL, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_GROUP, B_STRING_TYPE, 0);
			fs_create_index(vol.Device(), P_NICKNAME, B_STRING_TYPE, 0);
		}
	}

	// install person mime type
	mime.SetType(B_PERSON_MIMETYPE);
	if (mime.IsInstalled()) {
		if (mime.GetAttrInfo(&info) == B_NO_ERROR) {
			while (info.FindString("attr:name", index++, &str) == B_NO_ERROR) {
				if (!strcmp(str, P_NAME)) {
					valid = TRUE;
					break;
				}
			}
			if (!valid)
				mime.Delete();
		}
	}
	if (!valid) {
		mime.Install();
		mime.SetShortDescription("Person");
		mime.SetLongDescription("Contact information for a person.");
		mime.SetPreferredApp(APP_SIG);

		// add relevant person fields to meta-mime type
		msg.AddString("attr:public_name", "Contact Name"); 
		msg.AddString("attr:name", P_NAME); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Company"); 
		msg.AddString("attr:name", P_COMPANY); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Address"); 
		msg.AddString("attr:name", P_ADDRESS); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "City"); 
		msg.AddString("attr:name", P_CITY); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 90); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "State"); 
		msg.AddString("attr:name", P_STATE); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 50); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Zip"); 
		msg.AddString("attr:name", P_ZIP); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 50); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Country"); 
		msg.AddString("attr:name", P_COUNTRY); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Home Phone"); 
		msg.AddString("attr:name", P_HPHONE); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 90); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Work Phone"); 
		msg.AddString("attr:name", P_WPHONE); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 90); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Fax"); 
		msg.AddString("attr:name", P_FAX); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 90); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "E-mail"); 
		msg.AddString("attr:name", P_EMAIL); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "URL"); 
		msg.AddString("attr:name", P_URL); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Group"); 
		msg.AddString("attr:name", P_GROUP); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		msg.AddString("attr:public_name", "Nickname"); 
		msg.AddString("attr:name", P_NICKNAME); 
		msg.AddInt32("attr:type", B_STRING_TYPE); 
		msg.AddBool("attr:viewable", true); 
		msg.AddBool("attr:editable", true); 
		msg.AddInt32("attr:width", 120); 
		msg.AddInt32("attr:alignment", B_ALIGN_LEFT); 
		msg.AddBool("attr:extra", false); 

		mime.SetAttrInfo(&msg);
	}
}

//--------------------------------------------------------------------

TPeopleApp::~TPeopleApp(void)
{
	if (fPrefs)
		delete fPrefs;
}

//--------------------------------------------------------------------

void TPeopleApp::AboutRequested(void)
{
	(new BAlert("", "...by Robert Polic", "Big Deal"))->Go();
}

//--------------------------------------------------------------------

void TPeopleApp::ArgvReceived(int32 argc, char **argv)
{
	char			*arg;
	int32			index;
	int32			loop;
	TPeopleWindow	*window = NULL;

	for (loop = 1; loop < argc; loop++) {
		arg = argv[loop];
		if (!strncmp(P_NAME, arg, strlen(P_NAME)))
			index = F_NAME;
		else if (!strncmp(P_COMPANY, arg, strlen(P_COMPANY)))
			index = F_COMPANY;
		else if (!strncmp(P_ADDRESS, arg, strlen(P_ADDRESS)))
			index = F_ADDRESS;
		else if (!strncmp(P_CITY, arg, strlen(P_CITY)))
			index = F_CITY;
		else if (!strncmp(P_STATE, arg, strlen(P_STATE)))
			index = F_STATE;
		else if (!strncmp(P_ZIP, arg, strlen(P_ZIP)))
			index = F_ZIP;
		else if (!strncmp(P_COUNTRY, arg, strlen(P_COUNTRY)))
			index = F_COUNTRY;
		else if (!strncmp(P_HPHONE, arg, strlen(P_HPHONE)))
			index = F_HPHONE;
		else if (!strncmp(P_WPHONE, arg, strlen(P_WPHONE)))
			index = F_WPHONE;
		else if (!strncmp(P_FAX, arg, strlen(P_FAX)))
			index = F_FAX;
		else if (!strncmp(P_EMAIL, arg, strlen(P_EMAIL)))
			index = F_EMAIL;
		else if (!strncmp(P_URL, arg, strlen(P_URL)))
			index = F_URL;
		else if (!strncmp(P_GROUP, arg, strlen(P_GROUP)))
			index = F_GROUP;
		else if (!strncmp(P_NICKNAME, arg, strlen(P_NICKNAME)))
			index = F_NICKNAME;
		else
			index = F_END;

		if (index != F_END) {
			if (!window)
				window = NewWindow();
			while(*arg != ' ')
				arg++;
			arg++;
			window->SetField(index, arg);
		}
	}
}

//--------------------------------------------------------------------

void TPeopleApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_NEW:
			NewWindow();
			break;

		default:
			BApplication::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

void TPeopleApp::RefsReceived(BMessage *msg)
{
	char			type[B_FILE_NAME_LENGTH];
	int32			item = 0;
	BFile			file;
	BNodeInfo		*node;
	entry_ref		ref;
	TPeopleWindow	*window;

	while (msg->HasRef("refs", item)) {
		msg->FindRef("refs", item++, &ref);
		if (window = FindWindow(ref))
			window->Activate(TRUE);
		else {
			file.SetTo(&ref, O_RDONLY);
			if (file.InitCheck() == B_NO_ERROR)
				NewWindow(&ref);
		}
	}
}

//--------------------------------------------------------------------

void TPeopleApp::ReadyToRun(void)
{
	if (!fHaveWindow)
		NewWindow();
}

//--------------------------------------------------------------------

TPeopleWindow* TPeopleApp::NewWindow(entry_ref *ref)
{
	TPeopleWindow	*window;

	window = new TPeopleWindow(fPosition, "New Person", ref);
	window->Show();
	fHaveWindow = TRUE;
	fPosition.OffsetBy(20, 20);

	if (fPosition.bottom > BScreen(B_MAIN_SCREEN_ID).Frame().bottom)
		fPosition.OffsetTo(fPosition.left, TITLE_BAR_HEIGHT);
	if (fPosition.right > BScreen(B_MAIN_SCREEN_ID).Frame().right)
		fPosition.OffsetTo(6, fPosition.top);

	return window;
}

//--------------------------------------------------------------------

TPeopleWindow* TPeopleApp::FindWindow(entry_ref ref)
{
	int32			index = 0;
	TPeopleWindow	*window;

	while (window = (TPeopleWindow *)WindowAt(index++)) {
		if ((window->FindView("PeopleView")) && (window->fRef) && (*(window->fRef) == ref))
			return window;
	}
	return NULL;
}


//====================================================================

TPeopleWindow::TPeopleWindow(BRect rect, char *title, entry_ref *ref)
			 :BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE |
													B_NOT_ZOOMABLE)
{
	BMenu		*menu;
	BMenuBar	*menu_bar;
	BMenuItem	*item;
	BRect		r;

	fPanel = NULL;

	r.Set(0, 0, 32767, 15);
	menu_bar = new BMenuBar(r, "");
	menu = new BMenu("File");
	menu->AddItem(item = new BMenuItem("New Person", new BMessage(M_NEW), 'N'));
	item->SetTarget(NULL, be_app);
	menu->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED), 'W'));
	menu->AddSeparatorItem();
	menu->AddItem(fSave = new BMenuItem("Save", new BMessage(M_SAVE), 'S'));
	fSave->SetEnabled(FALSE);
	menu->AddItem(new BMenuItem("Save As"B_UTF8_ELLIPSIS, new BMessage(M_SAVE_AS)));
	menu->AddItem(fRevert = new BMenuItem("Revert", new BMessage(M_REVERT), 'R'));
	fRevert->SetEnabled(FALSE);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	menu_bar->AddItem(menu);

	menu = new BMenu("Edit");
	menu->AddItem(fUndo = new BMenuItem("Undo", new BMessage(M_UNDO), 'Z'));
	menu->AddSeparatorItem();
	menu->AddItem(fCut = new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
	fCut->SetTarget(NULL, this);
	menu->AddItem(fCopy = new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
	fCopy->SetTarget(NULL, this);
	menu->AddItem(fPaste = new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));
	fPaste->SetTarget(NULL, this);
	menu->AddItem(item = new BMenuItem("Select All", new BMessage(M_SELECT), 'A'));
	item->SetTarget(NULL, this);
	menu_bar->AddItem(menu);
	AddChild(menu_bar);

	if (ref) {
		fRef = new entry_ref(*ref);
		SetTitle(ref->name);
	}
	else
		fRef = NULL;

	r = Frame();
	r.OffsetTo(0, menu_bar->Bounds().bottom + 1);
	fView = new TPeopleView(r, "PeopleView", fRef);

	ResizeBy(0, menu_bar->Bounds().bottom + 1);
	Lock();
	AddChild(fView);
	Unlock();
}

//--------------------------------------------------------------------

TPeopleWindow::~TPeopleWindow(void)
{
	if (fRef)
		delete fRef;
	if (fPanel)
		delete fPanel;
}

//--------------------------------------------------------------------

void TPeopleWindow::MenusBeginning(void)
{
	bool			enabled;

	enabled = fView->CheckSave();
	fSave->SetEnabled(enabled);
	fRevert->SetEnabled(enabled);

	fUndo->SetEnabled(FALSE);
	enabled = fView->TextSelected();
	fCut->SetEnabled(enabled);
	fCopy->SetEnabled(enabled);

	be_clipboard->Lock();
	fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE));
	be_clipboard->Unlock();

	fView->BuildGroupMenu();
}

//--------------------------------------------------------------------

void TPeopleWindow::MessageReceived(BMessage* msg)
{
	char			str[256];
	const char		*name = NULL;
	entry_ref		dir;
	BDirectory		directory;
	BEntry			entry;
	BFile			file;
	BNodeInfo		*node;

	switch(msg->what) {
		case M_SAVE:
			if (!fRef) {
				SaveAs();
				break;
			}
		case M_REVERT:
		case M_SELECT:
			fView->MessageReceived(msg);
			break;

		case M_SAVE_AS:
			SaveAs();
			break;

		case M_GROUP_MENU:
			msg->FindString("group", &name);
			fView->SetField(F_GROUP, name, FALSE);
			break;

		case B_SAVE_REQUESTED:
			if (msg->FindRef("directory", &dir) == B_NO_ERROR) {
				msg->FindString("name", &name);
				directory.SetTo(&dir);
				if (directory.InitCheck() == B_NO_ERROR) {
					directory.CreateFile(name, &file);
					if (file.InitCheck() == B_NO_ERROR) {
						node = new BNodeInfo(&file);
						node->SetType("application/x-person");
						delete node;

						directory.FindEntry(name, &entry);
						entry.GetRef(&dir);
						if (fRef)
							delete fRef;
						fRef = new entry_ref(dir);
						SetTitle(fRef->name);
						fView->NewFile(fRef);
					}
					else {
						sprintf(str, "Could not create %s.", name);
						(new BAlert("", str, "Sorry"))->Go();
					}
				}
			}
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

bool TPeopleWindow::QuitRequested(void)
{
	int32			count = 0;
	int32			index = 0;
	BPoint			pos;
	BRect			r;
	status_t		result;
	TPeopleWindow	*window;

	if (fView->CheckSave()) {
		char		name[B_FILE_NAME_LENGTH];
		char		buf[B_FILE_NAME_LENGTH + 100];
		DefaultName(name);
		sprintf(buf, "Save changes to the person named \"%s\"?", name);
		result = (new BAlert("", buf, "Cancel", "Don't Save", "Save", 
							B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT))->Go();
		if (result == 2) {
			if (fRef)
				fView->Save();
			else {
				SaveAs();
				return FALSE;
			}
		}
		else if (result == 0)
			return FALSE;
	}

	while (window = (TPeopleWindow *)be_app->WindowAt(index++)) {
		if (window->FindView("PeopleView"))
			count++;
	}

	if (count == 1) {
		r = Frame();
		pos = r.LeftTop();
		if (((TPeopleApp*)be_app)->fPrefs) {
			((TPeopleApp*)be_app)->fPrefs->Seek(0, 0);
			((TPeopleApp*)be_app)->fPrefs->Write(&pos, sizeof(BPoint));
		}
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	return TRUE;
}

//--------------------------------------------------------------------

void TPeopleWindow::DefaultName(char *name)
{
	strncpy(name, fView->GetField(F_NAME), B_FILE_NAME_LENGTH);
	while (*name) {
		if (*name == '/')
			*name = '-';
		name++;
	}
}

//--------------------------------------------------------------------

void TPeopleWindow::SetField(int32 index, const char *text)
{
	fView->SetField(index, text, TRUE);
}

//--------------------------------------------------------------------

void TPeopleWindow::SaveAs(void)
{
	char		name[B_FILE_NAME_LENGTH];
	BDirectory	dir;
	BEntry		entry;
	BMessenger	window(this);
	BPath		path;

	DefaultName(name);
	if (!fPanel) {
		fPanel = new BFilePanel(B_SAVE_PANEL, &window);
		fPanel->SetSaveText(name);
		find_directory(B_USER_DIRECTORY, &path, true);
		dir.SetTo(path.Path());
		if (dir.FindEntry("people", &entry) == B_NO_ERROR)
			fPanel->SetPanelDirectory(&entry);
		else if (dir.CreateDirectory("people", &dir) == B_NO_ERROR) {
			dir.GetEntry(&entry);
			fPanel->SetPanelDirectory(&entry);
		}
	}
	else if (fPanel->Window()->Lock()) {
		if (!fPanel->Window()->IsHidden())
			fPanel->Window()->Activate();
		else
			fPanel->SetSaveText(name);
		fPanel->Window()->Unlock();
	}

	if (fPanel->Window()->Lock()) {
		if (fPanel->Window()->IsHidden())
			fPanel->Window()->Show();
		fPanel->Window()->Unlock();
	}	
}


//====================================================================

TPeopleView::TPeopleView(BRect rect, char *title, entry_ref *ref)
		   :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW)
{
	if (ref)
		fFile = new BFile(ref, O_RDWR);
	else
		fFile = NULL;
	_mLastEnd = 0;
}

//--------------------------------------------------------------------

TPeopleView::~TPeopleView(void)
{
	if (fFile)
		delete fFile;
}

//--------------------------------------------------------------------

void TPeopleView::AttachedToWindow(void)
{
	char		*text;
	float		offset;
	BBox		*box;
	BFont		font = *be_plain_font;
	BMenuField	*field;
	BRect		r;
	rgb_color	c;
	attr_info	info;

	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);

	r = Bounds();
//+	r.InsetBy(-1, -1);
	box = new BBox(r, "box", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(box);

	offset = font.StringWidth(HPHONE_TEXT) + 18;

	text = (char *)malloc(1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(NAME_TEXT) - 11, NAME_V,
		  NAME_H + NAME_WIDTH, NAME_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_NAME, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_NAME, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_NAME] = new TTextControl(r, NAME_TEXT, 256,
								text, M_DIRTY, M_NAME);
	fField[F_NAME]->SetTarget(this);
	box->AddChild(fField[F_NAME]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(NICKNAME_TEXT) - 11, NICKNAME_V,
		  NICKNAME_H + NICKNAME_WIDTH, NICKNAME_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_NICKNAME, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_NICKNAME, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_NICKNAME] = new TTextControl(r, NICKNAME_TEXT, 256,
								text, M_DIRTY, M_NICKNAME);
	fField[F_NICKNAME]->SetTarget(this);
	box->AddChild(fField[F_NICKNAME]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(COMPANY_TEXT) - 11, COMPANY_V,
		  COMPANY_H + COMPANY_WIDTH, COMPANY_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_COMPANY, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_COMPANY, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_COMPANY] = new TTextControl(r, COMPANY_TEXT, 256,
								text, M_DIRTY, M_COMPANY);
	fField[F_COMPANY]->SetTarget(this);
	box->AddChild(fField[F_COMPANY]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(ADDRESS_TEXT) - 11, ADDRESS_V,
		  ADDRESS_H + ADDRESS_WIDTH, ADDRESS_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_ADDRESS, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_ADDRESS, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_ADDRESS] = new TTextControl(r, ADDRESS_TEXT, 256,
								text, M_DIRTY, M_ADDRESS);
	fField[F_ADDRESS]->SetTarget(this);
	box->AddChild(fField[F_ADDRESS]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(CITY_TEXT) - 11, CITY_V,
		  CITY_H + CITY_WIDTH, CITY_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_CITY, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_CITY, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_CITY] = new TTextControl(r, CITY_TEXT, 256,
								text, M_DIRTY, M_CITY);
	fField[F_CITY]->SetTarget(this);
	box->AddChild(fField[F_CITY]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(STATE_TEXT) - 11, STATE_V,
		  STATE_H + STATE_WIDTH, STATE_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_STATE, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_STATE, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_STATE] = new TTextControl(r, STATE_TEXT, 256,
								text, M_DIRTY, M_STATE);
	fField[F_STATE]->SetTarget(this);
	box->AddChild(fField[F_STATE]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(ZIP_H + 11, ZIP_V,
		  ZIP_H + ZIP_WIDTH, ZIP_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_ZIP, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_ZIP, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_ZIP] = new TTextControl(r, ZIP_TEXT, 256,
								text, M_DIRTY, M_ZIP);
	fField[F_ZIP]->SetTarget(this);
	box->AddChild(fField[F_ZIP]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(COUNTRY_TEXT) - 11, COUNTRY_V,
		  COUNTRY_H + COUNTRY_WIDTH, COUNTRY_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_COUNTRY, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_COUNTRY, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_COUNTRY] = new TTextControl(r, COUNTRY_TEXT, 256,
								text, M_DIRTY, M_COUNTRY);
	fField[F_COUNTRY]->SetTarget(this);
	box->AddChild(fField[F_COUNTRY]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(HPHONE_TEXT) - 11, HPHONE_V,
		  HPHONE_H + HPHONE_WIDTH, HPHONE_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_HPHONE, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_HPHONE, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_HPHONE] = new TTextControl(r, HPHONE_TEXT, 256,
								text, M_DIRTY, M_HPHONE);
	fField[F_HPHONE]->SetTarget(this);
	box->AddChild(fField[F_HPHONE]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(WPHONE_TEXT) - 11, WPHONE_V,
		  WPHONE_H + WPHONE_WIDTH, WPHONE_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_WPHONE, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_WPHONE, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_WPHONE] = new TTextControl(r, WPHONE_TEXT, 256,
								text, M_DIRTY, M_WPHONE);
	fField[F_WPHONE]->SetTarget(this);
	box->AddChild(fField[F_WPHONE]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(FAX_TEXT) - 11, FAX_V,
		  FAX_H + FAX_WIDTH, FAX_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_FAX, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_FAX, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_FAX] = new TTextControl(r, FAX_TEXT, 256,
								text, M_DIRTY, M_FAX);
	fField[F_FAX]->SetTarget(this);
	box->AddChild(fField[F_FAX]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(EMAIL_TEXT) - 11, EMAIL_V,
		  EMAIL_H + EMAIL_WIDTH, EMAIL_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_EMAIL, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_EMAIL, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_EMAIL] = new TTextControl(r, EMAIL_TEXT, 256,
								text, M_DIRTY, M_EMAIL, C_EMAIL);
	fField[F_EMAIL]->SetTarget(this);
	box->AddChild(fField[F_EMAIL]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(URL_TEXT) - 11, URL_V,
		  URL_H + URL_WIDTH, URL_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_URL, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_URL, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_URL] = new TTextControl(r, URL_TEXT, 256,
								text, M_DIRTY, M_URL, C_URL);
	fField[F_URL]->SetTarget(this);
	box->AddChild(fField[F_URL]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - 11, GROUP_V,
		  GROUP_H + GROUP_WIDTH, GROUP_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_GROUP, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_GROUP, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_GROUP] = new TTextControl(r, "", 256,
								text, M_DIRTY, M_GROUP);
	fField[F_GROUP]->SetTarget(this);
	box->AddChild(fField[F_GROUP]);
	free(text);

	r.right = r.left + 3;
	r.left = r.right - font.StringWidth(GROUP_TEXT) - 26;
	r.top -= 1;
	fGroups = new BPopUpMenu("Group");
	fGroups->SetRadioMode(FALSE);
	field = new BMenuField(r, "", "", fGroups);
	field->SetDivider(0.0);
	field->SetFont(&font);
	field->SetEnabled(TRUE);
	box->AddChild(field);

	fField[F_NAME]->MakeFocus();
}

//--------------------------------------------------------------------

void TPeopleView::MessageReceived(BMessage* msg)
{
	int32		loop;
	BTextView	*text;

	switch (msg->what) {
		case M_SAVE:
			Save();
			break;

		case M_REVERT:
			for (loop = 0; loop < F_END; loop++)
				fField[loop]->Revert();
			break;

		case M_SELECT:
			for (loop = 0; loop < F_END; loop++) {
				text = (BTextView *)fField[loop]->ChildAt(0);
				if (text->IsFocus()) {
					text->Select(0, text->TextLength());
					break;
				}
			}
			break;
		case C_EMAIL: {
			const char * txt = fField[F_EMAIL]->Text();
			if (txt && *txt) {
				char * ptr = (char *)malloc(strlen(txt)+10);
				strcpy(ptr, "mailto:");
				strcat(ptr, txt);
				const char * argv[3] = { ptr, NULL };
				be_roster->Launch("text/x-email", 1, (char**)argv);
				free(ptr);
			}
			} break;
		case C_URL: {
			const char * txt = fField[F_URL]->Text();
			if (txt && *txt) {
				const char * argv[3] = { txt, NULL };
				be_roster->Launch("text/html", 1, (char**)argv);
			}
			} break;
		case M_DIRTY: {
			if (fField[F_GROUP]->TextView()->IsFocus()) {
				if (!fGroups->CountItems()) {
					BuildGroupMenu();
				}
				int32 from, to;
				fField[F_GROUP]->TextView()->GetSelection(&from, &to);
				if (from == to) {
					if ((to > _mLastEnd) && (from == fField[F_GROUP]->TextView()->TextLength())) {
						const char * ptr = fField[F_GROUP]->TextView()->Text();
						/* find start of current group -- this is UTF-8 OK */
						while (from >= 0) {
							if (ptr[from] == ',') {
								from++;
								break;
							}
							from--;
						}
						if (from < 0) from = 0;
						BMenuItem * item;
						/* find a matching group, if any, and do type-ahead */
						for (int ix=0; (item = fGroups->ItemAt(ix)) != NULL; ix++) {
							if (!strncmp(item->Label(), ptr+from, to-from)) {
								const char * x = item->Label()+to-from;
								fField[F_GROUP]->TextView()->Insert(x);
								fField[F_GROUP]->TextView()->Select(to, to+strlen(x));
								break;
							}
						}
					}
					_mLastEnd = to;
				}
			}
			} break;
	}
}

//--------------------------------------------------------------------

void TPeopleView::BuildGroupMenu(void)
{
	char			*offset;
	char			str[256];
	char			*text;
	char			*text1;
	int32			count = 0;
	int32			index;
	BEntry			entry;
	BFile			file;
	BMessage		*msg;
	BMenuItem		*item;
	BQuery			query;
	BVolume			vol;
	BVolumeRoster	volume;
	attr_info		info;

	while (item = fGroups->ItemAt(0)) {
		fGroups->RemoveItem(item);
		delete item;
	}


	// Find People on each mounted Volume
	while (volume.GetNextVolume(&vol) == B_OK)
	{
		if (vol.KnowsAttr() && vol.KnowsQuery())
		{
			query.SetVolume(&vol);
			sprintf(str, "%s=*", P_GROUP);
			query.SetPredicate(str);
			query.Fetch();
		
			while (query.GetNextEntry(&entry) == B_NO_ERROR) {
				file.SetTo(&entry, O_RDONLY);
				if ((file.InitCheck() == B_NO_ERROR) &&
					(file.GetAttrInfo(P_GROUP, &info) == B_NO_ERROR) &&
					(info.size > 1)) {
					text = (char *)malloc(info.size);
					text1 = text;
					file.ReadAttr(P_GROUP, B_STRING_TYPE, 0, text, info.size);
					while (1) {
						if (offset = strstr(text, ","))
							*offset = 0;
						if (!fGroups->FindItem(text)) {
							index = 0;
							while (item = fGroups->ItemAt(index)) {
								if (strcmp(text, item->Label()) < 0)
									break;
								index++;
							}
							msg = new BMessage(M_GROUP_MENU);
							msg->AddString("group", text);
							fGroups->AddItem(new BMenuItem(text, msg), index);
							count++;
						}
						if (offset) {
							text = offset + 1;
							while (*text == ' ')
								text++;
						}
						else
							break;
					}
					free(text1);
				}
			}
			
			// We must Clear() the BQuery before reusing it!
			query.Clear();
		}
	}
	

	if (!count) {
		fGroups->AddItem(item = new BMenuItem("none", new BMessage(M_GROUP_MENU)));
		item->SetEnabled(FALSE);
	}
}

//--------------------------------------------------------------------

bool TPeopleView::CheckSave(void)
{
	int32	loop;

	for (loop = 0; loop < F_END; loop++)
		if (fField[loop]->Changed())
			return TRUE;
	return FALSE;
}

//--------------------------------------------------------------------

const char* TPeopleView::GetField(int32 index)
{
	if (index < F_END)
		return fField[index]->Text();
	else
		return NULL;
}

//--------------------------------------------------------------------

void TPeopleView::NewFile(entry_ref *ref)
{
	if (fFile)
		delete fFile;
	fFile = new BFile(ref, O_RDWR);
	Save();
}

//--------------------------------------------------------------------

void TPeopleView::Save(void)
{
	const char	*text;
	int32		loop;

	text = fField[F_NAME]->Text();
	fFile->WriteAttr(P_NAME, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_NAME]->Update();

	text = fField[F_COMPANY]->Text();
	fFile->WriteAttr(P_COMPANY, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_COMPANY]->Update();

	text = fField[F_ADDRESS]->Text();
	fFile->WriteAttr(P_ADDRESS, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_ADDRESS]->Update();

	text = fField[F_CITY]->Text();
	fFile->WriteAttr(P_CITY, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_CITY]->Update();

	text = fField[F_STATE]->Text();
	fFile->WriteAttr(P_STATE, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_STATE]->Update();

	text = fField[F_ZIP]->Text();
	fFile->WriteAttr(P_ZIP, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_ZIP]->Update();

	text = fField[F_COUNTRY]->Text();
	fFile->WriteAttr(P_COUNTRY, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_COUNTRY]->Update();

	text = fField[F_HPHONE]->Text();
	fFile->WriteAttr(P_HPHONE, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_HPHONE]->Update();

	text = fField[F_WPHONE]->Text();
	fFile->WriteAttr(P_WPHONE, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_WPHONE]->Update();

	text = fField[F_FAX]->Text();
	fFile->WriteAttr(P_FAX, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_FAX]->Update();

	text = fField[F_EMAIL]->Text();
	fFile->WriteAttr(P_EMAIL, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_EMAIL]->Update();

	text = fField[F_URL]->Text();
	fFile->WriteAttr(P_URL, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_URL]->Update();

	text = fField[F_GROUP]->Text();
	fFile->WriteAttr(P_GROUP, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_GROUP]->Update();

	text = fField[F_NICKNAME]->Text();
	fFile->WriteAttr(P_NICKNAME, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_NICKNAME]->Update();
}

//--------------------------------------------------------------------

void TPeopleView::SetField(int32 index, const char *data, bool update)
{
	int32		end;
	int32		start;
	BTextView	*text;

	Window()->Lock();
	if (update) {
		fField[index]->SetText(data);
		fField[index]->Update();
	}
	else {
		text = (BTextView *)fField[index]->ChildAt(0);
		text->GetSelection(&start, &end);
		if (start != end) {
			text->Delete();
			text->Insert(data);
		}
		else if (end = text->TextLength()) {
			text->Select(end, end);
			text->Insert(",");
			text->Insert(data);
			text->Select(text->TextLength(), text->TextLength());
		}
		else
			fField[index]->SetText(data);
	}
	Window()->Unlock();
}

//--------------------------------------------------------------------

bool TPeopleView::TextSelected(void)
{
	int32		end;
	int32		loop;
	int32		start;
	BTextView	*text;

	for (loop = 0; loop < F_END; loop++) {
		text = (BTextView *)fField[loop]->ChildAt(0);
		text->GetSelection(&start, &end);
		if (start != end)
			return TRUE;
	}
	return FALSE;
}


//====================================================================

TTextControl::TTextControl(BRect r, char *label, int32 length,
						   char *text, int32 mod_msg, int32 msg, int32 click_msg)
			 :BTextControl(r, "", label, text, new BMessage(msg))
{
	SetModificationMessage(new BMessage(mod_msg));

	fLabel = (char *)malloc(strlen(label) + 1);
	strcpy(fLabel, label);
	fOriginal = (char *)malloc(strlen(text) + 1);
	strcpy(fOriginal, text);
	fLength = length;

	_mClickMessage = NULL;
	if (click_msg != 0) {
		SetClickMessage(new BMessage(click_msg));
	}
}

//--------------------------------------------------------------------

TTextControl::~TTextControl(void)
{
	free(fLabel);
	free(fOriginal);
	delete _mClickMessage;
}

//--------------------------------------------------------------------

void TTextControl::AttachedToWindow(void)
{
	BFont		font = *be_plain_font;
	BTextView	*text;

	BTextControl::AttachedToWindow();

	SetDivider(StringWidth(fLabel) + 7);
	text = (BTextView *)ChildAt(0);
	text->SetMaxBytes(fLength - 1);
}

//--------------------------------------------------------------------

bool TTextControl::Changed(void)
{
	return strcmp(fOriginal, Text());
}

//--------------------------------------------------------------------

void TTextControl::Revert(void)
{
	if (strcmp(fOriginal, Text()))
		SetText(fOriginal);
}

//--------------------------------------------------------------------

void TTextControl::Update(void)
{
	fOriginal = (char *)realloc(fOriginal, strlen(Text()) + 1);
	strcpy(fOriginal, Text());
}

//--------------------------------------------------------------------

void TTextControl::SetClickMessage(BMessage * message)
{
	if (_mClickMessage != message) {
		delete _mClickMessage;
		Invalidate();
	}
	_mClickMessage = message;
}

//--------------------------------------------------------------------

void TTextControl::Draw(BRect area)
{
	BTextControl::Draw(area);
	if ((_mClickMessage != 0) && Label()) {	/* draw an underline to show it's clickable */
		alignment label, text;
		GetAlignment(&label, &text);
		font_height fh;
		GetFontHeight(&fh);
		BRect bounds(Bounds());
		bounds.right = bounds.left+Divider();
		float fudge = StringWidth(Label());
		switch (label) {
		case B_ALIGN_LEFT:
			StrokeLine(BPoint(3,bounds.bottom-ceil(fh.descent)), 
				BPoint(fudge+3,bounds.bottom-ceil(fh.descent)));
			break;
		case B_ALIGN_RIGHT:
			StrokeLine(BPoint(bounds.right-3-fudge,bounds.bottom-ceil(fh.descent)), 
				BPoint(bounds.right-3,bounds.bottom-ceil(fh.descent)));
			break;
		case B_ALIGN_CENTER:
			fudge = floor((bounds.Width()-fudge)/2);
			StrokeLine(BPoint(bounds.left+fudge,bounds.bottom-ceil(fh.descent)), 
				BPoint(bounds.right-fudge,bounds.bottom-ceil(fh.descent)));
			break;
		}
	}
}

//--------------------------------------------------------------------

void TTextControl::MouseDown(BPoint where)
{
	BRect bounds(Bounds());
	if ((_mClickMessage != 0) && Label()) {	/* draw an underline to show it's clickable */
		alignment label, text;
		GetAlignment(&label, &text);
		font_height fh;
		GetFontHeight(&fh);
		BRect bounds(Bounds());
		bounds.right = bounds.left+Divider();
		float fudge = StringWidth(Label());
		switch (label) {
		case B_ALIGN_LEFT:
			bounds.left += 2;
			bounds.right = bounds.left+fudge+2;
			break;
		case B_ALIGN_RIGHT:
			bounds.right -= 2;
			bounds.left = bounds.right-fudge-2;
			break;
		case B_ALIGN_CENTER:
			fudge = floor((bounds.Width()-fudge)/2);
			bounds.left+= (fudge-1);
			bounds.right -= (fudge-1);
			break;
		}
		SetDrawingMode(B_OP_INVERT);
		bool in = false;
		bool newIn = bounds.Contains(where);
		while (true) {
			if (newIn != in) {
				FillRect(bounds, B_SOLID_HIGH);
				in = newIn;
			}
			BPoint newWhere;
			uint32 flags;
			GetMouse(&newWhere, &flags);
			if (!(flags & B_PRIMARY_MOUSE_BUTTON)) {
				break;
			}
			if (newWhere == where) {
				snooze(20000);
			}
			else {
				newIn = bounds.Contains(newWhere);
			}
		}
		if (in) {
			FillRect(bounds, B_SOLID_HIGH);
			Invoke(_mClickMessage);
		}
		SetDrawingMode(B_OP_COPY);
	}
}

