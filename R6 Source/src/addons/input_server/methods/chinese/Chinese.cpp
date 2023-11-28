/*
 * Chinese.cpp
 *
 *	An input method for both Traditional Chinese and Simplified Chinese.
 *	Based on Chinese input method code obtained from FIC, and on Be's
 *	Japanese input method.
 *
 *	Note:	There is probably too much file access in this code.  A better
 *			balance between speed and memory use should be relatively easy
 *			to implement.  Also, the global variable mess is an artifact of
 *			the FIC code, and should be improved.
 */

#include "Chinese.h"
#include <interface/Input.h>
#include <interface/Screen.h>
#include <support/Autolock.h>
#include <support/Beep.h>
#include <support/Debug.h>
#include <support/Locker.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define SEPCHAR         ' '
#define	MAX_CHCNT	420			//MAX_WORDCNT*3
#define	MAX_WORDCNT	140			//max number of match HZ

#define	MAX_KEYCNT	50			// number of input key
#define	MAX_CODECNT	8			// max input string length in chars
#define MATCH_PAGE_SIZE 9		// number of matches to display per page
#define MATCH_WINDOW_WIDTH 50	// width in pixels of match window
#define MATCH_WINDOW_MIN_FONT_SIZE 14.0f	// the minimum font size for the match window 
#define MATCH_WINDOW_OFFSET 4.0f			// the spacing between the text and the match window
/*
 * head struct of dict file 
 */
struct dict_head {
	char	ename[21];	// English name of inputmethod  
	char	cname[31];	// Chinese name of inputmethod  
	char	selkey[20];	// select key
	char	pagekey[2];	// scroll key
	char	endkey[20];	// end key
	int		keycnt;		// number of root
	struct	{
		char	name;	// root
		char	ch[4];	// HZ of root, UTF8 encoded (3 bytes)
	}	key[MAX_KEYCNT];
	int	codecnt;	//±àÂëÏîÊý
};	// head of dict

#define	HEADSZ	sizeof(struct dict_head )

/*
 * index struct of dict file
 */
struct	dict_index {
	char	i_code[MAX_CODECNT+1];
	short	i_cnt;	// bytes of HZ string
	int	i_offset;
};

char DictPath[120];

#define  MAX_METHOD	5

struct config_stru {
	char    InputName[20];
	char    DictName[10];
} ch_input[MAX_METHOD];     

int     MethodCount;
struct config_head {
        int     LastMethod;
        char    MethodName[20];
} ch_head;    
FILE	*fp;

struct dict_head 	DictHead;
struct dict_index       *dict_idx;
//char 	usrInput[MAX_CODECNT];
//char 	usrInput_hz[MAX_CODECNT*3];

char 	MatchList[MAX_WORDCNT][3];
int  	matchNum;
int8 	matchListNum;
//int8 	currentIndex;
//int8 	currentList;

#define	INDEXSZ	(sizeof(struct dict_index ))
#define ENTOFF	(HEADSZ + INDEXSZ * DictHead.codecnt)	//start position of dict

/*reserved */
const char* yuan = "abcdefghnqrstvwxyz125";
const char* fu = "890-iklop;',./";
const char* fuyuan = "jmu";

FILE	*dictfp;

BLocker gDictLocker;	// global lock to protect the global dictionary variables

const uint32 msg_MethodSelected			= 'Cmsl';
const uint32 msg_MethodActivated		= 'Cmac';
const uint32 msg_MatchSelected			= 'Msel';
const uint32 msg_MatchInvoked			= 'Mivk';
const uint32 msg_SelectMatch			= 'Selm';
const uint32 msg_ShowMatchWindow		= 'Smwn';

//-------------------------------------------------------------------

/*
FILE*	logfp = stdout;

void log(const char *text)
{
	_sPrintf("%s\n", text);
//	fprintf(logfp, "%s\n", text);
//	fflush(logfp);
}

void log(const char *text1, const char *text2)
{
	_sPrintf("%s : %s\n", text1, text2);
//	fprintf(logfp, "%s : %s\n", text1, text2);
//	fflush(logfp);
}

void openlog() {
	logfp = fopen("/boot/home/Desktop/chinese-log.txt", "w+");	
}

void closelog() {
	fclose(logfp);
	logfp = stdout;
}
*/

//-------------------------------------------------------------------

extern "C" {
BInputServerMethod * instantiate_input_method()
{
	return (new ChineseInputMethod("ä¸­æ–‡è¾“å…¥æ³•",NULL));
}
} // extern "C"

//-------------------------------------------------------------------

ChineseInputMethod::ChineseInputMethod(const char *name, const uchar *icon)
	: BInputServerMethod(name, icon)
{
	fLooper = new ChineseLooper(this);
}

ChineseInputMethod::~ChineseInputMethod()
{
	if (fLooper && fLooper->Lock()) {
		fLooper->Quit();
		fLooper = NULL;
	}
}

status_t ChineseInputMethod::MethodActivated(bool active)
{
	BMessage message(msg_MethodActivated);
	message.AddBool("active", active);
	fLooper->PostMessage(&message);
	return B_OK;
}

filter_result ChineseInputMethod::Filter(BMessage *message, BList *outList)
{
	if (message->what != B_KEY_DOWN) {
		return B_DISPATCH_MESSAGE;
	}

	fLooper->PostMessage(message);
	return B_SKIP_MESSAGE;
}

//-------------------------------------------------------------------

ChineseLooper::ChineseLooper(ChineseInputMethod *method)
	: BLooper("ChineseLooper")
{
	fSelf = BMessenger(NULL, this);
	SetState(EMPTY);
	fOwner = method;
	fLabelWin = NULL;
	fMatchWin = NULL;
	fMatchIndex = 0;
	fCurrentPage = 0;

	Run();
}

