//--------------------------------------------------------------------
//	
//	Mail.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <Clipboard.h>
#include <E-mail.h>
#include <InterfaceKit.h>
#include <Roster.h>
#include <Screen.h>
#include <StorageKit.h>
#include <String.h>
#include <UTF8.h>

#include <fs_index.h>
#include <fs_info.h>

#include "Mail.h"
#include "Header.h"
#include "Content.h"
#include "Enclosures.h"
#include "Prefs.h"
#include "Signature.h"
#include "Status.h"
#include "String.h"
#include "FindWindow.h"
#include "Utilities.h"
#include "ButtonBar.h"
#include "QueryMenu.h"
#include "FieldMsg.h"
#include "Words.h"

const char *kUndoStrings[] = {
	"Undo",
	"Undo Typing",
	"Undo Cut",
	"Undo Paste",
	"Undo Clear",
	"Undo Drop"
};
const char *kRedoStrings[] = {
	"Redo",
	"Redo Typing",
	"Redo Cut",
	"Redo Paste",
	"Redo Clear",
	"Redo Drop"
};

bool		header_flag = false;
bool		wrap_mode = true;
bool		show_buttonbar = true;
char		*signature;
int32		level = L_BEGINNER;
entry_ref	open_dir;
BMessage	*print_settings = NULL;
BPoint		prefs_window;
BRect		signature_window;
BRect		mail_window;
BRect		last_window;
uint32		mail_encoding = B_MS_WINDOWS_CONVERSION;
Words 		*gWords[MAX_DICTIONARIES], *gExactWords[MAX_DICTIONARIES];
int32 		gUserDict;
BFile 		*gUserDictFile;
int32 		gDictCount = 0;

static const char *kDraftPath = "mail/draft";
static const char *kDraftType = "text/plain";
static const char *kMailFolder = "mail";
static const char *kMailboxFolder = "mail/mailbox";

static const char *kDictDirectory = "word_dictionary";
static const char *kIndexDirectory = "word_index";
static const char *kWordsPath = "/boot/optional/goodies/words";
static const char *kExact = ".exact";
static const char *kMetaphone = ".metaphone";

//====================================================================

