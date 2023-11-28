// IADMediaRecorder.cpp

#include "MediaRecorder.h"
#include "SourceList.h"

#include "miniplay.h"

#include <Autolock.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <MediaDefs.h>

using namespace BPrivate;

#define DEFAULT_THREAD_PRIORITY 115

BMediaRecorder::~BMediaRecorder()
{
	ASSERT(!_mLock.IsLocked());
	Disconnect();
	
	if(_mRunSem >= B_OK)
		delete_sem(_mRunSem);
		
	if (_mSources)
		delete _mSources;
}


BMediaRecorder::BMediaRecorder(const char *name, int32 priority) :
	_mLock(name ? name : "BMediaRecorder"),
	_mRun(0),
	_mRunSem(create_sem(0, "BMediaRecorder:_mRunSem")),
	_mThread(-1),
	_mInitErr(B_OK),
	_mThreadPriority(priority > 5 ? priority : DEFAULT_THREAD_PRIORITY),
	_mStreamID(-1),
	_mBufferHook(0),
	_mBufferCookie(0),
	_mConnected(false),
	_mRunning(false),
	_mSources(0)
	
{
	if(_mRunSem <= B_OK)
		_mInitErr = _mRunSem;
}

status_t 
BMediaRecorder::InitCheck()
{
	return _mInitErr;
}

status_t 
BMediaRecorder::SetBufferHook(buffer_hook hook, void *cookie)
{
	if(_mInitErr < B_OK) return _mInitErr;
	BAutolock _l(_mLock);
	_mBufferCookie = cookie;
	_mBufferHook = hook;
	return B_OK;
}

void 
BMediaRecorder::BufferReceived(void *data, size_t size, const media_header &header)
{
	if (_mBufferHook)
		(*_mBufferHook)(_mBufferCookie, data, size, header);
}

status_t 
BMediaRecorder::FetchSources(const media_format &format, bool physicalSourcesOnly)
{
	if(_mInitErr < B_OK) return _mInitErr;
	if (_mConnected) return B_MEDIA_ALREADY_CONNECTED;
	if(!_mSources)
		_mSources = new BSourceList;
		
	return _mSources->Fetch(format, physicalSourcesOnly);
}

int32 
BMediaRecorder::CountSources() const
{
	if (_mInitErr < B_OK) return _mInitErr;
	if (_mConnected) return B_MEDIA_ALREADY_CONNECTED;

	return _mSources ? _mSources->CountItems() : 0;
}

status_t 
BMediaRecorder::GetSourceAt(int32 index, BString *outName, media_format *outFormat) const
{
	if (_mInitErr < B_OK)
		return _mInitErr;
	if (_mConnected)
		return B_MEDIA_ALREADY_CONNECTED;
	if(!_mSources)
		return B_BAD_INDEX;
	
	return _mSources->GetItemAt(index, outName, outFormat);
}

status_t 
BMediaRecorder::ConnectSourceAt(int32 index, uint32 flags)
{
	if (_mInitErr < B_OK)
		return _mInitErr;
	if (_mConnected)
		return B_MEDIA_ALREADY_CONNECTED;
	if(!_mSources)
		return B_BAD_INDEX;

	source_list_item item;
	media_format format;	
	status_t err = _mSources->GetItemAt(index, 0, &format, &item);
	if(err < B_OK)
		return err;

	if(item.type != B_SLIT_DEVICE)
		return B_ERROR;
	
	// +++++ device selection goes here
	
	err = _open_stream();
	if(err == B_OK) {
		_mConnected = true;
		_start_recorder_thread();
	}
	return err;
}

status_t 
BMediaRecorder::Connect(const media_format &format, uint32 flags)
{
	if (_mInitErr < B_OK)
		return _mInitErr;
	if (_mConnected)
		return B_MEDIA_ALREADY_CONNECTED;

	status_t err = _open_stream();
	if(err < B_OK)
		return err;
		
	// format check
	if(!format_is_compatible(format, _mFormat)) {
		// whoops, no go.
		_close_stream();
		return B_MEDIA_BAD_FORMAT;
	}
	
	_mConnected = true;
	_start_recorder_thread();
	
	return B_OK;
}


status_t 
BMediaRecorder::Disconnect()
{
	if (_mInitErr < B_OK)
		return _mInitErr;
	if (!_mConnected)
		return B_MEDIA_NOT_CONNECTED;
	
	_stop_recorder_thread();	
	_close_stream();

	_mConnected = false;

	return B_OK;
}

bool 
BMediaRecorder::IsConnected() const
{
	return _mConnected;
}

status_t 
BMediaRecorder::Start(bool force)
{
	fprintf(stderr, "BMediaRecorder::Start()\n");
	BAutolock _l(_mLock);
	if(_mInitErr < B_OK)
		return _mInitErr;
	if(!_mConnected || _mStreamID < 0)
		return B_MEDIA_NOT_CONNECTED;
	if(_mRunning && !force)
		return EALREADY;
	if(_mRunSem < 0)
		return B_ERROR;
	
	fprintf(stderr, "sending start signal\n");
	// send start signal
	if(!atomic_or(&_mRun, 1))
		release_sem_etc(_mRunSem, 1, B_DO_NOT_RESCHEDULE);

	return B_OK;	
}

