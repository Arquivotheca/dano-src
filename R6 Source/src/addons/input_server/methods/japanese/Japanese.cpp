#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include "Japanese.h"
#include "JapaneseCommon.h"
#include "KanaKan.h"
#include "ModePalette.h"
#include "KouhoWindow.h"

#include <FindDirectory.h>
#include <Input.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <Roster.h>
#include <Debug.h>

#include <ctype.h>

inline int32
UTF8CharLen(uchar c)
	{ return (((0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1); }


// method name/icon data
const char	*kJapaneseName	= "Japanese";
const uchar	kJapaneseIcon[]	= {
	0xff,0xff,0xff,0x2a,0x2a,0x2a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
	0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x00,0x17,0x17,0x00,0x00,0xff,0xff,0xff,0xff,
	0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x00,0x17,0x0b,0x17,0x17,0x18,0x00,0x00,0xff,0xff,
	0x2a,0x2a,0x2a,0x2a,0x2a,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x00,0x00,
	0x2a,0x2a,0x2a,0x2a,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0a,0x0b,0x00,
	0xff,0x2a,0x2a,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0a,0x0a,0x00,0x0f,
	0xff,0x2a,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0b,0x0a,0x00,0x0f,0x0f,
	0xff,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0b,0x0a,0x00,0x0f,0x0f,0xff,
	0x00,0x17,0x17,0x17,0x0a,0x18,0x0b,0x17,0x17,0x0a,0x0b,0x00,0x0f,0x0f,0xff,0xff,
	0x00,0x11,0x11,0x17,0x17,0x0b,0x17,0x17,0x0a,0x0a,0x00,0x0f,0x0f,0xff,0xff,0xff,
	0x00,0x11,0x11,0x11,0x11,0x17,0x17,0x0b,0x0a,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,
	0xff,0x00,0x00,0x11,0x11,0x11,0x0b,0x0b,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0x00,0x11,0x0b,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

const uint32 msg_MethodActivated		= 'Jmac';
const uint32 msg_ShowPalette			= 'Jspl';
const uint32 msg_HidePalette			= 'Jhpl';

const uint32 kInputModeMenuOffset = 2;


static char **build_argv(char *str, int	*argc);


// globals
KGrammarDic gGrammar;


// static member variables
port_id		JapaneseInputMethod::sDropBox = B_ERROR;
BPath		JapaneseInputMethod::sUserDicPath;
KDicCtrl	JapaneseInputMethod::sDicCtrl;

bool		JapaneseLooper::sModePaletteShown = J_DEFAULT_PALETTE_WINDOW;


BInputServerMethod*
instantiate_input_method()
{
	return (new JapaneseInputMethod());
}


JapaneseInputMethod::JapaneseInputMethod()
	: BInputServerMethod(kJapaneseName, kJapaneseIcon)
{
	fInputMode = HIRA_INPUT;

	ReadSettings();
}


JapaneseInputMethod::~JapaneseInputMethod()
{	
	delete_port(sDropBox);
	sDropBox = B_ERROR;

	BLooper *target = NULL;
	fJapaneseLooper.Target(&target);

	if (target != NULL)			
		target->Quit();

	status_t err = B_NO_ERROR;

	err = sDicCtrl.UnRegist("yomi.dic");
	if (err != B_NO_ERROR)
		printf("japanese method: unregister yomi.dic err(%Ld)\n", err);
	
	err = sDicCtrl.UnRegist("user.dic");
	if (err != B_NO_ERROR) 
		printf("japanese method: unregister user.dic err(%Ld)\n", err);

	WriteSettings();
}


status_t
JapaneseInputMethod::InitCheck()
{
	status_t err = B_NO_ERROR;

	err = gGrammar.Init(NULL);
	if (err != B_NO_ERROR) {
		printf("japanese method: grammar init err(%Ld)\n", err);	
		return (err);
	}

	BPath yomiPath;
	err = find_directory(B_BEOS_ETC_DIRECTORY, &yomiPath);
	if (err != B_NO_ERROR) {
		printf("japanese method: find_directory(/etc) error\n");
		return (err);
	}
	
	yomiPath.Append("yomi.dic");
	err = sDicCtrl.RegistDic(yomiPath.Path());
	if (err != B_NO_ERROR) {
		printf("japanese method: register yomi.dic err(%Ld)\n", err);
		return (err);
	}

	err = find_directory(B_USER_SETTINGS_DIRECTORY, &sUserDicPath);
	if (err != B_NO_ERROR) {
		printf("japanese method: find_directory(~/config/settings) error\n");
		return (err);
	}

	sUserDicPath.Append("user.dic");
	err = sDicCtrl.RegistDic(sUserDicPath.Path());
	if (err != B_NO_ERROR) {
		printf("japanese method: register user.dic err(%Ld)\n", err);
		return (err);
	}

	sDropBox = create_port(1, J_DROP_BOX_NAME);
	if (sDropBox < 0)
		return (sDropBox);

	fJapaneseLooper = BMessenger(NULL, new JapaneseLooper(this));

	return (B_NO_ERROR);
}


status_t
JapaneseInputMethod::MethodActivated(
	bool	active)
{
	BMessage message(msg_MethodActivated);

	if (active)
		message.AddBool("active", true);

	fJapaneseLooper.SendMessage(&message);

	return (B_NO_ERROR);
}


filter_result
JapaneseInputMethod::Filter(
	BMessage	*message,
	BList		*outList)
{
	if ((message->what != B_KEY_DOWN) || (fInputMode == DIRECT_INPUT))
		return (B_DISPATCH_MESSAGE);

	fJapaneseLooper.SendMessage(message);

	return (B_SKIP_MESSAGE);
}


void
JapaneseInputMethod::SetInputMode(
	uint32	mode)
{
	fInputMode = mode;
}


uint32
JapaneseInputMethod::InputMode() const
{
	return (fInputMode);
}


void
JapaneseInputMethod::ReadSettings()
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
			JapaneseLooper::sModePaletteShown = strtol(argv[1], NULL, 0) != 0;
		else if (strcmp(argv[0], J_SETTINGS_PALETTE_WINDOW_LOC) == 0) {
			if (argc < 3)
				continue;

			ModePalette::sLocation = BPoint(strtol(argv[1], NULL, 0), strtol(argv[2], NULL, 0));
		}
		else if (strcmp(argv[0], J_SETTINGS_KUTOUTEN_TYPE) == 0) {
			uint32 punctMode = strtoul(argv[1], NULL, 0);
			punctMode = (punctMode < 0) ? 0 : punctMode;
			punctMode = (punctMode > 3) ? 3 : punctMode;

			KanaString::sKutoutenMode = punctMode;			 
		}
		else if (strcmp(argv[0], J_SETTINGS_SPACE_TYPE) == 0) {
			KanaString::sFullSpaceMode = strtol(argv[1], NULL, 0) != 0;
		}
		else if (strcmp(argv[0], J_SETTINGS_THRESHOLD_VALUE) == 0) {
			int32 threshold = strtol(argv[1], NULL, 0);
			threshold = (threshold < 1) ? 1 : threshold;

			HenkanManager::sHenkanWindowThreshold = threshold;
		}

		free(argv);
	}

	fclose(settingsFile);
}


void
JapaneseInputMethod::WriteSettings()
{
	BPath settingsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath) != B_NO_ERROR)
		return;
	
	settingsPath.Append("japanese_settings");

	FILE *settingsFile = fopen(settingsPath.Path(), "w+");
	if (settingsFile == NULL)
		return;

	fprintf(settingsFile, J_SETTINGS_PALETTE_WINDOW" %ld\n", (JapaneseLooper::sModePaletteShown) ? 1 : 0); 
	fprintf(settingsFile, J_SETTINGS_PALETTE_WINDOW_LOC" %ld %ld\n", (int32)ModePalette::sLocation.x, (int32)ModePalette::sLocation.y); 
	fprintf(settingsFile, J_SETTINGS_KUTOUTEN_TYPE" %ld\n", KanaString::sKutoutenMode); 
	fprintf(settingsFile, J_SETTINGS_SPACE_TYPE" %ld\n", (KanaString::sFullSpaceMode) ? 1 : 0); 
	fprintf(settingsFile, J_SETTINGS_THRESHOLD_VALUE" %ld\n", HenkanManager::sHenkanWindowThreshold); 

	fclose(settingsFile);
}