ChineseLooper::~ChineseLooper()
{

}

void ChineseLooper::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_KEY_DOWN:
			HandleKeyDown(message);
			break;

		case B_INPUT_METHOD_EVENT:
			{
				uint32 opcode = 0;
				message->FindInt32("be:opcode", (int32 *)&opcode);
	
				switch (opcode) {
					case B_INPUT_METHOD_STOPPED:
						// the application killed the input method transaction
						Reset();
						break;
	
					case B_INPUT_METHOD_LOCATION_REQUEST:
						{
							BPoint	where = B_ORIGIN;
							float	height = 0.0;
							message->FindPoint("be:location_reply", 0, &where);
							message->FindFloat("be:height_reply", 0, &height);

							// show the match window
							BMessage showMsg(msg_ShowMatchWindow);
							showMsg.AddPoint("point", where);
							showMsg.AddFloat("height", height);
							if (fMatchWin) {
								fMatchWin->PostMessage(&showMsg);
							}
						}
						break;
	
					default:
						break;
				}
			}
			break;

		case msg_MethodActivated:
			{
				bool active;
				if (message->FindBool("active", &active) == B_OK) {
					HandleMethodActivated(active);
				}
			}
			break;

		case msg_MatchSelected:
			{
				const char *text;
				bool dismiss;
				if (message->FindString("text", &text) == B_OK) {
					switch (fState) {
					//case SHOWING_CONVERTED: // fall through
					case SHOWING_CHOICES:
						fConverted = text;
						EnqueueMessage(GenerateInputMethodEvent(fConverted.String(), true, false));
						break;
						
					case EMPTY:				// fall through	
					case SHOWING_TEXT:		// fall through
					default:
						// do nothing
						break;
					}
				}
				if (message->FindBool("dismiss", &dismiss) == B_OK) {
					if (dismiss) {
						fMatchWin->PostMessage(B_QUIT_REQUESTED);
					}
				}
			}
			break;
			
		case msg_MethodSelected:
			{
				int32 methodIndex;
				if (message->FindInt32("index", &methodIndex) == B_OK) {
					if ((ch_head.LastMethod != methodIndex) && gDictLocker.Lock()) {
						StopInput(true, false);
						Reset();
				  		//usrInput[0]='\0';
				  		//usrInput_hz[0]='\0';
						//currentIndex = 0;
						//currentList = 0;
						CloseDict();
						if (OpenDict(ch_input[methodIndex].DictName) != 0){
							// OpenDict failed for methodIndex, so open the method at index 0
							OpenDict(ch_input[0].DictName);
							ch_head.LastMethod = 0;
						}
						ch_head.LastMethod = methodIndex;
						gDictLocker.Unlock();
					}
				}
			}
			break;
		
		default:
			break;
	}
}

void ChineseLooper::EnqueueMessage(BMessage *message)
{
	fOwner->EnqueueMessage(message);
}

int ChineseLooper::Append(int8 theKey, int32 modifiers)
{
//log("Append invoked");
	int r = -1;
	int8 keyIndex;
	theKey = tolower(theKey);
	if (IsInputKey(theKey, &keyIndex) == 0) {
		char buf[8];
		if (gDictLocker.Lock()) {
			strcpy(buf, DictHead.key[keyIndex].ch);
			gDictLocker.Unlock();
		}

		switch (fState) {
		case EMPTY:
			// start new input area
			StartInput();
			SetState(SHOWING_TEXT);
			break;

		case SHOWING_TEXT:
			{		
				int32 count = fEntered.CountChars() + 1;
				if (count % MAX_CODECNT == 0) {
					// adding another character would exceed the limit, so truncate
					// the entry string and start over
					ResetText();
				}
			}
			break;

		//case SHOWING_CONVERTED:
		case SHOWING_CHOICES:	// fall through
			// stop current input area and start new one
			StopInput(false, true);
			StartInput();
			ResetText();
			SetState(SHOWING_TEXT);
			break;		 
		
		default:
			// do nothing
			break;
		}
		
		fEntered << (char)theKey;
		fEnteredHanzi << buf;
		EnqueueMessage(GenerateInputMethodEvent(fEnteredHanzi.String(), false, false));
		r = 0;
	}
	return r;
}

void ChineseLooper::SelectMatch(int8 matchIndex)
{
	BAutolock lock(gDictLocker);
	char buf[8];
//sprintf(buf, "%d", matchIndex);
//log("SelectMatch invoked with index", buf);
	// clamp the value to the valid range
	if (matchIndex < 0) {
		matchIndex = 0;
	} else if (matchIndex >= matchNum) {
		matchIndex = matchNum - 1;
	}
	if (fMatchIndex == matchIndex) {
//log("\tfMatchIndex == matchIndex, returning...");
		return;
	}
	
//log("\tnew index selected");

	fMatchIndex = matchIndex;
	
	// update input method transaction to reflect new match
	if (GetMatch(fMatchIndex, buf) == 0) {
		fConverted = buf;
		EnqueueMessage(GenerateInputMethodEvent(fConverted.String(), true, false));
	}

	int32 startOfPage = fCurrentPage * MATCH_PAGE_SIZE;
	if (fMatchIndex < startOfPage) {
		fCurrentPage--;
	} else if (fMatchIndex >= (startOfPage + MATCH_PAGE_SIZE)) {
		fCurrentPage++;
	}

	// update match window
	if (fMatchWin) {
		BMessage selMessage(msg_SelectMatch);
		selMessage.AddInt32("index", fMatchIndex);
		selMessage.AddInt32("page", fCurrentPage);
		fMatchWin->PostMessage(&selMessage);
	}
}

