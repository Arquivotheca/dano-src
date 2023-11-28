
/*	This control panel lets the user configure the default settings	*/
/*	that an application can use with translators. Typically, this will	*/
/*	also configure whatever settings the translator saves in its		*/
/*	settings file. Later, we might want to disable some translators	*/
/*	and do other such things from this control panel.				*/

#include <TranslationKit.h>
#include <Application.h>
#include <Window.h>
#include <ScrollView.h>
#include <ListView.h>
#include <Button.h>
#include <Box.h>
#include <List.h>
#include <Message.h>
#include <File.h>
#include <Directory.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Alert.h>
#include <StringView.h>
#include <Screen.h>
#include <NodeInfo.h>
#include <Mime.h>
#include <Bitmap.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


enum {
	INFO_CMD = 'cmd1',
	SAVE_CMD = 'cmd2',
	SELECTION_CMD = 'cmd3',
	REGEN_LIST = 'cmd4'
};


class CView;


class DListView : public BListView {
public:
		DListView(
				const BRect & frame,
				const char * name,
				list_view_type type = B_SINGLE_SELECTION_LIST,
				uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE) :
			BListView(frame, name, type, resize, flags)
			{
				m_hilited = false;
			}

virtual		void MessageReceived(
				BMessage * message)
			{
				switch (message->what) {
				case B_SIMPLE_DATA:
				case B_REFS_RECEIVED: {
					if (m_hilited) {
						m_hilited = false;
						Invalidate();
					}
					bool b = false;
					if (message->FindBool("be:_Translations", &b) || b) {
						if (message->HasRef("refs")) {
							Install(message);
							return;
						}
					}
					}
				default:
					BListView::MessageReceived(message);
					break;
				}
			}
virtual		void Draw(
				BRect area)
			{
				BListView::Draw(area);
				if (m_hilited) {
					rgb_color hi_color = HighColor();
					SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
					SetDrawingMode(B_OP_COPY);
					BRect r = Bounds();
					StrokeRect(r);
					r.InsetBy(1, 1);
					StrokeRect(r);
					SetHighColor(hi_color);
				}
			}
virtual		void MouseMoved(
				BPoint where,
				uint32 code,
				const BMessage *a_message)
			{
				if (!a_message || !a_message->HasRef("refs")) {
					BListView::MouseMoved(where, code, a_message);
					return;
				}
				bool b = false;
				if (!a_message->FindBool("be:_Translations", &b) && !b) {
					BListView::MouseMoved(where, code, a_message);
					return;
				}
				if (code == B_ENTERED_VIEW || code == B_INSIDE_VIEW) {
					if (!m_hilited) {
						m_hilited = true;
						rgb_color hi_color = HighColor();
						SetHighColor(ViewColor());
						FillRect(Bounds());
						SetHighColor(hi_color);
						Draw(Bounds());
						Flush();
					}
				}
				else if (code == B_EXITED_VIEW || code == B_OUTSIDE_VIEW) {
					if (m_hilited) {
						m_hilited = false;
						rgb_color hi_color = HighColor();
						SetHighColor(ViewColor());
						FillRect(Bounds());
						SetHighColor(hi_color);
						Draw(Bounds());
						Flush();
					}
				}
			}
private:

