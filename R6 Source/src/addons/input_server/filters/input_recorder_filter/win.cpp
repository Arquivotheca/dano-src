
#include <Application.h>
#include <Alert.h>
#include <Debug.h>
#include <WindowScreen.h>
#include <Path.h>
#include <OS.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <String.h>
#include <add-ons/input_server/InputServerFilter.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>

#include "win.h"
#include "MessageWriter.h"
#include "view.h"

#define DEFAULT_TITLE "Input Recorder"

status_t call_device(port_id devicePort, BMessage *event);

InputRecorderWin::InputRecorderWin() : BWindow(BRect(50,50,300,120), DEFAULT_TITLE, B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	BRect rect = Bounds();
	BMenuBar *menu = NULL;
	BMenuItem *item = NULL;
	BMenu *sub = NULL;

	menu = new BMenuBar(rect, "MenuBar");
	sub = new BMenu("File");

#if INPUT_RECORDER_WRITES
	sub->AddItem(item = new BMenuItem("Open" B_UTF8_ELLIPSIS, new BMessage(T_OPEN), 'O'));
	sub->AddItem(item = new BMenuItem("Save" B_UTF8_ELLIPSIS, new BMessage(T_SAVE), 'S'));
	sub->AddItem(item = new BMenuItem("Save As" B_UTF8_ELLIPSIS, new BMessage(T_SAVEAS)));
	sub->AddSeparatorItem();
#endif

	sub->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED),'W'));
	menu->AddItem(sub);

	sub = new BMenu("Options");

	sub->AddItem(new BMenuItem("Loop", new BMessage(T_TOGGLE_LOOP),'L'));
	sub->AddItem(new BMenuItem("Lock", new BMessage(T_TOGGLE_LOCK),'K'));
	menu->AddItem(sub);

	AddChild(menu);

	rect = menu->Bounds();
	float hi = rect.bottom;
	rect = Bounds();
	rect.top += hi+ 1;

	BView *view = new TView(rect, "View", true, false, true);
	AddChild(view);
	
#if USE_TMP_FILE
#if INPUT_RECORDER_WRITES
	_Filename.Unset();
	_FileDirty = false;
#endif
	__internal_filename.SetTo(TMP_INPUT_RECORD);
#else
	_input_area = find_area("InputRecord");
	if(_input_area < 0) {
		_input_area = create_area("InputRecord", NULL, B_ANY_ADDRESS,
			MAX_INPUT_RECORD_SIZE, B_NO_LOCK, B_READ_AREA|B_WRITE_AREA );
	}
#endif

	_LoopMode = false;
	_LockMode = false;
	_Thread = -1;

#if INPUT_RECORDER_WRITES
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append(INPUT_RECORD_PATH);

	_Load = new BFilePanel(B_OPEN_PANEL);
	_Load->SetPanelDirectory(path.Path());
	_Load->Window()->SetTitle("Load InputRecord");

	_Save = new BFilePanel(B_SAVE_PANEL);
	_Save->SetPanelDirectory(path.Path());
	_Save->Window()->SetTitle("New InputRecord");
#endif
	_Messenger = NULL;
}

InputRecorderWin::~InputRecorderWin()
{
	if(_Thread!=-1) kill_thread(_Thread);
#if USE_TMP_FILE
	unlink(__internal_filename.Path());
#else
	delete_area(_input_area);
#endif
	if(_Messenger!=NULL) delete _Messenger;
}

bool InputRecorderWin::QuitRequested()
{
	if(IsMinimized()) return true;
	else {
		Minimize(true);
		return(false);
	}
}

void InputRecorderWin::_SetLoopMode(bool state)
{
	_LoopMode = state;
	BMenuBar *main = KeyMenuBar();
	BMenuItem *item;
	if(main!=NULL){
		item = main->FindItem("Loop");
		if(item!=NULL) item->SetMarked(_LoopMode);
	}
}

bool InputRecorderWin::IsInputLocked(void)
{
	return(_LockMode);
}