void ChineseLooper::DoBackspace()
{
//log("DoBackspace invoked", fEntered.String());
	int32 len = fEntered.Length() - 1;
	if (len > 0) {
		fEntered.Truncate(len);
		fEnteredHanzi.Truncate(len * 3); 	// makes the big assumptions that all characters in
											// fEnteredHanzi are 3 bytes long, and that there are
											// as many characters in fEnteredHanzi as in fEntered
		EnqueueMessage(GenerateInputMethodEvent(fEnteredHanzi.String(), false, false));
	} else {
		ResetText();
		StopInput(false, true);
		SetState(EMPTY);
	}
}

bool ChineseLooper::DoConversion()
{
//log("DoConversion invoked", fEntered.String());
	bool r = false;
	char buf[8];
	
	if (gDictLocker.Lock()) {
//log("\t\tcalling MatchDict");
		// convert text and replace it with the conversion
		matchNum = MatchDict(fEntered.String(), (char **)MatchList);
		if (matchNum <= 0) {
//log("\t\tMatchDict returned an error");
			matchListNum = 0;
			// no match for this input string
			beep();
		} else {
//log("\t\tMatchDict returned some matches");
			r = true;
			if (matchNum % MATCH_PAGE_SIZE == 0) {
				matchListNum = matchNum / MATCH_PAGE_SIZE;
			} else {
				matchListNum = matchNum / MATCH_PAGE_SIZE + 1;
			}
			fMatchIndex = 0;
			fCurrentPage = 0;
			if (GetMatch(fMatchIndex, buf) == 0) {
//log("\t\tGetMatch succeeded");
				fConverted = buf;
				EnqueueMessage(GenerateInputMethodEvent(fConverted.String(), true, false));
			}
		}
		gDictLocker.Unlock();
	}
	return r;	
}

void ChineseLooper::RevertConversion()
{
	// make fConverted empty
	fConverted = B_EMPTY_STRING;	
}

BMessage* ChineseLooper::GenerateInputMethodEvent(const char *string, bool selected, bool confirmed)
{
	int len = strlen(string);
	int32 selStart = (selected) ? 0 : len;
	int32 selEnd = len;
	BMessage *imEvent = new BMessage(B_INPUT_METHOD_EVENT);

	imEvent->AddInt32("be:opcode", B_INPUT_METHOD_CHANGED);
	imEvent->AddString("be:string", string);
	imEvent->AddInt32("be:clause_start", 0);
	imEvent->AddInt32("be:clause_end", len);
	imEvent->AddInt32("be:selection", selStart);
	imEvent->AddInt32("be:selection", selEnd);
	if (confirmed) {
		imEvent->AddBool("be:confirmed", true);
	}

	return imEvent;
}

const char* ChineseLooper::CurrentText()
{
	const char *r = B_EMPTY_STRING;
	if (fState == SHOWING_TEXT) {
		r = fEnteredHanzi.String();
	} else if //((fState == SHOWING_CONVERTED) ||
				(fState == SHOWING_CHOICES)//)
	{
		r = fConverted.String();
	}
	
	return r;
}

void ChineseLooper::HandleMethodActivated(bool active)
{
	int i;

	//BRect r1(50,400,350,420);
	//BRect r2(360,260,420,420);
	BRect r3(450,10,520,30);

	if (active) {
		Reset();

		if (gDictLocker.Lock()) {
			//usrInput[0] = '\0';
			//usrInput_hz[0] = '\0';
			//currentIndex = 0;
			//currentList = 0;
			strcpy(DictPath,"/boot/home/config/settings/chinese-input/");
			memset(&ch_head, '\0', sizeof(struct config_head));
			for (i = 0; i < MAX_METHOD; i++) {
				memset(&ch_input[i], '\0', sizeof(struct config_stru));
			}
	
			InitSub();
		
			if (OpenDict(ch_input[i].DictName) != 0){
				OpenDict(ch_input[0].DictName);
				ch_head.LastMethod = 0;
			}
			gDictLocker.Unlock();
		}

		// the label window must be created after the dictionary has been loaded
		fLabelWin = new TLabelWindow(r3, "LabelWin", this);
		fLabelWin->Show();
		fMatchWin = NULL;
	} else {
		StopMatchWindow();
		
		if (fState == SHOWING_TEXT && fEntered.Length() > 1) {
			// we don't want to leave around the text from an input area
			// that has multiple radicals in it, but leaving around the
			// converted text or one radical is OK.
			ResetText();
		}
		StopInput(false, true);
		
		fLabelWin->PostMessage(B_QUIT_REQUESTED);
		fLabelWin = NULL;
		CloseDict();

		fp = fopen("/boot/home/config/settings/chinese-input/.last","w+");
		if (fp != NULL && gDictLocker.Lock()) {
			fprintf(fp, "%d", ch_head.LastMethod);
			fclose(fp);
			gDictLocker.Unlock();
		}
	}
}

// validate user's input.  returns 0 on success, -1 on failure
int ChineseLooper::IsInputKey(int8 keycode, int8 *index)
{
	int i;
	int r = -1;

	if (gDictLocker.Lock()) {
		for (i = 0; i < DictHead.keycnt; i++) {
			if (keycode == DictHead.key[i].name) {
				*index = i;
				r = 0;
				break;
			}
		}
		gDictLocker.Unlock();
	}	
	return r;
}