JapaneseLooper::JapaneseLooper(
	JapaneseInputMethod	*method)
		: BLooper("Japanese Method Looper")
{
	fOwner = method;

	fSelf = BMessenger(NULL, this);

	fMenu = new BMenu(B_EMPTY_STRING);
	fMenu->SetFont(be_plain_font);
	if (sModePaletteShown)
		fMenu->AddItem(new BMenuItem("入力パレットを隠す", new BMessage(msg_HidePalette)));
	else
		fMenu->AddItem(new BMenuItem("入力パレットを表示", new BMessage(msg_ShowPalette)));
	fMenu->AddSeparatorItem();
	fMenu->AddItem(new BMenuItem("ひらがな (ローマ字入力)", new BMessage(msg_HiraganaInput)));
	fMenu->AddItem(new BMenuItem("全角カタカナ (ローマ字入力)", new BMessage(msg_ZenkakuKatakanaInput)));
	fMenu->AddItem(new BMenuItem("全角英数", new BMessage(msg_ZenkakuEisuuInput)));
	fMenu->AddItem(new BMenuItem("半角カタカナ", new BMessage(msg_HankakuKatakanaInput)));
	fMenu->AddItem(new BMenuItem("半角英数", new BMessage(msg_HankakuEisuuInput)));
	fMenu->AddItem(new BMenuItem("直接入力", new BMessage(msg_DirectInput)));
	fMenu->AddItem(new BMenuItem("ひらがな (ヵな入力)", new BMessage(msg_DirectHiraInput)));
	fMenu->AddItem(new BMenuItem("全角カタカナ (ヵな入力)", new BMessage(msg_DirectKataInput)));
	fMenu->ItemAt(fOwner->InputMode() + kInputModeMenuOffset)->SetMarked(true);

	fModePalette = NULL;
	fModeMessenger = BMessenger(); // invalid messenger
	fInlineHenkanManager = NULL;
	
	ReplenishDropBox();
	
	Run();
}


