#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Application.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <Debug.h>
#include <add-ons/input_server/InputServerFilter.h>

#include "MessageWriter.h"
#include "win.h"

#define APP_VERSION "0.4"
/*
	Version Hisory:
	0.4 - integrated InputRecorder and InputRecorderFilter
	0.3.2 - added support for Warm Keys message-based hotkeys
	0.3.1 - added --minimized flag
	0.3 - released with the R4.5 CD sample code
	0.2 - released with Steven Black's March nerwsletter article.
	0.1.x - lacked version information
*/

/* For WarmKeys support */
#define RECORD_KEY 'R' 
#define PLAY_KEY 'P'
#define STOP_KEY 'S'
#define WINDOW_KEY 'W'

#define T_WATCH_MODIFIER (B_CONTROL_KEY|B_COMMAND_KEY)

extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

class InputRecorderFilter : public BInputServerFilter 
{
public:
	InputRecorderFilter();
	virtual ~InputRecorderFilter();
	virtual	filter_result Filter(BMessage *message, BList *outList);
private:

#if !USE_TMP_FILE
	area_id _input_area;
#endif
	InputRecorderWin *win;
	int32 _record_keycode, _stop_keycode, _play_keycode, _window_keycode;
	MessageWriter *msw;
	int32 _HavePaused;
#if RELATIVE_COORDS
	BPoint last;
#endif

	// A generally useful helper function
	// Last two arguments are in case you want to cache them, if NULL it finds/frees them.
	int32 get_keycode_for_key(char in, int32 *mods = NULL, key_map *KeyMap = NULL, char *KeysBuffer = NULL);
	int pstrcmp(char *str0, char *str1);
};

BInputServerFilter* instantiate_input_filter()
{
	return (new InputRecorderFilter());
}


InputRecorderFilter::InputRecorderFilter()
{
	// SET_DEBUG_ENABLED(TRUE);
	SERIAL_PRINT(("InputRecorder: We see signs of life\n"));
	
#if INPUT_RECORDER_WRITES
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append(INPUT_RECORD_PATH);
	create_directory(path.Path(),0755);

	BMimeType mime;
	status_t err;
	if((err = mime.SetTo(DATA_TYPE))!=B_OK){
		SERIAL_PRINT(("InputRecorder: Unable to set mime type - err: (0x%x) %s\n",err,strerror(err)));
	} else if ( !mime.IsInstalled()) {
		err = mime.Install();
		err = mime.SetShortDescription("Input Record");
		err = mime.SetLongDescription("Record of Input Events");
	}
#endif

	_record_keycode = get_keycode_for_key(RECORD_KEY, NULL);
	_stop_keycode = get_keycode_for_key(STOP_KEY, NULL);
	_play_keycode = get_keycode_for_key(PLAY_KEY, NULL);
	_window_keycode = get_keycode_for_key(WINDOW_KEY, NULL);

#if RELATIVE_COORDS
	last.Set(320, 240);
#endif
	
	win = new InputRecorderWin();
	win->Minimize(true);
	win->Show();

#if USE_TMP_FILE
	msw = new FlatMessageWriter(TMP_INPUT_RECORD,B_WRITE_ONLY);
#else
	area_id input_area = find_area("InputRecord");
	if(input_area < 0) {
		input_area = create_area("InputRecord", NULL, B_ANY_ADDRESS,
			MAX_INPUT_RECORD_SIZE, B_NO_LOCK, B_READ_AREA|B_WRITE_AREA );
	}
	msw = new AreaMessageWriter(input_area,B_WRITE_ONLY);
#endif

	status_t err = B_OK;
	if((err = msw->InitCheck())!=B_OK) {
#if USE_TMP_FILE
		SERIAL_PRINT(("%s:%d - Failed to open for write %s : (0x%x) %s\n", __FILE__, __LINE__,
			TMP_INPUT_RECORD, 
			err, strerror(err)));
#else
		SERIAL_PRINT(("%s:%d - Failed to open for write %s (0x%x): (0x%x) %s\n", __FILE__, __LINE__,
			"<<AREA>>", _input_area,
			err, strerror(err)));
#endif
	}

}

InputRecorderFilter::~InputRecorderFilter()
{
	win->Minimize(true);
	if(win->Lock()){
		win->Quit();
	}
}