int main()
{
	TMailApp().Run();	
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TMailApp::TMailApp()
	:	BApplication("application/x-vnd.Be-MAIL"),
		trackerMessenger(NULL),
		fWindowCount(0),
		fPrefsWindow(NULL),	
		fSigWindow(NULL),
		fFont(*be_plain_font)
{
	fFont.SetSize(FONT_SIZE);
	signature = (char*) malloc(strlen(SIG_NONE) + 1);
	strcpy(signature, SIG_NONE);
	mail_window.Set(0, 0, 0, 0);
	signature_window.Set(6, TITLE_BAR_HEIGHT, 6 + kSigWidth, TITLE_BAR_HEIGHT + kSigHeight);
	prefs_window.Set(6, TITLE_BAR_HEIGHT);

	//
	//	Find and read preferences
	//
	BDirectory dir;
	BEntry entry;
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	dir.SetTo(path.Path());
	if (dir.FindEntry("Mail_data", &entry) == B_NO_ERROR) {
		fPrefs = new BFile(&entry, O_RDWR);
		if (fPrefs->InitCheck() == B_NO_ERROR) {
			fPrefs->Read(&mail_window, sizeof(BRect));
			fPrefs->Read(&level, sizeof(level));

			font_family	f_family;
			font_style	f_style;
			float size;
			fPrefs->Read(&f_family, sizeof(font_family));
			fPrefs->Read(&f_style, sizeof(font_style));
			fPrefs->Read(&size, sizeof(float));
			if (size >= 9)
				fFont.SetSize(size);

			if ((strlen(f_family)) && (strlen(f_style)))
				fFont.SetFamilyAndStyle(f_family, f_style);

			fPrefs->Read(&signature_window, sizeof(BRect));
			fPrefs->Read(&header_flag, sizeof(bool));
			fPrefs->Read(&wrap_mode, sizeof(bool));
			fPrefs->Read(&prefs_window, sizeof(BPoint));
			int32 len;
			if (fPrefs->Read(&len, sizeof(int32)) > 0) {
				free(signature);
				signature = (char *)malloc(len);
				fPrefs->Read(signature, len);
			}

			fPrefs->Read(&mail_encoding, sizeof(int32));

			if (fPrefs->Read(&len, sizeof(int32)) > 0) {
				char *findString = (char *)malloc(len+1);
				fPrefs->Read(findString, len);
				findString[len] = 0;
				FindWindow::SetFindString(findString);
				free(findString);
			}
			if( fPrefs->Read(&show_buttonbar, sizeof(bool)) <= 0 )
				show_buttonbar = true;
		}
		else {
			delete fPrefs;
			fPrefs = NULL;
		}
	}
	else {
		fPrefs = new BFile();
		if (dir.CreateFile("Mail_data", fPrefs) != B_NO_ERROR) {
			delete fPrefs;
			fPrefs = NULL;
		}
	}

	fFont.SetSpacing(B_BITMAP_SPACING);
	last_window = mail_window;
}

//--------------------------------------------------------------------

TMailApp::~TMailApp()
{
	delete fPrefs;
	delete trackerMessenger;
}

//--------------------------------------------------------------------

void TMailApp::AboutRequested()
{
	(new BAlert("", "BeMail\nBy Robert Polic", "Close"))->Go();
}

//--------------------------------------------------------------------

bool helpOnly = false;

void TMailApp::ArgvReceived(int32 argc, char **argv)
{
	BEntry entry;
	BString names;
	BString ccNames;
	BString bccNames;
	BString subject;
	BString body;
	BMessage enclosure(B_REFS_RECEIVED);
	// a "mailto:" with no name should open an empty window
	// so remember if we got a "mailto:" even if there isn't a name
	// that goes along with it (this allows deskbar replicant to open
	// an empty message even when BeMail is already running)
	bool gotmailto = false;
	
	for (int32 loop = 1; loop < argc; loop++) {
		if (strcmp(argv[loop], "-h") == 0
			|| strcmp(argv[loop], "--help") == 0) {
			printf(" usage: %s [ mailto:<address> ] [ -subject \"<text>\" ] [ ccto:<address> ] [ bccto:<address> ] "
				"[ -body \"<body text\" ] [ enclosure:<path> ] [ <message to read> ...] \n",
				argv[0]);
			helpOnly = true;
			be_app->PostMessage(B_QUIT_REQUESTED);
			return;
		} else if (strncmp(argv[loop], "mailto:", 7) == 0) {
			if (names.Length())
				names += ", ";
			names += argv[loop] + 7;
			gotmailto = true;
		} else if (strncmp(argv[loop], "ccto:", 5) == 0) {
			if (ccNames.Length())
				ccNames += ", ";
			ccNames += argv[loop] + 5;
		} else if (strncmp(argv[loop], "bccto:", 6) == 0) {
			if (bccNames.Length())
				bccNames += ", ";
			bccNames += argv[loop] + 6;
		} else if (strcmp(argv[loop], "-subject") == 0) 
			subject = argv[++loop];
		else if (strcmp(argv[loop], "-body") == 0 && argv[loop + 1])
			body = argv[++loop];
		else if (strncmp(argv[loop], "enclosure:", 10) == 0) {
			BEntry tmp(argv[loop] + 10, true);
			if (tmp.InitCheck() == B_OK && tmp.Exists()) {
				entry_ref ref;
				tmp.GetRef(&ref);
				enclosure.AddRef("refs", &ref);
			}
		} else if (entry.SetTo(argv[loop]) == B_NO_ERROR) {
			BMessage msg(B_REFS_RECEIVED);
			entry_ref ref;
			entry.GetRef(&ref);
			msg.AddRef("refs", &ref);
			RefsReceived(&msg);
		}
	}

	if (gotmailto || names.Length() || ccNames.Length() || bccNames.Length() || subject.Length()
		|| body.Length() || enclosure.HasRef("refs")) {
		TMailWindow	*window = NewWindow(NULL, names.String());
		window->SetTo(names.String(), subject.String(), ccNames.String(), bccNames.String(),
			&body, &enclosure);
		window->Show();
	}
}

//--------------------------------------------------------------------

void TMailApp::MessageReceived(BMessage* msg)
{
	bool			all = false;
	int32			size;
	int32			type;
	BFont			*font;
	BMenuItem		*item;
	BMessage		*message;
	entry_ref		ref;
	TMailWindow		*window = NULL;
	TMailWindow		*src_window;
	TPrefsWindow	*prefs;

	switch (msg->what) {
		case M_NEW:
			msg->FindInt32("type", &type);
			switch (type) {
				case M_NEW:
					window = NewWindow();
					break;

				case M_RESEND:
				{
					msg->FindRef("ref", &ref);
					status_t status;
					attr_info info;
					BNode file( &ref );
					BString string;
					
					if( ((status=file.InitCheck()) == B_OK)&&((status=file.GetAttrInfo( B_MAIL_ATTR_TO, &info )) == B_NO_ERROR) )
					{
						char *str;
						
						str = string.LockBuffer(info.size+1);
						str[info.size] = 0;
						file.ReadAttr( B_MAIL_ATTR_TO, B_STRING_TYPE, 0, str, info.size );
						string.UnlockBuffer( info.size+1 );
					}
					
					window = NewWindow(&ref, string.String(), true);
					break;
				}
				case M_FORWARD:
					msg->FindRef("ref", &ref);
					window = NewWindow();
					window->Lock();
					window->Forward(&ref);
					window->Unlock();
					break;

				case M_REPLY:
				case M_REPLY_ALL:
				case M_COPY_TO_NEW:
					msg->FindPointer("window", (void **)&src_window);
					if (!src_window->Lock())
						break;
					msg->FindRef("ref", &ref);
					window = NewWindow();
					window->Lock();
					if( type == M_COPY_TO_NEW )
						window->CopyMessage(&ref, src_window);
					else
						window->Reply(&ref, src_window, type == M_REPLY_ALL);
					window->Unlock();
					src_window->Unlock();
					break;
			}
			if (window)
				window->Show();
			break;

		case M_WRAP_TEXT:
		{
			BMenuItem *item = NULL;
			if (msg->FindPointer("source", (void **)&item) == B_NO_ERROR) {
				wrap_mode = !wrap_mode;
				item->SetMarked(wrap_mode);
			}
			break;
		}

		case M_PREFS:
			if (fPrefsWindow) {
				fPrefsWindow->Activate(true);
			} else {
				fPrefsWindow = new TPrefsWindow(BRect(prefs_window.x, 
				  prefs_window.y, prefs_window.x + PREF_WIDTH, 
				  prefs_window.y + PREF_HEIGHT), &fFont, &level, &wrap_mode, 
				  &signature, &mail_encoding, &show_buttonbar);
				fPrefsWindow->Show();
				fPrevBBPref = show_buttonbar;
			}
			break;
		
		case PREFS_CHANGED:
		{
			// Do we need to update the state of the button bars?
			if( fPrevBBPref != show_buttonbar )
			{
				// Notify all BeMail windows
				TMailWindow	*window;
				for( int32 i=0; (window=(TMailWindow *)fWindowList.ItemAt(i)); i++ )
				{
					window->Lock();
					window->UpdateViews();
					window->Unlock();
				}
				fPrevBBPref = show_buttonbar;
			}
			break;
		}
		
		case M_EDIT_SIGNATURE:
			if (fSigWindow)
				fSigWindow->Activate(true);
			else {
				fSigWindow = new TSignatureWindow(signature_window);
				fSigWindow->Show();
			}
			break;

		case M_FONT:
			FontChange();
			break;

		case M_BEGINNER:
		case M_EXPERT:
			level = msg->what - M_BEGINNER;
			break;

		case REFS_RECEIVED:
			if (msg->HasPointer("window")) {
				msg->FindPointer("window", (void **)&window);
				message = new BMessage(*msg);
				window->PostMessage(message, window);
				delete message;
			}
			break;

		case WINDOW_CLOSED:
			switch (msg->FindInt32("kind")) {
				case MAIL_WINDOW:
					{
						TMailWindow	*window;
						if( msg->FindPointer( "window", (void **)&window ) == B_OK )
							fWindowList.RemoveItem( window );
						fWindowCount--;
						break;
					}
				case PREFS_WINDOW:
					fPrefsWindow = NULL;
					break;

				case SIG_WINDOW:
					fSigWindow = NULL;
					break;
			}

			if ((!fWindowCount) && (!fSigWindow) && (!fPrefsWindow))
				be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case B_REFS_RECEIVED:
			RefsReceived(msg);
			break;
			
		case B_PRINTER_CHANGED:
			ClearPrintSettings();
			break;

		default:
			BApplication::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

bool TMailApp::QuitRequested()
{
	int32		len;
	float		size;
	font_family	f_family;
	font_style	f_style;

	if (BApplication::QuitRequested()) {
		if (fPrefs) {
			fFont.GetFamilyAndStyle(&f_family, &f_style);
			size = fFont.Size();

			fPrefs->Seek(0, 0);
			fPrefs->Write(&last_window, sizeof(BRect));
			fPrefs->Write(&level, sizeof(level));
			fPrefs->Write(&f_family, sizeof(font_family));
			fPrefs->Write(&f_style, sizeof(font_style));
			fPrefs->Write(&size, sizeof(float));
			fPrefs->Write(&signature_window, sizeof(BRect));
			fPrefs->Write(&header_flag, sizeof(bool));
			fPrefs->Write(&wrap_mode, sizeof(bool));
			fPrefs->Write(&prefs_window, sizeof(BPoint));
			len = strlen(signature) + 1;
			fPrefs->Write(&len, sizeof(int32));
			fPrefs->Write(signature, len);
			fPrefs->Write(&mail_encoding, sizeof(int32));
			const char *findString = FindWindow::GetFindString();
			len = strlen(findString);
			fPrefs->Write(&len, sizeof(int32));
			fPrefs->Write(findString, len);
			fPrefs->Write(&show_buttonbar, sizeof(bool));
		}
		return true;
	}
	else
		return false;
}

//--------------------------------------------------------------------

void TMailApp::ReadyToRun()
{
	TMailWindow	*window;

	if (!helpOnly && !fWindowCount) {
		window = NewWindow();
		window->Show();
	}
	
	
	// Load dictionaries
	BPath indexDir;
	BPath dictionaryDir;
	BPath dataPath;
	BPath indexPath;
	BDirectory directory;
	BEntry entry;
	
	// Locate user settings directory
	find_directory( B_BEOS_ETC_DIRECTORY, &indexDir, true );
	dictionaryDir = indexDir;
	
	// Setup directory paths
	indexDir.Append( kIndexDirectory );
	dictionaryDir.Append( kDictDirectory );
	
	// Create directories if needed
	directory.CreateDirectory( indexDir.Path(), NULL );
	directory.CreateDirectory( dictionaryDir.Path(), NULL );
	
	dataPath = dictionaryDir;
	dataPath.Append( "words" );
	
	// Only Load if Words Dictionary
	if( BEntry( kWordsPath ).Exists() || BEntry( dataPath.Path() ).Exists() )
	{
		// If "/boot/optional/goodies/words" exists but there is no system dictionary, copy words
		if( !BEntry( dataPath.Path() ).Exists() && BEntry( kWordsPath ).Exists() )
		{
			BFile words( kWordsPath, B_READ_ONLY );
			BFile copy( dataPath.Path(), B_WRITE_ONLY | B_CREATE_FILE );
			char buffer[4096];
			ssize_t size;
			
			while( (size=words.Read( buffer, 4096 )) > 0 )
				copy.Write( buffer, size );
			BNodeInfo( &copy ).SetType( "text/plain" );
		}
		
		// Create user dictionary if it does not exist
		dataPath = dictionaryDir;
		dataPath.Append( "user" );
		if( !BEntry( dataPath.Path() ).Exists() )
		{
			BFile user( dataPath.Path(), B_WRITE_ONLY | B_CREATE_FILE );
			BNodeInfo( &user ).SetType( "text/plain" );
		}
		
		// Load dictionaries
		directory.SetTo( dictionaryDir.Path() );
		
		BString leafName;
		gUserDict = -1;
		
		while( (gDictCount<MAX_DICTIONARIES)&&(directory.GetNextEntry( &entry ) != B_ENTRY_NOT_FOUND) )
		{
			dataPath.SetTo( &entry );
			
			// Identify the user dictionary
			if( strcmp( "user", dataPath.Leaf() ) == 0 )
			{
				gUserDictFile = new BFile( dataPath.Path(), B_WRITE_ONLY | B_OPEN_AT_END );
				gUserDict = gDictCount;
			}
			
			indexPath = indexDir;
			leafName.SetTo( dataPath.Leaf() );
			leafName.Append( kMetaphone );
			indexPath.Append( leafName.String() );
			gWords[gDictCount] = new Words( dataPath.Path(), indexPath.Path(), true );
			
			indexPath = indexDir;
			leafName.SetTo( dataPath.Leaf() );
			leafName.Append( kExact );
			indexPath.Append( leafName.String() );
			gExactWords[gDictCount] = new Words( dataPath.Path(), indexPath.Path(), false );
			gDictCount++;
		}
	}
}

//--------------------------------------------------------------------

void TMailApp::RefsReceived(BMessage *msg)
{
	bool		have_names = false;
	BString		names;
	char		type[B_FILE_NAME_LENGTH];
	int32		item = 0;
	BFile		file;
	TMailWindow	*window;
	entry_ref	ref;

	//
	// If a tracker window opened me, get a messenger from it.
	//
	if (msg->HasMessenger("TrackerViewToken")) {
		trackerMessenger = new BMessenger;
		msg->FindMessenger("TrackerViewToken", trackerMessenger);
	}
	
	while (msg->HasRef("refs", item)) {
		msg->FindRef("refs", item++, &ref);
		if (window = FindWindow(ref))
			window->Activate(true);
		else {
			file.SetTo(&ref, O_RDONLY);
			if (file.InitCheck() == B_NO_ERROR) {
				BNodeInfo	node(&file);
				node.GetType(type);
				if (!strcmp(type, B_MAIL_TYPE)) {
					window = NewWindow(&ref, NULL, false, trackerMessenger);
					window->Show();
				} else if(!strcmp(type, "application/x-person")) {
					
					/* see if it has an Email address */
					BString name;
					BString email;
					attr_info	info;
					char *attrib;

					if (file.GetAttrInfo("META:email", &info) == B_NO_ERROR) {
						attrib = (char *) malloc(sizeof(info.size));
						file.ReadAttr("META:email", B_STRING_TYPE, 0, attrib, info.size);
						email << attrib;
						free(attrib);
					
					/* we got something... */	
					if (email.Length() > 0) {
							/* see if we can get a username as well */
							if(file.GetAttrInfo("META:name", &info) == B_NO_ERROR) {
								attrib = (char *) malloc(sizeof(info.size));
								file.ReadAttr("META:name", B_STRING_TYPE, 0, attrib, info.size);
								name << "\"" << attrib << "\" ";
								email.Prepend("<");
								email.Append(">");
								free(attrib);
							}
							
							if (names.Length() == 0) {
								names << name << email;
							} else {
								names << ", " << name << email;
							}
							have_names = true;
							email.SetTo("");
							name.SetTo("");
						}

					}
				}
				else if( strcmp(type, kDraftType) == 0 )
				{
					window = NewWindow();
					
					// If it's a draft message, open it
					window->OpenMessage(&ref);
					window->Show();
				}
			} /* end of else(file.InitCheck() == B_NO_ERROR */
		}
	}

	if (have_names) {
		window = NewWindow(NULL, names.String());
		window->Show();
	}
}

//--------------------------------------------------------------------

TMailWindow* TMailApp::FindWindow(const entry_ref &ref)
{
	int32		index = 0;
	TMailWindow	*window;

	while (window = (TMailWindow*) WindowAt(index++)) {
		if (window->FindView("m_header") && 
		  window->GetMailFile() != NULL && 
		  *window->GetMailFile() == ref)
		{
			return window;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------

void TMailApp::ClearPrintSettings()
{
	delete print_settings;
	print_settings = NULL;
}

//--------------------------------------------------------------------

void TMailApp::FontChange()
{
	int32		index = 0;
	BMessage	msg;
	BWindow		*window;
	
	msg.what = CHANGE_FONT;
	msg.AddPointer("font", &fFont);

	for (;;) {
		window = WindowAt(index++);
		if (!window)
			break;

		window->PostMessage(&msg);
	}
}

//--------------------------------------------------------------------

TMailWindow* TMailApp::NewWindow(const entry_ref *ref, const char *to, bool resend,	BMessenger* msng)
{
	char			*str1;
	char			*str2;
	char			*title = NULL;
	BFile			file;
	BPoint			win_pos;
	BRect			r;
	TMailWindow		*window;
	attr_info		info;
	
	BScreen screen(B_MAIN_SCREEN_ID);
	BRect screen_frame = screen.Frame();
	
	if ((mail_window.Width()) && (mail_window.Height()))
		r = mail_window;
	else
		r.Set(6, TITLE_BAR_HEIGHT, 6 + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT);
	
	r.OffsetBy(fWindowCount * 20, fWindowCount * 20);
	
	if ((r.left - 6) < screen_frame.left)
		r.OffsetTo(screen_frame.left + 8, r.top);
		
	if ((r.left + 20) > screen_frame.right)
		r.OffsetTo(6, r.top);
		
	if ((r.top - 26) < screen_frame.top)
		r.OffsetTo(r.left, screen_frame.top + 26);
		
	if ((r.top + 20) > screen_frame.bottom)
		r.OffsetTo(r.left, TITLE_BAR_HEIGHT);
		
	if (r.Width() < WIND_WIDTH)
		r.right = r.left + WIND_WIDTH;
		
	fWindowCount++;

	if (!resend && ref) {
		file.SetTo(ref, O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			if (file.GetAttrInfo(B_MAIL_ATTR_NAME, &info) == B_NO_ERROR) {
				str1 = (char *)malloc(info.size);
				file.ReadAttr(B_MAIL_ATTR_NAME, B_STRING_TYPE, 0, str1, 
			 	  info.size);
				if (file.GetAttrInfo(B_MAIL_ATTR_SUBJECT, &info) == B_NO_ERROR){
					str2 = (char *)malloc(info.size);
					file.ReadAttr(B_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, str2, 
					  info.size);
					title = (char *)malloc(strlen(str1) + strlen(str2) + 3);
					sprintf(title, "%s->%s", str1, str2);
					free(str1);
					free(str2);
				}
				else
					title = str1;
			}
		}
	}
	if (!title) {
		title = (char *)malloc(strlen("BeMail") + 1);
		sprintf(title, "BeMail");
	}
	
	window = new TMailWindow(r, title, ref, to, &fFont, resend, msng);
	fWindowList.AddItem( window );
	free(title);
	return window;
}


//====================================================================

// static list for tracking of Windows
BList	TMailWindow::sWindowList;

TMailWindow::TMailWindow(BRect rect, const char *title, const entry_ref *ref, const char *to,
						 const BFont *font, bool resending, BMessenger *msng)
			:BWindow(rect, title, B_DOCUMENT_WINDOW, 0),
			trackerMessenger(msng),
			fEnclosuresView(NULL),
			fSigAdded(false),
			fReplying(false),
			fResending(resending),
			fSent(false),
			fDraft(false),
			fPanel(NULL),
			fZoom(rect),
			fFieldState(0),
			fSendButton(NULL),
			fSaveButton(NULL),
			fPrintButton(NULL),
			fSigButton(NULL),
			fChanged(false)
{
	bool		done = false;
	bool		canResend = true;
	char		str[256];
	char		status[272];
	char		*header;
	char		*list;
	char		*recipients;
	int32		index = 0;
	int32		index1;
	int32		len;
	uint32		message;
	float		height;
	BMenu		*menu;
	BMenu		*sub_menu;
	BMenuBar	*menu_bar;
	BMenuItem	*item;
	BMessage	*msg;
	BRect		r;
	attr_info	info;

	if (ref) {
		fRef = new entry_ref(*ref);
		fFile = new BFile(fRef, O_RDONLY);
		fIncoming = true;
	}
	else {
		fRef = NULL;
		fFile = NULL;
		fIncoming = false;
	}

	r.Set(0, 0, RIGHT_BOUNDARY, 15);
	
	// Create real menu bar
	fMenuBar = menu_bar = new BMenuBar(r, "");

	//
	//	File Menu
	//
	menu = new BMenu("File");

	msg = new BMessage(M_NEW);
	msg->AddInt32("type", M_NEW);
	menu->AddItem(item = new BMenuItem("New Mail Message", msg, 'N'));
	item->SetTarget(be_app);
	
	QueryMenu *qmenu;
	qmenu = new QueryMenu( "Open Draft", false );
	qmenu->SetTargetForItems( be_app );
	
	// Make sure file type is indexed
	fs_create_index( dev_for_path( "/boot" ), "MAIL:draft", B_INT32_TYPE, 0 );
	qmenu->SetPredicate( "MAIL:draft==1" );
	menu->AddItem( qmenu );
	
	menu->AddSeparatorItem();

	if ((!resending) && (fIncoming)) {
		sub_menu = new BMenu("Close");
		if (fFile->GetAttrInfo(B_MAIL_ATTR_STATUS, &info) == B_NO_ERROR)
			fFile->ReadAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, str, info.size);
		else
			str[0] = 0;
		
		//if( (strcmp(str, "Pending")==0)||(strcmp(str, "Sent")==0) )
		//	canResend = true;
		if (!strcmp(str, "New")) {
			sub_menu->AddItem(item = new BMenuItem("Leave as 'New'",
							new BMessage(M_CLOSE_SAME), 'W', B_SHIFT_KEY));
			sub_menu->AddItem(item = new BMenuItem("Set to 'Read'",
							new BMessage(M_CLOSE_READ), 'W'));
			message = M_CLOSE_READ;
		}
		else {
			if (strlen(str))
				sprintf(status, "Leave as '%s'", str);
			else
				sprintf(status, "Leave same");
			sub_menu->AddItem(item = new BMenuItem(status,
							new BMessage(M_CLOSE_SAME), 'W'));
			message = M_CLOSE_SAME;
			AddShortcut('W', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(
			  M_CLOSE_SAME));
		}
		sub_menu->AddItem(new BMenuItem("Set to 'Saved'",
							new BMessage(M_CLOSE_SAVED), 'W', B_CONTROL_KEY));
		sub_menu->AddItem(new BMenuItem(new TMenu("Set to"B_UTF8_ELLIPSIS, 
		  INDEX_STATUS, M_STATUS), new BMessage(M_CLOSE_CUSTOM)));
		menu->AddItem(sub_menu);

		sub_menu->AddItem(new BMenuItem("Move to Trash", new BMessage(
		  M_DELETE), 'T', B_CONTROL_KEY));
		AddShortcut('T', B_SHIFT_KEY | B_COMMAND_KEY, new BMessage(
		  M_DELETE_NEXT));	
	}
	else
	{
		menu->AddItem(fSendLater = new BMenuItem("Save as Draft",
			new BMessage(M_SAVE_AS_DRAFT), 'S'));
		menu->AddItem(new BMenuItem("Close",
			new BMessage(B_CLOSE_REQUESTED), 'W'));
	}

	menu->AddSeparatorItem();
	menu->AddItem(fPrint = new BMenuItem("Page Setup"B_UTF8_ELLIPSIS,
								new BMessage(M_PRINT_SETUP)));
	menu->AddItem(fPrint = new BMenuItem("Print"B_UTF8_ELLIPSIS,
								new BMessage(M_PRINT), 'P'));
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("About BeMail"B_UTF8_ELLIPSIS,
								new BMessage(B_ABOUT_REQUESTED)));
	item->SetTarget(be_app);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("Quit",
								new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);
	menu_bar->AddItem(menu);

	//
	//	Edit Menu
	//
	menu = new BMenu("Edit");
	menu->AddItem(fUndo = new BMenuItem("Undo", new BMessage(B_UNDO), 'Z'));
	fUndo->SetTarget(NULL, this);
	menu->AddSeparatorItem();
	menu->AddItem(fCut = new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
	fCut->SetTarget(NULL, this);
	menu->AddItem(fCopy = new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
	fCopy->SetTarget(NULL, this);
	menu->AddItem(fPaste = new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));
	fPaste->SetTarget(NULL, this);
	menu->AddItem(item = new BMenuItem("Select All",
								new BMessage(M_SELECT), 'A'));
	menu->AddSeparatorItem();
	item->SetTarget(NULL, this);
	menu->AddItem(new BMenuItem("Find"B_UTF8_ELLIPSIS, new BMessage(M_FIND), 'F'));
	menu->AddItem(new BMenuItem("Find Again", new BMessage(M_FIND_AGAIN), 'G'));
	if (!fIncoming) {
		menu->AddSeparatorItem();
		menu->AddItem(fQuote =new BMenuItem("Quote",new BMessage(M_QUOTE),
			B_RIGHT_ARROW));
		menu->AddItem(fRemoveQuote = new BMenuItem("Remove Quote",
			new BMessage(M_REMOVE_QUOTE), B_LEFT_ARROW));
		fSignature = new TMenu("Add Signature", INDEX_SIGNATURE, M_SIGNATURE);
		menu->AddItem(new BMenuItem(fSignature));
		fSpelling = new BMenuItem( "Check Spelling", new BMessage( M_CHECK_SPELLING ), ';' );
		menu->AddItem(fSpelling);
	}
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem("Preferences"B_UTF8_ELLIPSIS,
								new BMessage(M_PREFS)));
	item->SetTarget(be_app);
	menu->AddItem(item = new BMenuItem("Signatures"B_UTF8_ELLIPSIS,
								new BMessage(M_EDIT_SIGNATURE)));
	item->SetTarget(be_app);
	menu_bar->AddItem(menu);


	//
	//	Message Menu
	//
	menu = new BMenu("Message");
	
	if ((!resending) && (fIncoming)) {
		BMenuItem *menuItem;
		menu->AddItem(new BMenuItem("Reply to Sender",
						new BMessage(M_REPLY), 'R'));
		menu->AddItem(new BMenuItem("Reply to All",
						new BMessage(M_REPLY_ALL), 'R', B_SHIFT_KEY));
		menu->AddItem(new BMenuItem("Forward", new BMessage(M_FORWARD), 'J'));
		menu->AddItem(menuItem=new BMenuItem("Resend", new BMessage(M_RESEND)));
		menu->AddItem(menuItem=new BMenuItem("Copy to New", new BMessage(M_COPY_TO_NEW), 'D'));
		
		deleteNext = new BMenuItem("Move to Trash", new BMessage(M_DELETE_NEXT), 'T');
		menu->AddItem(deleteNext);
		menu->AddSeparatorItem();
		prevMsg = new BMenuItem("Previous Message", new BMessage(M_PREVMSG), 
		 B_UP_ARROW);
		menu->AddItem(prevMsg);
		nextMsg = new BMenuItem("Next Message", new BMessage(M_NEXTMSG), 
		  B_DOWN_ARROW);
		menu->AddItem(nextMsg);
		menu->AddSeparatorItem();
		menu->AddItem(fHeader = new BMenuItem("Show Header",
								new BMessage(M_HEADER), 'H'));
		if (header_flag)
			fHeader->SetMarked(true);
		menu->AddItem(fRaw = new BMenuItem("Show Raw Message",
								new BMessage(M_RAW)));

		saveAddrMenu = sub_menu = new BMenu("Save Address");
		recipients = (char *)malloc(1);
		recipients[0] = 0;
		len = header_len(fFile);
		header = (char *)malloc(len);
		fFile->Seek(0, 0);
		fFile->Read(header, len);
		get_recipients(&recipients, header, len, true);
		list = recipients;
		while (1) {
			if ((!list[index]) || (list[index] == ',')) {
				if (!list[index])
					done = true;
				else
					list[index] = 0;
				index1 = 0;
				while (item = sub_menu->ItemAt(index1)) {
					if (strcmp(list, item->Label()) == 0)
						goto skip;
					if (strcmp(list, item->Label()) < 0)
						break;
					index1++;
				}
				msg = new BMessage(M_SAVE);
				msg->AddString("address", list);
				sub_menu->AddItem(new BMenuItem(list, msg), index1);

skip:			if (!done) {
					list += index + 1;
					index = 0;
					while (*list) {
						if (*list != ' ')
							break;
						else
							list++;
					}
				}
				else
					break;
			}
			else
				index++;
		}
		free(header);
		free(recipients);
		menu->AddItem(sub_menu);
	}
	else {
		menu->AddItem(fSendNow = new BMenuItem("Send Message",
								new BMessage(M_SEND_NOW), 'M'));
		if( !fResending )
		{
			// We want to make alt-shift-M work to send mail as well as just alt-M
			// Gross hack follows... hey, don't look at me like that... it works.
			// Create a hidden menu bar with a single "Send Now" item linked to alt-shift-M
			BMenuBar *fudge_bar;
			fudge_bar = new BMenuBar(r, "Fudge Bar");
			fudge_bar->Hide();
			fudge_bar->AddItem(new BMenuItem("Fudge Send", new BMessage(M_SEND_NOW), 'M', B_SHIFT_KEY) );
			AddChild( fudge_bar );
		}
	}
	menu_bar->AddItem(menu);


	//
	//	Enclosures Menu
	//
	if (!fIncoming) {
		menu = new BMenu("Enclosures");
		menu->AddItem(fAdd = new BMenuItem("Add"B_UTF8_ELLIPSIS, new BMessage(M_ADD), 'E'));
		menu->AddItem(fRemove = new BMenuItem("Remove",
									new BMessage(M_REMOVE), 'T'));
		menu_bar->AddItem(menu);
	}

	Lock();
	AddChild(menu_bar);
	height = menu_bar->Bounds().bottom + 1;
	Unlock();
	
	//
	// Button Bar
	//
	float bbwidth = 0, bbheight = 0;
	
	if( show_buttonbar )
	{
		BuildButtonBar();
		fButtonBar->ShowLabels( show_buttonbar & 1 );
		fButtonBar->Arrange( true );
		
		fButtonBar->GetPreferredSize( &bbwidth, &bbheight );
		fButtonBar->ResizeTo( Bounds().right+3, bbheight+1 );
		fButtonBar->MoveTo( -1, height-1 );
		fButtonBar->Show();
	}
	else
		fButtonBar = NULL;
	
	r.top = height+bbheight;
	if (!fIncoming)
		r.bottom = height + bbheight + HEADER_HEIGHT + 1;
	else
		r.bottom = height + bbheight + MIN_HEADER_HEIGHT + 1;
	fHeaderView = new THeaderView(r, rect, fIncoming, fFile, resending);

	r = Frame();
	r.OffsetTo(0, 0);
	if (fIncoming)
		r.top = height + bbheight + MIN_HEADER_HEIGHT;
	else
		r.top = height + bbheight + HEADER_HEIGHT;
	fContentView = new TContentView(r, fIncoming, fFile, const_cast<BFont *>(font));
		// TContentView needs to be properly const, for now cast away constness

	Lock();
	AddChild(fHeaderView);
	if (fEnclosuresView)
		AddChild(fEnclosuresView);
	AddChild(fContentView);
	Unlock();

	if (to) {
		Lock();
		fHeaderView->fTo->SetText(to);
		Unlock();
	}

	SetSizeLimits(WIND_WIDTH, RIGHT_BOUNDARY,
				  HEADER_HEIGHT + ENCLOSURES_HEIGHT + height + 50, RIGHT_BOUNDARY);

	AddShortcut('n', B_COMMAND_KEY, new BMessage(M_NEW));


	// 
	// 	If auto-signature, add signature to the text here.
	//
	if (!fIncoming && strcmp(signature, SIG_NONE) != 0) {
		if( strcmp(signature, SIG_RANDOM) == 0 )
			PostMessage( M_RANDOM_SIG );
		else
		{
			//
			//	Create a query to find this signature
			//
			BVolume			vol;
			BVolumeRoster().GetBootVolume(&vol);
			BQuery query;
	
			query.SetVolume(&vol);
			query.PushAttr(INDEX_SIGNATURE);
			query.PushString(signature);
			query.PushOp(B_EQ);
			query.Fetch();
	
			//
			//	If we find the named query, add it to the text.
			//
			BEntry entry;
			if (query.GetNextEntry(&entry) == B_NO_ERROR) {
				off_t size;
				BFile file;
				file.SetTo(&entry, O_RDWR);
				if (file.InitCheck() == B_NO_ERROR) {
					file.GetSize(&size);
					char *str = (char *)malloc(size);
					size = file.Read(str, size);
					fContentView->fTextView->Insert(str, size);
					fContentView->fTextView->GoToLine(0);
					fContentView->fTextView->ScrollToSelection();
				}
			} else {
				printf("Query failed.\n");
			}
		}
	}
}

//--------------------------------------------------------------------

void TMailWindow::BuildButtonBar( void )
{
	ButtonBar *bbar;
	
	bbar = new ButtonBar( BRect( 0, 0, 100, 100 ), "ButtonBar", 2, 3, 0, 1, 10, 2 );
	bbar->AddButton( "New", 28, new BMessage( M_NEW ) );
	bbar->AddDivider( 5 );
	
	if( fResending )
	{
		fSendButton = bbar->AddButton( "Send", 8, new BMessage( M_SEND_NOW ) );
		bbar->AddDivider( 5 );
	}
	else if(!fIncoming )
	{
		fSendButton = bbar->AddButton( "Send", 8, new BMessage( M_SEND_NOW ) );
		fSendButton->SetEnabled( false );
		fSaveButton = bbar->AddButton( "Save", 44, new BMessage( M_SAVE_AS_DRAFT ) );
		fSaveButton->SetEnabled( false );
		(fSigButton = bbar->AddButton( "Signature", 4, new BMessage( M_SIG_MENU ) ))->InvokeOnButton( B_SECONDARY_MOUSE_BUTTON );
		bbar->AddDivider( 5 );
		fPrintButton = bbar->AddButton( "Print", 16, new BMessage( M_PRINT ) );
		fPrintButton->SetEnabled( false );
		bbar->AddButton( "Trash", 0, new BMessage( M_DELETE ) );
	}
	else
	{
		(bbar->AddButton( "Reply", 12, new BMessage( M_REPLY ) ))->InvokeOnButton( B_SECONDARY_MOUSE_BUTTON );
		bbar->AddButton( "Forward", 40, new BMessage( M_FORWARD ) );
		fPrintButton = bbar->AddButton( "Print", 16, new BMessage( M_PRINT ) );
		bbar->AddButton( "Trash", 0, new BMessage( M_DELETE_NEXT ) );
		bbar->AddDivider( 5 );
		bbar->AddButton( "Next", 24, new BMessage( M_NEXTMSG ) );
		bbar->AddButton( "Previous", 20, new BMessage( M_PREVMSG ) );
	}
	bbar->AddButton( "Inbox", 36, new BMessage( M_OPEN_MAIL_BOX ) );
	bbar->AddButton( "Mail", 32, new BMessage( M_OPEN_MAIL_FOLDER ) );
	bbar->AddDivider( 5 );
	
	bbar->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	bbar->Hide();
	AddChild( bbar );
	fButtonBar = bbar;
}

//--------------------------------------------------------------------

void TMailWindow::UpdateViews( void )
{
	float bbwidth = 0, bbheight = 0;
	float nextY = fMenuBar->Frame().bottom+1;
	
	// Show/Hide Button Bar
	if( show_buttonbar )
	{
		// Create the Button Bar if needed
		if( !fButtonBar )
			BuildButtonBar();
		fButtonBar->ShowLabels( show_buttonbar & 1 );
		fButtonBar->Arrange( true );
		fButtonBar->GetPreferredSize( &bbwidth, &bbheight );
		fButtonBar->ResizeTo( Bounds().right+3, bbheight+1 );
		fButtonBar->MoveTo( -1, nextY-1 );
		nextY += bbheight;
		if( fButtonBar->IsHidden() )
			fButtonBar->Show();
		else
			fButtonBar->Invalidate();
	}
	else if( fButtonBar )
		fButtonBar->Hide();
	
	// Arange other views to match
	fHeaderView->MoveTo( 0, nextY );
	nextY = fHeaderView->Frame().bottom;
	if( fEnclosuresView )
	{
		fEnclosuresView->MoveTo( 0, nextY );
		nextY = fEnclosuresView->Frame().bottom+1;
	}
	BRect bounds( Bounds() );
	fContentView->MoveTo( 0, nextY-1 );
	fContentView->ResizeTo( bounds.right-bounds.left, bounds.bottom-nextY+1 );
}

//--------------------------------------------------------------------

TMailWindow::~TMailWindow()
{	
	delete fFile;
	last_window = Frame();
	delete fPanel;

	sWindowList.RemoveItem(this);
}

entry_ref* TMailWindow::GetMailFile() const
{
	return fRef;
}

bool TMailWindow::GetTrackerWindowFile(entry_ref *ref, bool next) const
{
	if (trackerMessenger == NULL) 
		return false;
	
	//
	//	Ask the tracker what the next/prev file in the window is.
	//	Continue asking for the next reference until a valid 
	//	email file is found (ignoring other types).
	//
	entry_ref nextRef = *ref;
	bool foundRef = false;
	while (!foundRef) {
		BMessage request(B_GET_PROPERTY);
		BMessage spc;
		if (next)
			spc.what = 'snxt';
		else
			spc.what = 'sprv';

		spc.AddString("property", "Entry");
		spc.AddRef("data", &nextRef);

		request.AddSpecifier(&spc);
		BMessage reply;
		if (trackerMessenger->SendMessage(&request, &reply) != B_OK)
			return false;

		if (reply.FindRef("result", &nextRef) != B_OK)
			return false;

		char fileType[256];
		BNode node(&nextRef);
		if (node.InitCheck() != B_OK) 
			return false;

		if (BNodeInfo(&node).GetType(fileType) != B_OK)
			return false;

		if (strcasecmp(fileType,"text/x-email") == 0)
			foundRef = true;	
	}

	*ref = nextRef;
	return foundRef;
}


void TMailWindow::SetTrackerSelectionToCurrent()
{
	BMessage setsel(B_SET_PROPERTY);
	setsel.AddSpecifier("Selection");
	setsel.AddRef("data", fRef);
	trackerMessenger->SendMessage(&setsel);
}

void TMailWindow::SetCurrentMessageRead()
{
	char status[255];
	BFile file(fRef, O_RDWR);
	if (file.InitCheck() == B_NO_ERROR)
	{
		attr_info info;
		if (file.GetAttrInfo(B_MAIL_ATTR_STATUS, &info) 
		  == B_NO_ERROR)	
		{
			file.ReadAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, 
			  status, info.size);
			if (strcmp(status, "New") == 0) {
				file.RemoveAttr(B_MAIL_ATTR_STATUS);
				file.WriteAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 
				  0, "Read", 5);
			}
		}
	}
}

void TMailWindow::FrameResized(float width, float height)
{
	fContentView->FrameResized(width, height);
}

//--------------------------------------------------------------------

void TMailWindow::MenusBeginning()
{
	bool		enable;
	int32		finish = 0;
	int32		len = 0;
	int32		start = 0;
	BTextView	*text_view;

	if (!fIncoming) {
		enable = strlen(fHeaderView->fTo->Text()) ||
				 strlen(fHeaderView->fBcc->Text());
		fSendNow->SetEnabled(enable);
		fSendLater->SetEnabled(enable);

		be_clipboard->Lock();
		fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE) &&
						   ((fEnclosuresView == NULL) || !fEnclosuresView->fList->IsFocus()));
		be_clipboard->Unlock();

		fQuote->SetEnabled(false);
		fRemoveQuote->SetEnabled(false);

		fAdd->SetEnabled(true);
		fRemove->SetEnabled((fEnclosuresView != NULL) && 
							(fEnclosuresView->fList->CurrentSelection() >= 0));
	}
	else {
		if (fResending) {
			enable = strlen(fHeaderView->fTo->Text());
			fSendNow->SetEnabled(enable);
			// fSendLater->SetEnabled(enable);
			text_view = (BTextView *)fHeaderView->fTo->ChildAt(0);
			if (text_view->IsFocus()) {
				text_view->GetSelection(&start, &finish);
				fCut->SetEnabled(start != finish);
				be_clipboard->Lock();
				fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE));
				be_clipboard->Unlock();
			}
			else {
				fCut->SetEnabled(false);
				fPaste->SetEnabled(false);
			}
		}
		else {
			fCut->SetEnabled(false);
			fPaste->SetEnabled(false);

			if (trackerMessenger == NULL || !trackerMessenger->IsValid())
			{
				nextMsg->SetEnabled(false);
				prevMsg->SetEnabled(false);
			}
		}
	}

	if ((!fIncoming) || (fResending))
		fHeaderView->BuildMenus();

	fPrint->SetEnabled(fContentView->fTextView->TextLength());

	text_view = (BTextView *)fHeaderView->fTo->ChildAt(0);
	if (text_view->IsFocus())
		text_view->GetSelection(&start, &finish);
	else {
		text_view = (BTextView *)fHeaderView->fSubject->ChildAt(0);
		if (text_view->IsFocus())
			text_view->GetSelection(&start, &finish);
		else {
			if (fContentView->fTextView->IsFocus()) {
				fContentView->fTextView->GetSelection(&start, &finish);
				if (!fIncoming) {
					fQuote->SetEnabled(true);
					fRemoveQuote->SetEnabled(true);
				}
			}
			else if (!fIncoming) {
				text_view = (BTextView *)fHeaderView->fCc->ChildAt(0);
				if (text_view->IsFocus())
					text_view->GetSelection(&start, &finish);
				else {
					text_view = (BTextView *)fHeaderView->fBcc->ChildAt(0);
					if (text_view->IsFocus())
						text_view->GetSelection(&start, &finish);
				}
			}
		}
	}

	fCopy->SetEnabled(start != finish);
	if (!fIncoming)
		fCut->SetEnabled(start != finish);

	// Undo stuff	
	bool		isRedo = false;
	undo_state	undoState = B_UNDO_UNAVAILABLE;

	BTextView *focusTextView = dynamic_cast<BTextView *>(CurrentFocus());
	if (focusTextView != NULL)
		undoState = focusTextView->UndoState(&isRedo);

	fUndo->SetLabel((isRedo) ? kRedoStrings[undoState] : kUndoStrings[undoState]);
	fUndo->SetEnabled(undoState != B_UNDO_UNAVAILABLE);
}