void InputRecorderWin::_SetLockMode(bool state)
{
	_LockMode = state;
	BMenuBar *main = KeyMenuBar();
	BMenuItem *item;
	if(main!=NULL){
		item = main->FindItem("Lock");
		if(item!=NULL) item->SetMarked(_LockMode);
	}
}

#if INPUT_RECORDER_WRITES
void InputRecorderWin::_SetFileName(const char *file, bool dirty)
{
	if(file!=NULL) {
		if(*file!='\0') _Filename.SetTo(file,NULL,true);
		else _Filename.Unset();
	}
	_FileDirty = dirty;
	if(Lock()){
		BString str;
		if(_Filename.InitCheck()==B_OK) {
			str = _Filename.Leaf();
		}else{
			str = DEFAULT_TITLE;
		}
		if(_FileDirty) str.Append(" (*)");
		SetTitle(str.String());
		Unlock();
	}
}
#endif

void InputRecorderWin::MessageReceived(BMessage *msg)
{
	status_t err = B_OK;
	switch(msg->what) {
#if INPUT_RECORDER_WRITES
		case T_OPEN:
			_Load->Show();
			break;
		case T_SAVE:
		case T_SAVEAS:
			_Save->Show();
			break;
		case B_SAVE_REQUESTED: {
			uint32 type; 
			int32 count;
			entry_ref ref;
			BPath path;
		
			type = B_ANY_TYPE;
			msg->GetInfo("directory", &type, &count); 
			if ( type == B_REF_TYPE && msg->FindRef("directory", 0, &ref) == B_OK ) { 
				BEntry entry(&ref, true);
				if ( entry.GetPath(&path)==B_OK){
					const char *name;
					if(msg->FindString("name",&name) == B_OK){
						path.Append(name);
						BMessage *mess = new BMessage(T_SAVE);
						mess->AddString("Path",path.Path());
						if(_Messenger==NULL) _Messenger = new BMessenger(NULL, this);
						_Messenger->SendMessage(mess);
					}
				}
			} 
			break;
		}
		case T_SAVE: {
			const char *file;
			if(msg->FindString("Path",&file)==B_OK) _SetFileName(file, true);
			if(_FileDirty){
				if(_Filename.InitCheck()==B_OK){
					_CopyTmpFileToSaveFile();
				}
			}
			break;
		}
		case T_START_INPUT: {
			const char *file;
			bool b;
			if(msg->FindString("Path",&file)==B_OK) _SetFileName(file, false);
			if(msg->FindBool("LoopState",&b)==B_OK) _SetLoopMode(b);
			if(msg->FindBool("LockInput",&b)==B_OK) _SetLockMode(b);
			if(msg->FindBool("AutoPlay",&b)!=B_OK) b = false;
			if(b) _StartThread(T_PLAY);
			break;
		}
		case B_SIMPLE_DATA: {
			uint32 type;
			int32 count = -1;
			msg->GetInfo("refs", &type, &count); 
			if ( count>0 && type == B_REF_TYPE ) {
				BMessage *mess = new BMessage(*msg);
				mess->what = B_REFS_RECEIVED;
				RefsReceived(msg);
			}
			break;
		}
#endif
		case T_TOGGLE_LOOP: {
			_SetLoopMode(!_LoopMode);
			break;
		}
		case T_TOGGLE_LOCK: {
			_SetLockMode(!_LockMode);
			break;
		}

		case T_RECORD: {
			SERIAL_PRINT(("%s:%d - ?\n",__FILE__,__LINE__));
#if INPUT_RECORDER_WRITES
			_SetFileName(NULL, true);
#endif
			SetMode(MODE_RECORD);
			break;
		}
			
		case T_PLAY: { 
			SERIAL_PRINT(("%s:%d - ?\n",__FILE__,__LINE__));
			SetMode(MODE_PLAY);
			break;	 
		}
						 	 
		case T_STOP: {
			SERIAL_PRINT(("%s:%d - ?\n",__FILE__,__LINE__));
			SetMode(MODE_STOP);
			break;
		}
		default: {
			BWindow::MessageReceived(msg); 
			break;
		}
	}
}