/*
// use up arrow and down arrow to select HZ in match window
bool ChineseLooper::IsArrowKey(int8 keycode, int32 *select1)
{
	bool r = false;
	int32 select;
	select = *select1;

	if (keycode == B_UP_ARROW && select > 0) { // up arrow
		select--;
		select = select % MATCH_PAGE_SIZE;
		*select1 = select;
		r = true;
	}
	
	if (keycode == B_DOWN_ARROW) { //down arrow
		select++;
		select = select % MATCH_PAGE_SIZE;
		*select1 = select;
		r = true;
	}

	return r;
}

//used to select one HZ
int ChineseLooper::IsSelectKey(int8 keycode)
{
	int keynum = strlen(DictHead.selkey);
	for (int i = 0 ; i < keynum; i++) {
		if (keycode == DictHead.selkey[i]) {
			return 0;
		}
	}
	return -1;
}

//used to scroll the match list 
//pageup:11,pagedown: 12
//-:45
//+:61
int ChineseLooper::IsScrollKey(int8 keycode)
{
	switch(keycode){
	case 12:
	case 61:
		// XXX: need to implement scrolling
		//currentList++;
		//return 0;
	case 11:
	case 45:
		// XXX: need to implement scrolling	
		//if(currentList>0){
		//	currentList--;
		//	return 0;
		//}
		//else
		//	return -1;
	default:
  		return -1;
	}
}
*/


// used in zhuyin input method : 3,4,6,7 are accent keys
// conversion should occur immediately for an "end key"
bool ChineseLooper::IsEndKey(int8 keycode)
{
	BAutolock lock(gDictLocker);
	bool r = false;  
	int8 keynum = strlen(DictHead.endkey);

	for (int8 i = 0; i < keynum; i++) {
		if (keycode == DictHead.endkey[i]) {
			r = true;
		}
	}
	return r;
}

//seek the HZ according to input string
//arguments:
//	dictEng:input string of user
//	dictCh:location of the match HZ
//return values:
//	n:number of match HZ
//	-1:no match HZ
//	-2:read dict file error
int ChineseLooper::MatchDict(const char *dictEng, char **dictCh)
{
	int	i;
	char	cstr[MAX_CHCNT+1];

	for (i = 0; i < DictHead.codecnt; i++) {
		if (strcmp(dict_idx[i].i_code, dictEng) == 0) {
			fseek(dictfp, ENTOFF + dict_idx[i].i_offset, 0);
			if (fread((void *)cstr, dict_idx[i].i_cnt, 1, dictfp) != 1) {
				return(-2);
			}
			cstr[dict_idx[i].i_cnt] = '\0';
			strcpy((char *)&dictCh[0], cstr);
			return(dict_idx[i].i_cnt / 3);
		}
	}
	return(-1);
}

int ChineseLooper::GetMatch(int8 matchIndex, char *dest)
{
//log("GetMatch invoked");
	int r = -1;
	if (gDictLocker.Lock()) {
//char buf[80];
//sprintf(buf, "\tmatchIndex = %d, matchNum = %d", matchIndex, matchNum);
//log(buf);
		if (matchIndex < matchNum) {
	   		for (int j = 0; j < 3; j++) {
	   			dest[j] = MatchList[matchIndex][j];
	   		}
	   		dest[3] = '\0';
			r = 0;
		}
	
		gDictLocker.Unlock();
	}
	return r;
}

/*
//get the key of user's input
void ChineseLooper::Get_Usr_Input(int8 utf8code, int8 keyidx)
{
 	char tempbuf[2];

	currentIndex++;
	currentIndex = currentIndex % MAX_CODECNT;
	if (currentIndex == 0) {
		strcpy(usrInput_hz, DictHead.key[keyidx].ch);
		usrInput_hz[3]='\0';
		currentIndex=1;
	}
	else {
		strcat(usrInput_hz, DictHead.key[keyidx].ch);
	}
	
  	if (currentIndex == 1) {
		sprintf(usrInput,"%c",utf8code);
		usrInput[1]='\0';
	} else {
		sprintf(tempbuf,"%c",utf8code);
		tempbuf[1]='\0';
		strcat(usrInput,tempbuf);
	}
	//if(fInputWin->IsHidden())
	//	fInputWin->Show();
	//fInputWin->Lock();
	//fInputWin->fInputview->SetText(usrInput_hz);
	//fInputWin->Unlock();
}
*/

