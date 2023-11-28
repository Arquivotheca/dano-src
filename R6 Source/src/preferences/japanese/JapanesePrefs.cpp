#include "JapanesePrefs.h"
#include "JapaneseCommon.h"

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <FindDirectory.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TabView.h>
#include <TextControl.h>

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


inline float
max(float a, float b)
{ return ((a > b) ? a : b); }


const char *kHinshiStrings[] = {
	"普通名詞",
	"固有名詞",
	"地名",
	"人名",
	"接頭語",
	"接尾語",
	"冠数詞",
	"助数詞",
	"連体詞",
	"副詞",
	"接続詞",
	"感動詞",
	"カ行五段",
	"ガ行五段",
	"サ行五段",
	"タ行五段",
	"ナ行五段",
	"バ行五段",
	"マ行五段",
	"ラ行五段",
	"ワ行五段",
	"一段／語幹",
	"サ変動詞",
	"サ変名詞",
	"ザ変動詞",
	"ザ変名詞",
	"形容詞",
	"形容動詞",
	"形容動詞名詞",
	"単漢字"
};
const int32 kNumHinshiStrings = sizeof(kHinshiStrings) / sizeof(kHinshiStrings[0]);

const uint32 msg_ChangeKutouten		= 'CKut';
const uint32 msg_ChangeSpace		= 'CSpc';
const uint32 msg_ChangeThreshold	= 'CThr';
const uint32 msg_AddToDict 			= 'AddD';


static char **build_argv(char *str, int	*argc);


bool		JapanesePrefsApp::sPaletteWindow = J_DEFAULT_PALETTE_WINDOW;
BPoint		JapanesePrefsApp::sPaletteWindowLoc(J_DEFAULT_PALETTE_WINDOW_LOC, J_DEFAULT_PALETTE_WINDOW_LOC);
uint32		JapanesePrefsApp::sKutoutenType = J_DEFAULT_KUTOUTEN_TYPE;
bool		JapanesePrefsApp::sFullWidthSpace = J_DEFAULT_SPACE_TYPE;
int32		JapanesePrefsApp::sWindowThreshold = J_DEFAULT_HENKAN_WINDOW_THRESHOLD;
BMessenger	JapanesePrefsApp::sMethod;


main()
{
	JapanesePrefsApp *app = new JapanesePrefsApp();
	app->Run();	
	delete (app);

	return (B_NO_ERROR);
}


JapanesePrefsApp::JapanesePrefsApp()
	: BApplication(J_PREFS_SIG)
{
	ReadSettings();
}


JapanesePrefsApp::~JapanesePrefsApp()
{
	WriteSettings();
}


void
JapanesePrefsApp::ReadyToRun()
{
	BMessage 	methodAddress;
	ssize_t		size = 0;
	char		*buf = NULL;
	int32		code = 0;
	status_t	err = B_ERROR;

	port_id dropBox = find_port(J_DROP_BOX_NAME);
	if (dropBox < 0)
		goto Error;

	size = port_buffer_size(dropBox);
	buf = (char *)malloc(size);
	code = 0;

	read_port(dropBox, &code, buf, size);
	err = methodAddress.Unflatten(buf);

	free(buf);
	
	if (err != B_NO_ERROR)
		goto Error;

	if (methodAddress.FindMessenger(J_MESSENGER, &sMethod) != B_NO_ERROR)
		goto Error;

	sMethod.SendMessage(J_GRABBED_DROP_BOX);
	new JapanesePrefsWindow();

	return;

Error:
	(new BAlert("Error", "Couldn't find Japanese Input Method.", "OK"))->Go();
	Quit();
}