		void Install(
				BMessage * message)
			{
				entry_ref ref;
				BList badnames;
				BList overwrites;
				BList installs;
				BList successes;
				BPath td_path;
				bool do_install = true;
				BDirectory target;
				if ((find_directory(B_COMMON_TRANSLATORS_DIRECTORY, &td_path, true) < B_OK) || 
					(target.SetTo(td_path.Path()) < B_OK)) {
					do_install = false;
					(new BAlert("", "The user-installed Translators folder could not be found or created.", "Stop"))->Go();
				}
				if (do_install) for (int ix=0; !message->FindRef("refs", ix, &ref); ix++) {
					BNode ent(&ref);
					struct stat ent_st;
					ent.GetStat(&ent_st);
					char type[256];
					BNodeInfo info(&ent);
					if (!info.GetType(type) && strcasecmp(type, B_APP_MIME_TYPE)) {
						badnames.AddItem(strdup(ref.name));
					}
					else {
						if (target.Contains(ref.name)) {
							overwrites.AddItem(strdup(ref.name));
						}
						installs.AddItem(new entry_ref(ref));
					}
				}
				if (badnames.CountItems()) {
					char * str;
					int len = 100;
					for (int ix=0; ix<badnames.CountItems(); ix++) {
						len += strlen((char *)badnames.ItemAt(ix))+3;
					}
					str = (char *)malloc(len);
					if (badnames.CountItems() == 1) {
						strcpy(str, "The item '");
						strcat(str, (char *)badnames.ItemAt(0));
						strcat(str, "' does not appear to be a Translator and will not be installed.");
					}
					else {
						strcpy(str, "The following items do not appear to be Translators and will not be installed:\n");
						for (int ix=0; ix<badnames.CountItems(); ix++) {
							strcat(str, (char *)badnames.ItemAt(ix));
							if (ix < badnames.CountItems()-2) {
								strcat(str, ", ");
							}
							else if (ix == badnames.CountItems()-2) {
								strcat(str, " and ");
							}
							else {
								strcat(str, ".");
							}
						}
					}
					if (installs.CountItems()) {
						int sel = (new BAlert("", str, "Stop", "Install Others"))->Go();
						do_install = (sel == 1);
					}
					else {
						(new BAlert("", str, "Stop"))->Go();
						do_install = false;
					}
					free(str);
				}
				if (do_install && installs.CountItems()) {
					if (overwrites.CountItems()) {
						char * str;
						int len = 100;
						for (int ix=0; ix<overwrites.CountItems(); ix++) {
							len += strlen((char *)overwrites.ItemAt(ix))+3;
						}
						str = (char *)malloc(len);
						if (overwrites.CountItems() == 1) {
							strcpy(str, "An item named '");
							strcat(str, (char *)overwrites.ItemAt(0));
							strcat(str, "' already is in the Translators folder.");
						}
						else {
							strcpy(str, "Items with the following names already are in the Translators folder:\n");
							for (int ix=0; ix<overwrites.CountItems(); ix++) {
								strcat(str, (char *)overwrites.ItemAt(ix));
								if (ix < overwrites.CountItems()-2) {
									strcat(str, ", ");
								}
								else if (ix == overwrites.CountItems()-2) {
									strcat(str, " and ");
								}
								else {
									strcat(str, ".");
								}
							}
						}
						do_install = (0 == (new BAlert("", str, "Overwrite", "Stop"))->Go());
					}
				}
				for (int ix=0; ix<badnames.CountItems(); ix++) {
					free(badnames.ItemAt(ix));
				}
				badnames.MakeEmpty();
				for (int ix=0; ix<overwrites.CountItems(); ix++) {
					free(overwrites.ItemAt(ix));
				}
				overwrites.MakeEmpty();
				int installed_cnt = 0;
				if (do_install && installs.CountItems()) {
					int bufsize = 1024*128;
					char * buf = (char *)malloc(bufsize);
					char fallback[512];
					if (!buf) {
						buf = fallback;
						bufsize = 512;
					}
					for (int ix=0; ix<installs.CountItems(); ix++) {
						int old_cnt = installed_cnt;
						entry_ref * ref = (entry_ref *)installs.ItemAt(ix);
						{
							BFile src;
							BFile dest;
							if (src.SetTo(ref, O_RDONLY)) {
#if DEBUG
								fprintf(stderr, "src.SetTo()\n");
#endif
								badnames.AddItem(strdup(ref->name));
								goto bad;
							}
							if (dest.SetTo(&target, ref->name, O_RDWR | O_CREAT | O_TRUNC)) {
#if DEBUG
								fprintf(stderr, "dest.SetTo()\n");
#endif
								badnames.AddItem(strdup(ref->name));
								goto bad;
							}
							status_t rd, wr;
							off_t togo = 0;
							rd = src.GetSize(&togo);
							if (rd < 0) {
#if DEBUG
								fprintf(stderr, "src.GetSize()\n");
#endif
								badnames.AddItem(strdup(ref->name));
								goto bad;
							}
							while (togo > 0) {
								rd = src.Read(buf, bufsize);
								if (rd <= 0) {
#if DEBUG
									fprintf(stderr, "src.Read()\n");
#endif
									badnames.AddItem(strdup(ref->name));
									goto bad;
								}
								wr = dest.Write(buf, rd);
								if (wr < rd) {
#if DEBUG
									fprintf(stderr, "dest.Write()\n");
#endif
									badnames.AddItem(strdup(ref->name));
									goto bad;
								}
								togo -= rd;
							}
							mode_t perm = 0755;
							if (!src.GetPermissions(&perm)) {
								dest.SetPermissions(perm);
							}
						}
						{	/* update MIME info rather than copy attributes */
							BEntry ent(&target, ref->name);
							BPath path;
							ent.GetPath(&path);
							(void)update_mime_info(path.Path(), false, false, true);
							(void)create_app_meta_mime(path.Path(), false, false, true);
							installed_cnt++;
							successes.AddItem(ref);
						}
					bad:
						if (installed_cnt == old_cnt) {	/* remove failed entries */
							BEntry ent(&target, ref->name);
							ent.Remove();
						}
						if (!successes.HasItem(ref)) {
							delete ref;
						}
					}
					installs.MakeEmpty();
					if (buf != fallback) {
						free(buf);
					}
					if (badnames.CountItems()) {
							/* report bad items */
						char * str;
						int len = 100;
						for (int ix=0; ix<badnames.CountItems(); ix++) {
							len += strlen((char *)badnames.ItemAt(ix))+3;
						}
						str = (char *)malloc(len);
						if (badnames.CountItems() == 1) {
							strcpy(str, "The item '");
							strcat(str, (char *)badnames.ItemAt(0));
							strcat(str, "' failed to install properly.");
						}
						else {
							strcpy(str, "The following items failed to install properly:\n");
							for (int ix=0; ix<badnames.CountItems(); ix++) {
								strcat(str, (char *)badnames.ItemAt(ix));
								if (ix < badnames.CountItems()-2) {
									strcat(str, ", ");
								}
								else if (ix == badnames.CountItems()-2) {
									strcat(str, " and ");
								}
								else {
									strcat(str, ".");
								}
							}
						}
						(new BAlert("", str, "Bummer"))->Go();
						for (int ix=0; ix<badnames.CountItems(); ix++) {
							free(badnames.ItemAt(ix));
						}
						badnames.MakeEmpty();
					}
					if (installed_cnt) {
						char * buf = (char *)malloc(100);
						if (installed_cnt > 1) {
							sprintf(buf, "These items are now copied into the right place: ");
							for (int ix=0; ix<installed_cnt; ix++) {
								entry_ref * ref = (entry_ref *)successes.ItemAt(ix);
								buf = (char *)realloc(buf, strlen(buf)+strlen(ref->name)+100);
								if (!buf) break;
								if (ix == 0) {
									sprintf(buf+strlen(buf), "%s", ref->name);
								}
								else if (ix < installed_cnt-1) {
									sprintf(buf+strlen(buf), ", %s", ref->name);
								}
								else {
									sprintf(buf+strlen(buf), " and %s.\n", ref->name);
								}
							}
							if (buf != NULL) {
								strcat(buf, "What do you want to do with the originals?");
							}
						}
						else {
							entry_ref * ref = (entry_ref *)successes.ItemAt(0);
							buf = (char *)realloc(buf, strlen(ref->name)+100);
							if (buf != NULL) {
								sprintf(buf, "The item '%s' is now copied into the right place.\n"
									"What do you want to do with the original?", ref->name);
							}
						}
						if (buf != NULL) {
							int response = (new BAlert("", buf, "Remove", "Keep"))->Go();
							if (response == 0) {
								for (int ix=0; ix<successes.CountItems(); ix++) {
									BEntry ent((entry_ref *)successes.ItemAt(ix));
									if (ent.Remove() < 0) {
										sprintf(buf, "DataTranslations cannot remove '%s' from its original location."
												"You have to do it manually.", ((entry_ref *)successes.ItemAt(ix))->name);
										(new BAlert("", buf, "Continue"))->Go();
									}
								}
							}
							free(buf);
						}
						for (int ix=0; ix<successes.CountItems(); ix++) {
							delete (entry_ref *)successes.ItemAt(ix);
						}
						successes.MakeEmpty();
						(new BAlert("", 
						"You have to quit and re-start running applications for the installed Translators to be available in them.", "OK"))->Go();
						Window()->PostMessage(REGEN_LIST);
					}
				}
			}
		bool m_hilited;
};