//--------------------------------------------------------------------

void TMailWindow::MessageReceived(BMessage* msg)
{
	bool			now = false;
	bool			raw;
	char			*arg;
	char			*str;
	int32			item = 0;
	BEntry			entry;
	BMenuItem		*menu;
	BMessage		*message;
	BMessage		open(B_REFS_RECEIVED);
	BMessenger		*me;
	BMessenger		*tracker;
	BQuery			query;
	BRect			r;
	BVolume			vol;
	BVolumeRoster	volume;
	entry_ref		ref;
	attr_info		info;
	status_t		result;

	switch(msg->what) {
		case FIELD_CHANGED:
		{
			int32 prevState = fFieldState, fieldMask = msg->FindInt32( "bitmask" );
			void *source;
			
			if( msg->FindPointer( "source", &source ) == B_OK )
			{
				int32 length;
				
				if( fieldMask == FIELD_BODY )
					length = ((TTextView *)source)->TextLength();
				else
					length = ((BComboBox *)source)->TextView()->TextLength();
				
				if( length )
					fFieldState |= fieldMask;
				else
					fFieldState &= ~fieldMask;
			}
			
			// Has anything changed?
			if( (prevState != fFieldState)||(!fChanged) )
			{
				// Change Buttons to reflect this
				if( fSaveButton )
					fSaveButton->SetEnabled( fFieldState );
				if( fPrintButton )
					fPrintButton->SetEnabled( fFieldState );
				if( fSendButton )
					fSendButton->SetEnabled( (fFieldState & FIELD_TO)||(fFieldState & FIELD_BCC) );
			}
			fChanged = true;
			
			// Update title bar if "subject" has changed 
			if( fieldMask & FIELD_SUBJECT )
			{
				// If no subject, set to "BeMail"
				if( !fHeaderView->fSubject->TextView()->TextLength() )
					SetTitle( "BeMail" );
				else
					SetTitle( fHeaderView->fSubject->Text() );
			}
			break;
		}
		case LIST_INVOKED:
			PostMessage(msg, fEnclosuresView);
			break;

		case CHANGE_FONT:
			PostMessage(msg, fContentView);
			break;

		case M_NEW:
			message = new BMessage(M_NEW);
			message->AddInt32("type", msg->what);
			be_app->PostMessage(message);
			delete message;
			break;

		case M_REPLY:
			{
				uint32 buttons;
				if( (msg->FindInt32( "buttons", (int32 *)&buttons ) == B_OK) && 
					(buttons == B_SECONDARY_MOUSE_BUTTON) )
				{
					BPopUpMenu menu( "Reply To", false, false );
					BMenuItem *item;
					BMessage *message;
					
					
					message = new BMessage( *msg );
					message->RemoveName( "buttons" );
					message->RemoveName( "where" );
					
					menu.AddItem( new BMenuItem( "Reply to Sender", message ) );
					message = new BMessage( *message );
					message->what = M_REPLY_ALL;
					menu.AddItem( new BMenuItem( "Reply to All", message ) );
					
					BPoint	where;
					BRect	bounds;
					
					msg->FindPoint( "where", &where );
					if( (item = menu.Go( where, false, false )) )
					{
						item->SetTarget( this );
						PostMessage( item->Message() );
					}
					break;
				}
			}
		// Fall Through
		case M_REPLY_ALL:
		case M_FORWARD:
		case M_RESEND:
		case M_COPY_TO_NEW:
			message = new BMessage(M_NEW);
			message->AddRef("ref", fRef);
			message->AddPointer("window", this);
			message->AddInt32("type", msg->what);
			be_app->PostMessage(message);
			delete message;
			break;
			
		case M_DELETE:
		case M_DELETE_PREV:
		case M_DELETE_NEXT:
		{
			if (level == L_BEGINNER) {
				beep();
				if (!(new BAlert("",
					"Are you sure you want to move this message to the trash?",
					"Cancel", "Trash", NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
					B_WARNING_ALERT))->Go())
					break;
			}
			
			if( (msg->what == M_DELETE_NEXT)&&(modifiers() & B_SHIFT_KEY) )
				msg->what = M_DELETE_PREV;
			
			bool foundRef = false;
			entry_ref nextRef;
			if (msg->what == M_DELETE_PREV || msg->what == M_DELETE_NEXT) {
				//
				//	Find the next message that should be displayed
				//
				nextRef = *fRef;
				foundRef = GetTrackerWindowFile(&nextRef, msg->what == 
				  M_DELETE_NEXT);
			}
			if( fIncoming )
				SetCurrentMessageRead();

			if (trackerMessenger == NULL || !trackerMessenger->IsValid()||(!fIncoming)) {
			
				//
				//	Not associated with a tracker window.  Create a new
				//	messenger and ask the tracker to delete this entry
				//
				if( fDraft || fIncoming )
				{
					BMessenger *tracker= new BMessenger("application/x-vnd.Be-TRAK",
					  -1, NULL);
					if (tracker->IsValid()) {
						BMessage msg('Ttrs');
						msg.AddRef("refs", fRef);	
						tracker->SendMessage(&msg);
					} else {
						(new BAlert("", "Need tracker to move items to trash",
						  "sorry"))->Go();
					}
				}
			} else {

				//
				// This is associated with a tracker window.  Ask the 
				// window to delete this entry.  Do it this way if we
				// can instead of the above way because it doesn't reset
				// the selection (even though we set selection below, this
				// still causes problems).
				//
				BMessage delmsg(B_DELETE_PROPERTY);
				BMessage entryspec('sref');
				entryspec.AddRef("refs", fRef);
				entryspec.AddString("property", "Entry");
				delmsg.AddSpecifier(&entryspec);
				trackerMessenger->SendMessage(&delmsg);
			}


			//
			// 	If the next file was found, open it.  If it was not,
			//	we have no choice but to close this window.
			//
			if (foundRef) {
				OpenMessage(&nextRef);
				SetTrackerSelectionToCurrent();
			} else {
				fSent = true;
				BMessage msg(B_CLOSE_REQUESTED);
				PostMessage(&msg);
			}
				
			break;
		}
			
		case M_CLOSE_READ:
			message = new BMessage(B_CLOSE_REQUESTED);
			message->AddString("status", "Read");
			PostMessage(message);
			delete message;
			break;

		case M_CLOSE_SAVED:
			message = new BMessage(B_CLOSE_REQUESTED);
			message->AddString("status", "Saved");
			PostMessage(message);
			delete message;
			break;

		case M_CLOSE_SAME:
			message = new BMessage(B_CLOSE_REQUESTED);
			message->AddString("status", "");
			message->AddString("same", "");
			PostMessage(message);
			delete message;
			break;

		case M_CLOSE_CUSTOM:
			if (msg->HasString("status")) {
				msg->FindString("status", (const char**) &str);
				message = new BMessage(B_CLOSE_REQUESTED);
				message->AddString("status", str);
				PostMessage(message);
				delete message;
			}
			else {
				r = Frame();
				r.left += ((r.Width() - STATUS_WIDTH) / 2);
				r.right = r.left + STATUS_WIDTH;
				r.top += 40;
				r.bottom = r.top + STATUS_HEIGHT;
				if (fFile->GetAttrInfo(B_MAIL_ATTR_STATUS, &info) == B_NO_ERROR) {
					str = (char *)malloc(info.size);
					fFile->ReadAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, str, info.size);
				}
				else {
					str = (char *)malloc(1);
					str[0] = 0;
				}
				new TStatusWindow(r, this, str);
				free(str);
			}
			break;

		case M_STATUS:
			msg->FindPointer("source", (void **)&menu);
			message = new BMessage(B_CLOSE_REQUESTED);
			message->AddString("status", menu->Label());
			PostMessage(message);
			delete message;
			break;

		case M_HEADER:
			header_flag = !(fHeader->IsMarked());
			fHeader->SetMarked(header_flag);
			message = new BMessage(M_HEADER);
			message->AddBool("header", header_flag);
			PostMessage(message, fContentView->fTextView);
			delete message;
			break;

		case M_RAW:
			raw = !(fRaw->IsMarked());
			fRaw->SetMarked(raw);
			message = new BMessage(M_RAW);
			message->AddBool("raw", raw);
			PostMessage(message, fContentView->fTextView);
			delete message;
			break;

		case M_SEND_NOW:
			now = true;
			// yes, we are suppose to fall through
		case M_SAVE_AS_DRAFT:
			Send(now);
			break;

		case M_SAVE:
			if (msg->FindString("address", (const char**) &str) == B_NO_ERROR) {
				arg = (char *)malloc(strlen("META:email ") + strlen(str) + 1);
				volume.GetBootVolume(&vol);
				query.SetVolume(&vol);
				sprintf(arg, "META:email=%s", str);
				query.SetPredicate(arg);
				query.Fetch();
				if (query.GetNextEntry(&entry) == B_NO_ERROR) {
					tracker = new BMessenger("application/x-vnd.Be-TRAK", -1, NULL);
					if (tracker->IsValid()) {
						entry.GetRef(&ref);
						open.AddRef("refs", &ref);
						tracker->SendMessage(&open);
					}
					delete tracker;
				}
				else {
					sprintf(arg, "META:email %s", str);
					result = be_roster->Launch("application/x-person", 1, &arg);
					if (result != B_NO_ERROR)
						(new BAlert("", "Sorry, could not find an application that supports the "
							"'Person' data type.", "OK"))->Go();
				}
				free(arg);
			}
			break;

		case M_PRINT_SETUP:
			PrintSetup();
			break;

		case M_PRINT:
			Print();
			break;

		case M_SELECT:
			break;

		case M_FIND:
			FindWindow::Find(this);
			break;
			
		case M_FIND_AGAIN:
			FindWindow::FindAgain(this);
			break;
			
		case M_QUOTE:
		case M_REMOVE_QUOTE:
			PostMessage(msg->what, fContentView);
			break;
		
		case M_RANDOM_SIG:
			{
				BEntry			entry;
				BFile			file;
				BQuery			query;
				BVolume			vol;
				entry_ref		ref;
				BList			sigList;
				char			predicate[128];
				BMessage		*message;
				
				BVolumeRoster().GetBootVolume(&vol);
				query.SetVolume(&vol);
				
				sprintf( predicate, "%s = *", INDEX_SIGNATURE );
				query.SetPredicate( predicate );
				query.Fetch();
				
				while (query.GetNextEntry(&entry) == B_NO_ERROR) {
					file.SetTo(&entry, O_RDONLY);
					if (file.InitCheck() == B_NO_ERROR) {
						message = new BMessage(M_SIGNATURE);
						entry.GetRef(&ref);
						message->AddRef("ref", &ref);
						sigList.AddItem( message );
					}
				}
				srand(time (0));
				message = (BMessage *)sigList.ItemAt( rand() % sigList.CountItems() );
				PostMessage( message );
				for( int32 i=0; (message=(BMessage *)sigList.ItemAt(i)); i++ )
					delete message;
			}
			break;
		case M_SIGNATURE:
			message = new BMessage(*msg);
			PostMessage(message, fContentView);
			delete message;
			fSigAdded = true;
			break;
		
		case M_SIG_MENU:
		{
			TMenu *menu;
			BMenuItem *item;
			menu = new TMenu( "Add Signature", INDEX_SIGNATURE, M_SIGNATURE, true );
			
			BPoint	where;
			bool open_anyway = true;
			
			if( msg->FindPoint( "where", &where ) != B_OK )
			{
				BRect	bounds;
				bounds = fSigButton->Bounds();
				where = fSigButton->ConvertToScreen( BPoint( (bounds.right-bounds.left)/2, (bounds.bottom-bounds.top)/2 ) );
			}
			else if( msg->FindInt32( "buttons" ) == B_SECONDARY_MOUSE_BUTTON )
				open_anyway = false;	
			
			if( (item = menu->Go( where, false, open_anyway )) )
			{
				item->SetTarget( this );
				(dynamic_cast<BInvoker *>(item))->Invoke();
			}
			delete menu;
		}
			break;

		case M_ADD:
			if (!fPanel) {
				me = new BMessenger(this);
				BMessage msg(REFS_RECEIVED);
				fPanel = new BFilePanel(B_OPEN_PANEL, me, &open_dir, 
								   false, true, &msg);
				delete me;
			}
			else if (!fPanel->Window()->IsHidden())
					fPanel->Window()->Activate();

			if (fPanel->Window()->IsHidden())
				fPanel->Window()->Show();
			break;

		case M_REMOVE:
			PostMessage(msg->what, fEnclosuresView);
			break;

		case M_TO_MENU:
		case M_CC_MENU:
		case M_BCC_MENU:
			fHeaderView->SetAddress(msg);
			break;

		case REFS_RECEIVED:
			AddEnclosure(msg);
			break;

		//
		//	Navigation Messages
		//
		case M_PREVMSG:
		case M_NEXTMSG:
		{
			entry_ref nextRef = *fRef;
			if (GetTrackerWindowFile(&nextRef, (msg->what == M_NEXTMSG))) {
				SetCurrentMessageRead();
				OpenMessage(&nextRef);
				SetTrackerSelectionToCurrent();
			} else {
				beep();		
			}
			break;
		}
		case M_OPEN_MAIL_FOLDER:
		case M_OPEN_MAIL_BOX:
		{
			BEntry folderEntry;
			BPath path;
			// Get the user home directory
			if( find_directory( B_USER_DIRECTORY, &path ) != B_OK )
				break;
			if(msg->what == M_OPEN_MAIL_FOLDER)
				path.Append( kMailFolder );
			else
				path.Append( kMailboxFolder );
			if( folderEntry.SetTo( path.Path() ) == B_OK && folderEntry.Exists() )
			{
				BMessage thePackage( B_REFS_RECEIVED );
				BMessenger nike( "application/x-vnd.Be-TRAK" );
				
				entry_ref ref;
				folderEntry.GetRef( &ref );
				thePackage.AddRef( "refs", &ref );
				nike.SendMessage( &thePackage );
			}
			break;
		}
		case RESET_BUTTONS:
			fChanged = false;
			fFieldState = 0;
			if( fHeaderView->fTo->TextView()->TextLength() )
				fFieldState |= FIELD_TO;
			if( fHeaderView->fSubject->TextView()->TextLength() )
				fFieldState |= FIELD_SUBJECT;
			if( fHeaderView->fCc->TextView()->TextLength() )
				fFieldState |= FIELD_CC;
			if( fHeaderView->fBcc->TextView()->TextLength() )
				fFieldState |= FIELD_BCC;
			if( fContentView->fTextView->TextLength() )
				fFieldState |= FIELD_BODY;
				
			if( fSaveButton )
				fSaveButton->SetEnabled( false );
			if( fPrintButton )
				fPrintButton->SetEnabled( fFieldState );
			if( fSendButton )
				fSendButton->SetEnabled( (fFieldState & FIELD_TO)||(fFieldState & FIELD_BCC) );
			break;
		case M_CHECK_SPELLING:
			if( !gDictCount )
			{
				beep();
				(new BAlert("",
					"The spell check feature requires the optional \"words\" file on your BeOS CD.",
					"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
					B_STOP_ALERT))->Go();
			}
			else
			{
				fSpelling->SetMarked( !fSpelling->IsMarked() );
				fContentView->fTextView->EnableSpellCheck( fSpelling->IsMarked() );
			}
			break;
			
		default:
			BWindow::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------
void 
TMailWindow::AddEnclosure(BMessage *msg)
{
	if ((fEnclosuresView == NULL) && (!fIncoming)) {
		BRect r;
		r.left = 0;
		r.top = fHeaderView->Frame().top + HEADER_HEIGHT;
		r.right = Frame().Width() + 2;
		r.bottom = r.top + ENCLOSURES_HEIGHT;

		fEnclosuresView = new TEnclosuresView(r, Frame());
		AddChild(fEnclosuresView, fContentView);
		fContentView->ResizeBy(0, -ENCLOSURES_HEIGHT);
		fContentView->MoveBy(0, ENCLOSURES_HEIGHT);

	}

	if ((fEnclosuresView) && (msg->HasRef("refs"))) {
		PostMessage(msg, fEnclosuresView);
		
		fChanged = true;
		BEntry entry;
		entry_ref ref;
		msg->FindRef("refs", &ref);
		entry.SetTo(&ref);
		entry.GetParent(&entry);
		entry.GetRef(&open_dir);
	}
}


bool TMailWindow::QuitRequested()
{
	char		status[256];
	char		tmp[256];
	const char	*str = NULL;
	int32		result;
	BFile		file;
	BMessage	message(WINDOW_CLOSED);
	attr_info	info;

	if ((!fIncoming ||
		((fIncoming) && (fResending))) && (fChanged) && (!fSent) &&
					((strlen(fHeaderView->fTo->Text())) ||
					(strlen(fHeaderView->fSubject->Text())) ||
					(fHeaderView->fCc && strlen(fHeaderView->fCc->Text())) ||
					(fHeaderView->fBcc && strlen(fHeaderView->fBcc->Text())) ||
					(strlen(fContentView->fTextView->Text())) ||
					(fEnclosuresView != NULL &&
					(fEnclosuresView->fList->CountItems())))) {
		if( fResending )
		{
			result = (new BAlert("",
				"Do you wish to send this message before closing?",
				"Discard", "Cancel", "Send", B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
				B_WARNING_ALERT))->Go();
			switch (result) {
			case 0:	// Discard
				break;
			case 1:	// Cancel
				return false;
			case 2:	// Send
				Send(true);
				break;
			}
		}
		else
		{
			result = (new BAlert("",
				"Do you wish to save this message as a draft before closing?",
				"Don't Save", "Cancel", "Save", B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
				B_WARNING_ALERT))->Go();
			switch (result) {
				case 0:	// Don't Save
					break;
				case 1:	// Cancel
					return false;
				case 2:	// Save
					Send(false);
					break;
			}
		}
	}
		
	message.AddInt32("kind", MAIL_WINDOW);
	message.AddPointer( "window", this );
	be_app->PostMessage(&message);

	if ((CurrentMessage()) && (CurrentMessage()->HasString("status"))) {

		//
		//	User explicitly requests a status to set this message to.
		//
		if (!CurrentMessage()->HasString("same")) {
			str = CurrentMessage()->FindString("status");
			if (str != 0) {
				char status[255];
				BFile file(fRef, O_RDWR);
				if (file.InitCheck() == B_NO_ERROR)
				{
					attr_info info;
					if (file.GetAttrInfo(B_MAIL_ATTR_STATUS, &info) 
					  == B_NO_ERROR)	
					{
						file.ReadAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, 
						  status, 255);
						file.RemoveAttr(B_MAIL_ATTR_STATUS);
						file.WriteAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 
						  0, str, strlen(str) + 1);
					}
				}
			}
		}
	}
	else if (fFile) {
		//
		//	...Otherwise just set the message read
		//
		SetCurrentMessageRead();
	}

	return true;
}