status_t 
BMediaRecorder::Stop(bool force)
{
	fprintf(stderr, "BMediaRecorder::Stop()\n");
	BAutolock _l(_mLock);
	if(_mInitErr < B_OK)
		return _mInitErr;
	if(!_mConnected || _mStreamID < 0) {
		fprintf(stderr, "stop: not connected\n");
		return B_MEDIA_NOT_CONNECTED;
	}
	if(!_mRunning && !force) {
		fprintf(stderr, "stop: not running\n");
		return EALREADY;
	}

	fprintf(stderr, "sending stop signal\n");
	atomic_and(&_mRun, ~1);

	return B_OK;
}

bool 
BMediaRecorder::IsRunning() const
{
	return _mRunning;
}

const media_format &
BMediaRecorder::Format() const
{
	return _mFormat;
}

status_t
BMediaRecorder::_open_stream() {
	BAutolock _l(_mLock);
	if(_mInitErr < B_OK) return _mInitErr;
	if(_mStreamID >= 0 || _mRunning) return B_NOT_ALLOWED;
	
	_mFormat.type = B_MEDIA_RAW_AUDIO;
	_mStreamID = mini_new_input_stream(&_mFormat.u.raw_audio);
	if(_mStreamID < B_OK)
		return _mStreamID;
	
	_mConnected = true;
	
	return B_OK;
}

status_t
BMediaRecorder::_close_stream() {
	BAutolock _l(_mLock);
	if(_mInitErr < B_OK) return _mInitErr;
	if(_mStreamID < 0 || _mRunning) return B_NOT_ALLOWED;
	
	status_t err = mini_close_input_stream(_mStreamID);
	_mStreamID = -1;
	_mConnected = false;
	return err;
}

status_t
BMediaRecorder::_start_recorder_thread() {
	BAutolock _l(_mLock);
	if(_mInitErr < B_OK)
		return _mInitErr;
	if(!_mConnected || _mStreamID < 0)
		return B_MEDIA_NOT_CONNECTED;
	if(_mThread >= 0)
		return EALREADY;
	
	_mRunSem = create_sem(0, "record trigger");
	if(_mRunSem < 0)
		return _mRunSem;

	_mThread = spawn_thread(_recorder_thread, "recorder_thread", _mThreadPriority, this);
	if(_mThread < 0) {
		delete_sem(_mRunSem);
		_mRunSem = -1;
		return _mThread;
	}
	
	resume_thread(_mThread);
	return B_OK;
}

status_t
BMediaRecorder::_stop_recorder_thread() {
	BAutolock _l(_mLock);
		
	if(_mInitErr < B_OK)
		return _mInitErr;
	if(!_mConnected || _mStreamID < 0)
		return B_MEDIA_NOT_CONNECTED;
	if(_mThread < 0)
		return B_NOT_ALLOWED;
		
	delete_sem(_mRunSem);
	_mRunSem = -1;
	Stop(true);
	
	status_t err;
	if (find_thread(0) != _mThread)
		while(wait_for_thread(_mThread, &err) == B_INTERRUPTED) {}
	_mThread = -1;

	return B_OK;
}

int32 
BMediaRecorder::_recorder_thread(void* arg) {
	BMediaRecorder* sp = (BMediaRecorder*)arg;
	
	fprintf(stderr, "_recorder_thread()\n");
	status_t err;
	
	// init dummy header
	media_header header;
	memset(&header, 0, sizeof(media_header));
	header.type = sp->_mFormat.type;
	
	sp->_mRunning = true;
	while(true) {
		if(!sp->_mRun) {
			fprintf(stderr, "_recorder_thread: stopped\n");
			sp->_mRunning = false;

			// wait for start signal
			err = acquire_sem(sp->_mRunSem);
			if(err == B_BAD_SEM_ID)
				// die
				break;
			
			if(err < 0) {
				fprintf(stderr, "_recorder_thread: acquire_sem(%ld): %s\n",
					sp->_mRunSem, strerror(err));
				snooze(1000LL);
				continue;
			}
			
			fprintf(stderr, "_recorder_thread: starting\n");
		}
		
		// fetch data
		size_t size;
		void* data;
		if(mini_acquire_input_buffer(sp->_mStreamID, &size, &data) < 0) {
			fprintf(stderr, "_recorder_thread: buffer read failed\n");
			break;
		}
		
		// handle buffer
		sp->BufferReceived(data, size, header);
	}

	return B_OK;
}


status_t BMediaRecorder::_Reserved_MediaRecorder_0(void*, ...) { return B_ERROR; }
status_t BMediaRecorder::_Reserved_MediaRecorder_1(void*, ...) { return B_ERROR; }
status_t BMediaRecorder::_Reserved_MediaRecorder_2(void*, ...) { return B_ERROR; }
status_t BMediaRecorder::_Reserved_MediaRecorder_3(void*, ...) { return B_ERROR; }
status_t BMediaRecorder::_Reserved_MediaRecorder_4(void*, ...) { return B_ERROR; }
status_t BMediaRecorder::_Reserved_MediaRecorder_5(void*, ...) { return B_ERROR; }
status_t BMediaRecorder::_Reserved_MediaRecorder_6(void*, ...) { return B_ERROR; }
status_t BMediaRecorder::_Reserved_MediaRecorder_7(void*, ...) { return B_ERROR; }

// END -- IADMediaRecorder.cpp --