#if INPUT_RECORDER_WRITES

void InputRecorderWin :: RefsReceived(BMessage *msg)
{
	uint32 type; 
	int32 count;
	entry_ref ref;
	BPath path;

	type = B_ANY_TYPE;
	msg->GetInfo("refs", &type, &count); 
	if ( type != B_REF_TYPE ) return; 

	for ( long i = --count; i >= 0; i-- ) { 
		if ( msg->FindRef("refs", i, &ref) == B_OK ) { 
			BEntry entry(&ref, true);
			if ( entry.IsFile() && entry.GetPath(&path)==B_OK){
				BMessage *mess = new BMessage(T_START_INPUT);
				mess->AddString("Path",path.Path());
				mess->AddBool("AutoPlay",false);
				if(_Messenger==NULL) _Messenger = new BMessenger(NULL, this);
				_Messenger->SendMessage(mess);
			}
		} 
	} 
}

status_t InputRecorderWin::_CopyTmpFileToSaveFile(void)
{
	BFile internal(__internal_filename.Path(),B_READ_ONLY);
	BFile real(_Filename.Path(),B_WRITE_ONLY|B_ERASE_FILE|B_CREATE_FILE);
	const int32 blocksize = 65536;
	char *attr = (char*)malloc(B_ATTR_NAME_LENGTH);
	char *data = (char*)malloc(blocksize);
	if(attr!=NULL && data!=NULL){
		ssize_t read, writ;
		off_t offset = 0;
		do {
			read = internal.ReadAt(offset, data, blocksize);
			if(read > 0) writ = real.WriteAt(offset, data, read);
			else if(read<0) SERIAL_PRINT(("%s:%d - failed to read offset %Lx (0x%x) %s\n", __FILE__, __LINE__, offset, read, strerror(read)));
			if(read>0 && writ< 0) SERIAL_PRINT(("%s:%d - failed to write offset %Lx (0x%x) %s\n", __FILE__, __LINE__, offset, writ, strerror(writ)));
			offset += blocksize;
		}while(read > 0);
		status_t err;
		internal.RewindAttrs();
		do {
			err = internal.GetNextAttrName(attr);
			if(!err) {
				read = internal.ReadAttr(attr, 0, 0, data, blocksize);
				if(read > 0) writ = real.WriteAttr(attr, 0, 0, data, read);
				else SERIAL_PRINT(("%s:%d - failed to read (0x%x) %s\n", __FILE__, __LINE__, read, strerror(read)));
				if(read>0 && writ< 0) SERIAL_PRINT(("%s:%d - failed to write offset %Lx (0x%x) %s\n", __FILE__, __LINE__, offset, writ, strerror(writ)));
			}
		} while(!err);
		_SetFileName(NULL, false);
	}
	if(data) free(data);
	if(attr) free(attr);
	// should actually return error code
	return(B_OK);
}
#endif

status_t InputRecorderWin::_StartThread (int32 which)
{
	if(_Thread!=-1) {
		BAlert *al = new BAlert("Again", "InputRecorder: Already playing.","Oops");
		SERIAL_PRINT(("%s:%d - _Thread not -1, assuming thread is still recording/playing.\n",__FILE__,__LINE__));
		al->Go(NULL);
		return(B_ERROR);
	} else {
		switch(which){
		case T_PLAY:
			_Thread = spawn_thread (_StartPlayThread,"InputRecorder:Play",B_NORMAL_PRIORITY,this);
			break;
		}
		return ( resume_thread (_Thread) );
	}
}

int32 InputRecorderWin::GetMode(void)
{
	return(_Mode);
}

int32 InputRecorderWin::ResetNewRecord(void)
{
	int32 res = _NewRecord;
	_NewRecord = 0;
	return(res);
}