//--------------------------------------------------------------------

void TMailWindow::Show()
{
	BTextView	*text_view;

	Lock();
	if ((!fResending) && ((fIncoming) || (fReplying)))
		fContentView->fTextView->MakeFocus(true);
	else {
		text_view = (BTextView *)fHeaderView->fTo->ChildAt(0);
		fHeaderView->fTo->MakeFocus(true);
		text_view->Select(0, text_view->TextLength());
	}
	Unlock();
	BWindow::Show();
}

//--------------------------------------------------------------------

void TMailWindow::Zoom(BPoint pos, float x, float y)
{
	float		height;
	float		width;
	BScreen		screen(this);
	BRect		r;
	BRect		s_frame = screen.Frame();

	r = Frame();
	width = 80 * ((TMailApp*)be_app)->fFont.StringWidth("M") +
			(r.Width() - fContentView->fTextView->Bounds().Width() + 6);
	if (width > (s_frame.Width() - 8))
		width = s_frame.Width() - 8;
	height = max_c(fContentView->fTextView->CountLines(), 20) *
			  fContentView->fTextView->LineHeight(0) +
			  (r.Height() - fContentView->fTextView->Bounds().Height());
	if (height > (s_frame.Height() - 29))
		height = s_frame.Height() - 29;
	r.right = r.left + width;
	r.bottom = r.top + height;

	if (abs((int)(Frame().Width() - r.Width())) < 5 &&
		abs((int)(Frame().Height() - r.Height())) < 5)
		r = fZoom;
	else {
		fZoom = Frame();
		s_frame.InsetBy(6, 6);

		if (r.Width() > s_frame.Width())
			r.right = r.left + s_frame.Width();
		if (r.Height() > s_frame.Height())
			r.bottom = r.top + s_frame.Height();

		if (r.right > s_frame.right) {
			r.left -= r.right - s_frame.right;
			r.right = s_frame.right;
		}
		if (r.bottom > s_frame.bottom) {
			r.top -= r.bottom - s_frame.bottom;
			r.bottom = s_frame.bottom;
		}
		if (r.left < s_frame.left) {
			r.right += s_frame.left - r.left;
			r.left = s_frame.left;
		}
		if (r.top < s_frame.top) {
			r.bottom += s_frame.top - r.top;
			r.top = s_frame.top;
		}
	}

	ResizeTo(r.Width(), r.Height());
	MoveTo(r.LeftTop());
}

