
#ifndef __INPUT_RECORDER_WIN_H
#define __INPUT_RECORDER_WIN_H 1

#include <Window.h>
#include <FilePanel.h>

#define INPUT_RECORD_PATH "InputRecorder/records"
#define TMP_INPUT_RECORD "/tmp/InputRecord"

#define MAX_INPUT_RECORD_SIZE 65536

#define RELATIVE_COORDS 0

#define T_SET_INPUT_LOCK 'tSIL'
#define T_START_INPUT 'tSTI'
#define T_TOGGLE_LOOP 'tLOP'
#define T_TOGGLE_LOCK 'tLCK'
#define T_RECORD 'tREC'
#define T_PLAY 'tPLY'
#define T_STOP 'tSTP'
#define T_SET_BUTTONS 'tSBT'
#define T_TIME_WARP 'tTWP'

enum {
MODE_STOP = 0, MODE_PLAY, MODE_RECORD
};

#define T_OPEN 'tOPN'
#define T_SAVE 'tSAV'
#define T_SAVEAS 'tSAA'

class InputRecorderWin: public BWindow
{
public:
	InputRecorderWin(void);
	virtual ~InputRecorderWin();

	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *msg);
#if INPUT_RECORDER_WRITES
	virtual void RefsReceived(BMessage *msg);
#endif
	bool IsInputLocked(void);
	int32 GetMode(void);
	status_t SetMode(int32 mode);
	int32 ResetNewRecord(void);
private:
	BMessenger *_Messenger;
	void _SetLoopMode(bool state);
	void _SetLockMode(bool state);
#if INPUT_RECORDER_WRITES
	void _SetFileName(const char *file, bool dirty);
	void _SetFileName(const char *file) { _SetFileName(file, _FileDirty); };
#endif

	status_t _StartThread ( int32 which );
	static int32 _StartPlayThread(void *arg);
	int32 _PlayThread(void);
	thread_id _Thread;
	int32 _NewRecord;
	int32 _Mode;
	
#if INPUT_RECORDER_WRITES
	status_t _CopyTmpFileToSaveFile(void);
	BPath _Filename;
	bool _FileDirty;
	BFilePanel *_Load;
	BFilePanel *_Save;
#endif

#if USE_TMP_FILE
	BPath __internal_filename;
#else
	area_id _input_area;
#endif
	bool _LoopMode;
	bool _LockMode;
};

#endif // __INPUT_RECORDER_WIN_H