JapaneseLooper::~JapaneseLooper()
{
	if (fModeMessenger.IsValid()) {
		fModeMessenger.SendMessage(B_CLOSE_REQUESTED);
	}

	fOwner->SetMenu(NULL, BMessenger());

	delete (fMenu);
	delete (fInlineHenkanManager);
}


void
JapaneseLooper::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case B_KEY_DOWN:
			HandleKeyDown(message);
			break;
		
		case J_FAKED_KEYPRESS:
			DetachCurrentMessage();
			message->what=B_KEY_DOWN;
			EnqueueMessage(message);
			break;

		case J_GET_INPUT_MODE:
			{
				BMessage reply;
				reply.AddInt32("inputmode",fOwner->InputMode());
				message->SendReply(&reply);
			}
			break;

		case J_SET_KOUHO_LIST_SELECTION:
			if(fInlineHenkanManager && fInlineHenkanManager->fKouhoView)
			{
				int32 index;
				if(B_OK==message->FindInt32("index",&index))
				{
					//_sPrintf("set selection\n");
					fInlineHenkanManager->fKouhoView->Select(index);
//					fInlineHenkanManager->fKouhoView->SelectKouho(index,true);
				}
			}
			break;

		case B_INPUT_METHOD_EVENT:
		{
			uint32 opcode = 0;
			message->FindInt32("be:opcode", (int32 *)&opcode);

			switch (opcode) {
				case B_INPUT_METHOD_STOPPED:
					delete (fInlineHenkanManager);
					fInlineHenkanManager = NULL;
					break;

				case B_INPUT_METHOD_LOCATION_REQUEST:
					if (fInlineHenkanManager != NULL)
						fInlineHenkanManager->HandleLocationReply(message);
					break;

				default:
					break;
			}
			break;
		}

		case msg_MethodActivated:
			HandleMethodActivated(message->HasBool("active"));
			break;

		case msg_ShowPalette:
			HandleShowHidePalette(true);
			break;

		case msg_HidePalette:
			HandleShowHidePalette(false);
			break;

		case msg_HiraganaInput:
			HandleSetInputMode(HIRA_INPUT, message);
			break;

		case msg_ZenkakuKatakanaInput:
			HandleSetInputMode(ZEN_KATA_INPUT, message);
			break;

		case msg_ZenkakuEisuuInput:
			HandleSetInputMode(ZEN_EISUU_INPUT, message);
			break;

		case msg_HankakuKatakanaInput:
			HandleSetInputMode(HAN_KATA_INPUT, message);
			break;

		case msg_HankakuEisuuInput:
			HandleSetInputMode(HAN_EISUU_INPUT, message);
			break;

		case msg_DirectInput:
			HandleSetInputMode(DIRECT_INPUT, message);
			break;

		case msg_DirectHiraInput:
			HandleSetInputMode(DIRECT_HIRA_INPUT, message);
			break;

		case msg_DirectKataInput:
			HandleSetInputMode(DIRECT_KATA_INPUT, message);
			break;

		case msg_LaunchJPrefs:
			//be_roster->Launch(J_PREFS_SIG);
			// Queries don't work on CFS, so we have to use a hardcoded location
			{
				entry_ref prefref;
				get_ref_for_path("/boot/beos/preferences/Japanese",&prefref);
				be_roster->Launch(&prefref);
			}
			break;

		case J_GRABBED_DROP_BOX:
			ReplenishDropBox();
			break;
		case J_SET_MODE_MESSENGER:
			message->FindMessenger(J_MODE_MESSENGER_NAME, &fModeMessenger);
			if (fModeMessenger.IsValid()) {
				// there is a possible race between creating the ModeButton
				// in the browser plugin and setting the input mode, so
				// update the ModeButton's input mode here.
				BMenuItem *item = fMenu->FindMarked();
				if (item != NULL && (item->Message() != NULL)) {
					fModeMessenger.SendMessage(item->Message());
				}
			}
			break;
		case J_ADD_TO_DICTIONARY:
			HandleAddToDictionary(message);
			break;

		case J_CHANGE_KUTOUTEN_TYPE:
			KanaString::sKutoutenMode = message->FindInt32(J_KUTOUTEN);
			break;

		case J_CHANGE_SPACE_TYPE:
			KanaString::sFullSpaceMode = message->FindInt32(J_SPACE);
			break;

		case J_CHANGE_HENKAN_WINDOW_THRESHOLD:
			HenkanManager::sHenkanWindowThreshold = message->FindInt32(J_THRESHOLD);
			break;

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
JapaneseLooper::ResetInlineHenkanManager()
{
	fInlineHenkanManager = NULL;
}