//--------------------------------------------------------------------

void TMailWindow::WindowActivated(bool status)
{
	if (status) {
		sWindowList.RemoveItem(this);
		sWindowList.AddItem(this, 0);
	}
}

//--------------------------------------------------------------------

void TMailWindow::Forward(entry_ref *ref)
{
	char		*str;
	char		*str1;
	BFile		*file;
	attr_info	info;

	file = new BFile(ref, O_RDONLY);
	if (file->InitCheck() == B_NO_ERROR) {
		if (file->GetAttrInfo(B_MAIL_ATTR_SUBJECT, &info) == B_NO_ERROR) {
			str = (char *)malloc(info.size);
			file->ReadAttr(B_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, str, info.size);
			if ((strstr(str, "fwd")) || (strstr(str, "forward")) ||
				(strstr(str, "FW")) || (strstr(str, "FORWARD")))
				fHeaderView->fSubject->SetText(str);
			else {
				str1 = (char *)malloc(strlen(str) + 1 + 6);
				sprintf(str1, "%s (fwd)", str);
				fHeaderView->fSubject->SetText(str1);
				free(str1);
			}
			free(str);
		}
		fContentView->fTextView->fHeader = true;
		fContentView->fTextView->LoadMessage(file, false, true, "Forwarded message:\n");
		fChanged = false;
		fFieldState = 0;
	}
	else
		delete file;
}