void
JapanesePrefsApp::ReadSettings()
{
	BPath settingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath) != B_NO_ERROR)
		return;
	
	settingsPath.Append("japanese_settings");

	FILE *settingsFile = fopen(settingsPath.Path(), "r");
	if (settingsFile == NULL)
		return;

	int		argc = 0;
	char	**argv = NULL;
	char	buf[512] = "";

	while (fgets(buf, sizeof(buf), settingsFile) != NULL) {
		if (buf[0] == '#' || buf[0] == '\n' || buf[0] == '\r')
			continue;

		int32 i = 0;
		for (i = 0; isspace(buf[i]) && buf[i]; i++)
			;

		if (buf[i] == '\0')
			continue;

		argc = 0;
		argv = build_argv(buf, &argc);
	
		if (argv == NULL)
			continue;

		if (argc < 2)
			continue;

		if (strcmp(argv[0], J_SETTINGS_PALETTE_WINDOW) == 0)
			sPaletteWindow = strtol(argv[1], NULL, 0) != 0;
		else if (strcmp(argv[0], J_SETTINGS_PALETTE_WINDOW_LOC) == 0) {
			if (argc < 3)
				continue;

			sPaletteWindowLoc = BPoint(strtol(argv[1], NULL, 0), strtol(argv[2], NULL, 0));
		}
		else if (strcmp(argv[0], J_SETTINGS_KUTOUTEN_TYPE) == 0) {
			uint32 punctMode = strtoul(argv[1], NULL, 0);
			punctMode = (punctMode < 0) ? 0 : punctMode;
			punctMode = (punctMode > 3) ? 3 : punctMode;

			sKutoutenType = punctMode;			 
		}
		else if (strcmp(argv[0], J_SETTINGS_SPACE_TYPE) == 0) {
			sFullWidthSpace = strtol(argv[1], NULL, 0) != 0;
		}
		else if (strcmp(argv[0], J_SETTINGS_THRESHOLD_VALUE) == 0) {
			int32 threshold = strtol(argv[1], NULL, 0);
			threshold = (threshold < 1) ? 1 : threshold;

			sWindowThreshold = threshold;
		}

		free(argv);
	}

	fclose(settingsFile);
}


void
JapanesePrefsApp::WriteSettings()
{
	BPath settingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath) != B_NO_ERROR)
		return;
	
	settingsPath.Append("japanese_settings");

	FILE *settingsFile = fopen(settingsPath.Path(), "w+");
	if (settingsFile == NULL)
		return;

	fprintf(settingsFile, J_SETTINGS_PALETTE_WINDOW" %ld\n", (sPaletteWindow) ? 1 : 0); 
	fprintf(settingsFile, J_SETTINGS_PALETTE_WINDOW_LOC" %ld %ld\n", (int32)sPaletteWindowLoc.x, (int32)sPaletteWindowLoc.y); 
	fprintf(settingsFile, J_SETTINGS_KUTOUTEN_TYPE" %ld\n", sKutoutenType); 
	fprintf(settingsFile, J_SETTINGS_SPACE_TYPE" %ld\n", (sFullWidthSpace) ? 1 : 0); 
	fprintf(settingsFile, J_SETTINGS_THRESHOLD_VALUE" %ld\n", sWindowThreshold); 

	fclose(settingsFile);
}


JapanesePrefsWindow::JapanesePrefsWindow()
	: BWindow(BRect(100.0, 100.0, 370.0, 280.0), "Japanese", 
			  B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BBox *base = new BBox(Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	AddChild(base);

	BRect tabViewBounds = Bounds();
	tabViewBounds.InsetBy(10.0, 10.0);
	BTabView *tabView = new BTabView(tabViewBounds, B_EMPTY_STRING);

	BRect tabsBound = tabView->Bounds();
	tabsBound.InsetBy(5.0, 5.0);
	tabsBound.right -= 2.0;
	tabsBound.bottom -= tabView->TabHeight() + 2.0;
	tabView->AddTab(new SettingsView(tabsBound));
	tabView->AddTab(new DictionaryView(tabsBound));

	base->AddChild(tabView);

	Show();
}


JapanesePrefsWindow::~JapanesePrefsWindow()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
}