void ChineseLooper::HandleKeyDown(BMessage *message)
{
//log("HandleKeyDown:");
	uint32 modifiers = message->FindInt32("modifiers");

	// return if any modifiers other than the shift-key are down
	if ( ((modifiers & B_COMMAND_KEY) != 0) || 
		 ((modifiers & B_CONTROL_KEY) != 0) || 
		 ((modifiers & B_OPTION_KEY) != 0) )
	{
		EnqueueMessage(DetachCurrentMessage());
		return;
	}

	int8 theChar = 0;
	int8 keyIndex;

	message->FindInt8("byte", &theChar);

	switch (fState) {
	case EMPTY:
//log("\tcase EMPTY");
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
			// don't start an input area for any of these keys
			EnqueueMessage(DetachCurrentMessage());
			return;

		default:
			Append(theChar, modifiers);
			break;
		}
		break;

	case SHOWING_TEXT:
//log("\tcase SHOWING_TEXT");
		switch (theChar) {
		case B_ESCAPE:
			ResetText();
			StopInput(false, true);
			Reset();
			break;
			
		case B_SPACE:
			if (DoConversion()) {
				SetState(SHOWING_CHOICES);
				StartMatchWindow();
			}
			break;

		case B_BACKSPACE:
		case B_DELETE:
			DoBackspace();
			break;
		
		default:
			Append(theChar, modifiers);
			if (IsEndKey(theChar)) {
				if (DoConversion()) {
					SetState(SHOWING_CHOICES);
					StartMatchWindow();
				}				
			}
			break;
		}
		break;
/*
	case SHOWING_CONVERTED:
//log("\tcase SHOWING_CONVERTED");
		switch (theChar) {
		case B_ESCAPE:
			SetState(SHOWING_TEXT);
			StopInput(false, true);
			break;

		case B_ENTER:
			StopInput(false, true);
			Reset();
			break;

		case B_BACKSPACE:
		case B_DELETE:
			SetState(SHOWING_TEXT);
			EnqueueMessage(GenerateInputMethodEvent(fEnteredHanzi.String(), false, false));
			break;

		case B_UP_ARROW:
			SetState(SHOWING_CHOICES);
			StartMatchWindow();
			SelectMatch(fMatchIndex - 1);
			break;
			
		case B_DOWN_ARROW:
		case B_SPACE:
			SetState(SHOWING_CHOICES);
			StartMatchWindow();
			SelectMatch(fMatchIndex + 1);
			break;
			
		default:
			// Check to see if this key would start a new input area, in which case a
			// new area should be started and this one should be finalized
			if (IsInputKey(tolower(theChar), &keyIndex) == 0) {
				StopInput(false, true);
				Reset();
				Append(theChar, modifiers);
			}
			
			break;	
		}
		break;
*/		
	case SHOWING_CHOICES:
//log("\tcase SHOWING_CHOICES");
		switch (theChar) {
		case B_ENTER:
			StopMatchWindow();
			StopInput(false, true);
			Reset();
			break;

		case B_ESCAPE:
		case B_BACKSPACE:
		case B_DELETE:
			StopMatchWindow();
			SetState(SHOWING_TEXT);
			EnqueueMessage(GenerateInputMethodEvent(fEnteredHanzi.String(), false, false));
			break;

		case B_UP_ARROW:
			SelectMatch(fMatchIndex - 1);
			break;
			
		case B_DOWN_ARROW:
		case B_SPACE:
			SelectMatch(fMatchIndex + 1);
			break;
		
		case B_PAGE_DOWN:
		case '+':
		case '=':
			SelectMatch((fCurrentPage + 1) * MATCH_PAGE_SIZE);
			break;
			
		case B_PAGE_UP:
		case '-':
			SelectMatch((fCurrentPage - 1) * MATCH_PAGE_SIZE);
			break;

		default:
			// look for number keys, and select if applicable
			if (theChar >= '1' && theChar <= '9') {
				SelectMatch((theChar - '1') + (fCurrentPage * MATCH_PAGE_SIZE));
				StopMatchWindow();
				StopInput(false, true);
				Reset();
			} else {
				// Check to see if this key would start a new input area, in which case a
				// new area should be started and this one should be finalized
				if (IsInputKey(tolower(theChar), &keyIndex) == 0) {
					StopMatchWindow();
					StopInput(false, true);
					Reset();
					Append(theChar, modifiers);
				}		
			break;	

			}
			break;
		}	
		break;
	}

}

//const char *states[] = {
//	"EMPTY",
//	"SHOWING_TEXT",
//	"SHOWING_CONVERTED",
//	"SHOWING_CHOICES",
//	NULL
//};

void ChineseLooper::SetState(input_state state)
{
//log("SetState called", states[state]);
	fState = state;
}

void ChineseLooper::StartInput()
{
//log("StartInput invoked");
	BMessage *startInput = new BMessage(B_INPUT_METHOD_EVENT);
	startInput->AddInt32("be:opcode", B_INPUT_METHOD_STARTED);
	startInput->AddMessenger("be:reply_to", BMessenger(this));

	EnqueueMessage(startInput);	
}

void ChineseLooper::StopInput(bool onlyStop, bool confirm)
{
//log("StopInput invoked");
	if (fState == EMPTY) {
		return;
	}
	
	if (!onlyStop) {
		EnqueueMessage(GenerateInputMethodEvent(CurrentText(), false, confirm));
	}

	BMessage *stopInput = new BMessage(B_INPUT_METHOD_EVENT);
	stopInput->AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);

	EnqueueMessage(stopInput);
}

void ChineseLooper::StartMatchWindow()
{
//log("StartMatchWindow invoked");
	BRect frame(50.0f, 50.0f, 50.0f + MATCH_WINDOW_WIDTH, 50.0f + 100);
	fMatchWin = new ChineseMatchWindow(frame, this);
	// starts the process of displaying the match window
	BMessage *locRequest = new BMessage(B_INPUT_METHOD_EVENT);
	locRequest->AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);
	EnqueueMessage(locRequest);
}

void ChineseLooper::StopMatchWindow()
{
//log("StopMatchWindow invoked");
	if (fMatchWin) {
		fMatchWin->PostMessage(B_QUIT_REQUESTED);
		fMatchWin = NULL;
	}
}

void ChineseLooper::ResetText()
{
//log("ResetText invoked");
	fEntered = B_EMPTY_STRING;
	fEnteredHanzi = B_EMPTY_STRING;
	fConverted = B_EMPTY_STRING;
}

void ChineseLooper::Reset()
{
//log("Reset invoked");
	StopMatchWindow();
	ResetText();
	SetState(EMPTY);
	fMatchIndex = 0;
	fCurrentPage = 0;
}

//open the dict file,get head information.called when enter  the inputmethod
//argument:dictFile:path of dict file
//return value:
//0:success
//-1:open dict file error
//-2:read dict file error
//-3:memory error
//-4:read index of dict file error
int ChineseLooper::OpenDict(const char *dictFile)
{
//openlog();
	int r = 0;
	BAutolock lock(gDictLocker);
	
	char tmppath[120];
	sprintf(tmppath, "%s%s", DictPath, dictFile);
	if ((dictfp = fopen(tmppath,"r")) == NULL) {
		r = -1;
		goto done;
	}

	if (fread((void *)&DictHead, HEADSZ, 1, dictfp) !=  1) {
		fclose(dictfp);
		r = -2;
		goto done;
	}

	dict_idx = (struct dict_index *)malloc(INDEXSZ * DictHead.codecnt);
	if (dict_idx == NULL) {
		fclose(dictfp);
		r = -3;
		goto done;
	}

	if (fread((void *)dict_idx, INDEXSZ, DictHead.codecnt, dictfp) != (size_t)DictHead.codecnt) {
		fclose(dictfp);
		free(dict_idx);
		r = -4;
	}

done:
	return r;
}