//--------------------------------------------------------------------

void TMailWindow::Print()
{
	if (!print_settings) {
		PrintSetup();
		if (!print_settings)
			return;
	}

	BPrintJob print("mail_print");
	print.SetSettings(new BMessage(*print_settings));

	if (print.ConfigJob() == B_NO_ERROR)
	{
		int32 curPage = 1;
		int32 lastLine = 0;
		int32 maxLine = fContentView->fTextView->CountLines();
		BRect pageRect = print.PrintableRect();
		BRect curPageRect = pageRect;	

		print.BeginJob();
		
		do
		{
			int32 lineOffset = fContentView->fTextView->OffsetAt(lastLine);
			curPageRect.OffsetTo(0, fContentView->fTextView->PointAt(lineOffset).y);
			
			int32 fromLine = lastLine;
			lastLine = fContentView->fTextView->LineAt(BPoint(0.0, curPageRect.bottom));
			
			float curPageHeight = fContentView->fTextView->TextHeight(fromLine, lastLine);
			if(curPageHeight > pageRect.Height())
			{
				curPageHeight = fContentView->fTextView->TextHeight(fromLine, --lastLine);
			}
			curPageRect.bottom = curPageRect.top + curPageHeight - 1.0;
			
			if((curPage >= print.FirstPage()) &&
				(curPage <= print.LastPage()))
			{
				print.DrawView(fContentView->fTextView, curPageRect, BPoint(0.0, 0.0));
				print.SpoolPage();
			}
			
			curPageRect = pageRect;
			lastLine++;
			curPage++;
		
		} while (print.CanContinue() && (lastLine < maxLine));

		print.CommitJob();

out:;
	}
}

