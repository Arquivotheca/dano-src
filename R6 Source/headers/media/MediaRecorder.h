/*******************************************************************************
/
/ File:          MediaRecorder.h
/
/ Description:   Provides a simple interface for direct recording of any
/                supported media format.  Call Connect() or ConnectSourceAt() to
/                select a source and/or format.  Either call SetBufferHook()
/                with a pointer to a buffer-handling function or override
/                BufferReceived().
/
/	Copyright 2000, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_MEDIA_RECORDER_H)
#define _MEDIA_RECORDER_H

#include <MediaDefs.h>
#include <String.h>
#include <Locker.h>

#if _SUPPORTS_MEDIA_NODES
#include <MediaNode.h>
#endif

namespace BPrivate {
	class BMediaRecorderNode;
	class BSourceList;
};

class BMediaRecorder {
public:
	typedef void (*buffer_hook)(
		void* cookie,
		const void* data,
		size_t size,
		const media_header& header);

	virtual	~BMediaRecorder();
	BMediaRecorder(
		const char* name,
		int32 priority =0);

	status_t InitCheck();

	status_t SetBufferHook(
		buffer_hook hook,
		void* cookie =0);

	void SetCookie(
		void* cookie =0);

	virtual void BufferReceived(
		void* data,
		size_t size,
		const media_header& header);

	status_t FetchSources(
		const media_format& format,
		bool physicalSourcesOnly=true);

	int32 CountSources() const;

	status_t GetSourceAt( 
		int32 index,
		BString* outName,
		media_format* outFormat) const;

	status_t ConnectSourceAt( 
		int32 index, 
		uint32 flags =0); 

	status_t Connect(
		const media_format& format,
		uint32 flags =0);
									
#if _SUPPORTS_MEDIA_NODES

	status_t Connect(
		const dormant_node_info& info,
		const media_format* format =0,
		uint32 flags =0);
	
	status_t Connect(
		const media_node& node,
		const media_source* use_source =0,
		const media_format* format =0,
		uint32 flags =0);

	const media_node& Node() const;
	const media_node& SourceNode() const;

	const media_input& Input() const;
	const media_output& SourceOutput() const;

#endif // _SUPPORTS_MEDIA_NODES

	status_t Disconnect();
	bool IsConnected() const;
		
	status_t Start(
		bool force =false);
	status_t Stop(
		bool force =false);
	bool IsRunning() const;

	const media_format& Format() const;

private:

	virtual status_t _Reserved_MediaRecorder_0(void*, ...);
	virtual status_t _Reserved_MediaRecorder_1(void*, ...);
	virtual status_t _Reserved_MediaRecorder_2(void*, ...);
	virtual status_t _Reserved_MediaRecorder_3(void*, ...);
	virtual status_t _Reserved_MediaRecorder_4(void*, ...);
	virtual status_t _Reserved_MediaRecorder_5(void*, ...);
	virtual status_t _Reserved_MediaRecorder_6(void*, ...);
	virtual status_t _Reserved_MediaRecorder_7(void*, ...);

	//	unimplemented constructors
	BMediaRecorder();
	BMediaRecorder(const BMediaRecorder&);
	BMediaRecorder& operator=(const BMediaRecorder&);

#if _SUPPORTS_MEDIA_NODES
	friend class BPrivate::BMediaRecorderNode;

	BPrivate::BMediaRecorderNode* _mNode;

	media_node        _mSourceNode;
	media_output      _mSourceOutput;
	media_node        _mMyNode;
	media_input       _mMyNodeInput;
	BTimeSource*      _mTimeSource;
	bool              _mReleaseSourceNode;

	status_t giga_connect(
		const media_format* format,
		uint32 flags,
		const dormant_node_info* dormant,
		const media_node* node,
		const media_source* source);

#else	//_SUPPORTS_MEDIA_NODES

	BLocker           _mLock;
	media_format      _mFormat;
	int32             _mRun;
	sem_id            _mRunSem;
	thread_id         _mThread;
	int32             _mThreadPriority;
	int32             _mStreamID;
	
	status_t _open_stream();
	status_t _close_stream();
	
	status_t _start_recorder_thread();
	status_t _stop_recorder_thread();

	static status_t _recorder_thread(void* arg);

#endif	//_SUPPORTS_MEDIA_NODES

	status_t        _mInitErr;
	
	buffer_hook		_mBufferHook;
	void*           _mBufferCookie;
	bool            _mConnected;
	volatile bool   _mRunning;
	
	BPrivate::BSourceList*	_mSources;

	uint32          _mReserved[10];
};


#endif	//_MEDIA_RECORDER_H