// close the dict file, called when leaving input method
int ChineseLooper::CloseDict()
{
//closelog();
	if (gDictLocker.Lock()) {
		free(dict_idx);
		int r = fclose(dictfp);
		gDictLocker.Unlock();
		return r;
	} else {
		return -1;
	}
}

void ChineseLooper::Trim(char *srcstr)
{
	int	i = 0;
	char	str1[1024];
	char	*strp = srcstr;

	while (*srcstr != '\0') {
		if (*srcstr == ' ') {
			srcstr++;
			continue;
		}
		str1[i++] = *(srcstr++);
	}
	str1[i] = '\0';
	strcpy(strp, str1);
}

int ChineseLooper::InitSub()
{
	char estr[1024], cstr1[1024],cstr2[1024];
	char	*strp;

	if (gDictLocker.Lock()) {
		MethodCount = 0;
		fp = fopen("/boot/home/config/settings/chinese-input/chinese.cfg","rw+");
		if (fp == NULL){
			return B_ERROR;
		}
		while (!feof(fp)) {
	 		if (fgets(estr,1024,fp) == NULL) {
				break;
			}
			if ((strp = strchr(estr, SEPCHAR)) == NULL) {
				if ((strp = strchr(estr, '\t')) == NULL) {
					continue;
				}
			}
			*strp = '\0';
			strcpy(cstr1, strp+1);
			if ((strp = strchr(cstr1, '\n')) != NULL) {
				*strp = '\0';
			}
			Trim(cstr1);
			strcpy(cstr2, estr);
			Trim(cstr2);
			
			if (cstr2[0]=='#') {
				continue;
			} else if (strncmp(cstr2, "MethodType", 10) == 0){
				strcpy(ch_head.MethodName,cstr1);
			} else {
				strcpy(ch_input[MethodCount].InputName, cstr2);
				strcpy(ch_input[MethodCount].DictName, cstr1);
				MethodCount++;
			}
			continue;
	 	}
		fclose(fp);
	
		fp = fopen("/boot/home/config/settings/chinese-input/.last","r");
		if (fp == NULL || (fgets(estr,1024,fp) == NULL)){
			ch_head.LastMethod=0;
		} else {
			ch_head.LastMethod = atoi(estr);
		}
		fclose(fp);
		gDictLocker.Unlock();
		return 0;
	} else {
		return -1;
	}
}

//---------------------------------------------------------------------------

TLabelWindow::TLabelWindow(BRect frame, const char *title, ChineseLooper* owner)
	: BWindow(frame, title, (window_look)25/*B_FLOATING_WINDOW_LOOK*/, B_FLOATING_ALL_WINDOW_FEEL,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE |
		B_NOT_CLOSABLE | B_AVOID_FOCUS),
	  fOwner(owner)
{
	int32 i;

	BMenuItem* menuItem;
	menuBar = new BMenuBar(BRect(1,1,80,15), "menubar");
	AddChild(menuBar);

	if (gDictLocker.Lock()) {
		menu = new BMenu(ch_head.MethodName);
		menu->SetRadioMode(true);
		for (i = 0; i < MethodCount; i++) {
			if (i > 0) {
				BSeparatorItem* separator = new BSeparatorItem();
				menu->AddItem(separator);
			}
	
			BMessage *msg = new BMessage(msg_MethodSelected);
			msg->AddInt32("index", i);
			menuItem = new BMenuItem(ch_input[i].InputName, msg);
			menu->AddItem(menuItem);
		}
	
		menuBar->AddItem(menu);
		menu->SetTargetForItems(fOwner);
		i = ch_head.LastMethod;
	
		if (i >= MethodCount){
			i = 0;
			ch_head.LastMethod = 0;
		}

		menuItem = menu->FindItem(i);
		if (menuItem != NULL) {
			menuItem->SetMarked(true);
		}

		gDictLocker.Unlock();
	}
}

void TLabelWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
	default:
	  	BWindow::MessageReceived(message);
	  	break;
	}
}

//----------------------------------------------------------------------------