//--------------------------------------------------------------------

void TMailWindow::PrintSetup()
{
	BPrintJob	print("mail_print");
	status_t	result;

	if (print_settings)
		print.SetSettings(new BMessage(*print_settings));

	if ((result = print.ConfigPage()) == B_NO_ERROR) {
		delete print_settings;
		print_settings = print.Settings();
	}
}

void 
TMailWindow::SetTo(const char *mailTo, const char *subject, const char *ccTo,
	const char *bccTo, const BString *body, BMessage *enclosures)
{
	Lock();
	if (mailTo && mailTo[0])
		fHeaderView->fTo->SetText(mailTo);
	if (subject && subject[0])
		fHeaderView->fSubject->SetText(subject);
	if (ccTo && ccTo[0])
		fHeaderView->fCc->SetText(ccTo);
	if (bccTo && bccTo[0])
		fHeaderView->fBcc->SetText(bccTo);
		
	if (body && body->Length()) {
		fContentView->fTextView->SetText(body->String(), body->Length());
		fContentView->fTextView->GoToLine(0);
	}

	if (enclosures && enclosures->HasRef("refs")) 
		AddEnclosure(enclosures);
	Unlock();
}

void TMailWindow::CopyMessage( entry_ref *ref, TMailWindow *src )
{
	status_t status;
	attr_info info;
	BNode file( ref );
	BString string;
	
	if( (status=file.InitCheck()) == B_OK )
	{
		char *str;
		
		if( fHeaderView->fTo && ((status=file.GetAttrInfo( B_MAIL_ATTR_TO, &info )) == B_NO_ERROR) )
		{
			str = string.LockBuffer(info.size+1);
			str[info.size] = 0;
			file.ReadAttr( B_MAIL_ATTR_TO, B_STRING_TYPE, 0, str, info.size );
			string.UnlockBuffer( info.size+1 );
			fHeaderView->fTo->SetText( string.String() );
		}
		if( fHeaderView->fSubject && ((status=file.GetAttrInfo( B_MAIL_ATTR_SUBJECT, &info )) == B_NO_ERROR) )
		{
			str = string.LockBuffer(info.size+1);
			str[info.size] = 0;
			file.ReadAttr( B_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, str, info.size );
			string.UnlockBuffer( info.size+1 );
			fHeaderView->fSubject->SetText( string.String() );
		}
		if( fHeaderView->fCc && ((status=file.GetAttrInfo( B_MAIL_ATTR_CC, &info )) == B_NO_ERROR) )
		{
			str = string.LockBuffer(info.size+1);
			str[info.size] = 0;
			file.ReadAttr( B_MAIL_ATTR_CC, B_STRING_TYPE, 0, str, info.size );
			string.UnlockBuffer( info.size+1 );
			fHeaderView->fCc->SetText( string.String() );
		}
	}
	fContentView->fTextView->SetText( src->fContentView->fTextView->Text(), src->fContentView->fTextView->TextLength() );
}

void TMailWindow::Reply(entry_ref *ref, TMailWindow *wind, bool all)
{
	char		*to = NULL;
	char		*cc;
	char		*header;
	char		*str;
	char		*str1;
	int32		finish;
	int32		len;
	int32		loop;
	int32		start;
	BFile		*file = NULL;
	attr_info	info;

	file = new BFile(ref, O_RDONLY);
	if (file->InitCheck() == B_NO_ERROR) {
		if (file->GetAttrInfo(B_MAIL_ATTR_REPLY, &info) == B_NO_ERROR) {
			to = (char *)malloc(info.size);
			file->ReadAttr(B_MAIL_ATTR_REPLY, B_STRING_TYPE, 0, to, info.size);
			fHeaderView->fTo->SetText(to);
		}
		else if (file->GetAttrInfo(B_MAIL_ATTR_FROM, &info) == B_NO_ERROR) {
			to = (char *)malloc(info.size);
			file->ReadAttr(B_MAIL_ATTR_FROM, B_STRING_TYPE, 0, to, info.size);
			fHeaderView->fTo->SetText(to);
		}

		if (file->GetAttrInfo(B_MAIL_ATTR_SUBJECT, &info) == B_NO_ERROR) {
			str = (char *)malloc(info.size);
			file->ReadAttr(B_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, str, info.size);
			if (cistrncmp(str, "re:", 3) != 0) {
				str1 = (char *)malloc(strlen(str) + 1 + 4);
				sprintf(str1, "Re: %s", str);
				fHeaderView->fSubject->SetText(str1);
				free(str1);
			} else {
				fHeaderView->fSubject->SetText(str);
			}
			free(str);
		}

		wind->fContentView->fTextView->GetSelection(&start, &finish);
		if (start != finish) {
			str = (char *)malloc(finish - start + 1);
			wind->fContentView->fTextView->GetText(start, finish - start, str);
			if (str[strlen(str) - 1] != '\n') {
				str[strlen(str)] = '\n';
				finish++;
			}
			fContentView->fTextView->SetText(str, finish - start);
			free(str);

			finish = fContentView->fTextView->CountLines() - 1;
			for (loop = 0; loop < finish; loop++) {
				fContentView->fTextView->GoToLine(loop);
				fContentView->fTextView->Insert((const char *)QUOTE);
			}
			fContentView->fTextView->GoToLine(0);
		}
		else if (file)
			fContentView->fTextView->LoadMessage(file, true, true, NULL);

		if (all) {
			cc = (char *)malloc(1);
			cc[0] = 0;
			len = header_len(file);
			header = (char *)malloc(len);
			file->Seek(0, 0);
			file->Read(header, len);
			get_recipients(&cc, header, len, false);
			if (strlen(cc)) {
				if (to) {
					if (str = cistrstr(cc, to)) {
						len = 0;
						if (str == cc) {
							while ((strlen(to) + len < strlen(cc)) &&
								   ((str[strlen(to) + len] == ' ') ||
									(str[strlen(to) + len] == ','))) {
								len++;
							}
						}
						else {
							while ((str > cc) && ((str[-1] == ' ') || (str[-1] == ','))) {
								str--;
								len++;
							}
						}
						memmove(str, &str[strlen(to) + len], &cc[strlen(cc)] - 
														 &str[strlen(to) + len] + 1);
					}
				}
				fHeaderView->fCc->SetText(cc);
			}
			free(cc);
			free(header);
		}
		if (to) {
			free(to);
		}
		
		fReplying = true;
	}
}

//--------------------------------------------------------------------

status_t TMailWindow::Send(bool now)
{
	if( !now )
	{
		status_t status;
		
		if( (status = SaveAsDraft()) != B_OK )
		{
			beep();
			(new BAlert("","E-mail draft could not be saved!", "OK"))->Go();
		}
		return status;
	}
	
	bool			close = false;
	char			mime[256];
	int32			index = 0;
	int32			len;
	BMailMessage	*mail;
	status_t		result;
	TListItem		*item;

	if (fResending)
		result = forward_mail(fRef, fHeaderView->fTo->Text(), now);
	else {
		mail = new BMailMessage();
				
		if (len = strlen(fHeaderView->fTo->Text()))
			mail->AddHeaderField(mail_encoding, B_MAIL_TO, fHeaderView->fTo->Text());

		if (len = strlen(fHeaderView->fSubject->Text()))
			mail->AddHeaderField(mail_encoding, B_MAIL_SUBJECT, 
			  fHeaderView->fSubject->Text());

		if (len = strlen(fHeaderView->fCc->Text()))
			mail->AddHeaderField(mail_encoding, B_MAIL_CC, 
			  fHeaderView->fCc->Text());

		if (len = strlen(fHeaderView->fBcc->Text()))
			mail->AddHeaderField(mail_encoding, B_MAIL_BCC, 
			  fHeaderView->fBcc->Text());

		/* add a message-id */
		BString message_id;
		/* empirical evidence indicates message id must be enclosed in
		** angle brackets and there must be an "at" symbol in it
		*/
		message_id << "<";
		message_id << system_time();
		message_id << "-BeMail@";
		
		utsname uinfo;
		uname(&uinfo);
		message_id << uinfo.nodename;

		message_id << ">";
		mail->AddHeaderField(mail_encoding, "Message-Id: ", message_id.String());
		/****/
		
		if (len = fContentView->fTextView->TextLength())
			fContentView->fTextView->AddAsContent(mail, wrap_mode);

		if (fEnclosuresView != NULL) {
			while (item = (TListItem *)fEnclosuresView->fList->ItemAt(index++)){
				mail->AddEnclosure(item->Ref());
			}
		}
		result = mail->Send(now);
		delete mail;
	}

	switch (result) {
		case B_NO_ERROR:
			close = true;
			fSent = true;
			
			// If it's a draft, remove the draft file
			if( fDraft )
			{
				BEntry entry( fRef );
				entry.Remove();
			}
			break;

		case B_MAIL_NO_DAEMON:
			close = true;
			fSent = true;
			sprintf(mime, "The mail_daemon is not running.  The message is "
				"queued and will be sent when the mail_daemon is started.");
			break;

		case B_MAIL_UNKNOWN_HOST:
		case B_MAIL_ACCESS_ERROR:
			sprintf(mime, "An error occurred trying to connect with the SMTP "
				"host.  Check your SMTP host name.");
			break;

		case B_MAIL_NO_RECIPIENT:
			sprintf(mime, "You must have either a \"To\" or \"Bcc\" recipient.");
			break;

		default:
			sprintf(mime, "An error occurred trying to send mail (0x%.8x).",
							result);
	}
	if (result != B_NO_ERROR) {
		beep();
		(new BAlert("", mime, "OK"))->Go();
	}
	if (close)
		PostMessage(B_QUIT_REQUESTED);
	return result;
}

//--------------------------------------------------------------------

status_t TMailWindow::SaveAsDraft( void )
{
	status_t	status;
	BPath		draftPath;
	BDirectory	dir;
	BFile		draft;
	uint32		flags = 0;
	
	if( fDraft )
	{
		if( (status=draft.SetTo( fRef, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE  )) != B_OK )
			return status;
	}
	else
	{
		// Get the user home directory
		if( (status = find_directory( B_USER_DIRECTORY, &draftPath )) != B_OK )
			return status;
		
		// Append the relative path of the draft directory
		draftPath.Append( kDraftPath );
		
		// Create the file
		status = dir.SetTo( draftPath.Path() );
		switch( status )
		{
			// Create the directory if it does not exist
			case B_ENTRY_NOT_FOUND:
				if( (status = dir.CreateDirectory( draftPath.Path(), &dir )) != B_OK )
					return status;
			case B_OK:
				{
					char fileName[512];
					int32 i;
					
					flags = B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS;
					// Create the file; if the name exists, find a unique name
					for( i=1, strcpy( fileName, fHeaderView->fSubject->Text() ); (status = draft.SetTo(&dir, fileName, flags )) != B_OK; i++ )
					{
						if( status != B_FILE_EXISTS )
							return status;
						sprintf( fileName, "%s %ld", fHeaderView->fSubject->Text(), i );
					}
					// Cache the ref
					if( fRef )
						delete fRef;
					BEntry entry( &dir, fileName );
					fRef = new entry_ref;
					entry.GetRef( fRef );
				}
				break;
			default:
				return status;
		}
	}
	
	// Write the content of the message
	draft.Write( fContentView->fTextView->Text(), fContentView->fTextView->TextLength() );
	
	//
	// Add the header stuff as attributes
	//
	draft.WriteAttr( B_MAIL_ATTR_NAME, B_STRING_TYPE, 0, fHeaderView->fTo->Text(), strlen(fHeaderView->fTo->Text()) );
	draft.WriteAttr( B_MAIL_ATTR_TO, B_STRING_TYPE, 0, fHeaderView->fTo->Text(), strlen(fHeaderView->fTo->Text()) );
	draft.WriteAttr( B_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, fHeaderView->fSubject->Text(), strlen(fHeaderView->fSubject->Text()) );
	draft.WriteAttr( B_MAIL_ATTR_CC, B_STRING_TYPE, 0, fHeaderView->fCc->Text(), strlen(fHeaderView->fCc->Text()) );
	draft.WriteAttr( "MAIL:bcc", B_STRING_TYPE, 0, fHeaderView->fBcc->Text(), strlen(fHeaderView->fBcc->Text()) );
	
	// Add the draft attribute for indexing
	uint32 draftAttr = true;
	draft.WriteAttr( "MAIL:draft", B_INT8_TYPE, 0, &draftAttr, sizeof(uint32) );
	
	// Add Attachment paths in attribute
	if (fEnclosuresView != NULL)
	{
		TListItem *item;
		BPath path;
		BString pathStr;
		
		for( int32 i=0; (item = (TListItem *)fEnclosuresView->fList->ItemAt(i)); i++ )
		{
			if( i )
				pathStr.Append( ":" );
			BEntry entry( item->Ref(), true );
			
			entry.GetPath( &path );
			pathStr.Append( path.Path() );
		}
		if( pathStr.Length() )
			draft.WriteAttr( "MAIL:attachments", B_STRING_TYPE, 0, pathStr.String(), pathStr.Length() );
	}
	
	// Set the MIME Type of the file
	BNodeInfo		info( &draft );
	info.SetType( kDraftType );
	
	fSent = true;
	fDraft = true;
	fChanged = false;
	
	return B_OK;
}