status_t InputRecorderWin::SetMode(int32 mode)
{
	BMessage *mess;
	BView *view;
	port_id devicePort;
	status_t status = B_OK;
	if(mode==_Mode) return(B_OK);
	switch(mode){
	case MODE_RECORD:
		_Mode = mode;
		_NewRecord++;
		if(!IsMinimized()) Minimize(true);
		mess = new BMessage(T_SET_BUTTONS);
		mess->AddBool("Record", false);
		mess->AddBool("Stop", true);
		mess->AddBool("Play", false);
		view = FindView("View");
		if(_Messenger==NULL) _Messenger = new BMessenger(NULL, this);
		if(view!=NULL) _Messenger->SendMessage(mess, view);
		else delete mess;

#if RELATIVE_COORDS
		devicePort = find_port("InputDevicePort");
		if(devicePort >= 0) {
			mess = new BMessage(B_MOUSE_MOVED);
			initMessage.AddInt64("when", system_time());
			initMessage.AddPoint("where", BPoint(320,240));
			initMessage.AddInt32("buttons", 0); 
			initMessage.AddInt32("modifiers", 0);
			call_device(devicePort, &initMessage);
		}else{
			SERIAL_PRINT(("Fatal Error: Couldn't find InputDevicePort.\n"
				"InputRecorderDevice not installed properly.\n"));
			SetMode(MODE_STOPPED);
		}
#endif

		break;
	case MODE_STOP:
		_Mode = mode;
		mess = new BMessage(T_SET_BUTTONS);
		mess->AddBool("Record", true);
		mess->AddBool("Stop", false);
		mess->AddBool("Play", true);
		view = FindView("View");
		if(_Messenger==NULL) _Messenger = new BMessenger(NULL, this);
		if(view!=NULL) _Messenger->SendMessage(mess, view);
		else delete mess;
		break;
	case MODE_PLAY:
		if(!IsMinimized()) Minimize(true);
		_Mode = mode;
		mess = new BMessage(T_SET_BUTTONS);
		mess->AddBool("Record", false);
		mess->AddBool("Stop", true);
		mess->AddBool("Play", false);
		view = FindView("View");
		if(_Messenger==NULL) _Messenger = new BMessenger(NULL, this);
		if(view!=NULL) _Messenger->SendMessage(mess, view);
		else delete mess;

		_StartThread(T_PLAY);
		break;
	default:
		status = B_BAD_VALUE;
	}
	return(status);
}

int32 InputRecorderWin::_StartPlayThread(void *arg)
{
	InputRecorderWin *self = (InputRecorderWin *)arg;
	return (self->_PlayThread() );
}

