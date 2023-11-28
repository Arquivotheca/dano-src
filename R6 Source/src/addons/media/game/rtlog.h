#if !defined(__RTLOG_H__)
#define __RTLOG_H__

#include <Autolock.h>
#include <Debug.h>
#include <Locker.h>
#include <OS.h>
#include <RealtimeAlloc.h>
#include <SupportDefs.h>
#include <String.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace BPrivate {

// realtime logging and profiling tools: provides more-or-less constant time
// logging of simple messages:
// - text (40 chars)
// - value1, value2 (32 bit)
// - time1, time2 (64 bit)

const uint32 RTLOG_MESSAGE_LENGTH = 40;

struct rtlog_entry // 64 bytes
{
	char		message[RTLOG_MESSAGE_LENGTH];
	bigtime_t	time1, time2;
	float		value1, value2;
};

class RTLogContext
{
public:
	RTLogContext(const char* name, const char* path, uint32 capacity) :
	_name(name),
	_capacity(capacity),
	_entries(0),
	_offset(0),
	_used(0),
	_run(false),
	_lock("RTLogContext/lock")
	{
		 ASSERT(_capacity);
		 _entries = (rtlog_entry*)rtm_alloc(0, _capacity * 2 * sizeof(rtlog_entry));
		 ASSERT(_entries);
		 
		 _file = fopen(path, "w");
		 if (!_file) return;
		 _thread = spawn_thread(&Entry, "RTLogThread", B_LOW_PRIORITY, this);
		 if (_thread < B_OK) return;
		 _port = create_port(2, "RTLogPort");
		 if (_port < B_OK) return;
		 resume_thread(_thread);
		 _run = true;
	}
		
	~RTLogContext()
	{
		if (_run)
		{
			_run = false;
			status_t err = write_port_etc(_port, -1, 0, 0, B_TIMEOUT, 0);
			if (err < B_OK)
			{
				fprintf(stderr, "RTLogContext::~RTLogContext(): write_port_etc(): %s\n", strerror(err));
			}
			thread_id tid = _thread;
			if (tid >= B_OK)
			{
				status_t r;
				while (wait_for_thread(tid, &r) == B_INTERRUPTED) {}
			}
		}
		if (_used) Write(_offset, _used);
		if (_port >= B_OK) delete_port(_port);
		if (_file) fclose(_file);
		if (_entries) rtm_free(_entries);
	}

	status_t Log(
		const char* message,
		float value1 = 0.0f,
		float value2 = 0.0f,
		bigtime_t time1 =0LL,
		bigtime_t time2 =0LL)
	{
		if (!_run) return B_ERROR;
		BAutolock _l(_lock);
		rtlog_entry* e = _entries + _offset + _used++;
		strncpy(e->message, message, RTLOG_MESSAGE_LENGTH);
		e->time1 = time1; e->time2 = time2;
		e->value1 = value1; e->value2 = value2;
		if (_used >= _capacity)
		{
			status_t err = write_port_etc(_port, _offset, 0, 0, B_TIMEOUT, 0);
			if (err < B_OK)
			{
				fprintf(stderr, "RTLogContext::Log(): write_port_etc(): %s\n", strerror(err));
			}
			_offset = _offset ? 0 : _capacity;
			_used = 0;
		}
		return B_OK;
	}
	
	status_t Flush()
	{
		if (!_run) return B_ERROR;
		BAutolock _l(_lock);
		Write(_offset, _used);
		_used = 0;
		return B_OK;
	}

private:
	void Write(uint32 offset, uint32 used)
	{
		FILE* f = (_file) ? _file : stderr;
		for (uint32 n = 0; n < used; n++)
		{
			fprintf(f, "%40s  %10g, %10g, %12Ld, %12Ld\n",
				_entries[offset + n].message,
				_entries[offset + n].value1,
				_entries[offset + n].value2,
				_entries[offset + n].time1,
				_entries[offset + n].time2);
		}
		fflush(f);
	}
	
	static status_t Entry(void* user) { ((RTLogContext*)user)->Run(); return B_OK; }

	void Run()
	{
		int32 data;
		status_t err;
		while (_run)
		{
			err = read_port(_port, &data, 0, 0);
			if (err < B_OK)
			{
				fprintf(stderr, "RTLogContext::Run(): read_port(): %s\n", strerror(err));
				_run = false;
				break;
			}
			if (data >= 0)
			{
				Write(data, _capacity);
			}
		}
	}

private:
	BString			_name;
	const uint32	_capacity;
	rtlog_entry*	_entries;
	uint32			_offset;
	uint32			_used;
	FILE*			_file;
	thread_id		_thread;
	port_id			_port;
	volatile bool	_run;
	BLocker			_lock;
};

}; // BPrivate
#endif //__RTLOG_H__