void
JapaneseLooper::EnqueueMessage(
	BMessage	*message)
{
	fOwner->EnqueueMessage(message);
}


void
JapaneseLooper::HandleMethodActivated(
	bool	active)
{
    BMessenger msngr("application/x-vnd.Web");
	if (active) {
    	if (msngr.IsValid()) {
			// if the pop-up palette is currently showing and the browser running, hide it
			if (fModePalette != NULL) {
				fModePalette->PostMessage(B_CLOSE_REQUESTED);
				fModePalette = NULL;
				fModeMessenger = BMessenger();
			}

			// show the browser-based JIM, 5 second timeout in case browser is locked or something
			BMessage showjapanese('shwj');
			BMessage reply;
			msngr.SendMessage(&showjapanese,&reply,5000000LL);

		} else {
			// show the modepalette only if Wagner is not running
			HandleShowHidePalette(active);
		}
		
		fOwner->SetMenu(fMenu, fSelf);
	}
	else {
		// hide the browser-based JIM
		if(msngr.IsValid())
		{
			BMessage hidejapanese('hidj');
			BMessage reply;
			msngr.SendMessage(&hidejapanese,&reply,5000000LL);
		}
		if (fModePalette != NULL) {
			fModePalette->PostMessage(B_CLOSE_REQUESTED);
			fModePalette = NULL;
			fModeMessenger = BMessenger();
		}

		fOwner->SetMenu(NULL, fSelf);
	
		if (fInlineHenkanManager != NULL) {
			delete (fInlineHenkanManager);
			fInlineHenkanManager = NULL;
		}
	}
}