int32 InputRecorderWin::_PlayThread(void)
{
	port_id devicePort;
	int32 code;
	BMessage event;
	bool input_device_stop_requested = false;
	bool first_pass=true;
	bool end_of_file;
	bigtime_t lasttime = -1, newtime, offtime;
	MessageWriter *msr;

	BAlert *al;
	devicePort = find_port("InputDevicePort");
	if(devicePort < 0){
		SERIAL_PRINT(("InputRecorder: unable to find device port\n"));
		al = new BAlert("InputRecorderError", "InputRecorder: unable to find device port", "Gasp");
		al->Go();
		return(B_ERROR);
	}

	BMessage *mess = new BMessage(T_SET_BUTTONS);
	mess->AddBool("Record", false);
	mess->AddBool("Stop", !_LoopMode);
	mess->AddBool("Play", false);
	BView *view = FindView("View");
	if(_Messenger==NULL) _Messenger = new BMessenger(NULL, this);
	if(view!=NULL) _Messenger->SendMessage(mess, view);
	else delete mess;

	end_of_file = false;
	status_t err;
	
#if USE_TMP_FILE
	const char *name = __internal_filename.Path();
#if INPUT_RECORDER_WRITES
	if(_Filename.InitCheck()==B_OK && !_FileDirty) name = _Filename.Path();
#endif
	msr = new FlatMessageWriter(name,B_READ_ONLY);
#else
	msr = new AreaMessageWriter(_input_area,B_READ_ONLY);
#endif
	if((err = msr->InitCheck())!=B_OK) {
		SERIAL_PRINT(("%s:%d - Failed to open file for read %s: (0x%x) %s\n", __FILE__, __LINE__, name, err, strerror(err)));
	}

#if RELATIVE_COORDS
	BPoint last;
#endif
	first_pass = true;
	bool have_paused = false;
	while(! GetMode() == MODE_PLAY ) {
		while(!IsMinimized() && GetMode() == MODE_PLAY){
			have_paused = true;
			snooze(1000);
		}
		if (first_pass) {
#if RELATIVE_COORDS
			BMessage initMessage(B_MOUSE_MOVED);
			initMessage.AddInt64("when", system_time());
			initMessage.AddPoint("where", BPoint(320, 240));
			initMessage.AddInt32("buttons", 0); 
			initMessage.AddInt32("modifiers", 0);
			initMessage.AddInt32("InputRecorder:Mark", B_DISPATCH_MESSAGE);
			call_device(devicePort, &initMessage);
#endif
			msr->ResetDataPointer();
		}

		if (msr->HaveMore()) {
			if(msr->Read(&event) != B_OK){
				SERIAL_PRINT(("InputRecorder: resource read error\n"));
				al = new BAlert("InputRecorderError", "InputRecorder: input record read error", "Gasp");
				al->Go();
				SetMode(MODE_STOP);
			} else {
#if RELATIVE_COORDS
				BPoint cur(0,0);
				if(have_paused){ // go back to a location we know about
					BMessage initMessage(B_MOUSE_MOVED);
					initMessage.AddInt64("when", system_time());
					initMessage.AddPoint("where", last);
					initMessage.AddInt32("buttons", 0); 
					initMessage.AddInt32("modifiers", 0);
					initMessage.AddInt32("InputRecorder:Mark", B_DISPATCH_MESSAGE);
					call_device(devicePort, &initMessage);
				}

				if(event.FindPoint("where", &cur)==B_OK){
					event.AddInt32("x", cur.x - last.x);
					event.AddInt32("y", last.y - cur.y);
					// Yes, 'y' is backwards. It's to allow backwards compatibility
					//   with a bug. (Steven Black)
					event.RemoveName("where");
					last = cur;
				}
#endif
				event.AddInt32("InputRecorder:Mark", B_DISPATCH_MESSAGE);
				event.FindInt64("when",&newtime);
				offtime = newtime - lasttime;
				lasttime = newtime;

				if(event.what != T_TIME_WARP){
					if(first_pass || have_paused){
						first_pass = false;
						have_paused = false;
					}else {
						snooze(offtime); 
					}
					event.ReplaceInt64("when",system_time());
					if(call_device(devicePort, &event)!=B_OK){
						SERIAL_PRINT(("InputRecorder: failed to send packet\n"));
						al = new BAlert("InputRecorderError", "InputRecorder: failed to send packet", "Gasp");
						al->Go();
						SetMode(MODE_STOP);
					}
				}
			}

		} else {
			if(GetMode()==MODE_PLAY && _LoopMode) first_pass = true;
			else SetMode(MODE_STOP);
		}
	}
	delete msr;

	_Thread = -1;
	return B_OK;
}

status_t call_device(port_id devicePort, BMessage *event)
{
	status_t err = B_OK;
	if(devicePort < 0) return(devicePort);
	size_t size = event->FlattenedSize();
	char *buffer = (char*)malloc(size);
	if(buffer!=NULL){
		event->Flatten(buffer, size);
		if((err = write_port(devicePort, 0, buffer, size))!=B_OK){
			SERIAL_PRINT(("Error finding writing to device port: (0x%x) %s\n", err, strerror(err)));
		}
		free(buffer);
	}else{
		if(errno < 0) err = errno;
		if(!err) err = B_ERROR;
		SERIAL_PRINT(("Failed to allocate space for device buffer: (0x%x) %s\n", err, strerror(err)));
	}
	return(err);
}