class TranslationWindow : public BWindow {
public:
		TranslationWindow();
virtual	~TranslationWindow();

virtual	bool QuitRequested();
virtual	void MessageReceived(
				BMessage * message);

private:

		void IWindow();
		void DWindow();

//		BButton * m_save;
//		BButton * m_revert;
		BButton * m_info;

//		BScrollView * m_scroller;
		CView * m_container;
		DListView * m_list;
		BView * m_nothing;

		BList m_items;
		BList m_settings;
		bool m_settings_loaded;
		BPoint m_saved_location;

		BMessage * FindConfig(
				const char * name,
				int32 version);
		BPoint SavedLocation();

		void LoadSettings();
		void SaveSettings();

		void DoInfo();
		void DoSave();
		void DoSelection();
		void RegenList();

};


class BGView : public BView {
public:
		BGView(
				const BRect & area, 
				const char * name, 
				uint32 resize = B_FOLLOW_NONE, 
				uint32 flags = B_WILL_DRAW);
virtual	~BGView();

//		void SetDivider(
//				BPoint where);
virtual	void Draw(
				BRect area);
virtual	void SetViewColor(
				rgb_color c);

private:

//		BPoint m_divider;
};

class CView : public BView {
public:
		CView(BView * target, const char * name, uint32 resize, uint32 flags);
virtual	~CView();

		void SetTarget(
				BView * target, 
				translator_id id = 0);
		BView * Target();
virtual	void MouseDown(
				BPoint where);
virtual	void Draw(
				BRect area);
private:
		BView * m_target;
		BBitmap * m_icon;
		char * m_name;
		translator_id m_translator_id;
};


struct store_item {
	translator_id	translator;
	BView *			view;
	BRect			area;
	BMessage *		settings;
	char *			name;
	char *			infotext;
	int32			version;
};

struct saved_setting {
	char *			name;
	int32			version;
	BMessage *		message;
};


rgb_color the_gray;
rgb_color light_gray = the_gray;


TranslationWindow::TranslationWindow() :
	BWindow(BRect(50,50,450,350), "DataTranslations", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	IWindow();
}