void
JapaneseLooper::HandleKeyDown(
	BMessage	*message)
{
	uint32 modifiers = message->FindInt32("modifiers");

	// return if anything but the shift-key is down
	if ( ((modifiers & B_COMMAND_KEY) != 0) || 
		 ((modifiers & B_CONTROL_KEY) != 0) || 
		 ((modifiers & B_OPTION_KEY) != 0) ) {
		EnqueueMessage(DetachCurrentMessage());
		return;
	}

	uchar theChar = 0;
	message->FindInt8("byte", (int8 *)&theChar);

	// look for the extra keys on Japanese keyboards
	int32 theKey = 0;
	message->FindInt32("key", &theKey);
	
	switch (theKey) {
		case 0x11:
			switch (fOwner->InputMode()) {
				case ZEN_EISUU_INPUT:
					PostMessage(msg_HankakuEisuuInput);
					break;

				case HAN_EISUU_INPUT:
					PostMessage(msg_ZenkakuEisuuInput);
					break;

				case ZEN_KATA_INPUT:
					PostMessage(msg_HankakuKatakanaInput);
					break;

				case HAN_KATA_INPUT:
					PostMessage(msg_ZenkakuKatakanaInput);
					break;			

				default:
					break;
			}
			return; // return!!!

		case 0x6c:	// key to left of space bar
			theChar = B_ESCAPE;
			break;

		case 0x6d:	// ZENKOUHO key, first key to right of space bar
			theChar = B_SPACE;
			break;

		case 0x6e:	// KATAKANA key, second key to right of space bar
			if (modifiers & B_COMMAND_KEY) {
				// toggle between romaji and direct kana input
				switch (fOwner->InputMode()) {
				case HIRA_INPUT:
					PostMessage(msg_DirectHiraInput);
					break;
				case ZEN_KATA_INPUT:
				case HAN_KATA_INPUT:
					PostMessage(msg_DirectKataInput);					
					break;
				case DIRECT_HIRA_INPUT:
					PostMessage(msg_HiraganaInput);
					break;
				case DIRECT_KATA_INPUT:
					PostMessage(msg_ZenkakuEisuuInput);
					break;
				case ZEN_EISUU_INPUT:
				case HAN_EISUU_INPUT:
				case DIRECT_INPUT:
				default:
					PostMessage(msg_DirectHiraInput);
					break;
				}				
			} else if (modifiers & B_SHIFT_KEY) {
				// shift-KATAKANA should switch into Katakana input mode, either
				// normal romaji-conversion or direct mode, depending on the current mode
				switch (fOwner->InputMode()) {
				case DIRECT_HIRA_INPUT:
					PostMessage(msg_DirectKataInput);
					break;
				case DIRECT_KATA_INPUT:
					// do nothing
					break;
				default:
					PostMessage(msg_ZenkakuKatakanaInput);
					break;
				}
			} else {
				// KATAKANA without shift modifier should switch into Hiragana input mode, either
				// normal romaji-conversion or direct mode, depending on the current mode
				switch (fOwner->InputMode()) {
				case DIRECT_HIRA_INPUT:
					// do nothing
					break;
				case DIRECT_KATA_INPUT:
					PostMessage(msg_DirectHiraInput);
					break;
				default:
					PostMessage(msg_HiraganaInput);
					break;
				}
			}
			return;

		default:
			break;
	}

	// return if any of these non-printables are down
	switch (theChar) {
		case B_INSERT:
		case B_HOME:
		case B_END:
		case B_PAGE_UP:
		case B_PAGE_DOWN:
		case B_FUNCTION_KEY:
			EnqueueMessage(DetachCurrentMessage());
			return;
		
		default:
			break;			
	}

	if (fInlineHenkanManager == NULL) {
		// don't start an input area if any of these keys are down
		switch (theChar) {
			case B_ESCAPE:
			case B_BACKSPACE:
			case B_DELETE:
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW:
			case B_UP_ARROW:
			case B_DOWN_ARROW:
			case B_ENTER:
			case B_TAB:
				EnqueueMessage(DetachCurrentMessage());
				return;
	
			default:
				// return if a half-width space is down
				if ((theChar == B_SPACE) && (!KanaString::sFullSpaceMode)) {
					EnqueueMessage(DetachCurrentMessage());
					return;
				}

				fInlineHenkanManager = new InlineHenkanManager(this, fOwner->InputMode());
				break;
		}
	}

	char bytes[5];
	uint32 bytesLen = 0;
	while (bytesLen < (sizeof(bytes) - 1)) {
		if (message->FindInt8("byte", bytesLen, (int8 *)(bytes + bytesLen)) != B_OK) {
			break;
		}
		bytesLen++;		
	}
	bytes[bytesLen] = '\0';
	
	fInlineHenkanManager->Append(bytes, bytesLen, modifiers);
}