SettingsView::SettingsView(
	BRect	frame)
		: BView(frame, "基本設定", B_FOLLOW_ALL, B_WILL_DRAW)
{
	float		maxDivider = 0.0;
	BPopUpMenu	*menu = NULL;
	BRect		menuFieldBounds = Bounds();
	menuFieldBounds.InsetBy(10.0, 10.0);

	menu = new BPopUpMenu(B_EMPTY_STRING);
	menu->AddItem(new BMenuItem("。 、", new BMessage(msg_ChangeKutouten)));
	menu->AddItem(new BMenuItem("． ，", new BMessage(msg_ChangeKutouten)));
	menu->AddItem(new BMenuItem("。 ，", new BMessage(msg_ChangeKutouten)));
	menu->AddItem(new BMenuItem("． 、", new BMessage(msg_ChangeKutouten)));
	menu->ItemAt(JapanesePrefsApp::sKutoutenType)->SetMarked(true);
	menu->SetLabelFromMarked(true);	

	fKutouten = new BMenuField(menuFieldBounds, B_EMPTY_STRING, "句読点：", menu);
	maxDivider = max(maxDivider, fKutouten->StringWidth(fKutouten->Label()) + 6.0);
	AddChild(fKutouten);

	menuFieldBounds.OffsetTo(menuFieldBounds.left, menuFieldBounds.top + 26.0);

	menu = new BPopUpMenu(B_EMPTY_STRING);
	menu->AddItem(new BMenuItem("半角", new BMessage(msg_ChangeSpace)));
	menu->AddItem(new BMenuItem("全角", new BMessage(msg_ChangeSpace)));
	menu->ItemAt((JapanesePrefsApp::sFullWidthSpace) ? 1 : 0)->SetMarked(true);
	menu->SetLabelFromMarked(true);	

	fSpace = new BMenuField(menuFieldBounds, B_EMPTY_STRING, "スペース：", menu);
	maxDivider = max(maxDivider, fSpace->StringWidth(fSpace->Label()) + 6.0);
	AddChild(fSpace);

	menuFieldBounds.OffsetTo(menuFieldBounds.left, menuFieldBounds.top + 26.0);

	menu = new BPopUpMenu(B_EMPTY_STRING);
	menu->AddItem(new BMenuItem("1回目", new BMessage(msg_ChangeThreshold)));
	menu->AddItem(new BMenuItem("2回目", new BMessage(msg_ChangeThreshold)));
	menu->AddItem(new BMenuItem("3回目", new BMessage(msg_ChangeThreshold)));
	menu->AddItem(new BMenuItem("4回目", new BMessage(msg_ChangeThreshold)));
	menu->AddItem(new BMenuItem("5回目", new BMessage(msg_ChangeThreshold)));
	menu->AddItem(new BMenuItem("6回目", new BMessage(msg_ChangeThreshold)));
	menu->ItemAt(JapanesePrefsApp::sWindowThreshold - 1)->SetMarked(true);
	menu->SetLabelFromMarked(true);	

	fThreshold = new BMenuField(menuFieldBounds, B_EMPTY_STRING, "変換ウィンドウ表示：", menu);
	maxDivider = max(maxDivider, fThreshold->StringWidth(fThreshold->Label()) + 6.0);
	AddChild(fThreshold);

	fKutouten->SetDivider(maxDivider);
	fSpace->SetDivider(maxDivider);
	fThreshold->SetDivider(maxDivider);
}


void
SettingsView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());

	fKutouten->Menu()->SetTargetForItems(this);
	fSpace->Menu()->SetTargetForItems(this);
	fThreshold->Menu()->SetTargetForItems(this);
}