void
TranslationWindow::IWindow()
{
	m_settings_loaded = false;

	BGView * bg = new BGView(Bounds(), "Background");
	AddChild(bg);

//	m_revert = new BButton(BRect(0,0,80,22), "Revert", "Revert", new BMessage(REVERT_CMD));
//	m_revert->MoveTo(m_save->Frame().left-10-m_revert->Bounds().Width(), 
//		Bounds().bottom-10-m_revert->Bounds().Height());
//	bg->AddChild(m_revert);

	m_info = new BButton(BRect(0,0,80,22), "Info…", "Info…", new BMessage(INFO_CMD));
	bg->AddChild(m_info);
	m_info->MoveTo(Bounds().right-m_info->Bounds().Width()-22, Bounds().bottom-22-m_info->Bounds().Height());

//	m_save = new BButton(BRect(0,0,80,22), "Save", "Save", new BMessage(SAVE_CMD));
//	m_save->MoveTo(Bounds().right-10-m_save->Bounds().Width(), 
//		Bounds().bottom-10-m_save->Bounds().Height());
//	bg->AddChild(m_save);
//	SetDefaultButton(m_save);

//	bg->SetDivider(BPoint(10, m_info->Frame().top-11));

	m_list = new DListView(BRect(12,12,120,Bounds().bottom-11), 
		"Translators", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	BScrollView * listScroll = new BScrollView("TranslatorScroller", m_list, 
		B_FOLLOW_LEFT|B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, false, true, B_FANCY_BORDER);
	bg->AddChild(listScroll);

	m_nothing = new BView(BRect(m_list->Frame().right+24+B_V_SCROLL_BAR_WIDTH, 11, 
		Bounds().right-11/*-B_V_SCROLL_BAR_WIDTH*/, Bounds().bottom-10/*-B_H_SCROLL_BAR_HEIGHT*/), 
		"_nothing", B_FOLLOW_ALL, B_WILL_DRAW);
	m_nothing->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	m_container = new CView(m_nothing, "SettingContainer", B_FOLLOW_ALL, B_WILL_DRAW);
	m_container->SetViewColor(light_gray);
	bg->AddChild(m_container);
	m_nothing->ResizeBy(0, m_info->Frame().top-m_container->Frame().bottom-10);
//	m_scroller = new BScrollView("SettingScroller", m_nothing, B_FOLLOW_ALL, B_WILL_DRAW, 
//		true, true, B_FANCY_BORDER);
//	bg->AddChild(m_scroller);

	listScroll->SetViewColor(the_gray);
//	m_container->SetViewColor(the_gray);
//	m_scroller->SetViewColor(the_gray);
	bg->SetViewColor(the_gray);
	{
		BBitmap * map = BTranslationUtils::GetBitmap("window.tga");
		if (map != NULL) {
			bg->SetViewBitmap(map);
		}
	}
//	m_save->SetViewColor(the_gray);
	m_info->SetViewColor(the_gray);

	int32 count = -1;
	translator_id * list = NULL;
	status_t error = BTranslatorRoster::Default()->GetAllTranslators(&list, &count);
	if ((error < B_OK) || (count < 1) || (list == NULL)) {
		char str[300];
		if (error >= B_OK) error = -1;
		sprintf(str, "There are no translators installed, or the translation kit could "
			"not be properly initialized.\n\n[%x] %s", 
			(uint)error, strerror(error));
		BAlert * alert = new BAlert("No translators", str, "Too Bad");
		alert->Go();
	}
	else for (int t=0; t<count; t++) {
		store_item * si = new store_item;
		si->translator = list[t];
		const char * name = NULL; const char * infotext = NULL;
		int32 version = 0;
		BTranslatorRoster::Default()->GetTranslatorInfo(list[t], &name, &infotext, &version);
		si->settings = FindConfig(name, version);
		si->view = NULL;
		si->area.Set(0,0,0,0);
		si->name = strdup(name);
		si->infotext = strdup(infotext);
		si->version = version;
		if (B_OK != BTranslatorRoster::Default()->MakeConfigurationView(list[t], 
			si->settings, &si->view, &si->area)) {
			char str[300];
			sprintf(str, "The translator '%s' has no settings.", name);
			si->view = new BStringView(BRect(0,0,300,20), "No Settings", 
				str, B_FOLLOW_ALL);
			font_height fh;
			be_plain_font->GetHeight(&fh);
			si->area.Set(0,0,be_plain_font->StringWidth(str),fh.ascent+fh.descent+fh.leading);
		}
		si->view->SetViewColor(light_gray);
		m_list->AddItem(new BStringItem(name));
		m_items.AddItem(si);
	}
	delete[] list;

	BRect r = m_info->Frame();
	bg->RemoveChild(m_info);
	m_container->ConvertFromParent(&r);
	m_info->MoveTo(r.LeftTop());
	m_container->AddChild(m_info);
	m_list->MakeFocus(true);

	if (m_list->CountItems()) {
		m_list->Select(0);
		m_list->SetSelectionMessage(new BMessage(SELECTION_CMD));
		DoSelection();
	}

	BPoint there = SavedLocation();
	{
		BScreen scrn(this);
		BRect r = scrn.Frame();
		r.InsetBy(5,15);
		if (r.Contains(there)) {
			MoveTo(there);
		}
	}
}


TranslationWindow::~TranslationWindow()
{
	DWindow();
}

void
TranslationWindow::DWindow()
{
//	m_scroller->RemoveChild(m_scroller->ScrollBar(B_HORIZONTAL)->Target());
	m_container->SetTarget(NULL);
	for (int ix=0; ix<m_items.CountItems(); ix++) {
		store_item *si = (store_item *)m_items.ItemAt(ix);
		delete si->view;
		free(si->name);
		free(si->infotext);
		delete si;
	}
	m_items.MakeEmpty();
	for (int ix=0; ix<m_list->CountItems(); ix++) {
		delete (BListItem *)m_list->ItemAt(ix);
	}
	for (int ix=0; ix<m_settings.CountItems(); ix++) {
		saved_setting * ss = (saved_setting *)m_settings.ItemAt(ix);
		delete ss->message;
		free(ss->name);
		delete ss;
	}
	m_settings.MakeEmpty();
	BView * v = ChildAt(0);
	v->RemoveSelf();
	delete v;
}


bool
TranslationWindow::QuitRequested()
{
	DoSave();
	SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
TranslationWindow::MessageReceived(
	BMessage * message)
{
	switch (message->what) {
	case INFO_CMD:
		DoInfo();
		break;
//	case SAVE_CMD:
//		DoSave();
//		break;
	case SELECTION_CMD:
		DoSelection();
		break;
	case REGEN_LIST:
		RegenList();
		break;
	default:
		BWindow::MessageReceived(message);
		break;
	}
}


BMessage *
TranslationWindow::FindConfig(
	const char * name,
	int32 version)
{
	saved_setting * ss = NULL;
	if (!m_settings_loaded) {
		LoadSettings();
	}
	for (int ix=0; ix<m_settings.CountItems(); ix++) {
		saved_setting * ns = (saved_setting *)m_settings.ItemAt(ix);
		if (!strcmp(ns->name, name)) {
			if (!ss || ((ns->version > ss->version) && (ns->version <= version))) {
				ss = ns;
			}
		}
	}
	if (!ss) {
#if DEBUG
//		fprintf(stderr, "creating new for %s %d\n", name, version);
#endif
		ss = new saved_setting;
		ss->name = strdup(name);
		ss->version = version;
		ss->message = new BMessage;
		m_settings.AddItem(ss);
	}
	return ss->message;
}


BPoint
TranslationWindow::SavedLocation()
{
	if (!m_settings_loaded) {
		m_saved_location = Frame().LeftTop();
		LoadSettings();
	}
#if DEBUG
//	printf("returning location: %g, %g\n", m_saved_location.x, m_saved_location.y);
#endif
	return m_saved_location;
}


struct file_header {
	char	magic[32];
	int32 version_ltl;
};
struct file_tag {
	char tag[8];
	size_t size_ltl;
};
#define VERSION 100
static char file_magic[32] = {
	'T', 'r', 'a', 'n', 's', 'l', 'a', 't', 
	'i', 'o', 'n', ' ', 'S', 'e', 't', 't', 
	'i', 'n', 'g', 's', ' ', 'f', 'i', 'l', 
	'e',  13,  10,  26,   0, 255, 254,   0,
};

void
TranslationWindow::LoadSettings()
{
	m_settings_loaded = true;
	BPath path;
	status_t err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (err < B_OK) {
		path.SetTo("/tmp");
	}
	path.Append("Translation Settings");
	BFile file;
	if (file.SetTo(path.Path(), O_RDONLY) != B_OK) {
#if DEBUG
		fprintf(stderr, "Translation Settings not found.\n");
#endif
		return;
	}
	file_header hdr;
	if ((file.Read(&hdr, sizeof(hdr)) != sizeof(hdr)) || memcmp(hdr.magic, file_magic, sizeof(file_magic))) {
#if DEBUG
		fprintf(stderr, "Translation Settings corrupt.\n");
#endif
		return;
	}
	if (B_LENDIAN_TO_HOST_INT32(hdr.version_ltl) > VERSION) {
#if DEBUG
		fprintf(stderr, "Translation Settings is too new (version %d, I'm %d).\n",
			B_LENDIAN_TO_HOST_INT32(hdr.version_ltl), VERSION);
#endif
		return;
	}
	file_tag tag;
	while (file.Read(&tag, sizeof(tag)) == sizeof(tag)) {
		off_t pos = file.Position();
		if (!strncmp(tag.tag, "setting ", 8)) {
			char * data = (char *)malloc(B_LENDIAN_TO_HOST_INT32(tag.size_ltl));
			if (!data) {
#if DEBUG
				fprintf(stderr, "Out of memory on %ld bytes for setting\n", 
					B_LENDIAN_TO_HOST_INT32(tag.size_ltl));
#endif
				goto skip;
			}
			if (file.Read(data, B_LENDIAN_TO_HOST_INT32(tag.size_ltl)) != 
				(ssize_t)B_LENDIAN_TO_HOST_INT32(tag.size_ltl)) {
#if DEBUG
				fprintf(stderr, "IO Error reading %ld bytes in setting\n", 
					B_LENDIAN_TO_HOST_INT32(tag.size_ltl));
#endif
				goto skip;
			}
			char * temp = data;
			int32 version;
			memcpy(&version, temp, sizeof(version));
			version = B_LENDIAN_TO_HOST_INT32(version);
			temp += sizeof(version);
			char * name = strdup(temp);
			temp += strlen(name)+1;
			BMessage * message = new BMessage;
			if (message->Unflatten(temp) < B_OK) {
#if DEBUG
				fprintf(stderr, "Message unflatten error in setting\n");
#endif
				free(name);
				delete message;
				goto skip;
			}
			free(data);
			saved_setting * ss = new saved_setting;
			ss->name = name;
			ss->version = version;
			ss->message = message;
			m_settings.AddItem(ss);
#if DEBUG
//			fprintf(stderr, "loaded setting %s %d\n", ss->name, ss->version);
#endif
		}
		else if (!strncmp(tag.tag, "panelpos", 8)) {
			BPoint pt;
			if (file.Read(&pt, sizeof(pt)) != sizeof(pt)) {
				goto skip;
			}
			pt.x = B_LENDIAN_TO_HOST_FLOAT(pt.x);
			pt.y = B_LENDIAN_TO_HOST_FLOAT(pt.y);
			m_saved_location = pt;
#if DEBUG
//			printf("read location: %g, %g\n", pt.x, pt.y);
#endif
		}
		else {
#if DEBUG
			fprintf(stderr, "unknown tag: %.8s\n", tag.tag);
#endif
		}
	skip:
		if (file.Seek(pos+B_LENDIAN_TO_HOST_INT32(tag.size_ltl), SEEK_SET) < B_OK) {
			break;
		}
	}
#if DEBUG
//	fprintf(stderr, "done loading settings\n");
#endif
}


void
TranslationWindow::SaveSettings()
{
#if DEBUG
//	fprintf(stderr, "SaveSettings() called\n");
#endif
	BPath path;
	status_t err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (err < B_OK) {
		path.SetTo("/tmp");
	}
	BPath real(path);
	real.Append("Translation Settings");
	path.Append("Translation Settings~");
	BFile file;
	if (file.SetTo(path.Path(), O_RDWR | O_CREAT | O_TRUNC) != B_OK) {
		goto write_err;
	}
	file_header hdr;
	memcpy(hdr.magic, file_magic, sizeof(file_magic));
	hdr.version_ltl = B_HOST_TO_LENDIAN_INT32(VERSION);
	if (file.Write(&hdr, sizeof(hdr)) != sizeof(hdr)) {
		goto write_err;
	}
	file_tag tag;
	strncpy(tag.tag, "setting ", 8);
#if DEBUG
//	printf("num to save: %d\n", m_settings.CountItems());
#endif
	for (int ix=0; ix<m_settings.CountItems(); ix++) {
#if DEBUG
//		fprintf(stderr, "writing setting %d\n", ix);
#endif
		off_t pos = file.Position();
		tag.size_ltl = 0;
		if (file.Write(&tag, sizeof(tag)) != sizeof(tag)) {
			goto write_err;
		}
		saved_setting * ss = (saved_setting *)m_settings.ItemAt(ix);
		int32 version = B_HOST_TO_LENDIAN_INT32(ss->version);
		if (file.Write(&version, sizeof(version)) != sizeof(version)) {
			goto write_err;
		}
		if (file.Write(ss->name, strlen(ss->name)+1) != (ssize_t)(strlen(ss->name)+1)) {
			goto write_err;
		}
		if (ss->message->Flatten(&file) < B_OK) {
			goto write_err;
		}
		off_t pos2 = file.Position();
		if (file.Seek(pos, SEEK_SET) < B_OK) {
			goto write_err;
		}
		tag.size_ltl = B_HOST_TO_LENDIAN_INT32(((int32)(pos2-(pos+sizeof(tag)))));
		if (file.Write(&tag, sizeof(tag)) != sizeof(tag)) {
			goto write_err;
		}
		if (file.Seek(pos2, SEEK_SET) < B_OK) {
			goto write_err;
		}
	}
	{
		strncpy(tag.tag, "panelpos", 8);
		tag.size_ltl = B_HOST_TO_LENDIAN_INT32(sizeof(BPoint));
		if (file.Write(&tag, sizeof(tag)) != sizeof(tag)) {
			goto write_err;
		}
		BPoint pt = Frame().LeftTop();
		pt.x = B_HOST_TO_LENDIAN_FLOAT(pt.x);
		pt.y = B_HOST_TO_LENDIAN_FLOAT(pt.y);
		if (file.Write(&pt, sizeof(pt)) != sizeof(pt)) {
			goto write_err;
		}
#if DEBUG
//		printf("Wrote location: %g,%g\n", pt.x, pt.y);
#endif
	}
	file.Sync();
	if (rename(path.Path(), real.Path()) < 0) {
#if DEBUG
		fprintf(stderr, "Error replacing old Translation Settings file.\n");
#endif
	}
	else {
#if DEBUG
//		fprintf(stderr, "Successfully saved settings\n");
#endif
	}
	return;
write_err:
#if DEBUG
	fprintf(stderr, "Error saving Translation Settings.\n");
#endif
	unlink(path.Path());
}


void
TranslationWindow::DoInfo()
{
	int32 index = m_list->CurrentSelection();
	if (index < 0) {
		BAlert * alrt = new BAlert("Panel Info", "Translation Settings\n\nUse this control panel to set values that various translators use when no other settings are specified in the using application.", 
			"OK");
		alrt->Go();
		return;
	}
	store_item * si = (store_item *)m_items.ItemAt(index);
	char * str = (char *)malloc(strlen(si->name)+strlen(si->infotext)+100);
	char versStr[20];
	if (si->version < 256) {
		sprintf(versStr, "%ld", si->version);
	}
	else {
		sprintf(versStr, "%ld.%ld.%ld", si->version>>8, (si->version>>4)&0xf, si->version&0xf);
	}
	sprintf(str, "Name: %s\nVersion: %s\nInfo: %s\nPath: ", si->name, versStr, 
		si->infotext);
	entry_ref ref;
	if (!BTranslatorRoster::Default()->GetRefFor(si->translator, &ref)) {
		BPath path;
		BEntry ent(&ref);
		ent.GetPath(&path);
		strcat(str, path.Path());
	}
	BAlert * alrt = new BAlert("Translator Info", str, "OK");
	alrt->Go();
}


void
TranslationWindow::DoSave()
{
	int32 index = m_list->CurrentSelection();
	if (index < 0) {
		return;
	}
	store_item * si = (store_item *)m_items.ItemAt(index);
	BTranslatorRoster::Default()->GetConfigurationMessage(si->translator, si->settings);
}


void
TranslationWindow::DoSelection()
{
	DoSave();
	int32 index = m_list->CurrentSelection();
	BView * to_target = NULL;
	translator_id id = (translator_id)-1;
	if (index < 0) {
		to_target = m_nothing;
	}
	else {
		to_target = ((store_item *)m_items.ItemAt(index))->view;
		id = ((store_item *)m_items.ItemAt(index))->translator;
	}
	if (!to_target) {
		to_target = m_nothing;
	}
//	BView * old_target = m_scroller->ScrollBar(B_HORIZONTAL)->Target();
	BView * old_target = m_container->Target();
	if (old_target != to_target) {
//		m_scroller->RemoveChild(old_target);
//		m_scroller->AddChild(to_target);
//		m_scroller->Invalidate();
		m_container->SetTarget(to_target, id);
	}
}


void
TranslationWindow::RegenList()
{
	DoSave();
	SaveSettings();
	DWindow();
	delete BTranslatorRoster::Default();
	IWindow();
}




BGView::BGView(
	const BRect & area, 
	const char * name, 
	uint32 resize, 
	uint32 flags) :
	BView(area, name, resize, flags)
{
//	m_divider.x = 10;
//	m_divider.y = area.bottom-50;
}

BGView::~BGView()
{
}

//void 
//BGView::SetDivider(
//	BPoint where)
//{
//	m_divider = where;
//	if (Window()->Lock()) {
//		Invalidate();
//		Window()->Unlock();
//	}
//}

void 
BGView::Draw(
	BRect area)
{
	BView::Draw(area);
	BRect r(Bounds());
	if (area.left <= r.left || area.top <= r.top || 
		area.right >= r.right || area.bottom >= r.bottom) {
		rgb_color hi = ViewColor();
		rgb_color lo = ViewColor();
#if 0
		hi.red = (1023 + hi.red)/5;
		hi.green = (1023 + hi.green)/5;
		hi.blue = (1023 + hi.blue)/5;
#else
		hi.red = hi.green = hi.blue = 255;
#endif
		lo.red = (uint8)(lo.red / 1.7);
		lo.green = (uint8)(lo.green / 1.7);
		lo.blue = (uint8)(lo.blue / 1.7);
		BeginLineArray(4);
		AddLine(r.LeftBottom(), r.LeftTop(), hi);
		AddLine(r.LeftTop(), r.RightTop(), hi);
		AddLine(r.RightTop(), r.RightBottom(), lo);
		AddLine(r.RightBottom(), r.LeftBottom(), lo);
		EndLineArray();
	}
}

void 
BGView::SetViewColor(
	rgb_color c)
{
	BView::SetViewColor(c);
	if (Window()->Lock()) {
		Invalidate();
		Window()->Unlock();
	}
}


	static BRect
	get_rect(
		BView * view)
	{
		BRect r = view->Frame();
		r.left -= 1;
		r.top -= 1;
		r.right += 1;
		r.bottom += 1;
		return r;
	}

CView::CView(BView * target, const char * name, uint32 resize, uint32 flags) :
	BView(get_rect(target), name, resize, flags | B_FULL_UPDATE_ON_RESIZE)
{
	m_target = NULL;
	m_icon = new BBitmap(BRect(0,0,31,31), B_COLOR_8_BIT);
	m_name = NULL;
	m_translator_id = 0;
	SetTarget(target);
}

CView::~CView()
{
	SetTarget(NULL);
	delete m_icon;
}

void
CView::SetTarget(
	BView * target,
	translator_id id)
{
	Window()->Lock();
	if (m_target) {
		RemoveChild(m_target);
	}
	free(m_name);
	m_name = NULL;
	m_translator_id = id;
	memset(m_icon->Bits(), B_TRANSPARENT_8_BIT, m_icon->BitsLength());
	entry_ref ref;
	status_t err = B_OK;
	if ((id > 0) && !(err = BTranslatorRoster::Default()->GetRefFor(id, &ref))) {
		m_name = strdup(ref.name);
		BNode node;
		err = node.SetTo(&ref);
		BNodeInfo info(&node);
		err = info.GetTrackerIcon(m_icon, B_LARGE_ICON);
	}
	if (target) {
		target->ResizeTo(Bounds().Width()-4, Bounds().Height()-48);
		target->MoveTo(2, 2);
		AddChild(target);
	}
	if (Window()) Invalidate();
	m_target = target;
	if (Window()) Window()->Unlock();
}

BView *
CView::Target()
{
	return m_target;
}


void
CView::MouseDown(
	BPoint where)
{
	BRect r(Bounds());
	r.left += 10;
	r.bottom -= 10;
	r.top = r.bottom-31;
	r.right = r.left+31;
	if (r.Contains(where)) {
		entry_ref ref;
		if (!BTranslatorRoster::Default()->GetRefFor(m_translator_id, &ref)) {
			BMessage msg(B_SIMPLE_DATA);
			msg.AddRef("refs", &ref);
			msg.AddBool("be:_Translations", false);
//			ConvertToScreen(&r);
			r.OffsetBy(2,2);
			DragMessage(&msg, r);
		}
	}
}


void
CView::Draw(
	BRect area)
{
	SetDrawingMode(B_OP_OVER);
	DrawBitmapAsync(m_icon, BPoint(Bounds().left+10, Bounds().bottom-41));
	SetDrawingMode(B_OP_COPY);
	SetLowColor(ViewColor());
	DrawString(m_name ? m_name : "", BPoint(Bounds().left+48, Bounds().bottom-20));
	area = area;
	rgb_color hi = ViewColor();
	rgb_color lo = ViewColor();
#if 0
	hi.red = (1023 + hi.red)/5;
	hi.green = (1023 + hi.green)/5;
	hi.blue = (1023 + hi.blue)/5;
#else
	hi.red = hi.green = hi.blue = 255;
#endif
	lo.red = (uint8)(lo.red / 1.8);
	lo.green = (uint8)(lo.green / 1.8);
	lo.blue = (uint8)(lo.blue / 1.8);
	BeginLineArray(8);
	BRect r = Bounds();
#if 0
	AddLine(r.LeftBottom(), r.LeftTop(), lo);
	AddLine(r.LeftTop(), r.RightTop(), lo);
	AddLine(r.RightTop(), r.RightBottom(), hi);
	AddLine(r.RightBottom(), r.LeftBottom(), hi);
	r.InsetBy(1, 1);
	AddLine(r.LeftBottom(), r.LeftTop(), hi);
	AddLine(r.LeftTop(), r.RightTop(), hi);
	AddLine(r.RightTop(), r.RightBottom(), lo);
	AddLine(r.RightBottom(), r.LeftBottom(), lo);
#else
	r.left += 1; r.top += 1;
	AddLine(r.LeftTop(), r.RightTop(), hi);
	AddLine(r.LeftTop(), r.LeftBottom(), hi);
	AddLine(r.RightBottom(), r.LeftBottom(), hi);
	AddLine(r.RightBottom(), r.RightTop(), hi);
	r.OffsetBy(-1, -1);
	AddLine(r.LeftTop(), r.RightTop(), lo);
	AddLine(r.LeftTop(), r.LeftBottom(), lo);
	AddLine(r.RightBottom(), r.LeftBottom(), lo);
	AddLine(r.RightBottom(), r.RightTop(), lo);
#endif
	EndLineArray();
}





int
main()
{
	BApplication app("application/x-vnd.Be-prefs-translations");
	the_gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	light_gray = the_gray;
	(void)BTranslatorRoster::Default();
	(new TranslationWindow)->Show();
	app.Run();
	return 0;
}