void
JapaneseLooper::HandleShowHidePalette(bool show)
{
	// never show the ModePalette window when the browser is running
	BMessenger browserMessenger("application/x-vnd.Web");
	bool createModePalette = !browserMessenger.IsValid();

	sModePaletteShown = show;

	if ((sModePaletteShown) && !fModeMessenger.IsValid()) {
		if (createModePalette) {
			fModePalette = new ModePalette(this, fOwner->InputMode());
			fModeMessenger = BMessenger(fModePalette);
		}
	} else if ((!sModePaletteShown) && fModeMessenger.IsValid()) {
		fModeMessenger.SendMessage(B_CLOSE_REQUESTED);
		fModePalette = NULL;
		fModeMessenger = BMessenger();
	}
	
	BMenuItem	*item = fMenu->ItemAt(0);
	const char	*label = NULL;
	BMessage	*message = NULL;

	if (sModePaletteShown) {
		label = "入力パレットを隠す";
		message = new BMessage(msg_HidePalette);
	}
	else {
		label = "入力パレットを表示";
		message = new BMessage(msg_ShowPalette);
	}

	item->SetLabel(label);
	item->SetMessage(message);

	fOwner->SetMenu(fMenu, fSelf);
}


void
JapaneseLooper::HandleSetInputMode(
	uint32		mode,
	BMessage	*message)
{
	if (fOwner->InputMode() == mode)
		return;

	fOwner->SetInputMode(mode);
	
	if (fInlineHenkanManager != NULL) {
		if (mode != DIRECT_INPUT)
			fInlineHenkanManager->SetInputMode(mode);
		else {
			delete (fInlineHenkanManager);
			fInlineHenkanManager = NULL;
		}
	}

	// keep the mode palette synchronized with the input mode
	if (fModeMessenger.IsValid()) {
		fModeMessenger.SendMessage(message);
	}

	fMenu->FindMarked()->SetMarked(false);
	fMenu->ItemAt(mode + kInputModeMenuOffset)->SetMarked(true);	

	fOwner->SetMenu(fMenu, fSelf);
}


void
JapaneseLooper::HandleAddToDictionary(
	BMessage	*message)
{
	const char	*yomi = message->FindString(J_YOMI);
	const char	*hyoki = message->FindString(J_HYOKI);
	uint32		hinshi = message->FindInt32(J_HINSHI);

	KDicRec rec;
	rec.SetData((const uchar *)yomi, (const uchar *)hyoki, (KHinshiCode)hinshi);
	JapaneseInputMethod::sDicCtrl.Append(JapaneseInputMethod::sUserDicPath.Path(), &rec);
}


void
JapaneseLooper::ReplenishDropBox()
{
	BMessage methodAddress;
	methodAddress.AddMessenger(J_MESSENGER, fSelf);

	ssize_t	size = methodAddress.FlattenedSize();
	char	*buf = (char *)malloc(size);
	methodAddress.Flatten(buf, size);

	write_port(JapaneseInputMethod::sDropBox, 0, buf, size);

	free(buf);
}


InlineHenkanManager::InlineHenkanManager(
	JapaneseLooper	*owner,
	uint32			mode)
	: HenkanManager()
{
	fOwner = owner;
	fKanaString.SetMode(mode);
}


InlineHenkanManager::~InlineHenkanManager()
{
	if ((fKanaKan != NULL) || (fKanaString.Length() > 0))
		Kakutei();

	fOwner->ResetInlineHenkanManager();
}