//
//	Open *another* message in the existing mail window.  Some code here is 
//	duplicated from various constructors.  
//
status_t TMailWindow::OpenMessage(entry_ref *ref)
{
	
	//
	//	Set some references to the email file
	//
	if( fRef )
		delete fRef;
	
	fRef = new entry_ref(*ref);

	delete fFile;
	fFile = new BFile(fRef, O_RDONLY);
	status_t err = fFile->InitCheck();
	if (err != B_OK) {
		delete fFile;
		return err;
	}
	
	char mimeType[256];
	BNodeInfo fileInfo( fFile );
	fileInfo.GetType( mimeType );
	
	// Check if it's a draft file
	if( strcmp( kDraftType, mimeType ) == 0 )
	{
		off_t size;
		fFile->GetSize( &size );
		fContentView->fTextView->SetText( fFile, 0, size );
		fIncoming = false;
		fDraft = true;
	}
	else
	{
		fIncoming = true;
		fHeaderView->LoadMessage(fFile);
	}
	
	//
	//	Figure out the title of this message and set the title bar
	//
	char *title=NULL;
	BFile file;
	attr_info info;
	file.SetTo(ref, O_RDONLY);
	
	if (fIncoming && (file.InitCheck() == B_NO_ERROR)) {	
		if (file.GetAttrInfo(B_MAIL_ATTR_NAME, &info) == B_NO_ERROR) {
			char *str1 = (char*) malloc(info.size+1);
			file.ReadAttr(B_MAIL_ATTR_NAME, B_STRING_TYPE, 0, str1, 
			  info.size);
			str1[info.size] = 0;
			if (file.GetAttrInfo(B_MAIL_ATTR_SUBJECT, &info) == B_NO_ERROR){
				char *str2 = (char*) malloc(info.size+1);
				str2[info.size] = 0;
				file.ReadAttr(B_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, str2, 
				  info.size);
	
				//	
				//	Get the title for the window
				//
				title = (char *)malloc(strlen(str1) + strlen(str2) + 3);
				sprintf(title, "%s->%s", str1, str2);
				free(str1);
				free(str2);
			}
			else
				title = str1;
		}
		else if( fHeaderView->fSubject->TextView()->TextLength() )
		{
			title = (char *) malloc (fHeaderView->fSubject->TextView()->TextLength() + 1);
			strcpy( title, fHeaderView->fSubject->Text() );
		}
		else {
		/* if something slips through the cracks (No NAME, no SUBJECT) */
		/* we just play it safe. */
		title = (char *) malloc (strlen("BeMail") + 1);
		strcpy(title, "BeMail");
		}
	}
	SetTitle(title);
	free(title);
	
	if( fIncoming )
	{
		//
		//	Put the addresses in the 'Save Address' Menu
		//
		BMenuItem *i = saveAddrMenu->RemoveItem(0L);
		while (i != NULL)
		{
			delete i;
			i = saveAddrMenu->RemoveItem(0L);
		}
	
		char *recipients = (char *)malloc(1);
		recipients[0] = 0;
		int32 len = header_len(fFile);
		char *header = (char *)malloc(len);
		fFile->Seek(0, 0);
		fFile->Read(header, len);
		get_recipients(&recipients, header, len, true);
		char *list = recipients;
		BMenuItem *item = NULL;
		int32 index = 0;
		bool done = false;
		BMessage *msg;
		while (1) {
			if ((!list[index]) || (list[index] == ',')) {
				if (!list[index])
					done = true;
				else
					list[index] = 0;
				int32 index1 = 0;
				while (item = saveAddrMenu->ItemAt(index1)) {
					if (strcmp(list, item->Label()) == 0)
						goto skip;
					if (strcmp(list, item->Label()) < 0)
						break;
					index1++;
				}
				msg = new BMessage(M_SAVE);
				msg->AddString("address", list);
				saveAddrMenu->AddItem(new BMenuItem(list, msg), index1);
	
	skip:			if (!done) {
					list += index + 1;
					index = 0;
					while (*list) {
						if (*list != ' ')
							break;
						else
							list++;
					}
				}
				else
					break;
			}
			else
				index++;
		}
		free(header);
		free(recipients);
	
		//
		// Clear out existing contents of text view. 
		//
		fContentView->fTextView->SetText("", (int32)0);
	
		fContentView->fTextView->LoadMessage(fFile, false, false, NULL);
	}
	else
	{
		char *str1;
		
		// Restore Fields from attributes
		if( (file.GetAttrInfo( B_MAIL_ATTR_TO, &info ) == B_NO_ERROR) )
		{
			str1 = (char*) malloc(info.size+1);
			str1[info.size] = 0;
			file.ReadAttr( B_MAIL_ATTR_TO, B_STRING_TYPE, 0, str1, info.size );
			fHeaderView->fTo->SetText( str1 );
			free( str1 );
		}
		
		if( (file.GetAttrInfo( B_MAIL_ATTR_SUBJECT, &info ) == B_NO_ERROR) )
		{
			str1 = (char*) malloc(info.size+1);
			str1[info.size] = 0;
			file.ReadAttr( B_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, str1, info.size );
			fHeaderView->fSubject->SetText( str1 );
			free( str1 );
		}
		
		if( (file.GetAttrInfo( "MAIL:bcc", &info ) == B_NO_ERROR) )
		{
			str1 = (char*) malloc(info.size+1);
			str1[info.size] = 0;
			file.ReadAttr( "MAIL:bcc", B_STRING_TYPE, 0, str1, info.size );
			fHeaderView->fBcc->SetText( str1 );
			free( str1 );
		}
		
		if( (file.GetAttrInfo( B_MAIL_ATTR_CC, &info ) == B_NO_ERROR) )
		{
			str1 = (char*) malloc(info.size+1);
			str1[info.size] = 0;
			file.ReadAttr( B_MAIL_ATTR_CC, B_STRING_TYPE, 0, str1, info.size );
			fHeaderView->fCc->SetText( str1 );
			free( str1 );
		}
		
		// Restore attachements
		if( (file.GetAttrInfo( "MAIL:attachments", &info ) == B_NO_ERROR) )
		{
			str1 = (char*) malloc(info.size+1);
			str1[info.size] = 0;
			file.ReadAttr( "MAIL:attachments", B_STRING_TYPE, 0, str1, info.size );
			BMessage msg( REFS_RECEIVED );
			entry_ref	enc_ref;
			
			char *s;
			s = strtok( str1, ":" );
			while( s )
			{
				BEntry entry( s, true );
				if( entry.Exists() )
				{
					entry.GetRef( &enc_ref );
					msg.AddRef( "refs", &enc_ref );
				}
				s = strtok( NULL, ":" );
			}
			AddEnclosure( &msg );
			free( str1 );
		}
		PostMessage( RESET_BUTTONS );
	}
	
	return B_OK;
}

//--------------------------------------------------------------------

TMailWindow* TMailWindow::FrontmostWindow()
{
	if (sWindowList.CountItems() > 0)
		return (TMailWindow*)sWindowList.ItemAt(0);
	else
		return NULL;
}

//====================================================================

TMenu::TMenu(const char *name, const char *attribute, int32 message, bool popup)
	  // :	BMenu(name),
	  : BPopUpMenu( name, false, false ),
		fMessage(message),
		fPopup( popup )
{
	fAttribute = (char *)malloc(strlen(attribute) + 1);
	strcpy(fAttribute, attribute);
	fPredicate = (char *)malloc(strlen(fAttribute) + 5);
	sprintf(fPredicate, "%s = *", fAttribute);
	BuildMenu();
}

//--------------------------------------------------------------------

TMenu::~TMenu()
{
	free(fAttribute);
	free(fPredicate);
}

//--------------------------------------------------------------------

void TMenu::AttachedToWindow()
{
	BuildMenu();
	BPopUpMenu::AttachedToWindow();
}

//--------------------------------------------------------------------

BPoint TMenu::ScreenLocation(void)
{
	if( fPopup )
		return BPopUpMenu::ScreenLocation();
	else
		return BMenu::ScreenLocation();
}

void TMenu::BuildMenu()
{
	char			name[B_FILE_NAME_LENGTH];
	int32			index = 0;
	BEntry			entry;
	BFile			file;
	BMenuItem		*item;
	BMessage		*msg;
	BQuery			query;
	BVolume			vol;
	BVolumeRoster	volume;
	entry_ref		ref;

	while (item = RemoveItem((int32)0)) {
		delete item;
	}
	volume.GetBootVolume(&vol);
	query.SetVolume(&vol);
	query.SetPredicate(fPredicate);
	query.Fetch();
	
	while (query.GetNextEntry(&entry) == B_NO_ERROR) {
		file.SetTo(&entry, O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			msg = new BMessage(fMessage);
			entry.GetRef(&ref);
			msg->AddRef("ref", &ref);
			file.ReadAttr(fAttribute, B_STRING_TYPE, 0, name, sizeof(name));
			if ((index < 9)&&(!fPopup))
				AddItem(new BMenuItem(name, msg, '1' + index));
			else
				AddItem(new BMenuItem(name, msg));
			index++;
		}
	}
	if( CountItems() )
	{
		AddItem( new BSeparatorItem(), 0 );
		//AddSeparatorItem();
		msg = new BMessage(M_RANDOM_SIG);
		if(!fPopup)
			AddItem( new BMenuItem( "Random", msg, '0' ), 0 );
		else
			AddItem( new BMenuItem( "Random", msg ), 0 );
	}
}

//====================================================================

int32 header_len(BFile *file)
{
	char	*buffer;
	int32	len;
	int32	result = 0;
	off_t	size;

	if (file->ReadAttr(B_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, &result, sizeof(int32)) != sizeof(int32)) {
		file->GetSize(&size);
		buffer = (char *)malloc(size);
		if (buffer) {
			file->Seek(0, 0);
			if (file->Read(buffer, size) == size) {
				while ((len = linelen(buffer + result, size - result, true)) > 2) {
					result += len;
				}
				result += len;
			}
			free(buffer);
			file->WriteAttr(B_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, &result, sizeof(int32));
		}
	}
	return result;
}