ChineseMatchWindow::ChineseMatchWindow(BRect frame, ChineseLooper* owner)
	: BWindow(frame , "ChineseMatchWindow", B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
		B_NOT_CLOSABLE | B_AVOID_FOCUS),
	  fOwner(owner)
{
//log("ChineseMatchWindow constructor invoked");
	fListView = new BListView(Bounds(), "matchview", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
	fListView->SetSelectionMessage(new BMessage(msg_MatchSelected));
	fListView->SetInvocationMessage(new BMessage(msg_MatchInvoked));
	
	// set the list's font to be at least the minimum point size
	BFont font;
	fListView->GetFont(&font);
	if (font.Size() < MATCH_WINDOW_MIN_FONT_SIZE) {
		font.SetSize(MATCH_WINDOW_MIN_FONT_SIZE);
		fListView->SetFont(&font, B_FONT_ALL);
	}
	
	AddChild(fListView);
	fListView->SetTarget(this);
	
	
	Populate();
	// start the window hidden
	Hide();
	Show();
}

ChineseMatchWindow::~ChineseMatchWindow()
{
	// do nothing
}

bool ChineseMatchWindow::Populate(int32 page)
{
//log("ChineseMatchWindow Populate invoked");
	bool r = false;
	
	fCurrentPage = page;
	if (Lock()) {
		// first, empty the list
		int32 count = fListView->CountItems();
		for (int32 i = count - 1; i >= 0; i--) {
			BListItem* item = fListView->RemoveItem(i);
			delete item;
		} 

		// next, populate the list with entries that match the user input
		if (gDictLocker.Lock()) {	
			int8 i,j;
			char HZ[4];
			int startIndex;
			int tmpcnt;
		
			startIndex = MATCH_PAGE_SIZE * fCurrentPage;
			tmpcnt = matchNum - startIndex;
			if (tmpcnt >= MATCH_PAGE_SIZE) {
				tmpcnt = MATCH_PAGE_SIZE;
			}

			for (i = 0; i < tmpcnt; i++) {
				for (j = 0; j < 3; j++) {
					HZ[j] = MatchList[i + startIndex][j];
				}
				HZ[3] = '\0';
				fListView->AddItem(new ChineseMatchItem(i + 1, HZ));
				if (!r) r = true;
			}

			// the window must be sized at least 1 line tall
			if (i < 1) i = 1;
			// if there are more than MATCH_PAGE_SIZE matches, don't
			// resize the window to be smaller than its full size so
			// that the window will not change in size when the pages
			// are flipped
			if (matchNum > MATCH_PAGE_SIZE) i = MATCH_PAGE_SIZE;
			
			gDictLocker.Unlock();

			// resize the window appropriately
			float itemHeight = ceilf(fListView->ItemFrame(0).Height() + 1.0f);
			ResizeTo(MATCH_WINDOW_WIDTH, (itemHeight * i) - 1.0f);
		}
		fListView->Select(0);
		Unlock();
	}
	return r;
}

void ChineseMatchWindow::MessageReceived(BMessage *message)
{
	int32 index = 0;
	int32 page = 0;
	int32 count = 0;

	switch (message->what) {
	case msg_ShowMatchWindow:
		{
			BPoint p;
			float h;
			if ((message->FindPoint("point", &p) == B_OK) &&
				(message->FindFloat("height", &h) == B_OK))
			{
				// determine if the popup should be displayed above or below the point
				BRect screenFrame(BScreen().Frame());
				BRect bounds(Bounds());
				BPoint windowLoc(p);

				if ((p.y + h + bounds.Height() + MATCH_WINDOW_OFFSET) <= screenFrame.bottom) {
					windowLoc.y = p.y + h + MATCH_WINDOW_OFFSET;
				} else {
					windowLoc.y = p.y - bounds.Height() - MATCH_WINDOW_OFFSET;
				}
				
				MoveTo(windowLoc);
			}
			if (IsHidden()) {
				Show();
			}
		}
		break;
	case msg_SelectMatch:
//log("ChineseMatchWindow::MessageReceived() processing msg_SelectMatch\n");
		if (message->FindInt32("page", &page) == B_OK) {
			if (page != fCurrentPage) {
				fCurrentPage = page;
				Populate(fCurrentPage);
			}
		}

		if (message->FindInt32("index", &index) == B_OK) {
			index = index % MATCH_PAGE_SIZE;
			count = fListView->CountItems();
			if (index >= count) index = count - 1;
			if (index < 0) index = 0;
			fListView->Select(index);
		}
		break;

	default:
		BWindow::MessageReceived(message);
	}
}

//------------------------------------------------------------------------------

const rgb_color	kHilightColor		= {255, 152, 152, 255};

ChineseMatchItem::ChineseMatchItem(int32 index, const char *hanzi)
	: BListItem(),
	  fHanzi(hanzi),
	  fIndex(B_EMPTY_STRING),
	  fIndexWidth(0.0f),
	  fTextBaseline(0.0f)
{
	fIndex << index << ".";
}

ChineseMatchItem::~ChineseMatchItem()
{
}

void ChineseMatchItem::DrawItem(BView *owner, BRect bounds, bool complete)
{
	rgb_color	saveLow = owner->LowColor();
	bool		selected = IsSelected();

	if (complete || selected) {
		if (selected) {
			owner->SetLowColor(kHilightColor);
		}
		owner->FillRect(bounds, B_SOLID_LOW);
	}
	
	float offset = bounds.left + 3.0f;
	owner->MovePenTo(offset, bounds.top + fTextBaseline);
	owner->DrawString(fIndex.String());
	owner->MovePenTo(offset + fIndexWidth + 3.0f, bounds.top + fTextBaseline);
	owner->DrawString(fHanzi.String());

	owner->SetLowColor(saveLow);
}

void ChineseMatchItem::Update(BView */*owner*/, const BFont *font)
{
	font_height fh;
	font->GetHeight(&fh);

	fTextBaseline = ceilf(fh.ascent);
	fIndexWidth = ceilf(font->StringWidth("M."));
	
	SetHeight(ceilf(fh.ascent + fh.descent + fh.leading));
	SetWidth(6.0f + fIndexWidth + ceilf(font->StringWidth(fHanzi.String())));
}
	
//------------------------------------------------------------------------------

/*
filter_result ChineseInputMethod::InputFilter(BMessage *message)
{
	filter_result res = B_SKIP_MESSAGE;
	int8 utf8code;
	int8 keyidx;
       	int32 select;

	message->FindInt8("byte",&utf8code);
	//wait for user's  input
 	if(fInputWin->IsHidden()&&fMatchWin->IsHidden())
	{
	  	if(IsInputKey(utf8code,&keyidx)==0)//a-z
	  	{
			Get_Usr_Input(utf8code,keyidx);
            		return res;
	  	}
		else{
			res = B_DISPATCH_MESSAGE;
			return res;
		}
	}
	else
	//wait for user to  select one HZ from match list
 	if(!fInputWin->IsHidden()&&!fMatchWin->IsHidden())
	{
        	select=fMatchWin->fListview->CurrentSelection(0);
		if(IsSelectKey(utf8code)==0){
			fMatchWin->Lock();
			fMatchWin->fListview->Select(utf8code-49);
			fMatchWin->fListview->ScrollToSelection();
			fMatchWin->Unlock();
			Get_Select_HZ();
			return res;
		}
		else if(utf8code==32){//use space key to select HZ
			Get_Select_HZ();
			return res;
		}
	  	else if(IsScrollKey(utf8code)==0){
			currentList = currentList % matchListNum;
			Set_Match_List();
	     		return res;
		}
		else if(IsArrowKey(utf8code,&select)==0){
        		fMatchWin->Lock();
        		fMatchWin->fListview->Select(select);
        		fMatchWin->fListview->ScrollToSelection();
        		fMatchWin->Unlock();
		}	
		else if(utf8code==27){ //ESC
			Hide_All_Window();		
	     		return res;
               	}
		else{
			res = B_DISPATCH_MESSAGE;
			return res;
		}
	}
	//in input state
	else if(!fInputWin->IsHidden())
	{
		if(IsEndKey(utf8code,&keyidx)==0){
			Get_Usr_Input(utf8code,keyidx);
  			matchNum=MatchDict((char *)usrInput,(char **)MatchList);
  			if(matchNum>0){
				if(matchNum%MATCH_PAGE_SIZE==0)
					matchListNum=matchNum/MATCH_PAGE_SIZE;
				else
					matchListNum=matchNum/MATCH_PAGE_SIZE+1;
  				fMatchWin->Show();
				Set_Match_List();
			}
	    		return res ;
		}
		else
	  	if(IsInputKey(utf8code,&keyidx)==0)
	  	{
			Get_Usr_Input(utf8code,keyidx);
	    		return res ;
		 }
		else
		if(utf8code==32){ //space key
			if(usrInput[0]=='\0')
			{
 				if(!fMatchWin->IsHidden())
					fMatchWin->Hide();
	    		return res ;
			}
  			matchNum=MatchDict((char *)usrInput,(char **)MatchList);
  			if(matchNum>0){
				if(matchNum%MATCH_PAGE_SIZE==0)
					matchListNum=matchNum/MATCH_PAGE_SIZE;
				else
					matchListNum=matchNum/MATCH_PAGE_SIZE+1;
  				fMatchWin->Show();
				Set_Match_List();
			}
	    		return res ;
		}
		else if(utf8code== 8){ //backspace
			Del_Usr_Input();
	    		return res ;
		}
		else if(utf8code==27){ //ESC
			Hide_All_Window();		
               	}
	   }		
  return res;
}

void ChineseInputMethod::Hide_All_Window()		
{
	if(!fInputWin->IsHidden())
		fInputWin->Hide();
 	if(!fMatchWin->IsHidden())
		fMatchWin->Hide();
 	usrInput[0]='\0';
 	usrInput_hz[0]='\0';
 	currentIndex=0;
 	currentList=0;
}

//modify user's input
void ChineseInputMethod::Del_Usr_Input()
{
   int i,j;
	if(currentIndex==0){
		if(!fInputWin->IsHidden())
			fInputWin->Hide();
		} 
	else{
		currentIndex--;
		if(currentIndex==0){
			usrInput_hz[0]='\0';
			if(!fInputWin->IsHidden())
				fInputWin->Hide();
		} 
		else{
			usrInput[currentIndex]='\0';
			usrInput_hz[0]='\0';
			fInputWin->Lock();
			for(i=0;i<currentIndex;i++)
				for(j=0;j<DictHead.keycnt;j++)
        				if(usrInput[i]==DictHead.key[j].name)       
						strcat(usrInput_hz,DictHead.key[j].ch);
			fInputWin->fInputview->SetText(usrInput_hz);
			fInputWin->Unlock();
		}
	}
}


//get the HZ of user's select
void ChineseInputMethod::Get_Select_HZ()
{
 	int32 selected;
 	int tmpidx;
 	BMessage *newmsg;
 
 	usrInput[0]='\0';
 	usrInput_hz[0]='\0';
	
 	if(fMatchWin->IsHidden())
        	return;
 	if(fMatchWin->fListview->IsEmpty()) {
    		if(!fMatchWin->IsHidden())
			fMatchWin->Hide();
    		fInputWin->Lock();
    		fInputWin->fInputview->SetText("");
    		fInputWin->Unlock();
        	return;
  	}  
 	selected=fMatchWin->fListview->CurrentSelection(0);
 	if(!fMatchWin->IsHidden())
 		fMatchWin->Hide();
 	fInputWin->Lock();
 	fInputWin->fInputview->SetText("");
 	fInputWin->Unlock();
 	tmpidx=currentList*MATCH_PAGE_SIZE+selected;
 	for (int8 i = 0; i < 3; i++) {
		newmsg=new BMessage(B_KEY_DOWN);
		newmsg->AddInt8("byte",(int8)MatchList[tmpidx][i]);
		EnqueueMessage(newmsg);
	}
 	currentList=0;
 	currentIndex=0;
 	if(!fInputWin->IsHidden())
 		fInputWin->Hide();
}

//add the match list HZ
void ChineseInputMethod::Set_Match_List()
{
  	int8 i,j;
  	char HZ[4];
  	char tmpstr[3];
  	int8 currlist;
  	int8 tmpcnt;

  	fMatchWin->Lock();
  	if(!fMatchWin->fListview->IsEmpty())
		fMatchWin->fListview->MakeEmpty();
  	currlist=MATCH_PAGE_SIZE*currentList;
  	tmpcnt=matchNum-currlist;
  	if(tmpcnt>=MATCH_PAGE_SIZE)
		tmpcnt=MATCH_PAGE_SIZE;
  	for(i=0;i<tmpcnt;i++) {
     		for(j=0;j<3;j++)
     			HZ[j]=MatchList[i+currlist][j];
     		HZ[3]='\0';
     		sprintf(tmpstr,"%d",i+1);
     		tmpstr[1]='.';
     		tmpstr[2]='\0';
     		strcat(tmpstr,HZ);
     		fMatchWin->fListview->AddItem(new BStringItem(tmpstr));
   	}                  
  	fMatchWin->fListview->Select(0);
  	fMatchWin->Unlock();
}
*/