void
InlineHenkanManager::OpenInput()
{
	BMessage *startInput = new BMessage(B_INPUT_METHOD_EVENT);
	startInput->AddInt32("be:opcode", B_INPUT_METHOD_STARTED);
	startInput->AddMessenger("be:reply_to", BMessenger(fOwner));

	fOwner->EnqueueMessage(startInput);	
}


void
InlineHenkanManager::CloseInput()
{
	delete (this);
}


bool
InlineHenkanManager::ClauseLocation(
	BPoint	*where,
	float	*height)
{
	BMessage *locRequest = new BMessage(B_INPUT_METHOD_EVENT);
	locRequest->AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);

	fOwner->EnqueueMessage(locRequest);

	return (false);
}


void
InlineHenkanManager::Kakutei()
{
	fOwner->EnqueueMessage(GenerateInputMethodEvent(true));

	BMessage *stopInput = new BMessage(B_INPUT_METHOD_EVENT);
	stopInput->AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);

	fOwner->EnqueueMessage(stopInput);
}


void
InlineHenkanManager::Update()
{
	fOwner->EnqueueMessage(GenerateInputMethodEvent(false));

	if ((fKanaKan == NULL) && (fKanaString.Length() < 1)) {
		BMessage *stopInput = new BMessage(B_INPUT_METHOD_EVENT);
		stopInput->AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);

		fOwner->EnqueueMessage(stopInput);
	}
}


void
InlineHenkanManager::HandleLocationReply(
	BMessage	*message)
{
	BPoint	where = B_ORIGIN;
	float	height = 0.0;
	int32	activeClause = fKanaKan->ActiveClause();			
	int32	charNum = 0;

	for (int32 i = 0; i < activeClause; i++) {
		const char	*kouho = fKanaKan->SelectedKouhoAt(i);
		int32		kouhoLen = fKanaKan->SelectedKouhoLengthAt(i);

		for (int32 j = 0; j < kouhoLen; j += UTF8CharLen(kouho[j]))
			charNum++;
	}

	message->FindPoint("be:location_reply", charNum, &where);
	message->FindFloat("be:height_reply", charNum, &height);

	StartKouhoWindow(&where, height);
}


void
InlineHenkanManager::SetInputMode(
	uint32	mode)
{
	fKanaString.SetMode(mode);
}


BMessage*
InlineHenkanManager::GenerateInputMethodEvent(
	bool	confirmed)
{
	BMessage *imEvent = new BMessage(B_INPUT_METHOD_EVENT);
	imEvent->AddInt32("be:opcode", B_INPUT_METHOD_CHANGED);

	if (fKanaKan == NULL) {
		int32 kanaLen = fKanaString.Length();
		int32 insertionPoint = fKanaString.InsertionPoint();

		imEvent->AddString("be:string", fKanaString.String(false));
		imEvent->AddInt32("be:clause_start", 0);
		imEvent->AddInt32("be:clause_end", kanaLen);
		imEvent->AddInt32("be:selection", insertionPoint);
		imEvent->AddInt32("be:selection", insertionPoint);
	}
	else {
		char	*string = fKanaKan->CompositeString();
		int32	stringLen = strlen(string);
		imEvent->AddString("be:string", string);
		free(string);

		int32 numClauses = fKanaKan->CountClauses();
		int32 activeClause = fKanaKan->ActiveClause();			
		int32 offset = 0;

		for (int32 i = 0; i < numClauses; i++) {
			int32 clauseLen = fKanaKan->SelectedKouhoLengthAt(i);

			imEvent->AddInt32("be:clause_start", offset);
			imEvent->AddInt32("be:clause_end", offset + clauseLen);

			if ((i == activeClause) && (!confirmed)) {
				imEvent->AddInt32("be:selection", offset);
				imEvent->AddInt32("be:selection", offset + clauseLen);				
			}

			offset += clauseLen;
		}

		if (confirmed) {
			imEvent->AddInt32("be:selection", stringLen);
			imEvent->AddInt32("be:selection", stringLen);				
		}
	}

	if (confirmed)
		imEvent->AddBool("be:confirmed", true);

	return (imEvent);
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