filter_result InputRecorderFilter::Filter(BMessage *message, BList *outList)
{
	long code, length;
	char *buffer ;
	BMessage event;
	status_t err;
	filter_result res = B_DISPATCH_MESSAGE;
	if(message->what==0){
		SERIAL_PRINT(("InputRecorderFilter: bogus message (what==0)\n"));
		return(res);
	}
	if(message->what==B_KEY_DOWN){
		int32 key;
		if(message->FindInt32("modifiers",&key)==B_OK) {
			// This should catch multiple modifiers reasonably
			// Note: Checking for pure equality is problematic as modifier keys with 
			//   left/right versions have their own modifier mask.
			if((key&T_WATCH_MODIFIER) == T_WATCH_MODIFIER) {
				if(message->FindInt32("key",&key)==B_OK) {
					if( key == _record_keycode) {
						if(win->GetMode() == MODE_STOP) {
							SERIAL_PRINT(("InputRecorder: Record Mode Requested\n"));
							win->SetMode(MODE_RECORD);
						}
						message->AddInt32("InputRecorder:Mark", B_SKIP_MESSAGE);
					}else if( key == _play_keycode) {
						if(win->GetMode() == MODE_STOP) {
							SERIAL_PRINT(("InputRecorder: Play Mode Requested\n"));
							win->SetMode(MODE_PLAY);
						}
						message->AddInt32("InputRecorder:Mark", B_SKIP_MESSAGE);
					}else if( key == _stop_keycode) {
						if(win->GetMode() != MODE_STOP) {
							SERIAL_PRINT(("InputRecorder: Stop Mode Requested\n"));
							win->SetMode(MODE_STOP);
						}
						message->AddInt32("InputRecorder:Mark", B_SKIP_MESSAGE);
					}else if( key == _window_keycode) {
						win->Minimize(!win->IsMinimized());
						message->AddInt32("InputRecorder:Mark", B_SKIP_MESSAGE);
					}
				}
			}
		}
	}
	int32 mark;
	if(message->FindInt32("InputRecorder:Mark",&mark)==B_OK) {
		if(mark==B_DISPATCH_MESSAGE) res = B_DISPATCH_MESSAGE;
		else if(mark==B_SKIP_MESSAGE) res = B_SKIP_MESSAGE;
		else if(win->IsInputLocked()) res = B_SKIP_MESSAGE;
	}else if(win->GetMode()==MODE_PLAY && message->what!=B_MODIFIERS_CHANGED && win->IsMinimized()) {
		if(win->IsInputLocked()) res = B_SKIP_MESSAGE;
	} else if(win->GetMode()==MODE_RECORD){
		if(!win->IsMinimized()){
			if(win->ResetNewRecord()) {
				msw->ResetDataPointer();
				_HavePaused = false;
			} else if (_HavePaused) {
				BMessage timewarp(T_TIME_WARP);
				timewarp.AddInt64("when",system_time());
				if (msw->Write(&timewarp) != B_OK) {
					SERIAL_PRINT(("InputRecorderFilter: Error writing time-warp in to file\n"));
				}
			}
			if (msw->Write(message) != B_OK) {
				SERIAL_PRINT(("InputRecorderFilter: Error writing data in to file\n"));
			}
		}else{
			_HavePaused = true;
		}
	}	
	return (res);
}

/* Pascal string compare */
int InputRecorderFilter::pstrcmp(char *str0, char *str1) 
{
	int i, res = 0;
	if (str0[0] != str1[0]) return(str0[0] - str1[0]);
	for (i=1; i<=str0[0] && !(res = str0[i] - str1[i]); i++) ;
	return res;
}

int32 InputRecorderFilter::get_keycode_for_key(char in, int32 *mods, key_map *KeyMap, char *KeysBuffer)
{
	char pin[2] = {1, in};
	int32 loop;
	char *cs;
	int32 lmods = 0;

	key_map *_KeyMap = NULL;
	char *_KeysBuffer = NULL;
	bool freethem=false;

	if(KeyMap == NULL || KeysBuffer==NULL){
		get_key_map(&_KeyMap, &_KeysBuffer);
		freethem = true;
	} else {
		_KeyMap = KeyMap;
		_KeysBuffer = KeysBuffer;
	}
	if(!mods) mods = &lmods;
	*mods = 0;
	loop = -1;
	if(_KeyMap != NULL && _KeysBuffer!=NULL){
		do{
			loop ++;
			cs = _KeysBuffer+_KeyMap->normal_map[loop];
		} while(loop < 128 && pstrcmp(cs, pin));
		if(loop==128){
			loop = -1;
			do{
				loop ++;
				cs = _KeysBuffer+_KeyMap->shift_map[loop];
			} while(loop < 128 && pstrcmp(cs, pin));
			if(loop!=128) *mods |= B_SHIFT_KEY;
		}
		if(loop==128) loop=-1;
	}
	if(freethem){
		if(_KeyMap != NULL) free(_KeyMap);
		if(_KeysBuffer != NULL) free(_KeysBuffer);
	}
	return(loop);
}