void
SettingsView::MessageReceived(
	BMessage	*message)
{
	JapanesePrefsApp	*app = (JapanesePrefsApp *)be_app;
	BMenuItem			*item = NULL;
	BMessage			command;

	switch (message->what) {
		case msg_ChangeKutouten:
			message->FindPointer("source", (void **)&item);
			JapanesePrefsApp::sKutoutenType = fKutouten->Menu()->IndexOf(item);

			command.what = J_CHANGE_KUTOUTEN_TYPE;
			command.AddInt32(J_KUTOUTEN, JapanesePrefsApp::sKutoutenType);
			app->sMethod.SendMessage(&command);
			break;

		case msg_ChangeSpace:
			message->FindPointer("source", (void **)&item);
			JapanesePrefsApp::sFullWidthSpace = fSpace->Menu()->IndexOf(item) > 0;

			command.what = J_CHANGE_SPACE_TYPE;	
			command.AddInt32(J_SPACE, (JapanesePrefsApp::sFullWidthSpace) ? 1 : 0);
			app->sMethod.SendMessage(&command);								
			break;

		case msg_ChangeThreshold:
			message->FindPointer("source", (void **)&item);
			JapanesePrefsApp::sWindowThreshold = fThreshold->Menu()->IndexOf(item) + 1;

			command.what = J_CHANGE_HENKAN_WINDOW_THRESHOLD;
			command.AddInt32(J_THRESHOLD, JapanesePrefsApp::sWindowThreshold);
			app->sMethod.SendMessage(&command);	
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


DictionaryView::DictionaryView(
	BRect	frame)
		: BView(frame, "辞書", B_FOLLOW_ALL, B_WILL_DRAW)
{
	float	maxDivider = 0.0;
	BRect	controlBounds = Bounds();
	controlBounds.InsetBy(10.0, 10.0);

	fYomi = new BTextControl(controlBounds, B_EMPTY_STRING, "読み：", NULL, NULL);
	maxDivider = max(maxDivider, fYomi->StringWidth(fYomi->Label()) + 6.0);
	AddChild(fYomi);

	controlBounds.OffsetTo(controlBounds.left, controlBounds.top + 26.0);

	fHyoki = new BTextControl(controlBounds, B_EMPTY_STRING, "表記：", NULL, NULL);
	maxDivider = max(maxDivider, fYomi->StringWidth(fYomi->Label()) + 6.0);
	AddChild(fHyoki);

	controlBounds.OffsetTo(controlBounds.left, controlBounds.top + 26.0);

	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING);
	for (int32 i = 0; i < kNumHinshiStrings; i++)
		menu->AddItem(new BMenuItem(kHinshiStrings[i], NULL));
	menu->ItemAt(0)->SetMarked(true);
	menu->SetLabelFromMarked(true);	
	fHinshi = new BMenuField(controlBounds, B_EMPTY_STRING, "品詞：", menu);
	maxDivider = max(maxDivider, fHinshi->StringWidth(fHinshi->Label()) + 6.0);
	AddChild(fHinshi);

	fYomi->SetDivider(maxDivider);
	fHyoki->SetDivider(maxDivider);
	fHinshi->SetDivider(maxDivider);	

	BRect buttonRect(0.0, 0.0, 0.0, 0.0);
	fAddButton = new BButton(buttonRect, B_EMPTY_STRING, "追加", new BMessage(msg_AddToDict));
	fAddButton->ResizeToPreferred();
	fAddButton->MoveTo(Bounds().right - 10.0 - fAddButton->Bounds().Width(),
					   Bounds().bottom - 10.0 - fAddButton->Bounds().Height());
	AddChild(fAddButton);
}


void
DictionaryView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());

	fYomi->MakeFocus(true);
	fAddButton->SetTarget(this);
}


void
DictionaryView::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case msg_AddToDict:
			AddToDictionary();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
DictionaryView::AddToDictionary()
{
	JapanesePrefsApp *app = (JapanesePrefsApp *)be_app;

	const char	*yomiText = fYomi->Text();
	const char	*hyokiText = fHyoki->Text();
	uint32		hinshi = fHinshi->Menu()->IndexOf(fHinshi->Menu()->FindMarked()) + 1;

	if ((yomiText == NULL) || (hyokiText == NULL))
		return;

	if ((strlen(yomiText) < 1) || (strlen(hyokiText) < 1))
		return;

	BMessage message(J_ADD_TO_DICTIONARY);
	message.AddString(J_YOMI, yomiText);
	message.AddString(J_HYOKI, hyokiText);
	message.AddInt32(J_HINSHI, hinshi);

	app->sMethod.SendMessage(&message);
}


static char**
build_argv(
	char	*str, 
	int		*argc)
{
	char	**argv = NULL;
	int32	table_size = 16;

	if (argc == NULL)
		return (NULL);

	*argc = 0;
	argv  = (char **)calloc(table_size, sizeof(char *));

	if (argv == NULL)
		return (NULL);
	
	while (*str) {
		// skip intervening white space
		while(*str != '\0' && (*str == ' ' || *str == '\t' || *str == '\n'))
			str++;
		
		if (*str == '\0')
			break;
		
		if (*str == '"') {
			argv[*argc] = ++str;
			while (*str && *str != '"') {
				if (*str == '\\')
					strcpy(str, str+1);  // copy everything down
				str++;
			}
		} else if (*str == '\'') {
			argv[*argc] = ++str;
			while (*str && *str != '\'') {
				if (*str == '\\')
					strcpy(str, str+1);  // copy everything down
				str++;
			}
		} else {
			argv[*argc] = str;
			while (*str && *str != ' ' && *str != '\t' && *str != '\n') {
				if (*str == '\\')
					strcpy(str, str+1);  // copy everything down
				str++;
			}
		}
		
		if (*str != '\0')
			*str++ = '\0';   // chop the string
		
		*argc += 1;
		if (*argc >= table_size-1) {
			table_size *= 2;
			char **nargv = (char **)calloc(table_size, sizeof(char *));
			
			if (nargv == NULL) {   // drats! failure.
				free(argv);
				return (NULL);
			}
			
			memcpy(nargv, argv, (*argc) * sizeof(char *));
			free(argv);
			argv = nargv;
		}
	}
	
	return (argv);
}
